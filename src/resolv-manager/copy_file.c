/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 *
 */
#include "resolv_manager.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum {BUFFER_SIZE = 4096,};

struct Info {
    ssize_t bytes_read;
    char buffer[BUFFER_SIZE];

    int src_fd;
    int dst_fd;

    bool tag_active;
    bool tag_matches;

    char tmp_name[PATH_MAX];
    int read_flags;
    int write_flags;
    int write_flags_create;
    mode_t file_perms;
};

/*
 * Check tag info
 * sets info->tag_amtches and return it where it is:
 *  - true if tag in source matchaes copy->tag
 *  - other false.
 *
 * If there aer any tag restrictions at all, then the tag must be passed.
 * If a tag is forbidden for example, the tag must be known to check if it
 * present when it should not be for example.
 *
 * If there is no copy->tag then it should not matter as that only applies
 * to when tags are ignored.
 */
static bool source_has_tag(CopyFile *copy, struct Info *info) {

    /*
     * When no tag required its fine not to have a tag in source
     * For example when restoring (non-tagged) standard resolv.conf
     */
    info->tag_matches = false;
    if (!copy->tag || copy->tag_size == 0) {
        return false;
    }
    /* 
     * Safety check: ensure tag isn't absurdly large 
     * - by construction tag is always < 128 bytes.
     */
    if (copy->tag_size > BUFFER_SIZE) {
        return false;
    }

    /* 
     * If file is smaller than the tag, it obviously doesn't match 
     */
    if ((size_t)info->bytes_read >= copy->tag_size && memcmp(info->buffer, copy->tag, copy->tag_size) == 0) {
        info->tag_matches = true;
    }

    return info->tag_matches;
}


/*
 * Create temp file in same directory (so rename is atomic)
 * If tmp file happens to exist we truncate it
 */
static bool create_temp_file(CopyFile *copy, struct Info *info) {

    if (snprintf(info->tmp_name, PATH_MAX, ".tmp_%s", copy->dst_name) >= PATH_MAX) {
        return false;
    }

    /*
     * Check if tmp file exists - if not create it.
     */
    info->dst_fd = openat(copy->dst_dir_fd, info->tmp_name, info->write_flags, info->file_perms);
    if (info->dst_fd < 0 && errno == ENOENT) {
        info->dst_fd = openat(copy->dst_dir_fd, info->tmp_name, info->write_flags_create, info->file_perms);
    }

    if (info->dst_fd < 0) {
        return false;
    }
    return true;
}


/*
 * Write temp file
 * - handle adding tag if needed
 */
static int write_temp_file(CopyFile *copy, struct Info *info) {
    int result = 0;
    /*
     * copy->tag_add:
     * - add tag to front of file if not already there.
     */
    if (info->tag_active && copy->tag_add && !info->tag_matches) {
        if (write(info->dst_fd, copy->tag, copy->tag_size) != (ssize_t)copy->tag_size) {
            result = -1;
        }
    }

    /*
     * Write the first chunk that we already read
     */
    if (result == 0 && info->bytes_read > 0) {
        if (write(info->dst_fd, info->buffer, (size_t)info->bytes_read) != info->bytes_read) {
            result = -1;
        }
    }

    /*
     * Write remainder of file
     */
    if (result == 0) {
        while ((info->bytes_read = read(info->src_fd, info->buffer, sizeof(info->buffer))) > 0) {
            if (write(info->dst_fd, info->buffer, (size_t)info->bytes_read) != info->bytes_read) {
                result = -1;
                break;
            }
        }

        if (info->bytes_read < 0) {
            result = -1;
        }
    }

    return result;
}


/*
 * Okay to write output if:
 * a) tags are ignored
 * a) tags active and src has matching tag and: tag_required or tag_add
 * b) tags active and src has no tag and: has_tag is false
 */
