#pragma once

#include "Log.h"
#include "SD.h"
#include <TimeLib.h>
#include <cctype>
#include <unordered_map>

namespace SDIO {

bool initalizedSD;
File SDFile;

LOG_TAG ID = "SDCard";

bool initialize() {
    if (!initalizedSD) {
        if ((initalizedSD = SD.begin(BUILTIN_SDCARD))) {
            char filename[64], mapname[64], dirname[64];
            snprintf(dirname, sizeof(dirname), "%d", UNIX_TIME);
            snprintf(mapname, sizeof(mapname), "%s/map.json", dirname);

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

class StringPrint : public Print {
public:
    String buffer;
    std::unordered_map<int, std::pair<String, String>> options;

    virtual size_t
    write(uint8_t c) override {
        if (std::isdigit(c) || c == '\n')
            buffer += (char)c;
        return 1;
    }

    void loadOptions(bool includeMap = false) {
        int startIndex = 0, cnt = (1 + includeMap);
        if (includeMap) {
            Serial.println("[1] map.json");
        }
        while (startIndex < buffer.length()) {
            // Find the end of the current line
            int endIndex = buffer.indexOf('\n', startIndex);
            if (endIndex == -1) {
                // If there are no more newline characters, use the rest of the string as the line
                endIndex = buffer.length();
            }
            // Get the current line as a substring
            String line = buffer.substring(startIndex, endIndex);
            // Do something with the line
            if (line.length() > 6) {
                time_t unixTime = line.toInt(); // convert Unix timestamp string to int
                tmElements_t tm;                // create tmElements_t structure to store date and time information
                // convert Unix timestamp to tmElements_t structure
                breakTime(unixTime, tm);

                String date = String(tm.Month) + "/" + tm.Day + "/" + tmYearToCalendar(tm.Year) + " " + hourFormat12(unixTime) + ":" + tm.Minute + ":" + tm.Second;
                if (isAM(unixTime))
                    date += " AM";
                else
                    date += " PM";
                Serial.println(String("[") + cnt + "] " + date);
                options[cnt++] = std::pair<String, String>(line, date);
            }
            // Move to the start of the next line
            startIndex = endIndex + 1;
        }
    }
    void clear() {
        options.clear();
        buffer = "";
    }
};

void outputFile(String fileName) {
    if (!SD.sdfs.exists(fileName)) {
        Serial.println("Failed to locate file: " + fileName);
        return;
    }
    Serial.println("Outputing: " + fileName);
    FsFile selection = SD.sdfs.open(fileName);
    char buf[2 << 13];
    int read;

    do {
        if ((read = selection.readBytes(buf, 2 << 13)) > 0)
            Serial.write(buf, read);
    } while (read > 0);
}

void enterSDMode() {
    StringPrint sp;
    SD.sdfs.ls(&sp);
    Serial.println("Select Build (0 for exit):");
    sp.loadOptions();
    long select;
    do {
        while (!Serial.available()) {
        }
        select = Serial.readString().toInt();
        Serial.println(select);
        if (select == 0)
            return;
    } while (sp.options.find(select) == sp.options.end());
    Serial.println("Selected : " + sp.options[select].second);
    if (!SD.sdfs.chdir(sp.options[select].first)) {
        Serial.println("Failed to chdir, exiting...");
        return;
    }
    do {
        sp.clear();
        SD.sdfs.ls(&sp);
        Serial.println("Available Files:");
        sp.loadOptions(true);

        while (true) {
            Serial.println("\nSelect File (0 for exit):");
            while (!Serial.available()) {
            }
            select = Serial.readString().toInt();

            if (select == 1) {
                Serial.println("Selected : map.json");
                outputFile("map.json");
                break;
            } else if (sp.options.find(select) != sp.options.end()) {
                Serial.println("Selected : " + sp.options[select].second);
                outputFile(sp.options[select].first + "_SDLog.log");
                break;
            } else if (select == 0) {
                SD.sdfs.chdir('/');
                return;
            } else {
                Serial.println("Invalid option");
            }
        }

    } while (true);
}

void trySDMode() {
    if (Serial.available()) {
        if (Serial.readString().toLowerCase() == "sdcard") {
            enterSDMode();
        }
    }
}
} // namespace SDIO