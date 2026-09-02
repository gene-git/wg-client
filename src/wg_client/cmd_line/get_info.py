# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
GUI : gets Info by running wg-client -
 - wg iface
 - ssh_server
 - is wg running
 - is ssh running
"""
import os


def is_valid_interface(iface: str) -> bool:
    """
    Check interface exists
    """
    is_valid = False
    if not iface:
        return is_valid

    iface_path = f'/sys/class/net/{iface}'
    if os.path.exists(iface_path):
        is_valid = True

    return is_valid


def is_wg_running(iface: str) -> bool:
    """
    If its running then interface exists
    """
    return is_valid_interface(iface)
