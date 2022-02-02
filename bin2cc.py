"""
@file bin2cc.py
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

LINE_LEN = 8


def bin2cc(data: bytes, var_name: str) -> str:
    """Generate a C char array from bytes
    Args:
        data (bytes): The bytes
        var_name (str): The C variable name

    Returns:
        str: The string of C code
    """

    # zlib compress data
    ulen = len(data)
    data = bytes(data, "UTF-8")
    data = zlib.compress(data, 9)

    data = data.hex(" ", 1).split(" ")

    out: list[str] = []
    out.append(f"unsigned char {var_name}[] = {{")

    chunks = [data[i : i + LINE_LEN] for i in range(0, len(data), LINE_LEN)]

    padding_len = 0

    for i, nums in enumerate(chunks):
        line = ", ".join([f"0x{c}" for c in nums])
        comma = "," if (i < len(chunks) - 1) else ""
        padding_len = LINE_LEN - len(nums)
        out.append(f"    {line}{comma}")

    if padding_len > 0:
        out[len(out) - 1] += ", 0x00" * padding_len
    # out[len(out) - 1] += ","
    out.append("};")
    out.append(f"unsigned int {var_name}_pad_len = {len(data)+padding_len};")
    out.append(f"unsigned int {var_name}_uncmp_len = {ulen};")
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

    out = bin2cc(data, args.var)
    with open(args.out, "w", encoding="UTF-8") as file:
        file.write(out)

    return 0


if __name__ == "__main__":
    sys.exit(main())
