# Plan 28 - Gameplay/UI - automatic and manual player attacks

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Plan28 Executor.
- Plan authored by (AI side): Gavyn-side AI.
- Implementation authored by (AI side): Gavyn-side AI.
- Task status: `InProgress` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `PendingBeforeClose` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Local planning / implementation base: behavior reference `03423bf`; combined adaptation base `origin/main` at `0726bd6`.
- Implementation branch: behavior reference `plan/28-player-attack-modes`; combined adaptation occurs on local `integration/gavyn-umg-gameplay-20260812`.
- Depends on / Blocks: depends on the current run-locked weapon implementation, GAS basic-attack loop and pause-aware encounter clock. It does not block Plan26's read-only Inventory/Stats UI work.
- Writes: PlayerPawn attack-mode/target-selection state; the narrow weapon/enemy queries needed for effective range and deterministic tie-breaking; `Config/DefaultInput.ini`; Plan28 automation; typed Restart-WBP bindings and an embedded attack-mode panel. Old code-built Restart layout and direct viewport ownership are excluded from migration.
- Stable Reads: existing GAS abilities/tags, weapon definitions and ordered attack steps, enemy alive/runtime identity, recorder contract, pause/settings menu behavior and encounter timing.
- Impact mode: `Isolated`. Any public query added for target selection must be additive and must not change the Plan24/26 typed data consumer contract.
- Compatibility promise / downstream action: keep current weapon damage, ordered steps, GAS cooldowns, active-skill input, run-locked WeaponId, recording schema, save schema, Echo behavior and existing B/M/Tab menus unchanged. Reuse the existing pause menu through the latest human-selected `P` entry and preserve its resume/restart/settings/save-and-quit, death and victory behavior. Plan26 may continue in parallel because its declared Writes are disjoint.
- Explicit exclusions: no XLSX/CSV/schema changes; no save-format or recording-format change; no Echo auto-attack change; no runtime weapon switching; no active-skill automation; no new `.uasset`/`.umap` or art; no visual sign-off by the Executor.

## Locked goal

Add mutually exclusive automatic and manual basic-attack modes for the player. A new run defaults to automatic mode. Keep `P` as the single pause entry and add `Automatic Attack` / `Manual Attack` choices to the existing pause menu; choosing a mode updates the authoritative mode and UI but does not resume automatically. The player resumes with the existing Resume action or `P`.

Automatic mode deterministically targets the nearest living enemy inside the current weapon attack's effective range, aims at it and drives the existing GAS basic-attack/cooldown path continuously. Manual mode preserves the existing mouse aim and held left-mouse/`J` basic attack. `Q`/Space active skill remains manual in both modes. Automatic basic attacks are never serialized into Echo recordings.

Attack mode is run-local persisted state: a new run starts automatic, while Continue restores the player's saved choice before gameplay resumes.

## Locked acceptance

- [ ] A fresh run starts `Automatic`; later encounters and Continue restore the run's saved attack mode before accepting gameplay input.
- [x] `P` is the only pause entry: it pauses the world, blocks gameplay abilities and opens the existing `UReEchoRestartWidget`; `Esc` is not retained as a second pause mapping and no second attack-mode modal is introduced.
- [ ] The latest Restart WBP visibly shows the current mode through an embedded attack-mode panel. Choosing either mode does not close or resume; Resume or `P` restores gameplay exactly once.
- [x] Existing start/loadout, trait, shop, inventory, stats, settings, death/victory and pause-menu ownership cannot stack. Adding attack-mode choices does not alter resume, restart, settings, save-and-quit confirmation, save failure, death or victory actions.
- [x] Automatic mode selects the nearest living in-range enemy with a deterministic tie-break, updates aim/facing, holds the existing GAS basic-attack input while a target is valid, releases it when the target dies/leaves range or a menu/mode transition occurs, and reacquires safely.
- [ ] Automatic mode does not attack when no target is in range. Manual mode ignores automatic acquisition and preserves current mouse-facing plus held left-mouse/`J` behavior.
- [x] Both modes use the same current weapon definition, ordered attack step, cooldown, damage and VFX path. Active skill behavior and input remain unchanged.
- [x] Recorder output remains positions plus successful active-skill events only; changing attack mode and automatic/basic attacks create no recording events and require no save/recording revision.
- [x] Focused automation covers only cheap deterministic seams that are faster and more reliable in code: default/mode transitions, deterministic nearest-target selection and the non-recording contract.
- [x] Project validation, Editor Development build, focused/full affected automation and `git diff --check` pass; `.clang-format` was unavailable locally and the diff was manually inspected against repository style.
- [x] The Executor and Planner do not perform PIE, screenshots or visual judgment. The user receives a short ordered test checklist after objective checks pass.
- [ ] The human owner validates the actual automatic/manual behavior, target reacquisition, pause-menu interaction, readability and play feel before Plan closure.
- [x] No generated UE build products or machine-local paths are committed.

