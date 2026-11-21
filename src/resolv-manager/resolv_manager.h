/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 *
 * Managing /etc/resolv.conf for wireguard
 *
 * Run as root from the wireguard PostUp / PostDown scripts.
 * Long running (monitor) instances drop to nobody keeping only
 * CAP_CHOWN and CAP_DAC_OVERRIDE.
 */
#ifndef RESOLV_MANAGER_H
#define RESOLV_MANAGER_H

#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

/*
 * track permissions / capabilities
 */
typedef struct perms {
    uid_t euid;
    bool cap_chown;
    bool cap_dac_override;
    const char *user_run;
    const char *group_run;
} Perms;

/*
 * Options and Tasks
 */
typedef enum {
    NOTHING = 0,
    INSTALL_WG = 1,
    INSTALL_STANDARD = 2,
    MONITOR = 3,
} Task;

/*
 * - task : install_wg, install_standard, monitor
 * - iface_name: wireguard interface name
 */
typedef struct options {
    Task task;
    const char *iface_name;
    bool daemon;
    bool testing;
    const char *file;

    const char *logdir;
    int num_logfiles;
    FILE *log_file;

} Options;


/*
 * We manage 3 files in /etc
 * - resolv.conf
 * - resolv.conf.saved
 * - resolv.conf.wg
 *
 * And monitor the wireguard interface <iface> ~ wg0, wgc, etc
 * (rtnetlink + /sys/class/net/<iface>)
 */
typedef struct dir_target {
    char *path;
    int fd;
} DirTarget;

typedef struct file_target {
    const char *name;
} FileTarget;

/*
 * User info
 * - can be real or effective or other
 */
enum {
    GRP_USER_NAME_SZ = 1024U,
};

typedef struct user_info {
    uid_t   uid;
    gid_t   gid;

    char group[GRP_USER_NAME_SZ];
    char user[GRP_USER_NAME_SZ];

} UserInfo;

/*
 * Top level configuration.
 * - filenames, wireguard interface, options etc.
 * - iface is wireguard interface name (e.g. wg0)
 * - wg_name = copy of wireguard resolv.conf (/etc/resolv.conf.wg)
 * - save_name = copy of current standard resolv.conf (/etc/resolv.conf.saved)
 * - resolv_name = active resolv.conf (/etc/resolv.conf)
 */
typedef struct config {

    Options opts;
    Perms perms;

    DirTarget sys_net;
    FileTarget iface;

    DirTarget etc;
    FileTarget wg;
    FileTarget save;
    FileTarget resolv;

    const char *tag_wg;
    size_t tag_wg_size;

    UserInfo    user_real;
    UserInfo    user_effective;

} Config;


/*
 * Locking
 * - err: errno of the last failure (0 if none)
 */
typedef struct locking {
    const char *dir;
    const char *file;
    int dir_fd;
    int fd;
    int err;
    bool acquired;
    bool locked_by_other;
} Locking;

/*
 * Copy File with tag support.
 * - TAG_ADD:
 *   output file has tag added to start of file
 * - TAG_REQUIRED
 *   output only written if input has the tag at start of file
 * - TAG_FORBIDDEN
 *   output only written if input does not have the tag
 *
 * Used to tag the wireguard resolv.conf so it is never mixed up with
 * the standard one. Just dont tag the standard one please!
 */

typedef struct copy_file {
    int src_dir_fd;
    const char *src_name;

    int dst_dir_fd;
    const char *dst_name;

    bool tag_ignore;
    bool tag_add;
    bool tag_required;
    bool tag_forbidden;

    const char *tag;
    size_t tag_size;

    /*
     * forbid_empty: do not copy an empty source (COPY_EMPTY_SOURCE).
     *   An empty resolv.conf is never a valid standard config - typically
     *   it was read while another process was rewriting it in place.
     */
    bool forbid_empty;

    /*
     * displaced_name: if set, the destination is replaced with an atomic
     *   exchange (renameat2 RENAME_EXCHANGE) and the file it replaced is
     *   left as dst_dir/displaced_name (displaced = true) for the caller to
     *   inspect and remove. Lets the caller see anything written to the
     *   destination after it was last read.
     */
    const char *displaced_name;
    bool displaced;

} CopyFile;

/*
 * Result of copy_file_at() / copy_symlink_at()
 */
typedef enum {
    COPY_ERROR       = -1,  /* error - see errno / log */
    COPY_DONE        =  0,  /* destination written */
    COPY_SKIPPED_TAG =  1,  /* not written: tag constraints */
    COPY_NO_SOURCE   =  2,  /* not written: source does not exist */
    COPY_UNCHANGED   =  3,  /* not written: destination already identical */
    COPY_EMPTY_SOURCE = 4,  /* not written: source empty (forbid_empty) */
} CopyResult;

/*
 * Public functions
 * Application is statically compiled since there is little to no point using a library.
 */
int parse_options(int argc, char **argv, Options *opts);
void config_clean(Config *conf);

int makedir_if_needed(const char *path);
int log_init(Options *opts);
void log_msg(Options *opts, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void log_close(Options *opts);
const char *errno_str(int errnum);

/*
 * capabilities (caps.c)
 */
[[nodiscard]] int caps_restrict();
[[nodiscard]] int caps_drop_all();
[[nodiscard]] int caps_raise();
int caps_lower();
bool caps_can_chown();

int check_permissions(Perms *perms);
[[nodiscard]] int change_privileges(Config *conf);

bool gid_from_group_name(const char *group, gid_t *gid);
bool group_name_from_gid(gid_t gid, char *group_name, size_t group_name_size);
bool uid_from_user_name(const char *user, uid_t *uid);
bool user_name_from_uid(uid_t uid, char *user_name, size_t user_name_size);
bool get_user_info(UserInfo *real, UserInfo *effective);

int save_wg_resolv(Config *conf, const char *src_file);
int save_standard_resolv(Config *conf);
int save_standard_from(Config *conf, const char *src_name);
int install_wg_resolv(Config *conf);
int install_standard_resolv(Config *conf);

[[nodiscard]] CopyResult copy_file_at(CopyFile *copy);
[[nodiscard]] CopyResult copy_symlink_at(int src_dir_fd, const char *src_name, int dst_dir_fd, const char *dst_name);
int is_symlink_at(int dir_fd, const char *name);
int files_identical_at(int dir_fd, const char *name_a, const char *name_b);
int file_has_tag_at(int dir_fd, const char *name, bool follow, const char *tag, size_t tag_size);
int unlink_at_priv(int dir_fd, const char *name);
int rename_at_priv(int dir_fd, const char *old_name, const char *new_name);
off_t file_size_at(int dir_fd, const char *name);

int daemonize();
int monitor_loop(Config *conf);
bool iface_exists(Config *conf);

[[nodiscard]] bool acquire_lock(Locking *locking);
void release_lock(Locking *locking);
[[nodiscard]] bool initialize_lock(Config *conf, Locking *locking);
[[nodiscard]] int initialize(int argc, char **argv, Config *conf);


#endif // RESOLV_MANAGER_H
