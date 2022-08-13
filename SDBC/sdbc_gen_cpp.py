"""Generate C/C++ headers and definition files from an SDBC file"""

import sdbc_parse as sdbc

GEN_LOC = "libraries/SDBC/SDBC.def"
SDBC_LOC = "SDBC/Hawkrod.sdbc"

START_DOC = """/**
 * AUTO GENERATED FILE - DO NOT EDIT
 * 
 * SDBC Values, defined in C as their respective ID
 */

#include <stdint.h>

"""
VALUE_LINE = "#define {} {}\n"
X_MAC = "    X({}) \\\n"

TYPE_CONV = {
    "long": "int64_t",
    "int": "int32_t",
    "short": "int16_t",
    "byte": "int8_t",
    "float": "float",
    "double": "double",
    "bool": "bool",
    "binary": "char",
}

db = sdbc.parse_file(SDBC_LOC)


def is_int(num):
    return int(num) == num


def main() -> None:
    with open(GEN_LOC, "w", encoding="utf-8") as f:
        f.write(START_DOC)
        f.write(f"#define CAN_MESSAGE_NO {len(db.messages)}\n")

        msg_x = "\n"

        for msg in db.messages:
            f.write("\n// " + msg.name + "\n")
            for sig in msg.signals:

                flow = f"| {msg.sender if msg.sender else ''} >> {sig.receiver if sig.receiver else ''}" if msg.sender or sig.receiver else None
                desc = sig.description
                comment = f" // {f'{desc}' if desc else ''}{' 'if desc and flow else ''}{flow if flow else ''}" if desc or flow else ''

                f.write(VALUE_LINE.format(
                    f"POST_{sig.name}", f"{msg.uid}, {sig.bits}, {sig.position}{comment}"))
                typ = sig.form.f_type.name.lower()
                typ = TYPE_CONV[typ] if typ in TYPE_CONV else typ
                if not sig.signed and typ.startswith("int"):
                    typ = "u" + typ

                scale = "" if sig.scale == 1 else f"{sig.scale}"
                offset = "" if sig.offset == 0 else f"{sig.offset}"

                sver = "_s" if scale or offset else ""

                scale = scale if scale else "1"
                offset = offset if offset else "0"

                calc_type = None

                if is_int(sig.scale) and is_int(sig.offset):
                    calc_type = "uint32_t" if sig.bits <= 32 else "uint64_t"
                else:
                    calc_type = "float" if sig.bits <= 32 else "double"

                f.write(
                    VALUE_LINE.format(
                        f"POST_{sig.name}_META", f"{typ}, {32 if sig.bits <= 32 else 64}, {scale}, {offset}, {calc_type}, {sver}")
                )
            if msg is db.messages[-1]:
                msg_x += X_MAC[:-2].format('"' + msg.name + '", ' + str(msg.can_id))
                msg_x += "\n"
            else:
                msg_x += X_MAC.format('"' + msg.name + '", ' + str(msg.can_id))

        f.write("\n#define CAN_IDS \\")
        f.write(msg_x)


if __name__ == "__main__":
    main()
