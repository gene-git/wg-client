/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>


/*
 * Open directories
 */
static bool open_directories(Config *conf) {

    conf->sys_net.fd = open(conf->sys_net.path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (conf->sys_net.fd < 0) {
        return false;
    }

    conf->etc.fd = open(conf->etc.path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (conf->etc.fd < 0) {
        (void)close(conf->sys_net.fd);
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

    if (!conf->iface.name) {
        log_msg(&conf->opts, " Missing Wireguard interface name");
        return false;
    }

    if (!conf->wg.name) {
        log_msg(&conf->opts, " Missing Wireguard resolv file (resolv.conf.wg)");
        return false;
    }

    if (faccessat(conf->sys_net.fd, conf->iface.name, F_OK, 0) != 0) {
        log_msg(&conf->opts, " Wireguard interface not available: %s", conf->iface.name);
        return false;
    }

    if (faccessat(conf->etc.fd, conf->wg.name, F_OK, 0) != 0) {
        log_msg(&conf->opts, " Wireguard resolv file not available: %s", conf->wg.name);
        return false;
    }
    return true;
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
            if (install_wg_resolv(conf) == 0) {
                ret = monitor_loop(conf);
            } else {
                ret = EX_SOFTWARE;
            }
            break;

        default:
            break;
    }
    return ret;
}

/*
 * 3 modes:
 *  - monitor (the default)
 *      - install_wg_resolv()
 *      - wait for any file changes 
 *        - if resolv.conf changes then do install_wg_resolv()
 *        - if resolv.conf.wg changes then do install_wg_resolv()
 *        - if sys/net/wg0 goes away (wireguard stopped) - restore() then exit
 *
 *  - standard
 *      - copy resolv.conf.save to resolv.conf
 *
 *  - wireguard
 *      - copy resolv.conf to resolv.conf.save
 *      - copy resolv.conf.wg to resolv.conf
 *
 * Returns from sysexits.h:
 *      EX_OK = successful termination (including already running)
 *      EX_USAGE = options
 *      EX_OSFILE = file errors
 *      EX_TEMPFAIL = not ready - wireguard is not running. - try again
 *      EX_OSERR = failure to daemonize or drop privs.
 *      EX_IOERR = typically read/write on /etc/resolv.conf et al
 *      EX_SOFTWARE = some other problem - mostly likely the monitor daemon encoutered 
 *                    some unrecoverable issue.
 */
int main(int argc, char **argv) {
    int result = EX_OK;
    Config conf = {}; 
    Locking locking = {};

    if (!initialize(argc, argv, &conf)) {
        result = EX_USAGE;
        goto cleanup;
    }

    /*
     * Make sure no other executable is running
     */
    if (!initialize_lock(&conf, &locking)) {
        result = EX_USAGE;
        goto cleanup;
    }

    /*
     * If semaphore provided and does not exist - exit.
     */
    if (conf.opts.semaphore && access (conf.opts.semaphore, F_OK) != 0) {
        log_msg(&conf.opts, "Semaphore not found - all done: %s", conf.opts.semaphore);
        goto cleanup;
    }

    /*
     * Open directories
     */
    if (!open_directories(&conf)) {
        result = EX_OSFILE;
        goto cleanup;
    }

    /*
     * Save any provided wireguar resolv.conf file
     */
    if (conf.opts.file && save_wg_resolv(&conf, conf.opts.file) != 0) {
        result = EX_IOERR;
        goto cleanup;
    }

    /*
     * Check wireguard up and provided resolv.wg file
     * - not needed if simply restoring standard resolv.
     */
    if (conf.opts.task != INSTALL_STANDARD && !check_wireguard_ready(&conf)) {
        result = EX_TEMPFAIL;
        goto cleanup;
    }

    /*
     * Daemonize
     */
    if (conf.opts.daemon) {
        log_msg(&conf.opts, " daemon mode");

        result = daemonize();
        if (result != EX_OK) {
            goto cleanup;
        }
    }

    /*
     * Drop privs if root but keep caps.
     * - we'll check caps after we drop privs
     */
    if (change_privileges(&conf) != 0) {
        result = EX_OSERR;
        goto cleanup;
    }

    check_permissions(&conf.perms);
    if (!conf.perms.cap_dac_override) {
        log_msg(&conf.opts, "Warning : missing CAP_DAC_OVERRIDE capability");
        log_msg(&conf.opts, "        : this is okay in test mode");
    }

    /*
     * tasks
     */
    result = do_tasks(&conf);

cleanup:
    release_lock(&locking);

    if (result == EX_OK) {
        log_msg(&conf.opts, "Done all good");
    } else {
        log_msg(&conf.opts, "Done with warnings or errors");
    }

    config_clean(&conf);

    return result;
}

