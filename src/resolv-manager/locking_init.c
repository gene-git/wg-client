/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <errno.h>      // IWYU pragma: keep  (EINTR)
#include <time.h>

enum {
    LOCK_RETRY_MSEC = 250,              /* poll interval while another instance holds the lock */
    LOCK_MAX_WAIT_MSEC = 30 * 1000,     /* give up after this long */
    LOCK_LOG_EVERY = 8,                 /* log every N retries (~2 sec) */
    NSEC_PER_MSEC = 1000 * 1000,
};

/*
 * Sleep msec - full duration even if interrupted.
 * clock_nanosleep() returns the error number (does not set errno).
 */
static void sleep_msec(int msec) {
    struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = (long)msec * NSEC_PER_MSEC,
    };
    while (clock_nanosleep(CLOCK_MONOTONIC, 0, &delay, &delay) == EINTR) {
        /* continue with remaining time */
    }
}

/*
 * Acquire the lock.
 * - retries (every LOCK_RETRY_MSEC, up to LOCK_MAX_WAIT_MSEC) only while
 *   another process holds it. A monitor exiting (wg-quick down) typically
 *   releases it within a fraction of a second, so PostDown waits briefly.
 * - any other failure is reported immediately.
 */
bool initialize_lock(Config *conf, Locking *locking) {
    const int max_tries = LOCK_MAX_WAIT_MSEC / LOCK_RETRY_MSEC;

    locking->dir = "/run/lock/wg-client";
    locking->file = "lock";
    locking->fd = -1;
    locking->dir_fd = -1;

    log_msg(&conf->opts, "Acquiring lock:");

    for (int attempt = 0; ; attempt++) {
        if (acquire_lock(locking)) {
            log_msg(&conf->opts, "  Acquired");
            return true;
        }

        if (!locking->locked_by_other) {
            log_msg(&conf->opts, " Error: unable to lock %s/%s: %s",
                    locking->dir, locking->file, errno_str(locking->err));
            return false;
        }

        if (attempt >= max_tries) {
            break;
        }

        if (attempt % LOCK_LOG_EVERY == 0) {
            log_msg(&conf->opts, " lock held by another process - waiting (%d/%d ms)",
                    attempt * LOCK_RETRY_MSEC, LOCK_MAX_WAIT_MSEC);
        }
        sleep_msec(LOCK_RETRY_MSEC);
    }

    log_msg(&conf->opts, "Error: lock still held by another process after %d ms.", LOCK_MAX_WAIT_MSEC);
    return false;
}
