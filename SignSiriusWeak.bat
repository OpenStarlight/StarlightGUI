@echo off
setlocal

set "TARGET=%~1"
set "PFX=%~dp0SiriusLocalCodeSigningWeak.pfx"
set "PFX_PASSWORD=SiriusWeakCallerSigningPassword"

if "%TARGET%"=="" (
    echo [SiriusSign] No target was specified, skip signing.
    exit /b 0
)

if not exist "%TARGET%" (
    echo [SiriusSign] Target does not exist: "%TARGET%"
    exit /b 0
)

if not exist "%PFX%" (
    echo [SiriusSign] PFX file was not found, skip signing.
    exit /b 0
)

if "%PFX_PASSWORD%"=="FILL_YOUR_PFX_PASSWORD_HERE" (
    echo [SiriusSign] PFX password is not set, skip signing.
    exit /b 0
)

signtool sign /fd SHA256 /f "%PFX%" /p "%PFX_PASSWORD%" "%TARGET%"
if errorlevel 1 (
    echo [SiriusSign] SignTool failed, skip signing.
    exit /b 0
)

exit /b 0
