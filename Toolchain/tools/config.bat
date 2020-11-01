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
if not exist build/build.ninja (
    set no_ninja_script=1
)

IF "%1"=="build" GOTO BUILD
IF "%1"=="upload" GOTO UPLOAD
IF "%1"=="config" (
    set only_config=1
    GOTO END_CLEAN
)
IF "%1"=="clean" (
    if "%no_ninja_script%"=="1" (
        echo Project is invalid ⛔
        echo Consider running config or hard_clean
        start /wait exit /b 1
        GOTO EXIT
    )
    cd build
    echo Cleaning 🧹
    %tool_path%ninja.exe clean
    if errorlevel 1 echo Error cleaning up build files ⛔
    GOTO EXIT
)
IF "%1"=="hard_clean" (
    if "%new_build_dir%"=="1" (
        set only_config=1
        GOTO END_CLEAN
    )
    echo Hard Cleaning 🧼🧽
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
if "%no_ninja_script%"=="1" (
    GOTO CLEAN
)
if "%new_build_dir%"=="1" (
    GOTO END_CLEAN
    :CLEAN
    rmdir /Q /S build
    timeout /t 1 /nobreak >NUL
    mkdir build
    timeout /t 1 /nobreak >NUL
    :END_CLEAN
    cd build
    echo Configuring CMake project ⚙️
    cmake .. -G Ninja
    if errorlevel 1 (
        echo Failed to configure cmake ⛔
        GOTO EXIT
    )
    if "%only_config%"=="1" GOTO EXIT
    cd ".."
)
cd build
echo Building ⏳
%tool_path%ninja.exe -j16
if errorlevel 1 (
    echo Ninja failed to build ⛔
    GOTO EXIT
)
echo Build Finished ☕
cd ".."
for /f "tokens=2 delims==" %%a in ('type build\CMakeCache.txt^|find "FINAL_OUTPUT_FILE:INTERNAL="') do (
    set FINAL_OUTPUT_FILE=%%a & goto :continueB
)
:continueB

if not exist "%FINAL_OUTPUT_FILE%" (
    GOTO BINARY_DOES_NOT_EXIST
) else (
    echo Ready to Upload ⏫
)
GOTO EXIT

:UPLOAD

if not exist build\CMakeCache.txt (
    echo CMake has not been configured ⛔
    start /wait exit /b 1
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
:BINARY_DOES_NOT_EXIST
    echo Final binary file was not found ⛔
    start /wait exit /b 1
    GOTO EXIT
)

if "%2"=="" (
    echo Warning! no port defined, unable to auto reboot
    set no_auto_reboot=1
)

if "%no_auto_reboot%"=="" ( 
    %tool_path%ComMonitor.exe "%2" 134 -c --priority
    timeout /t 1 > NUL
)

%tool_path%teensy_loader_cli.exe -mmcu=%TEENSY_CORE_NAME% -v %FINAL_OUTPUT_FILE%

if errorlevel 1 (
    echo Failed to upload once ⛔
    %tool_path%teensy_loader_cli.exe -mmcu=%TEENSY_CORE_NAME% -v %FINAL_OUTPUT_FILE%
    if errorlevel 1 (
        echo Failed to upload ⛔
        echo Is the teensy connected?
        GOTO EXIT
    )
)

echo Good to go 🦼
GOTO EXIT

:EXIT
if errorlevel 1 (
    echo.
    echo Task Failed ❌
) else (
    echo.
    echo Task Succeeded ✔️
)
cd %call_path%