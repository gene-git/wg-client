/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 */
#include "resolv_manager.h"
#include <stdio.h>

void log_close(Options *opts) {
    if (opts != nullptr && opts->log_file != nullptr) {
        (void)fclose(opts->log_file);
        opts->log_file = nullptr;
    }
}

