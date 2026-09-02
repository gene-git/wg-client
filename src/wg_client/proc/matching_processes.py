# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Get list of running processes
"""
import psutil


def get_matching_processes(exe_path: str, required_args: list[str]) -> list[dict[str, str]]:
    """
    Given executable path find list of any running.
    Returns list of tuples(pid, owner, args)
    :param exe_path: The program path to find.
    :returns: A list of  dictionaries each ~ {pid, owner, args}
    """
    matches: list[dict[str, str]] = []

    #
    # Iterate over all running processes
    #
    for proc in psutil.process_iter(attrs=['pid', 'username', 'exe', 'cmdline']):
        try:
            path = proc.info['exe']
            cmdline = proc.info['cmdline']
            if not cmdline or path != exe_path:
                continue

            # cmd = cmdline[0]
            args = ' '.join(cmdline[1:])

            matching_args: bool = True
            for arg in required_args:
                if arg not in args:
                    matching_args = False
                    break

            if matching_args:
                matches.append({
                    'pid': proc.info['pid'],
                    'owner': proc.info['username'],
                    'args': args,
                    })

        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            # Skip processes that terminate during iteration,  lack permissions etc
            continue

    return matches
