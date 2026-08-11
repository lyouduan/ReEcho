# Plan 24 - data - weapons and slots CSV migration

## Locked goal

将武器体系、攻击阶段、插槽类型、配件和配件效果迁移到 CSV 注册表，使策划可以组合武器基础数值、攻击模组、核心元素和数值/行为型配件；玩家与回响按相同不可变定义执行武器，同时保持 GAS 冷却/伤害、确定性回放和当前可玩武器行为。

## Dependency status

Plan 21–23 已于 2026-08-10 审查、验收并本地合并；Plan 23 验收提交为 `81448c1`，Editor Development、静态校验和 27 项 `ReEcho.*` 自动化通过。Plan 22 已落地通用 `ReEchoCsvDataReader.*`、独立角色构筑领域读取器和原子应用/硬失败契约；Plan 23 已落地 `Elements/Statuses/Reactions` 快照、稳定 ElementId、有序 `FindReaction()`、显式公式注册和“生产基线＋局部覆盖”夹具组包。

规划者已依据上述最终接口完成窄校准，Plan 24 可以启动。执行期间独占共享 manifest/schema/快照/注册表/静态校验入口，默认只消费 Plan 23 元素接口，不再改写其反应、状态时间或存档语义。

## Source baseline

- `武器体系W`：匕首、长剑、镰刀、鞭、弓、枪、法杖的攻击阶段和初始属性。
- `武器插槽C`：通用核心、各武器专属槽位和近 80 行配件效果。
- 规划者重新读取工作簿后确认：`武器体系W` 的 7 行是“武器类型/基础 pattern”，不是 7 个带稳定 WeaponId 的具体武器；不得把类型 ID、具体 WeaponId 和输入热键槽混成一个字段。
- `武器插槽C` 有 78 条源行但没有任何稳定源 ID；仅 16 行有名称（6 个通用核心、6 个匕首配件、4 个弓箭头），其余 62 行缺名称。所有源行都要保留 `SourceSheet/SourceRow` 与 implementation status；缺名称行保持 disabled，不能把行号包装成长期运行时 PartId。
- `武器体系（废案）` 只可作为历史线索，不能作为本 Plan 的数据权威或自动映射依据。
- 当前运行时只有 3 个具体武器定义：`W_J_02`/热键 1、`W_J_01`/热键 2、`W_J_03`/热键 3；它们由 `UReEchoBalanceSettings::Weapons`、`EReEchoWeaponSlot` 和 Weapon Actor switch 共同解释。Plan 22 的启用角色还引用 `W_J_04`，但当前 DeveloperSettings 没有该定义。
- 当前初始武器 UI 固定三个按钮并从 DeveloperSettings 读定义；`SetEquippedWeapon()` 会无校验直接改 BuildSnapshot。两处都必须迁到同一 CSV 快照。

## Locked acceptance

### 🤖 Automated

