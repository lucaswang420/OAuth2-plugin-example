@echo off
setlocal

call "%~dp0\env_common.bat"
if errorlevel 1 exit /b 1

REM Check for drogon_ctl
where drogon_ctl >nul 2>&1
if errorlevel 1 (
    echo [Error] drogon_ctl not found in PATH.
    exit /b 1
)

set PROJECT_DIR=%~dp0..\..
set MODELS_SRC_DIR=%PROJECT_DIR%\OAuth2Plugin\src\models
set MODELS_INC_DIR=%PROJECT_DIR%\OAuth2Plugin\include\oauth2\models
set MODELS_BACKUP=%PROJECT_DIR%\OAuth2Plugin\models_backup
set MODEL_JSON_DIR=%PROJECT_DIR%\OAuth2Server

echo.
echo ========================================
echo OAuth2 Plugin Model Generation
echo ========================================
echo.

REM Parse arguments
set AUTO_MODE=0
if "%1"=="-y" set AUTO_MODE=1
if "%1"=="--force" set AUTO_MODE=1

if %AUTO_MODE%==0 (
  echo WARNING: This will regenerate ORM models in %MODELS_SRC_DIR%
  pause
)

REM Backup existing models
if exist "%MODELS_SRC_DIR%" (
  echo Backing up existing models to %MODELS_BACKUP%...
  if exist "%MODELS_BACKUP%" rmdir /s /q "%MODELS_BACKUP%"
  mkdir "%MODELS_BACKUP%"
  xcopy /e /i /y "%MODELS_SRC_DIR%" "%MODELS_BACKUP%" >nul
  if exist "%MODELS_INC_DIR%\*.h" xcopy /y "%MODELS_INC_DIR%\*.h" "%MODELS_BACKUP%" >nul
)

echo Generating ORM models...
if not exist "%MODELS_SRC_DIR%" mkdir "%MODELS_SRC_DIR%"

cd /d "%MODEL_JSON_DIR%"
if %AUTO_MODE%==1 (
  echo y | drogon_ctl create model "../OAuth2Plugin/src/models"
) else (
  drogon_ctl create model "../OAuth2Plugin/src/models"
)

if errorlevel 1 (
  echo [Error] Model generation failed.
  exit /b 1
)

echo Moving header files to %MODELS_INC_DIR%...
if not exist "%MODELS_INC_DIR%" mkdir "%MODELS_INC_DIR%"
REM Remove old headers first to ensure no stale headers remain
del /q "%MODELS_INC_DIR%\*.h" >nul 2>&1
move /y "%MODELS_SRC_DIR%\*.h" "%MODELS_INC_DIR%\" >nul

echo.
echo ========================================
echo Model generation complete!
echo ========================================
endlocal
exit /b 0
