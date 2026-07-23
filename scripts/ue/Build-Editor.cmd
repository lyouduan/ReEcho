@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Build-Editor.ps1" %*
exit /b %ERRORLEVEL%
