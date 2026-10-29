@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Run-Prototype.ps1" -Mode Strict -CombinedMilestone -RequireRx5700Xt %*
exit /b %errorlevel%
