/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */

#include "resolv_manager.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


/*
 * Split path into directory and file name (no dirname()/basename():
 * they may modify their argument and are not thread safe).
 *   "/a/b/c" -> "/a/b", "c"     "c" -> ".", "c"     "/c" -> "/", "c"
 * Returns false if there is no file name (empty, trailing '/', ".", "..").
 * *dir is malloc'd (caller frees); *base points into path.
 */
static bool split_path(const char *path, char **dir, const char **base) {
    const char *slash = strrchr(path, '/');

    *dir = nullptr;
    *base = slash ? slash + 1 : path;

    if (**base == '\0' || strcmp(*base, ".") == 0 || strcmp(*base, "..") == 0) {
        return false;
    }

    if (!slash) {
        *dir = strdup(".");
    } else if (slash == path) {
        *dir = strdup("/");
    } else {
        *dir = strndup(path, (size_t)(slash - path));
    }
    return *dir != nullptr;
}

/*
 * Is dir_fd/name the same file as etc/resolv.conf.wg ?
 */
static bool is_wg_file(Config *conf, int dir_fd, const char *name) {
    struct stat st_src = {};
    struct stat st_wg = {};

    if (fstatat(dir_fd, name, &st_src, 0) != 0 || fstatat(conf->etc.fd, conf->wg.name, &st_wg, 0) != 0) {
        return false;
    }
    if (st_src.st_dev != st_wg.st_dev || st_src.st_ino != st_wg.st_ino) {
        return false;
    }
    return true;
}

/*
 * Copy the file : src_file to etc/resolv.conf.wg
 * - this is a wireguard file so tag it.
 * - src_file is provided by root (PostUp)
 */
int save_wg_resolv(Config *conf, const char *src_file) {
    int status = 0;
    char *dir = nullptr;
    const char *base = nullptr;

    if (!conf) {
        return -1;
    }
    if (!src_file) {
        return 0;
    }
    Options *opts = &conf->opts;

    log_msg(opts, " Saving new wg resolv file from %s", src_file);

    if (!split_path(src_file, &dir, &base)) {
        log_msg(opts, "  error: not a file: %s", src_file);
        free(dir);
        return -1;
    }

    int fd_dir = open(dir, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd_dir < 0) {
        log_msg(opts, "  error opening directory %s from %s: %s", dir, src_file, errno_str(errno));
        free(dir);
        return -1;
    }

    if (is_wg_file(conf, fd_dir, base)) {
        log_msg(opts, "  source is %s - nothing to do", conf->wg.name);

    } else {
        CopyFile copy = {};

        copy.src_dir_fd = fd_dir;
        copy.src_name = base;

        copy.dst_dir_fd = conf->etc.fd;
        copy.dst_name = conf->wg.name;

        copy.tag_ignore = false;
        copy.tag_forbidden = false;
        copy.tag_required = false;
        copy.tag_add = true;
        copy.tag = conf->tag_wg;
        copy.tag_size = conf->tag_wg_size;

        CopyResult result = copy_file_at(&copy);
        if (result == COPY_DONE) {
            log_msg(opts, " Saved %s", conf->wg.name);
        } else {
            log_msg(opts, "  error copy %s to %s: %s", src_file, conf->wg.name,
                    result == COPY_NO_SOURCE ? "no such file" : errno_str(errno));
            status = -1;
        }
    }

    free(dir);
    (void)close(fd_dir);
    return status;
}

/*
 * Where the file replaced by an atomic exchange is left for inspection
 */
static const char *const DISPLACED_INSTALL = ".displaced_install";

/*
 * After installing with an exchange, the file we replaced is in DISPLACED_INSTALL.
 * We saved resolv.conf just before - but another process may have rewritten it
 * in between. If so, theirs is the newest standard: save it. Then remove it.
 */
static void handle_displaced_on_install(Config *conf) {

    if (file_has_tag_at(conf->etc.fd, DISPLACED_INSTALL, false, conf->tag_wg, conf->tag_wg_size) != 1) {
        (void)save_standard_from(conf, DISPLACED_INSTALL);
    }
    (void)unlink_at_priv(conf->etc.fd, DISPLACED_INSTALL);
}

/*
 * Make sure the wireguard resolv.conf is used.
 *
 *  copy:   etc/resolv.conf -> etc/resolv.conf.saved   (unless it is the wireguard one)
 *  copy:   etc/resolv.conf.wg -> etc/resolv.conf       (unless already identical)
 *
 * Idempotent: calling it again when already installed writes nothing.
 * The monitor relies on this - our own writes generate inotify events.
 */
int install_wg_resolv(Config *conf) {

    if (!conf) {
        return -1;
    }
    Options *opts = &conf->opts;

    log_msg(opts, "install_wg_resolv");

    /*
     * Save resolv.conf -> resolv.conf.saved
     * provided its not a tagged wireguard resolv for example set up by post-up.sh
     */
    if (save_standard_resolv(conf) != 0) {
        return -1;
    }

    /*
     * Already installed ?
     */
    if (files_identical_at(conf->etc.fd, conf->wg.name, conf->resolv.name) == 1) {
        log_msg(opts, "  %s already installed", conf->wg.name);
        return 0;
    }

    /*
     * resolv.conf.wg -> resolv.conf
     * - resolv.conf.wg must be tagged (save_wg_resolv always tags it).
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
    copy.displaced_name = DISPLACED_INSTALL;

    CopyResult result = copy_file_at(&copy);

    if (copy.displaced) {
        handle_displaced_on_install(conf);
    }

    switch (result) {
        case COPY_DONE:
            log_msg(opts, "  installed %s", conf->wg.name);
            return 0;

        case COPY_SKIPPED_TAG:
            log_msg(opts, "  error: %s is not tagged - not installed (re-save it with -f)", conf->wg.name);
            return -1;

        case COPY_NO_SOURCE:
            log_msg(opts, "  error: %s not found", conf->wg.name);
            return -1;

        case COPY_UNCHANGED:
            return 0;

        case COPY_ERROR:
        default:
            log_msg(opts, "  error copy %s to %s: %s", conf->wg.name, conf->resolv.name, errno_str(errno));
            return -1;
    }
}
