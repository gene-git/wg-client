# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
Command line Start and Stop Wireguard
"""
# pylint: disable=no-name-in-module,invalid-name,too-few-public-methods
# pylint: disable=too-many-instance-attributes
import os
import sys
from PyQt6.QtCore import (Qt)
from PyQt6.QtGui import (QGuiApplication, QIcon)
from PyQt6.QtWidgets import (QWidget, QApplication, QPlainTextEdit, QPushButton)
from PyQt6.QtWidgets import (QVBoxLayout, QGridLayout, QMainWindow)

from wg_client.proc import MySignals
from wg_client.utils import gLog
from wg_client.cmd_line.get_info import is_wg_running

from .get_info import is_ssh_running
from .get_info import get_wg_iface
from .get_info import get_ssh_server

from .class_worker import MyRunners


def _wg_client_cmd() -> str:
    """
    return path to wg-client
     Directory are checked in order
        - './', '/usr/bin'
    This helps for development. Since this process is run with user privs
    there is no security risk.
    """
    cmd: str = ''
    dirs = ('./', '/usr/bin')
    for tdir in dirs:
        cmd = os.path.join(tdir, 'wg-client')
        if os.path.exists(cmd):
            break
    return cmd


class WgClientGui():
    '''
    Main Application - GUI
    '''
    def __init__(self):
        """
        multi-threaded to keep gui responsive
        """
        self._setup: bool = False
        self.mysignals: MySignals
        self.runners: MyRunners

        #
        # invoke wg-client to do all the 'real' work
        #
        self.cmd: str = _wg_client_cmd()

        #
        # each runner has id_num - map it to what it does
        #
        self.id_num_map: dict[str, str] = {}

        #
        # gui stuff
        #
        self.MainWindow = QMainWindow()

        self.text = QPlainTextEdit()
        self.text.setReadOnly(True)

        #
        # gui has it's own log file
        #
        gLog.initialize()
        gLog.msg('Start GUI Client:')

        #
        # Get the wireguard interface name
        # uses to check if wg is running
        #
        self.wg_iface: str = get_wg_iface(gLog.msg)
        if self.wg_iface:
            gLog.msg(f'wg iface : {self.wg_iface}')
        else:
            self.message('Error: Failed to get wireguard interface')

        self.ssh_server: str = get_ssh_server(gLog.msg)
        if self.ssh_server == 'None':
            self.ssh_server = ''

    def setup(self):
        """
        Create Window
        """
        icon = '/usr/share/icons/hicolor/scalable/apps/wg-client.png'
        QGuiApplication.setDesktopFileName('wg-client')
        QGuiApplication.setWindowIcon(QIcon(icon))

        mwin = self.MainWindow
        mwin.setObjectName('VPN-Client-Window')
        mwin.setWindowTitle('VPN Client')
        mwin.resize(400, 250)
        btn_width = 100

        #
        # Start Stop buttons on top
        #
        btn_wg_up = QPushButton("Start VPN")
        btn_wg_up.clicked.connect(self.vpn_up)
        btn_wg_up.setFixedWidth(btn_width)

        btn_ssh_start = QPushButton("Start SSH")
        btn_ssh_start.clicked.connect(self.ssh_start)
        btn_ssh_start.setFixedWidth(btn_width)

        btn_ssh_stop = QPushButton("Stop SSH")
        btn_ssh_stop.clicked.connect(self.ssh_stop)
        btn_ssh_stop.setFixedWidth(btn_width)

        btn_wg_dn = QPushButton("Stop VPN")
        btn_wg_dn.clicked.connect(self.vpn_dn)
        btn_wg_dn.setFixedWidth(btn_width)

        #
        # Quit button on bottom
        #
        btn_quit = QPushButton("Quit All")
        btn_quit.clicked.connect(self.quit)
        btn_quit.setFixedWidth(btn_width)

        layout = QVBoxLayout()

        btn_layout_top = QGridLayout()
        btn_layout_bot = QGridLayout()

        btn_layout_top.addWidget(btn_wg_up, 0, 0)
        btn_layout_top.addWidget(btn_wg_dn, 0, 1)
        btn_layout_top.addWidget(btn_ssh_start, 1, 0)
        btn_layout_top.addWidget(btn_ssh_stop, 1, 1)
        btn_layout_top.setAlignment(Qt.AlignmentFlag.AlignRight)

        btn_layout_bot.addWidget(btn_quit, 0, 0)
        btn_layout_bot.setAlignment(Qt.AlignmentFlag.AlignRight)

        layout.addLayout(btn_layout_top)
        layout.addWidget(self.text)
        layout.addLayout(btn_layout_bot)
        layout.addStretch()

        widget = QWidget()
        widget.setLayout(layout)

        mwin.setCentralWidget(widget)

        #
        # worker pool
        #
        self.mysignals = MySignals()
        self.runners = MyRunners(self.mysignals)

        self._setup = True

    def message(self, s):
        ''' save message '''
        self.text.appendPlainText(s)

    def vpn_up(self):
        '''
        Start
        '''
        wg_running = is_wg_running(self.wg_iface)
        if wg_running:
            self.message('vpn already running')
            gLog.msg('vpn_up - vpn already running')

        else:
            gLog.msg('vpn_up - starting vpn')
            self.message('start vpn')
            pargs = [self.cmd, '--wg-up']

            if self.runners is None:
                txt = 'Error vpn_up: runners is None'
                gLog.msg(txt)
                self.message(txt)
                return

            id_num = self.runners.new_worker(self.complete, pargs)
            self.id_num_map[id_num] = 'start vpn'

    def vpn_dn(self):
        ''' Stop '''
        #
        # stop wireguard
        #
        wg_running = is_wg_running(self.wg_iface)
        if wg_running:
            gLog.msg('vpn_dn - stopping vpn')
            self.message('stop vpn')
            pargs = [self.cmd, '--wg-dn']
            id_num = self.runners.new_worker(self.complete, pargs)
            self.id_num_map[id_num] = 'stop vpn'
        else:
            gLog.msg('vpn_dn - vpn not running')
            self.message('vpn not running')

    def ssh_start(self):
        ''' Start SSH '''
        ssh_running = is_ssh_running(gLog.msg)
        if ssh_running:
            gLog.msg('ssh_start ssh already running')
            self.message(' ssh aleady running')
        else:
            wg_running = is_wg_running(self.wg_iface)
            if wg_running:
                gLog.msg('ssh_start - starting ssh listener')
                self.message('Starting ssh')
                pargs = [self.cmd, '--ssh-start']

                if self.runners is None:
                    txt = 'Error ssh_start: runners is None'
                    gLog.msg(txt)
                    self.message(txt)
                    return

                id_num = self.runners.new_worker(self.complete, pargs)
                self.id_num_map[id_num] = 'ssh start'
            else:
                gLog.msg('ssh_start - vpn not running')
                self.message('vpn not running - cant run ssh')

    def ssh_stop(self):
        ''' Stop SSH '''
        ssh_running = is_ssh_running(gLog.msg)
        if ssh_running:
            gLog.msg('ssh_stop - stopping ssh listener')
            self.message('Stopping ssh')
            pargs = [self.cmd, '--ssh-stop']
            if self.runners is None:
                txt = 'Error ssh_stop: runners is None'
                gLog.msg(txt)
                self.message(txt)
                return
            id_num = self.runners.new_worker(self.complete, pargs)
            self.id_num_map[id_num] = 'ssh stop'
        else:
            gLog.msg('ssh_stop ssh not running')
            self.message('ssh not running')

    def complete(self, id_num):
        '''
        we only allow 1 thing to run at a time -
        we're either starting or stopping.
        '''
        which = self.id_num_map[id_num]
        gLog.msg(f'{id_num} {which} : completed')
        self.message(f'{which} : completed')

    def quit(self):
        ''' Done '''
        gLog.msg('Quit : Client GUI')
        self.ssh_stop()
        self.vpn_dn()
        # self.runners.quit()
        sys.exit()


def MainGui(myname):
    """
    Application to start stop vpn
    Runs : wg-quick up/down <interface>
    Defaul <interface> is wgc but can be provided on command line
    The special <interface> 'test-dummy' makes program run in test mode
    Since wg-quick must be run as root :
    create /etc/sudoers.d/010-wg-quick
        <user>   ALL=NOPASSWD: SETENV: /usr/bin/wg-quick
    where <user> is the user who will be using the tool

    Add buttons:
        Run ssh -R to create remote listener
            ssh -R 44<xxx>:127.0.0.1:22 <vpn-server>
            <xxx> is take from /etc/wireguard/<interface>.conf
            Address         = a.b.c.<xxx>/32
        Run wg show latest-handshake
    """
    app = QApplication([myname])
    gui = WgClientGui()
    gui.setup()
    gui.MainWindow.show()
    sys.exit(app.exec())
