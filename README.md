# ReEcho

UE 5.8 installed-build graybox prototype for the Time Echo vertical slice.

## Start here

- AI entry and first-contact role question: [`AGENTS.md`](AGENTS.md)
- AI/code retrieval map: [`shared/CODEBASE_MAP.md`](shared/CODEBASE_MAP.md)
- Project rules: [`shared/PROJECT_RULES.md`](shared/PROJECT_RULES.md)
- Professional routes: [`PROGRAMMER_RULES.md`](shared/PROGRAMMER_RULES.md), [`DESIGNER_RULES.md`](shared/DESIGNER_RULES.md), [`ARTIST_RULES.md`](shared/ARTIST_RULES.md)
- Project Secretary route: [`SECRETARY_RULES.md`](shared/SECRETARY_RULES.md)
- Runtime architecture: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
- UE MCP: [`docs/UE_MCP.md`](docs/UE_MCP.md)
- Validation commands: [`scripts/README.md`](scripts/README.md)
- Designer XLSX workflow and acceptance: [`Design/Data/ReEchoData使用说明.md`](Design/Data/ReEchoData使用说明.md), [`Design/Data/ReEchoData策划验收清单.md`](Design/Data/ReEchoData策划验收清单.md)

## Current prototype

The project runtime-generates a Basic Shapes arena with a manually controlled player, four enemy archetypes, damage feedback, deterministic encounter recording, translucent echo playback and a six-encounter loop. Migrated character/build, element/reaction and weapon/slot domains are authored in `Design/Data/ReEchoData.xlsx`, generated into validated CSV under `Content/Data`, and loaded into immutable runtime snapshots; remaining JSON is migration-only until its domain moves.
