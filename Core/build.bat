@echo off
REM Builds the engine-free rules core: its tests and its balance harness.
REM
REM No CMake and no project files on purpose. This is a handful of headers and
REM two entry points with zero dependencies, and it should stay something you
REM can build with one command and no ceremony.
setlocal
set VCVARS="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist %VCVARS% (
  echo Could not find vcvars64.bat. Install Visual Studio Build Tools with the
  echo "Desktop development with C++" workload.
  exit /b 1
)
call %VCVARS% >nul
cd /d "%~dp0"
if not exist build mkdir build

REM /W4 /WX: warnings are errors. This is rules code; a warning here is a bug.
set FLAGS=/std:c++20 /EHsc /O2 /W4 /WX /nologo /Isrc

echo Building tests...
cl %FLAGS% tests\Tests.cpp /Fe:build\tests.exe /Fo:build\ || exit /b 1

echo Building depth harness...
cl %FLAGS% tools\DepthHarness.cpp /Fe:build\depth.exe /Fo:build\ || exit /b 1

echo.
echo Built build\tests.exe and build\depth.exe
