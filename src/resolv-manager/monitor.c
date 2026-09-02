/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

#define EVENT_BUF_LEN (1024 * (sizeof(struct inotify_event) + 16))

/*
 * Data needed for managing inotify
 */
typedef struct {
    int     fd;
    int     wd_etc;
    int     wd_sys;
    int     wd_sem;

    uint32_t mask_etc;
    uint32_t mask_etc_mod;
    uint32_t mask_etc_del;

    uint32_t mask_sys;
    uint32_t mask_sys_new;
    uint32_t mask_sys_del;

    uint32_t mask_sem;

    const struct inotify_event *event;

    bool    exit_now;
    bool    action_triggered;    

} InotifyConf;


/*
 * Set up fresh inotify watches.
 * Close any existing. 
 */
static int inotify_setup(InotifyConf *iconf, Config *conf) {

    if (iconf->fd >= 0) {
        close(iconf->fd);
        iconf->fd = -1;
    }

    iconf->fd = inotify_init1(IN_CLOEXEC);
    if (iconf->fd < 0) {
        iconf->exit_now = true;
        return -1;
    }

    iconf->wd_etc = inotify_add_watch(iconf->fd, conf->etc.path, iconf->mask_etc);
    iconf->wd_sys = inotify_add_watch(iconf->fd, conf->sys_net.path, iconf->mask_sys);

    if (iconf->wd_etc < 0 || iconf->wd_sys < 0) {
        iconf->exit_now = true;
        return -1;
    }

    /*
     * Monitor semaphore for removal
     * There is a small race here between checking for file and setting up inotify?
     * How to make sure we dont miss file removal?
     * Should we switch to a signal handler?
     */
    if (conf->sem.path) {
        if (access(conf->opts.semaphore, F_OK) != 0) {
            log_msg(&conf->opts, " inotify setup: semaphore not found:i %s", conf->opts.semaphore);
            iconf->exit_now = true;
        } else {
            iconf->wd_sem = inotify_add_watch(iconf->fd, conf->sem.path, iconf->mask_sem);
            if (iconf->wd_sem < 0) {
                iconf->exit_now = true;
                return -1;
            }
        }
    }

    return 0;
}


/*
 * Handle events for "etc"
 */
static int handle_etc_event(InotifyConf *iconf, Config *conf) {

    Options *opts = &conf->opts;

    /*
     * Modified: resolv.conf resolv.conf.wg
     * - install_wg_resolv()
     */
    if ((iconf->event->mask & iconf->mask_etc_mod) != 0U) {

        if (strcmp(iconf->event->name, conf->resolv.name) == 0) {
            log_msg(opts, "  [etc] modified %s -> install_wg_resolv()", conf->resolv.name);
            install_wg_resolv(conf);
            iconf->action_triggered = true;
            return inotify_setup(iconf, conf);
        }
    } 

    /*
     * Deleted: resolv.conf.wg
     * - exit
     */
    if ((iconf->event->mask & iconf->mask_etc_del) != 0U) {
        bool do_install_standard = false;
        if (strcmp(iconf->event->name, conf->wg.name) == 0) {
            do_install_standard = true;
        }

        if (do_install_standard) {
            log_msg(opts, "  [etc] file deleted %s -> restore() and exit.\n", conf->wg.name);
            iconf->exit_now = true;
            iconf->action_triggered = true;
            return install_standard_resolv(conf);
        }
    }
    return 0;
}

/*
 * Handle events for "sys/net"
 */
static int handle_sys_event(InotifyConf *iconf, Config *conf) {

    Options *opts = &conf->opts;

    if (strcmp(iconf->event->name, conf->iface.name) == 0) {

        if ((iconf->event->mask & iconf->mask_sys_del) != 0U) {
            log_msg(opts, "  [sysfs] Interface removed %s -> install_standard_resolv() and exit.\n", conf->iface.name);
            iconf->exit_now = true;
            iconf->action_triggered = true;
            return install_standard_resolv(conf);
        }

        if ((iconf->event->mask & iconf->mask_sys_new ) != 0U) {
            log_msg(opts, "  [sysfs] Interface created %s -> install_wg_resolv()\n", conf->iface.name);
            install_wg_resolv(conf);
            iconf->action_triggered = true;
            return inotify_setup(iconf, conf);
        }
    }
    return 0;
}

/*
 * Handle events for "sem"
 */
static int handle_sem_event(InotifyConf *iconf, Config *conf) {

    Options *opts = &conf->opts;

    if (strcmp(iconf->event->name, conf->semaphore.name) == 0) {

        /*
         * Do we need to bother checking the mask - all
         * we care is if file is gone we shut down?
         */
        if ((iconf->event->mask & iconf->mask_sem) != 0U) {
            log_msg(opts, "  [sem] removed -> shutting down %s\n", opts->semaphore);
            iconf->exit_now = true;
            iconf->action_triggered = true;
            return 0;
        }
    }
    return 0;
}



/*
 * Main inotify event monitor loop
 */
int monitor_loop(Config *conf) {
    InotifyConf iconf = {};

    iconf.fd = -1;
    iconf.wd_etc = -1;
    iconf.wd_sys = -1;

    iconf.mask_etc_mod = IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_TO ; // | IN_CREATE ;
    iconf.mask_etc_del = IN_DELETE | IN_MOVED_FROM;
    iconf.mask_etc = iconf.mask_etc_mod | iconf.mask_etc_del;

    iconf.mask_sys_new = IN_CREATE | IN_MOVED_TO;
    iconf.mask_sys_del = IN_DELETE | IN_MOVED_FROM;
    iconf.mask_sys = iconf.mask_sys_new | iconf.mask_sys_del;

    iconf.mask_sem = IN_DELETE | IN_MOVED_FROM;

    if (inotify_setup(&iconf, conf) != 0) {
        return -1;
    }

    char buffer[EVENT_BUF_LEN];

    while (!iconf.exit_now) {
        ssize_t length = read(iconf.fd, buffer, EVENT_BUF_LEN);
        if (length < 0) {
            break;
        }

        ssize_t buffer_offset = 0;
        iconf.action_triggered = false;

        while (buffer_offset < length && !iconf.action_triggered) {

            iconf.event = (const struct inotify_event *)&buffer[buffer_offset];
            
            if (iconf.event->len > 0) {
                int status = 0;

                if (iconf.event->wd == iconf.wd_etc) {
                    status = handle_etc_event(&iconf, conf);

                } else if (iconf.event->wd == iconf.wd_sys) {
                    status = handle_sys_event(&iconf, conf);

                } else if (iconf.event->wd == iconf.wd_sem) {
                    status = handle_sem_event(&iconf, conf);
                }

                if (status != 0) {
                    iconf.action_triggered = true;
                    iconf.exit_now = true;
                    break;
                }
            }
            
            buffer_offset += (ssize_t)(sizeof(struct inotify_event) + iconf.event->len);
        }
    }

    if (iconf.fd >= 0) {
        close(iconf.fd);
        iconf.fd = -1;
    }

    /*
     * Restore the standard resolv.conf
     */
    return install_standard_resolv(conf);
    //return 0;
}

