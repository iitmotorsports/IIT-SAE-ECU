import json
from multiprocessing import BoundedSemaphore

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

TAG_SEM = BoundedSemaphore(1)
ID_SEM = BoundedSemaphore(1)
FILE_SEM = BoundedSemaphore(1)


async def map_unique_pair(string_tag: str, string_id: str, map_range: range) -> int:
    """Allocates both a TAG and ID mapping where they both map to the same number.

    Args:
        string_tag (str): The string tag to map
        string_id (str): The string message to map
        map_range (range): The range the mapping should be in

    Raises:
        Error.TAGIDMismatchException: If both mappings already exist and do not match in value
        Error.OutOfRangeException: If either mapping is unable to allocate another value within the given range

    Returns:
        int: The value mapping to both the string tag and message
    """
    if TAGs.get(string_tag) and IDs.get(string_id):  # TODO: attempt to match one mapping to the other if one already exists
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


async def map_unique_id(string_id: str, map_range: range = ID_RANGE_ELSE) -> int:
    """Allocates an ID mapping

    Args:
        string_id (str): The string message to map
        map_range (range, optional): The range the mapping should be in. Defaults to ID_RANGE_ELSE.

    Raises:
        Error.OutOfRangeException: If mapping is unable to allocate another value within the given range

    Returns:
        int: The value mapping to the string message
    """
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


async def map_unique_tag(string_tag: str, map_range: range = TAG_RANGE_ELSE) -> int:
    """Allocates a TAG mapping

    Args:
        string_tag (str): The string tag to map
        map_range (range, optional): The range the mapping should be in. Defaults to TAG_RANGE_ELSE.

    Raises:
        Error.OutOfRangeException: If mapping is unable to allocate another value within the given range

    Returns:
        int: The value mapping to the string tag
    """
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
    """Remove empty strings mappings, if any"""
    try:
        IDs.pop("")
    except KeyError:
        pass
    try:
        TAGs.pop("")
    except KeyError:
        pass


def save_lookup(file_path: str) -> None:
    """Saves current mappings

    Args:
        file_path (str): Valid path to a file to save to
    """
    to_save = (TAGs, IDs)
    Util.touch(file_path)
    with open(file_path, "w", encoding="utf-8") as file:
        json.dump(to_save, file, indent=4, separators=(",", ": "))
