"""
@file Pre_Build.py
@author IR
@brief This script preprocesses source files for use with Log_t
@version 0.1
@date 2020-11-11

@copyright Copyright (c) 2022

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
import glob
import os
import asyncio
import threading
import time
import re
import sys
import json
import subprocess

from os.path import join as join_path
from vs_conf import load_json
from vs_conf import Settings

import script.util as Util
import script.text as Text
import script.id_matcher as IDMatch
from script.file_entry import FileEntry, LIB_PATH, LIB_FILE
from script.progress_bar import ProgressBar

SOURCE_NAME = "src"
LIBRARIES_NAME = "libraries"
LIB_DEFINE = ("#define CONF_LOGGING_MAPPED_MODE 0", "#define CONF_LOGGING_MAPPED_MODE 1")
WORKING_DIRECTORY_OFFSET = join_path("build", "Pre_Build", "")
FILE_OUTPUT_PATH = ""

BYPASS_SCRIPT = os.path.exists("script.disable")  # bypass script if this file is found

MAX_RESULT = 8  # The maximum number of results to print out for errors and modified files

SOURCE_DEST_NAME = f"{WORKING_DIRECTORY_OFFSET}{SOURCE_NAME}"
LIBRARIES_DEST_NAME = f"{WORKING_DIRECTORY_OFFSET}{LIBRARIES_NAME}"

INCLUDED_FILE_TYPES = (".c", ".cpp", ".h", ".hpp", ".t", ".tpp", ".s", ".def")


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


def allocate_files(path, offset):
    async def lib_flag(line):
        return line.replace(*LIB_DEFINE)

    blacklist = Util.getLibraryBlacklist()
    model: dict[str, str]
    try:
        model = load_json()[Settings.CORE_NAME.key]
    except json.JSONDecodeError:
        sys.exit(Text.error("Error loading settings file, consider running the 'VS Setup' task"))

    if model in blacklist:
        blacklist = blacklist[model]
    else:
        blacklist = []

    for subdir, _, files in os.walk(path):
        cont = False
        for directory in blacklist:
            if str(subdir).startswith(directory):
                Excluded_dirs.add(directory)
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
                Util.syncFile(filepath, offset, rawpath, suppress=True)
                continue
            if rawpath.startswith(LIB_PATH):
                libFile = FileEntry(rawpath, filepath, filename, offset)
                if libFile.name == LIB_FILE:
                    asyncio.run(libFile.walkLines(lib_flag))
                    continue
            File_Ent = FileEntry(rawpath, filepath, filename, offset)
            Files.add(File_Ent)
            FileRefs.add(File_Ent)

    for directory in blacklist:
        rmPath = join_path(offset, directory)
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


def begin_scan():
    for t in Threads:
        t.start()

    for t in Threads:
        t.join()

    IDMatch.clear_blanks()


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


# Start Script
def main():  # TODO: remove libraries from prebuild folder that are no longer in the actual folder
    Util.checkGitSubmodules(INCLUDED_FILE_TYPES)

    Util.touch(SOURCE_DEST_NAME)
    Util.touch(LIBRARIES_DEST_NAME)

    allocate_files(SOURCE_NAME, WORKING_DIRECTORY_OFFSET)
    allocate_files(LIBRARIES_NAME, WORKING_DIRECTORY_OFFSET)

    if not BYPASS_SCRIPT:
        time.sleep(0.5)  # Let terminal settle

        print(Text.warning(f"Available Ram: {Util.available_ram()} GBs\n"))

        prehash = Util.hashFile(Util.getOutputFile(FILE_OUTPUT_PATH))

        print(f"Files to search: {len(FileRefs)}")

        tb = ProgressBar(len(Files), Text.important("Completed Files:"))

        dole_files(8, tb.progress)

        print(f"Threads to run: {len(Threads)}\n\n")

        tb.start()
        begin_scan()
        tb.finish()

        printResults()
        IDMatch.save_lookup(FILE_OUTPUT_PATH)
        newhash = Util.hashFile(Util.getOutputFile(FILE_OUTPUT_PATH))
        if Util.FILES_CHANGED:
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


# TODO: only remove files/modules that no longer exist in actual directories

try:
    shutil.rmtree(SOURCE_DEST_NAME)
    shutil.rmtree(LIBRARIES_DEST_NAME)
except FileNotFoundError:
    pass


if __name__ == "__main__":
    main()

# @endcond
