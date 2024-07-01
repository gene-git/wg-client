# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Gene C
"""
App stuff
"""
import os
import signal
from pathlib import Path
import pwd
from collections.abc import Collection
import psutil
from .utils import open_file

def all_in(col1:Collection, col2:Collection):
    """ return true if every element of col1 is in col2 """
    s_col1 = set(col1)
    s_col2 = set(col2)
    return s_col1.intersection(s_col2) == s_col1

def is_pid_running(pid:str, pargs:[str]=None, check_user:bool=True) -> bool:
    """
    Check if pid is active
       - check username, cmd and args to be cautious of pid rollover
       - unlikely but conceivable in a 64 bit world
    """
    # pylint: disable=too-many-return-statements
    if not pid or int(pid) < 0:
        return False

    pid_exists = psutil.pid_exists(pid)
    if not pid_exists:
        return False

    uid = os.geteuid()
    username = pwd.getpwuid(uid)[0]

    process = psutil.Process(pid)
    attribs = process.as_dict(attrs=['username', 'exe', 'cmdline'])

    p_username = attribs['username']
    p_cmdline = attribs['cmdline']

    if check_user and username != p_username:
        # not our process
        return False
    #
    # Care is needed - for program running with shebang p_exe will be the shell
    #
    if pargs and pargs[0]:
        if p_cmdline and len(p_cmdline) > 0:
            if not all_in(pargs, p_cmdline):
                return False
    return True

def homedir():
    """ return user home dir """
    return Path.home()

def get_appdir():
    """
    Place we save state
    """
    home = homedir()
    app_dir = os.path.join(home, '.local/share/state/wg-client')
    os.makedirs(app_dir, exist_ok=True)
    return app_dir

def read_pidfile(fpath):
    """
    Read file with PID
    """
    pid = None
    if not os.path.exists(fpath):
        return pid

    fobj = open_file(fpath, 'r')
    if fobj:
        data = fobj.read()
        fobj.close()
        pid = int(data.strip())
    return pid

def write_pidfile(pid, fpath):
    """
    write
     - caller ensurs basedir exists
    """
    if not pid:
        return

    fobj = open_file(fpath, 'w')
    if fobj:
        pid = str(pid)
        fobj.write(pid)
        fobj.close()

def pid_filename(tag:str) -> str:
    """
    full path for pid file
      <app_dir>/<tag>.pid
    """
    app_dir = get_appdir()
    pidfile = os.path.join(app_dir, f'{tag}.pid')
    return pidfile

def read_pid(tag:str) -> int:
    """
    Read pid of last ssh
    """
    pidfile = pid_filename(tag)
    pid = read_pidfile(pidfile)
    return pid

def check_pid(pid, pargs:[str], check_user:bool=True):
    """
    Check pid is valid
     - we write pid = -1 when child process terminates cleanly
    """
    if not pid or int(pid) < 0:
        return False

    pid_is_valid = is_pid_running(pid, pargs=pargs, check_user=check_user)
    return pid_is_valid

def write_pid(pid:str, tag:str) -> None:
    """
    Save pid to pidfile
    """
    pidfile = pid_filename(tag)
    write_pidfile(pid, pidfile)

def kill_program(pid:str, pargs:[str]) -> None:
    """
    kill processs with pid and program given by pargs
     - check its valid
    """
    pid_is_valid = is_pid_running(pid, pargs=pargs)
    if pid_is_valid:
        os.kill(pid, signal.SIGKILL)