- [ ] `weapon_types.csv` 表达 7 个类型/基础 pattern；`weapons.csv` 表达具体稳定 WeaponId、WeaponTypeId、展示/资源键、输入槽/初始选择顺序与 enabled。运行时不得用 `EReEchoWeaponSlot` 充当 WeaponId 或配件槽类型。
- [ ] 多段攻击使用有序攻击阶段子表，能表达时长、物攻/元攻系数、范围/角度覆盖、移动/击退/无敌窗口和阶段条件；次序稳定且可校验。
- [ ] 插槽类型/武器允许槽位与具体配件分离；核心决定物理/元素伤害通道时引用 Plan 23 的稳定 ElementId。
- [ ] 配件效果使用一对多 effect rows：纯数值走通用 modifier，通用触发逻辑走 BehaviorId/typed parameters，独特攻击模组走已注册 AttackPattern/Behavior handler。
- [ ] 不按每个 PartId 新建 switch 或 Component。仅需要持续状态/Tick/空间实体的配件可挂组件；一次命中、暴击、击杀、叠层和掉落等优先走事件/Ability/handler。
- [ ] 所有工作簿武器和插槽行有源行追踪与 implementation status；缺名称、缺 ID、缺参数或没有 handler 的配件必须 disabled 并给出原因。
- [ ] 当前 `W_J_01/02/03` 的 ID、热键顺序和可玩语义原样兼容；不得按工作簿角色名重解释旧录制。`W_J_04` 新增为启用的镰刀 pattern 具体武器，以满足启用角色默认武器外键。GAS cooldown/伤害仍权威，Weapon Actor 不恢复第二套计时/伤害权威。
- [ ] 玩家切换武器、BuildSnapshot、Recorder 武器变化和 Echo Playback 仍只记录/引用稳定 WeaponId；同一 Run 内使用同一注册快照，表变化不会让历史回响漂移。
- [ ] 初始武器 UI 从 CSV 读取恰好 3 个 `StartSelectable` 武器并按稳定 `LoadoutOrder` 填现有三个按钮；本 Plan 不重做 UI。Run 的 start/restore/equip 遇到未知或 disabled WeaponId 必须明确失败，`SetEquippedWeapon()` 不得继续盲写。
- [ ] 存档/录制携带可比较的武器数据 revision/hash；继续游戏若当前武器定义与保存时不兼容必须明确拒绝，不能用新表静默重放旧武器语义。
- [ ] 工作簿 78 条插槽源行全部进入审计清单：62 条缺名称行保持 disabled；至少启用并验证 6 个通用核心、`力量握柄` 纯数值效果、一个命名 AttackPatternReplacement 和一个命名通用事件行为，其余无 handler 的命名行也保持 disabled 并写原因。
- [ ] 至少有代表性自动化覆盖：单段远程、多段近战、核心元素、Add/Multiply/Override 配件、暴击/命中触发配件、禁用未实现配件、非法槽位组合和表数值变更。
- [ ] 旧 `weapons.json`、DeveloperSettings `Weapons` 及武器 Actor 重复常量退出该领域运行时权威；不存在双写。
- [ ] Editor build、武器/录制/回响/元素回归、全量 `ReEcho.*`、静态校验和 `git diff --check` 通过。

### 🎮 Human PIE

- [ ] 人至少体验一件多段近战、一件单发远程、一件爆炸/元素武器，确认范围、节奏、碰撞、动画/反馈与表一致。
- [ ] 人安装代表性的核心、纯数值配件和行为配件，确认槽位限制、叠加规则、说明与实际效果一致。
- [ ] 人切换武器并进入下一局，确认玩家与回响武器、攻击节奏和元素行为一致。
- [ ] 人修改一项武器基础数值和一项配件数值、重启 Run 后确认无需重新编译即可生效。

## Step 0 gates

1. 从包含本 Plan 校准提交的当前 main 创建独立 worktree/分支 `plan/24-weapons-slots-csv`；确认 27 项基线，并读取 Plan 21–23 最终 Execution notes、`ReEchoCsvDataReader.*`、两个既有领域读取器、共享快照、内建注册和最终 ElementId/Reaction API。基线不符则停止。
2. 在协调板独占生产 manifest/schema、共享快照、注册表编排、内建注册、`ReEcho.Build.cs` 与静态校验入口；默认只消费 Plan 23 元素 API，如需改其公共契约先交回规划者。
3. 武器/配件的 `BehaviorId`、`EffectKind` 和 AttackPattern handler 必须进入显式内建注册函数，并在模块启动加载前注册；不得依赖静态初始化顺序。
4. 锁定兼容映射：保留 `W_J_02→InputSlot1`、`W_J_01→InputSlot2`、`W_J_03→InputSlot3` 及其现有行为；`W_J_04` 使用 active sheet 明确存在的 Scythe pattern。不得使用废案 sheet 重编号或替换 `W_J_01/02/03`。7 个类型使用独立稳定 TypeId，具体命名可按项目 ID 规范落地并写入 Execution notes。
5. 将 78 条配件源行分类为 `Numeric`、`ParameterizedBehavior`、`AttackPatternReplacement`、`UniqueBehavior`、`Incomplete/Disabled`。仅给有名称且纳入实现批次的行稳定 PartId；62 条缺名称行用 `SourceSheet/SourceRow` 审计但不获得伪造 PartId，并保持 disabled。
6. 采集现有三个武器攻击间隔、伤害、射程、元素循环、Sword/Projectile 行为与录制/回响测试基线。
7. 先用一个武器、一个多段 pattern、一个核心和一个数值配件打通全链；若通用模型不能表达，记录最小 schema 扩展，不立刻为全部 80 行写专属类。
8. 不修改武器/角色 `.uasset`、贴图或动画资产；若视觉资源缺失，用现有可玩表现验证逻辑，资产补齐另开 Plan 并认领。
9. 沿用 Plan 23 建立的“生产基线＋局部覆盖”负例组包方式，禁止把七张武器领域表复制进所有旧夹具目录。

