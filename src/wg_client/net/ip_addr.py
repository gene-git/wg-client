# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Parse output of wg-quick and get client wg address
"""
import ipaddress
import json
from pyconcurrent import run_prog


def wg_quick_out_to_ip4(wg_quick_output: str) -> str:
    """
    Parse output of wg-quick and get client wg address
    wg-quick up shows :
        [#] ip link add wgc type wireguard
        [#] wg setconf wgc /dev/fd/63
        [#] ip -4 address add <ip4-address>/32 dev wgc
        [#] ip link set mtu 1420 up dev wgc
        [#] wg set wgc fwmark 51820
        [#] ip -4 rule add not fwmark 51820 table 51820
        [#] ip -4 rule add table main suppress_prefixlength 0
        [#] ip -4 route add 0.0.0.0/0 dev wgc table 51820
        [#] sysctl -q net.ipv4.conf.all.src_valid_mark=1
        [#] nft -f /dev/fd/63
        [#] /etc/wireguard/scripts/wg-peer-updn
                    -u -dnsrch <internal.com> -dns <comma list of dns ips>
    """
    ip4 = ''
    if not wg_quick_output:
        return ip4

    # get the ip address
    rows = wg_quick_output.splitlines()
    for row in rows:
        if row.startswith('[#] ip -4 address'):
            row_split = row.split()
            ip4 = row_split[5]
            ip4 = ip4.replace('/32', '')
            break
    return ip4


def get_host_bits(ip: str, prefix: int = 24) -> int:
    '''
    Gets the host bits from an IP address given the netmask
    '''
    ipa = ipaddress.ip_address(ip)
    net = ipaddress.ip_network(ip)
    netpfx = net.supernet(new_prefix=prefix)

    hostmask = netpfx.hostmask
    host_bits = int(ipa) & int(hostmask)

    return host_bits


def ip_to_octet(ip_str: str) -> str:
    '''
    extract trailing octet of addrip
    '''
    host_bits = get_host_bits(ip_str, prefix=24)
    octet = f'{host_bits:03d}'
    return octet


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

    except json.JSONDecodeError as err:
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
