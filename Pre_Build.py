"""
@file Pre_Build.py
@author IR
@brief This script preprocesses source files for use with Log_t
@version 0.1
@date 2020-11-11

@copyright Copyright (c) 2020

This script works by first duplicating source files to the build folder. \n
Then it scans each file for calls to a Log function and modifies them as follows. \n

If the call has a string for `LOG_TAG` parameter, give that string a unique integer ID and replace it with that integer. \n
If the ID is not a string, leave the variable alone. \n

Replace the call's `LOG_MSG` string with a unique ID as well. \n
NOTE: The `LOG_MSG` parameter must always be an inline string. \n

`LOG_TAG`s and `LOG_MSG`s do not share IDs. \n

Eg.
```
Log(ID, "My Message"); -> Log(ID, 1);
Log("My Str ID", "My Message"); -> Log(1, 1);
```
    
Calls to Log functions also have the option to send a number with a third parameter. \n

Eg.
```
Log("My Str ID", "My Message", 56); -> Log(1, 1, 56);
Log(ID, "My Message", A_Num_Var); -> Log(ID, 1, A_Num_Var);
```
    
Declarations of `LOG_TAG` also have their strings replaced with a unique ID. \n
NOTE: Definition of `LOG_TAG`s must always be an inline string. \n

Eg.
```
LOG_TAG TAG = "Logging Tag"; -> LOG_TAG TAG = 2;
```

A special case has been made to also allocate and replace string that call the following macro

```
_LogPrebuildString(x)
```

Where x is the string, it will be given a unique ID and replaced with said ID as if it were being called by a Logging function.
This is useful where one wishes to generate log functions using the C preprocessor.
Note, however, if this script is not run the macro should still allow everything to compile normally, leaving the string untouched

"""

# @cond

#TODO: Keep same lookup table when compiling both back and front teensy

import fileinput
import shutil
import hashlib
import os
import ctypes
import asyncio
import threading
import time
import errno
import pprint
import weakref
import gc
import re
import sys
import json
import pickle
from pathlib import Path
import math
import io
from typing import List, BinaryIO, TextIO, Iterator, Union, Optional, Callable, Tuple, Type, Any, IO, Iterable
import random
import multiprocessing

SOURCE_NAME = "src"
LIBRARIES_NAME = "libraries"
LIB_PATH = "libraries\\Log"  # Path to the implementation of Log
LIB_FILE = "LogConfig.def"
LIB_DEFINE = ("#define CONF_LOGGING_MAPPED_MODE 0", "#define CONF_LOGGING_MAPPED_MODE 1")
WORKING_DIRECTORY_OFFSET = "build\\Pre_Build\\"
# FILE_OUTPUT_PATH = "build\\bin"
FILE_OUTPUT_PATH = ""

BYPASS_SCRIPT = os.path.exists("script.disable")  # bypass script if this file is found

LIMIT_TAG = 254
LIMIT_ID = 65535
BLACKLIST_ADDRESSESS = (0, 5, 9)

SOURCE_DEST_NAME = "{}{}".format(WORKING_DIRECTORY_OFFSET, SOURCE_NAME)
LIBRARIES_DEST_NAME = "{}{}".format(WORKING_DIRECTORY_OFFSET, LIBRARIES_NAME)
DATA_FILE = "{}.LogInfo".format(WORKING_DIRECTORY_OFFSET)

# PLACEHOLDER_TAG = "__PYTHON__TAG__PLACEHOLDER__{}__"
# PLACEHOLDER_ID = "__PYTHON__ID__PLACEHOLDER__{}__"


def AvailableRam():
    kernel32 = ctypes.windll.kernel32
    c_ulong = ctypes.c_ulong

    class MEMORYSTATUS(ctypes.Structure):
        _fields_ = [
            ("dwLength", c_ulong),
            ("dwMemoryLoad", c_ulong),
            ("dwTotalPhys", c_ulong),
            ("dwAvailPhys", c_ulong),
            ("dwTotalPageFile", c_ulong),
            ("dwAvailPageFile", c_ulong),
            ("dwTotalVirtual", c_ulong),
            ("dwAvailVirtual", c_ulong),
        ]

    memoryStatus = MEMORYSTATUS()
    memoryStatus.dwLength = ctypes.sizeof(MEMORYSTATUS)
    kernel32.GlobalMemoryStatus(ctypes.byref(memoryStatus))
    return int((((1 - memoryStatus.dwMemoryLoad / 100) * memoryStatus.dwAvailPhys) * 10) / 1073741824)


