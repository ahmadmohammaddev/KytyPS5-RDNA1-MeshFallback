@echo off
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Run-Prototype.ps1" -Mode Strict -RequireRx5700Xt -IrAdd %*
exit /b %errorlevel%
