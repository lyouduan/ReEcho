# ReEcho project state

Last updated: 2026-08-12. Current snapshot only; history belongs in Plans and Git.

## Playable state

- UE 5.8 C++ 2.5D prototype starts from `/Game/Level00`; `AReEchoGameMode` generates the bounded arena and six-encounter run.
- GAS-authoritative combat, four enemy archetypes, deterministic 20 Hz recording, echo playback, traits, health UI, damage feedback, pause/restart/quit and final-Boss settlement are connected.
- Startup supports new/continue, character and three-weapon initial selection, in-encounter save-and-quit, a shared start/pause settings shell, animated trait draw and post-draw shop flow.
- Character/build, element/status/reaction and weapon/slot domains load from validated generated CSV snapshots authored in `Design/Data/ReEchoData.xlsx`.
- The pre-run selected weapon is locked for the full run; continue and echo initialization retain its stable WeaponId. Legacy InputSlot 1/2/3 values remain data compatibility only and have no runtime hotkeys. Parts, ordered attack steps, reactions, save/recording revisions and active-run pinned snapshots remain connected.
- Run/save v5 now separates the just-completed pending echo, rolling previous-encounter echo, explicitly stored full recordings and stable-GUID replay selections. Specific single/multi runtime spawning and shop selection UI remain follow-up work.
- Human PIE for feel, UI/DPI/font readability and full menu/save regression remains outstanding; this is not a finished vertical slice.

## Current progress

| Area | Current state | Remaining |
|---|---|---|
| Combat/run | Six encounters, Boss gate, GAS input/effects/cooldowns, CSV run-locked weapons/parts/reactions, pinned run/echo snapshots and v5 pending/latest/stored echo persistence | Plan30 runtime replay resolution, then combined human combat/save regression including confirmation that 1/2/3 do not switch weapons |
| Presentation | 2D actors, weather, start/continue/loadout, shared start/pause settings shell, draw/shop, inventory and stats UI | Human visual/DPI/menu regression; Plan26 read-only weapon UI work is reserved |
| Data | One canonical XLSX deterministically generates 16 validated production CSVs; legacy JSON is migration-only for migrated domains | Designer usability QA; later enemy/economy/global-balance migration |
| Validation | Canonical XLSX/project validation and Editor Development build pass; the 34-test full-suite baseline predates Plan29, whose 5 focused echo-storage tests pass on the accepted local implementation | Refresh the full suite before publication; human PIE/usability evidence remains separate |

## Milestones

| Milestone | Status | Exit gate |
|---|---|---|
| A - Combat skeleton | Implemented and packaged; tuning remains | Broader determinism and play-feel tuning |
| B - Planning loop | Partial | Preview/setup beat and direction check |
| C - Build/run/data | Functional prototype with XLSX-authored CSV domains | Designer usability and full content run |
| D - Elements/keystones | Reactions and four promotion roles implemented | Vertical-slice acceptance |
| E - Validation | 34 automation tests and current CSV/XLSX checks | Go/No-Go report plus human regression |

## Collaboration protocol

- Plans, task branches and worktrees remain local until accepted integration; `origin/main` is the only remote branch.
- Lifecycle uses `Proposed | Ready | InProgress | Review | Closed | Blocked`; human validation uses `NotRequired | PendingBeforeClose | PendingFollowUp | Passed`; ownership state is separate.
- Executors write their Plan and owned implementation docs. Planners update shared state/lessons/routes once during review, reducing parallel Markdown conflicts.
- `Isolated` and `ReadOnly` local work may proceed immediately. `SharedContract`/`Exclusive` overlap requires agreement; only `Active Exclusive` ownership blocks another writer.
- Cross-machine consumers use only accepted surfaces on `origin/main`; unpublished provider/consumer parallelism is limited to one clone.
- Local merge does not authorize remote publication. Each publication uses a current-remote candidate, proportional verification, non-force push and either one-candidate or documented standing scoped human authorization.
- When fetch reveals external main commits, the Planner reports Physical/Git conflict, Logical conflict, Coupling and Plan-number collisions before any pull/merge/rebase/push, then integrates only the human-selected outcome; a fast-forward does not bypass this gate.
- Remote main owns published Plan numbers. A colliding unpublished local Plan and every later unpublished local Plan shift together to the first free ordered range.

## Verified toolchain

- UE 5.8 installed/release build; separate source checkout is out of scope.
- Plan25 integrated baseline: deterministic XLSX/CSV check, 10 focused Python tests, Editor Development build and 34/34 `ReEcho.*` automation tests pass.
- Plan29 local acceptance: project validation, Editor Development build, 5/5 focused echo-storage automation tests and whitespace/scope checks pass.
- Latest clean Windows Shipping Cook/Pak/Archive and five-second launch smoke passed with the current generated CSV package.
- `scripts/validate_project.py` checks CSV/XLSX drift, legacy JSON, workflow consistency and project structure.

## Regression risks and technical debt

- Some shop/enemy/global-balance values remain provisional C++/DeveloperSettings values until later data-domain Plans.
- Runtime arena generation has no dedicated serialized test-map pipeline.
- Pause/restart/quit interaction, round advancement, weather and visual menu transitions still lack deterministic automation.
- Human PIE remains necessary for movement/combat feel, echo clarity, UI glyphs/DPI and full menu interaction.
- Generated production CSV edits must round-trip through `Design/Data/ReEchoData.xlsx`.
