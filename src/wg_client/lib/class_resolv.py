# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Gene C
"""
Use inotify to monitor resolv.conf
"""
# pylint: disable=too-few-public-methods
import os
from .resolv import restore_resolv

class WgResolv():
    """
    Monitor and/or fix WG resolv.conf
     - when network is restarted (happens under resume from sleep) 
       it can overwrite resolv.conf
     - tool to monitor and restore the wireguard config kept in /etc/resolv.conf.wg
    """
    def __init__(self, log=print):
        self.okay = True
        self.resolv = '/etc/resolv.conf'
        self.resolv_wg = '/etc/resolv.conf.wg'
        self.euid = os.geteuid()
        self.log = log

    def restore_resolv(self):
        """
        restore wg resolv.conf
        """
        if self.euid == 0:
            if not restore_resolv(self.resolv, self.resolv_wg):
                self.log('Fix dns resolv failed')
                self.okay = False
        else:
            msg = 'Error: needs root privs to fix dns resolv'
            print(msg)
            self.log(msg)
            self.okay = False
        return self.okay
