/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 *
 * Monitor (started from wireguard PostUp):
 *  - inotify on /etc         : resolv.conf / resolv.conf.wg changes
 *  - inotify on dir of -f    : changes to the wireguard resolv.conf source
 *                              (e.g. /etc/wg-client/wireguard-resolv.conf)
 *  - inotify on sys/class/net: interface (only works in testing mode - kernel
 *                              changes to sysfs generate no inotify events)
 *  - rtnetlink RTMGRP_LINK   : interface removal (wg-quick down) / creation
 *  - signalfd                : SIGTERM, SIGINT, SIGHUP, SIGQUIT
 *
 * Whatever the reason for exiting, the standard resolv.conf is restored.
 */
#include "resolv_manager.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <signal.h>     // sigset_t, pthread_sigmask, sigabbrev_np
#include <stdint.h>
#include <stdio.h>      // ssize_t (first declared by stdio.h via resolv_manager.h)
#include <string.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

enum {
    EVENT_BUF_LEN = 64 * 1024,
    NL_BUF_LEN = 32 * 1024,

    /*
     * Settle: act on resolv.conf changes once there has been no further
     * change for SETTLE_QUIET_MS - so a burst of writes (resume, wifi change,
     * several network tools) is handled once, on the final content, and we
     * never read while another process is still rewriting the file.
     * Never wait longer than SETTLE_MAX_MS from the first change.
     */
    SETTLE_QUIET_MS = 200,
    SETTLE_MAX_MS = 2000,
    MSEC_PER_SEC = 1000,
    NSEC_PER_MSEC = 1000000,
};

typedef enum {
    POLL_SIG = 0,
    POLL_INOTIFY = 1,
    POLL_NETLINK = 2,
    POLL_NUM = 3,
} PollIdx;

/*
 * inotify masks (unsigned - event->mask is uint32_t)
 */
static const uint32_t MASK_CHANGED = IN_CLOSE_WRITE | IN_MOVED_TO;
static const uint32_t MASK_GONE    = IN_DELETE | IN_MOVED_FROM;
static const uint32_t MASK_NEW     = IN_CREATE | IN_MOVED_TO;
static const uint32_t MASK_ETC     = IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE | IN_MOVED_FROM;
static const uint32_t MASK_SYS     = IN_CREATE | IN_MOVED_TO | IN_DELETE | IN_MOVED_FROM;

/*
 * poll() revents bits (unsigned)
 */
static const unsigned int REVENTS_IN  = POLLIN;
static const unsigned int REVENTS_BAD = POLLERR | POLLNVAL;

/*
 * Data needed for monitoring
 */
typedef struct {
    int     ifd;
    int     nlfd;
    int     sigfd;

    int     wd_etc;
    int     wd_sys;
    int     wd_src;

    sigset_t old_mask;
    bool    sigmask_set;

    bool    iface_present;

    /*
     * set by event handlers, acted on after each batch
     */
    bool    need_install;
    bool    need_src_save;
    bool    need_resync;
    bool    exit_now;
    bool    error;

    /*
     * settle timer for need_install (monotonic msec)
     */
    bool      install_pending;
    long long pending_since_ms;
    long long install_at_ms;

} Monitor;


/*
 * Interface exists ? (works for real sysfs and testing dir)
 */
bool iface_exists(Config *conf) {
    struct stat stbuf = {};

    if (caps_raise() != 0) {
        return false;
    }
    int ret = fstatat(conf->sys_net.fd, conf->iface.name, &stbuf, AT_SYMLINK_NOFOLLOW);
    (void)caps_lower();
    return ret == 0;
}

/*
 * File in etc exists ? (fstatat uses effective creds + caps; faccessat does not)
 */
static bool etc_file_exists(Config *conf, const char *name) {
    struct stat stbuf = {};

    if (caps_raise() != 0) {
        return false;
    }
    int ret = fstatat(conf->etc.fd, name, &stbuf, AT_SYMLINK_NOFOLLOW);
    (void)caps_lower();
    return ret == 0;
}

