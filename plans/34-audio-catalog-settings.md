# Plan 34 - Audio - authored catalog and persistent settings

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Plan34 Executor.
- Plan authored by (AI side): Gavyn-side AI.
- Implementation authored by (AI side): Gavyn-side AI.
- Task status: `Closed` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `PendingFollowUp` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Local planning / implementation base: freshly fetched `origin/main` containing closed Plan33 and this Ready revision.
- Implementation branch: local `plan/34-audio-catalog-settings-v3` in separate worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan34`.
- Depends on / Blocks: Plan33 is closed; Plan34 provides authored event definitions, preload behavior and persistent bus settings consumed independently by Plans35/36.
- Writes: `Source/ReEchoAudio/**` catalog/settings implementation and tests; `Source/ReEcho/UI/ReEchoSettingsWidget.*`; `/Game/ReEcho/UI/WBP_ReEchoSettings.uasset`; `Design/UI/ReEcho_UI修改指导.md`; `shared/CODEBASE_MAP/README.md` and documentation-only `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`; standalone canonical `Design/Data/ReEchoAudioEvents.xlsx` (not coupled to `ReEchoData.xlsx` or `ReEchoEnemyData.xlsx`); export schema plus `scripts/data/**`, `Content/Data/reecho_data_manifest.csv`, `Content/Data/csv_schema.csv` and generated `Content/Data/audio_events.csv`; deterministic diagnostic-tone source/import tooling, `/Game/ReEcho/Audio/Diagnostics/**`, provenance documentation; this Plan's notes.
- Stable Reads: closed Plan33 public audio IDs/buses/service/catalog seam; existing XLSX-to-CSV contract; Plan29 UMG/C++ UI architecture; typed settings-shell open/close behavior.
- Impact mode: `Exclusive` while writing the canonical XLSX and `WBP_ReEchoSettings`; `SharedContract` for the event-definition schema, WBP binding names, preload contract and persisted bus values.
- Compatibility promise / downstream action: preserve every pre-existing generated CSV byte-for-byte; preserve Graphics/Controls placeholders, typed UMG lifecycle and settings return flow. Normal UI presentation remains designer-authored in WBP; C++ WidgetTree construction remains asset-failure fallback only. Missing/invalid optional assets remain safe. Plans35/36 publish semantic IDs through Plan33 and never parse the catalog or own AudioComponents.
- Explicit exclusions: no new `ReEchoUI` Runtime Module or source-directory migration; no combat/GameMode/weapon/enemy/echo event hooks; no production music/SFX selection, mix balancing or final sound-design claim; no unlicensed third-party media; no hand-edited generated production CSV; no Executor visual or aural sign-off.

## 架构影响与设计决策

- 受影响标识：`MOD-ReEchoAudio`、`MOD-ReEcho` 内的 `AREA-UI`；新增 `MOD-ReEchoUI` 仅作为文档型逻辑入口，不代表 `ReEcho.uproject` 中存在同名 Runtime Module。
- UI 设计边界沿用 Plan29：`WBP_ReEchoSettings` 拥有固定音频控件的布局、样式、尺寸与焦点表现；`UReEchoSettingsWidget` 只负责类型化绑定、状态刷新、事件转发和资产失效 fallback。
- 权威状态不变：音量和静音偏好仍由 `ReEchoAudio` 拥有；UI 只预览、提交或撤销受控设置，不复制偏好状态。
- 依赖方向不变：`ReEcho -> ReEchoAudio`；本 Plan 不新增 Runtime Module，也不改变全局模块拓扑。
- 文档同步：更新 `Design/UI/ReEcho_UI修改指导.md` 的 Settings 绑定契约；创建 `MOD-ReEchoUI.md` 引导至该设计文档；更新 `CODEBASE_MAP/README.md` 的 `AREA-UI` 路由。`ARCHITECTURE.md` 已审阅，因 Runtime Module 拓扑不变而无需修改。

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

设置页在正常路径中由 `WBP_ReEchoSettings` 静态拥有 `AudioPanel`、五个 Slider、五个静音 Checkbox 和一个非持久化诊断音 Checkbox；布局、样式、尺寸、间距和焦点表现归 UMG/WBP。`UReEchoSettingsWidget` 通过与控件完全同名的 `BindWidgetOptional` 属性负责事件绑定、状态刷新和服务调用；C++ 动态构造只用于 WBP 缺失时的最低可用 fallback，不得作为正式布局路径。控件名为 `MasterVolumeSlider`、`MusicVolumeSlider`、`AmbienceVolumeSlider`、`CombatSfxVolumeSlider`、`UiSfxVolumeSlider`、对应的 `*MuteCheckBox` 以及 `DiagnosticToneCheckBox`。编辑通过 `UReEchoAudioService` 立即预览；`Apply and Return` 将其持久化到 Run Save 之外，`Restore Defaults` 在持久化前恢复全部五组 Bus/静音默认值。现有 Graphics 和 Controls 占位内容保持不变。

## Locked acceptance

- [ ] The canonical `AudioEvents` Table deterministically generates `Content/Data/audio_events.csv`; full and `--sheet AudioEvents` sync preserve every pre-existing generated CSV byte-for-byte, and `--check` detects drift.
- [ ] Schema/allowlist/reference validation enforces the locked columns and rejects duplicate/unknown IDs, invalid policies/ranges, `Master` event rows and malformed asset paths with row-level diagnostics.
- [ ] `ReEchoAudio` loads a complete immutable catalog snapshot and asynchronously preloads non-empty soft paths without adding a reverse dependency on `ReEcho` or blocking gameplay.
- [ ] Master/Music/Ambience/Combat/UI volume and mute values preview immediately, restore defaults and persist across a real process restart; preferences never enter run-save or recording bytes.
- [ ] `WBP_ReEchoSettings` 按 Plan29 的 UMG/C++ 边界静态包含 `AudioPanel`、五个 Slider、五个静音 Checkbox 和诊断音 Checkbox；全部绑定名称/类型正确，Blueprint 编译无错误，并能从开始菜单和暂停菜单进入。Graphics/Controls 占位内容和既有 Apply/Return 流程保持完整。
- [ ] 正常 WBP 路径不依赖 `BuildWidgetTree()`/`BuildAudioPanel()` 生成布局；C++ fallback 仅在设计资产缺失时提供最低可用界面。
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
4. 按 Plan29 的正式 UI 路径更新 `WBP_ReEchoSettings`：由 UMG 静态制作固定 `AudioPanel` 布局并添加全部 Slider/Checkbox；C++ 属性名与 WBP 控件名严格一致，只负责绑定、刷新、服务调用和资产缺失 fallback。禁止用“向旧 WBP 动态塞入完整布局”替代正式资产修改。
5. Generate and import the diagnostic tone through reviewed scripts, record provenance and bind only its authored event row.
6. Run data/runtime/UI regression, update Execution notes and leave subjective layout/listening acceptance to the user.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Data | focused XLSX sync tests; full and `--sheet AudioEvents` canonical `--check` | deterministic `audio_events.csv`; no drift in existing generated CSV |
| Static/build | project validation, dependency/include audit, formatting when available, Editor build | schema/runtime/UI compile; no reverse module dependency |
| Automation | atomic catalog/preload failure, invalid rows, bus persistence/defaults and typed Widget bindings | focused tests pass without modifying the user's real settings file |
| UI asset | Unreal Editor/UMG 修改并编译 `WBP_ReEchoSettings`；随后运行 `CompileAllBlueprints` | Parent Class 正确；固定音频控件名称/类型与 C++ 契约一致；0 compile errors、0 failed loads；`.uasset` 不在 Editor 外手改 |
| Audio asset | deterministic tone generation/import and source-path audit | imported diagnostic asset resolves; provenance is recorded; source/import is reproducible |
| Human | PIE diagnostic event and five bus/mute controls from both settings entry points | user reports visible controls plus audible preview/mute/default/apply and restart persistence; Executor supplies only the checklist |
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
- `WBP_ReEchoSettings` 现按 Plan29 正式 UMG 路径静态拥有 `AudioPanel`、五个固定 Bus 行、五个 Slider、五个静音 Checkbox 和诊断音 Checkbox；布局由 WBP 持有，C++ fallback 不再承担正常路径布局。
- `UReEchoSettingsWidget` 的 Checkbox 属性统一为与 WBP 完全一致的 `*CheckBox` 名称；类型化绑定负责状态刷新与事件转发，Graphics/Controls 占位内容保持不变。
- Deterministic 440 Hz generator, Unreal Editor import script and provenance note were added; `UI.Error` is the sole stable catalog binding.
- Catalog atomic failure/quoted-field automation was added to the existing ReEchoAudio test suite.
- `Design/UI/ReEcho_UI修改指导.md` 已补充 Settings 正式 WBP/fallback 边界；新增文档型 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 并从 `CODEBASE_MAP/README.md` 的 `AREA-UI` 路由进入。该文档不注册 Runtime Module；`ARCHITECTURE.md` 已审阅、无需修改，因为运行时拓扑和依赖方向未变化。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoAudio.md` 已审阅、无需再次修改：音频设置权威、服务 API 和依赖方向没有因 UI 资产补全而变化。
- Startup regression fix: the shared manifest reader now enforces `PrimaryKey=Id` only for gameplay registry-owned tables. External module tables such as `AudioEvents(EventId)` remain globally validated by tooling and module loaders without making `FReEchoModule::StartupModule` fatal.
- Battle BGM runtime completion: `BeginEncounter` requests stable state `Music.Encounter`; the catalog resolves `The_Iron_Waltz`; `ResolveActiveWorld()` supplies the active PIE/Game world without introducing an Audio-to-gameplay dependency.
- Audible fade fix: the Unreal backend now keeps Policy Engine's resolved bus gain in `UAudioComponent::VolumeMultiplier` and drives the independent fader from zero to unity. The rejected implementation created the component at zero base volume and therefore remained silent after `FadeIn`.

### Evidence

- `python scripts/data/sync_xlsx_to_csv.py --check` -> PASS across `ReEchoData.xlsx`, `ReEchoEnemyData.xlsx`, and `ReEchoAudioEvents.xlsx`; generated bytes match production CSV.
- `python -m py_compile scripts/data/author_audio_events.py scripts/data/sync_xlsx_to_csv.py scripts/validate_project.py` -> PASS.
- `scripts/ue/Build-Editor.cmd -Configuration Development` -> succeeded after the binding-name correction; UHT/UBT compiled and linked all five Runtime Modules and refreshed the curated Editor bundle.
- Unreal Editor MCP `UMGToolSet` -> `WBP_ReEchoSettings` Parent Class is `/Script/ReEcho.ReEchoSettingsWidget`; 38 instantiated widgets; `AudioPanel` and all eleven bound audio controls exist with exact expected classes, every Slider/Checkbox is a WBP variable; compile and save returned success. `/Game/ReEcho/UI` 下全部 12 个 Widget Blueprint 随后逐一编译成功。
- 独立窗口 PIE + Slate snapshot -> 从 Start Menu 进入 Settings 并点击 Audio 后，运行时实际存在 6 个标签、5 个 Slider 和 6 个 Checkbox；原“Audio 高亮但内容空白”已不再复现。该结果只证明控件可见与层级有效，不替代用户对布局质量和音频可听性的主观验收。
- `python scripts/validate_project.py` -> PASS after the build and documentation refresh (schema, fixtures, modules, XLSX drift, workflow and prebuilt fingerprint).
- `git diff --check` -> PASS for the current candidate.
- Final local publish candidate: `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` -> `Succeeded`; curated five-module Editor bundle refreshed with UE build id `55116800` and source fingerprint `ad0db813b54a`. Follow-up `python scripts/validate_project.py`, canonical XLSX `--check` and `git diff --check` all passed.
- Reported startup Fatal root cause reproduced by inspection: `ReadManifest` incorrectly forced every shared-manifest row to `PrimaryKey=Id`; `AudioEvents(EventId)` was the only issue. The ownership gate fix builds successfully and the production/default CSV test fixture now exercises that manifest contract.
- `scripts/ue/Run-Automation.cmd -Filter ReEcho.Audio` -> automation did not launch: platform preflight stopped after reporting unavailable LinuxArm64/VisionOS SDK metadata. No audio test result is claimed; focused automation remains pending in the user's configured Editor environment.
- IDE diagnostics for edited C++ paths -> no reported diagnostics.
- Audio Insights on the rejected candidate showed `The_Iron_Waltz` receiving play/stop events while the active Sounds view stayed empty. After separating component gain from the fade target, the user confirmed battle BGM is audible in local PIE on 2026-08-17.

### Remaining risks

- Focused audio automation still needs a configured Editor run because this invocation stopped in cross-platform SDK preflight.
- The real diagnostic `.wav`/`.uasset` is intentionally not generated/imported by the coding executor. Until the user runs both scripts, `UI.Error` is a safe no-op.
- `WBP_ReEchoSettings.uasset` 已通过 Unreal Editor/UMG ToolSet 修改、编译和保存，没有在 Editor 外手改；Editor 关闭后以绝对项目路径执行 `CompileAllBlueprints` commandlet，退出码为 0，日志确认 `WBP_ReEchoSettings` 及其他 Blueprint 均编译成功。
- 本任务启动的 Unreal Editor/MCP 已关闭；同克隆 Editor 锁和临时 PIE 截图均已清理，无残留进程或监听端口。
- Full settings-matrix validation and final sound-design balance remain follow-up human PIE checks; battle BGM audibility itself has passed.

### Human validation result/request

`PendingFollowUp`: user has confirmed `Music.Encounter` / `The_Iron_Waltz` is audible in local PIE. Diagnostic-event import plus the complete five-bus mute/default/apply/cancel/process-restart matrix remains explicitly deferred; Plan34 is published as an initial usable audio-system baseline rather than claimed fully closed.

### Planner closure

- 2026-08-17：用户明确决定关闭 Plan34，并把其余资源接入、诊断音替换、完整听感/设置矩阵及审计中新发现的非阻塞加载、衰减应用、author 脚本可复现性和动态软引用 cook 风险转入 Plan46。
- 本次关闭不把聚焦音频自动化、诊断音导入或完整人工设置矩阵伪报为通过；它们保持 `PendingFollowUp`，由 Plan46 的新候选和用户验收重新建立证据。
- 关闭前重新执行 `python scripts/data/sync_xlsx_to_csv.py --check` 与 `python scripts/validate_project.py`，均通过；证据等级仍为 `static verified only`，没有复用为 Plan46 的最终构建、自动化、cook 或 PIE 证据。
