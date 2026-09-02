/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 *
 * If runnning as root drop privileges to nobody.
 * Retain the required capabilities
 */
#include "resolv_manager.h"
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>


/*
 * Lookup gid from groupname
 * Note: Returns false if group is nullptr.
 *
 * :param group: group name to lookup
 * :parm gid: the gid 
 * :returns: true if found, otherwise false
 */
bool gid_from_group_name(const char *group, gid_t *gid) {
    char buf[GRP_USER_NAME_SZ] = {};

    if (!group) {
        return false;
    }

    struct group grp_struct = {};
    struct group *grp_result = nullptr;

    int ret = getgrnam_r(group, &grp_struct, buf, sizeof(buf), &grp_result);

    if (ret != 0 || !grp_result) {
        (void)fprintf(stderr, "Error: lookup group name'%s'.\n", group);
        return false;
    }

    *gid = grp_struct.gr_gid;

    return true;
}

/*
 * Get group name from gid.
 *
 * group_name must be large enough
 * :param gid: gid to lookup
 * :param group_name: has group name if found
 * :param group_name_size: large enough to hold the name
 * :returns: true if found else false
 */
bool group_name_from_gid(gid_t gid, char *group_name, size_t group_name_size) {
    char buf[GRP_USER_NAME_SZ] = {};
    struct group grp_struct = {};
    struct group *grp_result = nullptr;

    int ret = getgrgid_r(gid, &grp_struct, buf, sizeof(buf), &grp_result);
     
    if (ret != 0 || !grp_result) {
        (void)fprintf(stderr, "Error: group name from gid %u.\n", (unsigned int)gid);
        return false;
    }   
    
    if (!grp_result->gr_name) {
        return false;
    }
    if (strlcpy(group_name, grp_result->gr_name, group_name_size) >= group_name_size) {
        (void)fprintf(stderr, "Error: group name larger than provided buffer %s\n", 
                grp_result->gr_name);
        return false;
    }
    return true;
}


/*
 * Lookup uid from username
 * Note: Returns false if user name not provided.
 *
 * :param user: user name to lookup
 * :param uid: the uid
 * :returns: true if found, otherwise false
 */
bool uid_from_user_name(const char *user, uid_t *uid) {
    char buf[GRP_USER_NAME_SZ] = {};

    if (!user) {
        return false;
    }

    struct passwd pwd_struct = {};
    struct passwd *pwd_result = nullptr;

    int ret = getpwnam_r(user, &pwd_struct, buf, sizeof(buf), &pwd_result);

    if (ret != 0 || !pwd_result) {
        (void)fprintf(stderr, "Error: lookup user name '%s'.\n", user);
        return false;
    }
    *uid = pwd_struct.pw_uid;
    return true;
}

/*
 * Get user name from uid.
 *
 * user_name buffer must be large enough to hold the name
 * :param gid: gid to lookup
 * :param user_name: has group name if found
 * :param user_name_size: large enough to hold the name
 * :returns: true if found else false
 */
bool user_name_from_uid(uid_t uid, char *user_name, size_t user_name_size) {

    char buf[GRP_USER_NAME_SZ] = {};
    struct passwd pwd_struct = {};
    struct passwd *pwd_result = nullptr;

    int ret = getpwuid_r(uid, &pwd_struct, buf, sizeof(buf), &pwd_result);
     
    if (ret != 0 || !pwd_result) {
        (void)fprintf(stderr, "Error: lookup user from uid %u\n", (unsigned int)uid);
        return false;
    }   
    
    if (!pwd_result->pw_name) {
        return false;
    }

    if (strlcpy(user_name, pwd_result->pw_name, user_name_size) >= user_name_size) {
        (void)fprintf(stderr, "Error: user name larger than provided buffer %s\n", 
                pwd_result->pw_name);
        return false;
    }
    return true;
}

/*
 * Get real and effective user info.
 * If real is null, then nothing is looked up and not an error.
 * Likewise if effective is null.
 *
 * :param real: fill real uid, gid, user and group
 * :param effective: fill effective uid, gid, user and group
 * :returns: true if all successful, false if some or all not found.
 */
bool get_user_info(UserInfo *real, UserInfo *effective) {

    bool status = true;

    if (real) {
        real->uid = getuid();
        real->gid = getgid();

        if (!user_name_from_uid(real->uid, real->user, sizeof(real->user))) {
            real->user[0] = '\0';
            status = false;
        }

        if (!group_name_from_gid(real->gid, real->group, sizeof(real->group))){
            real->group[0] = '\0';
            status = false;
        }
    }

    if (effective) {
        effective->uid = geteuid();
        effective->gid = getegid();

        if (!user_name_from_uid(effective->uid, effective->user, sizeof(effective->user))) {
            effective->user[0] = '\0';
            status = false;
        }

        if (!group_name_from_gid(effective->gid, effective->group, sizeof(effective->group))){
            effective->group[0] = '\0';
            status = false;
        }
    }

    return status;
}