static long long now_ms() {
    struct timespec now = {};
    (void)clock_gettime(CLOCK_MONOTONIC, &now);
    return ((long long)now.tv_sec * MSEC_PER_SEC) + (now.tv_nsec / NSEC_PER_MSEC);
}

/*
 * A change was seen: (re)start the settle timer.
 */
static void schedule_install(Monitor *mon) {
    long long now = now_ms();

    if (!mon->install_pending) {
        mon->install_pending = true;
        mon->pending_since_ms = now;
    }

    long long quiet_at = now + SETTLE_QUIET_MS;
    long long max_at = mon->pending_since_ms + SETTLE_MAX_MS;
    mon->install_at_ms = (quiet_at < max_at) ? quiet_at : max_at;
}

/*
 * poll() timeout: until the pending install is due, else forever
 */
static int poll_timeout(const Monitor *mon) {
    if (!mon->install_pending) {
        return -1;
    }
    long long remaining = mon->install_at_ms - now_ms();
    if (remaining <= 0) {
        return 0;
    }
    return (int)remaining;
}

static bool revents_has(const struct pollfd *pfd, unsigned int bits) {
    return ((unsigned int)pfd->revents & bits) != 0U;
}

static void monitor_close(Monitor *mon) {
    if (mon->ifd >= 0) {
        (void)close(mon->ifd);
        mon->ifd = -1;
    }
    if (mon->nlfd >= 0) {
        (void)close(mon->nlfd);
        mon->nlfd = -1;
    }
    if (mon->sigfd >= 0) {
        (void)close(mon->sigfd);
        mon->sigfd = -1;
    }
    if (mon->sigmask_set) {
        (void)pthread_sigmask(SIG_SETMASK, &mon->old_mask, nullptr);
        mon->sigmask_set = false;
    }
}

/*
 * Block termination signals and receive them via signalfd instead
 * so we always restore the standard resolv.conf before exiting.
 */
static int setup_signals(Monitor *mon) {
    sigset_t mask;

    (void)sigemptyset(&mask);
    (void)sigaddset(&mask, SIGTERM);
    (void)sigaddset(&mask, SIGINT);
    (void)sigaddset(&mask, SIGHUP);
    (void)sigaddset(&mask, SIGQUIT);

    int err = pthread_sigmask(SIG_BLOCK, &mask, &mon->old_mask);
    if (err != 0) {
        errno = err;
        return -1;
    }
    mon->sigmask_set = true;

    mon->sigfd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    return (mon->sigfd < 0) ? -1 : 0;
}

/*
 * rtnetlink: link add/remove notifications
 */
static int setup_netlink(Monitor *mon) {
    struct sockaddr_nl addr = {};

    mon->nlfd = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK, NETLINK_ROUTE);
    if (mon->nlfd < 0) {
        return -1;
    }

    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_LINK;

    if (bind(mon->nlfd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        return -1;
    }
    return 0;
}

/*
 * inotify watches (fixed, trusted directories - caps raised)
 */
