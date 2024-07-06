"""
Who is logged in
"""
import os
import pwd
from .class_proc import run_prog

def who_logged_in(with_self:bool=True) -> [str]:
    """
    Returns list of logged in users 
    """
    [retc, output, _errors] = run_prog(['/usr/bin/users'])
    if retc != 0:
        return []

    # get list of unique users
    users = output.split()
    if not users:
        return []

    users = set(users)
    if not with_self:
        self = process_owner()
        users.discard(self)
    users = list(users)
    return users

def process_owner() -> str:
    """
    username of current process effective user id
    """
    uid = os.geteuid()
    user = pwd.getpwuid(uid)[0]
    return user
