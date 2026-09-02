/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>


enum {
    MILLI = 1000000U,
    CENTI = 10000000U,
    DECI  = 100000000U,
};
/*
 * Output message to logfile
 * - timestamp: <message>
 * - Message uses standard printf() format.
 * - newline is appended to the message
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

    struct timespec ts = {};
    clock_gettime(CLOCK_REALTIME, &ts);         // NOLINT(misc-include-cleaner)

    /*
     * Use localtime
     */
    struct tm tm_human = {};
    localtime_r(&ts.tv_sec, &tm_human);

    /*
     * timestamp YYYY-mm-dd-HH-MM-SS
     * Keep to nearest second
     * 32 is more than enough space so dont bother checkign return of strftime
     */
    char time_string[32] = {};
    (void) strftime(time_string, sizeof(time_string), "%Y-%m-%d-%H:%M:%S", &tm_human);
    long deciseconds = ts.tv_nsec / DECI;

    (void)fprintf(fptr, "%s.%01ld: ", time_string, deciseconds);

    /*
     * Write the log message 
     */
    va_list args = {};
    va_start(args, fmt);
    (void)vfprintf(fptr, fmt, args);        // NOLINT(clang-analyzer-security.VAList)
    va_end(args);

    (void)fprintf(fptr, "\n");
}

