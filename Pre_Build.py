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

import pathlib
import shutil
import hashlib
import glob
import os
import asyncio
import threading
import time
import errno
import re
import sys
import json
import pickle
from pathlib import Path
import io
from typing import Optional, IO
import subprocess
from os.path import join as join_path
from vs_conf import load_json
from vs_conf import Settings

SOURCE_NAME = "src"
LIBRARIES_NAME = "libraries"
LIB_PATH = join_path("libraries", "Log")  # Path to the implementation of Log
LIB_FILE = "LogConfig.def"
LIB_DEFINE = ("#define CONF_LOGGING_MAPPED_MODE 0", "#define CONF_LOGGING_MAPPED_MODE 1")
BLACKLIST = join_path("libraries", ".blacklist")
WORKING_DIRECTORY_OFFSET = join_path("build", "Pre_Build", "")
# FILE_OUTPUT_PATH = "build\\bin"
FILE_OUTPUT_PATH = ""

IGNORE_KEYWORD = "PRE_BUILD_IGNORE"  # Keyword that makes this script ignore a line

BYPASS_SCRIPT = os.path.exists("script.disable")  # bypass script if this file is found

MAX_RESULT = 8  # The maximum number of results to print out for errors and modified files

LIMIT_TAG = 254
LIMIT_ID = 65535
BLACKLIST_ADDRESSESS = (0, 5, 9)

SOURCE_DEST_NAME = f"{WORKING_DIRECTORY_OFFSET}{SOURCE_NAME}"
LIBRARIES_DEST_NAME = f"{WORKING_DIRECTORY_OFFSET}{LIBRARIES_NAME}"
DATA_FILE = f"{WORKING_DIRECTORY_OFFSET}.LogInfo"

LOW_RAM = 4
BUF_SIZE = 65536

RAM_MEMO = False

INCLUDED_FILE_TYPES = (".c", ".cpp", ".h", ".hpp", ".t", ".tpp", ".s", ".def")


def getLibraryBlacklist() -> dict[str, list]:
    """Get the library folder blacklist based on core model

    Returns:
        dict[str, list]: folder blacklist dict
    """
    blacklist: dict[str, list] = {}
    with open(BLACKLIST, "r", encoding="utf-8") as file:
        currentModel = ""
        for line in file.readlines():
            if line[0] == ".":
                currentModel = line.split(" ")[0][1:]
                if currentModel not in blacklist:
                    blacklist[currentModel] = []
            else:
                for token in line.split(" "):
                    token = token.strip(" \n")
                    if not token or token[0] == "#":
                        break
                    elif os.path.exists(join_path(os.path.dirname(BLACKLIST), token)):
                        blacklist[currentModel].append(join_path(os.path.dirname(BLACKLIST), token))
    return blacklist


def available_ram():
    global RAM_MEMO
    if not RAM_MEMO:
        out = subprocess.check_output("wmic OS get FreePhysicalMemory /Value", stderr=subprocess.STDOUT, shell=True)
        RAM_MEMO = round(
            int(str(out).strip("b").strip("'").replace("\\r", "").replace("\\n", "").replace("FreePhysicalMemory=", "")) / 1048576, 2
        )
    return RAM_MEMO


def hashFile(filePath):
    if os.path.exists(filePath):
        if available_ram() <= LOW_RAM:
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
        return f"\033[91m\033[1m\033[4m{text}\033[0m"

    @staticmethod
    def underline(text):
        return f"\033[4m{text}\033[0m"

    @staticmethod
    def header(text):
        return f"\033[1m\033[4m{text}\033[0m"

    @staticmethod
    def warning(text):
        return f"\033[93m\033[1m{text}\033[0m"

    @staticmethod
    def yellow(text):
        return f"\033[93m{text}\033[0m"

    @staticmethod
    def important(text):
        return f"\033[94m\033[1m{text}\033[0m"

    @staticmethod
    def reallyImportant(text):
        return f"\033[94m\033[1m\033[4m{text}\033[0m"

    @staticmethod
    def green(text):
        return f"\033[92m{text}\033[0m"

    @staticmethod
    def red(text):
        return f"\033[91m{text}\033[0m"


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