LOW_RAM = 4
BUF_SIZE = 65536


def hashFile(filePath):
    if os.path.exists(filePath):
        if AvailableRam() <= LOW_RAM:
            sha256 = hashlib.sha256()
            with open(filePath, "rb") as f:
                while True:
                    data = f.read(BUF_SIZE)
                    if not data:
                        break
                    sha256.update(data)
            return sha256.digest()
        else:
            with open(filePath, "rb") as f:
                return hashlib.sha256(f.read()).hexdigest()
    return ""


class Text:
    @staticmethod
    def error(text):
        return "\033[91m\033[1m\033[4m" + text + "\033[0m"

    @staticmethod
    def underline(text):
        return "\033[4m" + text + "\033[0m"

    @staticmethod
    def header(text):
        return "\033[1m\033[4m" + text + "\033[0m"

    @staticmethod
    def warning(text):
        return "\033[93m\033[1m" + text + "\033[0m"

    @staticmethod
    def important(text):
        return "\033[94m\033[1m" + text + "\033[0m"

    @staticmethod
    def reallyImportant(text):
        return "\033[94m\033[1m\033[4m" + text + "\033[0m"

    @staticmethod
    def green(text):
        return "\033[92m" + text + "\033[0m"

    @staticmethod
    def red(text):
        return "\033[91m" + text + "\033[0m"


def save_data(Object):
    with open(DATA_FILE, "wb") as f:
        pickle.dump(Object, f)


def load_data():
    # if os.path.exists(DATA_FILE):
    #     with open(DATA_FILE, "rb") as f:
    #         return pickle.load(f)
    # print("No {} found".format(DATA_FILE))
    return ({}, {}, {})


def touch(rawpath):
    try:
        Path(rawpath).mkdir(parents=True, exist_ok=True)
    except OSError as exc:
        if exc.errno != errno.EEXIST:
            raise


class ScriptException(Exception):
    pass


class OutOfIDsException(ScriptException):
    def __init__(self, message):
        super().__init__(message.strip(), "Script has ran out of allocatable IDs")


class OutOfTAGsException(ScriptException):
    def __init__(self, message):
        super().__init__(message.strip(), "Script has ran out of allocatable TAG IDs")


class MalformedTAGDefinitionException(ScriptException):
    def __init__(self, message):
        super().__init__(message.strip(), "Implicit, single char or number definition of a LOG_TAG type")


class MalformedLogCallException(ScriptException):
    def __init__(self, message):
        super().__init__(message.strip(), "Implicit string or number inside a call to Log")


def splitErrorString(error):
    if issubclass(type(error), ScriptException):
        return error.args[1] + "\n\t" + error.args[0]
    else:
        return error


OLD_DATA = load_data()  # TBI

O_TAGs = OLD_DATA[0]
O_IDs = OLD_DATA[1]
O_Files = OLD_DATA[2]

TAGs = {}
IDs = {}
Files = {}

TAG_SEM = threading.BoundedSemaphore(1)
ID_SEM = threading.BoundedSemaphore(1)
FILE_SEM = threading.BoundedSemaphore(1)


# IMPROVE: Find a better way to get unique numbers


async def getUniqueID(findDuplicate=None):
    if IDs.get(findDuplicate):
        return IDs[findDuplicate]

    ID_SEM.acquire()
    old_vals = set(O_IDs.values())
    vals = set(IDs.values())

    for i in range(1, LIMIT_ID):
        if i not in old_vals and i not in vals:
            IDs[""] = i  # Claim before returning
            ID_SEM.release()
            return i

    # Attempt to clear up some IDs
    raise OutOfIDsException


async def getUniqueTAG(findDuplicate=None):
    if TAGs.get(findDuplicate):
        return TAGs[findDuplicate]

    TAG_SEM.acquire()
    old_vals = set(O_TAGs.values())
    vals = set(TAGs.values())

    for i in range(1, LIMIT_TAG):
        if i not in BLACKLIST_ADDRESSESS and i not in old_vals and i not in vals:
            TAGs[""] = i  # Claim before returning
            TAG_SEM.release()
            return i

    # Attempt to clear up some IDs
    raise OutOfIDsException

