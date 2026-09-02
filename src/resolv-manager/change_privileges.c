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
#include <sys/capability.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>


/*
 * Restore the 2 required capabilities to the effective set
 * since setresuid cleared them.
 * Only restore these caps: CAP_DAC_OVERRIDE, CAP_CHOWN
 * Even if process has additional caps.
 */
static int restore_caps() {

    cap_t caps = cap_get_proc();
    if (!caps) {
        (void)fprintf(stderr, "Error: cap_get_proc failed.\n");
        return -1;
    }

    cap_value_t cap_allowed_list[] = { CAP_DAC_OVERRIDE, CAP_CHOWN };
    int num_allowed_caps = sizeof(cap_allowed_list) / sizeof(cap_allowed_list[0]);

    cap_flag_value_t is_permitted = {};

    for (int i = 0; i < num_allowed_caps; i++) {
        cap_value_t cap_num = cap_allowed_list[i];

        if (cap_get_flag(caps, cap_num, CAP_PERMITTED, &is_permitted) == 0) {
            if (is_permitted == CAP_SET) {
                if (cap_set_flag(caps, CAP_EFFECTIVE, 1, &cap_num, CAP_SET) != 0) {
                    (void)fprintf(stderr, "Error: cap_set_flag failed for capability %d\n", (int)cap_num);
                    cap_free(caps);
                    return -1;
                }
            }
        }
    }

    /*
     * Restore all the existing caps to the process
     */
    if (cap_set_proc(caps) != 0) {
        (void)fprintf(stderr, "Error: cap_set_proc failed to raise effective caps.\n");
        (void)cap_free(caps);
        return -1;
    }

    if (cap_free(caps) != 0) {
        (void)fprintf(stderr, "Warning: cap_free error.\n");
    }

    return 0;
}

/*
 * Lookup uid/gid from username/groupname
 * If user/group are null then return the real uid/gid.
 */
static bool lookup_uid_gid(const char *user, const char *group, uid_t *uid, gid_t *gid) {

    if (group && !gid_from_group_name(group, gid)) {
        *gid = getgid();
    }

    if (user && !uid_from_user_name(user, uid)) {
        *uid = getuid();
    }

    return true;
}

int change_privileges(Config *conf) {

    Perms *perms = {};

    if (!conf) {
        return -1;
    }
    perms = &conf->perms;

    /*
     * Only applies if running as root.
     */
    perms->euid = geteuid();
    if (perms->euid != 0) {
        return 0;
    }

    /*
     * Get unprivileged uid/gid
     */
    if (!perms->user_run && !perms->group_run) {
        return 0; 
    }

    uid_t uid_run = getuid();
    gid_t gid_run = getgid();

    if (!lookup_uid_gid(perms->user_run, perms->group_run, &uid_run, &gid_run)) {
        return -1;
    }

    /*
     * preserve (file) capabilities
     */
    if (prctl(PR_SET_KEEPCAPS, 1L, 0L, 0L, 0L) != 0) {
        (void)fprintf(stderr, "Error: failed to keep caps using PR_SET_KEEPCAPS.\n");
        return -1;
    }

    /*
     * Clear any supplemental groups
     */
    if (perms->euid == 0 && getgroups(0, nullptr) > 0 && setgroups(0, nullptr) != 0) {
        (void)fprintf(stderr, "Error: clearing supplemental groups.\n");
        return -1;
    }

    /*
     * Drop to unpriv user/group 
     * - only attempt to change uid/gid if provided user/group
     */
    if (perms->group_run) {
        if (setresgid(gid_run, gid_run, gid_run) != 0) {
            (void)fprintf(stderr, "Error: setting gid: %u\n", (unsigned int)gid_run);
            return -1;
        }
    }

    if (perms->user_run) {
        if (setresuid(uid_run, uid_run, uid_run) != 0) {
            (void)fprintf(stderr, "Error: setting uid: %u\n", (unsigned int)uid_run);
            return -1;
        }
    }

    log_msg(&conf->opts, " Success - privs dropped to %u:%u", 
            (unsigned int)uid_run, (unsigned int)gid_run);

    /*
     * Restore required capabilities to the effective set
     * because setresuid cleared them.
     * Only restore these caps: CAP_DAC_OVERRIDE, CAP_CHOWN
     * Even if process has additional caps.
     */
    if (restore_caps() < 0) {
        log_msg(&conf->opts, " Error - caps not restored");
        return -1;
    }
    log_msg(&conf->opts, " Success - caps restored");

    return 0;
}

