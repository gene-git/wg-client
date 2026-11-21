# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Ensure that nothing overwrites the wireguard resolv.conf
(Use inotify to monitor for file resolv.conf change, then repair)
"""
# pylint: disable=too-many-instance-attributes
from wg_client.utils import cLog
from wg_client.proc import get_matching_processes


def _is_monitor_args(argv: list[str]) -> bool:
    """
    True if argv selects the monitor task:
      -t m[onitor]  --task m[onitor]  --task=m[onitor]  -tm[onitor]
    """
    for (idx, arg) in enumerate(argv):
        value = None
        if arg in ('-t', '--task'):
            if idx + 1 < len(argv):
                value = argv[idx + 1]
        elif arg.startswith('--task='):
            value = arg[len('--task='):]
        elif arg.startswith('-t') and not arg.startswith('--'):
            value = arg[2:]

        if value and 'monitor'.startswith(value):
            return True
    return False


class ResolvManager():
    """
    resolv-manager is a standalone daemon process.
    It is started (as root) by the wireguard PostUp script (post-up.sh) and
    exits by itself when the wireguard interface goes away (wg-quick down),
    restoring the standard resolv.conf.
    This class provides a way to check if it is running.
    """
    def __init__(self):
        self.okay: bool = True
        self.resolv_manager: str = '/usr/lib/wg-client/resolv-manager'

    def running_info(self) -> dict[str, str]:
        """
        Get details about any currently running resolv-manager
        It uses locking to guarantee only 1 instance at a time.
        :returns: dictionary keyed by  "pid", "owner", "args"
        """
        matches = get_matching_processes(self.resolv_manager, [])
        num = len(matches)

        if num > 1:
            cLog.msg(f" WARNING -  {num} resolv_manager processes!")

        if num > 0:
            return matches[0]
        return {}

    def is_running(self) -> bool:
        """
        Checks if resolv-manager is running in monitor mode
        """
        info = self.running_info()
        if not info:
            return False

        if not _is_monitor_args(info['args'].split()):
            pid = info['pid']
            cLog.msg(f' resolv-manager running but not in monitor mode pid: {pid}')
            return False
        return True
