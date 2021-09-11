@echo off
set FRONT=%1
set BACK=%2
set BAUD=%3
set MODE=%4

set CALL0=..\TeensyToolchain\tools\ComMonitor.exe %BACK% %BAUD% -t%MODE% -r -w -m8 --jsonPath log_lookup.json
set CALL1=..\TeensyToolchain\tools\ComMonitor.exe %FRONT% %BAUD% -t%MODE% -r -w -m8 --jsonPath log_lookup.json

start cmd.exe /d /c %CALL0%
start cmd.exe /d /c %CALL1%