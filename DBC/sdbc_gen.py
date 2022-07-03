"""Generate a C header file from an SDBC file"""

import sdbc_parse as sdbc

GEN_LOC = "src/DBC.h"
SDBC_LOC = "DBC/Astriatus.sdbc"

START_DOC = """/**
 * AUTO GENERATED FILE - DO NOT EDIT
 * 
 * SDBC Values, defined in C as their respective ID
 */

"""
VALUE_LINE = "#define {} {}\n"

messages = sdbc.parse_file(SDBC_LOC)

with open(GEN_LOC, "w", encoding="utf-8") as f:
    f.write(START_DOC)
    f.write(f"#define CAN_MESSAGE_NO {len(messages)}\n\n")

    for msg in messages:
        for sgn in msg.signals:
            f.write(VALUE_LINE.format(f"CAN_{sgn.name}", msg.id))
