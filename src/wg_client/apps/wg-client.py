#!/usr/bin/python
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Command line Start and Stop Wireguard
"""
# pylint: disable=invalid-name
from wg_client.cmd_line import WgClient


def main():
    """
    Application to start vpn, stop vpn and start ssh remote listener
    Runs : wg-quick up/down <interface>
    Defaul <interface> is wgc but can be provided on command line
    The special <interface> 'test-dummy' runs in test mode
    Since wg-quick must be run as root :
    create /etc/sudoers.d/wg-runner
        <user>   ALL=NOPASSWD: SETENV: /usr/bin/wg-quick
    where <user> is the user who will be using the tool
    """
    client = WgClient()

    if client.okay:
        client.do_all()


# -----------------------------------------------------
if __name__ == '__main__':
    main()
