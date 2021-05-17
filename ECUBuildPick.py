"""
@file ECUBuildPick.py
@author IR
@brief Simple Script used to make it easier to compile for two ECUs
@version 0.1
@date 2021-02-21

@copyright Copyright (c) 2021

This script is merely used to modify a global define inside a header file to change whether to compile for the front or back ECU.

I am not sure whether this is the best way to do this, but oh well ¯\\_(ツ)_/¯.
"""

# @cond
import os
import sys
from tempfile import mkstemp
from shutil import move, copymode
from os import fdopen, remove

FILE_REPLACE = "build\\Pre_Build\\libraries\\ECUGlobalConfig\\ECUGlobalConfig.h"

STR_BASE = "#define CONF_ECU_POSITION "
STR_FRONT = "FRONT_ECU"
STR_BACK = "BACK_ECU"


def replace(file_path, pattern, subst):
    # Create temp file
    fh, abs_path = mkstemp()
    with fdopen(fh, "w") as new_file:
        with open(file_path) as old_file:
            for line in old_file:
                new_file.write(line.replace(pattern, subst))
    # Copy the file permissions from the old file to the new file
    copymode(file_path, abs_path)
    # Remove original file
    remove(file_path)
    # Move new file
    move(abs_path, file_path)


if os.path.exists(FILE_REPLACE):
    if sys.argv[1] == "1":
        print("Setting to build for \033[91m\033[4mFront ECU\033[0m")
        replace(FILE_REPLACE, STR_BASE + STR_BACK, STR_BASE + STR_FRONT)
    elif sys.argv[1] == "0":
        print("Setting to build for \033[92m\033[4mBack ECU\033[0m")
        replace(FILE_REPLACE, STR_BASE + STR_FRONT, STR_BASE + STR_BACK)
    else:
        print("Invalid argument {}\n0 == Back\n1 == Front".format(sys.argv[1]))
else:
    print("Config files does not exist!\nHas the project been built at least once?")

# @endcond