"""
@file Pre_Build.py
@author IR
@brief This script preprocesses source files for use with Log_t
@version 0.3
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
"""

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

SOURCE_NAME = "src"
LIBRARIES_NAME = "libraries"
LIB_PATH = "libraries\\Log"  # Path to the implementation of Log
LIB_FILE = "LogConfig.def"
LIB_DEFINE = ("#define CONF_LOGGING_MAPPED_MODE 0", "#define CONF_LOGGING_MAPPED_MODE 1")
WORKING_DIRECTORY_OFFSET = "build\\Pre_Build\\"
# FILE_OUTPUT_PATH = "build\\bin"
FILE_OUTPUT_PATH = ""

BYPASS_SCRIPT = os.path.exists("script.disable") # bypass script if this file is found

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


class Text:
    @staticmethod
    def error(text):
        return "\033[91m\033[1m\033[4m" + text + "\033[0m"

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


# def updateMap(oMap, uMap):
#     keys = set(oMap.keys())
#     for e in uMap.items:
#         if e[0] in keys:  # Does the key already exist
#             if e[1] != oMap[e[0]]:  # If mismatched values, throw warning
#                 print("Warning, existing mapping has changed {} = {} >> {}".format(e[0], oMap[e[0]], e[1]))
#                 oMap[e[0]] = e[1]
#         else:  # If not, just update dict
#             oMap[e[0]] = e[1]


# async def logFile(FilePath, File_TAGs, File_IDs):
#     FILE_SEM.acquire()

#     updateMap(TAGs, File_TAGs)
#     updateMap(IDs, File_IDs)

#     Files.update({FilePath: {}})
#     Files[FilePath].update({"Hash": 1})
#     FILE_SEM.release()


FIND_CALL_REGEX_SS = r"Log(\.[diwef])?\s*\(\s*(\".*?\")\s*,\s*(\".*?\")\s*\)\s*;"  # -> Log("Str", "Str");
FIND_CALL_REGEX_VS = r"Log(\.[diwef])?\s*\(\s*([^\"]+?)\s*,\s*(\".*?\")\s*\)\s*;"  # -> Log(Var, "Str");
FIND_CALL_REGEX_SSV = r"Log(\.[diwef])?\s*\(\s*(\".*?\")\s*,\s*(\".*?\")\s*,\s*([^\"]+?)\s*\)\s*;"  # -> Log("Str", "Str", Var);
FIND_CALL_REGEX_VSV = r"Log(\.[diwef])?\s*\(\s*([^\"]+?)\s*,\s*(\".*?\")\s*,\s*([^\"]+?)\s*\)\s*;"  # -> Log(Var, "Str", Var);
FIND_CALL_REGEX_BAD = r"(Log(?:\.[diwef])?\s*\(\s*(?:[^\"]+?|\"(?:[^\"]|\\\")*?\")\s*,\s*)([^\";]+?)(\s*(?:,\s*(?:[^\"]+?))?\s*\)\s*;)"  # Implicit string or number where it should not be | IDE will warn about numbers but will still compile

FIND_TAG_DEF_REGEX_BAD = r"(LOG_TAG\s*[^\"=]+?=\s*)([^\"=]+?)(\s*;)"  # Implicit or single char definition of a tag type
FIND_TAG_DEF_REGEX_GOOD = r"LOG_TAG\s*[^\"=]+?=\s*(\".*?\")\s*;"

Log_Levels = {
    "": "[ LOG ] ",
    ".d": "[DEBUG] ",
    ".i": "[INFO]  ",
    ".w": "[WARN]  ",
    ".e": "[ERROR] ",
    ".f": "[FATAL] ",
}


class FileEntry:
    name = ""
    path = ""
    rawpath = ""
    workingPath = ""
    offset = ""
    modified = False
    error = None

    def __init__(self, RawPath, FilePath, FileName, Offset):
        if not os.path.exists(FilePath):
            raise FileNotFoundError(FilePath)

        self.name = FileName
        self.path = FilePath
        self.rawpath = RawPath
        self.workingPath = "{}{}".format(WORKING_DIRECTORY_OFFSET, FilePath)
        self.offset = Offset

        # if O_Files.get(self.workingPath):
        # print("Records show {} exists".format(FileName))

        touch("{}{}".format(WORKING_DIRECTORY_OFFSET, RawPath))
        shutil.copyfile(FilePath, self.workingPath)

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
        inplacePath = self.workingPath + ".__Lock"
        lineNo = 1
        try:
            with open(self.workingPath, "r") as f1, open(inplacePath, "w") as f2:
                for line in f1:
                    newline = await function(line)
                    f2.write(newline)

                    if newline != line:
                        self.modified = True
                    lineNo += 1

            shutil.copyfile(inplacePath, self.workingPath)
        except Exception as e:
            self.error = "{}{}".format(
                Text.warning("  {}:{}\n".format(self.path, lineNo)),
                "   {}\n    > {}".format(Text.red(type(e).__name__), splitErrorString(e)),
            )
        finally:
            os.remove(inplacePath)

    async def findLogMatch(self, line):
        newline = line
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