static int setup_inotify(Monitor *mon, Config *conf) {
    Options *opts = &conf->opts;

    mon->ifd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
    if (mon->ifd < 0) {
        log_msg(opts, " inotify_init1 failed: %s", errno_str(errno));
        return -1;
    }

    if (caps_raise() != 0) {
        return -1;
    }
    mon->wd_etc = inotify_add_watch(mon->ifd, conf->etc.path, MASK_ETC);
    int saved_errno = errno;

    /*
     * Only effective in testing mode (sysfs has no inotify events) - not fatal.
     */
    mon->wd_sys = inotify_add_watch(mon->ifd, conf->sys_net.path, MASK_SYS);
    (void)caps_lower();

    if (mon->wd_etc < 0) {
        log_msg(opts, " inotify watch %s failed: %s", conf->etc.path, errno_str(saved_errno));
        return -1;
    }

    /*
     * -f source directory (root provided - caps raised).
     * IN_MASK_ADD: it may be /etc itself (same watch).
     * Not fatal - only live updates of the source are lost.
     */
    if (conf->wg_src.path) {
        if (caps_raise() != 0) {
            return -1;
        }
        mon->wd_src = inotify_add_watch(mon->ifd, conf->wg_src.path, MASK_CHANGED | MASK_GONE | IN_MASK_ADD);
        saved_errno = errno;
        (void)caps_lower();

        if (mon->wd_src < 0) {
            log_msg(opts, " Warning: inotify watch %s failed: %s - changes to %s not applied until restart",
                    conf->wg_src.path, errno_str(saved_errno), conf->wg_src_name);
        }
    }
    return 0;
}

/*
 * Events for "etc"
 */
static void handle_etc_event(Monitor *mon, Config *conf, const struct inotify_event *event) {

    Options *opts = &conf->opts;

    if (strcmp(event->name, conf->resolv.name) == 0) {
        /*
         * resolv.conf written, replaced, removed or (sym)linked
         * IN_CREATE only for symlinks - a regular file is acted on at
         * IN_CLOSE_WRITE once the writer is done.
         */
        if ((event->mask & (MASK_CHANGED | MASK_GONE)) != 0U) {
            log_msg(opts, "  [etc] %s changed", conf->resolv.name);
            mon->need_install = true;

        } else if ((event->mask & IN_CREATE) != 0U && is_symlink_at(conf->etc.fd, conf->resolv.name) == 1) {
            log_msg(opts, "  [etc] %s symlink created", conf->resolv.name);
            mon->need_install = true;
        }
        return;
    }

    if (strcmp(event->name, conf->wg.name) == 0) {
        /*
         * Removed or renamed away: may be an editor saving it (rename the
         * original to a backup, then write the new file). Decided after the
         * settle delay: if it exists then, install it - else restore and exit.
         */
        if ((event->mask & MASK_GONE) != 0U) {
            log_msg(opts, "  [etc] %s removed or renamed - checking after settle", conf->wg.name);
            mon->need_install = true;

        } else if ((event->mask & MASK_CHANGED) != 0U) {
            log_msg(opts, "  [etc] %s changed", conf->wg.name);
            mon->need_install = true;
        }
    }
}

/*
 * Events for the -f source directory
 * Changed, replaced, removed or renamed: decided after the settle delay
 * (editors may rename the original away before writing the new file).
 */
static void handle_src_event(Monitor *mon, Config *conf, const struct inotify_event *event) {

    if (strcmp(event->name, conf->wg_src_name) != 0) {
        return;
    }

    if ((event->mask & (MASK_CHANGED | MASK_GONE)) != 0U) {
        log_msg(&conf->opts, "  [src] %s/%s changed", conf->wg_src.path, conf->wg_src_name);
        mon->need_src_save = true;
        mon->need_install = true;
    }
}

/*
 * Events for "sys/class/net" (testing mode only in practice)
 */
static void handle_sys_event(Monitor *mon, Config *conf, const struct inotify_event *event) {

    if (strcmp(event->name, conf->iface.name) != 0) {
        return;
    }

    if ((event->mask & MASK_GONE) != 0U) {
        log_msg(&conf->opts, "  [sysfs] Interface removed %s -> restore and exit", conf->iface.name);
        mon->iface_present = false;
        mon->exit_now = true;

    } else if ((event->mask & MASK_NEW) != 0U && !mon->iface_present) {
        log_msg(&conf->opts, "  [sysfs] Interface created %s", conf->iface.name);
        mon->iface_present = true;
        mon->need_install = true;
    }
}

/*
 * Dispatch one inotify event
 */
