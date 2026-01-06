# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Library
"""
from .class_client import WgClient

from .get_info import is_wg_running
from .get_info import get_wg_iface
from .get_info import is_ssh_running
from .get_info import get_ssh_server
