@echo off

for /f "tokens=4-5 delims=. " %%i in ('ver') do set VERSION=%%i.%%j
if not "%version%" == "10.0" (
    echo Warning, only tested on Windows 10
    echo.
)

chcp 65001 >NUL
set ASCII27=
set A_RESET=%ASCII27%[0m
set A_BOLD=%ASCII27%[1m
set A_UNDER=%ASCII27%[4m
set A_BLACK=%ASCII27%[30m
set A_RED=%ASCII27%[31m
set A_GREEN=%ASCII27%[32m
set A_YELLOW=%ASCII27%[33m
set A_BLUE=%ASCII27%[34m
set A_MAGENTA=%ASCII27%[35m
set A_CYAN=%ASCII27%[36m
set A_WHITE=%ASCII27%[37m


set call_path=%CD%
set tool_path=%~dp0%
set only_config=0
set new_build_dir=0
if not exist build (
    mkdir build
    set new_build_dir=1
)
if not exist build/build.ninja (
    set no_ninja_script=1
)

set "option=%1"
set "COM_PORT=%2"

shift
set CMAKE_PARAMS=%1
:CMAKE_LOOP
shift
if [%1]==[] goto CMAKE_AFTERLOOP
set CMAKE_PARAMS=%CMAKE_PARAMS% %1
goto CMAKE_LOOP
:CMAKE_AFTERLOOP
set CMAKE_PARAMS=%CMAKE_PARAMS:\"=%

if "%option%"=="" GOTO HELP_STR

if "%option%"=="build" (
    echo %A_BOLD%%A_UNDER%Build Project%A_RESET%
    echo.
    GOTO BUILD
)
if "%option%"=="upload" (
    echo %A_BOLD%%A_UNDER%Upload Binary%A_RESET%
    echo.
    GOTO UPLOAD
)
if "%option%"=="config" (
    echo %A_BOLD%%A_UNDER%Configure Project%A_RESET%
    echo.
    set only_config=1
    GOTO END_CLEAN
)
if "%option%"=="clean" (
    if "%no_ninja_script%"=="1" (
        echo %A_BOLD%%A_RED%Project is invalid%A_RESET% ⛔
        echo Consider running config or hard_clean
        start /wait exit /b 1
        GOTO END_SCRIPT
    )
    cd build
    echo %A_BOLD%%A_UNDER%Cleaning%A_RESET% 🧹
    %tool_path%ninja.exe clean
    if errorlevel 1 echo %A_BOLD%%A_RED%Error cleaning up build files%A_RESET% ⛔
    GOTO END_SCRIPT
)
if "%option%"=="hard_clean" (
    if "%new_build_dir%"=="1" (
        set only_config=1
        GOTO END_CLEAN
    )
    echo %A_BOLD%%A_UNDER%Hard Cleaning%A_RESET% 🧼🧽
    set only_config=1
    GOTO BUILD_CLEAN
)

:HELP_STR

echo.
echo %A_UNDER%%A_BOLD%Valid options%A_RESET%
echo.
echo    %A_BOLD%build%A_RESET%              : Build project, configuring if necessary
echo    %A_BOLD%upload%A_RESET% [%A_YELLOW%com_port%A_RESET%]  : Upload binary file to a connected teensy
echo    %A_BOLD%clean%A_RESET%              : Cleanup build files
echo    %A_BOLD%hard_clean%A_RESET%         : Refresh project to a clean state, can pass
echo                         extra variables to auto config cmake
echo    %A_BOLD%config%A_RESET%             : Reconfigure cmake project, can pass any
echo                         extra variables for cmake
echo.
echo If a script is named %A_MAGENTA%`Pre_Build`%A_RESET%  and is at the root of a project
echo It is run before configuring CMake, It can be a %A_MAGENTA%`.bat`%A_RESET%, %A_MAGENTA%`.ps1`%A_RESET%, or %A_MAGENTA%`.py`%A_RESET%
echo Only one is run, prefering the file type is that order

exit /b 0

:BUILD

if "%no_ninja_script%"=="1" GOTO BUILD_CLEAN
if "%new_build_dir%"=="1" GOTO END_CLEAN



GOTO FINISH_CLEAN_SECTION
:BUILD_CLEAN
rmdir /Q /S build
timeout /t 1 /nobreak >NUL
mkdir build
timeout /t 1 /nobreak >NUL
:END_CLEAN

if exist Pre_Build.bat (
    echo %A_CYAN%%A_BOLD%Running Pre-Build Batch Script%A_RESET% 🧰
    echo.
    Start Pre_Build.bat
    GOTO __END_PREBUILD
)
if exist Pre_Build.ps1 ( 
    echo %A_CYAN%%A_BOLD%Running Pre-Build PowerShell Script%A_RESET% 🧰
    echo.
    powershell.exe .\Pre_Build.ps1
    GOTO __END_PREBUILD
)
if exist Pre_Build.py ( 
    echo %A_CYAN%%A_BOLD%Running Pre-Build Python Script%A_RESET% 🧰
    echo.
    Python.exe Pre_Build.py
    GOTO __END_PREBUILD
)
GOTO __NO_PREBUILD
:__END_PREBUILD
if errorlevel 1 (
    echo %A_BOLD%%A_RED%Pre_Build script failed%A_RESET% ⛔
    GOTO END_SCRIPT
)
:__NO_PREBUILD

cd build
echo.
echo %A_BOLD%Configuring CMake project%A_RESET% ⚙️
cmake .. -G Ninja %CMAKE_PARAMS%
if errorlevel 1 (
    echo %A_BOLD%%A_RED%Failed to configure cmake%A_RESET% ⛔
    GOTO END_SCRIPT
)
if "%only_config%"=="1" GOTO END_SCRIPT
cd ".."
:FINISH_CLEAN_SECTION

if exist Pre_Build.bat (
    echo %A_CYAN%%A_BOLD%Running Pre-Build Batch Script%A_RESET% 🧰
    echo.
    Start Pre_Build.bat
    GOTO END_PREBUILD
)
if exist Pre_Build.ps1 ( 
    echo %A_CYAN%%A_BOLD%Running Pre-Build PowerShell Script%A_RESET% 🧰
    echo.
    powershell.exe .\Pre_Build.ps1
    GOTO END_PREBUILD
)
if exist Pre_Build.py ( 
    echo %A_CYAN%%A_BOLD%Running Pre-Build Python Script%A_RESET% 🧰
    echo.
    Python.exe Pre_Build.py
    GOTO END_PREBUILD
)
GOTO NO_PREBUILD
:END_PREBUILD
if errorlevel 1 (
    echo %A_BOLD%%A_RED%Pre_Build script failed%A_RESET% ⛔
    GOTO END_SCRIPT
)
:NO_PREBUILD

cd build
echo %A_CYAN%%A_BOLD%Building%A_RESET% ⏳
%tool_path%ninja.exe -j16
if errorlevel 1 (
    echo %A_BOLD%%A_RED%Ninja failed to build%A_RESET% ⛔
    GOTO END_SCRIPT
)
echo.
echo %A_BOLD%%A_GREEN%Build Finished%A_RESET% ☕
cd ".."
for /f "tokens=2 delims==" %%a in ('type build\CMakeCache.txt^|find "FINAL_OUTPUT_FILE:INTERNAL="') do (
    set FINAL_OUTPUT_FILE=%%a & goto :continueB
)
:continueB

if not exist "%FINAL_OUTPUT_FILE%" (
    GOTO BINARY_DOES_NOT_EXIST
) else (
    echo.
    echo %A_BOLD%%A_BLUE%Ready to Upload%A_RESET% ⏫
)
GOTO END_SCRIPT

:UPLOAD

if not exist build\CMakeCache.txt (
    echo %A_BOLD%%A_RED%CMake has not been configured%A_RESET% ⛔
    start /wait exit /b 1
    GOTO END_SCRIPT
)

for /f "tokens=2 delims==" %%a in ('type build\CMakeCache.txt^|find "TEENSY_CORE_NAME:INTERNAL="') do (
    set TEENSY_CORE_NAME=%%a & goto :continue0
)
:continue0
for /f "tokens=2 delims==" %%a in ('type build\CMakeCache.txt^|find "FINAL_OUTPUT_FILE:INTERNAL="') do (
    set FINAL_OUTPUT_FILE=%%a & goto :continue1
)
:continue1

if not exist "%FINAL_OUTPUT_FILE%" (
:BINARY_DOES_NOT_EXIST
    echo %A_BOLD%%A_RED%Final binary file was not found%A_RESET% ⛔
    start /wait exit /b 1
    GOTO END_SCRIPT
)

if "%COM_PORT%"=="" (
    echo %A_YELLOW%Warning! no port defined, unable to auto reboot%A_RESET%
    set no_auto_reboot=1
)

if "%no_auto_reboot%"=="" ( 
    %tool_path%ComMonitor.exe "%COM_PORT%" 134 -c --priority
    timeout /t 1 > NUL
)

%tool_path%teensy_loader_cli.exe -mmcu=%TEENSY_CORE_NAME% -v %FINAL_OUTPUT_FILE%

if errorlevel 1 (
    echo %A_RED%Failed to upload once%A_RESET% ⛔
    %tool_path%teensy_loader_cli.exe -mmcu=%TEENSY_CORE_NAME% -v %FINAL_OUTPUT_FILE%
    if errorlevel 1 (
        echo %A_BOLD%%A_RED%Failed to upload%A_RESET% ⛔
        echo Is the teensy connected?
        GOTO END_SCRIPT
    )
)

echo %A_GREEN%Good to go 🦼%A_RESET%
GOTO END_SCRIPT

:END_SCRIPT
if errorlevel 1 (
    echo.
    echo %A_BOLD%%A_RED%Task Failed%A_RESET% ❌
    echo.
    cd %call_path%
    exit /b 1
) else (
    echo.
    echo %A_BOLD%%A_GREEN%Task Succeeded%A_RESET% ✔️
    cd %call_path%
    exit /b 0
)