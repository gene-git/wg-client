# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Gene C
"""
Parse output of wg-quick and get client wg address
"""
import random
from .ip_addr import ip4_to_octet

def choose_pfx(pfx_range):
    """
    Pick 2 digit num from range
    """
    if not pfx_range:
        return None

    # make sure we got ints
    pfx_range = [int(num) for num in pfx_range]
    if len(pfx_range) == 1:
        return pfx_range[0]

    num = random.randint(pfx_range[0], pfx_range[1])
    return num


def ssh_listener_args(test, wg_ip, host, pfx_range:[int]):
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
    prefix = choose_pfx(pfx_range)
    if not (host and prefix and wg_ip):
        print('Err: Missing wg_server, wg_ip or port prefix for ssh listener')
        return (None, None, None)


    # get last octet
    octet = ip4_to_octet(wg_ip)

    port = f'{prefix}{octet}'
    print(f'ssh listening port : {port}')

    rfwd = f'{port}:127.0.0.1:22'

    pargs = []
    if test:
        pargs += ['/usr/bin/echo']
    pargs += ['/usr/bin/ssh']
    pargs += ['-R', rfwd, '-N',  host]

    return pargs
