# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Gene C
"""
Ssh application process managerment
"""
from .state import (kill_program, read_pid, write_pid, check_pid)

def read_ssh_pid():
    """
    Read pid of last ssh
    """
    pid = read_pid('ssh')
    return pid

def check_ssh_pid(pid, host):
    """
    Check pid is valid
     - we write pid = -1 when child process terminates cleanly
    """
    pargs=['/usr/bin/ssh']
    if host:
        pargs += [host]

    pid_is_valid = check_pid(pid, pargs)
    return pid_is_valid

def write_ssh_pid(pid):
    """
    write pid of last ssh
    """
    write_pid(pid, 'ssh')

def kill_ssh(pid, host):
    """
    kill the ssh pid - with command match /usr/bin/ssh ... host
    """
    pargs = ['/usr/bin/ssh', host]
    kill_program(pid, pargs)