static void handle_inotify_event(Monitor *mon, Config *conf, const struct inotify_event *event) {

    if ((event->mask & IN_Q_OVERFLOW) != 0U) {
        log_msg(&conf->opts, " inotify queue overflow - resync");
        mon->need_resync = true;
        return;
    }

    /*
     * wd_src may equal wd_etc (-f file in /etc) - check both
     */
    if (mon->wd_src >= 0 && event->wd == mon->wd_src && event->len > 0) {
        handle_src_event(mon, conf, event);
    }

    if (event->wd == mon->wd_etc) {
        if ((event->mask & IN_IGNORED) != 0U) {
            log_msg(&conf->opts, " watch on %s removed - exit", conf->etc.path);
            mon->exit_now = true;
        } else if (event->len > 0) {
            handle_etc_event(mon, conf, event);
        }
        return;
    }

    if (mon->wd_sys >= 0 && event->wd == mon->wd_sys && event->len > 0) {
        handle_sys_event(mon, conf, event);
    }
}

/*
 * Drain inotify fd.
 */
static int read_inotify(Monitor *mon, Config *conf) {
    char buffer[EVENT_BUF_LEN] __attribute__((aligned(__alignof__(struct inotify_event))));

    for (;;) {
        ssize_t length = read(mon->ifd, buffer, sizeof(buffer));
        if (length < 0 && errno == EINTR) {
            continue;
        }
        if (length < 0) {
            if (errno == EAGAIN) {
                return 0;
            }
            log_msg(&conf->opts, " inotify read error: %s", errno_str(errno));
            return -1;
        }

        ssize_t offset = 0;
        while (offset < length) {
            const struct inotify_event *event = (const struct inotify_event *)&buffer[offset];
            handle_inotify_event(mon, conf, event);
            offset += (ssize_t)(sizeof(struct inotify_event) + event->len);
        }
    }
}

/*
 * Find IFLA_IFNAME in the attributes following ifinfomsg.
 * Bounds checked explicitly (the RTA_* macros mix signed/unsigned).
 */
static const char *find_ifname(const struct ifinfomsg *ifi, size_t remaining) {
    const char *ptr = (const char *)IFLA_RTA(ifi);

    while (remaining >= sizeof(struct rtattr)) {
        const struct rtattr *rta = (const struct rtattr *)(const void *)ptr;  // NOLINT(bugprone-casting-through-void)

        if (rta->rta_len < sizeof(struct rtattr) || rta->rta_len > remaining) {
            return nullptr;
        }

        if (rta->rta_type == IFLA_IFNAME) {
            size_t name_len = rta->rta_len - sizeof(struct rtattr);
            const char *data = (const char *)RTA_DATA(rta);
            if (name_len > 0 && name_len <= IFNAMSIZ && memchr(data, '\0', name_len) != nullptr) {
                return data;
            }
            return nullptr;
        }

        size_t step = RTA_ALIGN(rta->rta_len);
        if (step >= remaining) {
            return nullptr;
        }
        remaining -= step;
        ptr += step;
    }
    return nullptr;
}

/*
 * One netlink message
 */
static void handle_link_msg(Monitor *mon, Config *conf, const struct nlmsghdr *nlh) {

    if (nlh->nlmsg_type != RTM_NEWLINK && nlh->nlmsg_type != RTM_DELLINK) {
        return;
    }
    if (nlh->nlmsg_len < NLMSG_LENGTH(sizeof(struct ifinfomsg))) {
        return;
    }

    const struct ifinfomsg *ifi = NLMSG_DATA(nlh);
    const char *name = find_ifname(ifi, nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*ifi)));
    if (!name || strcmp(name, conf->iface.name) != 0) {
        return;
    }

    if (nlh->nlmsg_type == RTM_DELLINK) {
        log_msg(&conf->opts, "  [netlink] Interface removed %s -> restore and exit", conf->iface.name);
        mon->iface_present = false;
        mon->exit_now = true;

    } else if (!mon->iface_present) {
        log_msg(&conf->opts, "  [netlink] Interface created %s", conf->iface.name);
        mon->iface_present = true;
        mon->need_install = true;
    }
}

