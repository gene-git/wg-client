# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Who is logged in
"""
import os
import pwd
from pyconcurrent import run_prog


def who_logged_in(with_self: bool = True) -> list[str]:
    """
    Returns list of logged in users
    """
    users: list[str] = []

    (retc, output, _errors) = run_prog(['/usr/bin/users'])
    if retc != 0 or not output:
        return users

    # get list of unique users
    users = output.split()
    if not users:
        return users

    users_set = set(users)
    if not with_self:
        self_user = process_owner()
        users_set.discard(self_user)

    users = list(users_set)
    return users


def process_owner() -> str:
    """
    username of current process effective user id
    """
    uid = os.geteuid()
    user = pwd.getpwuid(uid)[0]
    return user
