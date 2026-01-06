# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Info
 - wg iface
 - ssh_server
 - is wg running
 - is ssh running
"""
import os
from typing import Callable
from pyconcurrent import run_prog


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


def get_wg_iface(log: Callable[[str], None]) -> str:
    """
    Run wg-client to find the wireguard interface name that will be used
    """
    iface = ''

    pargs = ['/usr/bin/wg-client', '--show-iface']
    (ret, out, err) = run_prog(pargs)
    if ret == 0:
        iface = out.strip()
    else:
        log('Error getting wg iface')
        if out:
            log(out)
        if err:
            log(err)
    return iface


def get_ssh_server(log: Callable[[str], None]) -> str:
    """
    Run wg-client to find the ssh_server that would be used
    """
    ssh_server = ''

    pargs = ['/usr/bin/wg-client', '--show-ssh-server']
    (ret, out, err) = run_prog(pargs)
    if ret == 0:
        ssh_server = out.strip()
    else:
        log('Error getting ssh server')
        if out:
            log(out)
        if err:
            log(err)
    return ssh_server


def is_ssh_running(log: Callable[[str], None]) -> bool:
    """
    Ask wg-client to confirm if ssh is running to host
    by looking up it's saved ssh PID
    """
    ssh_running = False

    pargs = ['/usr/bin/wg-client', '--show-ssh-running']
    (ret, out, err) = run_prog(pargs)
    if ret == 0:
        answer = out.strip()
        if answer == 'True':
            ssh_running = True
    else:
        log('Error getting ssh is running')
        if out:
            log(out)
        if err:
            log(err)
    return ssh_running
