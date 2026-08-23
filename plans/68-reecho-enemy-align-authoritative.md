# Plan 68 - 程序 - 怪物系统对齐权威表(怪物.xlsx)

> 本 Plan 为「全面对齐 `怪物.xlsx`」的总纲（umbrella），拆为 5 个 workstream（WS1..WS5），每个 workstream 在专属 worktree（`plan/68-wsN-*`）实现、验证、合并回 main。所有数值以 `C:\Users\gavynqiu\Documents\miniGame\怪物.xlsx` 为唯一权威源，经 `ReEchoEnemyData.xlsx` → `sync_xlsx_to_csv.py` → `Content/Data/*.csv` → 不可变运行时快照，运行时绝不直读 xlsx。

## 协调

- Planner 负责人：Gavyn-side AI（我，程序路线 Planner+Secretary）。
- Executor 负责人：Gavyn-side AI。
- Plan 编写方（AI 侧）：`Gavyn-side AI | ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Proposed`。
- 人工验收：`PendingBeforeClose`（SHEEP 两阶段表现/手感需 PIE 判断；普通怪双形态表现需确认）。
- 本地规划 / 实现基线：origin/main（fetch 2026-08-20，本地与远端 0/0 一致；远端最大 Plan 编号 = 66，本地 `plans/67-shop-drop-table-overhaul.md` 为无关未发布草稿，故本 Plan 取 68 避开）。
- 本地实现方式：每 WS 一个 worktree，验证后 fast-forward 合并回 main，main 保持仅接收合并。
- 依赖 / 阻塞：无外部阻塞；`怪物.xlsx` 已更新且为权威源。
- Writes：
  - `Design/Data/ReEchoEnemyData.xlsx`（Enemies 表加双形态列 + 新增 EnemyCombatStats sheet）
  - `scripts/data/sync_xlsx_to_csv.py`（扩展 Enemies 列映射 + 新增 EnemyCombatStats→enemy_combat_stats.csv）
  - `Content/Data/enemies.csv`、`Content/Data/enemy_combat_stats.csv`、`Content/Data/enemy_abilities.csv`、`Content/Data/boss_phases.csv`、`Content/Data/encounters.csv`
  - `Source/ReEcho/Private/Data/ReEchoEnemyDefinitionCompiler.cpp`（删 Plan65 写死桥接 + Compile 加 CombatIndex + 读双形态列 + 读 SHEEP 阶段）
  - `Source/ReEcho/Private/Data/ReEchoEnemyCsvReader.cpp`（HasExactColumns 加列 + 新增 ReadEnemyCombatStats + Row 结构加字段）
  - `Source/ReEcho/Private/Data/ReEchoCsvDataRegistry.cpp`（注册 enemy_combat_stats.csv）
  - `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyTypes.h`（FReEchoBossPhaseDefinition 加 TriggerHealthFraction；FReEchoEnemySenseSnapshot 加 bInCombat/HateRangeCm）
  - `Source/ReEchoEnemies/Private/Enemies/ReEchoEnemyLogicComponent.cpp`（双形态数据阈值 + IdleWander + SHEEP HP 阶段触发）
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`、`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
- Stable Reads：
  - `Source/ReEcho/Private/Encounter/ReEchoEncounterRuntime.cpp`（SpawnResolver 携带 CombatIndex）
  - `Source/ReEchoEnemies/Private/Enemies/ReEchoEnemyPresentationComponent.cpp`（HandlePhaseTransition 现有 Phase2 路径复用）
  - `Source/ReEchoCombat`（确认 BossIntent.PhaseDefinition 乘数被消费）
  - `Source/ReEcho/Private/Data/ReEchoEncounterCsvReader.cpp`（encounters.csv BossEnemyId 列）
  - `MOD-ReEchoCombat.md`、`MOD-ReEchoWeapons.md`、`MOD-ReEchoAudio.md`、`MOD-ReEchoUI.md`（Stable Read）
- 影响模式：`SharedContract`（修改 enemies.csv schema、新增 enemy_combat_stats.csv、修改 Compile 签名、SenseSnapshot 加字段 → 跨模块契约）。
- 兼容承诺 / 下游操作：
  - 不删 enemies.csv 现有列，仅**追加**双形态列（向后兼容，但 `HasExactColumns` 强校验要求 reader 与 csv 同步，否则 `validate_project.py` 失败）。
  - 运行时只读 CSV；任何数值改动必须先进 xlsx 再 sync，禁止手改 csv 作为真源。
  - 每 WS 合并前跑 `validate_project.py` + 增量 `Build-Editor` + 聚焦测试。
- 明确排除：
  - 不修改 `MOD-ReEchoCombat` 伤害公式核心；不引入 `Enemies→Weapons` 反向依赖。
  - 不做 SHEEP 新动画/特效/音频（v1 复用现有 Boss 表现路径，正式 VFX/音频留后续 Plan）。
  - 不改动确定性模拟语义（60Hz 模拟 / 20Hz 录制 / 暂停不推进）。
  - SLIME/RABBIT/FOX 双形态**不改数值**（维持用户裁定「双形态只是表现」）。

## 锁定目标

玩家可见结果：
1. 阵容对齐权威表：普通怪 = SLIME / RABBIT / FOX；Boss = SHEEP（两阶段，白→黑）。
2. 普通怪双形态 = 纯表现变身（触发 = 距离进入变身范围 **或** 受击次数达阈值），切 Phase2 + 播 `Transform_Phase2`，数值不变。
3. SHEEP Boss 两阶段 = 实装数值：血清空触发二阶段；二阶段伤害 ×1.5、冷却 ×0.7；四技能 SH_01..04 可释放。
4. 怪物数值随场次（战斗 1..8）成长（按 `(怪物ID, 战斗编号)` 查表覆盖 MaxHealth/ATK/间隔）。
5. 未进入战斗的怪物 = 无规则游走；进入战斗后正常接敌。