async def ingest_files(FilesEntries):
    for File in FilesEntries:
        await File.scan()


def run_ingest_files(*FilesEntries):
    asyncio.run(ingest_files(set(FilesEntries)))


Files = set()
FileRefs = set()
Threads = set()


def syncFile(filePath, offset, rawpath):
    workingFilePath = "{}{}".format(offset, filePath)
    new = hashFile(filePath)
    old = hashFile(workingFilePath)
    if not os.path.exists(workingFilePath) or new != old:
        touch("{}{}".format(offset, rawpath))
        shutil.copyfile(filePath, workingFilePath)
        print("Sync File: {} -> {}\n\tOld:{}\n\tNew:{}".format(filePath, offset, old, new))
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
                syncFile(filepath, Offset, rawpath)
                continue
            if rawpath.startswith(LIB_PATH):
                libFile = FileEntry(rawpath, filepath, filename, Offset)
                if libFile.name == LIB_FILE:
                    asyncio.run(libFile.walkLines(lib_flag))
                continue
            File_Ent = FileEntry(rawpath, filepath, filename, Offset)
            Files.add(File_Ent)
            FileRefs.add(File_Ent)


def dole_files(count):
    while True:
        file_set = set()

        i = 0

        while len(Files) != 0 and i != count:
            file_set.add(Files.pop())
            i += 1

        if len(file_set) != 0:
            Threads.add(threading.Thread(target=run_ingest_files, args=(file_set)))

        if len(Files) == 0:
            break


class Toolbar:
    toolbat_iter = 1
    toolbar_width = 25
    stop = False

    def __init__(self, num, prog_str):
        self.toolbat_iter = int(25 / num)

        # setup toolbar
        sys.stdout.write("{} [{}]".format(prog_str, " " * self.toolbar_width))
        sys.stdout.write("\b" * (self.toolbar_width + 1))
        sys.stdout.flush()

    def progress(self):
        if not self.stop:
            self.toolbar_width -= self.toolbat_iter
            if self.toolbar_width < self.toolbat_iter:
                self.toolbat_iter += self.toolbar_width
            sys.stdout.write("■" * self.toolbat_iter)
            sys.stdout.flush()
            if self.toolbar_width < self.toolbat_iter:
                self.stop = True
                sys.stdout.write("]\n")
                sys.stdout.flush()

    def finish(self):
        while not self.stop:
            self.progress()


def begin_scan():

    tb = Toolbar(len(Threads), Text.important("Completed Threads:"))

    for t in Threads:
        t.start()

    for t in Threads:
        t.join()
        tb.progress()

    tb.finish()  # do dead threads get gc?

    del IDs[""]
    del TAGs[""]

    print(Text.header("\nModified Files:"))
    for f in FileRefs:
        if f.modified:
            print("  {}".format(Text.green(f.name)))
        if f.error:
            Files.add(f)

    sys.stdout.flush()

    print(Text.header("\nFile Errors:"))
    for f in Files:
        print(f.error)


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

if not BYPASS_SCRIPT:
    try:  # TODO: Remove after implementing persistent data
        shutil.rmtree(SOURCE_DEST_NAME)
        shutil.rmtree(LIBRARIES_DEST_NAME)
    except FileNotFoundError:
        pass

touch(SOURCE_DEST_NAME)
touch(LIBRARIES_DEST_NAME)

allocate_files(SOURCE_NAME, WORKING_DIRECTORY_OFFSET)
allocate_files(LIBRARIES_NAME, WORKING_DIRECTORY_OFFSET)

if not BYPASS_SCRIPT:
    print(Text.warning("Available Ram: {} GBs\n".format(AvailableRam())))

    prehash = hashFile(getOutputFile(FILE_OUTPUT_PATH))

    print("Files to search: {}".format(len(FileRefs)))
    dole_files(int(math.log(len(FileRefs) + 1)) + 1)
    print("Threads to run: {}\n".format(len(Threads)))
    begin_scan()
    save_lookup(FILE_OUTPUT_PATH)
    newhash = hashFile(getOutputFile(FILE_OUTPUT_PATH))
    if newhash != prehash:
        print(Text.reallyImportant("\nNote: Output file values have changed"))

