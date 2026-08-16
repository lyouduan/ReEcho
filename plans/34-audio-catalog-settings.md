# Plan 34 - Audio - authored catalog and persistent settings

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Plan34 Executor.
- Plan authored by (AI side): Gavyn-side AI.
- Implementation authored by (AI side): Gavyn-side AI.
- Task status: `InProgress` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `PendingBeforeClose` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Local planning / implementation base: freshly fetched `origin/main` containing closed Plan33 and this Ready revision.
- Implementation branch: local `plan/34-audio-catalog-settings-v3` in separate worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan34`.
- Depends on / Blocks: Plan33 is closed; Plan34 provides authored event definitions, preload behavior and persistent bus settings consumed independently by Plans35/36.
- Writes: `Source/ReEchoAudio/**` catalog/settings implementation and tests; `Source/ReEcho/UI/ReEchoSettingsWidget.*`; typed optional bindings compatible with `/Game/ReEcho/UI/WBP_ReEchoSettings.uasset`; standalone canonical `Design/Data/ReEchoAudioEvents.xlsx` (not coupled to `ReEchoData.xlsx` or `ReEchoEnemyData.xlsx`); export schema plus `scripts/data/**`, `Content/Data/reecho_data_manifest.csv`, `Content/Data/csv_schema.csv` and generated `Content/Data/audio_events.csv`; deterministic diagnostic-tone source/import tooling, `/Game/ReEcho/Audio/Diagnostics/**`, provenance documentation; this Plan's notes.
- Stable Reads: closed Plan33 public audio IDs/buses/service/catalog seam; existing XLSX-to-CSV contract; typed settings-shell open/close behavior.
- Impact mode: `Exclusive` while writing the canonical XLSX and `WBP_ReEchoSettings`; `SharedContract` for the event-definition schema, preload contract and persisted bus values.
- Compatibility promise / downstream action: preserve every pre-existing generated CSV byte-for-byte; preserve Graphics/Controls placeholders, typed UMG lifecycle and settings return flow. Missing/invalid optional assets remain safe. Plans35/36 publish semantic IDs through Plan33 and never parse the catalog or own AudioComponents.
- Explicit exclusions: no combat/GameMode/weapon/enemy/echo event hooks; no production music/SFX selection, mix balancing or final sound-design claim; no unlicensed third-party media; no hand-edited generated production CSV; no Executor visual or aural sign-off.

## Locked goal

Make audio definitions designer-editable and audio volumes usable without coupling either concern to gameplay. Add an `AudioEvents` Table in standalone canonical `Design/Data/ReEchoAudioEvents.xlsx` that deterministically generates `Content/Data/audio_events.csv`, and replace only the audio settings placeholder with persistent controls for Master, Music, Ambience, Combat SFX and UI SFX.

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

### Baseline

- Fresh remote baseline: `origin/main@75f243c` (closed Plan44 included).
- Local-only branch/worktree: `plan/34-audio-catalog-settings-v3` / `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan34`; implementation commits `221f076` + persistence fix `1a7b9ef`; no merge or push.
- Plan41/43/44 module boundaries preserved: no GameMode/combat/weapon/enemy/boss hooks are added. Existing host-side `UReEchoCombatAudioAdapterComponent` remains the gameplay subscriber.

### Changed

- Standalone `Design/Data/ReEchoAudioEvents.xlsx` owns `tblAudioEvents -> audio_events.csv`; main and enemy workbooks are untouched.
- Latest three-workbook sync pipeline, manifest, schema, validator and `ReEcho.Build.cs` stage/validate the locked 14-column catalog with `EventId` primary key and the exact Plan33 stable-ID set.
- `FReEchoAudioCatalog` performs quote-aware strict parsing into temporary storage and atomically swaps only after whole-file success; explicit `EventType` and attenuation are loaded. Async preload exposes retryable `NotStarted/Loading/Ready/Failed` state.
- `UReEchoAudioUserSettings` owns only five volume/mute preferences. `UReEchoAudioService` previews changes immediately, persists only on Apply, restores saved values on cancel/destruction and treats diagnostic playback as a non-persistent action.
- `UReEchoSettingsWidget` uses typed optional audio bindings with the existing C++ fallback; Graphics/Controls placeholders remain unchanged.
- Deterministic 440 Hz generator, Unreal Editor import script and provenance note were added; `UI.Error` is the sole stable catalog binding.
- Catalog atomic failure/quoted-field automation was added to the existing ReEchoAudio test suite.
- `shared/CODEBASE_MAP/modules/MOD-ReEchoAudio.md` is updated as the current architecture authority.
- Startup regression fix: the shared manifest reader now enforces `PrimaryKey=Id` only for gameplay registry-owned tables. External module tables such as `AudioEvents(EventId)` remain globally validated by tooling and module loaders without making `FReEchoModule::StartupModule` fatal.

### Evidence

- `python scripts/data/sync_xlsx_to_csv.py --check` -> PASS across `ReEchoData.xlsx`, `ReEchoEnemyData.xlsx`, and `ReEchoAudioEvents.xlsx`; generated bytes match production CSV.
- `python -m py_compile scripts/data/author_audio_events.py scripts/data/sync_xlsx_to_csv.py scripts/validate_project.py` -> PASS.
- `scripts/ue/Build-Editor.cmd -Configuration Development` -> succeeded; UHT/UBT compiled and linked all five Runtime Modules and refreshed the curated Editor bundle.
- `python scripts/validate_project.py` -> PASS after the build refresh (schema, fixtures, modules, XLSX drift, workflow and prebuilt fingerprint).
- Reported startup Fatal root cause reproduced by inspection: `ReadManifest` incorrectly forced every shared-manifest row to `PrimaryKey=Id`; `AudioEvents(EventId)` was the only issue. The ownership gate fix builds successfully and the production/default CSV test fixture now exercises that manifest contract.
- `scripts/ue/Run-Automation.cmd -Filter ReEcho.Audio` -> automation did not launch: platform preflight stopped after reporting unavailable LinuxArm64/VisionOS SDK metadata. No audio test result is claimed; focused automation remains pending in the user's configured Editor environment.
- IDE diagnostics for edited C++ paths -> no reported diagnostics.

### Remaining risks

- Focused audio automation still needs a configured Editor run because this invocation stopped in cross-platform SDK preflight.
- The real diagnostic `.wav`/`.uasset` is intentionally not generated/imported by the coding executor. Until the user runs both scripts, `UI.Error` is a safe no-op.
- `WBP_ReEchoSettings.uasset` is not manually binary-edited; typed optional bindings support it when matching controls exist, while C++ fallback remains testable.
- Subjective layout, loudness and audibility remain human PIE checks.

### Human validation result/request

`PendingBeforeClose`: user imports/listens to the diagnostic event and verifies each bus/mute setting, defaults, apply/return, cancel rollback and restart persistence from start-menu and pause-menu entry. Executor visual/aural inspection is not accepted as human evidence.
