/**
 * Copyright (c) 2011-2020 Bill Greiman
 * This file is part of the SdFat library for SD memory cards.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef FsGetPartitionInfo_h
#define FsGetPartitionInfo_h
/**
* \file
* \brief Unicode Transformation Format functions.
*/
#include <stdint.h>
#include <stddef.h>
#include "FsStructs.h"
#include "BlockDeviceInterface.h"

namespace FsGetPartitionInfo {

typedef enum {INVALID_VOL=0, MBR_VOL, EXT_VOL, GPT_VOL, OTHER_VOL} voltype_t; // what type of volume did the mapping return
voltype_t getPartitionInfo(BlockDeviceInterface *blockDev, uint8_t part, uint8_t *secBuf,
      uint32_t *pfirstLBA, uint32_t *psectorCount=nullptr, uint32_t *pmbrLBA=nullptr, uint8_t *pmbrPart=nullptr);

}  // namespace FsGetPartitionInfo
#endif  // FsGetPartitionInfo_h
