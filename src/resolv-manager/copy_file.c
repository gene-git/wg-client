/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 *
 */
#include "resolv_manager.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

enum {
    BUFFER_SIZE = 4096,
    MAX_COMPARE_SIZE = 1024 * 1024,     /* resolv.conf files are tiny */
};

static const mode_t FILE_PERMS = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;   /* 0644 */

struct Info {
    ssize_t bytes_read;
    char buffer[BUFFER_SIZE];

    int src_fd;
    int dst_fd;

    bool tag_active;
    bool tag_matches;
    bool tmp_created;

    char tmp_name[PATH_MAX];
};

/*
 * read() that retries on EINTR and keeps reading until count bytes or EOF.
 */
static ssize_t read_full(int fdesc, char *buf, size_t count) {
    size_t total = 0;

    while (total < count) {
        ssize_t nbytes = read(fdesc, buf + total, count - total);
        if (nbytes < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (nbytes == 0) {
            break;
        }
        total += (size_t)nbytes;
    }
    return (ssize_t)total;
}

/*
 * write() all bytes - handles short writes and EINTR.
 */
static bool write_all(int fdesc, const char *buf, size_t count) {
    size_t total = 0;

    while (total < count) {
        ssize_t nbytes = write(fdesc, buf + total, count - total);
        if (nbytes < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        total += (size_t)nbytes;
    }
    return true;
}

/*
 * Open a regular file for reading.
 * O_NONBLOCK so a FIFO cannot block us; anything not a regular file is rejected.
 * Returns fd or -1 (errno set).
 */
static int open_regular_ro(int dir_fd, const char *name, int extra_flags) {
    int fdesc = openat(dir_fd, name, O_RDONLY | O_CLOEXEC | O_NOCTTY | O_NONBLOCK | extra_flags);  // NOLINT(hicpp-signed-bitwise)
    if (fdesc < 0) {
        return -1;
    }

    struct stat stbuf = {};
    if (fstat(fdesc, &stbuf) != 0 || !S_ISREG(stbuf.st_mode)) {
        (void)close(fdesc);
        errno = EINVAL;
        return -1;
    }
    return fdesc;
}

/*
 * Check tag info
 * sets info->tag_matches and returns it:
 *  - true if the source starts with copy->tag
 *  - otherwise false.
 */
static bool source_has_tag(CopyFile *copy, struct Info *info) {

    info->tag_matches = false;
    if (!copy->tag || copy->tag_size == 0 || copy->tag_size > BUFFER_SIZE) {
        return false;
    }

    /*
     * If file is smaller than the tag, it obviously doesn't match
     */
    if (info->bytes_read >= 0 && (size_t)info->bytes_read >= copy->tag_size &&
        memcmp(info->buffer, copy->tag, copy->tag_size) == 0) {
        info->tag_matches = true;
    }

    return info->tag_matches;
}


/*
 * Okay to write output if:
 * a) tags are ignored
 * b) tag_forbidden and src does not have the tag
 * c) tag_required and src has the tag
 * d) neither forbidden nor required
 */
static bool check_okay_to_write(CopyFile *copy, struct Info *info) {

    bool has_tag = source_has_tag(copy, info);

    if (copy->tag_ignore) {
        return true;
    }

    if (copy->tag_forbidden && has_tag) {
        return false;
    }

    if (copy->tag_required && !has_tag) {
        return false;
    }
    return true;
}


/*
 * Create temp file in same directory (so rename is atomic)
 * Any stale temp file is removed first and the new one created with O_EXCL
 * so we never write through a symlink or into someone else's file.
 * Caller has raised caps.
 */
static bool create_temp_file(CopyFile *copy, struct Info *info) {

    if (snprintf(info->tmp_name, PATH_MAX, ".tmp_%s", copy->dst_name) >= PATH_MAX) {
        return false;
    }

    if (unlinkat(copy->dst_dir_fd, info->tmp_name, 0) != 0 && errno != ENOENT) {
        return false;
    }

    const int flags = O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC;
    info->dst_fd = openat(copy->dst_dir_fd, info->tmp_name, flags, FILE_PERMS);
    info->tmp_created = info->dst_fd >= 0;

    return info->tmp_created;
}


/*
 * Write temp file
 * - handle adding tag if needed
 */
static int write_temp_file(CopyFile *copy, struct Info *info) {

    /*
     * copy->tag_add:
     * - add tag to front of file if not already there.
     */
    if (info->tag_active && copy->tag_add && !info->tag_matches &&
        !write_all(info->dst_fd, copy->tag, copy->tag_size)) {
        return -1;
    }

    /*
     * Write the first chunk that we already read
     */
    if (info->bytes_read > 0 && !write_all(info->dst_fd, info->buffer, (size_t)info->bytes_read)) {
        return -1;
    }

    /*
     * Write remainder of file
     */
    for (;;) {
        info->bytes_read = read(info->src_fd, info->buffer, sizeof(info->buffer));
        if (info->bytes_read < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (info->bytes_read == 0) {
            break;
        }
        if (!write_all(info->dst_fd, info->buffer, (size_t)info->bytes_read)) {
            return -1;
        }
    }

    return 0;
}

/*
 * Make file root:root 0644 (if we can chown) - done on the temp fd
 * before the rename so the final file never has the wrong owner.
 */
static int set_owner_mode(int fdesc) {
    if (fchmod(fdesc, FILE_PERMS) != 0) {
        return -1;
    }
    if (caps_can_chown() && fchown(fdesc, 0, 0) != 0) {
        return -1;
    }
    return 0;
}

/*
 * Open the source (caps raised - all sources are in /etc or root provided)
 * and read the first chunk.
 */
static CopyResult open_source(CopyFile *copy, struct Info *info) {

    if (caps_raise() != 0) {
        return COPY_ERROR;
    }
    info->src_fd = open_regular_ro(copy->src_dir_fd, copy->src_name, 0);
    int open_errno = errno;
    (void)caps_lower();

    if (info->src_fd < 0) {
        return (open_errno == ENOENT) ? COPY_NO_SOURCE : COPY_ERROR;
    }

    info->bytes_read = read_full(info->src_fd, info->buffer, sizeof(info->buffer));
    if (info->bytes_read < 0) {
        (void)close(info->src_fd);
        info->src_fd = -1;
        return COPY_ERROR;
    }
    return COPY_DONE;
}

/*
 * Temp file: create, write, owner/mode, fsync, close.
 * Caller has raised caps.
 */
static CopyResult write_destination(CopyFile *copy, struct Info *info) {
    CopyResult result = COPY_DONE;

    if (!create_temp_file(copy, info) ||
        write_temp_file(copy, info) != 0 ||
        set_owner_mode(info->dst_fd) != 0 ||
        fsync(info->dst_fd) != 0) {
        result = COPY_ERROR;
    }

    if (info->dst_fd >= 0 && close(info->dst_fd) != 0) {
        result = COPY_ERROR;
    }
    info->dst_fd = -1;
    return result;
}

/*
 * Put temp in place of destination - atomic.
 * - displaced_name set: exchange temp and destination, then move what was
 *   the destination to displaced_name. Falls back to a plain rename if the
 *   destination does not exist or the filesystem lacks RENAME_EXCHANGE.
 * Returns 0 or -1 (errno set).
 */
static int move_into_place(CopyFile *copy, struct Info *info) {
    const int dir_fd = copy->dst_dir_fd;

    copy->displaced = false;

    if (copy->displaced_name) {
        if (renameat2(dir_fd, info->tmp_name, dir_fd, copy->dst_name, RENAME_EXCHANGE) == 0) {
            /*
             * old destination is now at tmp_name
             */
            if (renameat(dir_fd, info->tmp_name, dir_fd, copy->displaced_name) == 0) {
                copy->displaced = true;
            } else {
                (void)unlinkat(dir_fd, info->tmp_name, 0);
            }
            return 0;
        }
        if (errno != ENOENT && errno != EINVAL && errno != ENOSYS) {
            return -1;
        }
    }
    return renameat(dir_fd, info->tmp_name, dir_fd, copy->dst_name);
}

/*
 * Move temp into place and sync the directory.
 * On any earlier failure remove the temp file.
 */
static CopyResult finish_destination(CopyFile *copy, struct Info *info, CopyResult result) {

    if (result == COPY_DONE) {
        if (move_into_place(copy, info) == 0) {
            (void)fsync(copy->dst_dir_fd);
            return COPY_DONE;
        }
        result = COPY_ERROR;
    }

    if (info->tmp_created) {
        int saved_errno = errno;
        (void)unlinkat(copy->dst_dir_fd, info->tmp_name, 0);
        errno = saved_errno;
    }
    return result;
}


/*
 * Copy file src_dir/src_name to dst_dir/dst_name
 * Use openat() so requires open directory fd for each.
 *
 * Capabilities are raised only while opening the source and while
 * creating/writing/renaming the destination.
 *
 * Files are read/written in 4k chunks
 * Destination is 0644, owned by root:root when we have CAP_CHOWN (or are root).
 *
 * Output is written to a temporary file, fsync'd, then renamed (atomic) and
 * the directory fsync'd.
 *
 * Tags (tag_required/tag_forbidden refer to the source):
 *
 *   tag_required  tag_forbidden  written if
 *   ------------  -------------  -----------------------
 *   no            no             always
 *   yes           -              src is tagged
 *   -             yes            src is not tagged
 *
 * tag_add: if the source is not tagged, the tag is prepended to the output.
 *
 * Standard resolv.conf are never tagged.
 * Wireguard configs are always tagged.
 *
 * forbid_empty / displaced_name: see CopyFile in resolv_manager.h
 *
 * Returns CopyResult:
 *   COPY_ERROR, COPY_DONE, COPY_SKIPPED_TAG, COPY_NO_SOURCE, COPY_EMPTY_SOURCE
 */
CopyResult copy_file_at(CopyFile *copy) {

    struct Info info = {};
    info.src_fd = -1;
    info.dst_fd = -1;

    if (!copy || !copy->src_name || !copy->dst_name) {
        return COPY_ERROR;
    }

    /*
     * Caller can set tag_ignore to ensure files are copied
     * independent of any tag(s)
     * When tags are active, they must be provided.
     */
    info.tag_active = true;
    if (copy->tag_ignore) {
        info.tag_active = false;
    }
    if (info.tag_active && !(copy->tag && copy->tag_size > 0 && copy->tag_size <= BUFFER_SIZE)) {
        return COPY_ERROR;
    }

    copy->displaced = false;

    CopyResult result = open_source(copy, &info);
    if (result != COPY_DONE) {
        return result;
    }

    if (copy->forbid_empty && info.bytes_read == 0) {
        (void)close(info.src_fd);
        return COPY_EMPTY_SOURCE;
    }

    /*
     * Tag handling (first chunk)
     */
    if (info.tag_active && !check_okay_to_write(copy, &info)) {
        (void)close(info.src_fd);
        return COPY_SKIPPED_TAG;
    }

    if (caps_raise() != 0) {
        (void)close(info.src_fd);
        return COPY_ERROR;
    }

    result = write_destination(copy, &info);
    (void)close(info.src_fd);
    result = finish_destination(copy, &info, result);

    int saved_errno = errno;
    (void)caps_lower();
    errno = saved_errno;
    return result;
}


/*
 * Returns 1 if name is a symlink, 0 if not (or does not exist), -1 on error
 */
int is_symlink_at(int dir_fd, const char *name) {
    struct stat stbuf = {};

    if (caps_raise() != 0) {
        return -1;
    }
    int ret = fstatat(dir_fd, name, &stbuf, AT_SYMLINK_NOFOLLOW);
    int saved_errno = errno;
    (void)caps_lower();

    if (ret != 0) {
        errno = saved_errno;
        return (saved_errno == ENOENT) ? 0 : -1;
    }
    return S_ISLNK(stbuf.st_mode) ? 1 : 0;
}


/*
 * Read whole (small) regular file - does not follow symlinks.
 * Returns malloc'd buffer (caller frees) or nullptr. errno set on failure.
 */
static char *read_small_file(int dir_fd, const char *name, size_t *size) {
    int fdesc = open_regular_ro(dir_fd, name, O_NOFOLLOW);
    if (fdesc < 0) {
        return nullptr;
    }

    char *buf = malloc(MAX_COMPARE_SIZE + 1U);
    if (!buf) {
        (void)close(fdesc);
        return nullptr;
    }

    ssize_t nbytes = read_full(fdesc, buf, MAX_COMPARE_SIZE + 1U);
    (void)close(fdesc);

    if (nbytes < 0 || (size_t)nbytes > MAX_COMPARE_SIZE) {
        free(buf);
        errno = (nbytes < 0) ? errno : EFBIG;
        return nullptr;
    }
    *size = (size_t)nbytes;
    return buf;
}

/*
 * Compare 2 small regular files in the same directory.
 * Symlinks are never "identical" (not followed).
 * Only used on files in /etc - read with capabilities raised.
 * Returns 1 if identical, 0 if different or either is missing/not regular.
 */
int files_identical_at(int dir_fd, const char *name_a, const char *name_b) {
    size_t size_a = 0;
    size_t size_b = 0;
    int same = 0;

    if (caps_raise() != 0) {
        return 0;
    }
    char *buf_a = read_small_file(dir_fd, name_a, &size_a);
    char *buf_b = read_small_file(dir_fd, name_b, &size_b);
    (void)caps_lower();

    if (buf_a && buf_b && size_a == size_b && memcmp(buf_a, buf_b, size_a) == 0) {
        same = 1;
    }
    free(buf_a);
    free(buf_b);
    return same;
}

/*
 * Does regular file start with tag ?
 * follow: follow a symlink to its target
 * Returns 1 yes, 0 no, -1 error/missing
 */
int file_has_tag_at(int dir_fd, const char *name, bool follow, const char *tag, size_t tag_size) {
    char buf[BUFFER_SIZE];

    if (!tag || tag_size == 0 || tag_size > sizeof(buf)) {
        return -1;
    }

    if (caps_raise() != 0) {
        return -1;
    }
    int fdesc = open_regular_ro(dir_fd, name, follow ? 0 : O_NOFOLLOW);
    (void)caps_lower();
    if (fdesc < 0) {
        return -1;
    }

    ssize_t nbytes = read_full(fdesc, buf, tag_size);
    (void)close(fdesc);

    if (nbytes < 0) {
        return -1;
    }
    return ((size_t)nbytes == tag_size && memcmp(buf, tag, tag_size) == 0) ? 1 : 0;
}

/*
 * unlink with caps raised.
 */
int unlink_at_priv(int dir_fd, const char *name) {
    if (caps_raise() != 0) {
        return -1;
    }
    int ret = unlinkat(dir_fd, name, 0);
    int saved_errno = errno;
    (void)caps_lower();
    errno = saved_errno;
    return ret;
}

/*
 * rename with caps raised.
 */
int rename_at_priv(int dir_fd, const char *old_name, const char *new_name) {
    if (caps_raise() != 0) {
        return -1;
    }
    int ret = renameat(dir_fd, old_name, dir_fd, new_name);
    int saved_errno = errno;
    if (ret == 0) {
        (void)fsync(dir_fd);
    }
    (void)caps_lower();
    errno = saved_errno;
    return ret;
}

/*
 * Size of file (not following symlinks) or -1
 */
off_t file_size_at(int dir_fd, const char *name) {
    struct stat stbuf = {};

    if (caps_raise() != 0) {
        return -1;
    }
    int ret = fstatat(dir_fd, name, &stbuf, AT_SYMLINK_NOFOLLOW);
    (void)caps_lower();
    if (ret != 0) {
        return -1;
    }
    return stbuf.st_size;
}

/*
 * readlinkat() into buf (size buf_size) and nul terminate.
 * Returns length, or -1 on error / truncation.
 */
static ssize_t read_link(int dir_fd, const char *name, char *buf, size_t buf_size) {
    ssize_t len = readlinkat(dir_fd, name, buf, buf_size);
    if (len < 0) {
        return -1;
    }
    if ((size_t)len >= buf_size) {
        errno = ENAMETOOLONG;
        return -1;
    }
    buf[len] = '\0';
    return len;
}

/*
 * Create symlink dst_name -> target via temp + rename. Caller has raised caps.
 */
static CopyResult install_symlink(const char *target, int dst_dir_fd, const char *dst_name) {
    char tmp_name[PATH_MAX];

    if (snprintf(tmp_name, sizeof(tmp_name), ".tmp_%s", dst_name) >= (int)sizeof(tmp_name)) {
        return COPY_ERROR;
    }

    if (unlinkat(dst_dir_fd, tmp_name, 0) != 0 && errno != ENOENT) {
        return COPY_ERROR;
    }

    if (symlinkat(target, dst_dir_fd, tmp_name) != 0) {
        return COPY_ERROR;
    }

    if ((caps_can_chown() && fchownat(dst_dir_fd, tmp_name, 0, 0, AT_SYMLINK_NOFOLLOW) != 0) ||
        renameat(dst_dir_fd, tmp_name, dst_dir_fd, dst_name) != 0) {
        int saved_errno = errno;
        (void)unlinkat(dst_dir_fd, tmp_name, 0);
        errno = saved_errno;
        return COPY_ERROR;
    }

    (void)fsync(dst_dir_fd);
    return COPY_DONE;
}

/*
 * Copy a symlink (the link itself, not its target) src -> dst.
 * Used to preserve /etc/resolv.conf when it is a symlink
 * (e.g. systemd-resolved stub or NetworkManager).
 *
 * Returns COPY_DONE, COPY_UNCHANGED (dst already same link),
 * COPY_NO_SOURCE or COPY_ERROR.
 */
CopyResult copy_symlink_at(int src_dir_fd, const char *src_name, int dst_dir_fd, const char *dst_name) {
    char target[PATH_MAX];
    char existing[PATH_MAX];

    if (!src_name || !dst_name) {
        return COPY_ERROR;
    }

    if (caps_raise() != 0) {
        return COPY_ERROR;
    }

    CopyResult result = COPY_DONE;
    ssize_t len = read_link(src_dir_fd, src_name, target, sizeof(target));

    if (len < 0) {
        result = (errno == ENOENT) ? COPY_NO_SOURCE : COPY_ERROR;

    } else if (read_link(dst_dir_fd, dst_name, existing, sizeof(existing)) == len &&
               strcmp(existing, target) == 0) {
        result = COPY_UNCHANGED;

    } else {
        result = install_symlink(target, dst_dir_fd, dst_name);
    }

    int saved_errno = errno;
    (void)caps_lower();
    errno = saved_errno;
    return result;
}
