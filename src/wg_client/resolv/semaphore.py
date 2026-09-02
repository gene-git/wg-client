# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
App stuff
"""
from pathlib import Path

from wg_client.proc import get_local_state_dir


def get_run_semaphore() -> str:
    """
    The semaphore file used to signal resolv-manager
    when to exit. Remove file and it will exit.
    Since wireguard config uses resolv-monitor to restore
    (in case its started manually and not by wg-client) we need a clean
    way to rell resolv-manager to exit
    :returns: semaphore filename (~/.local/state/wg-client/run_semaphore)
    """
    state_dir: str = get_local_state_dir()
    semaphore = f'{state_dir}/run_semaphore'
    return semaphore


def create_semaphore() -> str:
    """
    touch the file and return the file.
    If unable to create the file, return empty string
    """
    semaphore = get_run_semaphore()
    try:
        Path(semaphore).touch(exist_ok=True)

    except (PermissionError, FileNotFoundError, OSError):
        return ''

    return semaphore


def remove_semaphore() -> bool:
    """
    touch the file
    """
    semaphore = get_run_semaphore()
    try:
        Path(semaphore).unlink(missing_ok=True)
    except (IsADirectoryError, PermissionError, OSError):
        return False
    return True