static bool check_okay_to_write(CopyFile *copy, struct Info *info) {

    /*
     * has_tag same as info->tag_matches
     */
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
 * Copy file src_dir/src_name to dst_dir/dst_name
 * Use openat() so requires open directory fd for each.
 *
 * Files are read/written in 4k chunks
 * Write file permissions are set to 0644 (rw-r-r)
 *
 * Note if program is killed during a read/write operation files can be corrupted.
 * So its writes to a tmp file uses atomic rename.
 *
 * Tags tag_required refers to source file::
 *   
 *   tag_required    add     dst
 *   ------     ----    ---------------------
 *   yes        yes     dst is tagged 
 *   no         yes     dst is tagged 
 *   no         no      dst only if src not tagged     
 *   yes        no      dst only if src is  tagged
 *
 * Update file -> resolv.wg    (no, yes)
 * Save resolv -> resol.saved  (no, no)
 * resolv.wg -> resolv         (yes, yes) or possibly (no, yes) for backward compat
 * saved -> resolv             (no, no)
 *
 * if source tagged dst 
 *  -> written if (y,y) (n,y) (y,n)  
 *  -> not written (n, n)
 *
 * source not tagged:
 *  -> written if (no, no) (no, yes)
 *
 * Standard resolv.conf are never tagged.
 * Wireguard configs are always tagged.
 *
 * Note file output is written to a temporary file and renamed to ensure 
 * atomic write.
 *
 * Returns:
 * -1  = error
 *  0  = copy done
 *  1  = copy not done because tag constraints prevented it 
 */
int copy_file_at(CopyFile *copy) {

    struct Info info = {};

    info.read_flags = O_RDONLY | O_CLOEXEC;
    info.write_flags = O_WRONLY | O_TRUNC | O_CLOEXEC;
    info.write_flags_create = O_WRONLY | O_TRUNC | O_CREAT | O_CLOEXEC;
    info.file_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    if (!copy) {
        return -1;
    }

    if (!copy->src_name || !copy->dst_name ) {
        return -1;
    }

    /*
     * Open source file
     */
    info.src_fd = openat(copy->src_dir_fd, copy->src_name, info.read_flags);
    if (info.src_fd < 0) {
        return -1;
    }

    int result = 0;

    /*
     * Caller can set tag_ignore to ensure files are copied
     * independent of any tag(s)
     * When tags are active, they must be prvided.
     */
    info.tag_active = false;
    if (!copy->tag_ignore) {
        info.tag_active = true;

        /*
         * tag is required when active
         */
        if (!(copy->tag && copy->tag_size > 0)) {
            return -1;
        }
    }

    /*
     * Read first chunk BEFORE creating the temp destination file.
     * This avoids creating empty tmp files if tag_required fails.
     */
    info.bytes_read = read(info.src_fd, info.buffer, sizeof(info.buffer));
    if (info.bytes_read < 0) {
        (void)close(info.src_fd);
        return -1;
    }

    /*
     * Tag handling (first chunk)
     */
    info.tag_matches = false;
    if (info.tag_active) {
        if (!check_okay_to_write(copy, &info)) {
            (void)close(info.src_fd);
            return 0;
        }
    }
    

    /*
     * Create temp file to write to
     */
    if (!create_temp_file(copy, &info) || info.dst_fd < 0) {
        (void)close(info.src_fd);
        return -1;
    }


    /*
     * Write the file
     */
    result = write_temp_file(copy, &info);

    /*
     * sync to disk
     */
    if (result == 0 && fsync(info.dst_fd) != 0) {
        result = -1;
    }

    (void)close(info.src_fd);
    (void)close(info.dst_fd);

    /*
     * Atomic rename
     */
    if (result == 0) {
        if (renameat(copy->dst_dir_fd, info.tmp_name, copy->dst_dir_fd, copy->dst_name) != 0) {
            result = -1;
        }
    } else {
        /*
         * Clean up
         */
        unlinkat(copy->dst_dir_fd, info.tmp_name, 0);
    }

    return result;
}

