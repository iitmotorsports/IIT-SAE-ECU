import os
import re
from typing import Callable

import script.regex as REGEX
import script.error as Error
import script.text as Text
import script.util as Util
import script.id_matcher as IDMatch

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

    async def get_new_tag(self, raw_str: str) -> int:
        """get a new unique tag

        Args:
            raw_str (str): raw string to store

        Returns:
            int: uid to replace this string with
        """
        raw_str = raw_str.strip('"')
        string = f"[{raw_str}]"
        number_id = await IDMatch.getUniqueTAG(string)
        IDMatch.new_tag(string, number_id)
        return number_id

    async def get_new_id(self, raw_log_level: str, raw_str: str) -> int:
        """Get a new unique ID

        Args:
            raw_log_level (str): Logging level of this tag
            raw_str (str): raw string to store

        Returns:
            int: uid to replace this string with
        """
        string = FILE_LOG_LEVELS[raw_log_level] + raw_str.strip('"')
        number_id = await IDMatch.getUniqueID(string)
        IDMatch.new_id(string, number_id)
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
        return line.replace(matches, str(uid))

    async def line_tag(self, line: str, matches: list[str]) -> str:
        """Format defined log tags

        Args:
            line (str): The line that matched this type
            reMatch (list[str]): The resulting regex list

        Returns:
            str: Reformatted line
        """
        tag = await self.get_new_tag(matches)
        return line.replace(matches, str(tag))

    async def line_vsx(self, line: str, matches: list[str]) -> str:
        """Format 'variable string' type log calls

        Args:
            line (str): The line that matched this type
            reMatch (list[str]): The resulting regex list

        Returns:
            str: Reformatted line
        """
        uid = await self.get_new_id(matches[0], matches[2])
        return line.replace(matches[2], str(uid))

    async def line_ssx(self, line: str, matches: list[str]) -> str:
        """Format 'string string' type log calls

        Args:
            line (str): The line that matched this type
            reMatch (list[str]): The resulting regex list

        Returns:
            str: Reformatted line
        """
        tag = await self.get_new_tag(matches[1])
        uid = await self.get_new_id(matches[0], matches[2])
        return line.replace(matches[1], str(tag)).replace(matches[2], str(uid))

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
        self.modified = not Util.syncFile(temp_path, self.offset, self.rawpath, self.working_path)
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

    async def scan(self) -> None:
        """Begin replacing lines for mapping log calls"""
        await self.map_lines(self.match_log_mapping)
