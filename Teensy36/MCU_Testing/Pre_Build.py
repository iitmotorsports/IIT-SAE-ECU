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

SOURCE_NAME = "src"
LIBRARIES_NAME = "libraries"
LIB_PATH = "libraries\\Log"  # Path to the implementation of Log
LIB_FILE = "LogConfig.def"
LIB_DEFINE = ("#define CONF_LOGGING_MAPPED_MODE 0", "#define CONF_LOGGING_MAPPED_MODE 1")
WORKING_DIRECTORY_OFFSET = "build\\Pre_Build\\"
FILE_OUTPUT_PATH = "build\\bin"

DISABLE_SCRIPT = False

LIMIT_TAG = 254
LIMIT_ID = 65535
BLACKLIST_ADDRESSESS = (0, 5, 9)

FILE_DOLE_COUNT = 4

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


print("Available Ram: {} GBs\n".format(AvailableRam()))
LOW_RAM = 4
BUF_SIZE = 65536


def hashFile(filePath):
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
            return hashlib.sha256(f.read()).digest()


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


class OutOfIDsException(Exception):
    pass


class OutOfTAGsException(Exception):
    pass


class MalformedTAGDefinitionException(Exception):
    pass


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
FIND_TAG_DEF_REGEX_BAD = r"LOG_TAG\s*[^\"=]+?;|LOG_TAG\s*[^\"=]+?=[^\"=]+?;"  # Implicit or single char definition of a tag type
FIND_TAG_DEF_REGEX_GOOD = r"LOG_TAG\s*[^\"=]+?=\s*(\".*?\")\s*;"  # Implicit or single char definition of a tag type

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
    workingPath = ""
    modified = False

    def __init__(self, RawPath, FilePath, FileName):
        if not os.path.exists(FilePath):
            raise FileNotFoundError(FilePath)

        self.name = FileName
        self.path = FilePath
        self.workingPath = "{}{}".format(WORKING_DIRECTORY_OFFSET, FilePath)

        if O_Files.get(self.workingPath):
            print("Records show {} exists".format(FileName))

        touch("{}{}".format(WORKING_DIRECTORY_OFFSET, RawPath))
        shutil.copyfile(FilePath, self.workingPath)

    async def VSX(self, line, reMatch):
        fnlStr = Log_Levels[reMatch[0]] + reMatch[2].strip('"')
        ID = await getUniqueID(fnlStr)
        IDs[fnlStr] = ID
        return line.replace(reMatch[2], str(ID))

    async def SSX(self, line, reMatch):
        TAG = await getUniqueTAG(reMatch[1])
        ID = await getUniqueID(reMatch[2])
        fnlStr = Log_Levels[reMatch[0]] + reMatch[2].strip('"')
        fnlTag = reMatch[2].strip('"')
        TAGs[fnlTag] = TAG
        IDs[fnlStr] = ID
        return line.replace(reMatch[1], str(TAG)).replace(reMatch[2], str(ID))

    async def NEW_TAG(self, line, reMatch):
        TAG = await getUniqueTAG(reMatch)
        fnlTag = reMatch.strip('"')
        TAGs[fnlTag] = TAG
        return line.replace(reMatch, str(TAG))

    async def walkLines(self, function):
        inplacePath = self.workingPath + ".__Lock"
        try:
            with open(self.workingPath, "r") as f1, open(inplacePath, "w") as f2:
                for line in f1:
                    newline = await function(line)
                    f2.write(newline)

                    if newline != line:
                        self.modified = True

            shutil.copyfile(inplacePath, self.workingPath)
        except Exception as e:
            print("File error: {}\n  {}:\n    {}\n".format(self.name, type(e).__name__, e))
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
                TAG_BAD = re.findall(FIND_TAG_DEF_REGEX_BAD, line)
                if TAG_BAD:
                    raise MalformedTAGDefinitionException(line)
                else:
                    VSV = re.findall(FIND_CALL_REGEX_VSV, line)
                    if len(VSV) != 0:
                        newline = await self.VSX(line, VSV[0])
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


FileCount = 0
FileNames = ""
Files = set()
FileRefs = set()
Threads = set()


def touch_base():
    try:  # TODO: Remove after implementing persistent data
        shutil.rmtree(SOURCE_DEST_NAME)
        shutil.rmtree(LIBRARIES_DEST_NAME)
    except FileNotFoundError:
        pass

    touch(SOURCE_DEST_NAME)
    touch(LIBRARIES_DEST_NAME)


def allocate_files(Path):

    global FileCount, FileNames

    async def lib_flag(line):
        return line.replace(LIB_DEFINE[0], LIB_DEFINE[1])

    for subdir, _, files in os.walk(Path):
        for filename in files:
            filepath = subdir + os.sep + filename
            rawpath = subdir + os.sep
            if DISABLE_SCRIPT:
                touch("{}{}".format(WORKING_DIRECTORY_OFFSET, rawpath))
                shutil.copyfile(filepath, "{}{}".format(WORKING_DIRECTORY_OFFSET, filepath))
                continue
            if rawpath.startswith(LIB_PATH):
                libFile = FileEntry(rawpath, filepath, filename)
                if libFile.name == LIB_FILE:
                    asyncio.run(libFile.walkLines(lib_flag))
                continue
            File_Ent = FileEntry(rawpath, filepath, filename)
            Files.add(File_Ent)
            FileRefs.add(File_Ent)

    FileCount += len(Files)

    while True:
        file_set = set()

        i = 0

        while len(Files) != 0 and i != FILE_DOLE_COUNT:
            file_set.add(Files.pop())
            i += 1

        if len(file_set) != 0:
            Threads.add(threading.Thread(target=run_ingest_files, args=(file_set)))

        if len(Files) == 0:
            break


def begin_scan():
    print("Files to search: {}".format(FileCount))
    print("Threads to run: {}\n".format(len(Threads)))

    for t in Threads:
        t.start()

    for t in Threads:
        t.join()

    del IDs[""]
    del TAGs[""]

    print("Modified Files:")
    for f in FileRefs:
        if f.modified:
            print("  {}".format(f.name))
    print()


def save_lookup(path):

    toSave = ({v: k for k, v in IDs.items()}, {v: k for k, v in TAGs.items()})
    touch(path)
    savePath = "{}\\log_lookup.json".format(path)
    with open(savePath, "w") as f:
        json.dump(toSave, f, indent=4, separators=(",", ": "))


touch_base()
allocate_files(SOURCE_NAME)
allocate_files(LIBRARIES_NAME)
if not DISABLE_SCRIPT:
    begin_scan()
    save_lookup(FILE_OUTPUT_PATH)
