# SPDX-License-Identifier: MIT
# Copyright (c) 2022,2023 Gene C
"""
Support tools for fix dns
"""
import os
from .utils import read_file, copy_file

def _no_comments(data:str) -> str:
    """ strip comment lines """
    data_clean = ''
    for row in data.splitlines():
        if row.startswith('#'):
            continue
        data_clean += row + '\n'
    return data_clean

def compare_resolv(resolv, resolv_wg):
    """
    Compares 2 resolv.conf files
     returns
     - None if either is missing - likely means wg not running
     - True if files match
     - False otherwise
    """
    data = read_file(resolv)
    data_wg = read_file(resolv_wg)

    if not (data and data_wg):
        return None

    data_clean = _no_comments(data)
    data_wg_clean = _no_comments(data_wg)

    if data_clean == data_wg_clean:
        return True
    return False

def restore_resolv(resolv, resolv_wg):
    """
    If resolv.conf.wg exists then wg running.
    if resolve.conf doesn't match it then copy resolv.conf.wg to resolv.conf
    """
    if not os.path.exists(resolv_wg):
        print(f'Error: Missing backup wg resolv file : {resolv_wg}')
        return False

    comp = compare_resolv(resolv, resolv_wg)
    if comp :
        print('wg dns resolv is good; nothing to do')
        return True

    isok = copy_file(resolv_wg, resolv)
    if not isok:
        print(f'Error: copying {resolv_wg} to {resolv}')
        return False
    print('wg resolv fixed')
    return True