/*
 * Read one netlink datagram.
 * Returns 1 = got one, 0 = nothing more to read, -1 = error
 */
static int recv_netlink(Monitor *mon, Config *conf, char *buffer, size_t buf_size, size_t *len) {
    struct sockaddr_nl sender = {};
    socklen_t sender_len = sizeof(sender);

    ssize_t length = recvfrom(mon->nlfd, buffer, buf_size, 0, (struct sockaddr *)&sender, &sender_len);
    if (length >= 0) {
        /*
         * Only trust the kernel
         */
        *len = 0;
        if (sender_len == sizeof(sender) && sender.nl_pid == 0) {
            *len = (size_t)length;
        }
        return 1;
    }

    switch (errno) {
        case EINTR:
            *len = 0;
            return 1;

        case EAGAIN:
            return 0;

        case ENOBUFS:
            /* lost messages - recheck state */
            log_msg(&conf->opts, " netlink overrun - resync");
            mon->need_resync = true;
            *len = 0;
            return 1;

        default:
            log_msg(&conf->opts, " netlink read error: %s", errno_str(errno));
            return -1;
    }
}

/*
 * Drain netlink socket.
 */
static int read_netlink(Monitor *mon, Config *conf) {
    char buffer[NL_BUF_LEN] __attribute__((aligned(__alignof__(struct nlmsghdr))));
    size_t len = 0;
    int ret = 0;

    while ((ret = recv_netlink(mon, conf, buffer, sizeof(buffer), &len)) > 0) {
        for (const struct nlmsghdr *nlh = (const struct nlmsghdr *)buffer;
             NLMSG_OK(nlh, len); nlh = NLMSG_NEXT(nlh, len)) {
            handle_link_msg(mon, conf, nlh);
        }
    }
    return ret;
}

/*
 * Drain signalfd.
 */
static void read_signals(Monitor *mon, Config *conf) {
    struct signalfd_siginfo info = {};

    while (read(mon->sigfd, &info, sizeof(info)) == (ssize_t)sizeof(info)) {
        const char *name = sigabbrev_np((int)info.ssi_signo);
        log_msg(&conf->opts, "  [signal] received SIG%s -> restore and exit", name ? name : "?");
        mon->exit_now = true;
    }
}

/*
 * After overflow: re-check everything
 */
static void resync(Monitor *mon, Config *conf) {
    mon->need_resync = false;

    if (!etc_file_exists(conf, conf->wg.name)) {
        log_msg(&conf->opts, "  [resync] %s gone", conf->wg.name);
        mon->exit_now = true;
        return;
    }

    mon->iface_present = iface_exists(conf);
    if (!mon->iface_present) {
        log_msg(&conf->opts, "  [resync] interface %s gone", conf->iface.name);
        mon->exit_now = true;
        return;
    }

    mon->need_install = true;
    if (conf->wg_src.path) {
        mon->need_src_save = true;
    }
}

/*
 * Signals first - from there on SIGTERM etc. always lead to a restore.
 * Then watches - before installing - so no change can be missed.
 */
static int monitor_setup(Monitor *mon, Config *conf) {

    if (setup_signals(mon) != 0) {
        log_msg(&conf->opts, " signal setup failed: %s", errno_str(errno));
        return -1;
    }

    if (setup_netlink(mon) != 0) {
        log_msg(&conf->opts, " netlink setup failed: %s", errno_str(errno));
        return -1;
    }

    if (setup_inotify(mon, conf) != 0) {
        return -1;
    }

    mon->iface_present = iface_exists(conf);
    if (!mon->iface_present) {
        log_msg(&conf->opts, " Interface %s not present", conf->iface.name);
        mon->exit_now = true;
        return 0;
    }

    return install_wg_resolv(conf);
}

/*
 * Wait for and process one batch of events.
 */
