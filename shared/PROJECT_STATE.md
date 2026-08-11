# ReEcho project state

Last updated: 2026-08-11. This file is a current snapshot, not a chronological log; implementation history belongs in `plans/` and Git.

## Playable state

- UE 5.8 C++ 2.5D prototype starts from `/Game/Level00`; `AReEchoGameMode` generates the bounded arena and six-encounter run.
- GAS-authoritative player/enemy attributes and effect-based damage, CSV-backed weapon/character definitions, four enemy archetypes, deterministic 20 Hz recording, echo playback, trait selection, health UI, damage feedback, pause/restart/quit and final-Boss settlement are connected.
- Player, echo and enemies render as packaged 2D Billboards over a fixed orthographic 3D arena; collision remains authoritative and separate from visual animation.
- Configurable rain and player/echo-centered fog-of-war are visual-only. The top-left HUD shows the configured player portrait and live current/max health; only enemies retain world-space overhead health bars. B opens inventory, M opens the Time Shard shop, and Tab opens paused live player/echo stats. Renderer configuration explicitly keeps Substrate disabled.
- Runtime purchases are duplicate-guarded and update the shared build snapshot. Time-trait draws are deterministic per run state, prefer least-owned traits, contain no duplicate offer, and accept only the current pending choice. The centered animated draw screen shows current shards and opens the existing shop before the next encounter. Runs without an active echo show an explicit stats empty state.
- Development-console GM commands cover status, healing, Time Shards, weather override and enemy clearing; Shipping rejects them.
- Startup is blocked by a save-aware panel: profiles without a save can start new, while valid saves offer continue/new. New runs then require a data-backed character and exactly-three CSV start-selectable initial weapons; runtime weapon switching reads the same CSV InputSlot mapping and continue rejects incompatible saved weapon data revisions. Confirmed in-encounter exit saves the clock, player, active recording and living enemies before platform quit; save failure never exits.
- Weapon 3 cycles deterministic Water/Flame/Grass/Lightning ordered element pairs backed by CSV reactions. Weapon types, concrete WeaponIds, attack steps, input hotkeys, slot profiles, parts and part effects load from seven weapon-domain CSV tables; `W_J_02`/`W_J_01`/`W_J_03` keep legacy hotkeys 1/2/3, `W_J_04` remains an enabled Scythe-pattern weapon, and non-start `W_J_05`/`W_J_06` provide reachable Dagger part and StaffProjectile runtime paths. Build/recording/save snapshots now carry equipped parts plus a deterministic seven-table weapon-domain revision; restore/playback reject incompatible data before mutating a run. Four-card promotion selects Hunter, Poet, Brave or Sage mechanics, and bombers use separate configurable fuse-trigger and explosion-damage ranges.
- Human PIE play-feel, UI/DPI/font readability and final weather/blur tuning remain required; this is not yet a finished vertical slice.

## Current progress

| Area | Current state | Remaining |
|---|---|---|
| Combat/run | Six encounters, Boss gate, GAS input/effects/cooldowns, CSV weapons with idempotent part derivation, atomic switching, ordered attack steps and pinned player/echo snapshots, elemental reactions and echo loop connected | Unified Human PIE after Plan 25 for combat feel and authoring flow |
| Presentation | 2D actors, fixed camera, arena art, weather, start/continue/loadout, confirmed exit, animated trait draw, inventory/shop/stats UI | Human PIE and packaged-menu regression |
| Data | CSV runtime foundation plus character/build, element/status/reaction and current weapon/slot domains; legacy JSON remains migration-only | Plan 25 XLSX authoring generation and later domain migrations |
| Planning loop | Recording and active echo playback | Preview/setup beat, multi-echo and anchor depth |
| Validation | Plan 24 static validation, current Editor Development build, 7 focused weapon tests and all 34 `ReEcho.*` automation tests pass | Unified Human PIE after Plan 25 |

## Milestones

| Milestone | Status | Exit gate |
|---|---|---|
| A - Combat skeleton | Implemented and packaged; tuning remains | Broader determinism and play-feel tuning |
| B - Planning loop | Partial | Preview/setup beat and direction check |
| C - Build/run | Functional prototype; characters/current build cards, element reactions and current weapon/slot domain migrated to CSV | XLSX authoring and full content run |
| D - Elements/keystones | Element reactions and four promotion roles implemented; tuning remains | Vertical-slice acceptance |
| E - Validation | Thirty-four automation tests plus current clean Shipping CSV load smoke evidence | Go/No-Go report |

## Collaboration protocol

- Default branch is planner-owned; executors use isolated branches/worktrees and never merge without human approval and planner review.
- Human owns final acceptance and play-feel decisions.
- In distributed multi-Planner work, each planning batch and its impact/ownership announcement must be committed and pushed to an advertised remote planning ref before Executors start. Shared-contract or exclusive overlap is resolved Planner-to-Planner; read-only consumers may proceed against the explicitly published stable surface.
- The first trial advertises coordination on `origin/coord/planning-broadcast-20260811` and Plan24 data/consumer work on `origin/plan/24-weapons-slots-csv`; `origin/main` remains behind the accepted local baseline until a separate human-authorized synchronization.
- Only currently edited merge-hostile assets appear under `PLANNER_EXCHANGE.md` Active ownership; remove the row when implementation ends.
- `AGENTS.md` is the sole startup-order authority. Project-specific constraints live in `PROJECT_RULES.md`; generic blueprint material is read only for onboarding or workflow maintenance.

## Verified toolchain

- UE 5.8 installed/release build; separate source checkout is out of scope.
- Latest Plan 24 Editor Development build succeeds; 7 focused `ReEcho.Weapons` tests and all 34 `ReEcho.*` automation tests pass from the current source.
- Latest clean Windows Shipping Cook/Pak/Archive and five-second launch smoke passed with the Plan 21 CSV package; human visual/UI regression is still required before release claims.
- `scripts/validate_project.py` performs fast CSV, legacy JSON and workflow consistency checks.

## Regression risks and technical debt

- Legacy `Content/Data/*.json` is migration-only for migrated domains; character/build, element/status/reaction and weapon/slot runtime authority has moved to CSV. Some shop/enemy/global balance values still use provisional C++/DeveloperSettings values until later domain plans.
- Runtime arena generation has no dedicated serialized test-map pipeline.
- Full pause/restart/quit interaction, round advancement, weather and visual menu transitions still lack deterministic automation.
- Shared weapon, enemy and run-flow paths changed for Plans 14-16; human PIE should regress player/echo attacks, role transitions, forge/card sequencing and bomber escape behavior together.
- Human PIE remains necessary for movement, combat feel, echo clarity, UI glyphs/DPI and full menu interaction.
- CSV negative fixtures use a production-baseline-plus-local-override builder in Python and C++ automation; new fixtures should only store changed CSV files.
