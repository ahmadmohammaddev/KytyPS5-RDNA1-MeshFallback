@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Run-Prototype.ps1" -Mode Strict -RequireRx5700Xt -Ir %*
set "run_result=%errorlevel%"
echo Exit %run_result%: 0=PASS, 41=SKIPPED, other=FAIL. Logs are outside this package.
pause
exit /b %run_result%