static void monitor_wait(Monitor *mon, Config *conf, struct pollfd *pfds) {

    int ready = poll(pfds, POLL_NUM, poll_timeout(mon));
    if (ready < 0) {
        if (errno != EINTR) {
            log_msg(&conf->opts, " poll error: %s", errno_str(errno));
            mon->error = true;
            mon->exit_now = true;
        }
        return;
    }

    for (int i = 0; i < POLL_NUM; i++) {
        if (revents_has(&pfds[i], REVENTS_BAD)) {
            log_msg(&conf->opts, " poll: error on fd %d", pfds[i].fd);
            mon->error = true;
            mon->exit_now = true;
        }
    }

    if (revents_has(&pfds[POLL_SIG], REVENTS_IN)) {
        read_signals(mon, conf);
    }

    if (revents_has(&pfds[POLL_NETLINK], REVENTS_IN) && read_netlink(mon, conf) != 0) {
        mon->error = true;
        mon->exit_now = true;
    }

    if (revents_has(&pfds[POLL_INOTIFY], REVENTS_IN) && read_inotify(mon, conf) != 0) {
        mon->error = true;
        mon->exit_now = true;
    }

    /*
     * Changes seen in this batch restart the settle timer
     */
    if (mon->need_install) {
        mon->need_install = false;
        schedule_install(mon);
    }
}

/*
 * Act on what the last batch of events asked for.
 */
static void monitor_act(Monitor *mon, Config *conf) {

    if (mon->exit_now) {
        return;
    }

    if (mon->need_resync) {
        resync(mon, conf);
        if (mon->exit_now) {
            return;
        }
        if (mon->need_install) {
            mon->need_install = false;
            schedule_install(mon);
        }
    }

    if (mon->install_pending && now_ms() >= mon->install_at_ms) {
        mon->install_pending = false;

        /*
         * -f source changed: re-save it as resolv.conf.wg (tagged).
         * On failure (missing, empty) keep the current resolv.conf.wg.
         */
        if (mon->need_src_save) {
            mon->need_src_save = false;
            if (save_wg_resolv(conf) != 0) {
                log_msg(&conf->opts, "  Warning: keeping current %s", conf->wg.name);
            }
        }

        if (!etc_file_exists(conf, conf->wg.name)) {
            log_msg(&conf->opts, "  [etc] %s gone -> restore and exit", conf->wg.name);
            mon->exit_now = true;
            return;
        }

        if (install_wg_resolv(conf) != 0) {
            log_msg(&conf->opts, "  Warning: install_wg_resolv failed - will retry on next change");
        }
    }
}


/*
 * Main event monitor loop
 * - installs the wireguard resolv.conf
 * - keeps it installed
 * - restores the standard one on exit (always)
 *
 * Returns 0 if exited normally, -1 on error.
 */
int monitor_loop(Config *conf) {
    Monitor mon = {};

    mon.ifd = -1;
    mon.nlfd = -1;
    mon.sigfd = -1;
    mon.wd_etc = -1;
    mon.wd_sys = -1;
    mon.wd_src = -1;

    if (monitor_setup(&mon, conf) != 0) {
        mon.error = true;
        mon.exit_now = true;
    }

    struct pollfd pfds[POLL_NUM] = {};
    pfds[POLL_SIG].fd = mon.sigfd;
    pfds[POLL_INOTIFY].fd = mon.ifd;
    pfds[POLL_NETLINK].fd = mon.nlfd;
    for (int i = 0; i < POLL_NUM; i++) {
        pfds[i].events = POLLIN;
    }

    while (!mon.exit_now) {
        monitor_wait(&mon, conf, pfds);
        monitor_act(&mon, conf);
    }

    /*
     * Restore the standard resolv.conf
     */
    if (install_standard_resolv(conf) != 0) {
        mon.error = true;
    }
    monitor_close(&mon);

    return mon.error ? -1 : 0;
}