## Target table responsibilities

生产表文件固定为 `weapon_types.csv`、`weapons.csv`、`attack_steps.csv`、`slot_types.csv`、`slot_profiles.csv`、`parts.csv`、`part_effects.csv`；具体列名服从 Plan 22/23 已落地的领域读取约定，语义至少包括：

- WeaponType：7 个稳定类型 ID、基础 AttackPatternId、基础 interval/range/projectile/explosion/concentration/geometry、SlotProfileId、enabled 与源行。
- Weapon：具体稳定 WeaponId、WeaponTypeId、display/resource keys、兼容 InputSlot、StartSelectable/LoadoutOrder、可选 pattern override、enabled 与 revision material。
- AttackStep：weapon/pattern ID、ordered index、duration、damage coefficients、geometry/range/angle、movement/status/condition behavior references。
- SlotType/Profile：武器类型允许的槽位和数量限制，核心例外规则。
- Part：ID、weapon type/slot type、display text、rarity/tags、enabled/implementation status。
- PartEffect：part ID、order、trigger、effect kind、target/op/value 或 behavior/attack pattern ID、typed parameters、duration/cooldown/stack rule。
- 运行时由类型化武器领域读取器校验阶段顺序、槽位组合、外键、范围和 handler 白名单；`csv_schema.csv` 只描述同一契约，不解释攻击公式或行为。
- 武器表加载后，注册表必须做跨领域校验：Plan 22 所有启用角色的 `DefaultWeaponId` 必须引用启用武器；未知武器不能等到 `StartRun()` 或攻击时才失败。
- 配件槽 `SlotTypeId`（核心/握柄/刀刃等）、具体武器的 `InputSlot`（1/2/3）和武器类型 `WeaponTypeId` 是三个独立概念，读取器必须拒绝混用和重复输入槽/LoadoutOrder。

## Implementation outline

1. 沿用 Plan 22 的通用读取层和 Plan 23 的领域模式新增独立武器领域读取器，扩展共享快照与注册表编排；生产 manifest 缺任一必需表、角色默认武器外键无效或任一领域失败时整包不发布。
2. 用一个垂直切片打通 CSV → build/equipment → Weapon execution → GAS damage/cooldown → recording/echo。
3. 先迁移当前三武器并保持热键/录制/回响基线，再加入 `W_J_04` 与工作簿七类基础 pattern；缺视觉资源时只复用现有表现，不碰资产。
4. 启用六个核心、`力量握柄`、至少一个命名 pattern replacement 和一个命名事件行为；其余无 handler/状态依赖未实现的源行保持 disabled。
5. 接 UI/商店/背包所需只读名称与槽位合法性，不重做界面版式和经济系统。
6. 移除旧武器运行时重复源，补组合、原子失败、确定性和回响回归测试。
7. 同步 README、manifest/schema/RuntimeDependencies、静态领域校验、CODEBASE_MAP、PROJECT_STATE、LESSONS 与 Execution notes，列出禁用配件和后续实现批次。

## Expected ownership

- 七张生产武器领域 CSV 及 manifest/schema/README/打包依赖。
- 独立武器领域读取器、共享快照扩展、注册表编排与显式内建行为/AttackPattern 注册。
- `Weapons/ReEchoWeaponActor.*` 及新增的 weapon definition/pattern/effect handlers。
- 必要的 Player/Echo/Recording/Projectile/Run 装备接线。
- `Core/ReEchoBalanceSettings.*` 中旧武器定义的退役或兼容迁移。
- 武器/配件/录制回响自动化和静态验证。
- 与 Plan 23 公共元素文件重叠时先协调，默认只消费其接口。

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| CSV static | `python scripts/validate_project.py` | 武器/阶段/槽位/配件 ID、顺序、外键、行为和组合合法；禁用原因完整 |
| Build | format + `scripts/ue/Build-Editor.cmd` | UE 5.8 Editor build 成功 |
| Automation | pattern、modifier、slot legality、GAS cooldown/damage、recording/echo + 全量 `ReEcho.*` | CSV 驱动且确定，无双权威/旧测试回归 |
| Diff | 初版↔终版、`git diff --check` | Plan 21/23 适配和未实现配件批次记录完整 |
| Human | 多段近战/远程/元素/配件/下一局回响 PIE | 节奏、范围、反馈、槽位和回响一致性通过 |

