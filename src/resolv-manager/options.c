/*
 * SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 *
 * Determine what needs to be done.
 */
#include "resolv_manager.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_help(const char *prog_name) {
    printf("Usage: %s -t <taskname> [options]\n\n", prog_name);
    printf("Required Parameter:\n");
    printf("  -t, --task <name>      Task ('s[tandard]', 'w[ireguard]', 'm[onitor]')\n\n");
    printf("\n");
    printf("   task standard  : restores standard resolv.conf\n");
    printf("   task wireguard : saves standard resolv.conf, and installs wireguard resolv.conf\n");
    printf("   task monitor   : monitors file changes to refresh the resolv configs\n");
    printf("\n");
    printf("Optional Parameters:\n");
    printf("  -i, --iface <iface>    Wireguard interface name e.g. wg0\n");
    printf("  -d, --daemon           Run as background daemon (monitor mode)\n");
    printf("  -s, --semaphore        Watch this file and exit if file is removed (monitor mode)\n");
    printf("  -f, --file             Wireguard resolv.conf used with -t wireguard\n");
    printf("  -l, --logdir <path>    Path of log directory\n");
    printf("  -n, --numlogs <num>    Maximum number of log files (Default: 4)\n");
    printf("  -T, --testing          testing mode (development)\n");
    printf("  -h, --help             Display this help\n");
    printf("\n");
    printf("  The *wireguard* and *monitor* tasks both require the interface be provided as well.\n");

}

enum {
    BASE_10 = 10,
    MAX_NUM_LOGS = 20,
};

static Task parse_task(char *opt_in) {

    /*
     * s[tandard]
     */
    if (strncmp(opt_in, "s", 1U) == 0) {
        return INSTALL_STANDARD;

    } 
    
    /*
     * w[ireguard]
     */
    if (strncmp(opt_in, "w", 1U) == 0) {
        return INSTALL_WG;

    } 
   
    /*
     * m[onitor]
     */
    if (strncmp(opt_in, "m", 1U) == 0) {
        return MONITOR;

    } 

    (void)fprintf(stderr, "Bad: task '%s'. (Valid: restore, update, monitor)\n", opt_in);
    return NOTHING;
}

int parse_options(int argc, char **argv, Options *opts) {

    /*
     * Change default logdir to 
     * - testing : ./logs/
     *   or
     * - root - /var/log/wg-client/ 
     * - user - ./logs
     */
    *opts = (Options){};
    opts->task = NOTHING;
    opts->daemon = false;
    //opts->logdir = "./logs";
    opts->num_logfiles = 4;
    opts->log_file = nullptr;

    const struct option long_options[] = {
        {"task",      required_argument, nullptr, 't'},
        {"iface",     required_argument, nullptr, 'i'},
        {"daemon",    no_argument,       nullptr, 'd'},
        {"semaphore", required_argument, nullptr, 's'},
        {"help",      no_argument,       nullptr, 'h'},
        {"logdir",    required_argument, nullptr, 'l'},
        {"file",      required_argument, nullptr, 'f'},
        {"testing",   no_argument,       nullptr, 'T'},
        {"numfiles",  required_argument, nullptr, 'n'},
        {nullptr,     0,                 nullptr, 0  }
    };

    if (argv == nullptr || opts == nullptr) {
        return -1;
    }

    /*
     * Suppress default getopt error prints
     */
    opterr = 0;
    int opt = 0;

    while ((opt = getopt_long(argc, argv, "t:Tdl:n:hi:f:s:", long_options, nullptr)) != -1) {
        switch (opt) {
            case 't':
                if (opts->task != NOTHING) {
                    (void)fprintf(stderr, "Error: only one task allowed.\n");
                    return -1;
                }

                opts->task = parse_task(optarg);
                if (opts->task == NOTHING) {
                    (void)fprintf(stderr, "Bad: task '%s'. (Valid: s[tandard], w[ireguard], m[onitor])\n", optarg);
                    return -1;
                }
                break;

            case 'd':
                opts->daemon = true;
                break;

            case 'f':
                opts->file = optarg;
                break;

            case 'i':
                opts->iface_name = optarg;
                break;

            case 'l':
                opts->logdir = optarg;
                break;

            case 's':
                opts->semaphore = optarg;
                break;

            case 'n':
                long num = strtol(optarg, nullptr, BASE_10);
                if (num < 0) {
                    opts->num_logfiles = 0;
                } else if (num > MAX_NUM_LOGS) {
                    opts->num_logfiles = MAX_NUM_LOGS;
                } else {
                    opts->num_logfiles = (int)num;
                }
                break;

            case 'T':
                opts->testing = true;
                break;

            case 'h':
                print_help(argv[0]);
                return 1;

            case '?':
                /*
                 *  Check if the error was caused by a missing required argument string
                 */
                if (optopt == 't' || optopt == 'l' || optopt == 'n' || optopt == 'i' || optopt == 'f') {
                    (void)fprintf(stderr, "Error: -%c requires a value.\n", optopt);
                } else {
                    (void)fprintf(stderr, "Error: bad option  -%c\n", optopt);
                }
                return -1;

            default:
                return -1;
        }
    }

    /*
     * validation 
     */
    if (opts->task == NOTHING) {
        (void)fprintf(stderr, "Error: Missing task. -t <taskname>.\n");
        return -1;
    }

    if (opts->daemon && opts->task != MONITOR) {
        (void)fprintf(stderr, "Error: --daemon (-d) requires 'monitor' mode.\n");
        return -1;
    }

    return 0;
}

