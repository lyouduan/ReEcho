# ReEcho project state

Last updated: 2026-07-23

## Playable state

- The repository contains a source-first UE 5.8 C++ graybox prototype and design JSON registry.
- `AReEchoGameMode` generates the arena at runtime on the engine template map; no project `.umap` is committed.
- The current loop includes GAS-routed player attacks/weapon selection, swept sphere projectiles, four enemy archetypes, world health bars, damage VFX and hit reactions, deterministic recording, echo playback and automatic six-encounter advancement.
- Player, echo, grunt and boss use packaged 2D Billboard textures with soft ground shadows; enemies use sprite-height capsule hit volumes and debug-only bounds.
- Esc opens a packaged-build pause menu with resume, full restart and quit; the death screen supports restart and quit.
- UE 5.8 Editor Development and Windows Shipping builds succeed. Three ReEcho automation tests pass, and the clean-cooked Windows package launches without an immediate crash.
- Human PIE play-feel acceptance is still required; the project must not be presented as a finished vertical slice.

## Current progress

- Milestone A combat skeleton is implemented as a runtime-generated 2.5D prototype with 2D actors over a 3D arena.
- Player input activates dedicated GAS abilities; weapons still own compatible damage/cooldown execution, and projectiles use continuous capsule-path hit tests.
- Enemies include grunt, shield, bomber and boss behaviors plus stagger, knockback, shake feedback, fixed capsule hit volumes and camera-facing health bars.
- Recording/playback and six-encounter run seams are connected end to end.
- AI workflow, code routing, installed-engine scripts, UE MCP and lazy RenderDoc MCP guidance are present.

## Milestones

| Milestone | Scope | Status | Exit gate |
|---|---|---|---|
| A - Combat skeleton | Player, projectiles, four enemies, encounter clock, recording and echo | Implemented and packaged; tuning remains | Broader combat determinism and play-feel tuning |
| B - Planning loop | Preview, recorded path, route preview, setup beat, collaboration telemetry | Not started | Five-player direction check |
| C - Build/run | Stats, four characters/weapons, cards, shops, currency, six encounters | GAS input and trait loop connected; content/data migration partial | Data importer + full content run |
| D - Elements/keystones | Reactions, paths, multi-echo, anchor, Boss | Partial seams only | Vertical slice acceptance |
| E - Validation | Fixed-seed regressions, tuning, A/B test | Three automation tests + Windows Shipping smoke test | Go/No-Go report |

## Collaboration protocol

- Default branch is planner-owned; executors work in isolated branches/worktrees and do not merge.
- Human owns final acceptance and play-feel decisions.
- Shared/merge-hostile UE assets require an ownership row in `PLANNER_EXCHANGE.md`.
- `shared/` is the externalized memory bus. `CODEBASE_MAP.md` is the shortest authoritative code retrieval index.
- Generic workflow improvements go back to the canonical blueprint; ReEcho-specific decisions stay in project rules/state.

## Verified toolchain

- Engine: UE 5.8 installed/release build; the separate source checkout is out of scope.
- Build: `scripts/ue/Build-Editor.cmd` succeeds for ReEchoEditor Win64 Development.
- Automation: `scripts/ue/Run-Automation.cmd -Filter ReEcho` passes GAS structure plus two recording tests.
- Packaging: a clean Windows Shipping Cook includes all four 2D actor textures and the resulting EXE passes a 12-second launch smoke test.
- Static: `scripts/validate_project.py` validates JSON registries and workflow files.
- Style: project `.clang-format`, reflection-macro layout gate and `git diff --check` pass.

## Regression risks and technical debt

- `Content/Data/*.json` is not cooked/runtime data; gameplay still uses provisional C++ values.
- The runtime arena is not a committed test map and has no asset-generation pipeline.
- GAS structure and recording have automation; projectile damage, pause/restart/quit, round advancement and hit reactions still need deterministic tests.
- Experimental `AllToolsets` can emit unrelated GameFeatureData/Niagara Python commandlet warnings.
- Human PIE checks remain necessary for movement, projectile hit feel, health-bar readability, echo clarity and full packaged-menu interaction.
