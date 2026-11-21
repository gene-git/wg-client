/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 *
 * Check which of the capabilities we need are available.
 * After dropping privileges we need cap_chown and cap_dac_override.
 */
#include "resolv_manager.h"
#include <stdio.h>
#include <sys/capability.h>
#include <unistd.h>


/*
 * Check caps/perms
 * - checks the permitted set; caps are raised to effective only when needed.
 */
int check_permissions(Perms *perms) {
    cap_flag_value_t cap_val = CAP_CLEAR;

    if (!perms) {
        return -1;
    }

    perms->euid = geteuid();
    perms->cap_chown = false;
    perms->cap_dac_override = false;

    if (perms->euid == 0) {
        perms->cap_chown = true;
        perms->cap_dac_override = true;
        return 0;
    }

    cap_t cap = cap_get_proc();
    if (!cap) {
        perror("cap_get_proc");
        return -1;
    }

    int status = 0;
    if (cap_get_flag(cap, CAP_CHOWN, CAP_PERMITTED, &cap_val) != 0) {
        status = -1;
    } else if (cap_val == CAP_SET) {
        perms->cap_chown = true;
    }

    if (cap_get_flag(cap, CAP_DAC_OVERRIDE, CAP_PERMITTED, &cap_val) != 0) {
        status = -1;
    } else if (cap_val == CAP_SET) {
        perms->cap_dac_override = true;
    }

    if (cap_free(cap) != 0) {
        (void)fprintf(stderr, "Warning: failed to free cap_t\n");
    }
    return status;
}
