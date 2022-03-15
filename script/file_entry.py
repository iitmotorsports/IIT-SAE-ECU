import os
import re

from os.path import join as join_path

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

    async def walkLines(self, function):
        temp_path = self.workingPath + ".__Lock"
        line_no = 1
        newline = ""
        with open(self.path, "r", encoding="utf-8") as f1, open(temp_path, "w", encoding="utf-8") as f2:
            for line in f1:
                try:
                    newline = await function(line)
                    f2.buffer.write(newline.encode("utf-8"))
                except Exception as e:  # If prev exception was about IO then oh well
                    self.newError(e, self.path, line_no)
                    f2.buffer.write(line.encode("utf-8"))
                finally:
                    line_no += 1
        self.modified = not Util.syncFile(temp_path, self.offset, self.rawpath, self.workingPath)
        os.remove(temp_path)

    async def findLogMatch(self, line: str):
        newline: str = line

        if IGNORE_KEYWORD in newline:  # Return if this line has the ignore keyword
            return newline

        special = re.findall(REGEX.SPECIAL_PASS, line)
        if special:
            newline = await self.SPECIAL_STR(line, special[0])
        else:
            vs = re.findall(REGEX.CALL_VS, line)
            if len(vs) != 0:  # ¯\_(ツ)_/¯
                newline = await self.VSX(line, vs[0])
            else:
                tag_good = re.findall(REGEX.TAG_PASS, line)
                if tag_good:
                    newline = await self.NEW_TAG(line, tag_good[0])
                else:
                    vsv = re.findall(REGEX.CALL_VSV, line)
                    if len(vsv) != 0:
                        newline = await self.VSX(line, vsv[0])
                    else:
                        tag_bad = re.findall(REGEX.TAG_FAIL, line)
                        if tag_bad:
                            tag_bad = tag_bad[0]
                            raise Error.MalformedTAGDefinitionException(tag_bad[0] + Text.error(tag_bad[1]) + tag_bad[2])
                        else:
                            bad = re.findall(REGEX.CALL_ERR_LITERAL, line)
                            if bad:
                                bad = bad[0]
                                raise Error.MalformedLogCallException(bad[0] + Text.error(bad[1]) + bad[2])
                            else:
                                ss = re.findall(REGEX.CALL_SS, line)
                                if len(ss) != 0:
                                    newline = await self.SSX(line, ss[0])
                                else:
                                    ssv = re.findall(REGEX.CALL_SSV, line)
                                    if len(ssv) != 0:
                                        newline = await self.SSX(line, ssv[0])
        return newline

    async def scan(self):
        await self.walkLines(self.findLogMatch)
