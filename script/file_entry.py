import os
import re
import traceback
from typing import Callable

import script.regex as REGEX
import script.error as Error
import script.text as Text
import script.util as Util
import script.id_matcher as IDMatch

from script.token_map import sel_dict, get_type
import tokenize
from io import BytesIO

IGNORE_KEYWORD = "PRE_BUILD_IGNORE"  # Keyword that makes this script ignore a line

FILE_LOG_LEVELS = {
    "p": "",
    "": "[ LOG ] ",
    "d": "[DEBUG] ",
    "i": "[INFO]  ",
    "w": "[WARN]  ",
    "e": "[ERROR] ",
    "f": "[FATAL] ",
}


class FileEntry:  # IMPROVE: Make IDs persistent
    """Represents a source file to map log calls with"""

    name = ""
    path = ""
    rawpath = ""
    working_path = ""
    offset = ""
    modified = False
    errors: list[str]

    def __init__(self, RawPath, FilePath, FileName, Offset):
        if not os.path.exists(FilePath):
            raise FileNotFoundError(FilePath)

        self.name = FileName
        self.path = FilePath
        self.rawpath = RawPath
        self.working_path = f"{Offset}{FilePath}"
        self.offset = Offset
        self.errors = list()

        Util.touch(f"{Offset}{RawPath}")

    def new_error(self, exception: Exception, name: str, tag: str) -> None:
        """Store an error to display later

        Args:
            exception (Exception): Exception to store
            name (str): Name of this error
            tag (str): Additional tag for this error
        """
        self.errors.append(f"  {name}:{tag}\n   {Text.red(type(exception).__name__)}\n    > {Error.error_to_string(exception)}\n")

    async def get_new_pair_mapping(self, tag_str: str, id_str: str, map_range: range) -> int:
        """Get a new unique mapping to both a TAG and ID

        Args:
            tag_str (str): raw tag string to store
            id_str (str): raw id string to store

        Returns:
            int: uid to replace these strings with
        """

        tag_str = tag_str.strip('"')
        tstring = f"[{tag_str}]"
        istring = id_str.strip('"')
        number_id = await IDMatch.map_unique_pair(tstring, istring, map_range)
        return number_id

    async def get_new_tag(self, tag_str: str) -> int:
        """Get a new unique TAG mapping

        Args:
            tag_str (str): raw string to store

        Returns:
            int: uid to replace this string with
        """

        state_match = re.findall(REGEX.STATE_PASS, tag_str)

        tag_str = tag_str.strip('"')
        tstring = f"[{tag_str}]"

        number_id = await IDMatch.map_unique_tag(tstring, IDMatch.TAG_RANGE_STATE if len(state_match) != 0 else IDMatch.TAG_RANGE_ELSE)
        return number_id

    async def get_new_id(self, raw_log_level: str, id_str: str) -> int:
        """Get a new unique ID mapping

        Args:
            raw_log_level (str): Logging level of this tag
            id_str (str): raw string to store

        Returns:
            int: uid to replace this string with
        """
        istring = FILE_LOG_LEVELS[raw_log_level] + id_str.strip('"')
        number_id = await IDMatch.map_unique_id(istring)
        return number_id

    async def line_special(self, line: str, matches: list[str]) -> str:
        """Format special string msg calls

        Args:
            line (str): The line that matched this type
            reMatch (list[str]): The resulting regex list

        Returns:
            str: Reformatted line
        """
        uid = await self.get_new_id("", matches)  # Special strings are always LOG for simplicity
        return line.replace(matches, str(uid), 1)

    async def line_tag(self, line: str, matches: list[str]) -> str:
        """Format defined log tags

        Args:
            line (str): The line that matched this type
            reMatch (list[str]): The resulting regex list

        Returns:
            str: Reformatted line
        """
        tag = await self.get_new_tag(matches)
        return line.replace(matches, str(tag), 1)

    async def line_vsx(self, line: str, matches: list[str]) -> str:
        """Format 'variable string' type log calls

        Args:
            line (str): The line that matched this type
            reMatch (list[str]): The resulting regex list

        Returns:
            str: Reformatted line
        """
        uid = await self.get_new_id(matches[0], matches[2])
        return line.replace(matches[2], str(uid), 1)

    async def line_ssx(self, line: str, matches: list[str]) -> str:
        """Format 'string string' type log calls

        Args:
            line (str): The line that matched this type
            reMatch (list[str]): The resulting regex list

        Returns:
            str: Reformatted line
        """

        tag = 0
        uid = 0

        if matches[0] == "p":
            tag = await self.get_new_pair_mapping(matches[1], matches[2], IDMatch.TAG_RANGE_VALUE)
            uid = tag
        else:
            tag = await self.get_new_tag(matches[1])
            uid = await self.get_new_id(matches[0], matches[2])

        return line.replace(matches[1], str(tag), 1).replace(matches[2], str(uid), 1)

    async def map_lines(self, function: Callable[[str], str]) -> None:
        """Map a function to each line of this file

        Args:
            function (Callable[[str], None]): Function that takes in a str and returns a str
        """
        temp_path = self.working_path + ".__Lock"
        line_no = 1
        newline = ""

        with open(self.path, "r", encoding="utf-8") as original, open(temp_path, "w", encoding="utf-8") as new_file:
            for line in original:
                try:
                    newline = await function(line)
                    new_file.buffer.write(newline.encode("utf-8"))
                except Exception as err:  # If prev exception was about IO then oh well
                    self.new_error(err, self.path, line_no)
                    new_file.buffer.write(line.encode("utf-8"))
                finally:
                    line_no += 1
        self.modified = Util.sync_file(temp_path, self.offset, self.rawpath, self.working_path)
        os.remove(temp_path)

    async def match_log_mapping(self, line: str) -> str:
        """Maps a source file line to Log related syntax

        Args:
            line (str): The line to scan

        Raises:
            ScriptException: On invalid Log related syntax

        Returns:
            str: Reformatted line
        """
        newline: str = line

        if IGNORE_KEYWORD in newline:  # Return if this line has the ignore keyword
            return newline

        scan_sequence = [
            (REGEX.CALL_VS, self.line_vsx),
            (REGEX.CALL_VSV, self.line_vsx),
            (REGEX.CALL_SS, self.line_ssx),
            (REGEX.CALL_SSV, self.line_ssx),
            (REGEX.SPECIAL_PASS, self.line_special),
            (REGEX.TAG_PASS, self.line_tag),
        ]

        for reg, func in scan_sequence:
            matches = re.findall(reg, line)
            if len(matches) != 0:
                return await func(line, matches[0])

        fail_sequence = [
            (REGEX.TAG_FAIL, Error.MalformedTAGDefinitionException),
            (REGEX.CALL_ERR_LITERAL, Error.MalformedLogCallException),
            (REGEX.CALL_ERR_BLANK, Error.BlankTAGException),
        ]

        for reg, exception in fail_sequence:
            err = re.findall(reg, line)
            if len(err) != 0:
                err = err[0]
                raise exception(err[0] + Text.error(err[1]) + err[2])

        return newline

    async def tokenize_line(self, line: str) -> str:
        sel = sel_dict
        try:
            tkns = tokenize.tokenize(BytesIO(line.encode('utf-8')).readline)
            next(tkns)
            ext = sel_dict['Log']['.']
            lid = sel_dict['Log']['(']
            
            for tkn in tkns:
                try:
                    if tkn.type is tokenize.INDENT:
                        continue
                    if sel is lid:
                        lid = None
                    if sel is ext:
                        ext = None
                    if tkn.string not in sel:
                        tkn = tkn.type
                    else:
                        tkn = tkn.string
                    sel = sel[tkn]
                    
                    if not ext:
                        ext = tkn
                    if sel == tokenize.ENDMARKER:
                        return await self.match_log_mapping(line)
                except KeyError:
                    pass
        except tokenize.TokenError:
            pass
        return line

    async def scan(self) -> None:
        """Begin replacing lines for mapping log calls"""
        await self.map_lines(self.match_log_mapping)