FIND_SPECIAL_REGEX = (
    r"_LogPrebuildString\s*\(\s*(\".*?\")\s*\)"  # -> _LogPrebuildString("Str") # Special case where we can indirectly allocate a string
)
FIND_CALL_REGEX_SS = r"Log(\.[diwef])?\s*\(\s*(\".*?\")\s*,\s*(\".*?\")\s*\)\s*;"  # -> Log("Str", "Str");
FIND_CALL_REGEX_VS = r"Log(\.[diwef])?\s*\(\s*([^\"]+?)\s*,\s*(\".*?\")\s*\)\s*;"  # -> Log(Var, "Str");
FIND_CALL_REGEX_SSV = r"Log(\.[diwef])?\s*\(\s*(\".*?\")\s*,\s*(\".*?\")\s*,\s*([^\"]+?)\s*\)\s*;"  # -> Log("Str", "Str", Var); # FIXME: SSV does not seem to be working
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
    errors: list[str]

    def __init__(self, RawPath, FilePath, FileName, Offset):
        if not os.path.exists(FilePath):
            raise FileNotFoundError(FilePath)

        self.name = FileName
        self.path = FilePath
        self.rawpath = RawPath
        self.workingPath = f"{Offset}{FilePath}"
        self.offset = Offset
        self.errors = list()

        # if O_Files.get(self.workingPath):
        # print("Records show {} exists".format(FileName))

        touch(f"{Offset}{RawPath}")

    def newError(self, exception: Exception, name: str, tag: str):
        self.errors.append(f"  {name}:{tag}\n   {Text.red(type(exception).__name__)}\n    > {splitErrorString(exception)}\n")

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
        ID = await self.addNewID("", reMatch)  # Special strings are always LOG for simplicity
        return line.replace(reMatch, str(ID))

    async def VSX(self, line, reMatch):
        ID = await self.addNewID(reMatch[0], reMatch[2])
        return line.replace(reMatch[2], str(ID))

    async def SSX(self, line, reMatch):
        TAG = await self.addNewTag(reMatch[1])
        ID = await self.addNewID(reMatch[0], reMatch[2])
        return line.replace(reMatch[1], str(TAG)).replace(reMatch[2], str(ID))

    async def NEW_TAG(self, line, reMatch):
        TAG = await self.addNewTag(reMatch)
        return line.replace(reMatch, str(TAG))

    async def walkLines(self, function):
        temp_path = self.workingPath + ".__Lock"
        line_no = 1
        newline = ""
        with open(self.path, "r", encoding="utf-8") as f1, open(temp_path, "w", encoding="utf-8") as f2:
            if self.rawpath.startswith(LIB_PATH) and self.name != LIB_FILE:  # Ignore log library source files
                f2.writelines(f1.readlines())
            else:
                for line in f1:
                    try:
                        newline = await function(line)
                        f2.buffer.write(newline.encode("utf-8"))
                    except Exception as e:  # If prev exception was about IO then oh well
                        self.newError(e, self.path, line_no)
                        f2.buffer.write(line.encode("utf-8"))
                    finally:
                        line_no += 1
        self.modified = not syncFile(temp_path, self.offset, self.rawpath, self.workingPath)
        os.remove(temp_path)

    async def findLogMatch(self, line: str):
        newline: str = line

        if IGNORE_KEYWORD in newline:  # Return if this line has the ignore keyword
            return newline

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
        try:
            await File.scan()
        except Exception as e:
            File.newError(e, "Thread Error", File.name)


def run_ingest_files(finishFunc, *FilesEntries):
    asyncio.run(ingest_files(finishFunc, FilesEntries))


Files: set[FileEntry] = set()
FileRefs = set()
Threads = set()
Excluded_dirs = set()

FILE_CHANGE = False


def syncFile(filePath, offset, rawpath, workingFilePath=None, suppress=False):
    workingFilePath = workingFilePath or f"{offset}{filePath}"
    global FILE_CHANGE

    new = hashFile(filePath)
    old = hashFile(workingFilePath)
    if old == "":
        old = 0
    FILE_CHANGE = FILE_CHANGE and (new == old)

    if not os.path.exists(workingFilePath) or new != old:
        touch(f"{offset}{rawpath}")
        shutil.copyfile(filePath, workingFilePath)
        if not suppress:
            print(f"Sync File: {os.path.basename(workingFilePath)}")
        return False
    return True


def allocate_files(path, offset):
    async def lib_flag(line):
        return line.replace(*LIB_DEFINE)

    blacklist = getLibraryBlacklist()
    model = load_json()[Settings.CORE_NAME.key]
    if model in blacklist:
        blacklist = blacklist[model]
    else:
        blacklist = []

    for subdir, _, files in os.walk(path):
        cont = False
        for dir in blacklist:
            if str(subdir).startswith(dir):
                Excluded_dirs.add(dir)
                cont = True
                break
        if cont:
            continue
        for filename in files:
            if pathlib.Path(filename).suffix.lower() not in INCLUDED_FILE_TYPES:
                continue
            filepath = subdir + os.sep + filename
            rawpath = subdir + os.sep
            if BYPASS_SCRIPT:
                syncFile(filepath, offset, rawpath, suppress=True)
                continue
            if rawpath.startswith(LIB_PATH):
                libFile = FileEntry(rawpath, filepath, filename, offset)
                if libFile.name == LIB_FILE:
                    asyncio.run(libFile.walkLines(lib_flag))
                    continue
            File_Ent = FileEntry(rawpath, filepath, filename, offset)
            Files.add(File_Ent)
            FileRefs.add(File_Ent)

    for dir in blacklist:
        rmPath = join_path(offset, dir)
        if os.path.exists(rmPath):
            shutil.rmtree(rmPath)


