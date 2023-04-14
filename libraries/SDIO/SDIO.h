#pragma once

#include "Log.h"
#include "SD.h"

namespace SDIO {

bool initalizedSD;
File SDFile;

LOG_TAG ID = "SDCard";

bool initalize() {
    if (!initalizedSD) {
        if ((initalizedSD = SD.begin(BUILTIN_SDCARD))) {
            char filename[64], mapname[64], dirname[64];
            snprintf(dirname, sizeof(dirname), "%d", UNIX_TIME);
            snprintf(mapname, sizeof(mapname), "%s/%d_Map.json", dirname, UNIX_TIME);

            if (!SD.exists(dirname)) {
                Log.d(ID, "Creating new dir");
                if (SD.mkdir(dirname)) {
                    Log.d(ID, "New dir made");
                } else {
                    Log.e(ID, "Failed to make new dir");
                    return initalizedSD = false;
                }
            }

            Log.d(ID, "using dir", UNIX_TIME);

            if (!SD.exists(mapname)) {
                Log.d(ID, "Creating new map");
                File SDMap = SD.open(mapname, FILE_WRITE_BEGIN);
                SDMap.write(log_lookup, log_lookup_len);
                SDMap.flush();
                SDMap.close();
                Log.d(ID, "Map created");
            }

            Log.d(ID, "using map", UNIX_TIME);

            unsigned long off = 0;

            Log.d(ID, "Generating new log name");

            do {
                snprintf(filename, sizeof(filename), "%s/%d_SDLog.log", dirname, UNIX_TIME + off);
            } while (SD.exists(filename) && ++off != 0);

            Log.d(ID, "Opening new log", UNIX_TIME + off);

            SDFile = SD.open(filename, FILE_WRITE_BEGIN);
            Log.i(ID, "SD Card opened", UNIX_TIME);
            return true;
        }
        Log.e(ID, "SD Card failed to open");
        return false;
    }
    Log.i(ID, "SD Card opened");
    return true;
}
} // namespace SDIO