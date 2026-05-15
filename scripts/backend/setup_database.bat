@echo off
setlocal

call "%~dp0\env_common.bat"
if errorlevel 1 exit /b 1

REM Check for PostgreSQL client
where psql >nul 2>&1
if errorlevel 1 (
    echo [Error] psql not found in PATH.
    exit /b 1
)

set PROJECT_DIR=%~dp0..\..
set SQL_DIR=%PROJECT_DIR%\OAuth2Server\sql
echo Setting up oauth_test database...

set PGPASSWORD=123456
set PGCLIENTENCODING=UTF8

echo Dropping existing database...
psql -U test -d postgres -c "DROP DATABASE IF EXISTS oauth_test;" >nul 2>&1

echo Creating new database...
psql -U test -d postgres -c "CREATE DATABASE oauth_test;" >nul 2>&1

echo Applying SQL schemas from %SQL_DIR%...
for %%f in ("%SQL_DIR%\*.sql") do (
    echo Applying %%~nxf...
    psql -U test -d oauth_test -f "%%f"
    if errorlevel 1 (
        echo [Error] Failed to apply %%~nxf
        exit /b 1
    )
)

echo Database setup complete!
endlocal
exit /b 0
