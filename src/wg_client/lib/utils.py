# SPDX-License-Identifier: MIT
# Copyright (c) 2022,2023 Gene C
"""
wg-tool support utils`
"""
import os
from datetime import datetime
def date_time_str (fmt='%Y%m%d-%H:%M:%S'):
    """
    date time string
    """
    today = datetime.today()
    today_str = today.strftime(fmt)
    return today_str

def open_file(path, mode):
    """
    Open a file and return file object
    """
    # pylint: disable=W1514,R1732
    try:
        fobj = open(path, mode)
    except OSError as err:
        print(f'Error opening file {path} : {err}')
        fobj = None
    return fobj

def copy_file(f_from, f_to):
    """
    copy file
    """
    okay = True
    from_data = read_file(f_from)
    if from_data:
        if not write_file(from_data, f_to):
            print(f'Failed to write {f_to}')
            return not okay
    else:
        print(f'Failed to read {f_from}')
        return not okay
    return okay

def write_file(data, f_to, mode='w'):
    """
    write file
    """
    okay = True
    fobj = open_file(f_to, mode)
    if fobj:
        fobj.write(data)
        fobj.close()
        return okay
    return not okay

def read_file(f_from, mode='r'):
    """
    read file
    """
    data = None
    if os.path.exists(f_from):
        fobj = open_file(f_from, mode)
        if fobj:
            data = fobj.read()
            fobj.close()

    return data
