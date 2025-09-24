@echo off
cls
echo ===============================
echo Rune Chess Engine - Build
echo ===============================

REM Prompt for version name
set /p VERSION=Enter version name (e.g., v1, test, beta): 

REM Compiler flags
set FLAGS=-std=c++17 -Ofast -march=native -flto -Wall -Wextra -g -DNDEBUG -pipe -fno-omit-frame-pointer -funroll-loops

REM Include directories
set INCLUDE=-Iinclude

REM Source files (all cpp in src recursively)
set SOURCES=
for /R src %%f in (*.cpp) do set SOURCES=!SOURCES! %%f

REM Output file
set OUTPUT=-o rune_%VERSION%.exe

echo Compiling...
g++ %FLAGS% %INCLUDE% src/core/*.cpp src/search/*.cpp src/storage/*.cpp src/tables/*.cpp src/utils/*.cpp src/main.cpp %OUTPUT%

REM Check build result
if %errorlevel% neq 0 (
    echo ❌ Build failed.
    exit /b %errorlevel%
) else (
    echo ✅ Build succeeded. Output: rune_%VERSION%.exe
)
