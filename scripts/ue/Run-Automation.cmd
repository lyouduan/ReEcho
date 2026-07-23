@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Run-Automation.ps1" %*
exit /b %ERRORLEVEL%
