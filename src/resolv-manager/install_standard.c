/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <errno.h>

/*
 * Where the file replaced by an atomic exchange is left for inspection
 */
static const char *const DISPLACED_STANDARD = ".displaced_restore";

/*
 * No saved standard resolv.conf to restore.
 * If the current resolv.conf is our (tagged) wireguard file, remove it:
 * it points at DNS servers reachable only through the (now down) tunnel.
 * Whatever manages the network (NetworkManager, dhcpcd, ...) will write a
 * fresh one; until then glibc falls back to localhost.
 */
static int handle_missing_saved(Config *conf) {
    Options *opts = &conf->opts;

    int tagged = file_has_tag_at(conf->etc.fd, conf->resolv.name, false,
                                 conf->tag_wg, conf->tag_wg_size);
    if (tagged != 1) {
        log_msg(opts, "  no %s to restore - leaving %s as is", conf->save.name, conf->resolv.name);
        return 0;
    }

    if (unlink_at_priv(conf->etc.fd, conf->resolv.name) != 0) {
        log_msg(opts, "  error removing wireguard %s: %s", conf->resolv.name, errno_str(errno));
        return -1;
    }
    log_msg(opts, "  Warning: no %s to restore - removed wireguard %s",
            conf->save.name, conf->resolv.name);
    return 0;
}

/*
 * Is name (in etc) a standard resolv.conf written by someone else ?
 *  - a symlink, or a non-empty regular file without our tag
 */
static bool is_foreign_standard(Config *conf, const char *name) {

    if (is_symlink_at(conf->etc.fd, name) == 1) {
        return true;
    }
    if (file_has_tag_at(conf->etc.fd, name, false, conf->tag_wg, conf->tag_wg_size) != 0) {
        return false;
    }
    return file_size_at(conf->etc.fd, name) > 0;
}

/*
 * After restoring with an exchange, the file we replaced is in DISPLACED_STANDARD.
 * Normally it is our wireguard file - remove it.
 * If instead someone else wrote a new standard resolv.conf just before we
 * restored, theirs is newer than our saved copy: put it back.
 */
static void handle_displaced_on_restore(Config *conf) {
    Options *opts = &conf->opts;

    if (is_foreign_standard(conf, DISPLACED_STANDARD)) {
        if (rename_at_priv(conf->etc.fd, DISPLACED_STANDARD, conf->resolv.name) == 0) {
            log_msg(opts, "  %s was rewritten by another process during restore - keeping theirs",
                    conf->resolv.name);
            return;
        }
        log_msg(opts, "  error keeping newer %s: %s", conf->resolv.name, errno_str(errno));
    }
    (void)unlink_at_priv(conf->etc.fd, DISPLACED_STANDARD);
}

/*
 * Restores the system resolv.conf file
 * i.e copy: etc/resolv.conf.saved -> etc/resolv.conf
 * - if the saved one is a symlink, the symlink is restored.
 */
