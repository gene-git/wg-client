# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Command line options handling
"""
# pylint: disable=too-few-public-methods
# pylint: disable=too-many-statements
import os
from typing import Any
import argparse

from wg_client.utils import read_toml_file

type _Opt = tuple[str | tuple[str, ...], dict[str, Any]]


def _parse_ssh_pfx(ssh_pfx: str) -> list[str]:
    """
    Parse:
       'n'      -> [n]
       'n,m'    -> [n, m]
    """
    pfx_range: list[str] = []
    if not ssh_pfx:
        return pfx_range
    pfx_range = ssh_pfx.split('-')
    return pfx_range


def read_config(opts: WgClientOpts) -> bool:
    """
    Reads file with defaults for:
     iface = 'wgc'
     ssh_server = "xxx.example.org"
     ssh_pfx = "n" or "n-m"
    """
    okay = True
    conf: dict[str, Any] = {}

    conf_dirs = ['./etc/wg-client', '/etc/wg-client']
    for conf_dir in conf_dirs:
        file = os.path.join(conf_dir, 'config')
        conf = read_toml_file(file)
        if conf:
            for (key, val) in conf.items():
                setattr(opts, key, val)
            break

    if not conf:
        print(f'Error loading config {file}')
        return not okay
    return okay


def get_avail_options(defaults: dict[str, str]) -> list[_Opt]:
    """
    Current supported options
    """
    opts: list[_Opt] = []
    opt: _Opt
    ohelp: str

    ohelp = 'Test mode'
    opt = ('--test', {'help': ohelp, 'action': 'store_true'})
    opts.append(opt)

    ohelp = 'Turn wireguard on'
    opt = ('--wg-up', {'help': ohelp, 'action': 'store_true'})
    opts.append(opt)

    ohelp = 'Turn wireguard off'
    opt = ('--wg-dn', {'help': ohelp, 'action': 'store_true'})
    opts.append(opt)

    # ohelp = 'Fix wg dns if needed'
    # opt = ('--fix-dns', {'help': ohelp, 'action': 'store_true'})
    # opts.append(opt)

    ohelp = 'Auto fix wg dns if needed - stays running'
    opt = ('--fix-dns-auto-start', {'help': ohelp, 'action': 'store_true'})
    opts.append(opt)

    ohelp = 'Auto fix wg dns if needed - stays running'
    opt = ('--fix-dns-auto-stop', {'help': ohelp, 'action': 'store_true'})
    opts.append(opt)

    ohelp = 'Run ssh over vpn to create remote listener'
    opt = ('--ssh-start', {'help': ohelp, 'action': 'store_true'})
    opts.append(opt)

    ohelp = 'Kill ssh listener if its running'
    opt = ('--ssh-stop', {'help': ohelp, 'action': 'store_true'})
    opts.append(opt)

    ohelp = 'ssh port(s). 2 digits: "nn" or "nn-mmm" for range'
    default = defaults['ssh-pfx']
    opt = ('--ssh-pfx', {'help': ohelp, 'default': default})
    opts.append(opt)

    ohelp = 'Remote ssh server to set up listening port'
    default = defaults['ssh-server']
    opt = ('--ssh-server', {'help': ohelp, 'default': default})
    opts.append(opt)

    ohelp = 'Report the wg interface name'
    opt = ('--show-iface', {'help': ohelp, 'action': 'store_true'})
    opts.append(opt)

    ohelp = 'Report the ssh server name'
    opt = ('--show-ssh-server', {'help': ohelp, 'action': 'store_true'})
    opts.append(opt)

    ohelp = 'Report if ssh is running'
    opt = ('--show-ssh-running', {'help': ohelp, 'action': 'store_true'})
    opts.append(opt)

    ohelp = 'Report if wireguard is running'
    opt = ('--show-wg-running', {'help': ohelp, 'action': 'store_true'})
    opts.append(opt)

    ohelp = 'Report if auto fix dns is running'
    opt = ('--show-fix-dns-auto', {'help': ohelp, 'action': 'store_true'})
    opts.append(opt)

    ohelp = 'Display status - alias for --status'
    opt = ('--show-info', {'help': ohelp, 'action': 'store_true'})
    opts.append(opt)

    ohelp = 'Display status'
    opt = ('--status', {'help': ohelp, 'action': 'store_true'})
    opts.append(opt)

    ohelp = 'Display version'
    opt = ('--version', {'help': ohelp, 'action': 'store_true'})
    opts.append(opt)

    ohelp = 'Optional wg interface (or test-dummy for test mode)'
    default = defaults['iface']
    opt = ('iface', {'help': ohelp, 'default': default, 'nargs': '?'})
    opts.append(opt)

    return opts


class WgClientOpts:
    """
    Client Options
    """
    # pylint: disable=too-many-instance-attributes
    def __init__(self):
        desc = 'wg-client : manage wireguard peer'
        self.okay: bool = True
        self.wg_up: bool = False
        # self.fix_dns: bool = False
        self.fix_dns_auto_start: bool = False
        self.fix_dns_auto_stop: bool = False
        self.wg_dn: bool = False
        self.ssh_start: bool = False
        self.ssh_stop: bool = False
        self.show_iface: bool = False
        self.show_ssh_server: bool = False
        self.show_ssh_running: bool = False
        self.show_wg_running: bool = False
        self.show_fix_dns_auto: bool = False
        self.show_info: bool = False
        self.status: bool = False
        self.version: bool = False
        self.iface: str = 'wgc'
        self.ssh_server: str = ''
        self.ssh_pfx: str = ''
        self.pfx_range: list[str] = []

        # get config settings
        self.okay = read_config(self)
        if not self.okay:
            return
        if not self.ssh_pfx:
            self.ssh_pfx = '55'

        defaults = {
                'ssh-pfx': self.ssh_pfx,
                'ssh-server': self.ssh_server,
                'iface': self.iface,
                }

        opts = get_avail_options(defaults)

        par = argparse.ArgumentParser(description=desc)
        for opt in opts:
            opt_list, kwargs = opt
            if isinstance(opt_list, str):
                par.add_argument(opt_list, **kwargs)
            else:
                par.add_argument(*opt_list, **kwargs)

        parsed = par.parse_args()
        if parsed:
            for (key, val) in vars(parsed).items():
                setattr(self, key, val)

        self.pfx_range = _parse_ssh_pfx(self.ssh_pfx)

    def __getattr__(self, name):
        """ non-set items simply return None makes it easy to check existence"""
        return None
