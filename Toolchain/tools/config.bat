@echo off
chcp 65001 >NUL
set call_path=%CD%
set tool_path=%~dp0%
set only_config=0
set new_build_dir=0
if not exist build (
    mkdir build
    set new_build_dir=1
)

IF "%1"=="build" GOTO BUILD
IF "%1"=="upload" GOTO UPLOAD
IF "%1"=="config" (
    set only_config=1
    GOTO END_CLEAN
)
IF "%1"=="clean" (
    if "%new_build_dir%"=="1" (
        echo Nothing to clean
        GOTO EXIT
    )
    cd build
    echo Cleaning 🧹
    %tool_path%ninja.exe clean
    if not %errorlevel%==0 echo Error cleaning up build files ⛔
    GOTO EXIT
)
IF "%1"=="hard_clean" (
    if "%new_build_dir%"=="1" (
        echo Nothing to clean
        GOTO EXIT
    )
    echo Cleaning 🧹
    set only_config=1
    GOTO CLEAN
)

echo Valid options :
echo build              : Build project, configuring if necessary
echo upload [com_port]  : Upload binary file to a connected teensy
echo clean              : Cleanup build files
echo hard_clean         : Refresh project to a clean state, will auto config cmake
echo config             : Reconfigure cmake project

GOTO EXIT

:BUILD
if "%new_build_dir%"=="1" (
    GOTO END_CLEAN
    :CLEAN
    rmdir /Q /S build
    timeout /t 1 /nobreak >NUL
    mkdir build
    timeout /t 1 /nobreak >NUL
    :END_CLEAN
    cd build
    echo Configuring CMake project ⌚
    cmake .. -G Ninja
    if not %errorlevel%==0 (
        echo Failed to configure cmake ⛔
        GOTO EXIT
    )
    if "%only_config%"=="1" GOTO EXIT
    cd ".."
)
cd build
echo Building ⏳
%tool_path%ninja.exe -j16
if not %errorlevel%==0 (
    echo Ninja failed to build ⛔
    GOTO EXIT
)
echo Build Finished ☕
GOTO EXIT

:UPLOAD

if not exist build\CMakeCache.txt (
    echo CMake has not been configured ⛔
    GOTO EXIT
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
    echo Final binary file has not been built ⛔
    GOTO EXIT
)

if "%2"=="" (
    echo Warning! no port defined, unable to auto reboot
    set no_auto_reboot=1
)

if "%no_auto_reboot%"=="" %tool_path%ComMonitor.exe "%2" 134 -c --priority
timeout /t 1 > NUL
%tool_path%teensy_loader_cli.exe -mmcu=%TEENSY_CORE_NAME% -v %FINAL_OUTPUT_FILE%

if not %errorlevel%==0 (
    echo Failed to upload once ⛔
    if "%no_auto_reboot%"=="" %tool_path%ComMonitor.exe "%2" 134 -c --priority
    timeout /t 1 > NUL
    %tool_path%teensy_loader_cli.exe -mmcu=%TEENSY_CORE_NAME% -v %FINAL_OUTPUT_FILE%
    @REM if not %errorlevel%==0 (
    @REM     echo Failed to upload ⛔
    @REM     echo Is the teensy connected?
    @REM     GOTO EXIT
    @REM )
)

echo Good to go 🦼
GOTO EXIT

:EXIT
cd %call_path%
echo Task finished ✅