# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Get list of running processes
"""
import psutil


def get_matching_processes(exe_path: str, required_args: list[str]) -> list[dict[str, str]]:
    """
    Given executable path find list of any running.
    Matches on cmdline[0] (argv[0]) rather than /proc/<pid>/exe:
    exe is unreadable for other users' processes and for any process
    started from a binary with file capabilities (non-dumpable) - such as
    resolv-manager - so matching on exe would never find them.
    Note argv[0] is set by whoever starts the process - fine for a status display,
    not to be used for any security decision.

    :param exe_path: The program path to find.
    :param required_args: each must be one of the arguments (exact match)
    :returns: A list of  dictionaries each ~ {pid, owner, args}
    """
    matches: list[dict[str, str]] = []

    #
    # Iterate over all running processes
    #
    for proc in psutil.process_iter(attrs=['pid', 'username', 'cmdline']):
        try:
            cmdline = proc.info['cmdline']
            if not cmdline or cmdline[0] != exe_path:
                continue

            argv = cmdline[1:]
            if not all(arg in argv for arg in required_args):
                continue

            matches.append({
                'pid': proc.info['pid'],
                'owner': proc.info['username'],
                'args': ' '.join(argv),
                })

        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            # Skip processes that terminate during iteration,  lack permissions etc
            continue

    return matches
