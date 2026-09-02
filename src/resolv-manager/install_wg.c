/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */

#include "resolv_manager.h"
#include <fcntl.h>
#include <libgen.h> // Required for dirname() and basename()
#include <limits.h>
#include <stdlib.h>
#include <string.h>


/*
 * Copy the file : src_file to etc/resolv.conf.wg
 * - this is a wireguard file so tag it.
 */
int save_wg_resolv(Config *conf, const char *src_file) {
    int fd_dir = -1;
    int fd_dir_use = -1;
    int status = 0;
    char *dir_path = nullptr;
    char *base_path = nullptr;
    char dir[PATH_MAX] = {};

    if (!src_file) {
        return 0;
    }

    log_msg(&conf->opts, " Saving new wg resolv file from %s\n", src_file);

    dir_path = strdup(src_file);
    base_path = strdup(src_file);
    if (!dir_path || !base_path) {
        status = -1;
        goto cleanup;
    }

    char *dir_raw = dirname(dir_path);
    char *base = basename(base_path);

    if (!realpath(dir_raw, dir)) {
        status = -1;
        goto cleanup;
    }

    bool same_dir = false;
    bool same_file = false;

    if (strcmp(dir, "/etc") == 0) {
        same_dir = true;
    } 
    if (strcmp(base, conf->wg.name) == 0) {
        same_file = true;
    }

    if (same_dir && same_file) {
        goto cleanup; 
    }

    if (same_dir) {
        fd_dir_use = conf->etc.fd;

    } else {
        fd_dir = open(dir, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (fd_dir < 0) {
            log_msg(&conf->opts, "Error opening directory %s from %s", dir, src_file);
            status = -1;
            goto cleanup;
        }
        fd_dir_use = fd_dir;
    }

    CopyFile copy = {};

    copy.src_dir_fd = fd_dir_use;
    copy.src_name = (const char *)base;

    copy.dst_dir_fd = conf->etc.fd;
    copy.dst_name = conf->wg.name;

    copy.tag_ignore = false;
    copy.tag_forbidden = false;
    copy.tag_required = false;
    copy.tag_add = true;
    copy.tag = conf->tag_wg;
    copy.tag_size = conf->tag_wg_size;

    if (copy_file_at(&copy) != 0) {
        log_msg(&conf->opts, "  error copy %s to %s", src_file, conf->wg.name);
        status = -1;
        goto cleanup;
    }
    (void)chown_root(conf, conf->etc.fd, conf->wg.name);
    log_msg(&conf->opts, " Saved %s \n", conf->wg.name);

cleanup:
    if (dir_path) {
        free(dir_path);
    }
    if (base_path) {
        free(base_path);
    }

    if (fd_dir >= 0) {
        (void)close(fd_dir);
    }

    return status;
}

/*
 * Make sure the wireguard resolv.conf is used.
 *
 *  copy:   etc/resolv.conf -> etc/resolv_conf.save
 *  copy:   etc/resolv_conf_wg -> etc/resolv.conf
 */
int install_wg_resolv(Config *conf) {

    if (!conf) {
        return -1;
    }

    log_msg(&conf->opts, "install_wg_resolv");

    /*
     * Save resolv.conf -> resolv.conf.save
     * provided its not a tagged wireguard resolv for example set up by post-up.sh
     */
    if (save_standard_resolv(conf) != 0) {
        return -1;
    }

    /*
     * resaolv.conf.wg -> resolv.conf
     * - really prefer resolv.conf.wg is tagged but for backward compat
     *   we allow it untagged - for now - maybe
     */
    CopyFile copy = {};

    copy.src_dir_fd = conf->etc.fd;
    copy.src_name = conf->wg.name;

    copy.dst_dir_fd = conf->etc.fd;
    copy.dst_name = conf->resolv.name;

    copy.tag_ignore = false;
    copy.tag_forbidden = false;
    copy.tag_required = true;
    copy.tag_add = true;

    copy.tag = conf->tag_wg;
    copy.tag_size = conf->tag_wg_size;

    if (copy_file_at(&copy) != 0) {
        log_msg(&conf->opts, "  error copy %s to %s", conf->wg.name, conf->resolv.name);
        return -1;
    }
    (void)chown_root(conf, conf->etc.fd, conf->resolv.name);
    
    return 0;
}

