# Plan 26 - UI - run-locked weapon read-only presentation

## Coordination

- Planner owner: Gavyn-side Planner, taking over the stale teammate broadcast under the user's request to resolve Plan26.
- Executor owner: Unassigned.
- Task status: `Ready` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `PendingBeforeClose` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Planning ref / implementation base: fetch `origin/coord/gavyn-plan26-resolution`, verify it contains the refreshed Plan26 planning commit, and create the implementation branch from that ref. Its code base is current `origin/main` plus pure planning commits; the old `origin/coord/teammate-planning-broadcast-20260811` is superseded for execution and must not be merged.
- Implementation branch: `plan/26-weapon-readonly-ui` in a separate clean worktree.
- Depends on / Blocks: typed Plan24 weapon queries and the accepted full-run weapon lock already on `origin/main`. Blocks Plan31 writes to `ReEchoInventoryShopWidget.*` until review/merge/ownership release.
- Writes: `Source/ReEcho/Public/UI/ReEchoInventoryShopWidget.h`; `Source/ReEcho/Private/UI/ReEchoInventoryShopWidget.cpp`; `Source/ReEcho/Public/UI/ReEchoStatsWidget.h`; `Source/ReEcho/Private/UI/ReEchoStatsWidget.cpp`; new Plan26-only cheap UI/state automation if useful; this Plan's Execution notes and its live lifecycle/ownership row.
- Stable Reads: `UReEchoRunSubsystem::CurrentBuild.WeaponId`, `GetRunDataSnapshot()`, `FReEchoCsvDataSnapshot::FindEnabledWeapon` and stable weapon row display fields.
- Impact mode: `Isolated` plus `ReadOnly` provider consumption.
- Compatibility promise / downstream action: no gameplay, persistence, selection, shop transaction or data-contract change. Plan31 must build on the accepted Plan26 widget instead of replacing its weapon presentation.
- Explicit exclusions: GameMode; Loadout widget; Registry/Reader; XLSX/CSV/schema; Build/Run/Save/Recording mutation; Player/Echo/Weapon/Projectile/Enemy execution; parts/attack steps/economy; runtime weapon switching; `.uasset`/`.umap` and new art.

## Locked goal

Make the existing inventory/shop and player/echo stats overlays display the current run-locked weapon through Plan24's typed read-only data surface. Resolve `CurrentBuild.WeaponId` against the pinned run snapshot and show the configured weapon `DisplayName`, with a clear safe state when no run, WeaponId, snapshot or enabled row is available.

The selected weapon is locked for the full run. This Plan must not restore or test runtime weapon switching; continue/load restores the same stable WeaponId and the UI reflects that identity when reopened.

## Locked acceptance

- [ ] Inventory mode and shop mode both retain all existing owned-item, offer, currency, purchase and close behavior while adding a concise current-weapon label.
- [ ] Player/echo stats overlay retains its zero/one Echo behavior and adds the current player's configured weapon label without changing stat calculations.
- [ ] Both widgets resolve the current run-locked WeaponId through `UReEchoRunSubsystem` plus its pinned typed snapshot and `FindEnabledWeapon`; no CSV parsing, local weapon catalog, copied DTO or hard-coded weapon-name switch is introduced.
- [ ] Missing GameInstance, RunSubsystem, snapshot, WeaponId or enabled weapon row produces an explicit stable display such as `Weapon unavailable`; it never crashes or silently substitutes another weapon.
- [ ] Reopening either widget after new game or continue reflects the authoritative current WeaponId. No refresh path mutates build, weapon selection, save state, inventory or shop state.
- [ ] Only declared UI files and optional Plan26-only tests change; no Plan24 provider, GameMode or gameplay file changes.
- [ ] Executor performs C++ formatting, project validation, Editor Development build, cheap Plan26-focused automation where practical and `git diff --check`, then hands the user a concise PIE checklist without performing visual QA.
- [ ] The user validates label readability and the existing inventory/shop/stats interaction before closure.
- [ ] No generated products or machine-local paths are committed.

## Step 0 gate

- Baseline branch/commit: fetch current remote refs; verify `origin/main` is an ancestor of `origin/coord/gavyn-plan26-resolution`, then branch from the coordination ref so this refreshed Plan is present.
- Engine/build availability: ask the human to save and close the ReEcho Editor before compiling; installed UE 5.8 only.
- Existing focused-test result: record project validation and compile availability; do not spend the Executor pass on broad pre-change PIE or full visual regression.
- Active exclusive ownership or shared-contract approval: confirm Plan31 remains Proposed and no active task owns the four Plan26 UI files. Plan29/28 Writes are disjoint.
- Stop condition if the baseline is broken: if the stable typed snapshot cannot resolve the current WeaponId without provider changes, report the missing interface and stop; do not expand scope.

## Implementation outline

1. Read the final public RunSubsystem and CSV snapshot headers plus paired implementations, then add one small shared private UI resolver only if it avoids duplicated lookup code without creating a public DTO.
2. Add a current-weapon text element to the existing Inventory and Shop layouts without replacing their current panels or Plan07 behavior.
3. Add the current player weapon label to the Stats layout without changing player/echo stat data or zero-Echo presentation.
4. Cover only cheap resolver/state behavior in automation when practical, format, validate and build. Give the user short reopening/new/continue test steps; do not run visual QA.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py` | Project/data/workflow invariants pass |
| Format | Repository `.clang-format` on changed `.h`/`.cpp` | Intended UI formatting only |
| Build | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT exit code 0 |
| Focused automation | Cheap Plan26 resolver/state test if added | Typed configured label and missing-data state pass |
| Whitespace/scope | `git diff --check` and explicit path audit | Four UI files plus allowed Plan/test notes only |
| Human | User PIE: Inventory, Shop and Stats labels after new/continue; existing buttons still work | User reports `Passed` or concrete rework |

## Execution notes

### Changed

### Evidence

### Remaining risks

### Human validation result/request
