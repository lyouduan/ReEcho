# RenderDoc MCP on-demand policy

`renderdoc-mcp` is disabled in the user-level Codex configuration by default. Codex currently initializes every enabled STDIO MCP server when a task starts; it does not provide a native first-tool-call lazy-start switch.

## Use from Codex CLI

Start a temporary Codex session with RenderDoc enabled:

```powershell
scripts\mcp\Codex-With-RenderDoc.cmd
```

The wrapper applies only a process-level override:

```text
mcp_servers.renderdoc-mcp.enabled=true
```

It does not persistently enable the server.

## Use from Codex desktop

Open Settings -> MCP servers, enable `renderdoc-mcp`, then restart/start the task that needs capture analysis. Disable it again after the RenderDoc task and restart the task. The current task cannot hot-add a server whose tool catalog was absent at startup.

## Verification

```powershell
codex mcp get renderdoc-mcp
```

The normal result should be `renderdoc-mcp (disabled)`.
