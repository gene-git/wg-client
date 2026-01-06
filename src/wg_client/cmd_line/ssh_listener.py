# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: © 2023-present  Gene C <arch@sapience.com>
"""
Parse output of wg-quick and get client wg address
"""
import random
from .ip_addr import ip_to_octet

def get_ssh_port_prefix(pfx_range:[str]) -> int:
    """
    Pick 2 digit num from range
     - default 45 
    """
    if not pfx_range:
        return 45

    # make sure we got ints
    pfx_range = [int(num) for num in pfx_range]
    if len(pfx_range) == 1:
        return pfx_range[0]

    num = random.randint(pfx_range[0], pfx_range[1])
    return num

def ssh_args(wg_ip, server, prefix) -> (str, str, str, str):
    '''
    returns:
        (server, remote_port, local_ip, local_port)
    '''
    if not (server and prefix and wg_ip):
        print('ssh_args Error Need wg_server, wg_ip and port prefix')
        return (None, None, None, None)

    octet = ip_to_octet(wg_ip)
    remote_port = f'{prefix}{octet}'
    local_port = '22'
    local_ip = '127.0.0.1'

    return (server, remote_port, local_ip, local_port)


def ssh_listener_args(test, wg_ip, host, prefix):
    """
    Run ssh to create remote listening port
     - remote port: YYxxx with xxx the 3 digit from last ip octet
     - YY is randomly chosen from prefix range. This is pair of 2 digit numbers.
         If prefix has only 1 number then it is used.
       - remote end can 'discover' on server, by looking for port based on the
         3 digit client octet
         e.g. ss -lt | grep 'xxx'
     We run ssh in foregroud to ensure it exits when caller exits.
     i.e. this call will 'hang' until it is killed by Ctrl-C of main or if called
     from another program when that program exits.
    """
    pargs = []
    (server, remote_port, local_ip, local_port) = ssh_args(wg_ip, host, prefix)
    if not server:
        return pargs

    print(f'ssh listening port : {remote_port}')

    rfwd = f'{remote_port}:{local_ip}:{local_port}'

    pargs = []
    if test:
        pargs += ['/usr/bin/echo']
    pargs += ['/usr/bin/ssh', '-R', rfwd, '-N',  server]

    return pargs
