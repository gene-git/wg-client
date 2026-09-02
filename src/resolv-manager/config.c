
/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <stdlib.h>
#include <unistd.h>


void config_clean(Config *conf) {

    if (!conf) {
        return;
    }

    /*
     * directory fds
     * - 
     */
    if (conf->sys_net.fd >= 0) {
        (void)close(conf->sys_net.fd);
        conf->sys_net.fd = -1;
    }

    if (conf->etc.fd >= 0) {
        (void)close(conf->etc.fd);
        conf->etc.fd = -1;
    }

    if (conf->sem.fd >= 0) {
        (void)close(conf->sem.fd);
        conf->sem.fd = -1;
    }

    /*
     * directory names : 
     * - /sys/class/net
     * - /etc/
     * - <sem>
     */
    if (conf->sys_net.path) {
        free(conf->sys_net.path);
        conf->sys_net.path = nullptr;
    }

    if (conf->etc.path) {
        free(conf->etc.path);
        conf->etc.path = nullptr;
    }

    if (conf->sem.path) {
        free(conf->sem.path);
        conf->sem.path = nullptr;
    }

    /*
     * File names
     */
    if (conf->semaphore.name) {
        free(conf->semaphore.name);
        conf->semaphore.name = nullptr;
    }

    log_close(&conf->opts);
}
