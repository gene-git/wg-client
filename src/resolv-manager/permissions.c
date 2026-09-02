/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 *
 * Restore /etc/resolv.con by copying from saved /etc/resolv.conf.wg
 * Save any newly found version of file as resolv.conf.save. This handles
 * the case when machine is sleep/resumed or dhcp lease is refreshed or
 * even a new wifi location - each leading to a new resolv.conf - if its different
 * than current saved version then it replaces it.
 *
 * Requires :
 *  - cap_chown and cap_dac_override if not run as root.
 *  - openssl to compute file hash for fast compare
 *
 * Since resolv.conf files are tiny they are read into memory.
 */
#include "resolv_manager.h"
#include <linux/capability.h>
#include <stdio.h>
#include <sys/capability.h>
#include <unistd.h>


/*
 * Check caps/perms
 */
int check_permissions(Perms *perms) 
{
    cap_t cap = {};
    cap_flag_value_t cap_val = {};

    perms->euid = geteuid() ;
    perms->cap_chown = false ;
    perms->cap_dac_override = false ;

    if (perms->euid == 0) {
        perms->cap_chown = true;
        perms->cap_dac_override = true ;
        return 0;
    }

	cap = cap_get_pid(0);
	if (!cap) {
		perror("cap_get_pid");
		return -1;
	}

    cap_get_flag(cap, CAP_CHOWN, CAP_PERMITTED, &cap_val);
    if (cap_val == CAP_SET) {
        perms->cap_chown = true ;
    }

    cap_get_flag(cap, CAP_DAC_OVERRIDE, CAP_PERMITTED, &cap_val);
    if (cap_val == CAP_SET) {
        perms->cap_dac_override = true ;
    }

    if (cap_free(cap) != 0) {
        (void)fprintf(stderr, "Warning: failed to free cap_t\n");
    }
    return 0 ;
}

/*
 * Chown to root if possible
 * Switch to fchownat(fd, file, uid, gid, 0)
 * For all files in etc fd = conf->etc.fd
 */
int chown_root(Config *conf, int dir_fd, const char *file) {
    Perms *perms = &conf->perms;
    int flags = 0;
    uid_t uid = 0;
    gid_t gid = 0;

    if (perms->euid == 0 || perms->cap_chown) {
        if (fchownat(dir_fd, file, uid, gid, flags) != 0) {
            (void)fprintf(stderr, "Failed chown root : %s\n", file);
            return -1;
        }
    }
    return 0;
}