## Step 0 gate

- Baseline branch/commit: create the implementation worktree from a freshly fetched, human-approved `origin/main`; record the exact commit. The planning baseline was `6c1f890`.
- Engine/build availability: confirm the ReEcho Editor is saved and closed before C++ build/automation; use installed UE 5.8 only.
- Existing focused-test result: record current project validation and compile availability. Do not spend the Executor pass on broad pre-change automation unless a concrete failure requires diagnosis.
- Active exclusive ownership or shared-contract approval: re-read live Exchange. Plan26's Inventory/Stats files are disjoint; stop if another active task claims Player, GameMode, input, attack-mode UI or weapon/enemy target queries.
- Stop condition if the baseline is broken: do not disguise pre-existing compile/test failures or broaden into save/recording/data/UI refactors. Report the exact blocker to the Planner.

## Implementation outline

1. Introduce a small, testable attack-mode/target-selection unit with an explicit automatic default and deterministic nearest-target tie-break. Do not copy weapon data or parse CSV directly.
2. Let `AReEchoPlayerPawn` own the current mode and the automatic-target/held-basic-input lifecycle. Use the current GAS basic-attack path rather than calling weapon damage separately. Switch aim source between automatic target and mouse without two systems fighting over rotation.
3. Obtain the current ordered attack step's effective range through existing weapon state or the narrowest additive query needed. Do not add a second range constant like Echo's legacy `AutoTargetRange`.
4. Bind a `WBP_ReEchoAttackModePanel` inside the latest `WBP_ReEchoRestart`. Reuse its lifecycle and the UI Flow Coordinator; do not restore code-built layout, direct viewport calls, a top-level screen or a second pause state.
5. Ensure every close, mode switch, death/menu transition and no-target transition releases any synthetic held basic input. Keep active-skill recording unchanged.
6. Add only Plan28's cheap deterministic automation, format changed C++, validate and build. Hand off a runnable result and concise user test checklist without Executor PIE or visual review. Update only this Plan's Execution notes unless scope/ownership/contracts actually change.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py` | Project/source/data/workflow invariants pass |
| Format | Repository `.clang-format` on every changed `.h`/`.cpp` | Only intended formatting changes remain |
| Build | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT exit code 0 with installed UE 5.8 |
| Focused automation | `scripts/ue/Run-Automation.cmd -Filter ReEcho.AttackMode` | Cheap deterministic default, transitions, targeting and non-recording checks pass |
| Integration automation | Planner-selected affected or full `ReEcho.*` after integration, only when the final diff/base warrants it | Integration confidence without delaying the user's earlier hands-on test |
| Whitespace/scope | `git diff --check` and explicit path audit | No whitespace errors, generated products or out-of-scope files |
| Human | User PIE: default auto; `P` pause menu and mode display; auto/manual on ranged and melee weapons; no target/reacquire; resume/restart/settings/save-and-quit coexistence | Executor supplies only the launch/checklist handoff; user records `Passed` or concrete rework before closure |

## Execution notes

### Planner review finding (2026-08-12, rework required)

- Physical left-mouse/`J` input and synthetic automatic held input currently share `BasicAttack` / `StopBasicAttack` without independent source state.
- A physical press can therefore attack in automatic mode with no target, while a physical release can interrupt GAS input without clearing `bAutoAttackInputHeld`, preventing deterministic reacquisition.
- Separate or gate the two input sources, release the correct held source on mode/menu transitions, and cover physical press/release in automatic mode with and without a valid target before returning Plan28 to Review.

### Changed
- `Source/ReEcho/Public/Player/ReEchoPlayerPawn.h` / `.cpp`：新增攻击模式状态（`bAutoAttackMode` 默认 true）、`SetAutoAttackMode`、模拟 held basic-attack input 生命周期（`PressAutoAttackInput`/`ReleaseAutoAttackInput`/`IsAutoAttackInputHeld`）、`UpdateAutoAttack` 每帧驱动、`FindNearestEnemyInRange` 与确定性静态选择 `SelectNearestEnemyInRange`、`FReEchoAttackTargetCandidate` 结构；`Tick` 在自动模式下自动瞄准最近敌人并抑制鼠标覆盖，无目标时回退鼠标瞄准。默认在 `BeginSelectedRun` 重置为自动。
- `Source/ReEcho/Public/ReEchoGameMode.h` / `.cpp`：所有阻塞玩法的菜单通过统一入口释放自动 held input；暂停菜单显示当前模式；新增 `HandleAttackModeAutomaticRequested`/`HandleAttackModeManualRequested`/`ApplyAttackModeChoice`，选择后立即应用并恢复游戏。
- `Source/ReEcho/Public/UI/ReEchoRestartWidget.h` / `.cpp`：新增当前攻击模式文本与“自动攻击/手动攻击”按钮（死亡/胜利/退出确认/保存失败时隐藏），两个 `OnAttackMode*Requested` 委托，`SetAttackMode`/`SetAttackModeSelectable`。
- 只读接口：`AReEchoWeaponActor::GetCurrentAttackRangeCm()`（当前武器当前攻击步骤有效射程）、`AReEchoEnemyActor::GetSpawnIndex()`（稳定目标排序标识）、`AReEchoPlayerPawn::GetRecorder()`。
- `Source/ReEcho/Private/Tests/ReEchoAttackModeTests.cpp`：过滤器 `ReEcho.AttackMode`（Targeting / StateAndHeldInput / NoRecording）。
- `plans/28-player-automatic-manual-attack.md`：按最新人工决定锁定 `P` 为唯一暂停入口并补齐 AI provenance；共享 Exchange 由 Planner 集成时更新。
- Planner 返修：自动 Press 改为幂等 held-input；`SetPlayerMenuAbilityBlocked(true)` 成为所有暂停/阻塞菜单的统一释放点；Plan28 越界的 RunSubsystem、Ability、Combat、Data 与 WeaponRuntime 净修改已撤销。

### Evidence
- 确定性自动化覆盖：默认自动、模式转换、最近目标选择、相同距离稳定排序、无目标结果、模式切换释放 held input、自动普攻/模式切换不产生录像事件。
- 复用 GAS `BasicAttack` 的 held-input 生命周期（冷却/伤害/执行链/武器由现有逻辑决定），未另写伤害、冷却或攻击逻辑。
- 未修改 Save/Recording 结构或版本；未恢复运行时武器切换；攻击模式不进入存档。
- Planner 返修后校验：`python scripts/validate_project.py` 通过；Editor Development build `Result: Succeeded`；完整 `ReEcho` 自动化 38/38 `Result={Success}`，最终 `TEST COMPLETE. EXIT CODE: 0`；`git diff --check` 通过。
- 已知限制：`.clang-format` 不在本机 PATH，格式化步骤未执行（仅人工确保改动风格与上下文一致）。

### Remaining risks
- 自动瞄准仅设置水平朝向（2D 侧视）；远程/近战手感需用户 PIE 验证。
- 当前所有 GameMode 菜单通过统一 ability-block 入口释放 held input；若以后新增不经过该入口的玩法阻断状态，需要显式复用同一释放契约。
- 目标选择在每帧用 `TActorIterator` 遍历敌人，敌人数量极大时可能需空间分区（当前规模无影响）。

### Human validation result/request
- `PendingBeforeClose`：等待用户 PIE 验证后报告 `Passed` 或具体问题。
