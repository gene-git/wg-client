/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sysexits.h>
#include <unistd.h>

/*
 * Run as background daemon
 */
int daemonize(void) {

    /*
     * fork #1
     */
    pid_t pid = fork();
    if (pid < 0) {
        return EX_OSERR;
    }
    if (pid > 0) {
        exit(EX_OK);
    }

    if (setsid() < 0) {
        return EX_OSERR;
    }

    /*
     * fork #2
     */
    pid = fork();
    if (pid < 0) {
        return EX_OSERR;
    }
    if (pid > 0) {
        exit(EX_OK);
    }

    umask(0);

    if (chdir("/") < 0) {
        return EX_OSERR;
    }

    /*
     * dup to /dev/null more secure than closing
     * confirm /dev/null is valid character device
     */
    int dev_null = open("/dev/null", O_RDWR | O_CLOEXEC);
    if (dev_null >= 0) {
        struct stat stbuf = {};

        if (fstat(dev_null, &stbuf) < 0 || !S_ISCHR(stbuf.st_mode)) {
            (void)close(dev_null);
            (void)close(STDIN_FILENO);
            (void)close(STDOUT_FILENO);
            (void)close(STDERR_FILENO);
            return EX_OSERR;
        }

        dup2(dev_null, STDIN_FILENO);
        dup2(dev_null, STDOUT_FILENO);
        dup2(dev_null, STDERR_FILENO);
        (void)close(dev_null);

    } else {
        return EX_OSERR;
    }
    return EX_OK;
}

