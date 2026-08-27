# ReEcho

UE 5.8 installed-build graybox prototype for the Time Echo vertical slice.

## Start here

- AI entry and first-contact role question: [`AGENTS.md`](AGENTS.md)
- AI/code retrieval map and module design library: [`shared/CODEBASE_MAP/README.md`](shared/CODEBASE_MAP/README.md)
- Project rules: [`shared/PROJECT_RULES.md`](shared/PROJECT_RULES.md)
- Commit and publication rules: [`shared/GIT_RULES.md`](shared/GIT_RULES.md)
- Professional routes: [`PROGRAMMER_RULES.md`](shared/PROGRAMMER_RULES.md), [`DESIGNER_RULES.md`](shared/DESIGNER_RULES.md), [`ARTIST_RULES.md`](shared/ARTIST_RULES.md)
- Project Secretary route: [`SECRETARY_RULES.md`](shared/SECRETARY_RULES.md)
- 项目全局架构：[`shared/CODEBASE_MAP/ARCHITECTURE.md`](shared/CODEBASE_MAP/ARCHITECTURE.md)
- UE MCP: [`docs/UE_MCP.md`](docs/UE_MCP.md)
- Validation commands: [`scripts/README.md`](scripts/README.md)
- Designer XLSX workflow and acceptance: [`Design/Data/ReEchoData使用说明.md`](Design/Data/ReEchoData使用说明.md), [`Design/Data/ReEchoData策划验收清单.md`](Design/Data/ReEchoData策划验收清单.md)
- Designer Bug / request report template: [`issues/TEMPLATE.md`](issues/TEMPLATE.md)

## Current prototype

The project runtime-generates a Basic Shapes arena with a manually controlled player, four enemy archetypes, damage feedback, deterministic encounter recording, translucent echo playback and a six-encounter loop. Migrated character/build, element/reaction and weapon/slot domains are authored in `Design/Data/ReEchoData.xlsx`, generated into validated CSV under `Content/Data`, and loaded into immutable runtime snapshots; remaining JSON is migration-only until its domain moves.

On Win64, a normal checkout includes the UE 5.8 Editor module bundle required to open `ReEcho.uproject` directly. It is tied to the exact installed-engine Build ID in `Binaries/Win64/ReEchoEditor.prebuilt.json`; programmers refresh it through the repository Editor build before every publication.

This repository uses Git LFS for selected large runtime assets. After cloning, install Git LFS and run `python scripts/setup_lfs.py`; before opening `ReEcho.uproject`, building or packaging, run `python scripts/setup_lfs.py --check`. A checkout that still contains a small text pointer instead of the real media file is incomplete.
