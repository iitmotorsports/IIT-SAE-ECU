"""
@file interpretPaste.py
@author IR
@brief interpret a paste.ee paste
@version 0.1
@date 2021-04-14

@copyright Copyright (c) 2021
"""

import base64
from pip._vendor import requests
import json
from time import sleep
import asyncio
import sys
import math
import os
import datetime

try:
    import openpyxl
except ImportError:
    print("Trying to Install required module: openpyxl")
    os.system("python -m pip install openpyxl")
import openpyxl

from openpyxl.chart import ScatterChart, Reference, Series


GETURL = "https://api.paste.ee/v1/pastes/{}"
API = str(base64.b64decode("dTBXUXZabUNsdVFkZWJycUlUNjZSRHJoR1paTlVXaXE3U09LTVlPUE8="), "UTF-8")
headers = {"X-Auth-Token": API}
BeginJSON = "---[ LOG MAP START ]---"
EndJSON = "---[ LOG MAP END ]---"
SaveName = "_Interpret"


def listPastes():
    IDList = set()
    status = True
    req = requests.get("https://api.paste.ee/v1/pastes", headers=headers)
    if int(req.status_code / 100) == 2:
        print(req.status_code, req.reason)
    else:
        print(req.status_code, req.reason, req.text)
        return (False, IDList)
    try:
        values = json.loads(req.text)
        for paste in values["data"]:
            IDList.add(paste["id"])
    except:
        status = False
    return (status, IDList)


def getPaste(id):
    req = requests.get(GETURL.format(id), headers=headers)
    if int(req.status_code / 100) == 2:
        print(req.status_code, req.reason)
    else:
        print(req.status_code, req.reason, req.text)
    raw = json.loads(req.text)["paste"]["sections"][0]["contents"]
    with open("{}_raw.log".format(SaveName), "w") as rawFile:
        rawFile.writelines(raw)

    return raw


def usage():
    print("List available pastes: \t-l\nInterpret paste: \t-p[PasteID]\Graph paste: \t-p[PasteID] -g[StringValue]")


def interpret(string: str):
    jsonOBJ = json.loads(string[string.find(BeginJSON) + len(BeginJSON) : string.find(EndJSON)])
    dataStream = string[string.find(EndJSON) + len(EndJSON) :]
    TagDict = {v: k for k, v in jsonOBJ[0].items()}
    StrDict = {v: k for k, v in jsonOBJ[1].items()}

    with open("{}.log".format(SaveName), "w") as logFile:
        logFile.writelines(string[string.find(BeginJSON) + len(BeginJSON) : string.find(EndJSON)])
        for line in dataStream.splitlines():
            msg = line.split(" ")
            if len(msg) != 4:
                continue
            # epoch = datetime.datetime.fromtimestamp(int(msg[0]) / 1000.0).strftime("%H:%M:%S:%f")
            epoch = int(msg[0])
            TagStr = StrStr = "Invalid Msg Key"
            try:
                TagStr = TagDict[int(msg[1])]
            except KeyError:
                pass
            try:
                StrStr = StrDict[int(msg[2])]
            except KeyError:
                pass
            logFile.write("[{0}] {1: <16s} {2: <35s} {3}\n".format(epoch, TagStr, StrStr, str(int(msg[3]))))


import pprint

from itertools import repeat


def graph(string: str):

    jsonOBJ = json.loads(string[string.find(BeginJSON) + len(BeginJSON) : string.find(EndJSON)])
    dataStream = string[string.find(EndJSON) + len(EndJSON) :]
    StrDict = {v: k for k, v in jsonOBJ[1].items()}

    data = dict()

    dataTitles = list(["Elapsed Time (s)"])

    epochSmol = -1

    for line in dataStream.splitlines():
        try:
            msg = line.split(" ")
            if len(msg) != 4 or not StrDict.get(int(msg[2])):
                continue
            msgID = StrDict[int(msg[2])]
            if not data.get(msgID):
                data[msgID] = list()
                dataTitles.append(msgID)
            epoch = int(msg[0])
            data[msgID].append(list((epoch, int(msg[3]), data[msgID])))

            if epochSmol == -1 or epoch < epochSmol:
                epochSmol = epoch

        except KeyError as e:
            print("KeyError: ", e)
            pass

    for _, value in data.items():
        value.sort(key=lambda x: x[0])
        for lst in value:
            lst[0] -= epochSmol
            lst[0] /= 1000000000

    wb = openpyxl.Workbook()
    ws = wb.active

    ws.append(dataTitles)

    rows = list()

    def getRow():
        row = list()
        for title in dataTitles:
            column = data.get(title)
            if not column or len(column) == 0:
                row.append((epochSmol + 1, 0))
            else:
                row.append(column[0])

        smallestEpoch = epochSmol

        for item in row:
            if item[0] < smallestEpoch:
                smallestEpoch = item[0]

        if smallestEpoch == epochSmol:
            return

        finalRow = list([smallestEpoch])
        row.pop(0)

        for item in row:
            if item[0] != smallestEpoch:
                finalRow.append(item[1])
            else:
                finalRow.append(item[2].pop(0)[1])

        if len(finalRow) == 1:
            return

        return finalRow

    while True:
        row = getRow()
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

    for i in range(2, len(dataTitles) + 1):
        values = Reference(ws, min_col=i, min_row=1, max_row=len(rows) + 1)
        series = Series(values, xvalues, title_from_data=True)
        # series.marker.symbol = "circle"
        # series.marker.size = 3
        # series.graphicalProperties.line.noFill = True
        series.smooth = True
        series.graphicalProperties.line.width = 10000 # width in EMUs
        chart.series.append(series)

    ws.add_chart(chart, "A1")
    wb.save("{}.xlsx".format(SaveName))


def main():
    if len(sys.argv) == 1 or sys.argv[1][0:2] == "-h":
        usage()
        exit()

    if sys.argv[1][0:2] == "-l":
        Plist = listPastes()
        exit(Plist[1] if Plist[0] else "Failed to get pastes")

    if len(sys.argv) > 2 and sys.argv[2][0:2] == "-g":
        id = sys.argv[1][2:]
        # strId = sys.argv[2][2:]
        # print(id, strId)
        graph(getPaste(id))
        exit()

    if sys.argv[1][0:2] == "-p":
        id = sys.argv[1][2:]
        print(id)
        interpret(getPaste(id))
        exit()

    usage()


main()