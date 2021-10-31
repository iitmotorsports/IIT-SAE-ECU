"""
@file bin2c.py
@author LeHuman
@brief Convert file string to zlib compressed C array
@version 0.1
@date 2021-10-31

@copyright Copyright (c) 2021

https://nachtimwald.com/2019/10/09/python-binary-to-c-header/
"""

import sys
import zlib
import argparse


def bin2c(data: bytes, var_name: str) -> str:
    """Generate a C char array from bytes
    Args:
        data (bytes): The bytes
        var_name (str): The C variable name

    Returns:
        str: The string of C code
    """
    data = data.hex(" ", 1).split(" ")

    out: list[str] = []
    out.append(f"unsigned char {var_name}[] = {{")

    chunks = [data[i : i + 12] for i in range(0, len(data), 12)]

    for i, char in enumerate(chunks):
        line = ", ".join([f"0x{c}" for c in char])
        comma = "," if (i < len(chunks) - 1) else ""
        out.append(f"    {line}{comma}")

    out.append("};")
    out.append(f"unsigned int {var_name}_len = {len(data)};")

    return "\n".join(out)


def main():
    """Main Function"""
    parser = argparse.ArgumentParser(description="Generate binary header output")
    parser.add_argument("-i", "--input", required=True, help="Input file")
    parser.add_argument("-o", "--out", required=True, help="Output file")
    parser.add_argument("-v", "--var", required=True, help="Variable name to use in file")

    args = parser.parse_args()
    if not args:
        return 1

    with open(args.input, "r", encoding="UTF-8") as file:
        data = file.read()

    # Compress Data
    data = bytes(data, "UTF-8")
    data = zlib.compress(data, 9)

    out = bin2c(data, args.var)
    with open(args.out, "w", encoding="UTF-8") as file:
        file.write(out)

    return 0

if __name__ == "__main__":
    sys.exit(main())
