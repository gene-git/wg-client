# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: © 2023-present  Gene C <arch@sapience.com>
"""
wg-tool support utils`
"""
import os
from datetime import datetime
from dateutil import relativedelta

def relative_time_string(seconds:int) -> str:
    '''
    Human readable 
    '''
    delt = relativedelta.relativedelta(seconds=seconds)
    res = ''
    if delt.years > 0:
        res += f'{delt.years} years'

    if delt.months > 0:
        if res :
            res += ' '
        res += f'{delt.months} months'

    if delt.days > 0:
        if res :
            res += ' '
        res += f'{delt.days} days'

    if delt.hours > 0:
        if res :
            res += ' '
        res += f'{delt.hours} hours'

    if delt.minutes > 0:
        if res :
            res += ' '
        res += f'{delt.minutes} minutes'

    if delt.seconds > 0:
        if res :
            res += ' '
        res += f'{delt.seconds} seconds'

    return res

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
