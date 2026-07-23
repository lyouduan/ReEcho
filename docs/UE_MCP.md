# UE 5.8 MCP integration

ReEcho uses the official experimental MCP server included with the installed UE 5.8 release build.

## Components

- `ModelContextProtocol`: HTTP MCP server inside Unreal Editor.
- `AllToolsets`: exposes Epic's editor toolsets through MCP.
- `.codex/config.toml`: project-scoped Codex client connection.
- Endpoint: `http://127.0.0.1:8000/mcp` using Streamable HTTP.

## Usage

1. Close every ReEcho editor instance before compiling C++.
2. Build with `scripts/ue/Build-Editor.cmd`.
3. Open `ReEcho.uproject` using the UE 5.8 release installation.
4. Wait until the Output Log reports the MCP server listening on port 8000.
5. Restart/open a Codex task from this workspace so project MCP configuration is loaded.

The server serializes tool execution onto the UE game thread. Do not issue overlapping mutation calls. `.uasset`, `.umap`, generated DataTables, and Project Settings remain serially owned resources and must be claimed in `shared/PLANNER_EXCHANGE.md`.

## Diagnostics

- Port check: `Get-NetTCPConnection -LocalPort 8000 -State Listen`.
- UE log: search `Saved/Logs/ReEcho.log` for `ModelContextProtocol`.
- If port 8000 is occupied, change both `DefaultEditorPerProjectUserSettings.ini` and `.codex/config.toml` to the same free port.
