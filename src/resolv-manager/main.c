/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>


/*
 * Open directories
 * On failure, any fd opened is left in conf for config_clean() to close.
 */
static bool open_directories(Config *conf) {
    Options *opts = &conf->opts;

    conf->sys_net.fd = open(conf->sys_net.path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (conf->sys_net.fd < 0) {
        log_msg(opts, " Error opening %s: %s", conf->sys_net.path, errno_str(errno));
        return false;
    }

    conf->etc.fd = open(conf->etc.path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (conf->etc.fd < 0) {
        log_msg(opts, " Error opening %s: %s", conf->etc.path, errno_str(errno));
        return false;
    }

    return true;
}

/*
* Check wireguard up and provided resolv.wg file
* Note - if only restoring standard config - then
*        then wireguard inerface is not required.
*/
static bool check_wireguard_ready(Config *conf) {

    if (!iface_exists(conf)) {
        log_msg(&conf->opts, " Wireguard interface not available: %s", conf->iface.name);
        return false;
    }

    if (faccessat(conf->etc.fd, conf->wg.name, F_OK, AT_SYMLINK_NOFOLLOW) != 0) {
        log_msg(&conf->opts, " Wireguard resolv file not available: %s", conf->wg.name);
        return false;
    }
    return true;
}

/*
 * Must be root - run from wireguard PostUp/PostDown.
 * Testing mode may be run as a normal user: all capabilities are dropped.
 */
static int check_root(Config *conf) {

    if (getuid() == 0 && geteuid() == 0) {
        return EX_OK;
    }

    if (conf->opts.testing) {
        if (caps_drop_all() != 0) {
            return EX_OSERR;
        }
        log_msg(&conf->opts, " testing mode: all capabilities dropped");
        return EX_OK;
    }

    log_msg(&conf->opts, "Error: must be run as root (wireguard PostUp / PostDown)");
    (void)fprintf(stderr, "Error: must be run as root (wireguard PostUp / PostDown)\n");
    return EX_NOPERM;
}

static int do_tasks(Config *conf) {
    /*
     * tasks
     */
    int ret = EX_OK;

    switch (conf->opts.task) {
        case INSTALL_WG:
            if (install_wg_resolv(conf) != 0) {
                ret = EX_IOERR;
            }
            break;

        case INSTALL_STANDARD:
            if (install_standard_resolv(conf) != 0) {
                ret = EX_IOERR;
            }
            break;

        case MONITOR:
            /*
             * monitor_loop() installs the wireguard resolv.conf itself
             * (after its watches are in place) and always restores the
             * standard one before returning.
             */
            if (monitor_loop(conf) != 0) {
                ret = EX_SOFTWARE;
            }
            break;

        case NOTHING:
        default:
            ret = EX_USAGE;
            break;
    }
    return ret;
}

/*
 * Everything up to the point of doing the work - run as root.
 * Errors here are reported to the caller (wg-quick) via exit code.
 */
static int prepare(Config *conf, Locking *locking) {

    int result = check_root(conf);
    if (result != EX_OK) {
        return result;
    }

    /*
     * Make sure no other executable is running
     */
    if (!initialize_lock(conf, locking)) {
        return locking->locked_by_other ? EX_TEMPFAIL : EX_OSERR;
    }

    if (!open_directories(conf)) {
        return EX_OSFILE;
    }

    /*
     * Save any provided wireguard resolv.conf file
     */
    if (conf->opts.file && save_wg_resolv(conf, conf->opts.file) != 0) {
        return EX_IOERR;
    }

    /*
     * Check wireguard up and provided resolv.wg file
     * - not needed if simply restoring standard resolv.
     */
    if (conf->opts.task != INSTALL_STANDARD && !check_wireguard_ready(conf)) {
        return EX_TEMPFAIL;
    }
    return EX_OK;
}

/*
 * Daemonize (monitor), drop privileges and do the work.
 */
static int run(Config *conf) {

    if (conf->opts.daemon) {
        log_msg(&conf->opts, " daemon mode");

        int result = daemonize();
        if (result != EX_OK) {
            return result;
        }
    }

    /*
     * Drop privs if root but keep the 2 caps we need.
     */
    if (change_privileges(conf) != 0) {
        return EX_OSERR;
    }

    if (check_permissions(&conf->perms) != 0) {
        log_msg(&conf->opts, "Warning : unable to read capabilities");
    }
    if (!conf->perms.cap_dac_override || !conf->perms.cap_chown) {
        log_msg(&conf->opts, "Warning : missing CAP_DAC_OVERRIDE and/or CAP_CHOWN capability");
        log_msg(&conf->opts, "        : this is okay in test mode");
    }

    return do_tasks(conf);
}

/*
 * 3 tasks:
 *  - monitor  (started from PostUp as a daemon)
 *      - install_wg_resolv()
 *      - wait for any changes
 *        - if resolv.conf changes then do install_wg_resolv()
 *        - if resolv.conf.wg changes then do install_wg_resolv()
 *        - if resolv.conf.wg removed, interface removed (wg-quick down),
 *          or SIGTERM/SIGINT/SIGHUP/SIGQUIT - install_standard_resolv() then exit
 *
 *  - standard  (PostDown - safety net, monitor normally already restored)
 *      - copy resolv.conf.saved to resolv.conf
 *
 *  - wireguard  (PostUp, with -f)
 *      - copy resolv.conf to resolv.conf.saved
 *      - copy resolv.conf.wg to resolv.conf
 *
 * Returns from sysexits.h:
 *      EX_OK = successful termination (including -h)
 *      EX_USAGE = options
 *      EX_NOPERM = not run as root (and not testing mode)
 *      EX_OSFILE = file errors
 *      EX_TEMPFAIL = not ready - wireguard is not running, or lock held by another - try again
 *      EX_OSERR = failure to daemonize, drop privs, or acquire lock
 *      EX_IOERR = typically read/write on /etc/resolv.conf et al
 *      EX_SOFTWARE = some other problem - mostly likely the monitor daemon encoutered
 *                    some unrecoverable issue.
 */
int main(int argc, char **argv) {
    Config conf = {};
    Locking locking = {};

    locking.fd = -1;
    locking.dir_fd = -1;

    int init = initialize(argc, argv, &conf);
    if (init != 0) {
        config_clean(&conf);
        return (init > 0) ? EX_OK : EX_USAGE;
    }

    int result = prepare(&conf, &locking);
    if (result == EX_OK) {
        result = run(&conf);
    }

    release_lock(&locking);

    if (result == EX_OK) {
        log_msg(&conf.opts, "Done all good");
    } else {
        log_msg(&conf.opts, "Done with warnings or errors (%d)", result);
    }

    config_clean(&conf);

    return result;
}
