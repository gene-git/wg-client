/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

enum {
    MAX_LOG_SIZE = 1024 * 1024,
};
#define LOGFILE "resolv-manager"


/*
 * Rotate logs (log -> log.1, log.1 -> log.2 ...
 * Do it in reverse order obviously.
 */
static int rotate_logs(Options *opts) {
    char src_path[PATH_MAX];
    char dst_path[PATH_MAX];
    const char *logfile = LOGFILE;

    for (int i = opts->num_logfiles - 1; i >= 1; i--) {

        if (snprintf(dst_path, PATH_MAX, "%s/%s.%d", opts->logdir, logfile, i) >= PATH_MAX) {
            return -1;
        }

        int ret = 0;
        if (i == 1) {
            ret = snprintf(src_path, PATH_MAX, "%s/%s", opts->logdir, logfile);
        } else {
            ret = snprintf(src_path, PATH_MAX, "%s/%s.%d", opts->logdir, logfile, i - 1);
        }
        if (ret < 0 || ret >= PATH_MAX) {
            return -1;
        }

        /*
         * rename (atomic) - missing source is fine
         * - if rename fails otherwise, print warning and keep going
         */
        if (rename(src_path, dst_path) != 0 && errno != ENOENT) {
            (void)fprintf(stderr, "Warning: Failed to rotate %s to %s: %s\n", src_path, dst_path, errno_str(errno));
        }
    }
    return 0;
}


/*
 * Open log file (rotating first if it is too large).
 * Never follows a symlink.
 */
static int open_logfile(Options *opts, int *log_fd_out) {
    char log_path[PATH_MAX];
    const char *logfile = LOGFILE;
    const off_t max_size = MAX_LOG_SIZE;
    struct stat stbuf = {};
    const int write_flags = O_WRONLY | O_CREAT | O_APPEND | O_NOFOLLOW | O_NOCTTY | O_CLOEXEC;
    const int write_flags_trunc = O_WRONLY | O_CREAT | O_APPEND | O_NOFOLLOW | O_NOCTTY | O_CLOEXEC | O_TRUNC;
    const mode_t file_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    *log_fd_out = -1;

    if (snprintf(log_path, PATH_MAX, "%s/%s", opts->logdir, logfile) >= PATH_MAX) {
        return -1;
    }

    /*
     * Rotate if needed
     * - with a single log file there is nothing to rotate to: truncate instead.
     */
    bool truncate_log = false;
    if (lstat(log_path, &stbuf) == 0 && S_ISREG(stbuf.st_mode) && stbuf.st_size > max_size) {
        if (opts->num_logfiles > 1) {
            if (rotate_logs(opts) < 0) {
                return -1;
            }
        } else {
            truncate_log = true;
        }
    }

    int log_fd = open(log_path, truncate_log ? write_flags_trunc : write_flags, file_perms);
    if (log_fd < 0) {
        (void)fprintf(stderr, "Error: Unable to create or open log file %s: %s\n", log_path, errno_str(errno));
        return -1;
    }

    if (fstat(log_fd, &stbuf) != 0 || !S_ISREG(stbuf.st_mode)) {
        (void)fprintf(stderr, "Error: log file is not a regular file: %s\n", log_path);
        (void)close(log_fd);
        return -1;
    }
    *log_fd_out = log_fd;

    return 0;
}

/*
 * Prep the log file.
 * - if no logfile create it
 * - if logfile over size limit, rotate and create new one.
 * - open the log and bind to FILE stream: opts->log_file
 */
int log_init(Options *opts) {

    if (!opts || !opts->logdir || opts->num_logfiles <= 0) {
        return 0;
    }

    /*
     * /dev/null means no logging.
     */
    if (strcmp(opts->logdir, "/dev/null") == 0) {
        return 0;
    }

    if (makedir_if_needed(opts->logdir) != 0) {
        (void)fprintf(stderr, "Error: Failed creating log directory %s\n", opts->logdir);
        return -1;
    }

    /*
     * Open the file (will rotate if needed)
     */
    int log_fd = -1;
    if (open_logfile(opts, &log_fd) < 0 || log_fd < 0) {
        return -1;
    }

    /*
     * Bind file descriptor to stream
     */
    opts->log_file = fdopen(log_fd, "a");
    if (!opts->log_file) {
        (void)close(log_fd);
        return -1;
    }

    /*
     * Turn off stream buffering so logs flush when written
     */
    if (setvbuf(opts->log_file, nullptr, _IONBF, 0) != 0) {
        perror("warning: error setting log to unbuffered");
    }

    return 0;
}
