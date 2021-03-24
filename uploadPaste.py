"""
@file uploadPaste.py
@author IR
@brief 'Update' paste.ee of log_lookup.json for easy download by android app
@version 0.1
@date 2021-03-23

@copyright Copyright (c) 2021
"""

import base64
from pip._vendor import requests
import xml.etree.ElementTree as ET
import json
from time import sleep
import asyncio

DEV = str(base64.b64decode("YTVib2o4bk1NS2w0ZERBazRoT1hrdTRWYkJPNXExUW9HUkJoM0RRSWY="), "UTF-8")
API = str(base64.b64decode("dVE4NWZCOVVLanRhSnFBazlKVEExaGVVc3J2QURnZVBIejc5RXhKMlo="), "UTF-8")
headers = {"X-Auth-Token": API}


async def listPastes():
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


async def deletePaste(paste_id):
    req = requests.delete("https://api.paste.ee/v1/pastes/{}".format(paste_id), headers=headers)
    print(req.status_code, req.reason, req.text.replace('{"success":true}', "Deleted"))


async def uploadFile():
    JsonFile = ""
    try:
        with open("log_lookup.json", "r", encoding="utf8") as f:
            JsonFile = f.read()
    except FileNotFoundError:
        print("lookup file was not found")
        return False

    payload = {"sections": [{"name": "JSONMap", "syntax": "json", "contents": JsonFile}]}
    req = requests.post("https://api.paste.ee/v1/pastes", headers=headers, json=payload)
    print(req.status_code, req.reason)
    status = int(req.status_code / 100) == 2
    idVal = ""
    if status:  # get ID
        try:
            idVal = json.loads(req.text)["id"]
        except:
            status = False

    return (status, idVal)  # 2XX code


async def main():
    pasteList = await listPastes()
    status = await uploadFile()

    if not status[0] or not pasteList[0]:  # unable to upload file
        exit()

    for pasteID in pasteList[1]:
        await deletePaste(pasteID)


if __name__ == "__main__":
    asyncio.run(main())