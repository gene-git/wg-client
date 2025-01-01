#!/usr/bin/python3
# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: © 2023-present  Gene C <arch@sapience.com>
"""
When using linux as a client, there are 2 ways to bring up wg.
If using wg-tool postup script : wg-peer-updn
It sets up a good /etc/resolv.conf for wireguard.
It also creates a copy of in /etc/resolv.conf.wg
WHen wireguard is turned off, original resolv.conf is restored and the
resolv.conf.wg is removed.

Often when a computer is sleep resumed the networking puts the usual resolv.conf back
in - if wireuard was left running then we need to put back the correct wireguard
resolve.conf.

Thats what this tool does
Obviously must be run as root.
"""
# pylint: disable=invalid-name
from lib import WgResolv

def main():
    """
    Support for linux wg clients
    """
    fix_dns = WgResolv()
    if fix_dns.okay:
        fix_dns.restore_resolv()

if __name__ == '__main__':
    main()