int install_standard_resolv(Config *conf) {

    if (!conf) {
        return -1;
    }
    Options *opts = &conf->opts;

    log_msg(opts, "install_standard_resolv");

    int is_link = is_symlink_at(conf->etc.fd, conf->save.name);
    if (is_link < 0) {
        log_msg(opts, "  error checking %s: %s", conf->save.name, errno_str(errno));
        return -1;
    }

    CopyResult result = COPY_ERROR;
    CopyFile copy = {};

    if (is_link == 1) {
        result = copy_symlink_at(conf->etc.fd, conf->save.name, conf->etc.fd, conf->resolv.name);

    } else if (file_has_tag_at(conf->etc.fd, conf->save.name, false, conf->tag_wg, conf->tag_wg_size) == 1) {
        result = COPY_SKIPPED_TAG;

    } else if (files_identical_at(conf->etc.fd, conf->save.name, conf->resolv.name) == 1) {
        result = COPY_UNCHANGED;

    } else {
        /*
         * Only copy if a non-wireguard file
         */
        copy.src_dir_fd = conf->etc.fd;
        copy.src_name = conf->save.name;

        copy.dst_dir_fd = conf->etc.fd;
        copy.dst_name = conf->resolv.name;

        copy.tag_add = false;
        copy.tag_required = false;
        copy.tag_forbidden = true;
        copy.tag_ignore = false;
        copy.tag = conf->tag_wg;
        copy.tag_size = conf->tag_wg_size;
        copy.forbid_empty = true;
        copy.displaced_name = DISPLACED_STANDARD;

        result = copy_file_at(&copy);
    }

    if (copy.displaced) {
        handle_displaced_on_restore(conf);
    }

    switch (result) {
        case COPY_DONE:
            log_msg(opts, "  restored %s", conf->resolv.name);
            return 0;

        case COPY_UNCHANGED:
            log_msg(opts, "  %s already standard", conf->resolv.name);
            return 0;

        case COPY_NO_SOURCE:
        case COPY_EMPTY_SOURCE:
            return handle_missing_saved(conf);

        case COPY_SKIPPED_TAG:
            log_msg(opts, "  error: %s is a wireguard (tagged) file - not restoring", conf->save.name);
            return -1;

        case COPY_ERROR:
        default:
            log_msg(opts, "  error copy %s to %s: %s", conf->save.name, conf->resolv.name, errno_str(errno));
            return -1;
    }
}

/*
 * Save etc/<src_name> as the standard resolv.conf (resolv.conf.saved)
 * provided it is not a tagged wireguard file and not empty.
 *
 * - if src_name is a symlink, the symlink itself is saved
 *   (unless it leads to a tagged wireguard file)
 * - a missing or empty source is not an error (nothing to save)
 */
int save_standard_from(Config *conf, const char *src_name) {
    if (!conf || !src_name) {
        return -1;
    }
    Options *opts = &conf->opts;

    int is_link = is_symlink_at(conf->etc.fd, src_name);
    if (is_link < 0) {
        log_msg(opts, "  error checking %s: %s", src_name, errno_str(errno));
        return -1;
    }

    CopyResult result = COPY_ERROR;

    if (is_link == 1) {
        if (file_has_tag_at(conf->etc.fd, src_name, true, conf->tag_wg, conf->tag_wg_size) == 1) {
            result = COPY_SKIPPED_TAG;
        } else {
            result = copy_symlink_at(conf->etc.fd, src_name, conf->etc.fd, conf->save.name);
        }

    } else if (files_identical_at(conf->etc.fd, src_name, conf->save.name) == 1) {
        result = COPY_UNCHANGED;

    } else {
        CopyFile copy = {};

        copy.src_dir_fd = conf->etc.fd;
        copy.src_name = src_name;

        copy.dst_dir_fd = conf->etc.fd;
        copy.dst_name = conf->save.name;

        copy.tag_required = false;
        copy.tag_forbidden = true;
        copy.tag_add = false;
        copy.tag_ignore = false;
        copy.tag = conf->tag_wg;
        copy.tag_size = conf->tag_wg_size;
        copy.forbid_empty = true;

        result = copy_file_at(&copy);
    }

    switch (result) {
        case COPY_DONE:
            log_msg(opts, "  saved %s to %s", src_name, conf->save.name);
            return 0;

        case COPY_UNCHANGED:
        case COPY_SKIPPED_TAG:
            /* already saved, or it is the wireguard one */
            return 0;

        case COPY_NO_SOURCE:
            log_msg(opts, "  no %s to save", src_name);
            return 0;

        case COPY_EMPTY_SOURCE:
            log_msg(opts, "  %s is empty - not saved (keeping previous %s)", src_name, conf->save.name);
            return 0;

        case COPY_ERROR:
        default:
            log_msg(opts, "  error copy %s to %s: %s", src_name, conf->save.name, errno_str(errno));
            return -1;
    }
}

/*
 * Save current standard resolv to resolv.conf.saved
 * - /etc/resolv.conf -> /etc/resolv.conf.saved
 */
int save_standard_resolv(Config *conf) {
    if (!conf) {
        return -1;
    }
    log_msg(&conf->opts, "save_standard_resolv");
    return save_standard_from(conf, conf->resolv.name);
}