流水线结果：所有上述数值经 `ReEchoEnemyData.xlsx` → `sync` → CSV → 不可变快照，可被 `validate_project.py` 与聚焦测试断言。

## 架构影响与设计决策

- 受影响架构标识：
  - `MOD-ReEchoEnemies`（核心：EnemyLogic / 双形态 / Boss 招式 / 阶段 / 游走 / 定义编译）
  - `AREA-Data`（`MOD-ReEcho` 内：CSV→Definition 编译、新增 enemy_combat_stats 表）
  - `AREA-Encounter`（`MOD-ReEcho` 内：SpawnResolver 携带 CombatIndex）
  - `AREA-Enemies`（`MOD-ReEcho` 内：EnemyHost 创建敌人时注入 CombatIndex）
  - `MOD-ReEchoCombat`（确认 BossPhase 乘数消费；如加 HP 触发字段则需 Writes）
  - `MOD-ReEchoWeapons`（Stable Read；若 SHEEP SH_02 法杖弹走武器系统投射物则需 Writes，WS4 确认）
  - `MOD-ReEchoAudio` / `MOD-ReEchoUI`（Stable Read，v1 不改）
- 对应模块文档（加入 Writes）：
  - `MOD-ReEchoEnemies.md`：新增「双形态数据驱动」「按场次成长」「IdleWander」「SHEEP Boss 数据契约」章节。
  - `MOD-ReEcho.md`：AREA-Data 段补 `enemy_combat_stats` 表与 Compile(CombatIndex) 契约；AREA-Encounter/AREA-Enemies 段补 SpawnResolver/Host 携带 CombatIndex。
  - `MOD-ReEchoCombat.md`：如确认乘数消费则注明；如加 TriggerHealthFraction 则 Writes。
  - `MOD-ReEchoWeapons.md`：若 SHEEP 技能弹走武器则改，否则 Stable Read。
- 设计意图：
  - 数据唯一真源仍是 xlsx→csv；运行时只读不可变快照（维持 `CODEBASE_MAP` 数据链）。
  - 双形态普通怪仅表现（用户裁定），触发阈值由数据驱动（移除 Plan65 写死占位 `TriggerRangeCm=300/RequiredAttackCount=2/TransformSeconds=1`）。
  - SHEEP 两阶段实装数值（用户确认）：复用现有 `FReEchoBossPhaseDefinition` 的 `Physical/ElementalAttackMultiplier` 与 `AttackSpeedMultiplier`（=0.7 实现冷却×0.7），新增 `TriggerHealthFraction` 实现血清空触发。
  - 成长：新增 `enemy_combat_stats.csv`，`Compile` 按 `(EnemyId, CombatIndex)` 在编译期覆盖 `MaxHealth/AttackIntervalSeconds/ContactDamage`，保持每实例不可变快照语义（不每帧查表）。
  - 游走：新增 `IdleWander` 行为；战斗态由 Host 经 `FReEchoEnemySenseSnapshot` 注入 `bInCombat` / 仇恨范围，Logic 不发现 GameMode/PlayerController（维持模块边界）。
- 权威状态与依赖：不引入新 Runtime Module；不新增 `Enemies→Weapons` 反向依赖；Boss 招式策略仍由 `MOD-ReEchoEnemies` 拥有。
- 决策记录：
  - SHEEP 复用现有 `Boss` 原型（不加 `EReEchoEnemyArchetype` 枚举），靠 `BehaviorProfileId=Boss.Sheep` / `PresentationId=Enemy.Sheep` / `BossPhases` 区分 TimeGuard → 最小类型面。
  - 成长覆盖点在 `Compile` 期（非运行时每帧），保持不可变快照。
  - 双形态普通怪不改数值（裁定）；SHEEP 改数值（用户 2026-08-20 确认：SHEEP 实装数值，普通怪表现-only）。
  - 本 Plan 显式 override 当前「Demo 稳定化、不扩张阵容」纪律（用户选择 A 全量对齐的 deliberate 决策）。
- 相关文档同步范围：
  - `ARCHITECTURE.md`：新增 `enemy_combat_stats` 数据表注释。
  - `README.md`：若列新表则补。
  - `MOD-ReEchoEnemies.md`、`MOD-ReEcho.md`、`MOD-ReEchoCombat.md`、`MOD-ReEchoWeapons.md`、`MOD-ReEchoAudio.md`(Read)、`MOD-ReEchoUI.md`(Read)。
- 关闭前逐项填写审阅结果：（实现后回填）

## 锁定验收

- [ ] 阵容对齐：enemies.csv 仅含 M_SLIME / M_RABBIT / M_FOX / M_SHEEP；Grunt/Shield/Bomber 数据移除；encounters.csv 第8关 `BossEnemyId` = TimeGuard → SHEEP。
- [ ] 双形态：SLIME/RABBIT/FOX 触发 = 距离 / 受击，纯表现（数值不变），播 `Transform_Phase2`。
- [ ] SHEEP：两阶段血清空触发；二阶段伤害 ×1.5、冷却 ×0.7；SH_01..04 可释放且与 `怪物.xlsx` 数值一致。
- [ ] 成长：战斗 1..8 的 MaxHealth/ATK 与 `怪物.xlsx` 成长表一致（聚焦测试断言：SLIME 战斗1=28 / 战斗8=112，FOX 战斗1=55 / 战斗8=220）。
- [ ] 游走：未战斗怪物游走；进入战斗后正常接敌；Logic 不引用 GameMode。
- [ ] `python scripts/validate_project.py` 通过；`Build-Editor` 通过；`Run-Automation -Filter ReEcho` 受影响报告通过。
- [ ] `MOD-ReEchoEnemies.md` / `MOD-ReEcho.md` 已更新并审阅。

