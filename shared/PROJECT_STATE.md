# ReEcho project state

Last updated: 2026-08-10. This file is a current snapshot, not a chronological log; implementation history belongs in `plans/` and Git.

## Playable state

- UE 5.8 C++ 2.5D prototype starts from `/Game/Level00`; `AReEchoGameMode` generates the bounded arena and six-encounter run.
- GAS-authoritative player/enemy attributes and effect-based damage, four weapon/character definitions, four enemy archetypes, deterministic 20 Hz recording, echo playback, trait selection, health UI, damage feedback, pause/restart/quit and final-Boss settlement are connected.
- Player, echo and enemies render as packaged 2D Billboards over a fixed orthographic 3D arena; collision remains authoritative and separate from visual animation.
- Configurable rain and player/echo-centered fog-of-war are visual-only. The top-left HUD shows the configured player portrait and live current/max health; only enemies retain world-space overhead health bars. B opens inventory, M opens the Time Shard shop, and Tab opens paused live player/echo stats.
- Runtime purchases are duplicate-guarded and update the shared build snapshot. Time-trait draws are deterministic per run state, prefer least-owned traits, contain no duplicate offer, and accept only the current pending choice. Runs without an active echo show an explicit stats empty state.
- Development-console GM commands cover status, healing, Time Shards, weather override and enemy clearing; Shipping rejects them.
- Weapon 3 cycles deterministic Water/Flame/Grass/Lightning ordered element pairs backed by CSV reactions. Four-card promotion selects Hunter, Poet, Brave or Sage mechanics, and bombers use separate configurable fuse-trigger and explosion-damage ranges.
- Human PIE play-feel, UI/DPI/font readability and final weather/blur tuning remain required; this is not yet a finished vertical slice.

## Current progress

| Area | Current state | Remaining |
|---|---|---|
| Combat/run | Six encounters, Boss gate, tag-driven GAS input/effects/cooldowns, elemental reactions, four promotion roles, bounded bombers, weapons, enemies and echo loop connected | Broader deterministic combat tests and tuning |
| Presentation | 2D actors, fixed camera, arena art, weather, inventory/shop/stats UI | Human PIE and packaged-menu regression |
| Data | CSV runtime foundation plus character/build and element/status/reaction CSV migration for current playable content; legacy JSON remains migration-only | Domain migration for weapons and slots |
| Planning loop | Recording and active echo playback | Preview/setup beat, multi-echo and anchor depth |
| Validation | Twenty-two `ReEcho.*` automation tests; Editor build passes | New projectile collision/menu/round regressions |

## Milestones

| Milestone | Status | Exit gate |
|---|---|---|
| A - Combat skeleton | Implemented and packaged; tuning remains | Broader determinism and play-feel tuning |
| B - Planning loop | Partial | Preview/setup beat and direction check |
| C - Build/run | Functional prototype; characters/current build cards and element reactions migrated to CSV | Weapons, slots and full content run |
| D - Elements/keystones | Element reactions and four promotion roles implemented; tuning remains | Vertical-slice acceptance |
| E - Validation | Twenty-two automation tests plus current clean Shipping CSV load smoke evidence | Go/No-Go report |

## Collaboration protocol

- Default branch is planner-owned; executors use isolated branches/worktrees and never merge without human approval and planner review.
- Human owns final acceptance and play-feel decisions.
- Only currently edited merge-hostile assets appear under `PLANNER_EXCHANGE.md` Active ownership; remove the row when implementation ends.
- `AGENTS.md` is the sole startup-order authority. Project-specific constraints live in `PROJECT_RULES.md`; generic blueprint material is read only for onboarding or workflow maintenance.

## Verified toolchain

- UE 5.8 installed/release build; separate source checkout is out of scope.
- Latest Editor Development build succeeds and all twenty-two `ReEcho.*` automation tests pass.
- Latest clean Windows Shipping Cook/Pak/Archive and five-second launch smoke passed with the Plan 18 CSV package; human visual/UI regression is still required before release claims.
- `scripts/validate_project.py` performs fast CSV, legacy JSON and workflow consistency checks.

## Regression risks and technical debt

- Legacy `Content/Data/*.json` is migration-only; character/build and element/status/reaction runtime authority has moved to CSV, while weapons, slots and some shop/balance values still use provisional C++/DeveloperSettings values until Plan 21 switches them.
- Runtime arena generation has no dedicated serialized test-map pipeline.
- Projectile damage, pause/restart/quit, round advancement, weather and the new menus lack deterministic automation.
- Shared weapon, enemy and run-flow paths changed for Plans 14-16; human PIE should regress player/echo attacks, role transitions, forge/card sequencing and bomber escape behavior together.
- Human PIE remains necessary for movement, combat feel, echo clarity, UI glyphs/DPI and full menu interaction.
- CSV negative fixtures use a production-baseline-plus-local-override builder in Python and C++ automation; new fixtures should only store changed CSV files.
