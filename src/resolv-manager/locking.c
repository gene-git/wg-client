/* 
 * File locking.
 * Requires directory /run/lock/wg-client with mode 1777
 */
#include "resolv_manager.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

[[nodiscard]] bool acquire_lock(Locking *locking) {
    struct flock lock = {};
    int rw_flags = O_RDWR | O_CLOEXEC;
    int rw_flags_create = O_RDWR | O_CREAT | O_CLOEXEC;
    mode_t file_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    /*
     * Input checks
     */
    if (!locking->dir || !locking->file || *locking->dir == '\0' || *locking->file == '\0') {
        return false;
    }

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
        locking->dir_fd = open(locking->dir, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (locking->dir_fd < 0) {
            return false;
        }
    }

    if (locking->fd < 0) {
        /*
         * First - try without O_CREAT to avoid fs.protected_regular restrictions
         * in mode 1777 dirs for existing lock file.
         */
        locking->fd = openat(locking->dir_fd, locking->file, rw_flags, file_perms);

        /*
         *  If the file doesn't exist, create it.
         */
        if (locking->fd < 0 && errno == ENOENT) {
            mode_t old_umask = umask(0);
            locking->fd = openat(locking->dir_fd, locking->file, rw_flags_create, file_perms);
            umask(old_umask);
        }

        if (locking->fd < 0) {
            return false;
        }

        /*
         * fix any (old) bad permissions
         */
        (void)fchmod(locking->fd, file_perms);
    }

    /*
     * lock entire file
     */
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(locking->fd, F_OFD_SETLK, &lock) == -1) {
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
