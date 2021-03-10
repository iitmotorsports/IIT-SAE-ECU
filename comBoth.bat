@echo off
set BACK=COM3
set FRONT=COM4
set BAUD=115200
set MODE=Hex

set CALL0=..\TeensyToolchain\tools\ComMonitor.exe %BACK% %BAUD% -t%MODE% -r -w -m8 --jsonPath log_lookup.json
set CALL1=..\TeensyToolchain\tools\ComMonitor.exe %FRONT% %BAUD% -t%MODE% -r -w -m8 --jsonPath log_lookup.json

start cmd.exe /d /c %CALL0%
start cmd.exe /d /c %CALL1%