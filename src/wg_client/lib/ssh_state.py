# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Gene C
"""
Ssh application process managerment
"""
from .state import (get_parent_pid, kill_program, read_pid, write_pid, check_pid)

def read_ssh_pid(user:str=None) -> int:
    """
    Read pid of last ssh
    """
    pid = read_pid('ssh', user)
    return pid

def check_ssh_pid(pid, host, user:str=None) -> bool:
    """
    Check pid is valid
     - we write pid = -1 when child process terminates cleanly
    """
    pargs=['/usr/bin/ssh']
    if host:
        pargs += [host]

    pid_is_valid = check_pid(pid, pargs, user=user)
    return pid_is_valid

def write_ssh_pid(pid):
    """
    write pid of last ssh
    """
    write_pid(pid, 'ssh')

def kill_ssh(pid, server):
    """
    Since ssh is started by wg-client which will auto restart ssh if it dies
    we first stop the parent wg-client process then ssh if its not dead
     1 - kill wg-client
     2 - kill ssh if alive
    """
    if pid < 0:
        return

    pargs = ['/usr/bin/ssh', server]
    ppid = get_parent_pid(pid, pargs)
    if ppid > 0:
        ppargs = ['/usr/bin/wg-client', '--ssh-start']
        kill_program(ppid, ppargs)

    pargs = ['/usr/bin/ssh', server]
    kill_program(pid, pargs)