## Step 0 门禁

- 基线分支/提交：origin/main（fetch 2026-08-20，0/0）。
- 引擎/构建可用性：需 UE 5.8 标准安装版；发布前 `-FullRebuild` 门禁。
- 现有聚焦测试结果：（实现 WS 前 baseline 跑一次记录）。
- 共享契约 / 难合并资源风险：enemies.csv schema 变更触及 `HasExactColumns` 强校验；多 WS 共用 xlsx/csv，需顺序合并避免冲突。
- 基线损坏时的停止条件：任一 WS 构建/validate 失败即停，回退该 worktree，不污染 main。

## 实现提纲

### WS1 数据模型（AREA-Data / MOD-ReEchoEnemies Writes）
1. `ReEchoEnemyData.xlsx` → Enemies 表追加列：`Phase2TriggerReason`(None/Range/AttackCount)、`Phase2TriggerRangeCm`、`Phase2RequiredAttackCount`、`Phase2TransformSeconds`。
2. `ReEchoEnemyData.xlsx` → 新增 sheet「EnemyCombatStats」（★主交付★）：列 `EnemyId, CombatIndex, MaxHealth, AttackIntervalSeconds, ContactDamage`，行覆盖 SLIME/RABBIT/FOX 战斗 1..8（数值取自 `怪物.xlsx` 怪物属性和投放成长表）。
3. `scripts/data/sync_xlsx_to_csv.py`：扩展 Enemies 列映射；新增 `EnemyCombatStats` → `enemy_combat_stats.csv`。
4. `ReEchoEnemyCsvReader.cpp`：`HasExactColumns(Enemies)` 加入 4 个新列；`FReEchoCsvEnemyRow` 加 Phase2 字段；新增 `FReEchoCsvEnemyCombatStatRow` + `ReadEnemyCombatStats`。
5. `ReEchoCsvDataRegistry.cpp`：注册 `enemy_combat_stats.csv`。
6. 纯数据，不改逻辑；跑 `validate_project.py`。

### WS2 双形态数据驱动（MOD-ReEchoEnemies）
1. 删 `ReEchoEnemyDefinitionCompiler.cpp:127-134` Plan65 写死桥接。
2. 改读 Enemies 新列：`Phase2.TriggerRangeCm = Phase2TriggerRangeCm`；`Phase2.RequiredAttackCount = Phase2RequiredAttackCount`；`Phase2.TransformSeconds = Phase2TransformSeconds`；`Phase2.bEnabled = (Phase2TriggerReason != None)`。
3. 普通怪（非 Boss）双形态保持表现-only：`AdvancePhaseTransition` 仅置 `CurrentPhaseIndex=2`（现状已如此，确认不改数值）；`TryBeginPhaseTransition` 用数据阈值。
4. 聚焦测试：SLIME 受击 2 次 / 距离 < 变身范围 触发 Phase2，MaxHealth 不变。

### WS3 按场次成长（AREA-Data + AREA-Encounter + MOD-ReEchoEnemies）
1. `ReEchoEnemyDefinitionCompiler::Compile` 签名加 `int32 CombatIndex`（或重载）；编译时查 `EnemyCombatStats` 覆盖 `MaxHealth/AttackIntervalSeconds/ContactDamage`。
2. `AREA-Encounter` SpawnResolver / GameMode coordinator 创建敌人时携带 `CombatIndex`（1-based 遭遇序号，单局 1..8）。
3. 每个生成敌人按其 `CombatIndex` 编译出独立不可变 `FReEchoEnemyDefinition`。
4. 聚焦测试：断言 SLIME 战斗1 MaxHealth=28、战斗8=112；FOX 战斗1=55、战斗8=220。

### WS4 SHEEP Boss + 四技能 + 二阶段换算（MOD-ReEchoEnemies + MOD-ReEchoCombat + AREA-Data）
1. `enemies.csv` 增 `M_SHEEP`（Archetype=Boss, BehaviorProfileId=Boss.Sheep, PresentationId=Enemy.Sheep, MaxHealth=2600）。
2. `enemy_abilities.csv` 增 SH_01..04（数值以 `怪物.xlsx` 全怪物技能表为准：SH_01 平A 伤害6/前摇0.5/生效0.3/后摇0.5/冷却1.5；SH_02A 站定弹 伤害12/前摇0.8/生效0.4/后摇0.6/冷却3/弹数3/散射角20；SH_02B 移动弹 伤害8/前摇0.6/生效0.4/后摇0.4/冷却2/弹数3/散射角15；SH_03 闪身 冷却5/位移260；SH_04 祷告光束 伤害30/前摇1/生效1.5/后摇0.8/冷却6/长600/宽40）。
3. `boss_phases.csv` 增 M_SHEEP 两阶段：PhaseIndex=2，`TriggerHealthFraction`（新增字段，非正=禁用时间触发）= 二阶段触发血线；`PhysicalAttackMultiplier=1.5`、`ElementalAttackMultiplier=1.5`、`AttackSpeedMultiplier=0.7`、`RefillHealthPolicy=None`。
4. `ReEchoEnemyTypes.h`：`FReEchoBossPhaseDefinition` 加 `float TriggerHealthFraction = 0.0f`（<=0 表示不按血量触发，维持原 `TriggerSeconds`）。
5. `MOD-ReEchoCombat`：确认 `BossIntent.PhaseDefinition` 乘数被消费（TimeGuard 已用，应直接复用）；如缺口则补 Writes。
6. `encounters.csv`：第8关 `BossEnemyId` = TimeGuard → SHEEP。
7. 表现：SHEEP 复用现有 Boss 表现路径；黑色形态播 `Transform_Phase2` 同类动画；新动画/特效/音频留后续 Plan。
8. 聚焦测试：SHEEP 血清空切二阶段；二阶段伤害 ×1.5、冷却 ×0.7。

