import json
import threading

import script.error as Error
import script.util as Util


LIMIT_TAG = 65535
LIMIT_ID = 65535
BLACKLIST_IDS = (0, 1)

TAG_RANGE_STATE = range(2, 256)
TAG_RANGE_VALUE = range(256, 4096)
TAG_RANGE_ELSE = range(4096, LIMIT_TAG)

ID_RANGE_VALUE = range(256, 4096)
ID_RANGE_ELSE = list(range(2, 256)) + list(range(4096, LIMIT_ID))

TAGs = {}
IDs = {}
Files = {}

TAG_SEM = threading.BoundedSemaphore(1)
ID_SEM = threading.BoundedSemaphore(1)
FILE_SEM = threading.BoundedSemaphore(1)


async def map_unique_pair(string_id: str, string_tag: str, map_range: range) -> int:
    if TAGs.get(string_tag) and IDs.get(string_id):
        if TAGs[string_tag] != IDs[string_id]:
            raise Error.TAGIDMismatchException(f"{string_tag} : {string_id}")
        return TAGs[string_tag]

    with ID_SEM and TAG_SEM:
        ids = set(IDs.values())
        tags = set(TAGs.values())

        for i in map_range:
            if i not in BLACKLIST_IDS and i not in ids and i not in tags:
                IDs[string_id] = i
                TAGs[string_tag] = i
                return i

    raise Error.OutOfRangeException("Either IDs or TAGs are out")


async def map_unique_id(string_id: str, map_range: range = TAG_RANGE_ELSE) -> int:
    if IDs.get(string_id):
        return IDs[string_id]

    with ID_SEM:
        values = set(IDs.values())

        for i in map_range:
            if i not in BLACKLIST_IDS and i not in values:
                IDs[string_id] = i
                return i

    # Attempt to clear up some IDs
    raise Error.OutOfRangeException("IDs")


async def map_unique_tag(string_tag: str, map_range: range = ID_RANGE_ELSE) -> int:
    if TAGs.get(string_tag):
        return TAGs[string_tag]

    with TAG_SEM:
        values = set(TAGs.values())

        for i in map_range:
            if i not in BLACKLIST_IDS and i not in values:
                TAGs[string_tag] = i
                return i

    # Attempt to clear up some IDs
    raise Error.OutOfRangeException("TAGs")


def clear_blanks() -> None:
    try:
        del IDs[""]
    except KeyError:
        pass
    try:
        del TAGs[""]
    except KeyError:
        pass


def save_lookup(path) -> None:
    to_save = (TAGs, IDs)
    Util.touch(path)
    with open(path, "w", encoding="utf-8") as file:
        json.dump(to_save, file, indent=4, separators=(",", ": "))
