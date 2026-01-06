# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
wg-tool support utils`
"""
from datetime import datetime
from dateutil import relativedelta


def relative_time_string(seconds: int) -> str:
    '''
    Human readable
     - seems like the largest unit returned is days
       keep years/months in case that ever changes
    '''
    delt = relativedelta.relativedelta(seconds=seconds)
    res = ''
    if delt.years > 0:
        idelt = int(delt.years)
        res += f'{idelt} yrs'

    if delt.months > 0:
        if res:
            res += ' '
        idelt = int(delt.months)
        res += f'{idelt:02d} mons'

    if delt.days > 0:
        if res:
            res += ' '
        idelt = int(delt.days)
        res += f'{idelt:02d} days'

    time_seen = False
    if delt.hours > 0:
        time_seen = True
        if res:
            res += ' '
        idelt = int(delt.hours)
        res += f'{idelt:02d}h'

    if delt.minutes > 0 or time_seen:
        time_seen = True
        if res:
            res += ':'
        idelt = int(delt.minutes)
        res += f'{idelt:02d}m'

    if delt.seconds > 0:
        if res:
            res += ':'
        idelt = int(delt.minutes)
        res += f'{delt.seconds:02.1f}s'

    return res


def date_time_str(fmt: str = '%Y%m%d-%H:%M:%S'):
    """
    date time string
    """
    today = datetime.today()
    today_str = today.strftime(fmt)
    return today_str
