# Plan 34 - Audio - authored catalog and persistent settings

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Unassigned.
- Plan authored by (AI side): Gavyn-side AI.
- Implementation authored by (AI side): Unassigned.
- Task status: `Proposed` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `PendingBeforeClose` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Local planning / implementation base: accepted local Plan33 contract plus the latest integrated main.
- Implementation branch: local `plan/34-audio-catalog-settings` in a separate worktree.
- Depends on / Blocks: depends on Plan33; provides authored event definitions and persistent bus settings consumed independently by Plans35/36.
- Writes: Plan33 catalog-provider implementation; canonical `Design/Data/ReEchoData.xlsx` audio sheet plus matching generator/schema/CSV changes; audio preference persistence; `ReEchoSettingsWidget.*`; audio source/license documentation; Plan34 tests and notes.
- Stable Reads: Plan33 public audio IDs/buses; existing XLSX-to-CSV contract; shared settings-shell open/close behavior.
- Impact mode: `Exclusive` while writing the canonical XLSX; `SharedContract` for event-definition schema and persisted bus values.
- Compatibility promise / downstream action: preserve every existing migrated table byte-for-byte; preserve graphics/control placeholders and settings open/close flow. Missing optional assets remain safe. Plans35/36 consume event IDs and do not parse the catalog themselves.
- Explicit exclusions: no combat/GameMode/weapon/enemy/echo event hooks, no final sound-design claim, no unlicensed third-party media and no hand-edited generated production CSV.

## Locked goal

Make audio definitions designer-editable and audio volumes usable without coupling either concern to gameplay. Add an `AudioEvents` authoring sheet that deterministically generates the runtime catalog, and replace the audio settings placeholder with persistent controls for Master, Music, Ambience, Combat SFX and UI SFX.

Each authored event records at least stable event ID, soft asset path, bus, playback/state mode, 2D/3D policy, looping, base volume, pitch range, cooldown, concurrency, priority and pause policy. Invalid enum values, duplicate IDs, missing required fields and invalid ranges fail generation/validation with row-level errors.

## Locked acceptance

- [ ] One canonical XLSX sheet deterministically generates the audio catalog CSV; `--check` detects drift.
- [ ] Schema/allowlist/reference validation rejects duplicate IDs, invalid policies/ranges and malformed asset paths.
- [ ] The audio module loads the generated catalog without adding a reverse dependency on `ReEcho`.
- [ ] Master/Music/Ambience/Combat/UI volumes and mute state apply immediately, restore defaults and persist across process restart.
- [ ] Existing settings categories and return flow remain intact.
- [ ] Every committed audio asset has an origin/license row; generated placeholders are clearly identified and replaceable.
- [ ] Missing optional assets produce the Plan33 safe no-op, not a gameplay failure.
- [ ] Required Python tests, project validation, Editor build, focused automation and `git diff --check` pass.
- [ ] User validates slider/mute behavior and one diagnostic event in PIE before closure.

## Step 0 gate

- Baseline branch/commit: accepted Plan33 local integration commit.
- Engine/build availability: UE 5.8 and spreadsheet sync runtime available.
- Existing focused-test result: Plan33 automation/build pass; canonical workbook `--check` passes before editing.
- Active exclusive ownership or shared-contract approval: claim `WorkbookWriter` before changing XLSX; wait if another active publication edit owns it.
- Stop condition if the baseline is broken: any pre-existing workbook drift, invalid Plan33 catalog contract or unidentified binary audio source.

## Implementation outline

1. Lock the minimal AudioEvents sheet/schema and generator output before adding rows.
2. Add audio-module catalog loading/validation behind Plan33's provider seam.
3. Add module-owned config persistence for the five buses; do not put preferences in run save data.
4. Replace only the settings audio placeholder with controls and apply/restore behavior.
5. Add a small diagnostic authored event/asset sufficient for functional listening; record source/license.
6. Keep generated CSV, workbook and runtime bytes synchronized and test persistence/catalog failures.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Data | focused XLSX sync tests and canonical `--check` | deterministic AudioEvents output; no drift in existing tables |
| Static/build | project validation, `.clang-format`, Editor build | schema/runtime/UI compile and validate |
| Automation | catalog loading, invalid rows, bus persistence/defaults | focused tests pass |
| Human | PIE diagnostic event and five bus/mute controls | user reports audible apply, mute, default and restart persistence |
| Scope | `git diff --check`; asset source audit | no untracked provenance or generated build output |

## Execution notes

### Changed

### Evidence

### Remaining risks

### Human validation result/request

`PendingBeforeClose`: user listens to the diagnostic event and verifies each bus/mute setting, defaults and restart persistence.
