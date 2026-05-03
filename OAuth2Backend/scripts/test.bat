@echo off
setlocal enabledelayedexpansion

REM ========================================
REM OAuth2 Test Script
REM ========================================

REM Store the script directory and change to parent (OAuth2Backend)
set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%.."
set PROJECT_DIR=%CD%
echo Current work directory is "%PROJECT_DIR%"
echo.

REM Default build type
set BUILD_TYPE=Release
set VERBOSE=--verbose

REM Parse command line arguments
:parse_args
if "%1"=="" goto end_parse
if /i "%1"=="-debug" (
    set BUILD_TYPE=Debug
    shift
    goto parse_args
)
if /i "%1"=="-release" (
    set BUILD_TYPE=Release
    shift
    goto parse_args
)
if /i "%1"=="-q" (
    set VERBOSE=
    shift
    goto parse_args
)
if /i "%1"=="-h" goto usage
if /i "%1"=="--help" goto usage
echo Unknown option: %1
goto usage

:usage
echo Usage: %0 [-debug^|-release] [-q] [-h]
echo   -debug     Test debug version
echo   -release   Test release version (default)
echo   -q         Quiet mode (disable verbose output)
echo   -h, --help Show this help message
exit /b 1

:end_parse

echo ========================================
echo Running OAuth2 Tests
echo ========================================
echo Build Type: %BUILD_TYPE%
echo.

REM Check if build directory exists
if not exist build (
    echo Error: Build directory not found!
    echo Please run build.bat first to compile the project.
    cd "%SCRIPT_DIR%"
    exit /b 1
)

cd build

REM ========================================
REM First Test Run: Default Configuration
REM ========================================
echo Running ctest for %BUILD_TYPE% configuration (First run with default config)...
echo.
ctest -C %BUILD_TYPE% --output-on-failure %VERBOSE%

set FIRST_TEST_RESULT=%errorlevel%

if %FIRST_TEST_RESULT% equ 0 (
    echo.
    echo ========================================
    echo First test run passed!
    echo ========================================
) else (
    echo.
    echo ========================================
    echo First test run failed!
    echo ========================================
)

echo.
echo ========================================
REM ========================================
REM Second Test Run: CI Configuration
REM ========================================
echo Preparing second test run with config.ci.json...
echo.

REM Backup existing config.json if it exists
set CONFIG_BACKUP=0
if exist config.json (
    echo Backing up existing config.json to config.json.backup...
    copy /Y config.json config.json.backup >nul
    set CONFIG_BACKUP=1
)

REM Copy config.ci.json from project root to build directory
if exist "%PROJECT_DIR%\config.ci.json" (
    echo Copying config.ci.json to config.json...
    copy /Y "%PROJECT_DIR%\config.ci.json" config.json >nul
    echo Second test run will use CI configuration.
    echo.
) else (
    echo Warning: config.ci.json not found in project root!
    echo Running second test with current configuration...
    echo.
)

echo Running ctest for %BUILD_TYPE% configuration (Second run with CI config)...
echo.
ctest -C %BUILD_TYPE% --output-on-failure %VERBOSE%

set SECOND_TEST_RESULT=%errorlevel%

REM Restore original config.json if backup was created
if %CONFIG_BACKUP% equ 1 (
    echo.
    echo Restoring original config.json...
    copy /Y config.json.backup config.json >nul
    del config.json.backup
)

echo.
echo ========================================
echo Test Summary
echo ========================================
echo First run (default config):  %FIRST_TEST_RESULT%
echo Second run (CI config):       %SECOND_TEST_RESULT%
echo ========================================

REM Exit with error if either test failed
if %FIRST_TEST_RESULT% neq 0 (
    cd "%SCRIPT_DIR%"
    exit /b 1
)
if %SECOND_TEST_RESULT% neq 0 (
    cd "%SCRIPT_DIR%"
    exit /b 1
)

echo.
echo ========================================
echo Both test runs passed!
echo ========================================

cd "%SCRIPT_DIR%"
endlocal
