"""
@file interpretPaste.py
@author IR
@brief interpret a paste.ee paste
@version 0.1
@date 2021-04-14

@copyright Copyright (c) 2022

**Usage**

List available pastes:  `-l`

This flag pulls the IDs of whatever logs the companion app has
uploaded to api.paste.ee (a free text file hosting site).
    
Interpret paste:        `-p[PasteID]`

This argument, when given a valid available ID, will download and interpret the log file.

Where interpreting means to convert the raw numbers of the log file into readable text.

Graph paste:            `-p[PasteID] -g`

This argument, when given a valid available ID, will download and interpret the
log file into an excel sheet, where each message is graphed.

Graphing uses timestamps that the companion app embeds into each log message.

@see Logging::Log_t for more info on log file interpretation and the companion app

"""

# @cond

import argparse
import base64
import json
import os
import re
import sys

from pip._vendor import requests

try:
    import openpyxl
except ImportError:
    print("Trying to Install required module: openpyxl")
    os.system("python -m pip install openpyxl")
import openpyxl

from openpyxl.chart import ScatterChart, Reference, Series


GETURL = "https://api.paste.ee/v1/pastes/{}"
API = str(base64.b64decode("dTBXUXZabUNsdVFkZWJycUlUNjZSRHJoR1paTlVXaXE3U09LTVlPUE8="), "UTF-8")
HEADERS = {"X-Auth-Token": API}
BEGIN_JSON = "---[ LOG MAP START ]---"
END_JSON = "---[ LOG MAP END ]---"
SAVE_NAME = "_Interpret"


def list_pastes() -> tuple:
    """List all the pastes on the paste.ee API

    Returns:
        tuple: (Return Status, set of IDs)
    """
    id_list = set()
    status = True
    req = requests.get("https://api.paste.ee/v1/pastes", headers=HEADERS)
    if int(req.status_code / 100) == 2:
        print(req.status_code, req.reason)
    else:
        print(req.status_code, req.reason, req.text)
        return (False, id_list)
    try:
        values = json.loads(req.text)
        for paste in values["data"]:
            id_list.add(paste["id"])
    except:
        status = False
    return (status, id_list)


def get_paste(log_id: str) -> str:
    """Get a log file from the paste.ee api given it's id string

    Args:
        log_id (str): id of the log

    Returns:
        str: The full log file that was uploaded
    """
    req = requests.get(GETURL.format(log_id), headers=HEADERS)
    if int(req.status_code / 100) == 2:
        print(req.status_code, req.reason)
    else:
        print(req.status_code, req.reason, req.text)
    raw = json.loads(req.text)["paste"]["sections"][0]["contents"]
    with open(f"{SAVE_NAME}_raw.log", "w") as raw_file:
        raw_file.writelines(raw)

    return raw


def get_log(log_id: str) -> str:
    """Get a log file given either a paste.ee ID or a local path to one

    Args:
        log_id (str): id or path of the log

    Returns:
        str: The full log file
    """
    print(f"Parameter Given: {log_id}")
    if os.path.exists(log_id):
        with open(log_id, "r") as file:
            return file.read()
    return get_paste(log_id)


def interpret(string: str) -> None:
    """Interpret a log file and print the interpretation to a local file

    Args:
        string (str): The raw log file
    """

    json_obj = json.loads(string[string.find(BEGIN_JSON) + len(BEGIN_JSON) : string.find(END_JSON)])
    data_stream = string[string.find(END_JSON) + len(END_JSON) :]
    tag_dict = {v: k for k, v in json_obj[0].items()}
    str_dict = {v: k for k, v in json_obj[1].items()}

    with open(f"{SAVE_NAME}.log", "w") as log_file:
        log_file.writelines(string[string.find(BEGIN_JSON) + len(BEGIN_JSON) : string.find(END_JSON)])
        for line in data_stream.splitlines():
            msg = line.split(" ")
            if len(msg) != 4:
                continue
            epoch = int(msg[0])
            tag_str = str_str = "Invalid Msg Key"
            try:
                tag_str = tag_dict[int(msg[1])]
            except KeyError:
                pass
            try:
                str_str = str_dict[int(msg[2])]
            except KeyError:
                pass
            log_file.write(f"[{epoch}] {tag_str: <16s} {str_str: <35s} {str(int(msg[3]))}\n")


