#!/usr/bin/python
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Command line tool Start and Stop Wireguard
"""
# pylint: disable=invalid-name
from wg_client.cmd_line import WgClient


def main():
    """
    Application to start vpn, stop vpn and start ssh remote listener
    Runs : wg-quick up/down <interface>
    Default <interface> is wgc but can be provided on command line
    The special <interface> 'test-dummy' runs in test mode
    Since wg-quick must be run as root :
    wg-client provides /etc/sudoers.d/99-wireguard which gives wireguard
    access to members of wg-client group to tunnels with interface names
    starting with the letter "w".
    Add each user to group wg-client (gpasswd -a <user> wg-client).
    The wireguard config PostUp (post-up.sh) starts the resolv-manager monitor.
    Exact commands only - wg-quick with arbitrary arguments is equivalent to root.
    """
    client = WgClient()

    if client.okay:
        client.do_all()


if __name__ == '__main__':
    main()
