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

    /*
     * Check if current logfile exists - if not nothing to do.
     */
    if (snprintf(src_path, PATH_MAX, "%s/%s", opts->logdir, logfile) >= PATH_MAX) {
        return -1;
    }

    if (access(src_path, F_OK) != 0) {
        return 0;
    }

    /*
     * log file exists - rotate the logfiles.
     */
    for (int i = opts->num_logfiles - 1; i >= 1; i--) {

        if (snprintf(dst_path, PATH_MAX, "%s/%s.%d", opts->logdir, logfile, i) >= PATH_MAX) {
            return -1;
        }

        if (i == 1) {
            if (snprintf(src_path, PATH_MAX, "%s/%s", opts->logdir, logfile) >= PATH_MAX) {
                return -1;
            }

        } else {
            if (snprintf(src_path, PATH_MAX, "%s/%s.%d", opts->logdir, logfile, i - 1) >= PATH_MAX) {
                return -1;
            }
        }

        /*
         * If it exists, rename (atomic)
         * - if rename fails, print warning and keep going
         */
        if (access(src_path, F_OK) == 0) {
            if (rename(src_path, dst_path) != 0) {
                (void)fprintf(stderr, "Warning: Failed to rotate %s to %s\n", src_path, dst_path);
            }
        }
    }
    return 0;
}


static int open_logfile(Options *opts, int *fd) {
    char log_path[PATH_MAX];
    const char *logfile = LOGFILE;
    off_t max_size = MAX_LOG_SIZE;
    struct stat stbuf = {};
    const int write_flags_exists = O_WRONLY | O_APPEND | O_CLOEXEC; 
    const int write_flags_create = O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC; 
    int write_flags = write_flags_create;
    const mode_t file_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; 

    *fd = -1;

    /*
     * Check if log file exists and time to rotate
     * - logfile exists but smaller than max => do nothing.
     * - logfile > max size, rotate
     * - open logfile and return the file descriptor
     */
    if (snprintf(log_path, PATH_MAX, "%s/%s", opts->logdir, logfile) >= PATH_MAX) {
        return -1;
    }

    bool create_file = true;
    if (access(log_path, F_OK) == 0) {

        create_file = false;
        if (stat(log_path, &stbuf) < 0) {
            return -1;
        }

        if (stbuf.st_size > max_size) {
            if (rotate_logs(opts) < 0) {
                return -1;
            }

            /*
             * logfile should be gone after rotation but double check 
             */
            if (access(log_path, F_OK) != 0)  {
                create_file = true;
            }
        }
    }

    if (create_file) {
        write_flags = write_flags_create;
    } else {
        write_flags = write_flags_exists;
    }

    int log_fd = open(log_path, write_flags, file_perms);
    if (log_fd < 0) {
        (void)fprintf(stderr, "Error: Unable to create or open log file: %s\n", log_path);
        return -1;
    }
    *fd = log_fd;

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
    if (open_logfile(opts, &log_fd) < 0) {
        return -1;
    }

    if (log_fd < 0) {
        return -1;
    }

    /*
     * Bind file descriptor to stream
     */
    opts->log_file = fdopen(log_fd, "a");
    if (!opts->log_file) {
        (void)close(log_fd);
        log_fd = -1;
        return -1;
    }

    /* 
     * Turn off stream buffering so logs flush when written
     */
    if (setvbuf(opts->log_file, nullptr, _IONBF, 0) != 0) {
        perror("warning: error setting log to unbuffereed");
    }

    return 0;
}

