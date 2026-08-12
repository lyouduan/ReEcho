# Plan 34 - Audio - authored catalog and persistent settings

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Plan34 Executor.
- Plan authored by (AI side): Gavyn-side AI.
- Implementation authored by (AI side): Gavyn-side AI.
- Task status: `Ready` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `PendingBeforeClose` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Local planning / implementation base: freshly fetched `origin/main` containing closed Plan33 and this Ready revision.
- Implementation branch: local `plan/34-audio-catalog-settings` in a separate worktree.
- Depends on / Blocks: Plan33 is closed; Plan34 provides authored event definitions, preload behavior and persistent bus settings consumed independently by Plans35/36.
- Writes: `Source/ReEchoAudio/**` catalog/settings implementation and tests; `Source/ReEcho/UI/ReEchoSettingsWidget.*`; `/Game/ReEcho/UI/WBP_ReEchoSettings.uasset`; canonical `Design/Data/ReEchoData.xlsx`; its export map/schema plus `scripts/data/**`, `Content/Data/reecho_data_manifest.csv`, `Content/Data/csv_schema.csv` and generated `Content/Data/audio_events.csv`; deterministic diagnostic-tone source/import tooling, `/Game/ReEcho/Audio/Diagnostics/**`, provenance documentation; this Plan's notes.
- Stable Reads: closed Plan33 public audio IDs/buses/service/catalog seam; existing XLSX-to-CSV contract; typed settings-shell open/close behavior.
- Impact mode: `Exclusive` while writing the canonical XLSX and `WBP_ReEchoSettings`; `SharedContract` for the event-definition schema, preload contract and persisted bus values.
- Compatibility promise / downstream action: preserve every pre-existing generated CSV byte-for-byte; preserve Graphics/Controls placeholders, typed UMG lifecycle and settings return flow. Missing/invalid optional assets remain safe. Plans35/36 publish semantic IDs through Plan33 and never parse the catalog or own AudioComponents.
- Explicit exclusions: no combat/GameMode/weapon/enemy/echo event hooks; no production music/SFX selection, mix balancing or final sound-design claim; no unlicensed third-party media; no hand-edited generated production CSV; no Executor visual or aural sign-off.

## Locked goal

Make audio definitions designer-editable and audio volumes usable without coupling either concern to gameplay. Add an `AudioEvents` Table in the canonical workbook that deterministically generates `Content/Data/audio_events.csv`, and replace only the audio settings placeholder with persistent controls for Master, Music, Ambience, Combat SFX and UI SFX.

The exported schema is one row per semantic event with these locked columns, in order:

`EventId, AssetPath, Bus, EventType, Spatial3D, BaseVolume, PitchMin, PitchMax, CooldownSeconds, MaxConcurrency, Priority, PausePolicy, AttenuationMin, AttenuationMax`

- `EventId` is a Plan33 stable ID and appears at most once; seed every Plan33 public ID exactly once in the workbook. `AssetPath` may be empty for not-yet-produced sounds.
- `Bus` allows `Music`, `Ambience`, `CombatSfx`, `UiSfx`; `Master` is settings-only and invalid for an event row.
- `EventType` allows `OneShot` or `Loop`; `Spatial3D` is lowercase `true/false`; `PausePolicy` allows `PauseWithGame` or `ContinueOnPause`.
- Volumes are finite `[0,1]`; pitch bounds are finite and positive with `PitchMin <= PitchMax`; cooldown/attenuation are finite and non-negative with `AttenuationMin <= AttenuationMax`; concurrency is an integer `>= 0`; priority is an integer.
- Duplicate/unknown IDs, invalid enums, missing required fields, malformed Unreal soft-object paths and invalid ranges fail generation/validation with sheet/Table/row/column diagnostics.

`ReEchoAudio` owns CSV loading and asynchronous soft-asset preload. Gameplay may post immediately; an unresolved asset is a safe no-op and may become playable after preload without changing caller behavior. Catalog reload replaces the provider snapshot atomically and never exposes partially parsed rows.

Add a deterministic, script-generated diagnostic tone plus an Editor import script and provenance note. It is technical test media, not production sound design, and is bound to one `UI.*` event solely so the user can verify volume/mute behavior.

The settings screen owns five sliders and five mute toggles through typed optional bindings named `MasterVolumeSlider`, `MusicVolumeSlider`, `AmbienceVolumeSlider`, `CombatSfxVolumeSlider`, `UiSfxVolumeSlider` and matching `*MuteCheckBox` names. Edits preview immediately through `UReEchoAudioService`; `Apply and Return` persists them outside run saves, and `Restore Defaults` restores all five buses/mutes before persistence. Existing Graphics and Controls placeholders remain unchanged.

