#!/usr/bin/python
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
GUI Tool to Start and Stop Wireguard
"""
# pylint: disable=invalid-name
import os
import sys
from wg_client.gui import MainGui

os.environ["QT_NO_XDG_DESKTOP_PORTAL"] = "1"
os.environ["QT_LOGGING_RULES"] = "qt.qpa.wayland.textinput.warning=false;qt.qpa.services.warning=false"


def main():
    """
    Application to start stop vpn
    Runs : wg-quick up/down <interface>
    Default <interface> is wgc but can be provided in config file
    The special <interface> 'test-dummy' makes program run in test mode

    Since wg-quick must be run as root user must have sudoers :
    wg-client provides /etc/sudoers.d/99-wireguard which gives wireguard
    access to members of wg-client group to tunnels with interface names
    starting with the letter "w".
    Add each user to group wg-client (gpasswd -a <user> wg-client).
    The wireguard config PostUp (post-up.sh) starts the resolv-manager monitor.
    Exact commands only - wg-quick with arbitrary arguments is equivalent to root.

    """
    myname = sys.argv[0]
    MainGui(myname)


if __name__ == '__main__':
    main()
