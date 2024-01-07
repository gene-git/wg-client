# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Gene C
"""
Command line options handling
"""
# pylint: disable=too-few-public-methods
import os
import argparse
import tomllib as toml
from tomllib import TOMLDecodeError
from .utils import open_file

def _parse_ssh_pfx(ssh_pfx):
    """
    Parse:
       'n'      -> [n]
       'n,m'    -> [n, m]
    """
    if not ssh_pfx:
        return None
    pfx_range = ssh_pfx.split('-')
    return pfx_range

def read_config(opts):
    """
    Reads file with defaults for:
     iface = 'wgc'
     ssh_server = "xxx.example.org"
     ssh_pfx = "n" or "n-m"
    """
    okay = True
    conf_dirs = ['./etc/wg-client', '/etc/wg-client']
    for conf_dir in conf_dirs:
        file = os.path.join(conf_dir, 'config')
        if os.path.exists(file):
            fobj = open_file(file, 'r')
            data = fobj.read()
            fobj.close()
            try:
                conf = toml.loads(data)
            except TOMLDecodeError as err:
                print(f'Error loading config {file} : {err}')
                return not okay

            for (key, val) in conf.items():
                setattr(opts, key, val)
            break
    return okay

class WgClientOpts:
    """
    Client Options
    """
    # pylint: disable=too-many-instance-attributes
    def __init__(self):
        desc = 'wg-client : manage wireguard client'
        self.okay = True
        self.wg_up = False
        self.fix_dns = False
        self.wg_dn = False
        self.ssh_start = False
        self.show_iface = False
        self.show_ssh_server = False
        self.show_ssh_running = False
        self.show_wg_running = False
        self.show_info = False
        self.iface = 'wgc'
        self.ssh_server = None
        self.ssh_pfx = None
        self.pfx_range = []

        # get config settings
        self.okay = read_config(self)
        if not self.okay:
            return
        if not self.ssh_pfx:
            self.ssh_pfx = '55'

        opts = [
                ['--test',       {'help' : 'Test mode', 'action' : 'store_true'}],
                ['--wg-up',      {'help' : 'Turn wireguard on', 'action' : 'store_true'}],
                ['--wg-dn',      {'help' : 'Turn wireguard off', 'action' : 'store_true'}],
                ['--ssh-start',  {'help' : 'Run ssh over vpn to create remote listener',
                                  'action' : 'store_true'}],
                ['--fix-dns',    {'help' : 'Fix wg dns if needed', 'action' : 'store_true'}],
                ['--ssh-stop',   {'help' : 'Kill ssh listener if its running',
                                  'action' : 'store_true'}],
                ['--ssh-pfx',    {'help' : 'ssh port(s). 2 digits: "nn" or "nn-mmm" for range',
                                  'default' : self.ssh_pfx}],
                ['--ssh-server', {'help' : 'Remote ssh server to set up listening port',
                                  'default' : self.ssh_server}],
                ['--show-iface',  {'help' : 'Report the wg interface name',
                                   'action' : 'store_true'}],
                ['--show-ssh-server', {'help' : 'Report the ssh server name',
                                       'action' : 'store_true'}],
                ['--show-ssh-running', {'help' : 'Report the ssh is running',
                                       'action' : 'store_true'}],
                ['--show-wg-running', {'help' : 'Report if wireguard is running',
                                       'action' : 'store_true'}],
                ['--show-info',        {'help' : 'Show all info',
                                        'action' : 'store_true'}],
                ['iface',        {'help' : 'Optional wg interface (test-dummy for test mode)',
                                  'default' : self.iface,
                                  'nargs' : '?'}],
               ]
        par = argparse.ArgumentParser(description=desc)
        for (opt, kwargs) in opts:
            par.add_argument(opt, **kwargs)

        parsed = par.parse_args()
        if parsed:
            for (opt, val) in vars(parsed).items() :
                setattr(self, opt, val)

        self.pfx_range = _parse_ssh_pfx(self.ssh_pfx)

    def __getattr__(self, name):
        """ non-set items simply return None makes it easy to check existence"""
        return None
