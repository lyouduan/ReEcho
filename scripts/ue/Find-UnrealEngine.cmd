@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Find-UnrealEngine.ps1" %*
exit /b %ERRORLEVEL%
