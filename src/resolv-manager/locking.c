/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 *
 * File locking.
 * Uses directory /run/lock/wg-client with mode 1777 (created if missing).
 *
 * Note: the lock file is mode 0666 in a world-writable directory so that
 * any user's wg-client can take it. That also means any local user can
 * hold it. Making it root-only would require every participant to have
 * CAP_DAC_OVERRIDE.
 */
#include "resolv_manager.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>      // SEEK_SET
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static const mode_t LOCK_DIR_PERMS = S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO;   /* 1777 */
static const mode_t LOCK_FILE_PERMS = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;  /* 0666 */
static const mode_t MODE_PERM_BITS = S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO;   /* 07777 */

/*
 * open flags (literals only: avoids signed bitwise on variables)
 */
static const int LOCK_OPEN_FLAGS = O_RDWR | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC;
static const int LOCK_CREATE_FLAGS = O_RDWR | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC | O_CREAT | O_EXCL;

/*
 * Create lock directory if missing (needs caps when parent is root-owned).
 * Owned by root, mode 1777.
 */
static int create_lock_dir(const char *dir) {
    int status = 0;

    if (caps_raise() != 0) {
        return -1;
    }

    mode_t old_umask = umask(0);
    if (mkdir(dir, LOCK_DIR_PERMS) != 0) {
        if (errno != EEXIST) {
            status = -1;
        }
    } else {
        /*
         * We created it - make sure owner and mode are right
         */
        if (caps_can_chown() && chown(dir, 0, 0) != 0) {
            status = -1;
        }
        if (status == 0 && chmod(dir, LOCK_DIR_PERMS) != 0) {
            status = -1;
        }
    }
    umask(old_umask);

    int saved_errno = errno;
    (void)caps_lower();
    errno = saved_errno;
    return status;
}

static int open_lock_dir(const char *dir) {
    const int flags = O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC;

    int dir_fd = open(dir, flags);
    if (dir_fd < 0 && errno == ENOENT) {
        if (create_lock_dir(dir) != 0) {
            return -1;
        }
        dir_fd = open(dir, flags);
    }
    return dir_fd;
}

/*
 * Open (creating if needed) the lock file.
 * - First try without O_CREAT to avoid fs.protected_regular restrictions
 *   in mode 1777 dirs for an existing lock file.
 * - O_EXCL create: if someone else just created it, retry the plain open.
 * - O_NOFOLLOW: never follow a planted symlink. Must be a regular file.
 * Returns fd or -1 (errno set)
 */
static int open_lock_file(int dir_fd, const char *file) {

    int lock_fd = openat(dir_fd, file, LOCK_OPEN_FLAGS);

    if (lock_fd < 0 && errno == ENOENT) {
        mode_t old_umask = umask(0);
        lock_fd = openat(dir_fd, file, LOCK_CREATE_FLAGS, LOCK_FILE_PERMS);
        umask(old_umask);

        if (lock_fd < 0 && errno == EEXIST) {
            lock_fd = openat(dir_fd, file, LOCK_OPEN_FLAGS);
        }
    }

    if (lock_fd < 0) {
        return -1;
    }

    struct stat stbuf = {};
    if (fstat(lock_fd, &stbuf) != 0 || !S_ISREG(stbuf.st_mode)) {
        (void)close(lock_fd);
        errno = EINVAL;
        return -1;
    }

    /*
     * fix any (old) bad permissions - only possible if we own it
     */
    if (stbuf.st_uid == geteuid() && (stbuf.st_mode & MODE_PERM_BITS) != LOCK_FILE_PERMS) {
        (void)fchmod(lock_fd, LOCK_FILE_PERMS);
    }
    return lock_fd;
}

bool acquire_lock(Locking *locking) {
    struct flock lock = {};

    /*
     * Input checks
     */
    if (!locking || !locking->dir || !locking->file || *locking->dir == '\0' || *locking->file == '\0') {
        return false;
    }
    locking->err = 0;
    locking->locked_by_other = false;

    /*
     * Check we already have the lock
     */
    if (locking->acquired) {
        return true;
    }

    /*
     * Proceed to get the lock
     */
    if (locking->dir_fd < 0) {
        locking->dir_fd = open_lock_dir(locking->dir);
    }

    if (locking->dir_fd >= 0 && locking->fd < 0) {
        locking->fd = open_lock_file(locking->dir_fd, locking->file);
    }

    if (locking->dir_fd < 0 || locking->fd < 0) {
        locking->err = errno;
        return false;
    }

    /*
     * lock entire file
     */
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(locking->fd, F_OFD_SETLK, &lock) == -1) {
        locking->err = errno;
        if (errno == EAGAIN || errno == EACCES) {
            locking->locked_by_other = true;
        } else {
            (void)close(locking->fd);
            locking->fd = -1;
        }
        locking->acquired = false;
        return false;
    }

    locking->locked_by_other = false;
    locking->acquired = true;

    return true;
}

void release_lock(Locking *locking) {

    if (!locking) {
        return;
    }

    if (locking->fd >= 0) {
        (void)close(locking->fd);
        locking->fd = -1;
    }

    if (locking->dir_fd >= 0) {
        (void)close(locking->dir_fd);
        locking->dir_fd = -1;
    }

    locking->acquired = false;
    locking->locked_by_other = false;
}
