# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Parse output of wg-quick and get client wg address
"""
import json
from pyconcurrent import run_prog


def iface_to_ips(iface: str) -> tuple[list[str], list[str]]:
    """
    Use ni to get ips
    return list of ip4,ip6 addresses
    """
    ip4: list[str] = []
    ip6: list[str] = []

    pargs = ['ip', '-j', '-d', 'address', 'show', iface]
    (ret, out, _err) = run_prog(pargs)
    if ret != 0:
        # print(f'Error getting ips from interface: {err}')
        return (ip4, ip6)

    if not out:
        return (ip4, ip6)

    try:
        data = json.loads(out)

    except json.JSONDecodeError as _err:
        # print(f'Error decoding output of ip addr show: {err}')
        return (ip4, ip6)

    if not (data and data[0] and data[0].get('addr_info')):
        return (ip4, ip6)

    addr_infos = data[0].get('addr_info')
    if not addr_infos:
        return (ip4, ip6)

    for addr_info in addr_infos:
        fam = addr_info.get('family')
        ip = addr_info.get('local')

        if not (fam and ip):
            continue

        match fam:
            case 'inet':
                ip4.append(ip)

            case 'inet6':
                ip6.append(ip)

    return (ip4, ip6)
