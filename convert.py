import argparse
import os
import struct
import time

try:
    import tkinter as tk
    from tkinter import filedialog
    use_dialogs = True
except ImportError:
    use_dialogs = False


def process_binary(input_file_path: str, prepend_file_path: str, output_file_path: str) -> None:

    with open(prepend_file_path, 'r', encoding="utf-8") as input_file:
        input_data = input_file.read()

    with open(input_file_path, 'rb') as f:
        results = []
        while True:
            data = f.read(8)
            if not data:
                break
            num1, num2, num3 = struct.unpack('<HHI', data)
            results.append((num1, num2, num3))

    output_folder = os.path.dirname(output_file_path)
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    with open(output_file_path, 'w', encoding="utf-8") as output_file:
        output_file.write('---[ LOG MAP START ]---\n')
        output_file.write(input_data)
        output_file.write('\n---[ LOG MAP END ]---\n')
        cnt = int(time.time())
        for result in results:
            output_file.write(f'{cnt} {result[0]} {result[1]} {result[2]}\n')
            cnt += 1


def main() -> None:
    parser = argparse.ArgumentParser(description="Process binary file into three numbers per 8-byte chunk")
    parser.add_argument('-i', '--input_file_dir', required=False, type=str, help='the directory of the input file to prepend')
    parser.add_argument('-f','--file_dir', required=False, type=str, help='the directory of the binary file')
    parser.add_argument('-o','--output', required=False, type=str, help='the directory of the output file')
    args = parser.parse_args()

    if use_dialogs:
        root = tk.Tk()
        root.withdraw()
        if not args.file_dir:
            file_dir = filedialog.askopenfilename(title="Select log file")
        if not args.input_file_dir:
            input_file_dir = filedialog.askopenfilename(title="Select log map")
        if not args.output:
            output = filedialog.asksaveasfilename(title="Save output file as", defaultextension=".log")

    file_dir = args.file_dir or file_dir
    input_file_dir = args.input_file_dir or input_file_dir
    output = args.output or output

    process_binary(file_dir, input_file_dir, output)


if __name__ == "__main__":
    main()
