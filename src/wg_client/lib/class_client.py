# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Gene C
"""
Class for Start, Stop Wireguard client on linux
 - handles wg-quick up/dn
 - ssh to establish listener

 If not root, then we use sudo when rnning wg-quick.
 when this is called from GUI then user must have suoders to run with no password.
 e.g.
 /etc/sudoers.d/wg-quick
 username   ALL=NOPASSWD: SETENV: /usr/bin/wg-quick
"""
# pylint disable=no-name-in-module,invalid-name,too-few-public-methods
# pylint: disable=too-many-instance-attributes
import os
from .class_proc import MyProc
from .class_proc import MySignals
from .ip_addr import iface_to_ips
from .ssh_listener import ssh_listener_args
from .ssh_state import read_ssh_pid
from .ssh_state import write_ssh_pid
from .ssh_state import check_ssh_pid
from .ssh_state import kill_ssh
from .class_opts import WgClientOpts
from .class_logger import MyLog
from .get_info import is_wg_running
from .class_resolv import WgResolv

def wg_quick_cmd(test, euid, updn, iface):
    '''
    consruct pargs for running wg_quick
    if confg is "test-dummy" then prepend /usr/bim/echo
    '''
    pargs = []
    if test:
        pargs = ['/usr/bin/echo']

    if euid != 0:
        pargs += ['/usr/bin/sudo']
    pargs += ['/usr/bin/wg-quick', updn, iface]
    return pargs

def wg_fix_dns_cmd(test, euid):
    '''
    consruct pargs for running wg_quick
    if confg is "test-dummy" then prepend /usr/bim/echo
    '''
    pargs = []
    if test:
        pargs = ['/usr/bin/echo']

    if euid != 0:
        pargs += ['/usr/bin/sudo']
    pargs += ['/usr/lib/wg-client/wg-fix-dns']
    return pargs

