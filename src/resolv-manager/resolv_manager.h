/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 *
 * Managing /etc/resolv.conf for wireguard
 */
#ifndef RESOLV_MANAGER_H
#define RESOLV_MANAGER_H

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

/*
 * track permissions / capabilities
 */
typedef struct perms {
    uid_t euid ;
    bool cap_chown ;
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
 * - semaphore = if set, watch and exit monitor when this file goes away (HOME/.local/state/wg-client/run_semaphore)
 */
typedef struct options {
    Task task;
    const char *iface_name;
    bool daemon;
    bool testing;
    const char *file;
    const char *semaphore;

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
 * And monitor: 
 *
 *  - /sys/class/net/<iface> 
 *    wireguard interface <iface> ~ wg0, wgc, etc
 *  
 *  - <semaphore>
 */
typedef struct dir_target {
    char *path;
    int fd;
} DirTarget;

typedef struct file_target {
    const char *name;
} FileTarget;

typedef struct dyn_file_target {
    char *name;
} DynFileTarget;

/*
 * User info
 * - can be real or effective or other
 */
enum {GRP_USER_NAME_SZ = 1024U,};

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
 * - save_name = copy of curreent standard resolv.conf (/etc/resolv.conf.saved)
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

    DirTarget sem;
    DynFileTarget semaphore;

    const char *tag_wg;
    size_t tag_wg_size;

    UserInfo    user_real;
    UserInfo    user_effective;
    UserInfo    user_run;

} Config;


/*
 * Locking
 */
typedef struct locking {
    const char *dir;
    const char *file;
    int dir_fd;
    int fd;
    bool acquired;
    bool locked_by_other;
} Locking;

/*
 * Copy File with tag support.
 * - TAG_ADD:
 *   output file has tag added to start of file
 * - TAG_REQUIRED
 *   output only written if input has the tag at start of file
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

} CopyFile;

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

int check_permissions(Perms *perms); 
int chown_root(Config *conf, int dir_fd, const char *file);
int change_privileges(Config *config);

bool gid_from_group_name(const char *group, gid_t *gid);
bool group_name_from_gid(gid_t gid, char *group_name, size_t group_name_size);
bool uid_from_user_name(const char *user, uid_t *uid);
bool user_name_from_uid(uid_t uid, char *user_name, size_t user_name_size);
bool get_user_info(UserInfo *real, UserInfo *effective);

int save_wg_resolv(Config *conf, const char *src_file);
int save_standard_resolv(Config *conf);
int install_wg_resolv(Config *conf);
int install_standard_resolv(Config *conf);

int copy_file_at(CopyFile *copy);

int daemonize(void);
int monitor_loop(Config *conf);

bool acquire_lock(Locking *locking);
void release_lock(Locking *locking);
bool initialize_lock(Config *conf, Locking *locking);
bool initialize(int argc, char **argv, Config *conf);


#endif // RESOLV_MONITOR_H

