import json
import threading

import script.error as Error
import script.util as Util


LIMIT_TAG = 65535
LIMIT_ID = 65535
TAG_MAP: dict[str, tuple[int, int]] = {
    "STATE": (2, 256),
    "VALUE": (256, 4096),
    "ELSE": (4096, LIMIT_TAG),
}
BLACKLIST_IDS = (0, 1)

NEW_DATA = ({}, {}, {})

O_TAGs = NEW_DATA[0]
O_IDs = NEW_DATA[1]
O_Files = NEW_DATA[2]

TAGs = {}
IDs = {}
Files = {}

TAG_SEM = threading.BoundedSemaphore(1)
ID_SEM = threading.BoundedSemaphore(1)
FILE_SEM = threading.BoundedSemaphore(1)


def set_tag(string: str, numberID: int):
    TAG_SEM.acquire()
    TAGs[string] = numberID
    TAG_SEM.release()


async def getUniqueID(findDuplicate=None):
    if IDs.get(findDuplicate):
        return IDs[findDuplicate]

    ID_SEM.acquire()
    old_vals = set(O_IDs.values())
    vals = set(IDs.values())

    for i in range(1, LIMIT_ID):
        if i not in BLACKLIST_IDS and i not in old_vals and i not in vals:
            IDs[""] = i  # Claim before returning
            ID_SEM.release()
            return i

    # Attempt to clear up some IDs
    raise Error.OutOfIDsException


async def getUniqueTAG(findDuplicate=None):
    if TAGs.get(findDuplicate):
        return TAGs[findDuplicate]

    TAG_SEM.acquire()
    old_vals = set(O_TAGs.values())
    vals = set(TAGs.values())

    for i in range(1, LIMIT_TAG):
        if i not in BLACKLIST_IDS and i not in old_vals and i not in vals:
            TAGs[""] = i  # Claim before returning
            TAG_SEM.release()
            return i

    # Attempt to clear up some IDs
    raise Error.OutOfIDsException


def clear_blanks():
    try:
        del IDs[""]
    except KeyError:
        pass
    try:
        del TAGs[""]
    except KeyError:
        pass


def save_lookup(path):
    to_save = (TAGs, IDs)
    Util.touch(path)
    with open(Util.getOutputFile(path), "w", encoding="utf-8") as file:
        json.dump(to_save, file, indent=4, separators=(",", ": "))
