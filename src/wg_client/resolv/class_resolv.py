# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Ensure that nothing overwrites the wireguard resolv.conf
(Use inotify to monitor for file resolv.conf change, then repair)
"""
# pylint: disable=too-many-instance-attributes
import os
import sys
import time
from pynotify import Inotify
from pynotify import InotifyMask as mask

from wg_client.proc import (MyProc, MySignals)
from wg_client.proc import (kill_program, read_pid, write_pid, check_pid)
from wg_client.utils import MyLog


# for tests only
# from .resolv import restore_resolv


def get_events_mask() -> int:
    """
    Monitor all events on /etc/resolv.conf except for
     - access and open
    We are only interested in changes to the file
    Dont really care about attribute changes either but we need to be sure
    root can still adjust as needed so we check after attrib change just in case
    """
    events_remove = mask.IN_ACCESS | mask.IN_OPEN | mask.IN_CLOSE_NOWRITE
    resolv_events = mask.IN_ALL_EVENTS - events_remove
    return resolv_events


class WgResolv():
    """
    Monitor and/or fix WG resolv.conf
     - when network is restarted (happens under resume from sleep)
       it can overwrite resolv.conf
     - tool to monitor and restore the wireguard config kept in /etc/resolv.conf.wg
    """
    def __init__(self):
        self.okay: bool = True
        self.resolv: str = '/etc/resolv.conf'
        self.resolv_wg: str = '/etc/resolv.conf.wg'
        self.pid: int = os.getpid()
        self.pidfile_tag: str = 'wg-resolv-monitor'

        self.mysignals = None
        self.run_proc = None

        # use separate log file
        self.logger = MyLog('wg-mon-resolv')

    def log(self, msg):
        """ wrap logger """
        self.logger.log(msg)

    def fix_resolv_cmd(self) -> list[str]:
        """
        Which tool to call to manage /etc/resolv.conf et al
        Return : pargs list
        """
        pargs = ['/usr/lib/wg-client/wg-fix-resolv']
        return pargs

    # def restore_resolv(self):
    #     """
    #     Test tool only for user writable test files
    #     - restore wg resolv.conf
    #     - production use replaced by C-code using CAPABILITIES:
    #       /usr/lib/wg-client/wg-fix-resolv
    #     """
    #     if not restore_resolv(self.resolv, self.resolv_wg):
    #         #self.log('Fix dns resolv failed')
    #         print(f'Fix resolv failed : {self.resolv_wg} -> {self.resolv}')
    #         self.okay = False
    #     return self.okay

    def runit(self, pargs):
        """
        run a program via subprocess.run
        """
        self.log(f' Running : {pargs}')
        self.mysignals = MySignals()
        self.run_proc = MyProc(self.mysignals)
        (_ret, _outs, _errs) = self.run_proc.popen(pargs, logger=self.log, pid_saver=None)

    def pidfile(self) -> str:
        """ return the basenme of pid file """
        return self.pidfile_tag

    def save_pidfile(self):
        """ save pid to pidfile """
        write_pid(self.pid, self.pidfile_tag)

    def read_pidfile(self, user: str = '') -> int:
        """ save pid to pidfile """
        pid = read_pid(self.pidfile_tag, user=user)
        return pid

    def check_already_running(self, user: str = '') -> bool:
        """
        Check if monitor already running by this user.
        If no user given then user is process owner.
        Only root can query other users
        """
        pid = read_pid(self.pidfile_tag, user=user)
        pargs = ['--fix-dns-auto-start']
        pid_valid = check_pid(pid, pargs, user=user)

        return pid_valid

    def kill_monitor(self):
        """
        kill any running resol monitor process
        """
        pid = self.read_pidfile()
        pargs = sys.argv
        pargs = ['--fix-dns-auto-start']
        kill_program(pid, pargs)

    def _wait_for_events(self, inot: Inotify):
        """ returns when itnotify tells us file event occurred """
        try:
            for events in inot.get_events():
                num = len(events)
                self.log(f' num file events: {num}')
                return

        except (IOError, KeyboardInterrupt) as exc:
            self.log(f'Exception : {exc}')
            # inot.rm_watch(self.resolv)
            sys.exit()

    def monitor_resolv(self):
        """
        Monitor /etc/resolv.conf for any changes and call
        /usr/lib/wg-client/wg-fix-resolv to restore resolv.conf from .conf.wg
        wg-fix-resolv checks for file change so we dont need to.
        wg vpn version.
        NB this stays running until killed
         - must always start new inotify watch in case inode changes
         - file may be replaced and inode changes
         - file may be editied and can change or not
         - file may be symlink (inotify follows synlinks by default)
         - what is optimal inotify mask?
           - simplest is to trigger on any event and fix-resolv will
             do the right thing. Simple is good
        """
        #
        # Make sure only 1 copy running
        # save pid so wg-client can safely restore resolv.conf
        # without undoing it
        #
        self.log('monitor_resolv: start')
        if self.check_already_running():
            self.log(' resolv monitor already running')
            return
        self.save_pidfile()

        pargs = self.fix_resolv_cmd()
        events_mask = get_events_mask()

        while True:
            #
            # we *must* always create fresh watch
            #
            inot = Inotify()
            inot.add_watch(self.resolv, mask=events_mask)
            inot.timeout = -1               # wait forever

            self._wait_for_events(inot)
            inot.rm_watch(self.resolv)

            self.log(f' File change detected {self.resolv}')

            pargs = ['/usr/lib/wg-client/wg-fix-resolv']
            time.sleep(0.2)
            self.runit(pargs)
            # time.sleep(0.2)
            # inot.add_watch(self.resolv, mask=events_mask)
