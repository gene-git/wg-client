# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Gene C
"""
File logger class
"""
# pylint: disable=too-few-public-methods
import os
from pathlib import Path
import logging
from logging.handlers import RotatingFileHandler

class MyLog:
    """
    Use logger to save output
     - "~/log/wg-client"
     - hopefully thread safe
    """
    def __init__(self, logname):
        home = Path.home()

        log_dir = os.path.join(home, 'log')
        os.makedirs(log_dir, exist_ok=True)
        self.log_path = os.path.join(log_dir, logname)

        log_fmt = '%(asctime)s %(message)s'
        dt_fmt = '%Y-%m-%d %H:%M:%S '
        formatter = logging.Formatter(fmt=log_fmt, datefmt=dt_fmt)      #, style='%')

        self.logger = logging.getLogger(logname)
        self.logger.setLevel(logging.INFO)

        handler = RotatingFileHandler(self.log_path, maxBytes=10240, backupCount=1)
        handler.setFormatter(formatter)
        self.logger.addHandler(handler)

    def log(self, msg):
        """
        write log
        """
        self.logger.info(msg)

    def logfile(self):
        """ where the log is found """
        return self.log_path
