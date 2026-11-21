/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sysexits.h>
#include <unistd.h>

/*
 * Run as background daemon
 * Parents use _exit() so stdio buffers / atexit handlers are not run twice.
 * Locks are OFD locks so they stay with the child.
 */
int daemonize() {

    (void)fflush(nullptr);

    /*
     * fork #1
     */
    pid_t pid = fork();
    if (pid < 0) {
        return EX_OSERR;
    }
    if (pid > 0) {
        _exit(EX_OK);
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
        _exit(EX_OK);
    }

    /*
     * All files we create have explicit modes - 022 is defense in depth.
     */
    umask(S_IWGRP | S_IWOTH);

    if (chdir("/") < 0) {
        return EX_OSERR;
    }

    /*
     * dup to /dev/null more secure than closing
     * confirm /dev/null is valid character device
     */
    int dev_null = open("/dev/null", O_RDWR | O_CLOEXEC);
    if (dev_null < 0) {
        return EX_OSERR;
    }

    struct stat stbuf = {};
    if (fstat(dev_null, &stbuf) < 0 || !S_ISCHR(stbuf.st_mode)) {
        (void)close(dev_null);
        return EX_OSERR;
    }

    int status = EX_OK;
    if (dup2(dev_null, STDIN_FILENO) < 0 ||
        dup2(dev_null, STDOUT_FILENO) < 0 ||
        dup2(dev_null, STDERR_FILENO) < 0) {
        status = EX_OSERR;
    }

    if (dev_null > STDERR_FILENO) {
        (void)close(dev_null);
    }
    return status;
}
