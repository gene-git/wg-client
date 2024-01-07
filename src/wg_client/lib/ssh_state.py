# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Gene C
"""
App stuff
"""
import os
import signal
from pathlib import Path
import pwd
import psutil
from .utils import open_file

def is_pid_running(pid:str, pargs:[str]=None) -> bool:
    """
    Check if pid is active
       - check username, cmd and args to be cautious of pid rollover 
       - unlikely as that is in a 64 bit world
    """
    # pylint: disable=too-many-return-statements
    if not pid:
        return False

    pid_exists = psutil.pid_exists(pid)
    if not pid_exists:
        return False

    uid = os.geteuid()
    username = pwd.getpwuid(uid)[0]

    process = psutil.Process(pid)
    #attribs = process.as_dict(attrs=['pid', 'exe', 'username', 'status', 'cmdline'])
    attribs = process.as_dict(attrs=['username', 'exe', 'cmdline'])

    p_username = attribs['username']
    p_exe = attribs['exe']
    p_cmdline = attribs['cmdline']
    p_args = None

    if username != p_username:
        # not our process
        return False

    cmd = None
    args = None
    if pargs and pargs[0]:
        cmd = pargs[0]
        if len(pargs) > 1:
            args = pargs[1:]

        if len(p_cmdline) > 1:
            p_args = p_cmdline[1:]

    if cmd and cmd != p_exe:
        # wrong process
        return False

    if args :
        if not p_args:
            # missing args???
            return False

        for arg in args:
            if arg not in p_args:
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

def pid_filename():
    """
    where the ssh pid is written
     - can only be one of these
    """
    app_dir = get_appdir()
    ssh_pidfile = os.path.join(app_dir, 'ssh_pid')
    return ssh_pidfile

def read_ssh_pid():
    """
    Read pid of last ssh
    """
    ssh_pidfile = pid_filename()
    pid = read_pidfile(ssh_pidfile)

    return pid

def check_ssh_pid(pid, host):
    """
    Check pid is valid
     - we write pid = -1 when child process terminates cleanly
    """
    if not pid or int(pid) < 0:
        return False

    pargs=['/usr/bin/ssh']
    if host:
        pargs += [host]

    pid_is_valid = is_pid_running(pid, pargs=pargs)
    return pid_is_valid

def write_ssh_pid(pid):
    """
    write pid of last ssh
    """
    ssh_pidfile = pid_filename()
    write_pidfile(pid, ssh_pidfile)

def kill_ssh(pid, host):
    """
    kill the pid
     - for /usr/bin/ssh ... host
     - double check its valid
    """
    pid_is_valid = is_pid_running(pid, pargs=['/usr/bin/ssh', host])
    if pid_is_valid:
        os.kill(pid, signal.SIGKILL)
