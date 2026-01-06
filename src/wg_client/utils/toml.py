# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2022-present Gene C <arch@sapience.com>
"""
toml helper functions
"""
from typing import Any
import os
import tomllib as toml

from .file import open_file


def read_toml_file(fpath: str) -> dict[str, Any]:
    """
    read toml file and return a dictionary
    """
    data_dict: dict[str, Any] = {}
    data: str = ''
    if os.path.exists(fpath):
        fobj = open_file(fpath, 'r')
        if fobj:
            data = fobj.read()
            fobj.close()
        if data:
            try:
                data_dict = toml.loads(data)
            except toml.TOMLDecodeError as exc:
                # print(f'TOML file format error {exc}\n')
                pass
    return data_dict
