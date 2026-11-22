/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <ctype.h>
#include <limits.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


enum {
    ASCII_LAST_PRINTABLE = 0x7e,
};

/*
 * realpath() of path as a malloc'd string, or nullptr
 */
static char *real_path_dup(const char *path) {
    char buf[PATH_MAX] = {};

    if (!realpath(path, buf)) {
        (void)fprintf(stderr, "Error: directory not found: %s\n", path);
        return nullptr;
    }
    return strdup(buf);
}

/*
 * Directories /etc, /sys/class/net
 * - test mode prepend with "./"
 */
static bool init_standard_dirs(Config *conf) {
    Options *opts = &conf->opts;

    const char *sys_net_path = opts->testing ? "./sys/class/net" : "/sys/class/net";
    const char *etc_path = opts->testing ? "./etc" : "/etc";

    conf->sys_net.path = real_path_dup(sys_net_path);
    if (!conf->sys_net.path) {
        return false;
    }

    conf->etc.path = real_path_dup(etc_path);
    return conf->etc.path != nullptr;
}

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
 * -f <file>: wireguard resolv.conf source
 * Directory resolved to an absolute path now (the daemon changes to "/"),
 * file name kept separately so the monitor can watch the directory.
 */
static bool init_wg_source(Config *conf) {
    const char *file = conf->opts.file;
    char *dir = nullptr;
    const char *base = nullptr;

    if (!file) {
        return true;
    }

    if (!split_path(file, &dir, &base)) {
        (void)fprintf(stderr, "Error: not a file: %s\n", file);
        free(dir);
        return false;
    }

    conf->wg_src.path = real_path_dup(dir);
    free(dir);
    if (!conf->wg_src.path) {
        return false;
    }

    conf->wg_src_name = strdup(base);
    return conf->wg_src_name != nullptr;
}

/*
 * Interface name must be a plausible kernel interface name:
 * 1..IFNAMSIZ-1 chars, no '/', no whitespace / control chars, not "." or "..".
 */
static bool valid_iface_name(const char *name) {
    size_t len = strnlen(name, IFNAMSIZ);

    if (len == 0 || len >= IFNAMSIZ) {
        return false;
    }
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        unsigned char chr = (unsigned char)name[i];
        if (chr == '/' || chr == ':' || isspace(chr) || iscntrl(chr) || chr > ASCII_LAST_PRINTABLE) {
            return false;
        }
    }
    return true;
}

static const char *task_name(Task task) {
    switch (task) {
        case INSTALL_WG:
            return "Install WG";

        case INSTALL_STANDARD:
            return "Install Standard";

        case MONITOR:
            return "Monitor";

        case NOTHING:
        default:
            return " - ";
    }
}

static void log_welcome_message(Config *conf) {
    Options *opts = &conf->opts;

    const char *iface_name = conf->iface.name ? conf->iface.name : " - ";
    const char *daemon = opts->daemon ? "yes" : "no";

    log_msg(opts, "--------------");
    log_msg(opts, "resolv-manager initialized");
    log_msg(opts, "  %15s : %s", "iface-name", iface_name);
    log_msg(opts, "  %15s : %s", "task", task_name(opts->task));
    log_msg(opts, "  %15s : %s", "daemon", daemon);
    log_msg(opts, "  %15s : %s:%s", "real", conf->user_real.user, conf->user_real.group);
    log_msg(opts, "  %15s : %s:%s", "effective", conf->user_effective.user, conf->user_effective.group);
}


/*
 * Initialize options and config.
 * Returns:
 *   0  ok
 *   1  nothing more to do (e.g. help) - exit success
 *  -1  error
 */
int initialize(int argc, char **argv, Config *conf) {

    if (!conf) {
        return -1;
    }

    /*
     * fds first - so config_clean() is always safe
     */
    conf->sys_net.fd = -1;
    conf->etc.fd = -1;
    conf->wg_src.fd = -1;

    /*
     * command line
     */
    int ret = parse_options(argc, argv, &conf->opts);
    if (ret != 0) {
        return (ret > 0) ? 1 : -1;
    }

    /*
     * Interface name
     */
    conf->iface.name = conf->opts.iface_name ? conf->opts.iface_name : "wg0";
    if (!valid_iface_name(conf->iface.name)) {
        (void)fprintf(stderr, "Error: invalid interface name\n");
        return -1;
    }

    /*
     * Directories /etc, /sys/class/net
     */
    if (!init_standard_dirs(conf)) {
        return -1;
    }

    /*
     * -f source of the wireguard resolv.conf
     */
    if (!init_wg_source(conf)) {
        return -1;
    }

    /*
     * static files names (not freed by config_clean())
     */
    conf->resolv.name = "resolv.conf";
    conf->wg.name = "resolv.conf.wg";
    conf->save.name = "resolv.conf.saved";

    conf->perms.user_run = "nobody";
    conf->perms.group_run = "nobody";

    /*
     * File tag to mark wireguard resolv.conf
     */
    conf->tag_wg = "# === TAG: wireguard ===\n";
    conf->tag_wg_size = strlen(conf->tag_wg);

    /*
     * Get real/effectve user info
     */
    (void)get_user_info(&conf->user_real, &conf->user_effective);

    /*
     * Set up logging
     * Ok if fails (likely permission problem) - falls back to stdout
     */
    if (log_init(&conf->opts) != 0) {
        (void)fprintf(stderr, "Warning: logging to stdout\n");
    }
    log_welcome_message(conf);

    return 0;
}