### WS5 未战斗游走（MOD-ReEchoEnemies）
1. `ReEchoEnemyTypes.h`：`FReEchoEnemySenseSnapshot` 加 `bool bInCombat = false` 与 `float HateRangeCm = 0.0f`（由 Host 注入）。
2. `ReEchoEnemyLogicComponent`：新增 `IdleWander` 行为（玩家距离 > 仇恨范围 且 未受击 → 无规则游走，不消费攻击候选）；进入战斗后转 `Pursuing`。
3. Host（EnemyHost/Sense 注入点）按「玩家距离 < 仇恨范围 或 已受击」置 `bInCombat`，不引入 GameMode 引用。
4. 聚焦测试：未战斗怪物游走；玩家靠近后接敌。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | 项目/源码不变量通过（含 CSV schema） |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码 0 |
| 运行时行为 | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | 受影响报告通过 |
| 数据断言 | 聚焦测试（WS2/3/4/5） | 双形态/成长/SHEEP/游走行为符合 |
| 人工 | 具名 PIE 任务 | SHEEP 两阶段与双形态表现确认 |

## 执行记录

### 变化
（各 WS 合并后回填）

### 证据
（各 WS 验证输出回填）

### 剩余风险
- 多 WS 共用 xlsx/csv，需顺序合并避免 `HasExactColumns` / sync 冲突。
- SHEEP 技能弹（SH_02）若走武器系统投射物，需确认 `MOD-ReEchoWeapons` 契约，可能扩 Writes。
- 「全局技能间隔 1.5→1.0」需映射为 Boss 级间隔缩放，WS4 实现时定。

### 人工验收结果/请求
（PIE 后回填）

### 架构文档审阅结果
（关闭前回填）

---

## WS4/WS5 恢复交接记录（2026-08-22）

> 本段为「转交另一模型执行」的交接说明。工作树：`ReEcho-plan68-ws45-recovery`（分支 `plan/68-ws45-recovery`，基线 a281744）。本节描述当前真实进度，与上文较旧的 WS4 提纲有出入处以本节为准。

### 背景与方向变更（相对上文旧提纲）

- **Boss 方向已改**：以最新 `怪物.xlsx` 为准，boss 是**羊 M_SHEEP**；原 `M_TimeGuard` **废除**。上文 WS4 提纲里写的 `BehaviorProfileId=Boss.Sheep / PresentationId=Enemy.Sheep / MaxHealth=2600` 等已被实际实现取代——SHEEP 复用现有 `Boss.TimeGuard` 的 BehaviorProfile/Presentation（profile 名不改，仅 enemy id 换），MaxHealth=**1300**。
- **二阶段触发方式**：由「血清空百分比阈值(`TriggerHealthFraction`)」改为「**血条清空（HP 降到 0）变身**」，二阶段回满血到 **650** 再战（方案 i）。SH_04 冷却用默认 **13s**。
- 因此下文「锁定验收」「WS4 提纲」中关于 TriggerHealthFraction / MaxHealth=2600 / SH_01..04 旧数值的条目，一律以本节为准。

### 一、C++ 代码改动（已全部落位 + 增量 Development 构建 Result: Succeeded）

