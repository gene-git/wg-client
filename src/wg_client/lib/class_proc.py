# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Gene C
"""
Support
"""
# pylint: disable=consider-using-with
import os
import signal
import subprocess
from subprocess import PIPE

def run_prog(pargs,input_str=None,stdout=PIPE, stderr=PIPE):
    """
    run external program using subprocess
    """
    if not pargs:
        return [0, None, None]

    bstring = None
    if input_str:
        bstring = bytearray(input_str,'utf-8')

    ret = subprocess.run(pargs, input=bstring, stdout=stdout, stderr=stderr, check=False)
    retc = ret.returncode
    output = None
    errors = None
    if ret.stdout :
        output = str(ret.stdout, 'utf-8', errors='ignore')
    if ret.stderr :
        errors = str(ret.stderr, 'utf-8', errors='ignore')
    return [retc, output, errors]

def is_pid_valid(pid):
    """ Check For the existence of a unix pid. """
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    return True

def kill_process(pid):
    """
    kill process starting gently
    """
    if is_pid_valid(pid):
        os.kill(pid, signal.Signals.SIGHUP)
        if is_pid_valid(pid):
            os.kill(pid, signal.Signals.SIGTERM)
            if is_pid_valid(pid):
                os.kill(pid, signal.Signals.SIGKILL)

def signal_catcher(sighandler):
    """
     Catch signals and install sighandler(sig, frame)
     Can get list via [x for x in dir(signal) if x.startswith("SIG")]
     NB cannot catch SIGKILL SIGSTOP
    """
    sigs = ['SIGABRT', 'SIGALRM', 'SIGBUS', 'SIGHUP',  'SIGINT', 'SIGQUIT', 'SIGTERM']
    for sig in sigs:
        try:
            signum = getattr(signal, sig)
            signal.signal(signum, sighandler)
        except OSError :
            # skip any that cannot be caught in case user messes up above :)
            #print (f'Skipping {sig}')
            pass

class MySignals:
    """
    Handle signals once and share with each process handler
    Keep list of registered procs
    """
    def __init__(self):
        self.procs = []

        # install signal handler
        signal_catcher(self.signal_handler)

    def signal_handler(self, _signum, _frame):
        """ kill child proc """
        for proc in self.procs:
            if not proc.returncode:
                proc.terminate()
                proc.kill()
        # not needed as proc.Kill() should be final
        #if self.proc_pid:
        #    kill_process(self.proc_pid)

    def add_proc(self, proc):
        """ add proc to hadnler """
        if proc not in self.procs:
            self.procs.append(proc)

    def remove_proc(self, proc):
        """ delete proc from hadnler """
        if proc in self.procs:
            self.procs.remove(proc)

class MyProc:
    """
    Handle popen()
     - only have one MyProc() to have one signal handler
     - takes the instance of MySignals
    """
    def __init__(self, mysignals):
        self.proc = None
        self.mysignals = mysignals

    def popen(self, pargs, logger=None, pid_saver=None):
        """
        run using subprocess.Popen()
        """
        log = print
        if logger:
            log = logger
        outs = None
        errs = None
        ret = None
        try:
            self.proc = subprocess.Popen(pargs, text=True, stdout=PIPE, stderr=PIPE)
            if not self.proc:
                return (ret, outs, errs)

            self.mysignals.add_proc(self.proc)

            # pid if requested
            if pid_saver:
                pid_saver(self.proc.pid)

            # flush any output
            #while True:
            try:
                outs, errs = self.proc.communicate()
                if outs:
                    log(outs)
                if errs:
                    log(errs)

            except OSError:
                if not self.proc.returncode:
                    self.proc.terminate()
                    self.proc.kill()

        except OSError as err:
            cmd = ' '.join(pargs)
            log(f'Error starting {cmd} : {err}')

        if self.proc:
            self.mysignals.remove_proc(self.proc)
            ret = self.proc.returncode
            # when killed ret = 255 (= 128 + signal-127)
            #if ret != 0:
            #    cmd = ' '.join(pargs)
            #    print(f'Err: failed to run {cmd} : {ret}')
        # clear the child process pid
        if pid_saver:
            pid_saver('-1')
        return (ret, outs, errs)

    def run(self, pargs):
        """
        uses subprocess.run()
        """
        [retc, output, errors] = run_prog(pargs)
        return [retc, output, errors]
