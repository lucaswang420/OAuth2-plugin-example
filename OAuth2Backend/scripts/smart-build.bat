@echo off
setlocal enabledelayedexpansion

REM ========================================
REM Smart Build Script - Auto-detect Drogon version
REM ========================================

echo Checking Drogon library version...

REM Check if Drogon build directory exists
if not exist "D:\work\development\Repos\backend\drogon\build" (
    echo ERROR: Drogon build directory not found
    exit /b 1
)

REM Detect Drogon version by checking for Debug/Release directories
set "DROGON_BUILD="

if exist "D:\work\development\Repos\backend\drogon\build\Debug\drogon.lib" (
    set "DROGON_BUILD=Debug"
    echo Found: Drogon Debug version
) else if exist "D:\work\development\Repos\backend\drogon\build\Release\drogon.lib" (
    set "DROGON_BUILD=Release"
    echo Found: Drogon Release version
) else (
    echo ERROR: Cannot determine Drogon version
    echo Neither Debug nor Release library found
    exit /b 1
)

echo Configuring build for: !DROGON_BUILD!

REM Navigate to project directory
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%.."

REM Clean build directory
echo.
echo Cleaning build directory...
if exist build (
    rmdir /s /q build 2>nul
)

echo Creating build directory...
mkdir build
cd build

REM Check Conan
where conan >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: Conan is not installed or not in PATH
    cd ..
    exit /b 1
)

echo Initializing Conan profile...
conan profile detect --force
if %errorlevel% neq 0 (
    echo Error: Failed to initialize Conan profile
    cd ..
    exit /b 1
)

REM Install dependencies with detected build type
echo Installing dependencies with Conan for !DROGON_BUILD!...
conan install .. -s compiler="msvc" -s compiler.version=194 -s compiler.cppstd=20 -s build_type=!DROGON_BUILD! --output-folder . --build=missing
if %errorlevel% neq 0 (
    echo Error: Failed to install dependencies
    cd ..
    exit /b 1
)

REM Configure CMake
echo Configuring CMake for !DROGON_BUILD!...
cmake .. -DCMAKE_BUILD_TYPE=!DROGON_BUILD! -DCMAKE_CXX_STANDARD=20 -DCMAKE_TOOLCHAIN_FILE="conan_toolchain.cmake" -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
if %errorlevel% neq 0 (
    echo Error: CMake configuration failed
    cd ..
    exit /b 1
)

REM Build
echo Building !DROGON_BUILD! version...
cmake --build . --parallel --config !DROGON_BUILD!
if %errorlevel% neq 0 (
    echo Error: Build failed
    cd ..
    exit /b 1
)

REM Copy config files
echo Copying configuration files...
robocopy .. !DROGON_BUILD!\ config.json /NFL /NDL /NJH /NJS /NP
if %ERRORLEVEL% GEQ 8 (
  echo Error: copy failed
  cd ..
  exit /b 1
)
robocopy .. test\!DROGON_BUILD!\ config.json /NFL /NDL /NJH /NJS /NP
if %ERRORLEVEL% GEQ 8 (
  echo Error: copy failed
  cd ..
  exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo Configuration: !DROGON_BUILD!
echo ========================================

cd ..
