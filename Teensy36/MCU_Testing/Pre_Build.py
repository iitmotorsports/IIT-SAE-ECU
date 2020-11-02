import fileinput
import shutil
import dill  # Separate Dependency
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

SOURCE_NAME = "src"
LIBRARIES_NAME = "libraries"
WORKING_DIRECTORY_OFFSET = "build\\"
LIMIT_TAG = 254
LIMIT_ID = 65535

SOURCE_DEST_NAME = "{}{}".format(WORKING_DIRECTORY_OFFSET, SOURCE_NAME)
LIBRARIES_DEST_NAME = "{}{}".format(WORKING_DIRECTORY_OFFSET, LIBRARIES_NAME)
DATA_FILE = "{}.LogInfo".format(WORKING_DIRECTORY_OFFSET)

# TODO: Remove after implementing persistent data
shutil.rmtree(SOURCE_DEST_NAME)
shutil.rmtree(LIBRARIES_DEST_NAME)


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


try:
    shutil.rmtree(SOURCE_DEST_NAME)
    shutil.rmtree(LIBRARIES_DEST_NAME)
except FileNotFoundError:
    pass


if not os.path.isdir(SOURCE_DEST_NAME):
    os.mkdir(SOURCE_DEST_NAME)


if not os.path.isdir(LIBRARIES_DEST_NAME):
    os.mkdir(LIBRARIES_DEST_NAME)


def save_data(Object):
    with open(DATA_FILE, "wb") as f:
        dill.dump(Object, f)


def load_data():
    if os.path.exists(DATA_FILE):
        with open(DATA_FILE, "rb") as f:
            return dill.load(f)
    print("No {} found".format(DATA_FILE))
    return ({}, {}, {})


def touch(rawpath):
    if not os.path.exists(rawpath):
        try:
            os.makedirs(rawpath)
        except OSError as exc:
            if exc.errno != errno.EEXIST:
                raise


class OutOfIDsException(Exception):
    pass


class OutOfTAGsException(Exception):
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

BlackList_IDs = (0, 5, 9)


async def getUniqueID():
    ID_SEM.acquire()
    old_keys = set(O_IDs.keys())
    keys = set(IDs.keys())

    for i in range(LIMIT_ID):
        if i not in BlackList_IDs and i not in old_keys and i not in keys:  # IMPROVE: Any better way?
            ID_SEM.release()
            return i

    # Attempt to clear up some IDs
    raise OutOfIDsException


def updateMap(oMap, uMap):
    keys = set(oMap.keys())
    for e in uMap.items:
        if e[0] in keys:  # Does the key already exist
            if e[1] != oMap[e[0]]:  # If mismatched values, throw warning
                print("Warning, existing mapping has changed {} = {} >> {}".format(e[0], oMap[e[0]], e[1]))
                oMap[e[0]] = e[1]
        else:  # If not, just update dict
            oMap[e[0]] = e[1]


async def logFile(FilePath, File_TAGs, File_IDs):
    FILE_SEM.acquire()

    updateMap(TAGs, File_TAGs)
    updateMap(IDs, File_IDs)

    Files.update({FilePath: {}})
    Files[FilePath].update({"Hash": 1})
    FILE_SEM.release()


PLACEHOLDER_TAG = "__PYTHON__TAG__PLACEHOLDER__"
PLACEHOLDER_ID = "__PYTHON__ID__PLACEHOLDER__"


# def placeHolderID():
#     return "__PYTHON__ID__PLACEHOLDER__{}__".format()


class FileEntry:
    name = ""
    path = ""
    workingPath = ""
    TAGS = {}
    IDS = {}

    def __init__(self, FilePath, FileName):
        if not os.path.exists(FilePath):
            raise FileNotFoundError(FilePath)

        self.name = FileName
        self.path = FilePath
        self.workingPath = "{}{}".format(WORKING_DIRECTORY_OFFSET, FilePath)

        if O_Files.get(self.workingPath):
            print("Records show {} exists".format(FileName))

        touch(self.workingPath)
        shutil.copyfile(self.path, self.workingPath)

    async def scan(self):
        await self.getTAGPlaceholders()
        await self.getIDPlaceholders()

    async def getTAGPlaceholders(self):
        self.TAGS = {}

    async def getIDPlaceholders(self):
        self.IDS = {}

    def setTAGs(self):
        pass


async def ingest_file(FilePath, FileName):
    current_file = FileEntry(FilePath, FileName)
    await current_file.scan()


def run_ingest_file(filepath, filename):
    asyncio.run(ingest_file(filepath, filename))


def scan_files(Path):
    for subdir, _, files in os.walk(Path):
        for filename in files:
            filepath = subdir + os.sep + filename
            threading.Thread(target=run_ingest_file, args=(filepath, filename)).start()


scan_files(SOURCE_NAME)
# scan_files(LIBRARIES_NAME)
pp = pprint.PrettyPrinter(indent=4)
pp.pprint(Files)

# save_data((TAGs, IDs, Files))
