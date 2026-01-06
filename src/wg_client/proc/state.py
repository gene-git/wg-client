# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
App stuff
"""
import os
import signal
from typing import Iterable
import psutil

from wg_client.utils import open_file

from .users import process_owner


def all_in(col1: Iterable, col2: Iterable):
    '''
    return true if every element of collection col1 is in col2
    '''
    s_col1 = set(col1)
    s_col2 = set(col2)
    return s_col1.intersection(s_col2) == s_col1


def _get_process(pid: int, pargs: list[str] | None = None, user: str = '') -> psutil.Process | None:
    """
    Return psutil.process for given (pid, pargs, user)
    or None if not found
    """
    # pylint: disable=too-many-return-statements
    if pid <= 1:
        return None

    pid_exists = psutil.pid_exists(pid)
    if not pid_exists:
        return None

    if user:
        username = user
    else:
        username = process_owner()

    try:
        process = psutil.Process(pid)
    except psutil.Error:
        return None

    attribs = process.as_dict(attrs=['username', 'exe', 'cmdline'])

    p_username = attribs['username']
    p_cmdline = attribs['cmdline']

    if username and username != p_username:
        # wrong user's process
        return None
    #
    # Care is needed - for program running with shebang p_exe will be the shell
    #
    if pargs is not None and pargs[0]:
        if p_cmdline and len(p_cmdline) > 0:
            if not all_in(pargs, p_cmdline):
                return None
    return process


def is_pid_running(pid: int, pargs: list[str] | None = None, user: str = '') -> bool:
    """
    Check if pid is active
       - check username, cmd and args to be cautious of pid rollover
       - unlikely but conceivable in a 64 bit world
    """
    process = _get_process(pid, pargs, user)
    if process:
        return True
    return False


def get_parent_pid(pid: int, pargs: list[str] | None = None, user: str = '') -> int:
    '''
    Find parent pid of process
    '''
    ppid = -1
    process = _get_process(pid, pargs, user)
    if process:
        ppid = process.ppid()
    return ppid


def homedir(user: str = ''):
    """
    return user home dir
    If no user given, then self (process owner)
    """
    if user:
        upath = f'~{user}'
    else:
        upath = '~'
    hdir = os.path.expanduser(upath)
    return hdir


def get_appdir(user: str = ''):
    """
    Place we save state
    """
    home = homedir(user)
    app_dir = os.path.join(home, '.local/share/state/wg-client')
    return app_dir


def pid_filename(tag: str, user: str = '') -> str:
    """
    full path for pid file
      <app_dir>/<tag>.pid
    """
    app_dir = get_appdir(user)
    pidfile = os.path.join(app_dir, f'{tag}.pid')
    return pidfile


def read_file(fpath: str) -> str:
    """
    Read file with and return contents as str
    return None if file doesn't exist
    """
    contents = ''
    if not os.path.exists(fpath):
        return contents

    fobj = open_file(fpath, 'r')
    if fobj:
        contents = fobj.read()
        fobj.close()
    return contents


def write_file(contents: str, fpath: str):
    """
    write contents to file
     - caller ensures basedir exists
    """
    if not contents:
        return

    fobj = open_file(fpath, 'w')
    if fobj:
        fobj.write(contents)
        fobj.close()


def read_pidfile(fpath: str) -> int:
    """
    Read file with PID
    """
    pid = -1
    contents = read_file(fpath)
    if contents:
        pid = int(contents.strip())
    return pid


def write_pidfile(pid: int, fpath: str):
    """
    write
     - caller ensures basedir exists
    """
    if pid <= 1:
        return

    contents = str(pid)
    write_file(contents, fpath)


def read_pid(tag: str, user: str = '') -> int:
    """
    Read pid of last ssh
    Requires root if pidfile not readable by process owner
    If insufficient perm to read pidfile pid is set to None
    """
    pidfile = pid_filename(tag, user)
    pid = read_pidfile(pidfile)
    return pid


def check_pid(pid: int, pargs: list[str], user: str = '') -> bool:
    """
    Check pid is valid
     - we write pid = -1 when child process terminates cleanly
     - root permitted to read pidfile
    """
    if pid <= 1:
        return False

    pid_is_valid = is_pid_running(pid, pargs=pargs, user=user)
    return pid_is_valid


def write_pid(pid: int, tag: str) -> None:
    """
    Save pid to pidfile
    only owner can write pidfile
    """
    pidfile = pid_filename(tag)
    app_dir = os.path.dirname(pidfile)
    os.makedirs(app_dir, exist_ok=True)
    write_pidfile(pid, pidfile)


def kill_program(pid: int, pargs: list[str]) -> None:
    """
    kill processs with pid and program given by pargs
     - check its valid
     - only owner allowed to kill
    """
    pid_is_valid = is_pid_running(pid, pargs=pargs)
    if pid_is_valid:
        os.kill(pid, signal.SIGKILL)
