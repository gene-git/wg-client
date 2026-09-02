/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <unistd.h>


/*
 * Create directory (and intermediates) if it doesnt exist.
 * Create perms 0750
 */
int makedir_if_needed(const char *path) {

    char temp_path[PATH_MAX];       // NOLINT(misc-include-cleaner)
    size_t length = strlen(path);
    const mode_t dir_perms = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    if (!path || access(path, F_OK) == 0) {
        return 0;
    }

    if (length >= PATH_MAX || strlcpy(temp_path, path, PATH_MAX) >= PATH_MAX) {
        return -1;
    }

    /*
     * Keep track of each slash - walk through the path.
     * Write null over each one slash to create any intermediate paths
     * - skip leading slash.
     */
    char *slash = strchr(temp_path, '/');
    if (slash == temp_path) {
        slash = strchr(slash + 1U, '/');
    }

    /*
     * Walk the path creating any missing directories.
     */
    while (slash) {
        *slash = '\0';

        if (access(temp_path, F_OK) != 0) {
            if (mkdir(temp_path, dir_perms) != 0 && errno != EEXIST) {  // NOLINT(misc-include-cleaner)
                return -1;
            }
        }

        *slash = '/';
        slash = strchr(slash + 1U, '/');
    }

    /*
     * Handle any trailing slash
     */
    if (access(temp_path, F_OK) != 0) {
        if (mkdir(temp_path, dir_perms) != 0 && errno != EEXIST) {  // NOLINT(misc-include-cleaner)
            return -1;
        }
    }

    return 0;
}

