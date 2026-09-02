/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
//#define _POSIX_C_SOURCE 200112L // Required for clock_nanosleep
#include "resolv_manager.h"
#include <errno.h>
#include <stdio.h>
#include <time.h>


bool initialize_lock(Config *conf, Locking *locking) {
    locking->dir = "/run/lock/wg-client";
    locking->file = "lock";
    locking->fd = -1;
    locking->dir_fd = -1;
    int attempts = 0;
    int max_tries = 15; 
    int delay_secs = 2;

    log_msg(&conf->opts, "Acquiring lock:");

    while (attempts <= max_tries) {
        if (acquire_lock(locking)) {
            log_msg(&conf->opts, "  Acquired");
            return true;
        }

        if (locking->locked_by_other) {
            log_msg(&conf->opts, " Error lock held by another process.\n");
        }

        if (attempts == max_tries) {
            break;
        }

        attempts++;
        log_msg(&conf->opts, " locked held by other process - retry in %d sec (%d/%d",
                delay_secs, attempts, max_tries);;

        struct timespec delay = {
            .tv_sec = delay_secs,
            .tv_nsec = 0
        };

        /*
         * Sleep.
         * if dont want ENINTR to break sleep then
         * wrap in a while (clock_nanosleep() == -1 && errno == EINTR) loop 
         *
         */
        clock_nanosleep(CLOCK_MONOTONIC, 0, &delay, &delay);
    }

    log_msg(&conf->opts, "Error: Failed to acquire lock after %d attempts. Exiting.\n", max_tries);
    return false;
}

