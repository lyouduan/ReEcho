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
4. When MCP access is needed, run `ModelContextProtocol.StartServer 8000` in the UE console and wait for the Output Log to report that it is listening.
5. Restart/open a Codex task from this workspace so project MCP configuration is loaded.
6. Before packaging from an open editor, run `ModelContextProtocol.StopServer`; command-line packaging should run with the editor closed.

MCP auto-start is intentionally disabled. Cook commandlets load editor settings too, so auto-starting the server there can collide with the editor-owned port and turn an otherwise successful Cook into `Unknown Cook Failure`.

The server serializes tool execution onto the UE game thread. Do not issue overlapping mutation calls. Local `.uasset`, `.umap`, generated DataTables, and Project Settings edits may be organized freely, but competing binary/config candidates must be compared explicitly at the remote integration boundary and never silently overwrite another contributor's change.

## Diagnostics

- Port check: `Get-NetTCPConnection -LocalPort 8000 -State Listen`.
- UE log: search `Saved/Logs/ReEcho.log` for `ModelContextProtocol`.
- If port 8000 is occupied, change both `DefaultEditorPerProjectUserSettings.ini` and `.codex/config.toml` to the same free port.
