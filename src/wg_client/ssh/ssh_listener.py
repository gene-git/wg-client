# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Parse output of wg-quick and get client wg address
"""
import random
from py_cidr import PyCidr


def get_ssh_port_prefix(pfx_range: list[str]) -> int:
    """
    Pick 2 digit num from range
     - default 45
    """
    if not pfx_range:
        return 45

    # make sure we got ints
    pfx_ints = [int(num) for num in pfx_range]
    if len(pfx_ints) == 1:
        return pfx_ints[0]

    num = random.randint(pfx_ints[0], pfx_ints[1])
    return num


def ssh_args(wg_ip: str, server: str, ssh_prefix: int) -> tuple[str, str, str, str]:
    '''
    ssh_prfix is the first 2 digits of the remote port. The last 3 digits are
    taken from the last 8 bits of the ip.
    returns:
        (server, remote_port, local_ip, local_port)
    '''
    remote_port = ''
    local_ip = ''
    local_port = ''

    if not (server and ssh_prefix and wg_ip):
        print('ssh_args Error Need wg_server, wg_ip and port prefix')
        return (server, remote_port, local_ip, local_port)

    #
    # get the last 8 bits of the address (octet). Ports are limitied to 8 bits
    # so use last 8 bits of IP for the prefix tail - we should come up with better
    # algo for ipv6 where the port is pre-determined.
    #
    is_ipv6: bool = False
    if PyCidr.is_valid_ipv6(wg_ip):
        is_ipv6 = True
        pfx = 120
    else:
        pfx = 24

    # replace any prefix passed in so we always get 8 bits
    address = wg_ip.split('/')[0]
    cidr = f'{address}/{pfx}'
    ip_tail = PyCidr.get_host_bits(cidr)
    if is_ipv6:
        ip_tail = ip_tail.rsplit(':', maxsplit=1)[-1]
    else:
        ip_tail = ip_tail.rsplit('.', maxsplit=1)[-1]

    # 3 digits
    ip_tail = f'{int(ip_tail):03d}'

    remote_port = f'{ssh_prefix}{ip_tail}'
    local_port = '22'
    local_ip = '127.0.0.1'

    return (server, remote_port, local_ip, local_port)


def ssh_listener_args(test: bool, wg_ip: str, host: str, ssh_prefix: int) -> list[str]:
    """
    Run ssh to create remote listening port
     - remote port: YYxxx with xxx the 3 digit from last ip octet
     - YY is randomly chosen from ssh_prefix range. This is pair of 2 digit numbers.
         If ssh_prefix has only 1 number then it is used.
       - remote end can 'discover' on server, by looking for port based on the
         3 digit client octet
         e.g. ss -lt | grep 'xxx'
     We run ssh in foregroud to ensure it exits when caller exits.
     i.e. this call will 'hang' until it is killed by Ctrl-C of main or if called
     from another program when that program exits.
    """
    pargs: list[str] = []
    (server, remote_port, local_ip, local_port) = ssh_args(wg_ip, host, ssh_prefix)
    if not server:
        return pargs

    print(f'ssh listening port : {remote_port}')

    rfwd = f'{remote_port}:{local_ip}:{local_port}'

    pargs = []
    if test:
        pargs += ['/usr/bin/echo']
    pargs += ['/usr/bin/ssh', '-R', rfwd, '-N',  server]

    return pargs