## Locked acceptance

- [ ] The canonical `AudioEvents` Table deterministically generates `Content/Data/audio_events.csv`; full and `--sheet AudioEvents` sync preserve every pre-existing generated CSV byte-for-byte, and `--check` detects drift.
- [ ] Schema/allowlist/reference validation enforces the locked columns and rejects duplicate/unknown IDs, invalid policies/ranges, `Master` event rows and malformed asset paths with row-level diagnostics.
- [ ] `ReEchoAudio` loads a complete immutable catalog snapshot and asynchronously preloads non-empty soft paths without adding a reverse dependency on `ReEcho` or blocking gameplay.
- [ ] Master/Music/Ambience/Combat/UI volume and mute values preview immediately, restore defaults and persist across a real process restart; preferences never enter run-save or recording bytes.
- [ ] Typed `WBP_ReEchoSettings` bindings work from both start-menu and pause-menu entry, while Graphics/Controls placeholders and existing apply/return flow remain intact.
- [ ] The deterministic diagnostic tone is generated/imported repeatably, clearly marked replaceable and covered by origin/license documentation; no other audio media is introduced.
- [ ] Missing, invalid or not-yet-loaded optional assets produce the Plan33 safe no-op and warning-once behavior, not a gameplay failure or blocking load.
- [ ] Required Python tests, canonical workbook `--check`, project validation, formatting when available, Editor build, focused audio/settings/UI automation and `git diff --check` pass.
- [ ] User validates the diagnostic event, five slider/mute pairs, defaults, apply/return and process-restart persistence in PIE before closure.

## Step 0 gate

- Baseline branch/commit: freshly fetched `origin/main` containing closed Plan33 and this Ready Plan34 revision; record the exact commit in Execution notes.
- Engine/build availability: UE 5.8 and repository spreadsheet runtime are available.
- Existing focused-test result: Plan33 automation/build pass; canonical workbook `--check` passes before editing.
- Active exclusive ownership or shared-contract approval: claim both `WorkbookWriter` and `/Game/ReEcho/UI/WBP_ReEchoSettings.uasset` before changing them; wait only if another active publication edit owns either exact resource.
- Stop condition if the baseline is broken: pre-existing workbook drift, invalid Plan33 catalog contract, dirty/actively edited settings WBP, or an unidentified binary audio source.

## Implementation outline

1. Add the locked `AudioEvents` Table/export mapping/schema and seed the complete Plan33 event-ID set with empty paths except the diagnostic event; update generator/validator tests before runtime loading.
2. Implement atomic CSV parsing plus asynchronous soft-asset preload behind Plan33's provider seam, including retryability after a previously unresolved asset.
3. Add module-owned user-preference persistence for the five buses/mutes; do not put preferences in run-save data.
4. Add the typed slider/checkbox bindings to C++ and `WBP_ReEchoSettings`; preserve the existing fallback and screen lifecycle.
5. Generate and import the diagnostic tone through reviewed scripts, record provenance and bind only its authored event row.
6. Run data/runtime/UI regression, update Execution notes and leave subjective layout/listening acceptance to the user.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Data | focused XLSX sync tests; full and `--sheet AudioEvents` canonical `--check` | deterministic `audio_events.csv`; no drift in existing generated CSV |
| Static/build | project validation, dependency/include audit, formatting when available, Editor build | schema/runtime/UI compile; no reverse module dependency |
| Automation | atomic catalog/preload failure, invalid rows, bus persistence/defaults and typed Widget bindings | focused tests pass without modifying the user's real settings file |
| Asset | deterministic tone generation/import and source-path audit | imported diagnostic asset resolves; provenance is recorded; no manual `.uasset` edit |
| Human | PIE diagnostic event and five bus/mute controls from both settings entry points | user reports audible preview/mute/default/apply and restart persistence; Executor supplies only the checklist |
| Scope | `git diff --check`; explicit changed-path audit | no untracked provenance, unrelated media or generated UE output outside the curated bundle |

## Execution notes

### Changed

### Evidence

### Remaining risks

### Human validation result/request

`PendingBeforeClose`: user listens to the diagnostic event and verifies each bus/mute setting, defaults, apply/return and restart persistence. Executor visual/aural inspection is not accepted as human evidence.