FIND_SPECIAL_REGEX = r"_LogPrebuildString\s*\(\s*(\".*?\")\s*\)" # -> _LogPrebuildString("Str") # Special case where we can indirectly allocate a string
FIND_CALL_REGEX_SS = r"Log(\.[diwef])?\s*\(\s*(\".*?\")\s*,\s*(\".*?\")\s*\)\s*;"  # -> Log("Str", "Str");
FIND_CALL_REGEX_VS = r"Log(\.[diwef])?\s*\(\s*([^\"]+?)\s*,\s*(\".*?\")\s*\)\s*;"  # -> Log(Var, "Str");
FIND_CALL_REGEX_SSV = r"Log(\.[diwef])?\s*\(\s*(\".*?\")\s*,\s*(\".*?\")\s*,\s*([^\"]+?)\s*\)\s*;"  # -> Log("Str", "Str", Var);
FIND_CALL_REGEX_VSV = r"Log(\.[diwef])?\s*\(\s*([^\"]+?)\s*,\s*(\".*?\")\s*,\s*([^\"]+?)\s*\)\s*;"  # -> Log(Var, "Str", Var);
FIND_CALL_REGEX_BAD = r"(Log(?:\.[diwef])?\s*\(\s*(?:[^\"]+?|\"(?:[^\"]|\\\")*?\")\s*,\s*)([^\";]+?)(\s*(?:,\s*(?:[^\"]+?))?\s*\)\s*;)"  # Implicit string or number where it should not be | IDE will warn about numbers but will still compile

FIND_TAG_DEF_REGEX_BAD = r"(LOG_TAG(?= )\s*[^\"=]+?=\s*)([^\"=]+?)(\s*;)"  # Implicit or single char definition of a tag type
FIND_TAG_DEF_REGEX_GOOD = r"LOG_TAG(?= )\s*[^\"=]+?=\s*(\".*?\")\s*;"

Log_Levels = {
    "": "[ LOG ] ",
    ".d": "[DEBUG] ",
    ".i": "[INFO]  ",
    ".w": "[WARN]  ",
    ".e": "[ERROR] ",
    ".f": "[FATAL] ",
}


class FileEntry:  # IMPROVE: Make IDs persistent
    name = ""
    path = ""
    rawpath = ""
    workingPath = ""
    offset = ""
    modified = False
    error = ""

    def __init__(self, RawPath, FilePath, FileName, Offset):
        if not os.path.exists(FilePath):
            raise FileNotFoundError(FilePath)

        self.name = FileName
        self.path = FilePath
        self.rawpath = RawPath
        self.workingPath = "{}{}".format(Offset, FilePath)
        self.offset = Offset

        # if O_Files.get(self.workingPath):
        # print("Records show {} exists".format(FileName))

        touch("{}{}".format(Offset, RawPath))

    async def addNewTag(self, raw_str):
        string = "[{}]".format(raw_str.strip('"'))
        numberID = await getUniqueTAG(string)
        TAG_SEM.acquire()
        TAGs[string] = numberID
        TAG_SEM.release()
        return numberID

    async def addNewID(self, raw_log_level, raw_str):
        string = Log_Levels[raw_log_level] + raw_str.strip('"')
        numberID = await getUniqueID(string)
        ID_SEM.acquire()
        IDs[string] = numberID
        ID_SEM.release()
        return numberID

    async def SPECIAL_STR(self, line, reMatch):
        ID = await self.addNewID("", reMatch) # Special strings are always LOG for simplicity
        return line.replace(reMatch, str(ID))

    async def VSX(self, line, reMatch):
        ID = await self.addNewID(reMatch[0], reMatch[2])
        return line.replace(reMatch[2], str(ID))

    async def SSX(self, line, reMatch):
        TAG = await self.addNewTag(reMatch[2])
        ID = await self.addNewID(reMatch[0], reMatch[2])
        return line.replace(reMatch[1], str(TAG)).replace(reMatch[2], str(ID))

    async def NEW_TAG(self, line, reMatch):
        TAG = await self.addNewTag(reMatch)
        return line.replace(reMatch, str(TAG))

    async def walkLines(self, function):
        tempPath = self.workingPath + ".__Lock"
        lineNo = 1
        synced = False
        newline = ""
        with open(self.path, "r", encoding="utf-8") as f1, open(tempPath, "w", encoding="utf-8") as f2:
            for line in f1:
                try:
                    newline = await function(line)
                    f2.buffer.write(newline.encode("utf-8"))
                except Exception as e: # If prev exception was about IO then oh well
                    self.error = "{}{}{}\n".format(
                        self.error,
                        Text.warning("  {}:{}\n".format(self.path, lineNo)),
                        "   {}\n    > {}".format(Text.red(type(e).__name__), splitErrorString(e)),
                    )
                    f2.buffer.write(line.encode("utf-8"))
                finally:
                    lineNo += 1
        self.modified = not syncFile(tempPath, "", self.rawpath, self.workingPath)
        os.remove(tempPath)

    async def findLogMatch(self, line):
        newline = line

        SPECIAL = re.findall(FIND_SPECIAL_REGEX, line)
        if SPECIAL:
            newline = await self.SPECIAL_STR(line, SPECIAL[0])
        else:
            VS = re.findall(FIND_CALL_REGEX_VS, line)
            if len(VS) != 0:  # ¯\_(ツ)_/¯
                newline = await self.VSX(line, VS[0])
            else:
                TAG_GOOD = re.findall(FIND_TAG_DEF_REGEX_GOOD, line)
                if TAG_GOOD:
                    newline = await self.NEW_TAG(line, TAG_GOOD[0])
                else:
                    VSV = re.findall(FIND_CALL_REGEX_VSV, line)
                    if len(VSV) != 0:
                        newline = await self.VSX(line, VSV[0])
                    else:
                        TAG_BAD = re.findall(FIND_TAG_DEF_REGEX_BAD, line)
                        if TAG_BAD:
                            TAG_BAD = TAG_BAD[0]
                            raise MalformedTAGDefinitionException(TAG_BAD[0] + Text.error(TAG_BAD[1]) + TAG_BAD[2])
                        else:
                            BAD = re.findall(FIND_CALL_REGEX_BAD, line)
                            if BAD:
                                BAD = BAD[0]
                                raise MalformedLogCallException(BAD[0] + Text.error(BAD[1]) + BAD[2])
                            else:
                                SS = re.findall(FIND_CALL_REGEX_SS, line)
                                if len(SS) != 0:
                                    newline = await self.SSX(line, SS[0])
                                else:
                                    SSV = re.findall(FIND_CALL_REGEX_SSV, line)
                                    if len(SSV) != 0:
                                        newline = await self.SSX(line, SSV[0])
        return newline

    async def scan(self):
        await self.walkLines(self.findLogMatch)