| 文件 | 改动 |
|---|---|
| `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyTypes.h` | `EReEchoEnemyPhaseTriggerReason` 加 `HealthDepleted`；新增 `enum class EReEchoEnemyPhase2TriggerMode : uint8 { AttackCountOrRange, HealthThreshold }`；`FReEchoEnemyPhaseDefinition` 加 `TriggerMode`(默认 AttackCountOrRange) + `float HealthThresholdRatio=0.0f`；`FReEchoEnemySenseSnapshot` 加 `float CurrentHealthRatio=1.0f`（host 注入血量比）。 |
| `Source/ReEcho/Public/Data/ReEchoCsvDataRegistry.h` | `FReEchoCsvBossPhaseRow` 加 `float PhaseMaxHealth=0.0f`；`FReEchoCsvEnemyRow` 加 `FString Phase2TriggerMode` + `float Phase2HealthThresholdRatio=0.0f`。 |
| `Source/ReEcho/Private/Data/ReEchoEnemyCsvReader.cpp` | ReadBossPhases `HasExactColumns` 加 `PhaseMaxHealth` 并 `RequireFloat`；ReadEnemies `HasExactColumns` 加 `Phase2TriggerMode/Phase2HealthThresholdRatio` 并解析。**移除了对 `Graybox/ReEchoEnemyActor.h` 的 include**（违反模块边界）。 |
| `Source/ReEcho/Private/Data/ReEchoEnemyDefinitionCompiler.cpp` | Phase 循环加 `Phase.PhaseMaxHealth = PhaseRow.PhaseMaxHealth;`；Phase2 赋值按 `Equals("HealthThreshold")` 解析 TriggerMode + `Phase2.HealthThresholdRatio = Row->Phase2HealthThresholdRatio;`。 |
| `Source/ReEchoEnemies/Private/Enemies/ReEchoEnemyLogicComponent.cpp` | `TryBeginPhaseTransition` 加第三条触发路径 `bHealthDepleted`（TriggerMode==HealthThreshold 且 `IsHealthAtOrBelowPhase2Threshold(Sense.CurrentHealthRatio)`）；`IsHealthAtOrBelowPhase2Threshold(float)` 改为纯读 Sense 传入值（不查 Combatant）；新增 `TryTriggerPhase2OnFatalWound()`（构造 CurrentHealthRatio=0 的 Sense 调 TryBeginPhaseTransition）。 |
| `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyLogicComponent.h` | 声明 `bool TryTriggerPhase2OnFatalWound();` + `bool IsHealthAtOrBelowPhase2Threshold(float) const;`。 |
| `Source/ReEchoCombat/Public/Combat/ReEchoCombatantComponent.h` | `DECLARE_DELEGATE_RetVal_OneParam(bool, FReEchoFatalDamageIntercept, float&)`；加成员 `FReEchoFatalDamageIntercept OnFatalDamage;` + `SetFatalDamageInterceptDelegate(...)` + `TryDeferFatalDamageForPhaseTransition(float& InOutHealth)`。 |
| `Source/ReEchoCombat/Private/Combat/ReEchoCombatantComponent.cpp` | fallback 与 GAS(`HandleHealthChanged`) 两条死亡路径在 `OnDeath.Broadcast()` 前调用 `TryDeferFatalDamageForPhaseTransition(InOutHealth)`；GAS 路径若延迟则 `SetNumericAttributeBase` 钉住血量 + 移除 State_Dead tag。 |
| `Source/ReEcho/Private/Graybox/ReEchoEnemyActor.cpp` | `ConfigureFromDefinition` 末尾注册致命伤拦截 lambda（判断 bEnabled+HealthThreshold+未变身 → `TryTriggerPhase2OnFatalWound()`）；tick 注入 `Sense.CurrentHealthRatio = Clamp(Combatant->CurrentHealth/Stats.HpMax,0,1)`；phase-completed 分支：`bPhaseTransitionCompleted && TriggerReason==HealthDepleted && Archetype==Boss` → `ApplyBloodDepletedPhase2MaxHealth()`；新增 `ApplyBloodDepletedPhase2MaxHealth()`（找 BossPhases 里 PhaseIndex==2 且 RefillToMaximum && PhaseMaxHealth>0 → `Combatant->Stats.HpMax=PhaseMaxHealth; InitializeFromStats(_, true)` 回满）。header 加 `void ApplyBloodDepletedPhase2MaxHealth();`。 |
| `Source/ReEcho/Private/ReEchoGameMode.cpp` | 存档恢复 boss fallback id `M_TimeGuard` → `M_SHEEP`（行 ~1369）。 |
| 测试 | `ReEchoEnemyDefinitionCompilerTests.cpp`、`ReEchoCsvDataRegistryTests.cpp` 的 M_TimeGuard 断言重写为 M_SHEEP（HP=1300、5 技能、2 阶段、phase2 RefillToMaximum PhaseMaxHealth=650）。 |

### 二、数据改动（CSV 层已手工落位，但 xlsx 真源尚未同步 ← 待办核心）

- `Content/Data/enemies.csv`：删 M_TimeGuard，加 M_SHEEP（HP=1300, Boss, BehaviorProfileId=Boss.TimeGuard, PresentationId=Enemy.TimeGuard, Phase2Enabled=true, Phase2TransformSeconds=1, Phase2TriggerMode=HealthThreshold, Phase2HealthThresholdRatio=0）；header 加 Phase2TriggerMode/Phase2HealthThresholdRatio 两列；其余行补 `,,`。
- `Content/Data/enemy_abilities.csv`：删 TimeGuard 5 技能，加 SHEEP 5 技能（SH_01 MeleeSweep dmg10 cd3；SH_02A Projectile dmg5×4 cd3；SH_02B Projectile dmg5×3 cd2；SH_03 BlinkSlam dmg30 cd5；SH_04 PrayerBeam dmg20 cd13）。
- `Content/Data/boss_phases.csv`：删 TimeGuard，加 M_SHEEP_Phase1(PhaseIndex1,TriggerSeconds0,EchoPolicy None,不回血,PhaseMaxHealth0) + M_SHEEP_Phase2(PhaseIndex2,RefillToMaximum,PhaseMaxHealth650)；header 加 PhaseMaxHealth 列。
- `Content/Data/encounter_waves.csv` Encounter.8.Wave.1 BossEnemyId → M_SHEEP。
- `Content/Data/encounters.json` index:6 boss → M_SHEEP。
- `Content/Data/csv_schema.csv`：Enemies 加 Phase2TriggerMode/Phase2HealthThresholdRatio；BossPhases 加 PhaseMaxHealth。
- `scripts/validate_project.py`：Enemies schema 加两列(ratio required=False)；BossPhases schema 加 PhaseMaxHealth；required_ids M_TimeGuard→M_SHEEP；EchoPolicy 允许 None；RefillHealthPolicy 允许 RefillToMaximum(需 PhaseMaxHealth>0)；boss wave BossEnemyId→M_SHEEP。

### 三、当前卡点

`python scripts/validate_project.py` 的 **XLSX authoring sync check FAIL** —— CSV 已手工领先于 `Design/Data/ReEchoEnemyData.xlsx` 真源（字节不一致）。根因：`怪物.xlsx` 更新后，`ReEchoEnemyData.xlsx` 的三张表仍是 TimeGuard：
- `tblEnemies` header=28 列（到 Phase2TransformSeconds），仍含 M_TimeGuard(r7)，无 Phase2TriggerMode/Phase2HealthThresholdRatio 列；
- `tblEnemyAbilities` 全是 TimeGuard 5 技能；
- `tblBossPhases` 只有 TimeGuard，无 PhaseMaxHealth 列。

