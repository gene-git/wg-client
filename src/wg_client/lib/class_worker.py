# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: © 2023-present  Gene C <arch@sapience.com>
"""
THread support via pyqt6 QRunnable
"""
# pylint: disable=no-name-in-module,too-few-public-methods
from PyQt6.QtCore import QRunnable, QObject, QThreadPool, pyqtSignal, pyqtSlot
from .class_proc import MyProc

class MyQsignals(QObject):
    '''
    Track start and completions
      - leave start commented in case we want to turn on
      - we currently only track when worker thread is completed
    '''
    #started = pyqtSignal(int)
    completed = pyqtSignal(int)


class Worker(QRunnable):
    """
    Worker thread
    Inherits from QRunnable to handler worker thread setup, signals and wrap-up.

    :param callback: The function callback to run on this worker thread. Supplied args and
                     kwargs will be passed through to the runner.
    :type callback: function
    :param args: Arguments to pass to the callback function
    :param kwargs: Keywords to pass to the callback function
    """
    def __init__(self, id_num, complete_func, func, *args, **kwargs):
        super().__init__()

        # Save constructor arguments
        self.setAutoDelete(True)

        self.func = func
        self.args = args
        self.kwargs = kwargs

        self.id_num = id_num
        self.signals = MyQsignals()
        self.is_killed = False
        # set the callback func when complete
        self.signals.completed.connect(complete_func)

    @pyqtSlot()
    def run(self):
        '''
        Run the supplied function with passed args, kwargs.
        Send completion signal when done - arg is (int)
        '''
        self.func(*self.args, **self.kwargs)
        self.signals.completed.emit(self.id_num)

    def dir(self):
        '''
        Stop this worker
        '''
        self.is_killed = True


def _next_id(runners):
    """
    Get next available id
     - for now we just increment and dont worry about
        wrapping or reusing
    """
    runners.id_last += 1
    return runners.id_last

class MyRunners:
    """
    Convenience wrapper around QRunner to enable multi-threaded workers
    """
    def __init__(self, mylog, mysignals):
        #self.threadpool = QThreadPool()
        self.threadpool = QThreadPool.globalInstance()
        self.id_last = 0
        self.ids_used = []
        self.workers = {}
        self.complete_func = {}
        self.log = mylog
        self.mysignals = mysignals

    def new_worker(self, complete_func, pargs):
        """
        Return the worker id.
        """
        id_num = _next_id(self)
        self.ids_used.append(id_num)

        self.complete_func[id_num] = complete_func
        worker = Worker(id_num, self.complete, self.run_cmd, pargs)
        self.workers[id_num] = worker
        self.threadpool.start(worker)
        return id_num

    def end(self, id_num):
        """
        End this worker
        """
        if id_num in self.ids_used:
            self.ids_used.remove(id_num)

        if self.workers[id_num]:
            self.workers[id_num].die()
            self.workers.pop(id_num, None)

    def quit(self):
        """ 
        kill off any remaining workers
        use separate copy of id list as workers 
        and hence the list of keys is changed by end()
        """
        workers = self.workers.copy()
        id_nums = workers.keys()
        for id_num in id_nums:
            self.end(id_num)


    def run_cmd(self, pargs):
        """
        Actually run it
        """
        myproc = MyProc(self.mysignals)
        myproc.popen(pargs,logger=self.log)

    def complete(self, id_num):
        """
        track completions and call users complete func
        """
        if not id_num in self.ids_used:
            # should never happen
            self.log(f'Err: id {id_num} not in used list')

        # remove from used list, and and call the users complete func
        self.ids_used.remove(id_num)
        if id_num in self.complete_func:
            self.complete_func[id_num](id_num)
            self.complete_func.pop(id_num)
        else:
            self.log(f'Err: id {id_num} not in complete func list')
