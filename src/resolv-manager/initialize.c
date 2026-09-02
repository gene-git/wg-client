/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <fcntl.h>
#include <libgen.h> // Required for dirname() and basename()
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>


/*
 * Directories /etc, /sys/class/net
 * - test mode prepend with "./"
 */
static bool init_standard_dirs(Config *conf) {
    Options *opts = &conf->opts;
    char buf[PATH_MAX] = {};
    char *ptr = nullptr;
    char *sys_net_path = nullptr;
    char *etc_path = nullptr;

    /*
     * In test mode prepend with "./"
     */
    if (opts->testing) {
        sys_net_path = "./sys/class/net";
        etc_path = "./etc";

    } else {
        sys_net_path = "/sys/class/net";
        etc_path = "/etc";
    }

    /*
     * /sys/class/net
     */
    if (!realpath(sys_net_path, buf)) {
        return false;
    }
    
    ptr = strdup(buf);
    if (!ptr) {
        return false;
    }
    conf->sys_net.path = ptr;

    /*
     * /etc
     */
    if (!realpath(etc_path, buf)) {
        return false;
    } 

    ptr = strdup(buf);
    if (!ptr) {
        return false;
    }
    conf->etc.path = ptr;

    return true;
}

/*
 * Semaphore file to watch
 */
static bool init_semaphore(Config *conf) {
    Options *opts = &conf->opts;
    char dir[PATH_MAX] = {};
    char *ptr = nullptr;
    char *dir_path = nullptr;
    char *base_path = nullptr;
    bool status = true;

    if (!opts->semaphore) {
        return true;
    }

    /*
     * Separate dir and file
     */
    dir_path = strdup(opts->semaphore);
    base_path = strdup(opts->semaphore);
    if (!dir_path || !base_path) {
        status = false;
        goto cleanup;
    }

    char *dir_raw = dirname(dir_path);
    char *base = basename(base_path);

    if (!realpath(dir_raw, dir)) {
        status = false;
        goto cleanup;
    }

    /*
     * We're only going to watch this not open it.
     */
    conf->sem.fd = -1;

    ptr = strdup(dir);
    if (!ptr) {
        goto cleanup;
    }
    conf->sem.path = ptr;

    ptr = strdup(base);
    if (!ptr) {
        goto cleanup;
    }
    conf->semaphore.name = ptr;

cleanup:
    if (dir_path) {
        free((void *)dir_path);
    }
    if (base_path) {
        free((void *)base_path);
    }

    return status;
}


static void log_welcome_message(Config *conf) {
    Options *opts = &conf->opts;

    const char *iface_name = opts->iface_name ? opts->iface_name : " - ";
    const char *daemon = (int)(opts->daemon) ? "yes" : "no";

    const char *task = nullptr;

    switch (opts->task) {
        case INSTALL_WG:
            task = "Install WG";
            break;

        case INSTALL_STANDARD:
            task = "Install Standard";
            break;

        case MONITOR:
            task = "Monitor";
            break;

        default:
            task = " - ";
            break;
    }

    log_msg(opts, "--------------");
    log_msg(opts, "resolv-manager initialized");
    log_msg(opts, "  %15s : %s", "iface-name", iface_name);
    log_msg(opts, "  %15s : %s", "task", task);
    log_msg(opts, "  %15s : %s", "daemon", daemon);
    log_msg(opts, "  %15s : %s:%s", "real", conf->user_real.user, conf->user_real.group);
    log_msg(opts, "  %15s : %s:%s", "effective", conf->user_effective.user, conf->user_effective.group);
}


/*
 * Initialize options and config. 
 */
bool initialize(int argc, char **argv, Config *conf) {

    if (!conf) {
        return false;
    }

    /*
     * command line
     */
    if (parse_options(argc, argv, &conf->opts) != 0) {
        return false;
    }

    /*
     * Directories /etc, /sys/class/net
     */
    if (!init_standard_dirs(conf)) {
        return false;
    }

    if (conf->opts.semaphore) {
        if (!init_semaphore(conf)) {
            return false;
        }
    }

    /*
     * static files names (not freed by config_clean()
     */
    if (conf->opts.iface_name) {
        conf->iface.name = conf->opts.iface_name;
    } else {
        conf->iface.name = "wg0";
    }

    conf->resolv.name = "resolv.conf";
    conf->wg.name = "resolv.conf.wg";
    conf->save.name = "resolv.conf.saved";

    conf->sys_net.fd = -1;
    conf->etc.fd = -1;

    conf->perms.user_run = "nobody";
    conf->perms.group_run = "nobody";

    /*
     * File tag to mark wireguard resolv.conf
     */
    conf->tag_wg = "# === TAG: wireguard ===\n";
    conf->tag_wg_size = (size_t)strlen(conf->tag_wg);

    /*
     * Get real/effectve user info
     */
    (void)get_user_info(&conf->user_real, &conf->user_effective);

    /*
     * Set up logging
     * Ok if fails (likely permission problem)
     */
    log_init(&conf->opts);
    log_welcome_message(conf);

    return true;
}

