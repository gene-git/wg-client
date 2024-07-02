# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Gene C
"""
Command line Start and Stop Wireguard
"""
# pylint: disable=no-name-in-module,invalid-name,too-few-public-methods
# pylint: disable=too-many-instance-attributes
import os
import sys
from PyQt6.QtCore import (Qt)
from PyQt6.QtGui import QGuiApplication,QIcon
from PyQt6.QtWidgets import (QWidget, QApplication, QPlainTextEdit, QPushButton)
from PyQt6.QtWidgets import (QVBoxLayout, QGridLayout, QMainWindow)
from .class_proc import MySignals
from .class_worker import MyRunners
from .class_logger import MyLog
from .get_info import is_wg_running
from .get_info import is_ssh_running
from .get_info import get_wg_iface
from .get_info import get_ssh_server

def _wg_client_cmd():
    """
    return path to wg-client
     Directory are checked in order
        - './', '/usr/bin'
    This helps for development. Since this process is run with user privs
    there is no security risk.
    """
    cmd = None
    dirs = ('./', '/usr/bin')
    for tdir in dirs:
        cmd = os.path.join(tdir, 'wg-client')
        if os.path.exists(cmd):
            break
    return cmd

class WgClientGui():
    ''' Main Application - GUI'''
    def __init__(self):
        #
        # multi-threaded to keep gui responsive
        #
        self.mysignals = None
        self.runners = None

        #
        # invoke wg-client to do all the 'real' work
        #
        self.cmd = _wg_client_cmd()

        #
        # each runner has id_num - map it to what it does
        #
        self.id_num_map = {}

        #
        # gui stuff
        #
        self.MainWindow = QMainWindow()

        self.text = QPlainTextEdit()
        self.text.setReadOnly(True)

        #
        # gui has it's own log file
        #
        self.logger = MyLog('wg-client-gui')
        self.log('Start GUI Client:')

        logfile = self.logger.logfile()
        self.message(f'Info is logged to {logfile}')

        #
        # Get the wireguard interface name
        # uses to check if wg is running
        #
        self.wg_iface = get_wg_iface(self.log)
        if self.wg_iface:
            self.log(f'wg iface : {self.wg_iface}')
        else:
            self.message('Error: Failed to get wireguard interface')

        self.ssh_server = get_ssh_server(self.log)
        if self.ssh_server == 'None':
            self.ssh_server = None

    def log(self, msg):
        """ log to file """
        self.logger.log(msg)

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

        btn_layout_top.addWidget(btn_wg_up, 0,0)
        btn_layout_top.addWidget(btn_wg_dn, 0,1)
        btn_layout_top.addWidget(btn_ssh_start, 1,0)
        btn_layout_top.addWidget(btn_ssh_stop, 1,1)
        btn_layout_top.setAlignment(Qt.AlignmentFlag.AlignRight)

        btn_layout_bot.addWidget(btn_quit, 0,0)
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
        self.runners = MyRunners(self.log, self.mysignals)

    def message(self, s):
        ''' save message '''
        self.text.appendPlainText(s)

    def vpn_up(self):
        ''' Start '''
        wg_running = is_wg_running(self.wg_iface)
        if wg_running :
            self.message('vpn already running')
            self.log('vpn_up - vpn already running')
            #
            # not needed since --fix-dns-auto-start daemon should be running
            # wg-client will check on daemon and exit if running
            #
            self.log(' Checking wg-fix-dns')
            self.message(' double checking dns fix')
            pargs = [self.cmd, '--fix-dns']

            id_num = self.runners.new_worker(self.complete, pargs)
            self.id_num_map[id_num] = 'fix dns'

        else :
            self.log('vpn_up - starting vpn')
            self.message('start vpn')
            pargs = [self.cmd, '--wg-up']

            id_num = self.runners.new_worker(self.complete, pargs)
            self.id_num_map[id_num] = 'start vpn'

            #
            #  --fix-dns-auto-start
            #  Give enough time for wg to start
            #
            self.log(' Starting wg-fix-dns-auto-start')
            self.message(' starting dns auto fix')
            pargs = [self.cmd, '--fix-dns-auto-start']
            id_num = self.runners.new_worker(self.complete, pargs)
            self.id_num_map[id_num] = 'start auto fix dns'

    def vpn_dn(self):
        ''' Stop '''
        wg_running = is_wg_running(self.wg_iface)
        if wg_running:
            self.log('vpn_dn - stopping vpn')
            self.message('stop vpn')
            pargs = [self.cmd, '--wg-dn']
            id_num = self.runners.new_worker(self.complete, pargs)
            self.id_num_map[id_num] = 'stop vpn'
        else :
            self.log('vpn_dn - vpn not running')
            self.message('vpn not running')
        #
        # --fix-dns-auto-stop
        #
        self.log(' Stopping fix-dns-auto-start')
        self.message(' stopping dns auto fix')
        pargs = [self.cmd, '--fix-dns-auto-stop']
        id_num = self.runners.new_worker(self.complete, pargs)
        self.id_num_map[id_num] = 'stop auto fix dns'

    def ssh_start(self):
        ''' Start SSH '''
        ssh_running = is_ssh_running(self.log)
        if ssh_running :
            self.log('ssh_start ssh already running')
            self.message(' ssh aleady running')
        else :
            wg_running = is_wg_running(self.wg_iface)
            if wg_running:
                self.log('ssh_start - starting ssh listener')
                self.message('Starting ssh')
                pargs = [self.cmd, '--ssh-start']
                id_num = self.runners.new_worker(self.complete, pargs)
                self.id_num_map[id_num] = 'ssh start'
            else:
                self.log('ssh_start - vpn not running')
                self.message('vpn not running - cant run ssh')

    def ssh_stop(self):
        ''' Stop SSH '''
        ssh_running = is_ssh_running(self.log)
        if ssh_running :
            self.log('ssh_stop - stopping ssh listener')
            self.message('Stopping ssh')
            pargs = [self.cmd, '--ssh-stop']
            id_num = self.runners.new_worker(self.complete, pargs)
            self.id_num_map[id_num] = 'ssh stop'
        else :
            self.log('ssh_stop ssh not running')
            self.message('ssh not running')

    def complete(self, id_num):
        '''
        we only allow 1 thing to run at a time -
        we're either starting or stopping.
        '''
        which = self.id_num_map[id_num]
        self.log(f'{id_num} {which} : completed')
        self.message(f'{which} : completed')

    def quit(self):
        ''' Done '''
        self.log('Quit : Client GUI')
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
