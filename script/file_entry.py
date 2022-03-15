from msilib import sequence
import os
import re

import script.regex as REGEX
import script.error as Error
import script.text as Text
import script.util as Util
import script.id_matcher as IDMatch

IGNORE_KEYWORD = "PRE_BUILD_IGNORE"  # Keyword that makes this script ignore a line

FILE_LOG_LEVELS = {
    "": "[ LOG ] ",
    "d": "[DEBUG] ",
    "p": "[POST]  ",
    "i": "[INFO]  ",
    "w": "[WARN]  ",
    "e": "[ERROR] ",
    "f": "[FATAL] ",
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

        Util.touch(f"{Offset}{RawPath}")

    def newError(self, exception: Exception, name: str, tag: str):
        self.errors.append(f"  {name}:{tag}\n   {Text.red(type(exception).__name__)}\n    > {Error.error_to_string(exception)}\n")

    async def addNewTag(self, raw_str):
        string = "[{}]".format(raw_str.strip('"'))
        numberID = await IDMatch.getUniqueTAG(string)
        IDMatch.set_tag(string, numberID)
        return numberID

    async def addNewID(self, raw_log_level, raw_str):
        string = FILE_LOG_LEVELS[raw_log_level] + raw_str.strip('"')
        numberID = await IDMatch.getUniqueID(string)
        IDMatch.set_tag(string, numberID)
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

    async def map_lines(self, function):
        temp_path = self.workingPath + ".__Lock"
        line_no = 1
        newline = ""

        with open(self.path, "r", encoding="utf-8") as original, open(temp_path, "w", encoding="utf-8") as new_file:
            for line in original:
                try:
                    newline = await function(line)
                    new_file.buffer.write(newline.encode("utf-8"))
                except Exception as err:  # If prev exception was about IO then oh well
                    self.newError(err, self.path, line_no)
                    new_file.buffer.write(line.encode("utf-8"))
                finally:
                    line_no += 1
        self.modified = not Util.syncFile(temp_path, self.offset, self.rawpath, self.workingPath)
        os.remove(temp_path)

    async def match_log_mapping(self, line: str):
        newline: str = line

        if IGNORE_KEYWORD in newline:  # Return if this line has the ignore keyword
            return newline

        scan_sequence = [
            (REGEX.CALL_VS, self.VSX),
            (REGEX.CALL_VSV, self.VSX),
            (REGEX.CALL_SS, self.SSX),
            (REGEX.CALL_SSV, self.SSX),
            (REGEX.SPECIAL_PASS, self.SPECIAL_STR),
            (REGEX.TAG_PASS, self.NEW_TAG),
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

    async def scan(self):
        await self.map_lines(self.match_log_mapping)