### 四、待办（方案甲：写回 xlsx 真源 → sync → validate）

**【核心】把 WS4 数据写回 `Design/Data/ReEchoEnemyData.xlsx` 真源：**
1. `tblEnemies`：删 M_TimeGuard 行，加 M_SHEEP 行（HP=1300, Boss, BehaviorProfileId=Boss.TimeGuard, PresentationId=Enemy.TimeGuard, Phase2Enabled=true, Phase2TransformSeconds=1, Phase2TriggerMode=HealthThreshold, Phase2HealthThresholdRatio=0）；其余现存行在这两新列补空值。
2. `tblEnemyAbilities`：删 TimeGuard 5 行，加 SHEEP 5 技能行（SH_01..SH_04，数值同上第二节）。
3. `tblBossPhases`：删 TimeGuard 行，加 M_SHEEP 两阶段行（phase1: PhaseIndex=1, TriggerSeconds=0, EchoPolicy=None, 不回血, PhaseMaxHealth=0；phase2: PhaseIndex=2, RefillToMaximum, PhaseMaxHealth=650）；header 加 PhaseMaxHealth 列。
4. 运行 `python scripts/data/sync_xlsx_to_csv.py` 重新生成 CSV（会覆盖手改 CSV，使其与真源一致）。
5. 运行 `python scripts/validate_project.py` 确认通过（含 XLSX authoring sync check）。

**发布门禁（推 origin/main 前）：**
- `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` 全量构建 + 刷新精选预构建二进制（`Binaries/Win64/UnrealEditor-*.dll` + `ReEchoEditor.prebuilt.json`）。
- 正式提交 Author 须含人类 GitHub 账号 + AI 身份，标题以一个标签 `[PROGRAMMER]` 开头。

**人工验收（PendingBeforeClose）：** PIE 验证 SHEEP 血清空 → 变黑二阶段（满血 650）→ 再打死 → Boss 击败结算。

### 五、接手模型需注意的风险点

- **xlsx 备份**：改 `ReEchoEnemyData.xlsx` 前先备份（openpyxl 写表可能丢公式/格式）；只改目标单元格，勿重建整表。
- **openpyxl 隐藏行陷阱**：遍历行时务必同时检查 `ws.row_dimensions[r].hidden`，不要误把隐藏行当可见数据（见项目记忆）。
- **数据链铁律**：运行时绝不直读 xlsx；禁止把手改 CSV 当真源长期保留——必须 sync 回一致。
- **BehaviorId 枚举约束**：enemy_abilities 的 BehaviorId 必须落在 C++ 硬编码的 5 个（`Boss.MeleeSweep/Projectile/BlinkSlam/PrayerBeam/ElementCleanse`），经 `ResolveBossAbilityKind` 映射；SH_04 用 `PrayerBeam`。
- **模块边界纪律**：EnemyLogic 不得 include GameMode/PlayerController/presentation；血量由 host(EnemyActor) 注入 `Sense.CurrentHealthRatio`，Logic 只读 Sense（已在本次改动修复，勿回退引入 Graybox include）。
- **致命伤拦截签名**：`FReEchoFatalDamageIntercept` 为 `bool(float&)`（非无参），改委托须同步所有绑定/调用点。
- **ort 合并缺陷**：本恢复分支涉及整段删除，rebase/merge 易静默选边；如需合入远端 main，优先手动逐文件落位 + grep 核验，勿直接 merge。

---

## γ 重构：以怪物.xlsx 为权威重定义敌人数据模型（2026-08-22，决策 γ-B 已确认）

> 接手模型经核对判定：现有 `Enemies.MoveSpeedCmPerSecond` 量级混乱（Grunt95/Bomber175/TimeGuard45），怪物.xlsx 用相对值（人物=1.0，SHEEP=1.2），无直接映射，导致 SHEEP 数值被 TimeGuard 遗留值限制。用户决策 γ：以怪物.xlsx 为干净权威重定义敌人数据模型。**已确认采用 γ-B（彻底相对化）**（2026-08-22 三决策答复：q-ms=B 彻底相对化 / q-base210=不惜代价实现策划表(人物=1.0) / q-keep=不惜代价实现策划表）。

### 决策（已确认）
- **移速方案 = γ-B 彻底相对化**：废弃绝对列 `MoveSpeedCmPerSecond`，所有敌人改填相对 `MoveSpeedMultiplier`（玩家=1.0 基准）。全部 7 敌重填相对值。
- **相对基准 = 策划表（人物=1.0）**：全局 `ReEchoBalanceSettings.BaseMoveSpeed = 210.f` 作为基准；编译期归一 `EffectiveMoveSpeed = BaseMoveSpeed * MoveSpeedMultiplier`，运行时零改动。
- **EnemyAbilities / BossPhases = 按怪物.xlsx 落地，不另重构 schema**：SHEEP 5 技能 + 双阶段(回满650) 按怪物.xlsx 数值精确填入（ws45 WIP 已实现并随本重构一并 sync）。

