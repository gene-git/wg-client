/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"

/*
 * Restores the system resolv.conf file
 * i.e copy: etc/resolv.conf.save -> etc/resolv_conf
 */
int install_standard_resolv(Config *conf) {

    if (!conf) {
        return -1;
    }
    CopyFile copy = {};

    log_msg(&conf->opts, "install_standard_resolv");

    /*
     * src / dst
     */
    copy.src_dir_fd = conf->etc.fd;
    copy.src_name = conf->save.name;

    copy.dst_dir_fd = conf->etc.fd;
    copy.dst_name = conf->resolv.name;

    /*
     * Only copy if a non-wireguard file
     */
    copy.tag_add = false;
    copy.tag_required = false;
    copy.tag_forbidden = true;
    copy.tag_ignore = false;
    copy.tag = conf->tag_wg;
    copy.tag_size = conf->tag_wg_size;

    if (copy_file_at(&copy) != 0) {
        log_msg(&conf->opts, "  error copy %s to %s", conf->save.name, conf->resolv.name);
        return -1;
    }

    /*
     * root should own destination (if possible)
     */
    (void)chown_root(conf, conf->etc.fd, conf->resolv.name);

    return 0;
}

/*
 * Save current stanard resolv to resolv.conf.saved
 * provided it is not tagged wireguard file.
 *
 * - /etc/resolv.conf -> /etc/resolv.conf.saved
 */
int save_standard_resolv(Config *conf) {
    if (!conf) {
        return -1;
    }

    log_msg(&conf->opts, "save_standard_resolv");

    CopyFile copy = {};

    copy.src_dir_fd = conf->etc.fd;
    copy.src_name = conf->resolv.name;

    copy.dst_dir_fd = conf->etc.fd;
    copy.dst_name = conf->save.name;

    copy.tag_required = false;
    copy.tag_forbidden = true;
    copy.tag_add = false;
    copy.tag_ignore = false;
    copy.tag = conf->tag_wg;
    copy.tag_size = conf->tag_wg_size;

    if (copy_file_at(&copy) != 0) {
        log_msg(&conf->opts, "  error copy %s to %s", conf->resolv.name, conf->save.name);
        return -1;
    }
    (void)chown_root(conf, conf->etc.fd, conf->save.name);

    return 0;
}

