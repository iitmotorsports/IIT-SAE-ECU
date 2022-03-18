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
    lock = threading.Lock()
    counter = 0
    Lines = set()
    run = True
    prefix = ""
    formatStr = "{} │{}│ {}{}\r"

    class TextIO(io.TextIOWrapper):
        """Text wrapper to capture stdout"""

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

        def write(self, s) -> None:
            self.func(s.strip("\n "))

        def get_original(self) -> io.TextIOWrapper:
            """Get the original Text Wrapper

            Returns:
                io.TextIOWrapper: Original TextIOWrapper
            """
            return super()

    def __init__(self, maxcount: int, prefix: str):
        """Create a new progress bar

        Args:
            maxcount (int): The target counter value
            prefix (str): The string to prefix the bar with
        """
        self.maxcount = maxcount
        self.stdout = sys.stdout
        self.rename(prefix)
        self.wrapper = ProgressBar.TextIO(
            self._new_line,
            sys.stdout.buffer,
            sys.stdout.encoding,
            sys.stdout.errors,
            sys.stdout.newlines,
            sys.stdout.line_buffering,
            sys.stdout.write_through,
        )
        self.printer = threading.Thread(target=self._print_thread)

    def rename(self, prefix: str) -> None:
        """Change the prefix of the progress bar

        Args:
            prefix (str): The string to prefix the bar with
        """
        mx_sz = len(self.formatStr.format(prefix, " " * self.bar_len, 100.0, "%"))
        self.bar_len = min(int(os.get_terminal_size().columns - 1 - (mx_sz / 1.25)), mx_sz)
        self.bar_len = self.bar_len if self.bar_len > 0 else 0
        self.prefix = prefix

    def reset(self, maxcount: int = None, prefix: str = None) -> None:
        """Reset the progress bar

        Args:
            maxcount (int): New target counter value. Defaults to current maxcount.
            prefix (str): New string to prefix the bar with. Defaults to current prefix.
        """
        self.maxcount = maxcount or self.maxcount
        if prefix:
            self.rename(prefix)
        with self.lock:
            self.counter = 0

    def _new_line(self, line: str) -> None:
        """The TextIO line handler

        Args:
            line (str): line to consume
        """
        self.Lines.add(line)

    def _progress(self, count, total, prefix="", printString: str = ""):
        if total > 0:
            count = min(count, total)
            filled_len = int(round(self.bar_len * count / float(total)))

            percents = round(100.0 * count / float(total), 1)
            bar_prog = "█" * filled_len + "░" * (self.bar_len - filled_len)

            pro_str = self.formatStr.format(prefix, bar_prog, percents, "%")
            if len(printString) > 0:
                self.stdout.write(" " * (os.get_terminal_size().columns - 1))
                self.stdout.write("\033[F")
                printString = printString.strip(" \n")
                spacer = " " * (os.get_terminal_size().columns - 1 - len(printString))
                self.stdout.write(f"{printString}{spacer}"[: os.get_terminal_size().columns - 1])
                self.stdout.write("\n")
            self.stdout.write(pro_str)
            self.stdout.flush()

    def _print_thread(self):
        while self.run or len(self.Lines) > 0:
            count = 0
            with self.lock:
                count = self.counter
            if len(self.Lines) > 0:
                self._progress(count, self.maxcount, self.prefix, self.Lines.pop())
            else:
                self._progress(count, self.maxcount, self.prefix)
            self.wrapper.flush()

    def start(self) -> None:
        """Enable this progress bar"""
        sys.stdout = self.wrapper
        self.printer.start()

    def progress(self):
        """Increment the progress bar"""
        with self.lock:
            self.counter += 1

    def finish(self):
        """Disable the progress bar, setting it to 100%"""
        self.run = False
        self.printer.join()
        self._progress(self.maxcount, self.maxcount, self.prefix)
        self.wrapper.flush()
        sys.stdout = self.wrapper.get_original()
        print()
