# SPDX-License-SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>
"""
File logger class
"""
# pylint: disable=too-few-public-methods
# pylint: disable=missing-function-docstring
# pylint: disable=invalid-name

import os
from pathlib import Path
import logging
from logging.handlers import RotatingFileHandler


def _get_logdir() -> str:
    """
    Construct the full log path
    """
    home = Path.home()
    logdir = os.path.join(home, 'log')
    os.makedirs(logdir, exist_ok=True)
    return logdir


def _setup_logger(name: str, logdir: str) -> logging.Logger:
    """
    Provision a file logger.
    """
    path = os.path.join(logdir, name)

    logger = logging.getLogger(name)
    logger.setLevel(logging.INFO)
    logger.propagate = False

    handler = RotatingFileHandler(
        path,
        maxBytes=5 * 1024 * 1024,
        backupCount=3
    )
    formatter = logging.Formatter('%(asctime)s : - %(message)s')
    handler.setFormatter(formatter)
    logger.addHandler(handler)

    return logger


class _Log:
    """
    Private log handler cLog / gLog
    """
    def __init__(self, name: str):
        self.logdir_path: str = _get_logdir()
        self.logger: logging.Logger = _setup_logger(name, self.logdir_path)

    def msg(self, txt: str):
        self.logger.info(txt)

    def logdir(self) -> str:
        return self.logdir_path


class cLog:
    """
    command line Log handler
    """
    _log: _Log
    _init: bool = False

    @staticmethod
    def initialize():
        if not cLog._init:
            cLog._log = _Log('wg-client-cmd')
            cLog._init = True

    @staticmethod
    def msg(txt: str):
        if cLog._init:
            cLog._log.msg(txt)

    @staticmethod
    def logdir() -> str:
        if cLog._init:
            return cLog._log.logdir()
        return ''


class gLog:
    """
    gui Log handler
    """
    _log: _Log
    _init: bool = False

    @staticmethod
    def initialize():
        if not gLog._init:
            gLog._log = _Log('wg-client-gui')
            gLog._init = True

    @staticmethod
    def msg(txt: str):
        if gLog._init:
            gLog._log.msg(txt)

    @staticmethod
    def logdir() -> str:
        if gLog._init:
            return gLog._log.logdir()
        return ''
