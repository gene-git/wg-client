# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Gene C
"""
Parse output of wg-quick and get client wg address
"""
import netaddr
import netifaces as ni

def wg_quick_out_to_ip4(wg_quick_output):
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
    ip4 = None
    if not wg_quick_output:
        return ip4

    # get the ip address
    rows = wg_quick_output.splitlines()
    for row in rows:
        if row.startswith('[#] ip -4 address'):
            row_split = row.split()
            ip4 = row_split[5]
            ip4 = ip4.replace('/32','')
            break
    return ip4

def ip4_to_octet(ip4):
    """
    Strip of trailing octet for end of ssh listen port
    """
    octet = None
    if not ip4:
        return octet

    ip4_split = ip4.split('.')
    if len(ip4_split) < 4:
        return octet

    octet = ip4_split[3]
    octet = int(octet)
    octet = f'{octet:03d}'
    return octet

def ip_to_octet(ip_str):
    """ extract trailing octet of addrip """
    ipadd = netaddr.IPNetwork(ip_str)
    bits = ipadd.ip.bits()
    ipadd_str = str(ipadd.ip)

    if netaddr.valid_ipv4(ipadd_str):
        bits = bits.split('.')[-1]
    elif netaddr.valid_ipv6(ipadd_str):
        bits = bits.split(':')[-1]
        bits = bits[-9:-1]

    octet = int(bits, 2)
    octet =f'{octet:03d}'
    return octet

def iface_to_ips(iface):
    """
    Use ni to get ips
    return list of ip4,ip6 addresses
    """
    # pylint: disable=c-extension-no-member,invalid-name
    ips4 = []
    ips6 = []

    ifaces = ni.interfaces()
    if iface not in ifaces:
        return (ips4, ips6)

    adds = ni.ifaddresses(iface)
    if not adds:
        return (ips4, ips6)

    add4 = adds.get(ni.AF_INET)
    if add4:
        for add in add4:
            ip = add.get('addr')
            if ip:
                ips4.append(ip)

    add6 = adds.get(ni.AF_INET6)
    if add6:
        for add in add6:
            ip = add.get('addr')
            if ip:
                ips6.append(ip)

    return (ips4, ips6)