def graph(string: str) -> None:
    """Interpret and graph a log file to a local excel file

    Args:
        string (str): The raw log file
    """

    json_obj = json.loads(string[string.find(BEGIN_JSON) + len(BEGIN_JSON) : string.find(END_JSON)])
    data_stream = string[string.find(END_JSON) + len(END_JSON) :]
    str_dict = {v: k for k, v in json_obj[1].items()}

    data = dict()

    data_titles = list(["Elapsed Time (s)"])

    epoch_smol = -1

    for line in data_stream.splitlines():
        try:
            msg = line.split(" ")
            if len(msg) != 4 or not str_dict.get(int(msg[2])):
                continue
            msg_id = str_dict[int(msg[2])]
            if not data.get(msg_id):
                data[msg_id] = list()
                data_titles.append(msg_id)
            epoch = int(msg[0])
            data[msg_id].append(list((epoch, int(msg[3]), data[msg_id])))

            if epoch_smol == -1 or epoch < epoch_smol:
                epoch_smol = epoch

        except KeyError as err:
            print("KeyError: ", err)

    for _, value in data.items():
        value.sort(key=lambda x: x[0])
        for lst in value:
            lst[0] -= epoch_smol
            lst[0] /= 1000000000

    wb = openpyxl.Workbook()
    ws = wb.active

    ws.append(data_titles)

    rows = list()

    def get_row():  # IMPROVE: better series generation
        row = list()
        for title in data_titles:
            column = data.get(title)
            if not column or len(column) == 0:
                row.append((epoch_smol + 1, 0))
            else:
                row.append(column[0])

        smallest_epoch = epoch_smol

        for item in row:
            if item[0] < smallest_epoch:
                smallest_epoch = item[0]

        if smallest_epoch == epoch_smol:
            return

        final_row = list([smallest_epoch])
        row.pop(0)

        for item in row:
            if item[0] != smallest_epoch:
                final_row.append(item[1])
            else:
                final_row.append(item[2].pop(0)[1])

        if len(final_row) == 1:
            return

        return final_row

    while True:
        row = get_row()
        if row:
            rows.append(row)
        else:
            break

    for row in rows:
        ws.append(row)

    chart = ScatterChart()
    chart.title = "Interpreted Data"
    chart.style = 2
    chart.x_axis.title = "Elapsed Time (s)"
    chart.y_axis.title = "Value"
    chart.height = 20
    chart.width = 45

    xvalues = Reference(ws, min_col=1, min_row=2, max_row=len(rows) + 1)

    for i in range(2, len(data_titles) + 1):
        values = Reference(ws, min_col=i, min_row=1, max_row=len(rows) + 1)
        series = Series(values, xvalues, title_from_data=True)
        # series.marker.symbol = "circle"
        # series.marker.size = 3
        # series.graphicalProperties.line.noFill = True
        series.smooth = True
        series.graphicalProperties.line.width = 10000  # width in EMUs
        chart.series.append(series)

    ws.add_chart(chart, "A1")
    wb.save("{}.xlsx".format(SAVE_NAME))


def main():
    """Main Function"""

    parser = argparse.ArgumentParser(description="Interpret a log file obtained locally or from paste.ee")
    parser.add_argument("-l", "--list", action="store_true", required=False, help="List all the available paste IDs")
    parser.add_argument(
        "-g", "--graph", action="store_true", required=False, help="Interpret and graph a log file to an excel file, given paste ID"
    )
    parser.add_argument("-p", "--paste", required=False, help="Paste ID to download and interpret")
    parser.add_argument("--log", required=False, help="The raw log file")

    args = parser.parse_args()

    if args.list:
        p_list = list_pastes()
        sys.exit(p_list[1] if p_list[0] else "Failed to get pastes")

    if args.paste:
        if args.graph:
            graph(get_log(args.paste))
            return
        interpret(get_log(args.paste))
    else:
        parser.print_help()


main()

# @endcond