class WgClient():
    """ Primary Class (no gui) """
    def __init__(self):
        self.iface = None
        self.euid = os.geteuid()
        self.wg_ip = None
        self.test = False
        self.pid = None         # so we share after looking it up

        self.opts = WgClientOpts()
        self.iface = self.opts.iface
        if self.opts.test or self.iface == 'test-dummy' :
            self.test = True

        self.ssh_proc = None
        self.run_proc = None

        self.mysignals = MySignals()
        self.logger = MyLog('wg-client')
        self.log('wg-client starting')

        self.resolv = WgResolv(log=self.log)

    def log(self, msg):
        """ log file """
        self.logger.log(msg)

    def show_iface(self):
        """
        Simply print/log the wg iface that would be used
        """
        print(f'{self.iface}')
        self.log(f'wg-iface: {self.iface}')

    def show_ssh_server(self):
        """
        Simply print/log the ssh_server that would be used
        """
        ssh_server = self.opts.ssh_server
        if not ssh_server:
            ssh_server = 'None'
        print(f'{ssh_server}')
        self.log(f'ssh server: {ssh_server}')

    def show_ssh_running(self):
        """
        Simply print/log the wg iface that would be used
        """
        ssh_running = self.is_ssh_running()
        print(f'{ssh_running}')
        self.log(f'ssh_running: {ssh_running}')

    def show_ssh_pfx(self):
        """
        Simply print/log the wg iface that would be used
        """
        print(f'{self.opts.ssh_pfx}')
        self.log(f'ssh_pfx: {self.opts.ssh_pfx}')

    def show_wg_running(self):
        """
        Simply print/log the wg iface that would be used
        """
        wg_running = is_wg_running(self.iface)
        print(f'{wg_running}')
        self.log(f'wg_running: {wg_running}')

    def is_ssh_running(self):
        """
        Check saved PID and check if running
         - if ssh_server missing, we'll check pid is valid
        """
        self.pid = read_ssh_pid()
        ssh_server = self.opts.ssh_server
        is_running = check_ssh_pid(self.pid, ssh_server)
        return is_running

    def wg_up(self):
        """
        wg-quick up
         - parse output for our wg IP
        """
        pargs = wg_quick_cmd(self.test, self.euid, 'up', self.iface)
        self.log('wg-up requested')
        self.runit(pargs)

    def fix_dns(self):
        """
        Call wg-fix-dns
         - if wg is running there will be resolv.conf.wg
           to restore resolv.conf if its been overwritten.
           Happens sleep / resume where network start makese a new
           resolv.conf - we put back the vpn one (requires root)
        ? Should we check wg is running before invoking ?
        """
        self.log('fix-dns requested')
        if not is_wg_running(self.iface):
            self.log(' wg not running - skipping fix dns resolv')
            return

        if self.euid == 0:
            if self.resolv.restore_resolv():
                self.log(' done')
        else:
            # use sudo on standalone program
            pargs = wg_fix_dns_cmd(self.test, self.euid)
            self.log(f' not root so call: {pargs}')
            self.runit(pargs)

    def get_wg_ip(self):
        """
        find ip4 for wg iface
        todo: Add support for 3 digit ip6
        """
        (ips4, _ips6) = iface_to_ips(self.iface)
        self.wg_ip = None
        if ips4:
            self.wg_ip = ips4[0]
        #elif ip6s:
        #    self.wg_ip = ip6s[0]
        if not self.wg_ip:
            self.log('Err: failed to find wg iface IP')

    def wg_dn(self):
        """
        wg-quick dn
        """
        pargs = wg_quick_cmd(self.test, self.euid, 'down', self.iface)
        self.log('wg-dn requested')
        self.runit(pargs)

    def stop_ssh_listener(self):
        """
        kill based on saved pid
         - is running check fills self.pid
        """
        is_running = self.is_ssh_running()
        if is_running:
            if self.test:
                print(f'test: kill({self.pid}) server {self.opts.ssh_server}')
            else:
                kill_ssh(self.pid, self.opts.ssh_server)
        else:
            self.log('ssh not running - nothing to stop')

    def ssh_listener(self):
        """
        Run ssh over vpn to set up remote listener
         - only run if vpn is running
         - should we skip if on local network?
        This will not return until the ssh process exits
        """
        self.log('ssh-listener requested')
        self.get_wg_ip()
        if not self.wg_ip:
            self.log('VPN not running : can\'t start ssh')
            if not self.test:
                return
            self.wg_ip = '10.10.10.123'
            self.log(f'Test: using fake ip : {self.wg_ip}')

        if not self.opts.ssh_server:
            self.log('No ssh_server provided')
            return

        opts = self.opts
        ssh_server = opts.ssh_server
        ssh_args = ssh_listener_args(self.test, self.wg_ip, ssh_server, opts.pfx_range)

        #
        # Check not already running:
        #
        ssh_running = self.is_ssh_running()
        if ssh_running:
            self.log(f'ssh {ssh_server} already running with pid = {self.pid}')
            print(f'ssh to {ssh_server} already running')
            return

        #
        # open up pipe to ssh and wait for it to finish
        # child pid will be saved via write_ssh_pid and
        # set to "-1" when child exits normaly
        #
        if self.test:
            arg_str = ' '.join(ssh_args)
            print(f'test: {arg_str}')
        else:
            self.ssh_proc = MyProc(self.mysignals)
            self.ssh_proc.popen(ssh_args, logger=self.log, pid_saver=write_ssh_pid)

    def runit(self, pargs):
        """
        run a program via subprocess.run
            - used for wg-quick up/down
            - returns stdout
        """
        if not pargs:
            return
        self.run_proc = MyProc(self.mysignals)
        (_ret, _outs, _errs) = self.run_proc.popen(pargs, logger=self.log, pid_saver=write_ssh_pid)

    def show_info(self):
        """ show iface, ssh running and wg running """
        print(f'{"wg iface":>15s} : ', end='')
        self.show_iface()

        print(f'{"wg_running":>15s} : ', end='')
        self.show_wg_running()

        print(f'{"ssh_server":>15s} : ', end='')
        self.show_ssh_server()

        print(f'{"ssh_pfx":>15s} : ', end='')
        self.show_ssh_pfx()

        print(f'{"ssh_running":>15s} : ', end='')
        self.show_ssh_running()


    def do_all(self):
        """
        Perform the requested tasks
        """
        if self.opts.show_iface:
            self.show_iface()

        if self.opts.show_ssh_server:
            self.show_ssh_server()

        if self.opts.show_ssh_running:
            self.show_ssh_running()

        if self.opts.show_wg_running:
            self.show_wg_running()

        if self.opts.show_info:
            self.show_info()

        if self.opts.fix_dns:
            self.fix_dns()

        if self.opts.wg_up:
            self.wg_up()

        if self.opts.wg_dn:
            self.wg_dn()

        if self.opts.ssh_start:
            self.ssh_listener()

        if self.opts.ssh_stop:
            self.stop_ssh_listener()
