"""
@file uploadPaste.py
@author IR
@brief 'Update' pastebin of log_lookup.json for easy download by android app
@version 0.1
@date 2021-03-23

@copyright Copyright (c) 2021
"""

import base64
from pip._vendor import requests
import xml.etree.ElementTree as ET
import json
from time import sleep

API = str(base64.b64decode("NEFYcnlGRHBZWjFxUkhBX2FEUS0wTzZtTmRoNldMSjQ==="), "utf-8")
USER = str(base64.b64decode("MTlhZTQwNDg3OTAyYjljMDRjMjRmYjUyNzZmNmFmZDc="), "utf-8")


def deletePaste(paste_key):
    sleep(1)
    listParams = {
        "api_dev_key": API,
        "api_user_key": USER,
        "api_paste_key": paste_key,
        "api_option": "delete",
    }
    req = requests.post("https://pastebin.com/api/api_post.php", data=listParams)
    print(req.status_code, req.reason, req.text)


JsonFile = ""

try:
    with open("log_lookup.json", "r", encoding="utf8") as f:
        JsonFile = f.read()
except FileNotFoundError:
    exit("lookup file was not found")

listParams = {
    "api_dev_key": API,
    "api_user_key": USER,
    "api_option": "list",
}

req = requests.post("https://pastebin.com/api/api_post.php", data=listParams)
XMLlist = "<pastes>\n" + req.text + "\n</pastes>"
print(req.status_code, req.reason)

if XMLlist != "No pastes found.":
    xml = ET.fromstring(XMLlist)
    for paste in xml.iter("paste_key"):
        deletePaste(paste.text)
        print("Delete:", paste.text)

params = {
    "api_paste_name": "JsonMap",
    "api_paste_private": "0",
    "api_dev_key": API,
    "api_user_key": USER,
    "api_option": "paste",
    "api_paste_code": JsonFile,
}
req = requests.post("https://pastebin.com/api/api_post.php", data=params)
print(req.status_code, req.reason, req.text)