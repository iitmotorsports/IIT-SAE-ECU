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
#include "FsGetPartitionInfo.h"
namespace FsGetPartitionInfo {
  static const uint8_t mbdpGuid[16] PROGMEM = {0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x33, 0x44, 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7};

  voltype_t getPartitionInfo(BlockDeviceInterface *blockDev, uint8_t part, uint8_t *secBuf,
      uint32_t *pfirstLBA, uint32_t *psectorCount, uint32_t *pmbrLBA, uint8_t *pmbrPart) {

    //Serial.printf("PFsLib::getPartitionInfo(%x, %u)\n", (uint32_t)blockDev, part);
    uint32_t firstLBA;
    uint32_t sectorCount;
    MbrSector_t *mbr;
    MbrPart_t *mp;

    if (!part || !pfirstLBA) return INVALID_VOL; // won't handle this here.
    part--; // zero base it.

    if (!blockDev->readSector(0, secBuf)) return INVALID_VOL;
    mbr = reinterpret_cast<MbrSector_t*>(secBuf);

    // First check for GPT vs MBR
    mp = &mbr->part[0];
    if (mp->type == 0xee) {
      // This is a GPT initialized Disk assume validation done earlier.
      if (!blockDev->readSector(1, secBuf)) return INVALID_VOL; 
      GPTPartitionHeader_t* gptph = reinterpret_cast<GPTPartitionHeader_t*>(secBuf);
      // Lets do a little validation of this data.
      if (!gptph || (memcmp(gptph->signature, F("EFI PART"), 8) != 0))  return INVALID_VOL;
      uint32_t numberPartitions = getLe32(gptph->numberPartitions);
      if (part > numberPartitions)  return INVALID_VOL;

      // We will overload the mbr part to give clue where GPT data is stored for this volume
      uint32_t mbrLBA = 2 + (part >> 2);
      uint8_t mbrPart = part & 0x3;
      if (pmbrLBA) *pmbrLBA = mbrLBA;
      if (pmbrPart) *pmbrPart =mbrPart;
      if (!blockDev->readSector(mbrLBA, secBuf)) return INVALID_VOL; 
      GPTPartitionEntrySector_t *gptes = reinterpret_cast<GPTPartitionEntrySector_t*>(secBuf);
      GPTPartitionEntryItem_t *gptei = &gptes->items[mbrPart];

      // Mow extract the data...
      firstLBA = getLe64(gptei->firstLBA);
      sectorCount = 1 + getLe64(gptei->lastLBA) - getLe64(gptei->firstLBA);
      if ((firstLBA == 0) && (sectorCount == 1)) return INVALID_VOL;
      
      *pfirstLBA = firstLBA;
      if (psectorCount) *psectorCount = sectorCount;

      if (memcmp((uint8_t *)gptei->partitionTypeGUID, mbdpGuid, 16) != 0) return OTHER_VOL;
      return GPT_VOL; 
    }
    // So we are now looking a MBR type setups. 
    // Extended support we need to walk through the partitions to see if there is an extended partition
    // that we need to walk into. 
    // short cut:
    if (part < 4) {
      // try quick way through
      mp = &mbr->part[part];
      if (((mp->boot == 0) || (mp->boot == 0X80)) && (mp->type != 0) && (mp->type != 0xf)) {
        *pfirstLBA = getLe32(mp->relativeSectors);
        if (psectorCount) *psectorCount = getLe32(mp->totalSectors);
        if (pmbrLBA) *pmbrLBA = 0;
        if (pmbrPart) *pmbrPart = part; // zero based. 
        return MBR_VOL;
      }
    }  

    // So must be extended or invalid.
    uint8_t index_part;
    for (index_part = 0; index_part < 4; index_part++) {
      mp = &mbr->part[index_part];
      if ((mp->boot != 0 && mp->boot != 0X80) || mp->type == 0 || index_part > part) return INVALID_VOL;
      if (mp->type == 0xf) break;
    }

    if (index_part == 4) return INVALID_VOL; // no extended partition found. 

    // Our partition if it exists is in extended partition. 
    uint32_t next_mbr = getLe32(mp->relativeSectors);
    for(;;) {
      if (!blockDev->readSector(next_mbr, secBuf)) return INVALID_VOL;
      mbr = reinterpret_cast<MbrSector_t*>(secBuf);

      if (index_part == part) break; // should be at that entry
      // else we need to see if it points to others...
      mp = &mbr->part[1];
      uint32_t  relSec = getLe32(mp->relativeSectors);
      //Serial.printf("    Check for next: type: %u start:%u\n ", mp->type, volumeStartSector);
      if ((mp->type == 5) && relSec) {
        next_mbr = next_mbr + relSec;
        index_part++; 
      } else return INVALID_VOL;
    }
   
    // If we are here than we should hopefully be at start of segment...
    mp = &mbr->part[0];
    *pfirstLBA = getLe32(mp->relativeSectors) + next_mbr;
    if (psectorCount) *psectorCount = getLe32(mp->totalSectors);
    if (pmbrLBA) *pmbrLBA = next_mbr;
    if (pmbrPart) *pmbrPart = 0; // zero based. 
    return EXT_VOL;
  }

}  // namespace FsUtf