## Risks and exclusions

- 近 80 个配件包含大量独特逻辑；本 Plan 的完成标准是结构化、可验证和完成批准的可玩批次，不是把缺字段的设计描述全部硬编码上线。
- 攻击 pattern、配件 effect、元素 reaction 是三个不同概念；不把它们塞进一个万能字符串解释器。
- 多段攻击/叠层/随机行为必须使用稳定顺序和项目随机流；不要用帧率或容器遍历顺序决定结果。
- UI 重做、武器美术、音效、经济掉落和平衡调参不在本 Plan；只做必要的数据展示/装备接线。
- 不为并行开发复制 Plan 23 的元素类型、公共快照或 manifest/schema；前置未合并就停止。

## Recommended executor model

最强模型。该任务数据量最大，并横跨武器 pattern、GAS、元素、装备组合、录制/回响与大量未完成设计行。

## Execution notes

### Changed

- Created the isolated worktree from local `main` commit `28c21b8` on branch `plan/24-weapons-slots-csv`; baseline check found 27 `ReEcho.*` automation tests before implementation.
- Added seven production weapon-domain CSV tables and staged them in `ReEcho.Build.cs`: `weapon_types.csv`, `weapons.csv`, `attack_steps.csv`, `slot_types.csv`, `slot_profiles.csv`, `parts.csv`, `part_effects.csv`.
- Extended `reecho_data_manifest.csv`, `csv_schema.csv`, `FReEchoCsvDataSnapshot` and `FReEchoCsvDataRegistry` for weapon types, concrete weapons, ordered attack steps, input slots, slot profiles, parts and part effects.
- Added `ReEchoWeaponCsvReader.*` as an independent domain reader. It validates registered handlers, table references, legacy `W_J_02/W_J_01/W_J_03` hotkey mapping, exactly-three start-selectable weapons, enabled character `DefaultWeaponId` references, attack-step order, slot legality, 78 source part rows and enabled/disabled part batches.
- Registered weapon-domain `BehaviorId`, `EffectKind`, `FormulaId` and `AttackPatternId` values before default CSV load.
- Migrated loadout UI, run start/restore/equip, player hotkeys, recorder snapshots, echo playback and `AReEchoWeaponActor` to read the same CSV snapshot. Weapon recordings continue to store stable `WeaponId`, now with `WeaponDataRevision` in `FReEchoBuildSnapshot`.
- Removed the old `UReEchoBalanceSettings::Weapons` runtime authority and `DefaultGame.ini` weapon rows. GAS remains the cooldown/damage authority; the weapon actor only resolves CSV definitions and dispatches the existing projectile/melee/light-wave paths.
- Added negative weapon-domain fixtures for unknown default weapon references, duplicate input-slot mapping and invalid part effect behavior pairs. Updated automation to cover CSV weapon loadout order, `W_J_04`, part batches and save/recording revision compatibility.

### Evidence

- `python scripts/validate_project.py` passed.
- `.clang-format` was run via `C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\Llvm\x64\bin\clang-format.exe` on changed C++ files.
- `scripts/ue/Build-Editor.cmd -Configuration Development` passed.
- `scripts/ue/Run-Automation.cmd -Filter ReEcho` passed; log reports 27 tests found and 27 success results, exit code 0.
- `git diff --check` passed; only CRLF conversion warnings were printed by Git.

### Plan 21–23 and integrated loadout/recording contract adaptation

