/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


enum {
    NSEC_PER_DECI = 100000000L,
    TIME_STR_SIZE = 32,
    ERR_STR_SIZE = 128,
};

/*
 * Thread safe strerror()
 * Returns pointer to a thread local buffer (valid until next call in this thread).
 */
const char *errno_str(int errnum) {
    static thread_local char buf[ERR_STR_SIZE];

    /*
     * GNU strerror_r (_GNU_SOURCE) returns a pointer to the message
     * which may or may not be buf.
     */
    return strerror_r(errnum, buf, sizeof(buf));
}

/*
 * Output message to logfile
 * - timestamp: <message>
 * - Message uses standard printf() format.
 * - newline is appended to the message (do not add one)
 */
void log_msg(Options *opts, const char *fmt, ...) {

    if (!fmt) {
        return;
    }

    /*
     * fallback to stdout
     */
    FILE *fptr = nullptr;
    if (opts && opts->log_file) {
        fptr = opts->log_file;
    } else {
        fptr = stdout;
    }

    struct timespec now_ts = {};
    (void)clock_gettime(CLOCK_REALTIME, &now_ts);

    /*
     * Use localtime
     */
    struct tm tm_human = {};
    (void)localtime_r(&now_ts.tv_sec, &tm_human);

    /*
     * timestamp YYYY-mm-dd-HH:MM:SS.d
     * TIME_STR_SIZE is more than enough space so dont bother checking return of strftime
     */
    char time_string[TIME_STR_SIZE] = {};
    (void) strftime(time_string, sizeof(time_string), "%Y-%m-%d-%H:%M:%S", &tm_human);
    long deciseconds = now_ts.tv_nsec / NSEC_PER_DECI;

    (void)fprintf(fptr, "%s.%01ld: ", time_string, deciseconds);

    /*
     * Write the log message
     */
    va_list args = {};
    va_start(args, fmt);
    (void)vfprintf(fptr, fmt, args);        // NOLINT(clang-analyzer-security.VAList,clang-analyzer-valist.Uninitialized)
    va_end(args);

    (void)fprintf(fptr, "\n");
    if (fptr == stdout) {
        (void)fflush(stdout);
    }
}