async def ingest_files(finishFunc, FilesEntries):
    for File in FilesEntries[0]:
        finishFunc()
        await File.scan()


def run_ingest_files(finishFunc, *FilesEntries):
    asyncio.run(ingest_files(finishFunc, FilesEntries))


Files = set()
FileRefs = set()
Threads = set()

FILE_CHANGE = False


def syncFile(filePath, offset, rawpath, workingFilePath=None, suppress=False):
    workingFilePath = workingFilePath or "{}{}".format(offset, filePath)
    global FILE_CHANGE

    new = hashFile(filePath)
    old = hashFile(workingFilePath)
    if old == "":
        old = 0
    FILE_CHANGE = FILE_CHANGE and (new == old)

    if not os.path.exists(workingFilePath) or new != old:
        touch("{}{}".format(offset, rawpath))
        shutil.copyfile(filePath, workingFilePath)
        if not suppress:
            print("Sync File: {} -> {}".format(filePath, offset))
        return False
    return True


def allocate_files(Path, Offset):
    async def lib_flag(line):
        return line.replace(LIB_DEFINE[0], LIB_DEFINE[1])

    for subdir, _, files in os.walk(Path):
        for filename in files:
            filepath = subdir + os.sep + filename
            rawpath = subdir + os.sep
            if BYPASS_SCRIPT:
                syncFile(filepath, Offset, rawpath, suppress=True)
                continue
            if rawpath.startswith(LIB_PATH):
                libFile = FileEntry(rawpath, filepath, filename, Offset)
                if libFile.name == LIB_FILE:
                    asyncio.run(libFile.walkLines(lib_flag))
                    continue
            File_Ent = FileEntry(rawpath, filepath, filename, Offset)
            Files.add(File_Ent)
            FileRefs.add(File_Ent)


def dole_files(count, finishFunc):
    while True:
        file_set = set()

        i = 0

        while len(Files) != 0 and i != count:
            file_set.add(Files.pop())
            i += 1

        if len(file_set) != 0:  # IMPROVE: Use actual mutlithreading
            # Threads.add(multiprocessing.Process(target=function, args=(file_set)))
            Threads.add(threading.Thread(target=run_ingest_files, args=(finishFunc, file_set)))

        if len(Files) == 0:
            break