def dole_files(count, finishFunc):
    while True:
        file_set: set[FileEntry] = set()

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
    c = 0
    m = 0

    extStr: str = ""

    for dir in Excluded_dirs:
        if c < MAX_RESULT:
            extStr += f"  {dir}\n"
            c += 1
        else:
            m += 1

    if c > 0:
        print(Text.header("\nExcluded Folders:"))
        print(Text.yellow(extStr.strip("\n")))
        if m > 0:
            print(Text.underline(Text.yellow("  {} more folder{}".format(m, "s" if m > 1 else ""))))

    c = 0
    m = 0

    extStr: str = ""

    for f in FileRefs:
        if f.modified:
            if c < MAX_RESULT:
                extStr += f"  {f.name}\n"
                c += 1
            else:
                m += 1
        if len(f.errors) > 0:
            Files.add(f)

    if c > 0:
        print(Text.header("\nModified Files:"))
        print(Text.green(extStr.strip("\n")))
        if m > 0:
            print(Text.underline(Text.green("  {} more file{}".format(m, "s" if m > 1 else ""))))

    sys.stdout.flush()

    c = 0
    m = 0

    extStr: str = ""

    for f in Files:
        for e in f.errors:
            if c < MAX_RESULT:
                extStr += e
                c += 1
            else:
                m += 1

    if c > 0:
        print(Text.header("\nFile Errors:"))
        print(extStr.strip("\n"))

        if m > 0:
            print(Text.underline(Text.red("  {} more error{}".format(m, "s" if m > 1 else ""))))


def getOutputFile(path):
    output_name = "log_lookup.json"
    savePath = f"{path}\\{output_name}"
    if savePath.startswith("\\"):
        savePath = output_name
    return savePath


def save_lookup(path):
    toSave = (TAGs, IDs)
    touch(path)
    with open(getOutputFile(path), "w") as f:
        json.dump(toSave, f, indent=4, separators=(",", ": "))


def checkGitSubmodules():
    err = False
    try:
        response = subprocess.check_output("git config -f .gitmodules -l", stderr=subprocess.STDOUT, shell=True)
        submodules = tuple(line.split("=")[1] for line in response.decode("utf-8").splitlines() if ".url=" not in line)
        for module in submodules:
            if (
                not os.path.exists(module)
                or not os.path.isdir(module)
                or not list(1 for ext in INCLUDED_FILE_TYPES if len(glob.glob(f"**{os.path.sep}*{ext}", root_dir=module, recursive=True)))
            ):
                print(Text.warning("Submodule does not exist, or contains no source files : " + module))
                err = True
    except subprocess.CalledProcessError:
        print(Text.error("Failed to check for git submodules"))
    if err:
        print(Text.important("\nConsider running " + Text.red("git pull --recurse-submodules")))
        print(Text.important("or " + Text.red("git submodule update --init")) + Text.important(" if repo has just been cloned\n"))


# Start Script
def main():
    checkGitSubmodules()

    touch(SOURCE_DEST_NAME)
    touch(LIBRARIES_DEST_NAME)

    allocate_files(SOURCE_NAME, WORKING_DIRECTORY_OFFSET)
    allocate_files(LIBRARIES_NAME, WORKING_DIRECTORY_OFFSET)

    if not BYPASS_SCRIPT:
        time.sleep(0.5)  # Let terminal settle

        print(Text.warning(f"Available Ram: {available_ram()} GBs\n"))

        prehash = hashFile(getOutputFile(FILE_OUTPUT_PATH))

        print(f"Files to search: {len(FileRefs)}")

        tb = ThreadedProgressBar(len(Files), Text.important("Completed Files:"))

        dole_files(8, tb.progress)

        print(f"Threads to run: {len(Threads)}\n\n")

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

        print()

    print("Converting LogMap 📃\n")
    subprocess.Popen(
        [
            "python",
            "bin2cc.py",
            "-i",
            "log_lookup.json",
            "-o",
            f"{WORKING_DIRECTORY_OFFSET}{LIB_PATH}\\log_lookup.cpp",
            "-v",
            "log_lookup",
        ]
    ).wait()

    # try:
    #     shutil.rmtree(SOURCE_DEST_NAME)
    #     shutil.rmtree(LIBRARIES_DEST_NAME)
    # except FileNotFoundError:
    #     pass


if __name__ == "__main__":
    main()

# @endcond
