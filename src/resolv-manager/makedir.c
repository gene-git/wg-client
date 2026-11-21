/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>


static int mkdir_one(const char *path, mode_t perms) {
    if (mkdir(path, perms) != 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

/*
 * Create directory (and intermediates) if it doesnt exist.
 * Create perms 0755 (subject to umask)
 */
int makedir_if_needed(const char *path) {

    char temp_path[PATH_MAX];
    const mode_t dir_perms = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    if (!path || *path == '\0') {
        return -1;
    }

    struct stat stbuf = {};
    if (stat(path, &stbuf) == 0) {
        return S_ISDIR(stbuf.st_mode) ? 0 : -1;
    }

    if (strlcpy(temp_path, path, sizeof(temp_path)) >= sizeof(temp_path)) {
        return -1;
    }

    /*
     * Walk the path - write null over each slash to create any
     * intermediate directories (skip leading slash).
     */
    char *slash = strchr(temp_path + 1, '/');

    while (slash) {
        *slash = '\0';

        if (mkdir_one(temp_path, dir_perms) != 0) {
            return -1;
        }

        *slash = '/';
        slash = strchr(slash + 1, '/');
    }

    /*
     * Final component (no trailing slash case)
     */
    if (mkdir_one(temp_path, dir_perms) != 0) {
        return -1;
    }

    return 0;
}
