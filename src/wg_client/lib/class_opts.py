# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Gene C
"""
Command line options handling
"""
# pylint: disable=too-few-public-methods
# pylint: disable=too-many-statements
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

def get_avail_options(defaults:dict):
    """
    Current supported options
    """
    opts = []
    act = 'action'
    act_on = 'store_true'

    ohelp = 'Test mode'
    opt = ['--test', {'help' : ohelp, act : act_on}]
    opts.append(opt)

    ohelp = 'Turn wireguard on'
    opt = ['--wg-up', {'help' : ohelp, act : act_on}]
    opts.append(opt)

    ohelp = 'Turn wireguard off'
    opt = ['--wg-dn', {'help' : ohelp, act:act_on}]
    opts.append(opt)

    ohelp = 'Fix wg dns if needed'
    opt = ['--fix-dns', {'help' : ohelp, act:act_on}]
    opts.append(opt)

    ohelp = 'Auto fix wg dns if needed - stays running'
    opt = ['--fix-dns-auto-start', {'help' : ohelp, act:act_on}]
    opts.append(opt)

    ohelp = 'Auto fix wg dns if needed - stays running'
    opt = ['--fix-dns-auto-stop', {'help' : ohelp, act:act_on}]
    opts.append(opt)

    ohelp = 'Run ssh over vpn to create remote listener'
    opt = ['--ssh-start', {'help' : ohelp, act:act_on}]
    opts.append(opt)

    ohelp = 'Kill ssh listener if its running'
    opt = ['--ssh-stop', {'help' : ohelp, act:act_on}]
    opts.append(opt)

    ohelp = 'ssh port(s). 2 digits: "nn" or "nn-mmm" for range'
    default = defaults['ssh-pfx']
    opt = ['--ssh-pfx', {'help' : ohelp, 'default':default}]
    opts.append(opt)

    ohelp = 'Remote ssh server to set up listening port'
    default = defaults['ssh-server']
    opt = ['--ssh-server', {'help' : ohelp, 'default':default}]
    opts.append(opt)

    ohelp = 'Report the wg interface name'
    opt = ['--show-iface', {'help' : ohelp, act:act_on}]
    opts.append(opt)

    ohelp = 'Report the ssh server name'
    opt = ['--show-ssh-server', {'help' : ohelp, act:act_on}]
    opts.append(opt)

    ohelp = 'Report if ssh is running'
    opt = ['--show-ssh-running', {'help' : ohelp, act:act_on}]
    opts.append(opt)

    ohelp = 'Report if wireguard is running'
    opt = ['--show-wg-running', {'help' : ohelp, act:act_on}]
    opts.append(opt)

    ohelp = 'Report if auto fix dnsis running'
    opt = ['--show-fix-dns-auto', {'help' : ohelp, act:act_on}]
    opts.append(opt)

    ohelp = 'Report all info - alias for --status'
    opt = ['--show-info', {'help' : ohelp, act:act_on}]
    opts.append(opt)

    ohelp = 'Report all info'
    opt = ['--status', {'help' : ohelp, act:act_on}]
    opts.append(opt)

    ohelp = 'Display version'
    opt = ['--version', {'help' : ohelp, act:act_on}]
    opts.append(opt)

    ohelp = 'Optional wg interface (or test-dummy for test mode)'
    default = defaults['iface']
    opt = ['iface', {'help' : ohelp, 'default':default, 'nargs':'?'}]
    opts.append(opt)

    return opts


class WgClientOpts:
    """
    Client Options
    """
    # pylint: disable=too-many-instance-attributes
    def __init__(self):
        desc = 'wg-client : manage wireguard peer'
        self.okay = True
        self.wg_up = False
        self.fix_dns = False
        self.fix_dns_auto_start = False
        self.fix_dns_auto_stop = False
        self.wg_dn = False
        self.ssh_start = False
        self.ssh_stop = False
        self.show_iface = False
        self.show_ssh_server = False
        self.show_ssh_running = False
        self.show_wg_running = False
        self.show_fix_dns_auto = False
        self.show_info = False
        self.status = False
        self.version = False
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

        defaults = {'ssh-pfx' : self.ssh_pfx,
                    'ssh-server' : self.ssh_server,
                    'iface' : self.iface,
                   }

        opts = get_avail_options(defaults)

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
