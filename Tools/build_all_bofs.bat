@echo off
setlocal enabledelayedexpansion
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

if not exist "..\x64\Release" mkdir "..\x64\Release"

set CFLAGS=/c /GS- /std:c++20 /D_HAS_EXCEPTIONS=0 /GR-
set FAILED=0

for %%P in (EnumDeviceDrivers GetSystemDirectory Ipconfig RegistryPersistence TimeStomp WhoAmI FileExfiltrationUrlEncoded) do (
    echo === Building %%P ===
    cl %CFLAGS% /Fo:%%P.obj ..\%%P\bof.cpp
    if !ERRORLEVEL! NEQ 0 (
        echo FAILED: %%P
        set FAILED=1
    ) else (
        copy /y %%P.obj ..\x64\Release\%%P.x64.o >nul
        del %%P.obj
    )
)

if !FAILED! EQU 1 (
    echo.
    echo === Some builds failed ===
    exit /b 1
) else (
    echo.
    echo === All builds succeeded ===
    dir /b ..\x64\Release\*.o
)
