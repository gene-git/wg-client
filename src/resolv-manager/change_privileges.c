/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 *
 * If running as root drop privileges to the unprivileged run user (nobody).
 * Retain only CAP_CHOWN and CAP_DAC_OVERRIDE (permitted only - they
 * are raised to effective just when needed via caps_raise()).
 */
#include "resolv_manager.h"
#include <errno.h>
#include <grp.h>
#include <sys/capability.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>


/*
 * Remove everything but the allowed caps from the bounding set.
 * Requires CAP_SETPCAP (i.e. must be done while still root).
 * Not fatal - permitted set is restricted later regardless.
 */
static void drop_bounding_set(Config *conf) {
    int max_bits = (int)cap_max_bits();

    for (int cap = 0; cap < max_bits; cap++) {
        if (cap == CAP_CHOWN || cap == CAP_DAC_OVERRIDE) {
            continue;
        }
        if (cap_drop_bound((cap_value_t)cap) != 0 && errno != EINVAL) {
            log_msg(&conf->opts, " Warning - failed to drop cap %d from bounding set", cap);
        }
    }
}

/*
 * Lookup uid/gid from username/groupname
 * Failure to find either is an error: we must never silently remain root.
 */
static bool lookup_uid_gid(Config *conf, const char *user, const char *group, uid_t *uid, gid_t *gid) {

    if (!user || !group) {
        return false;
    }

    if (!gid_from_group_name(group, gid)) {
        log_msg(&conf->opts, " Error - unknown group %s", group);
        return false;
    }

    if (!uid_from_user_name(user, uid)) {
        log_msg(&conf->opts, " Error - unknown user %s", user);
        return false;
    }

    if (*uid == 0 || *gid == 0) {
        log_msg(&conf->opts, " Error - run user/group %s:%s resolves to root", user, group);
        return false;
    }

    return true;
}

/*
 * Verify real, effective and saved uid/gid are all uid/gid
 */
static bool ids_are(uid_t uid, gid_t gid) {
    uid_t ruid = 0;
    uid_t euid = 0;
    uid_t suid = 0;
    gid_t rgid = 0;
    gid_t egid = 0;
    gid_t sgid = 0;

    if (getresuid(&ruid, &euid, &suid) != 0 || getresgid(&rgid, &egid, &sgid) != 0) {
        return false;
    }
    if (ruid != uid || euid != uid || suid != uid) {
        return false;
    }
    if (rgid != gid || egid != gid || sgid != gid) {
        return false;
    }
    return true;
}

int change_privileges(Config *conf) {

    if (!conf) {
        return -1;
    }
    Perms *perms = &conf->perms;

    /*
     * Only applies if running as root.
     */
    perms->euid = geteuid();
    if (perms->euid != 0) {
        return 0;
    }

    uid_t uid_run = 0;
    gid_t gid_run = 0;

    if (!lookup_uid_gid(conf, perms->user_run, perms->group_run, &uid_run, &gid_run)) {
        return -1;
    }

    /*
     * Shrink bounding set while we still have CAP_SETPCAP
     */
    drop_bounding_set(conf);

    /*
     * preserve permitted capabilities across setresuid()
     */
    if (prctl(PR_SET_KEEPCAPS, 1L, 0L, 0L, 0L) != 0) {
        log_msg(&conf->opts, " Error - failed to keep caps using PR_SET_KEEPCAPS");
        return -1;
    }

    /*
     * Clear any supplemental groups
     */
    if (setgroups(0, nullptr) != 0) {
        log_msg(&conf->opts, " Error - clearing supplemental groups");
        return -1;
    }

    /*
     * Drop to unpriv group then user.
     */
    if (setresgid(gid_run, gid_run, gid_run) != 0) {
        log_msg(&conf->opts, " Error - setting gid: %u", (unsigned int)gid_run);
        return -1;
    }

    if (setresuid(uid_run, uid_run, uid_run) != 0) {
        log_msg(&conf->opts, " Error - setting uid: %u", (unsigned int)uid_run);
        return -1;
    }

    if (!ids_are(uid_run, gid_run)) {
        log_msg(&conf->opts, " Error - uid/gid not changed as expected");
        return -1;
    }

    if (prctl(PR_SET_KEEPCAPS, 0L, 0L, 0L, 0L) != 0) {
        log_msg(&conf->opts, " Error - failed to clear PR_SET_KEEPCAPS");
        return -1;
    }

    /*
     * Permitted <- {CAP_CHOWN, CAP_DAC_OVERRIDE}
     * Effective / inheritable / ambient cleared. no_new_privs set.
     */
    if (caps_restrict() != 0) {
        log_msg(&conf->opts, " Error - failed to restrict capabilities");
        return -1;
    }

    perms->euid = geteuid();
    log_msg(&conf->opts, " Success - privs dropped to %u:%u (caps: chown, dac_override)",
            (unsigned int)uid_run, (unsigned int)gid_run);

    return 0;
}
