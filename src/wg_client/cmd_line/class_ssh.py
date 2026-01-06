# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: © 2023-present  Gene C <arch@sapience.com>
'''
Manage ssh listener
'''
# pylint: disable=too-many-instance-attributes
import time
from typing import Callable

from .class_proc import MyProc
from .class_proc import MySignals

from .ssh_state import (read_ssh_pid, write_ssh_pid, check_ssh_pid, kill_ssh)
from .users import process_owner
from .utils import relative_time_string

class SshMgr:
    '''
    Set up & monitor a ssh listener process.
     - start and stop ssh listener process
     - if exits (not by stop request) then restart to keep it up
     Input: ssh_args : ['/usr/bin/ssh', '-R', <remote_fwd_string>, '-N', <server>]
    '''
    def __init__(self, test:bool, log:Callable=print):
        self.pid : int = -1
        self.server : str = None
        self.rport : str = None
        self.lip : str = None
        self.lport : str = None
        self.pargs : [str] = []
        self.proc : MyProc = None
        self.user : str = process_owner()
        self.log = log
        self.test : bool = test
        self.start_time : int = -1
        self.end_time : int = -1

        self.mysignals : MySignals = MySignals()

    def set_info(self, server:str, rport:str, lip:str, lport:str):
        '''
        Initialize the ssh info we need
        '''
        self.server = server
        self.rport = rport
        self.lip = lip
        self.lport = lport
        self.pargs = self.get_pargs()

    def get_pargs(self):
        '''
        pargs array to run
        '''
        remfwd = f'{self.rport}:{self.lip}:{self.lport}'
        pargs = []
        if self.test:
            pargs += ['/usr/bin/echo']
        pargs += ['/usr/bin/ssh', '-R', remfwd, '-N', self.server]
        return pargs

    def is_running(self, user:str=None):
        ''' check if running '''
        user_to_check = user
        if not user:
            user_to_check = self.user

        pid = read_ssh_pid(user_to_check)
        if not user or user == self.user:
            self.pid = pid

        if pid < 0:
            return False
        running = check_ssh_pid(pid, self.server, user=user)
        return running

    def stop(self):
        '''
        Kill the ssh listener
        So only way to stop this is from another process (wg-client --stop-ssh)
        Doing so will kill the parent wg-client process and it's child ssh
        '''
        is_running = self.is_running()
        if is_running:
            self.log('ssh:stop - terminating ssh process')
            if self.test:
                print(f'test: kill({self.pid}) server {self.server}')
            else:
                kill_ssh(self.pid, self.server)
        else:
            self.log(' ssh:stop: ssh not running')

    def running_time(self) -> str:
        '''
        Returns human readable time most recent ssh has been running
        '''
        secs = self.end_time - self.start_time
        delta_str = relative_time_string(secs)
        return delta_str

    def start(self):
        ''' run it '''
        running = self.is_running()
        if running:
            self.log('ssh:start already running')
            return

        if self.test:
            arg_str = ' '.join(self.pargs)
            self.log(f'ssh:start test: {arg_str}')
            return
        #
        # Set up ssh and reconnect if dropped
        #
        # NB This waits on the ssh process to exit.
        # Open up a pipe and wait for it to exit
        # the ssh (child) pid will be saved via write_ssh_pid
        # and set to "-1" when exited
        # In case ssh cannot be restarted we wait and try again.
        # Should we slowly increase the timeout in this case?
        #
        self.proc = MyProc(self.mysignals)
        delay_time = 30
        while True:
            re = 're-' if self.start_time > 0 else ''
            self.log(f'ssh:start - {re}connecting')

            self.start_time = time.time()
            self.proc.popen(self.pargs, logger=self.log, pid_saver=write_ssh_pid)
            self.end_time = time.time()

            delta_str = self.running_time()
            self.log(f'ssh: exited after {delta_str}')

            #
            # always wait before trying again
            #   - whether exited right away or after running for some time
            #
            time.sleep(delay_time)