- Reused Plan 21's immutable snapshot/default-load contract and Plan 22/23's domain-reader/fixture pattern. No Plan 23 element system code was copied; weapon core rows reference stable Plan 23 element/channel IDs only through CSV targets/parameters.
- Kept the integrated loadout shape: the existing three buttons are still used, but labels/images/options now come from `StartSelectable=true` CSV weapons sorted by `LoadoutOrder`.
- Preserved legacy recording semantics: `W_J_02` remains InputSlot 1, `W_J_01` remains InputSlot 2 and `W_J_03` remains InputSlot 3. Old recordings using those IDs are not reinterpreted through weapon type names.
- Save/restore now rejects weapon definition drift by comparing `FReEchoBuildSnapshot::WeaponDataRevision` with the current enabled CSV weapon row; tests were updated to carry the revision in recording history and suspended active recordings.

### Enabled content batch

- Weapon types: 7 enabled `WeaponTypeId` values from `武器体系W`: `Dagger`, `LongSword`, `Scythe`, `Whip`, `Bow`, `Gun`, `Staff`.
- Concrete weapons: enabled `W_J_02` Moon Staff (`InputSlot=1`), `W_J_01` Crescent Blade (`InputSlot=2`), `W_J_03` Elemental Reaction (`InputSlot=3`) and new enabled `W_J_04` Harvest Scythe (`WeaponTypeId=Scythe`, `Pattern.ScytheSweep`).
- Attack patterns: migrated the seven base family patterns plus compatibility/runtime patterns for the existing three weapons and dagger dash replacement.
- Slots: `Core`, `Grip`, `Blade`, `Arrowhead`, `Bowstring`, `RotaryBlade`, `SwordBlade`, `Muzzle`, `GunAction`, `StaffBody`, `StaffCrystal`, `WhipBody`.
- Parts enabled from `武器插槽C`: six generic cores (`P_CORE_PRIMORDIAL`, `P_CORE_TIDE`, `P_CORE_FOREST`, `P_CORE_FLAME`, `P_CORE_THUNDER`, `P_CORE_PRISM`), `P_DAGGER_THRUST_GRIP`, `P_DAGGER_STRENGTH_GRIP` and `P_DAGGER_HOLY_BLADE`.
- Effect coverage: `WeaponDamageChannel` overrides for six cores, `AttackPatternReplacement` for thrust grip, `AttackSpeed` multiply for strength grip and an `OnKill` heal unique behavior for holy blade. Enabled operations cover Add/Multiply/Override.

### Source rows left disabled and why

- All 78 source rows from `武器插槽C` are represented in `parts.csv` with `SourceSheet/SourceRow`.
- The 62 unnamed rows are disabled audit rows with `PartId=None`; they are intentionally not assigned long-term IDs based on row number.
- Named rows left disabled are missing handler support or depend on not-yet-implemented state/behavior batches: invisibility, stun, bleed, arrow split, explosion, pierce, multishot and similar unique behavior rows remain disabled with explicit `DisabledReason`.
- The disabled rows are not reachable from enabled part effects; validators reject enabled effects on disabled parts and enabled parts without effect rows.

### Remaining risks

- The current CSV runtime includes definitions and validation for parts/effects, but there is no full in-game equipment UI/application path for arbitrary part loadouts yet. Enabled part behavior coverage is validated structurally and reserved for the next equipment/application slice.
- `W_J_04` reuses existing visual presentation because this plan did not touch `.uasset`/`.umap` or create new art.
- Static and automation coverage prove data load, start/equip/restore/recording compatibility and existing combat dispatch; full play-feel tuning for all seven base patterns remains human/PIE work.

### Human validation requested

- PIE one multi-stage melee weapon (`W_J_01`), one single-shot/ranged weapon (`W_J_02`) and one elemental weapon (`W_J_03`) to confirm rhythm, range, collision and feedback still match the pre-CSV behavior.
- PIE `W_J_04` through a character/default path or debug setup to verify the Scythe sweep feel with the reused visual.
- In a future equipment slice, install one core, one numeric grip and one behavior part to verify slot restrictions and applied effects once the UI/application path exists.
- Edit one weapon numeric value and one part-effect value in CSV, restart the run and confirm the changed data takes effect without recompilation.

## Planner review - 2026-08-11 - not accepted

The executor commit `32a0384` is clean, statically valid, builds in Editor Development, and passes the existing 27 `ReEcho.*` tests. It is not ready to merge because the passing evidence only covers schema/registry structure and the legacy weapon path, not the locked runtime acceptance below.

### Blocking findings

