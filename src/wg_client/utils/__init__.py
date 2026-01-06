# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Common utils
"""
from .class_logger import MyLog
from .version import version
from .time import relative_time_string

from .file import open_file
from .file import copy_file
from .file import write_file
from .file import read_file

from .toml import read_toml_file