### 实现落位（C++ / schema / 数据层已改，待增量构建刷新预构建指纹）
- C++：`FReEchoCsvEnemyRow.MoveSpeedCmPerSecond` → `float MoveSpeedMultiplier = 0.0f`；`ReEchoEnemyCsvReader` `HasExactColumns` 列名 `MoveSpeedCmPerSecond`→`MoveSpeedMultiplier`，`RequireFloat("MoveSpeedMultiplier", 0.01f, 10.0f)`；`ReEchoEnemyDefinitionCompiler` 归一 `OutDefinition.MoveSpeedCmPerSecond = GetDefault<UReEchoBalanceSettings>()->BaseMoveSpeed * Row->MoveSpeedMultiplier`（`#include "Core/ReEchoBalanceSettings.h"`）。
- 运行时零改动：`ReEchoEnemyLogicComponent` L417/L1012/L1025 仍读 `FReEchoEnemyDefinition::MoveSpeedCmPerSecond`；`ReEchoBalanceSettings` 为 UDeveloperSettings（无 `Get()`，用 `GetDefault<>`）。
- schema：`csv_schema.csv` Enemies `MoveSpeedMultiplier,Float,true,0.01,10`；`validate_project.py` 列 spec 同改。
- 数据：`ReEchoEnemyData.xlsx` Enemies header 改 `MoveSpeedMultiplier` + 补 `Phase2TriggerMode/Phase2HealthThresholdRatio`；7 敌相对值（SLIME0.8/RABBIT0.7/FOX1.2/SHEEP1.2/Grunt≈0.452/Shield≈0.238/Bomber≈0.833）；非二阶段 `Phase2HealthThresholdRatio=0`；`M_TimeGuard`→`M_SHEEP`；EnemyAbilities 7 行(SH系列)、BossPhases 2 行(补PhaseMaxHealth) 重写；校验/表范围 openpyxl 重建。`ReEchoEncounterData.xlsx` `BossEnemyId` M_TimeGuard→M_SHEEP。

### 相对移速对照（基准 210）
| 敌 | Multiplier | 解析 cm/s | 来源 |
|---|---|---|---|
| M_SLIME | 0.8 | 168 | 怪物.xlsx |
| M_RABBIT | 0.7 | 147 | 怪物.xlsx |
| M_FOX | 1.2 | 252 | 怪物.xlsx |
| M_SHEEP | 1.2 | 252 | 怪物.xlsx |
| M_Grunt | ≈0.452 | 95 | legacy 95 ÷ 210 |
| M_Shield | ≈0.238 | 50 | legacy 50 ÷ 210 |
| M_Bomber | ≈0.833 | 175 | legacy 175 ÷ 210 |

### 验收
- `python scripts/data/sync_xlsx_to_csv.py` 通过（xlsx↔csv 一致）；`python scripts/validate_project.py` 通过（含 prebuilt 指纹）。
- 推 main 门禁：`-FullRebuild` + 刷精选预构建二进制。
- PIE：SHEEP≈252、SLIME/RABBIT/FOX=168/147/252、Grunt/Shield/Bomber 速度保持；血清空→黑二阶段650→击败；其他敌人不变。
- WS1-3 核查：EnemyCombatStats 按怪物.xlsx 补成长数据（现仅 SLIME_C1 一行）。

### 当前卡点（与 ws45 WIP 合并后）
- C++ 改动使 prebuilt bundle stale；需本地增量 `Build-Editor -Configuration Development`（编辑器须关闭）刷新指纹，sync 才能持久化 `enemies.csv`（曾因 stale 被 sync 回滚）。
- 分支 `plan/68-ws1-5` 与 origin/main 分叉（本地 1 WIP / 远端 2：Plan75 implement loadout + Plan76 Niagara）；合 main 前须 rebase 到 origin/main 并核对 weapon 相关无冲突（优先手动逐文件落位 + grep 核验，避 ort 静默选边）。

---

## 实现现状总览与交接给下一个 AI（2026-08-23，Secretary/Programmer 侧 AI 记录）

> 本段是**最新、最权威**的实现现状快照，供接手 AI 直接据此继续。与上文任何较旧提纲/数值冲突处，以本段为准。已把整个 γ-B 重构 + WS1..WS5 恢复线整合进单一分支 `plan/68-ws1-5`。

### A. 分支 / 工作树 / 提交拓扑（发布前实测）

- 实现工作树：`C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan68`；分支 `plan/68-ws1-5`；HEAD = `d382758`。
- 共同祖先（merge-base with origin/main）= `b7c4212` = `origin/plan/67-shop-drop` 的 tip。**注意：本分支实际基于 plan67（尚未进 main）而非直接基于 origin/main。**
- 领先该基线 **6 个提交**，落后 `origin/main` **27 个提交**（origin/main 已推进到 Plan84；期间合入 Plan76/77/78/79/80/81/82/83/84 等大量 weapon/UI/敌群/关卡工作）。
- 远端另有一条 WS4/WS5 独立实现线 `origin/plan/68-ws4-sheep-boss`（`771db62` "SHEEP two-phase boss and idle-wander aggro state machine"）——与本分支是**并行的两种实现**，接手前须先决定以哪条为准（本分支 `plan/68-ws1-5` 是更完整的整合线）。

本分支领先基线的 6 个提交（新→旧）：

