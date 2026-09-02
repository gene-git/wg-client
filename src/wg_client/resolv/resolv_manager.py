# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Ensure that nothing overwrites the wireguard resolv.conf
(Use inotify to monitor for file resolv.conf change, then repair)
"""
# pylint: disable=too-many-instance-attributes
from pyconcurrent import run_prog

from wg_client.utils import cLog
from wg_client.proc import get_matching_processes

from .semaphore import create_semaphore
from .semaphore import remove_semaphore


class ResolvManager():
    """
    resolv-manager is standalone daemon process.
    This class provides a way to:
    - check if resolv-manager is running
    - start resolv-manager
    """
    def __init__(self):
        self.okay: bool = True
        self.resolv_manager: str = '/usr/lib/wg-client/resolv-manager'

        self.pid: int = -1

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
        Return : pargs list
        """
        info = self.running_info()
        if info:
            pid = info['pid']
            if '-t m' in info['args']:
                cLog.msg(f' resolv-manager running not monitor mode pid: {pid}')
            return True
        return False

    def start(self, iface: str) -> bool:
        """
        Provide run/exit semaphore to resolv-manager
        - Fatal error if cannot touch the semaphore file.
        Start resolv-manager
        """
        cLog.msg(' Starting : resolv_manager')
        logdir = cLog.logdir()

        semaphore_file = create_semaphore()
        if not semaphore_file:
            cLog.msg(" Error: creating semaphore file for resolv-manager")
            return False

        pargs = [self.resolv_manager]
        pargs += ["--daemon", "--iface", iface, "-t", "monitor", "-l", logdir]
        pargs += ["--semaphore", semaphore_file]

        (ret, outs, errs) = run_prog(pargs)
        if ret == 0:
            cLog.msg(f" Success : dameon, monitor. logdir {logdir}")
        else:
            cLog.msg(" Error")
            cLog.msg(outs)
            cLog.msg(errs)
            return False
        return True

    def stop(self) -> bool:
        """
        Simply remove the semphore
        - if this fails - we need to have user manually kill it!
        - maybe we add code to get the process and kill it here.
        - we should add a separate routine that kills resolv-manager
          if not shut down after a second.
        """
        cLog.msg(' Stopping resolv-manager')
        if not remove_semaphore():
            cLog.msg(" Error: removeing semaphore file for resolv-manager")
            cLog.msg("   may need to manually kill it - should never happen!!")
            return False
        return True
