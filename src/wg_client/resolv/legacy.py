# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Transition support for resolv-manager monitors started by older wg-client.

Older versions started the resolv-manager monitor as the user, with a
"run semaphore" file: the monitor exited (restoring the standard
resolv.conf) only when that file was removed. It could not detect the
wireguard interface going away.

The monitor is now started by the wireguard PostUp script and exits by itself
when the interface goes away - no semaphore. But an old monitor still running
(e.g. tunnel up while the package was upgraded) would never exit, keeps the
lock and blocks the new PostUp / PostDown.

So before wg-quick up/down, defensively remove any old semaphore and give an
old monitor a moment to restore resolv.conf and exit.
"""
import os
import time
from pathlib import Path
from typing import Callable

from wg_client.proc import get_matching_processes

_RESOLV_MANAGER = '/usr/lib/wg-client/resolv-manager'
_SEMAPHORE_NAME = 'run_semaphore'


def _legacy_semaphores() -> list[str]:
    """
    Possible semaphore files of the current user.
    Paths are computed only - no directories are created.
    """
    paths: list[str] = [
        os.path.join(Path.home(), '.local/share/state/wg-client', _SEMAPHORE_NAME),
    ]
    runtime_dir = os.environ.get('XDG_RUNTIME_DIR', '')
    if runtime_dir and os.path.isabs(runtime_dir):
        paths.append(os.path.join(runtime_dir, 'wg-client', _SEMAPHORE_NAME))
    return paths


def _legacy_monitor_running() -> bool:
    """
    Any resolv-manager started with --semaphore (only old versions did)
    """
    return bool(get_matching_processes(_RESOLV_MANAGER, ['--semaphore']))


def stop_legacy_monitor(log: Callable[[str], None] = print, timeout: float = 3.0) -> bool:
    """
    Remove any old semaphore file and wait (up to timeout seconds) for an
    old monitor to exit.
    :returns: True if no old monitor is left running.
    """
    removed = False
    for path in _legacy_semaphores():
        try:
            os.unlink(path)
            removed = True
            log(f' Removed old resolv-manager semaphore {path}')
        except FileNotFoundError:
            pass
        except OSError as err:
            log(f' Warning: unable to remove old semaphore {path}: {err}')

    if not _legacy_monitor_running():
        return True

    if not removed:
        log(' Warning: old resolv-manager monitor running - semaphore not found')

    step = 0.1
    waited = 0.0
    while waited < timeout:
        time.sleep(step)
        waited += step
        if not _legacy_monitor_running():
            log(' Old resolv-manager monitor has exited')
            return True

    log(' Warning: old resolv-manager monitor still running - may need to be killed')
    return False
