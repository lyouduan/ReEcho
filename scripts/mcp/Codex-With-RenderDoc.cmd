@echo off
codex -c "mcp_servers.renderdoc-mcp.enabled=true" %*
exit /b %ERRORLEVEL%