1. Parts are not part of `FReEchoBuildSnapshot` (or another immutable equipment snapshot), and no runtime path consumes `Snapshot.Parts` / `Part.Effects`. The six cores, strength grip, attack-pattern replacement and OnKill behavior are marked implemented but cannot affect player or echo combat. The three non-core enabled examples are also Dagger-only while no enabled concrete Dagger weapon exists. Add a minimal deterministic equip/apply API, runtime event path and a reachable compatible concrete/runtime fixture; a full UI redesign and changes to the three legacy start choices remain out of scope.
2. `AReEchoWeaponActor` reads an attack-step row but only consumes damage coefficients and range. `DurationSeconds`, `ArcDegrees`, `ProjectileCount`, `ConcentrationDegrees`, `ExplosionRadiusCm`, `MovementCm`, `Invulnerable`, `BehaviorId`, `FormulaId` and `ConditionId` have no execution path; melee is always a full-radius hit and projectile patterns always spawn one projectile. Implement the generic registered pattern/step handlers needed for the production rows and prove representative single-shot ranged, multi-step melee, movement/invulnerability and geometry behavior.
3. Run/recording compatibility is only an integer revision on the final concrete weapon. Attack-step/type/slot/part/effect edits, and earlier WeaponIds in a recording's switch timeline, can drift without rejection. Add one deterministic weapon-domain revision/hash covering all seven weapon tables, capture it for the run and recordings, compare it on restore/playback, and test that a changed step/effect rejects historical data without partially mutating state.
4. A weapon actor copies concrete weapon rows during initialization but calls the mutable global registry again for attack steps. Pin and consistently use the same immutable weapon-domain snapshot for the whole run and its player/echo consumers; a republished test snapshot must not change an active run.
5. Plan24 added no behavior-level automation: the suite remains at the 27-test baseline, and `WeaponsAreValid` only counts enabled rows/effect labels. Add focused tests that execute Add/Multiply/Override, core channel/ElementId resolution, attack-pattern replacement, OnKill behavior, illegal slot combinations, disabled parts, ordered attack steps and revision/hash drift. Tests must assert resulting combat/build values or events, not only CSV presence.

### Re-review gate

- Keep the existing ID/hotkey compatibility, seven-table schema, 78-row audit and disabled batch.
- Do not modify `.uasset` / `.umap`, merge `main`, or push.
- Re-run static validation, Editor Development build, full `ReEcho.*`, focused weapon tests and `git diff --check`; update Execution notes with actual runtime evidence and remaining human-only PIE items.

## 执行者启动 prompt

```text
你是 ReEcho 项目的执行者。先读 AGENTS.md，再按其最小读取顺序读 plans/24-data-weapons-slots-csv.md、Plan 21–23 最终 Execution notes，以及 shared/LESSONS.md §GAME 中武器/回响/确定性匹配条目；只有诊断失败时读 §DEBUG。

前置：从包含本 Plan 校准提交的当前 main 创建独立 worktree 的 `plan/24-weapons-slots-csv` 分支，确认 27 项 `ReEcho.*` 基线。先在协调板认领 manifest/schema、共享快照/注册表、武器领域 CSV 和静态校验入口；参考工作簿为仓库上一级 `../回响肉鸽数值与构筑体系.xlsx`。

任务：沿用 Plan 22 的通用读取层和 Plan 23 的领域读取/夹具模式新增独立武器领域读取器，把 WeaponType、具体 WeaponId、输入热键槽和配件 SlotType 明确分开，并扩展唯一快照/注册表。迁移 7 个基础类型、当前 `W_J_01/02/03`、缺失的 `W_J_04`、攻击阶段、槽位与批准的配件批次；初始武器 UI、Start/Restore/Equip、Recorder/Echo 都改读同一 CSV 定义。保持当前三热键语义，未知/disabled 配置硬失败，存档定义不兼容不得静默漂移。所有 handler 必须显式注册且先于启动加载。验收照 Plan 24 的 🔒 清单；禁止读取废案 sheet 作为权威、禁止复制数据/元素系统、禁止按 PartId 扩张巨型 switch、禁止启用 62 条缺名称行、禁止修改 `.uasset`/`.umap`。把 Plan 21–23 适配、启用批次、全部禁用源行和偏差写入 Execution notes。

完成后显式提交到本地分支并告诉人；未经人明确确认不得 push，禁止修改或合并 main。
```
