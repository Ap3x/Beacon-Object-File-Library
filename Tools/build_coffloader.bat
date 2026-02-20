@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

echo === Building COFFLoader64.exe (x64) ===
cl /W3 /DCOFF_STANDALONE /D_CRT_SECURE_NO_WARNINGS COFFLoader\beacon_compatibility.c COFFLoader\COFFLoader.c /Fe:COFFLoader\COFFLoader64.exe /link Advapi32.lib
if %ERRORLEVEL% NEQ 0 (
    echo BUILD FAILED
    exit /b 1
)

del /Q *.obj 2>nul

echo.
echo === Build complete ===
echo Output: COFFLoader\COFFLoader64.exe
