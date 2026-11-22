/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <stdlib.h>
#include <unistd.h>

static void close_dir(DirTarget *target) {
    if (target->fd >= 0) {
        (void)close(target->fd);
        target->fd = -1;
    }
    free(target->path);
    target->path = nullptr;
}

/*
 * Release everything held by conf.
 * Safe to call more than once. Requires fds to be -1 when not open
 * (initialize() sets that first thing).
 */
void config_clean(Config *conf) {

    if (!conf) {
        return;
    }

    /*
     * directories:
     * - /sys/class/net
     * - /etc/
     * - directory of -f file
     */
    close_dir(&conf->sys_net);
    close_dir(&conf->etc);
    close_dir(&conf->wg_src);

    free(conf->wg_src_name);
    conf->wg_src_name = nullptr;

    log_close(&conf->opts);
}
