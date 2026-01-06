#!/usr/bin/python
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Command line Start and Stop Wireguard
"""
# pylint: disable=invalid-name
import sys
from wg_client.gui import MainGui


def main():
    """
    Application to start stop vpn
    Runs : wg-quick up/down <interface>
    Default <interface> is wgc but can be provided in config file
    The special <interface> 'test-dummy' makes program run in test mode

    Since wg-quick must be run as root user must have sudoers :
    create /etc/sudoers.d/wg-runner
        <user>   ALL=NOPASSWD: SETENV: /usr/bin/wg-quick
    where <user> is the user who will be using the tool

    """
    myname = sys.argv[0]
    MainGui(myname)


# -----------------------------------------------------
if __name__ == '__main__':
    main()