class ThreadedProgressBar:
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
            super(ThreadedProgressBar.TextIO, self).__init__(buffer, encoding, errors, newline, line_buffering, write_through)
            self.func = func

        def write(self, s):
            self.func(s.strip("\n "))

        def getOriginal(self):
            return super()

    def __init__(self, maxcount, prefix):
        self.maxcount = maxcount
        self.stdout = sys.stdout
        self.wrapper = ThreadedProgressBar.TextIO(
            self._newLine,
            sys.stdout.buffer,
            sys.stdout.encoding,
            sys.stdout.errors,
            sys.stdout.newlines,
            sys.stdout.line_buffering,
            sys.stdout.write_through,
        )
        sys.stdout = self.wrapper
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

    def _progress(self, count, total, prefix="", printString=""):
        if total > 0:
            filled_len = int(round(self.bar_len * count / float(total)))

            percents = round(100.0 * count / float(total), 1)
            bar = "█" * filled_len + "░" * (self.bar_len - filled_len)

            proStr = self.formatStr.format(prefix, bar, percents, "%")
            if len(printString) > 0:
                self.stdout.write(" " * (os.get_terminal_size().columns - 1))
                self.stdout.write("\r")
                self.stdout.write(printString)
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
        self.printer = threading.Thread(target=self._printThread)
        self.printer.start()

    def progress(self):
        self.counter += 1

    def finish(self):
        print("\0")  # Eh
        self.run = False
        self.printer.join()
        self._progress(self.counter, self.maxcount, self.prefix)
        self.wrapper.flush()
        sys.stdout = self.wrapper.getOriginal()
        print()


def begin_scan():
    for t in Threads:
        t.start()

    for t in Threads:
        t.join()

    try:
        del IDs[""]
    except KeyError:
        pass

    try:
        del TAGs[""]
    except KeyError:
        pass


def printResults():
    maxPrinted = 8
    print(Text.header("\nModified Files:"))
    c = 0
    m = 0
    for f in FileRefs:
        if f.modified:
            if c < maxPrinted:
                print("  {}".format(Text.green(f.name)))
                c += 1
            else:
                m += 1
        if f.error != "":
            Files.add(f)
    if m > 1:
        print("  {}".format(Text.underline(Text.green("{} more file{}".format(m, "s" if m > 1 else "")))))

    sys.stdout.flush()

    c = 0
    m = 0
    print(Text.header("\nFile Errors:"))
    for f in Files:
        if c < maxPrinted:
            print(f.error.strip("\n"))
            c += 1
        else:
            m += 1

    if m > 1:
        print("  {}".format(Text.underline(Text.red("{} more error{}".format(m, "s" if m > 1 else "")))))


def getOutputFile(path):
    output_name = "log_lookup.json"
    savePath = "{}\\{}".format(path, output_name)
    if savePath.startswith("\\"):
        savePath = output_name
    return savePath


def save_lookup(path):
    toSave = (TAGs, IDs)
    touch(path)
    with open(getOutputFile(path), "w") as f:
        json.dump(toSave, f, indent=4, separators=(",", ": "))


# Start Script

if __name__ == "__main__":
    touch(SOURCE_DEST_NAME)
    touch(LIBRARIES_DEST_NAME)

    allocate_files(SOURCE_NAME, WORKING_DIRECTORY_OFFSET)
    allocate_files(LIBRARIES_NAME, WORKING_DIRECTORY_OFFSET)

    if not BYPASS_SCRIPT:
        print()

        time.sleep(0.5)  # Let terminal settle

        print(Text.warning("Available Ram: {} GBs\n".format(AvailableRam())))

        prehash = hashFile(getOutputFile(FILE_OUTPUT_PATH))

        print("Files to search: {}".format(len(FileRefs)))

        tb = ThreadedProgressBar(len(Files), Text.important("Completed Files:"))

        dole_files(8, tb.progress)

        print("Threads to run: {}\n".format(len(Threads)))

        tb.start()
        begin_scan()
        tb.finish()

        printResults()
        save_lookup(FILE_OUTPUT_PATH)
        newhash = hashFile(getOutputFile(FILE_OUTPUT_PATH))
        if FILE_CHANGE:
            print(Text.important("\nNote: Files have changed, rebuild inbound"))
        if newhash != prehash:
            print(Text.reallyImportant("\nNote: Output file values have changed"))

    # try:
    #     shutil.rmtree(SOURCE_DEST_NAME)
    #     shutil.rmtree(LIBRARIES_DEST_NAME)
    # except FileNotFoundError:
    #     pass

# @endcond

