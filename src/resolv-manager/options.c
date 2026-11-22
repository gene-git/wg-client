/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
 *
 * Determine what needs to be done.
 */
#include "resolv_manager.h"
#include <errno.h>
#include <getopt.h>     // IWYU pragma: keep  (getopt_long, struct option)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_help(const char *prog_name) {
    printf("Usage: %s -t <taskname> [options]\n\n", prog_name);
    printf("Required Parameter:\n");
    printf("  -t, --task <name>      Task ('s[tandard]', 'w[ireguard]', 'm[onitor]')\n");
    printf("\n");
    printf("   task standard  : restores standard resolv.conf\n");
    printf("   task wireguard : saves standard resolv.conf, and installs wireguard resolv.conf\n");
    printf("   task monitor   : installs wireguard resolv.conf and keeps it installed until the\n");
    printf("                    interface goes away or SIGTERM - then restores standard resolv.conf\n");
    printf("\n");
    printf("Optional Parameters:\n");
    printf("  -i, --iface <iface>    Wireguard interface name (Default: wg0)\n");
    printf("  -d, --daemon           Run as background daemon (monitor mode)\n");
    printf("  -f, --file <file>      Wireguard resolv.conf - saved (tagged) as /etc/resolv.conf.wg\n");
    printf("                         With monitor, changes to this file are applied while running\n");
    printf("  -l, --logdir <path>    Path of log directory\n");
    printf("  -n, --numlogs <num>    Maximum number of log files (Default: 4, Max: 20)\n");
    printf("  -T, --testing          testing mode (development)\n");
    printf("  -h, --help             Display this help\n");
    printf("\n");
    printf("  Must be run as root (wireguard PostUp / PostDown) except in testing mode.\n");
    printf("  The *wireguard* and *monitor* tasks both require the interface be up.\n");
}

enum {
    BASE_10 = 10,
    MAX_NUM_LOGS = 20,
    OPT_DONE = 0,       /* option handled */
    OPT_HELP = 1,       /* help printed - stop */
    OPT_ERROR = -1,
};

/*
 * Task is any non-empty prefix of: standard, wireguard, monitor
 */
static Task parse_task(const char *opt_in) {
    static const struct {
        const char *name;
        Task task;
    } tasks[] = {
        { "standard",  INSTALL_STANDARD },
        { "wireguard", INSTALL_WG },
        { "monitor",   MONITOR },
    };

    size_t len = strlen(opt_in);
    if (len == 0) {
        return NOTHING;
    }

    for (size_t i = 0; i < sizeof(tasks) / sizeof(tasks[0]); i++) {
        if (len <= strlen(tasks[i].name) && strncmp(opt_in, tasks[i].name, len) == 0) {
            return tasks[i].task;
        }
    }
    return NOTHING;
}

/*
 * Non-empty string option
 */
static int set_string_opt(const char **dst, const char *value, int opt) {
    if (!value || *value == '\0') {
        (void)fprintf(stderr, "Error: -%c requires a non-empty value.\n", opt);
        return OPT_ERROR;
    }
    *dst = value;
    return OPT_DONE;
}

static int set_task(Options *opts, const char *value) {
    if (opts->task != NOTHING) {
        (void)fprintf(stderr, "Error: only one task allowed.\n");
        return OPT_ERROR;
    }

    opts->task = parse_task(value);
    if (opts->task == NOTHING) {
        (void)fprintf(stderr, "Error: bad task '%s'. (Valid: s[tandard], w[ireguard], m[onitor])\n", value);
        return OPT_ERROR;
    }
    return OPT_DONE;
}

static int set_num_logs(Options *opts, const char *value) {
    char *end = nullptr;

    errno = 0;
    long num = strtol(value, &end, BASE_10);
    if (errno != 0 || end == value || *end != '\0') {
        (void)fprintf(stderr, "Error: -n requires an integer: '%s'\n", value);
        return OPT_ERROR;
    }

    if (num < 0) {
        num = 0;
    } else if (num > MAX_NUM_LOGS) {
        num = MAX_NUM_LOGS;
    }
    opts->num_logfiles = (int)num;
    return OPT_DONE;
}

static int bad_option(char **argv) {
    if (optopt) {
        (void)fprintf(stderr, "Error: bad option -%c\n", optopt);
    } else {
        (void)fprintf(stderr, "Error: bad option %s\n", argv[optind - 1]);
    }
    return OPT_ERROR;
}

/*
 * Handle one option returned by getopt_long()
 */
static int handle_option(int opt, Options *opts, char **argv) {
    switch (opt) {
        case 't':
            return set_task(opts, optarg);

        case 'd':
            opts->daemon = true;
            return OPT_DONE;

        case 'f':
            return set_string_opt(&opts->file, optarg, opt);

        case 'i':
            return set_string_opt(&opts->iface_name, optarg, opt);

        case 'l':
            return set_string_opt(&opts->logdir, optarg, opt);

        case 'n':
            return set_num_logs(opts, optarg);

        case 'T':
            opts->testing = true;
            return OPT_DONE;

        case 'h':
            print_help(argv[0]);
            return OPT_HELP;

        case ':':
            (void)fprintf(stderr, "Error: -%c requires a value.\n", optopt);
            return OPT_ERROR;

        case '?':
            return bad_option(argv);

        default:
            return OPT_ERROR;
    }
}

/*
 * Returns:
 *   0 ok
 *   1 help printed - nothing else to do
 *  -1 error
 */
int parse_options(int argc, char **argv, Options *opts) {

    if (argv == nullptr || opts == nullptr) {
        return -1;
    }

    *opts = (Options){};
    opts->task = NOTHING;
    opts->daemon = false;
    opts->num_logfiles = 4;
    opts->log_file = nullptr;

    const struct option long_options[] = {
        {"task",      required_argument, nullptr, 't'},
        {"iface",     required_argument, nullptr, 'i'},
        {"daemon",    no_argument,       nullptr, 'd'},
        {"help",      no_argument,       nullptr, 'h'},
        {"logdir",    required_argument, nullptr, 'l'},
        {"file",      required_argument, nullptr, 'f'},
        {"testing",   no_argument,       nullptr, 'T'},
        {"numlogs",   required_argument, nullptr, 'n'},
        {"numfiles",  required_argument, nullptr, 'n'},     /* backward compat */
        {nullptr,     0,                 nullptr, 0  },
    };

    /*
     * Suppress default getopt error prints
     */
    opterr = 0;
    int opt = 0;

    // single threaded - getopt_long() global state is fine
    while ((opt = getopt_long(argc, argv, ":t:Tdl:n:hi:f:", long_options, nullptr)) != -1) {  // NOLINT(concurrency-mt-unsafe)
        int ret = handle_option(opt, opts, argv);
        if (ret != OPT_DONE) {
            return ret;
        }
    }

    for (int i = optind; i < argc; i++) {
        (void)fprintf(stderr, "Warning: ignoring unexpected argument: %s\n", argv[i]);
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
