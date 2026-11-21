/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 *
 * uid/gid <-> user/group name lookups.
 * Buffers are sized from sysconf() and grown on ERANGE.
 */
#include "resolv_manager.h"
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

enum {
    LOOKUP_BUF_MIN = 1024,
    LOOKUP_BUF_MAX = 1024 * 1024,
};

typedef enum { BY_NAME, BY_ID } LookupKind;

static size_t initial_size(int sc_name) {
    long size = sysconf(sc_name);
    if (size < LOOKUP_BUF_MIN) {
        return LOOKUP_BUF_MIN;
    }
    return (size_t)size;
}

/*
 * passwd lookup by name or uid.
 * Returns true and fills *pwd_out / *buf_out (caller frees buf) on success.
 */
static bool pw_lookup(LookupKind kind, const char *name, uid_t uid, struct passwd *pwd_out, char **buf_out) {
    size_t size = initial_size(_SC_GETPW_R_SIZE_MAX);

    *buf_out = nullptr;
    while (size <= LOOKUP_BUF_MAX) {
        char *buf = malloc(size);
        if (!buf) {
            return false;
        }

        struct passwd *result = nullptr;
        int ret = (kind == BY_NAME)
            ? getpwnam_r(name, pwd_out, buf, size, &result)
            : getpwuid_r(uid, pwd_out, buf, size, &result);

        if (ret == 0 && result) {
            *buf_out = buf;
            return true;
        }
        free(buf);

        if (ret != ERANGE) {
            return false;
        }
        size *= 2;
    }
    return false;
}

static bool gr_lookup(LookupKind kind, const char *name, gid_t gid, struct group *grp_out, char **buf_out) {
    size_t size = initial_size(_SC_GETGR_R_SIZE_MAX);

    *buf_out = nullptr;
    while (size <= LOOKUP_BUF_MAX) {
        char *buf = malloc(size);
        if (!buf) {
            return false;
        }

        struct group *result = nullptr;
        int ret = (kind == BY_NAME)
            ? getgrnam_r(name, grp_out, buf, size, &result)
            : getgrgid_r(gid, grp_out, buf, size, &result);

        if (ret == 0 && result) {
            *buf_out = buf;
            return true;
        }
        free(buf);

        if (ret != ERANGE) {
            return false;
        }
        size *= 2;
    }
    return false;
}


/*
 * Lookup gid from groupname
 * Note: Returns false if group is nullptr.
 *
 * :param group: group name to lookup
 * :parm gid: the gid
 * :returns: true if found, otherwise false
 */
bool gid_from_group_name(const char *group, gid_t *gid) {
    struct group grp = {};
    char *buf = nullptr;

    if (!group || !gid) {
        return false;
    }

    if (!gr_lookup(BY_NAME, group, 0, &grp, &buf)) {
        (void)fprintf(stderr, "Error: lookup group name '%s'.\n", group);
        return false;
    }
    *gid = grp.gr_gid;
    free(buf);
    return true;
}

/*
 * Get group name from gid.
 *
 * :param gid: gid to lookup
 * :param group_name: has group name if found
 * :param group_name_size: large enough to hold the name
 * :returns: true if found else false
 */
bool group_name_from_gid(gid_t gid, char *group_name, size_t group_name_size) {
    struct group grp = {};
    char *buf = nullptr;
    bool status = true;

    if (!group_name || group_name_size == 0) {
        return false;
    }

    if (!gr_lookup(BY_ID, nullptr, gid, &grp, &buf)) {
        (void)fprintf(stderr, "Error: group name from gid %u.\n", (unsigned int)gid);
        return false;
    }

    if (!grp.gr_name || strlcpy(group_name, grp.gr_name, group_name_size) >= group_name_size) {
        (void)fprintf(stderr, "Error: group name for gid %u too long\n", (unsigned int)gid);
        status = false;
    }
    free(buf);
    return status;
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
    struct passwd pwd = {};
    char *buf = nullptr;

    if (!user || !uid) {
        return false;
    }

    if (!pw_lookup(BY_NAME, user, 0, &pwd, &buf)) {
        (void)fprintf(stderr, "Error: lookup user name '%s'.\n", user);
        return false;
    }
    *uid = pwd.pw_uid;
    free(buf);
    return true;
}

/*
 * Get user name from uid.
 *
 * :param uid: uid to lookup
 * :param user_name: has user name if found
 * :param user_name_size: large enough to hold the name
 * :returns: true if found else false
 */
bool user_name_from_uid(uid_t uid, char *user_name, size_t user_name_size) {
    struct passwd pwd = {};
    char *buf = nullptr;
    bool status = true;

    if (!user_name || user_name_size == 0) {
        return false;
    }

    if (!pw_lookup(BY_ID, nullptr, uid, &pwd, &buf)) {
        (void)fprintf(stderr, "Error: lookup user from uid %u\n", (unsigned int)uid);
        return false;
    }

    if (!pwd.pw_name || strlcpy(user_name, pwd.pw_name, user_name_size) >= user_name_size) {
        (void)fprintf(stderr, "Error: user name for uid %u too long\n", (unsigned int)uid);
        status = false;
    }
    free(buf);
    return status;
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

        if (!group_name_from_gid(real->gid, real->group, sizeof(real->group))) {
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

        if (!group_name_from_gid(effective->gid, effective->group, sizeof(effective->group))) {
            effective->group[0] = '\0';
            status = false;
        }
    }

    return status;
}
