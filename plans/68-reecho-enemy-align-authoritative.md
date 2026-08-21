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
