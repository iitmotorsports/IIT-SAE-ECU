"""Progress bar"""

import os
import threading
import sys
import io
from typing import Optional, IO


class ProgressBar:
    """Progress bar that can be updated concurrently"""
    bar_len = 10
    maxcount = 0
    counter = 0
    Lines = set()
    run = True
    prefix = ""
    formatStr = "{} │{}│ {}{}\r"

    class TextIO(io.TextIOWrapper):
        def __init__(
            self,
            func,
            buffer: IO[bytes],
            encoding: str = ...,
            errors: Optional[str] = ...,
            newline: Optional[str] = ...,
            line_buffering: bool = ...,
            write_through: bool = ...,
        ):
            super(ProgressBar.TextIO, self).__init__(buffer, encoding, errors, newline, line_buffering, write_through)
            self.func = func

        def write(self, s):
            self.func(s.strip("\n "))

        def getOriginal(self):
            return super()

    def __init__(self, maxcount, prefix):
        self.maxcount = maxcount
        self.stdout = sys.stdout
        self.rename(prefix)

    def rename(self, prefix):
        mx_sz = len(self.formatStr.format(prefix, " " * self.bar_len, 100.0, "%"))
        self.bar_len = min(int(os.get_terminal_size().columns - 1 - (mx_sz / 1.25)), mx_sz)
        self.bar_len = self.bar_len if self.bar_len > 0 else 0
        self.prefix = prefix

    def reset(self, maxcount, prefix):
        self.maxcount = maxcount
        self.rename(prefix)
        self.counter = 0

    def _newLine(self, String):
        self.Lines.add(String)

    def _progress(self, count, total, prefix="", printString: str = ""):
        if total > 0:
            filled_len = int(round(self.bar_len * count / float(total)))

            percents = round(100.0 * count / float(total), 1)
            bar = "█" * filled_len + "░" * (self.bar_len - filled_len)

            proStr = self.formatStr.format(prefix, bar, percents, "%")
            if len(printString) > 0:
                self.stdout.write(" " * (os.get_terminal_size().columns - 1))
                self.stdout.write("\033[F")
                printString = printString.strip(" \n")
                spacer = " " * (os.get_terminal_size().columns - 1 - len(printString))
                self.stdout.write(f"{printString}{spacer}"[: os.get_terminal_size().columns - 1])
                self.stdout.write("\n")
            self.stdout.write(proStr)
            self.stdout.flush()

    def _printThread(self):
        while self.run or len(self.Lines) > 0:
            if len(self.Lines) > 0:
                self._progress(self.counter, self.maxcount, self.prefix, self.Lines.pop())
            else:
                self._progress(self.counter, self.maxcount, self.prefix)

    def start(self):
        self.wrapper = ProgressBar.TextIO(
            self._newLine,
            sys.stdout.buffer,
            sys.stdout.encoding,
            sys.stdout.errors,
            sys.stdout.newlines,
            sys.stdout.line_buffering,
            sys.stdout.write_through,
        )
        sys.stdout = self.wrapper
        self.printer = threading.Thread(target=self._printThread)
        self.printer.start()

    def progress(self):
        self.counter += 1

    def finish(self):
        self.run = False
        self.printer.join()
        self._progress(self.counter, self.maxcount, self.prefix)
        self.wrapper.flush()
        sys.stdout = self.wrapper.getOriginal()
        print()
