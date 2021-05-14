"""
@file qrJSON
@author IR
@brief Generate a QR Gif of log_lookup.json to upload to the android app with no connection
@version 0.1
@date 2021-05-14

@copyright Copyright (c) 2021
"""

# @cond
import qrcode, zlib, base64
from PIL import Image, ImageDraw, ImageFont

maxVersion = 1
rawSize = -1
compSize = -1
count = -1


def bytesPrepend(data: bytes, byteIterable):
    data = list(data)
    byteIterable = list(byteIterable)
    byteIterable.reverse()
    for b in byteIterable:
        data.insert(0, b)
    data = bytes(data)
    return data


def makeChunk(data, num):
    global maxVersion
    qr = qrcode.QRCode(
        version=maxVersion,
        error_correction=qrcode.ERROR_CORRECT_H,
        box_size=10,
        border=10,
    )
    data = bytesPrepend(data, [count, num])
    data = base64.b64encode(data)

    qr.add_data(data)
    qr.make(fit=True)
    if maxVersion == 1:
        maxVersion = qr.version

    img = qr.make_image(fill_color="black", back_color="white")
    return img


data = None

with open("log_lookup.json", "r") as json:
    data = json.read()

data = bytes(data, "UTF-8")
rawSize = len(data)
data = zlib.compress(data, 9)
data = bytesPrepend(data, rawSize.to_bytes(4, "little"))
compSize = len(data)

data = list(data[i : i + 200] for i in range(0, len(data), 200))
count = len(data) - 1

i = 0

images = list()

fnt = ImageFont.truetype("arial", 30)

for d in data:
    img = makeChunk(d, i).get_image().convert("RGBA")
    draw = ImageDraw.Draw(img)
    draw.text((20, 20), str(len(images)).rjust(2, ' '), font=fnt, fill="black")
    images.append(img)
    i += 1

img = Image.new("P", (images[0].height, images[0].width), "black")
draw = ImageDraw.Draw(img)
draw.text((50, 50), "QR GIF\nQR VERSION: {}\nCOUNT: {}".format(maxVersion, count), font=ImageFont.truetype("arial", 40), fill="white")
images.insert(0, img)

images[0].save("log_lookup.gif", append_images=images[1:], save_all=True, duration=100, loop=0)

print(
    "QR Gif\nVersion {}\nBytes {} / {} : {}%\nWait for #{}".format(
        maxVersion, compSize, rawSize, round((compSize / rawSize) * 100, 2), count
    )
)
# @endcond