# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
wg-tool support utils`
"""
import os
from typing import IO


def open_file(path: str, mode: str) -> IO | None:
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


def copy_file(f_from: str, f_to: str) -> bool:
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


def write_file(data: str, f_to: str, mode: str = 'w') -> bool:
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


def read_file(f_from: str, mode: str = 'r') -> str:
    """
    read file
    """
    data: str = ''
    if os.path.exists(f_from):
        fobj = open_file(f_from, mode)
        if fobj:
            data = fobj.read()
            fobj.close()

    return data
