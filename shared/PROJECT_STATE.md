# ReEcho project state

Last updated: 2026-07-30. This file is a current snapshot, not a chronological log; implementation history belongs in `plans/` and Git.

## Playable state

- UE 5.8 C++ 2.5D prototype starts from `/Game/Level00`; `AReEchoGameMode` generates the bounded arena and six-encounter run.
- GAS-routed player attacks, four weapon/character definitions, four enemy archetypes, deterministic 20 Hz recording, echo playback, trait selection, health UI, damage feedback, pause/restart/quit and final-Boss settlement are connected.
- Player, echo and enemies render as packaged 2D Billboards over a fixed orthographic 3D arena; collision remains authoritative and separate from visual animation.
- Configurable rain and layered fog are visual-only. B opens inventory, M opens the Time Shard shop, and Tab opens paused live player/echo stats over the blurred supplied background.
- Runtime purchases are duplicate-guarded and update the shared build snapshot. Runs without an active echo show an explicit stats empty state.
- Human PIE play-feel, UI/DPI/font readability and final weather/blur tuning remain required; this is not yet a finished vertical slice.

## Current progress

| Area | Current state | Remaining |
|---|---|---|
| Combat/run | Six encounters, Boss gate, GAS input, weapons, enemies and echo loop connected | Broader deterministic combat tests and tuning |
| Presentation | 2D actors, fixed camera, arena art, weather, inventory/shop/stats UI | Human PIE and packaged-menu regression |
| Data | Reviewable JSON registries and DeveloperSettings runtime values | Runtime data importer/migration |
| Planning loop | Recording and active echo playback | Preview/setup beat, multi-echo and anchor depth |
| Validation | Six `ReEcho.*` automation tests; Editor build passes | New projectile/menu/round regressions |

## Milestones

| Milestone | Status | Exit gate |
|---|---|---|
| A - Combat skeleton | Implemented and packaged; tuning remains | Broader determinism and play-feel tuning |
| B - Planning loop | Partial | Preview/setup beat and direction check |
| C - Build/run | Functional prototype; data migration partial | Data importer and full content run |
| D - Elements/keystones | Partial seams | Vertical-slice acceptance |
| E - Validation | Six automation tests plus prior Shipping smoke evidence | Go/No-Go report |

## Collaboration protocol

- Default branch is planner-owned; executors use isolated branches/worktrees and never merge without human approval and planner review.
- Human owns final acceptance and play-feel decisions.
- Only currently edited merge-hostile assets appear under `PLANNER_EXCHANGE.md` Active ownership; remove the row when implementation ends.
- `AGENTS.md` is the sole startup-order authority. Project-specific constraints live in `PROJECT_RULES.md`; generic blueprint material is read only for onboarding or workflow maintenance.

## Verified toolchain

- UE 5.8 installed/release build; separate source checkout is out of scope.
- Latest Editor Development build succeeds and all six `ReEcho.*` automation tests pass.
- Prior clean Windows Shipping Cook/Pak/Archive and launch smoke test passed before Plans 06-08; those UI/weather additions still require a fresh packaged regression before release claims.
- `scripts/validate_project.py` performs fast JSON and workflow consistency checks.

## Regression risks and technical debt

- `Content/Data/*.json` is not runtime-loaded; gameplay still uses provisional C++/DeveloperSettings values.
- Runtime arena generation has no dedicated serialized test-map pipeline.
- Projectile damage, pause/restart/quit, round advancement, weather and the new menus lack deterministic automation.
- Human PIE remains necessary for movement, combat feel, echo clarity, UI glyphs/DPI and full menu interaction.