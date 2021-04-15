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
import xml.etree.ElementTree as ET
import json
from time import sleep
import asyncio
import sys
import math

from matplotlib import pyplot as plt
import numpy as np

GETURL = "https://api.paste.ee/v1/pastes/{}"
API = str(base64.b64decode("dTBXUXZabUNsdVFkZWJycUlUNjZSRHJoR1paTlVXaXE3U09LTVlPUE8="), "UTF-8")
headers = {"X-Auth-Token": API}
BeginJSON = "---[ LOG MAP START ]---"
EndJSON = "---[ LOG MAP END ]---"


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
    return json.loads(req.text)["paste"]["sections"][0]["contents"]


def usage():
    print("List available pastes: \t-l\nInterpret paste: \t-p[PasteID]\Graph paste: \t-p[PasteID] -g[StringValue]")


def interpret(string: str):
    jsonOBJ = json.loads(string[string.find(BeginJSON) + len(BeginJSON) : string.find(EndJSON)])
    dataStream = string[string.find(EndJSON) + len(EndJSON) :]
    TagDict = {v: k for k, v in jsonOBJ[0].items()}
    StrDict = {v: k for k, v in jsonOBJ[1].items()}

    with open("interpret.log", "w") as logFile:
        logFile.writelines(string[string.find(BeginJSON) + len(BeginJSON) : string.find(EndJSON)])
        for line in dataStream.splitlines():
            msg = line.split(" ")
            if len(msg) != 3:
                continue
            logFile.write(TagDict[int(msg[0])] + StrDict[int(msg[1])] + " " + str(int(msg[2])) + "\n")


def graph(value: str, string: str):

    jsonOBJ = json.loads(string[string.find(BeginJSON) + len(BeginJSON) : string.find(EndJSON)])
    dataStream = string[string.find(EndJSON) + len(EndJSON) :]
    strKey = str(jsonOBJ[1][value])

    y = []

    for line in dataStream.splitlines():
        msg = line.split(" ")
        if len(msg) != 3 or msg[1] != strKey:
            continue
        y.append(int(msg[2]))

    x = np.linspace(0, 1, len(y))

    plt.plot(x, y, linewidth=1, color=(0, 0, 0, 0.5))
    plt.plot(x, y, linewidth=0.125, color=(1, 0, 0))

    plt.xlabel("Time Step")
    plt.ylabel("Value")
    plt.title(value)
    plt.savefig("interpret.png", dpi=2048)


def main():
    if len(sys.argv) == 1 or sys.argv[1][0:2] == "-h":
        usage()
        exit()

    if sys.argv[1][0:2] == "-l":
        Plist = listPastes()
        exit(Plist[1] if Plist[0] else "Failed to get pastes")

    if len(sys.argv) > 2 and sys.argv[2][0:2] == "-g":
        id = sys.argv[1][2:]
        strId = sys.argv[2][2:]
        print(id, strId)
        graph(strId, getPaste(id))
        exit()

    if sys.argv[1][0:2] == "-p":
        id = sys.argv[1][2:]
        print(id)
        interpret(getPaste(id))
        exit()

    usage()


main()