| 提交 | 语义 | 层 |
|---|---|---|
| `d382758` | 投射物齐射数据驱动：`ProjectileCount`/`SpreadAngleDegrees`/`bMovementDuringCast` 三列映射 + N 发扇形散射行为；修 RABBIT 测试调参（速度 500→432 → 玩家参照点改 475/560/645cm）+ 修 `ResolveVolleyDirection` 误用全局 BallCount 的潜伏 bug（改 InBallCount 参数）；清理 stale boss-phase validator | 数据+行为+测试 |
| `8f7e4ef` | 仇恨/感知范围 `HateRangeCm` 数据驱动（来自策划「变身范围」列），退役硬编码 resolver | WS5 |
| `d5a6dbc` | `EnemyCombatStats` 按场次成长接入 spawn（按 EncounterIndex 覆盖 MaxHealth/ContactDamage/AttackInterval） | WS3 |
| `36516de` | 非 boss 对齐 `怪物.xlsx`（HP/contact/Phase2）+ RABBIT RB_01B；发布 csv + 刷新 prebuilt | WS1/WS2 |
| `890de67` | γ-B 相对移速（player=1.0，`BaseMoveSpeed=210`，编译期归一）+ SHEEP align | γ-B |
| `e45242d` | **WIP WS4/WS5 恢复快照，提交说明标注「local temp, not for remote」** —— SHEEP 二阶段 csv/schema + 致命伤拦截委托 + Phase2 血量触发 C++ + 测试 + plan 文档 70 行 | WS4/WS5 |

### B. 未提交工作（本会话新增，尚未 commit，7 个文件）

GM 调试可视化工具（Plan68 验证用，非原 Plan Writes 清单）：
- `Source/ReEcho/Public/ReEchoGameMode.h`（+16）：`UFUNCTION(Exec) GMShowEnemyHealth` / `GMShowEnemyRange` 声明 + `IsEnemyHealthDebugEnabled()`/`IsEnemyRangeDebugEnabled()` 访问器 + `bShowEnemyHealthDebug`/`bShowEnemyRangeDebug` 标志。
- `Source/ReEcho/Private/ReEchoGameMode.cpp`（+53）：两个 Exec 命令实现（On/Off/Toggle + `EnsureGMCommandAvailable` + `PrintGMResult`），GMHelp 追加两行。
- `Source/ReEcho/Private/Graybox/ReEchoEnemyActor.cpp`（+57）：`Tick` 内 `#if !UE_BUILD_SHIPPING` 段——头顶 `DrawDebugString` 血量（HP/百分比）；`DrawDebugCircle` 红圈=近战 `ContactRangeCm`、橙圈=远程最大 `MaxRangeCm`（`Damage>0` 技能取最大），圈旁 `DrawDebugString` 标「接触 Xcm」/「远程 Xcm」。
- `docs/GM_COMMANDS.md`（+2）：登记两条命令。
- `Binaries/Win64/UnrealEditor-ReEcho.dll` / `UnrealEditor-ReEchoEnemies.dll` / `ReEchoEditor.prebuilt.json`：本会话增量构建刷新（非 FullRebuild）。

### C. 现状 vs 原提纲的确定性差异（以此为准）

- **Boss = 羊 `M_SHEEP`**（原 `M_TimeGuard` 废除）；SHEEP 复用 `Boss.TimeGuard` 的 BehaviorProfile/Presentation（仅换 enemy id），`MaxHealth=1300`。
- **二阶段触发 = 血条清空（HP→0）变身**，二阶段回满血到 **650**；SH_04 冷却默认 **13s**。已放弃 `TriggerHealthFraction=百分比` 与 `MaxHealth=2600` 旧提纲。
- **移速 = γ-B 彻底相对化**：废弃绝对 `MoveSpeedCmPerSecond`，改 `MoveSpeedMultiplier`（player=1.0，`BaseMoveSpeed=210` 编译期归一）；7 敌相对值见上文 γ 章节对照表。
- SHEEP 5 技能（SH_01 MeleeSweep / SH_02A、SH_02B Projectile / SH_03 BlinkSlam / SH_04 PrayerBeam）与双阶段已按 `怪物.xlsx` 落地到 csv + xlsx 真源。

### D. 当前卡点与待办（接手 AI 的关键路径）

1. **`e45242d` WIP 去留**：该提交标「not for remote」。发布分支给协作前应 `reword`/`squash` 成正式 `[PROGRAMMER] Plan68 ...` 提交（或用户明确确认可原样带上远端 feature 分支）。
2. **未提交 GM 工作**：决定是否随分支一起提交（对接手 AI 用 GM 命令 PIE 调试 SHEEP/怪物有用）。
3. **落后 origin/main 27 提交**：接手若要合 main，须 rebase/merge 到最新 origin/main，重点核对与 Plan76/77/78/79/80/81/82/83/84 的 weapon/敌群/关卡改动无冲突（优先手动逐文件落位 + grep 核验，避 ort 静默选边）。
4. **发布 origin/main 门禁**（仅当最终合 main 时）：`scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` + `python scripts/validate_project.py` 绿灯 + 刷新精选预构建二进制 + 用户 PIE 实测 + 用户明确「可以推 main」。
5. **人工验收（PendingBeforeClose）**：PIE 验证 SHEEP 血清空→变黑二阶段（满血 650）→再打死→击败结算；四怪双形态表现；按场次成长；未战斗游走。

### E. 接手须知（风险点，详见上文「WS4/WS5 恢复交接记录」第五节）

- xlsx 改前先备份，只改目标单元格；遍历行必查 `row_dimensions[r].hidden`（隐藏行=废除数据）。
- 运行时绝不直读 xlsx；禁止把手改 CSV 当真源——必须 `sync_xlsx_to_csv.py` 回一致。
- `enemy_abilities.BehaviorId` 必须落在 C++ 硬编码 5 个（`Boss.MeleeSweep/Projectile/BlinkSlam/PrayerBeam/ElementCleanse`）；SH_04 用 `PrayerBeam`。
- 模块边界：EnemyLogic 不得 include GameMode/PlayerController/presentation；血量由 host 注入 `Sense.CurrentHealthRatio`。
- 致命伤拦截委托签名 `FReEchoFatalDamageIntercept = bool(float&)`，改动须同步所有绑定/调用点。
