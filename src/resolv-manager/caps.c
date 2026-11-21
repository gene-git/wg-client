/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 *
 * Capability management.
 *
 * The only capabilities this program ever needs are CAP_CHOWN and
 * CAP_DAC_OVERRIDE, and only while writing files in /etc.
 *
 * Model:
 *  - caps_restrict(): called by change_privileges() after switching from root
 *    to the unprivileged run user. Permitted set is reduced to (at most) the
 *    2 allowed capabilities, effective and inheritable sets are cleared,
 *    ambient set is cleared and no_new_privs is set. From then on the
 *    process is "managed".
 *  - caps_raise() / caps_lower(): copy the allowed permitted caps into the
 *    effective set / clear the effective set. Used to bracket only those
 *    operations that need them.
 *  - caps_drop_all(): testing mode as a normal user - no capabilities at all.
 *
 * Until restricted (i.e. still root), raise/lower are no-ops.
 */
#include "resolv_manager.h"
#include <errno.h>
#include <stdio.h>
#include <sys/capability.h>
#include <sys/prctl.h>
#include <unistd.h>

static const cap_value_t cap_allowed_list[] = { CAP_CHOWN, CAP_DAC_OVERRIDE };
enum {
    NUM_ALLOWED_CAPS = sizeof(cap_allowed_list) / sizeof(cap_allowed_list[0]),
};

/*
 * true once we control the effective set (restricted or dropped).
 * Process wide state by nature: the capability sets are per process.
 */
static bool caps_managed = false;   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

/*
 * Clear ambient and set no_new_privs.
 */
static int caps_lockdown() {

    /*
     * EINVAL means kernel without ambient caps - nothing to clear.
     */
    if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_CLEAR_ALL, 0L, 0L, 0L) != 0 && errno != EINVAL) {
        (void)fprintf(stderr, "Error: failed clearing ambient capabilities.\n");
        return -1;
    }

    if (prctl(PR_SET_NO_NEW_PRIVS, 1L, 0L, 0L, 0L) != 0) {
        (void)fprintf(stderr, "Error: failed setting no_new_privs.\n");
        return -1;
    }
    return 0;
}

/*
 * Copy the allowed caps that are set in 'src' (flag src_flag) into
 * 'dst' (flag dst_flag).
 */
static int copy_allowed_caps(cap_t src, cap_flag_t src_flag, cap_t dst, cap_flag_t dst_flag) {

    for (int i = 0; i < NUM_ALLOWED_CAPS; i++) {
        cap_value_t cap_num = cap_allowed_list[i];
        cap_flag_value_t is_set = CAP_CLEAR;

        if (cap_get_flag(src, cap_num, src_flag, &is_set) != 0) {
            return -1;
        }

        if (is_set == CAP_SET && cap_set_flag(dst, dst_flag, 1, &cap_num, CAP_SET) != 0) {
            return -1;
        }
    }
    return 0;
}

/*
 * Reduce permitted to the allowed caps (those we currently have),
 * clear effective and inheritable.
 */
int caps_restrict() {
    int status = 0;
    cap_t cur = cap_get_proc();
    cap_t want = cap_init();

    if (!cur || !want) {
        (void)fprintf(stderr, "Error: cap_get_proc/cap_init failed.\n");
        status = -1;

    } else if (copy_allowed_caps(cur, CAP_PERMITTED, want, CAP_PERMITTED) != 0) {
        (void)fprintf(stderr, "Error: failed building restricted capability set.\n");
        status = -1;

    } else if (cap_set_proc(want) != 0) {
        (void)fprintf(stderr, "Error: cap_set_proc failed restricting capabilities.\n");
        status = -1;

    } else if (caps_lockdown() != 0) {
        status = -1;

    } else {
        caps_managed = true;
    }

    if (cur) {
        (void)cap_free(cur);
    }
    if (want) {
        (void)cap_free(want);
    }
    return status;
}

/*
 * Drop every capability (e.g. testing mode as a normal user).
 */
int caps_drop_all() {
    int status = 0;
    cap_t none = cap_init();

    if (!none) {
        return -1;
    }

    if (cap_set_proc(none) != 0) {
        (void)fprintf(stderr, "Error: cap_set_proc failed dropping capabilities.\n");
        status = -1;
    }
    (void)cap_free(none);

    if (status == 0 && caps_lockdown() != 0) {
        status = -1;
    }
    if (status == 0) {
        caps_managed = true;
    }
    return status;
}

/*
 * Set effective = allowed & permitted (raise=true) or clear effective (raise=false)
 */
static int caps_set_effective(bool raise) {
    int status = 0;

    if (!caps_managed) {
        return 0;
    }

    cap_t caps = cap_get_proc();
    if (!caps) {
        return -1;
    }

    if (cap_clear_flag(caps, CAP_EFFECTIVE) != 0 ||
        (raise && copy_allowed_caps(caps, CAP_PERMITTED, caps, CAP_EFFECTIVE) != 0) ||
        cap_set_proc(caps) != 0) {
        status = -1;
    }

    (void)cap_free(caps);
    return status;
}

int caps_raise() {
    return caps_set_effective(true);
}

int caps_lower() {
    return caps_set_effective(false);
}

/*
 * Can we chown files to root ?
 */
bool caps_can_chown() {
    if (!caps_managed) {
        return geteuid() == 0;
    }

    bool can = false;
    cap_t caps = cap_get_proc();
    if (caps) {
        cap_flag_value_t val = CAP_CLEAR;
        if (cap_get_flag(caps, CAP_CHOWN, CAP_PERMITTED, &val) == 0 && val == CAP_SET) {
            can = true;
        }
        (void)cap_free(caps);
    }
    return can;
}
