# 三档难度配表使用说明（策划）

本说明面向负责怪物、Boss、关卡波次和刷怪节奏的策划。三档难度不是统一倍率，而是三套彼此独立的完整配置；策划可以逐怪、逐场、逐技能分别调整。

## 1. 应该编辑哪一份

| 游戏显示 | 稳定难度 ID | 策划工作簿 | 运行时目录 |
|---|---|---|---|
| 派对（简单） | `Party` | `Design/Data/ReEchoDifficultyParty.xlsx` | `Content/Data/Difficulty/Party/` |
| 常规（正常） | `Standard` | `Design/Data/ReEchoDifficultyStandard.xlsx` | `Content/Data/Difficulty/Standard/` |
| 噩梦（困难） | `Nightmare` | `Design/Data/ReEchoDifficultyNightmare.xlsx` | `Content/Data/Difficulty/Nightmare/` |

- 只想调整某一档时，只改对应工作簿。
- 想让三档共同变化时，必须把同一修改明确填进三份工作簿；不要假定程序会自动同步另一档。
- 当前三份工作簿结构和初始内容相同，后续允许数值不同，但 Sheet、Table、列名和列顺序必须继续一致。
- 不要直接编辑 `Content/Data/Difficulty` 下的 CSV。CSV 是生成物，不是策划源。
- 旧 `ReEchoEnemyData.xlsx`、`ReEchoEncounterData.xlsx` 和 `ReEchoData.xlsx` 中同领域内容只保留启动兼容/公共数据；新局的怪物和 Encounter 以所选 Difficulty 工作簿为准，不要两边重复调同一项。

## 2. 修改前后的最短流程

在仓库根目录执行：

```powershell
python scripts\data\sync_difficulty_xlsx_to_csv.py --check
```

确认通过后再打开工作簿。修改完成后：

1. 保存并关闭 Excel/WPS。
2. 生成三档 CSV：

```powershell
python scripts\data\sync_difficulty_xlsx_to_csv.py
```

3. 再检查生成结果：

```powershell
python scripts\data\sync_difficulty_xlsx_to_csv.py --check
python scripts\validate_project.py
```

4. 关闭并重新打开 Unreal Editor，选择对应难度，使用“新游戏”验证。不要用已经开始的存档验证新数值。

成功日志会包含：

```text
[Difficulty] Run locked to Party
[Difficulty] Run locked to Standard
[Difficulty] Run locked to Nightmare
```

三档当前数值相同时，游戏表现看不出差别，应先用这条日志确认实际加载的难度。

## 3. 通用填写规则

1. 只编辑每个 Sheet 中 Excel Table 的数据行；不要修改 Sheet 名、Table 名、列名、列顺序或 `_ExportMap`。
2. 生产 Table 内不要合并单元格、插入说明行或使用 Excel 公式。说明写入 `Notes`。
3. `Id`、`EnemyId`、`StageId`、`EncounterId`、`BehaviorId`、`PresentationId` 等是程序契约，不是显示文案。已经发布的 ID 不要改名、复用或翻译。
4. 新增行要使用“插入表格行”，确保新行仍在 Table 范围内。删除配置时删除整行，不要只清空必填字段。
5. 布尔值使用 `true` / `false`；秒填入 `*Seconds`；Unreal 厘米填入 `*Cm`；比例使用 `0～1` 小数。
6. `Enabled=false` 表示该行不参与运行时；不要通过删除父表行但保留子表引用的方式禁用内容。
7. 有下拉的字段优先使用下拉。Excel/WPS 允许输入并不代表程序支持任意新值；新增 Behavior、Policy 或表现 ID 前先让程序注册。
8. `SourceSheet`、`SourceRow` 和 `Notes` 用于追溯，不决定玩法；不要把玩法逻辑写在备注里。
9. 每次只让一名策划编辑同一个 XLSX。Git 无法可靠自动合并两个同时修改的二进制工作簿。

## 4. 各 Sheet 填什么

### 4.1 Enemies：怪物基础定义

用于定义怪物稳定身份、基础状态、碰撞、移动和接触伤害。

| 字段 | 作用 |
|---|---|
| `Id` | 稳定怪物 ID；其他表通过它引用，不随难度改名 |
| `Enabled` | 是否允许该怪物进入运行时 |
| `MaxHealth` | 基础生命；存在对应 `EnemyCombatStats` 行时会被逐场生命覆盖 |
| `MoveSpeedMultiplier` | 相对项目基础移速的倍率，不是 cm/s |
| `ContactDamage` | 基础接触伤害；存在逐场行时会被覆盖 |
| `AttackIntervalSeconds` | 基础攻击/接触结算间隔；存在逐场行时会被覆盖 |
| `CollisionRadiusCm` / `CollisionHalfHeightCm` | 玩法碰撞尺寸，不是图片大小 |
| `ContactRangeCm` / `MovementStopDistanceCm` | 接触判定距离和靠近目标后的停步距离 |
| `HateRangeCm` | 感知/仇恨范围 |
| `HitReactionDurationSeconds` | 受击硬直时间 |
| `KnockbackSpeedCmPerSecond` / `KnockbackDrag` | 击退初速度与阻力 |
| `TriggerRadiusCm` / `DamageRadiusCm` / `FuseSeconds` | Bomber 类触发、伤害半径和引爆时间；非对应类型保持现有显式零值 |
| `Phase2*` | 双形态怪物/首领的二阶段触发配置；不要用于普通怪 |

不要把逐关成长全部写进 `Enemies.MaxHealth`。需要第 1～8 场分别调整时，应填写 `EnemyCombatStats`。

### 4.2 EnemyAbilities：怪物和 Boss 技能

一行表示一个已注册技能。`OwnerEnemyId` 关联 `Enemies.Id`，同一个怪物可以有多行技能。

| 字段组 | 作用 |
|---|---|
| `Damage` | 技能单次基础伤害；多发技能通常表示单发伤害，结合 `ProjectileCount` 计算总压迫量 |
| `WindupSeconds` / `ActiveSeconds` / `RecoverySeconds` / `CooldownSeconds` | 前摇、有效窗口、后摇和再次可用间隔 |
| `MinRangeCm` / `MaxRangeCm` | 技能允许选择的距离区间 |
| `RadiusCm` / `WidthCm` / `LengthCm` | 圆形、矩形或路径类技能的空间尺寸；不适用字段保持 `0` |
| `ProjectileSpeedCmPerSecond` | 投射物速度；是否允许 `0` 走兼容推导取决于具体 Behavior，不确定时不要改为 `0` |
| `ProjectileCount` / `SpreadAngleDegrees` | 投射物数量和散射总角度 |
| `TargetingMode` / `LockTiming` | 索敌方式与锁定时点，只能选择已有下拉值 |
| `SequenceOrder` | 同一 Owner 内的技能轮转顺序；主动技能使用唯一的正整数 |
| `bMovementDuringCast` | 施法期间是否允许继续移动 |

兔子站定连发的空间弹距由 `ActiveSeconds / (ProjectileCount - 1)` 形成发射间隔；`ActiveSeconds` 是首发到末发的总窗口，不是相邻两发间隔。

### 4.3 BossPhases：Boss 阶段数值

`BossEnemyId + PhaseIndex` 决定 Boss 的阶段定义。

| 字段 | 作用 |
|---|---|
| `PhaseIndex` | 阶段顺序，从 1 开始且同 Boss 内唯一 |
| `PhysicalAttackMultiplier` / `ElementalAttackMultiplier` | 当前阶段物理/元素伤害倍率 |
| `AttackSpeedMultiplier` / `MovementSpeedMultiplier` | 当前阶段攻击节奏/移动倍率 |
| `RefillHealthPolicy` | 进入阶段时的回血策略，只使用现有下拉值 |
| `PhaseMaxHealth` | 该阶段独立最大生命；与回血策略共同决定阶段切换后的血量 |
| `EchoPolicy` | 阶段对回响的策略，只使用已实现选项 |

不要仅凭字段名创造新的阶段触发逻辑。当前 Boss 变身时机与技能阶段切换仍受已注册运行时行为约束；需要新的触发方式先交程序实现。

### 4.4 EnemyCombatStats：逐小关怪物数值

这是策划做三档精细难度差异时最常用的表。

- `EnemyId + CombatIndex` 是唯一组合。
- `CombatIndex` 对应第几场战斗/小关，目前为 1～8。
- `MaxHealth`、`ContactDamage`、`AttackIntervalSeconds` 会覆盖 `Enemies` 中同名基础值。
- 没有对应逐场行的敌人会回退到 `Enemies` 基础值；Boss 等特殊敌人可继续使用基础/阶段配置。
- `AttackIntervalSeconds` 越小，攻击越频繁；不要误当成攻速倍率。

例如，只想提高噩梦第 5 场兔子的生命和接触伤害，应只修改噩梦工作簿中 `EnemyId=M_RABBIT, CombatIndex=5` 的对应行。

### 4.5 EnemyShardDrops：每只怪物掉落时间碎片

每行对应一个 `EncounterIndex`：

- `MeleeMin/MeleeMax`：近战及默认类别的闭区间。
- `RangedMin/RangedMax`：远程怪闭区间。
- `EliteMin/EliteMax`：精英怪闭区间。
- Boss 不使用这张表掉落基础时间碎片。
- 最小值不得大于最大值；随机结果包含区间两端。
- 表中数值是单只敌人死亡后生成的拾取物携带量，玩家拾取后才进入余额。

### 4.6 Stages：大关与小关范围

| 字段 | 作用 |
|---|---|
| `StageIndex` | 大关顺序 |
| `FirstEncounterIndex` / `LastEncounterIndex` | 该 Stage 包含的小关范围，必须连续且不重叠 |
| `PreserveEnemiesBetweenEncounters` | 同 Stage 的小关切换是否保留存活怪物 |
| `ClearEnemiesOnEnter` | 进入该 Stage 时是否清理上一 Stage 敌人 |
| `SceneId` | 场景稳定 ID；新增场景前需确认程序/资源已接入 |

### 4.7 Encounters：每个小关的总体规则

| 字段 | 作用 |
|---|---|
| `EncounterIndex` / `StageId` | 小关顺序及所属大关 |
| `DurationSeconds` / `EndCondition` | 小关时长和结束条件 |
| `EchoAnchorRatio` / `PlayerAnchorRatio` | 出生锚点比例，两者之和必须为 1 |
| `RangedBurstLimit` / `RangedBurstWindowSeconds` | 指定时间窗内允许的远程爆发数量 |
| `EliteSkillConcurrency` | 同时执行的精英技能上限 |
| `ActiveUnitLimit` | 场上活动单位上限；波次请求超过上限时会受容量门禁约束 |
| `BossCountsTowardUnitLimit` | Boss 是否占用活动单位上限 |

当前 `MeleeTargetingPolicy` 和 `ReplayPolicy` 是稳定契约预留列，首版运行时仍走固定逻辑。不要依赖修改它们产生玩法变化，也不要删除列。

### 4.8 EncounterWaves：小关内的分波数量

- `EncounterId` 必须引用 `Encounters.Id`。
- 同一 Encounter 内 `WaveIndex` 唯一且递增。
- `TriggerSeconds` 是从本小关开始算的触发时刻，不是与上一波的间隔。
- `MeleeCount/RangedCount/EliteCount` 是该波请求的各类数量。
- Boss 波通过 `BossEnemyId` 引用稳定 Boss ID；普通波该字段保持空值。
- 目前普通战的既有验证仍要求 0/10/20 秒波次结构；若要改变波次时刻契约，先与程序确认。

只提高 `MeleeCount` 不一定等于屏幕同时出现更多怪：还要同时检查 `Encounters.ActiveUnitLimit`、前一波存活量和 SpawnPolicy 是否能找到合法出生位置。

### 4.9 SpawnProfiles：各类怪的出生距离

一行对应一种 `EnemyRole`，并通过 `EnemyId` 指定实际出生怪物。

- `MinAnchorDistanceCm/MaxAnchorDistanceCm`：离玩家或回响锚点的出生距离区间。
- `MinSpacingCm`：同批出生点最小间距。
- `WarningLeadSeconds`：出生预警到实体进入玩法的时间。
- `DistributionPolicy`、`SpacingPolicy` 当前是预留契约，运行时仍使用固定环形取样和最小间距处理；不要依赖改这些列生效。

### 4.10 SpawnPolicy：全局出生约束

生产配置使用 `Id=SpawnPolicy.Default`：

- `AnchorLeadSeconds`：预测锚点的提前量。
- `MinPlayerDistanceCm/MinEchoDistanceCm`：出生点离玩家/回响的安全距离。
- `MaxCandidateAttempts`：寻找合法出生点的最大尝试次数；过低可能找不到点，过高会增加计算量。
- `BoundaryPolicy`、`CandidatePolicy`、`PlayerPredictionPolicy`、`MultiEchoPolicy` 当前是预留契约，首版运行时使用固定行为；不要删除，也不要把平衡效果建立在这些列上。

## 5. 常见调参任务

### 让某档整体更简单或更难

不要只改一个总倍率。按目标逐项检查：

1. `EnemyCombatStats`：逐场生命、接触伤害和攻击间隔。
2. `EnemyAbilities`：技能伤害、前后摇、冷却、弹速和弹数。
3. `EncounterWaves`：各波怪物数量。
4. `Encounters`：活动单位、远程爆发和精英技能并发上限。
5. `SpawnProfiles/SpawnPolicy`：出生距离、预警时间和安全距离。
6. `EnemyShardDrops`：该难度的经济补偿。
7. `BossPhases`：Boss 各阶段血量与倍率。

### 修改某场普通怪强度

优先改 `EnemyCombatStats`。只有希望所有未覆盖场次共同变化时，才改 `Enemies` 基础值。

### 修改某个技能压迫感

同时检查 `Damage`、前摇、后摇、冷却、射程、弹速和弹数；不要只看单发伤害。视觉特效大小不由这些数值列控制。

### 修改某关怪物数量

先改 `EncounterWaves`，再核对 `Encounters.ActiveUnitLimit`。如果怪物仍未全部出现，再检查跨小关保留规则和出生空间约束。

## 6. 游戏何时读取

- 游戏不读取 XLSX；保存表后必须生成 CSV。
- CSV 在新游戏/Continue 时按选择的难度装载并缓存，不是每关实时读盘。
- 玩家只能在开始菜单设置难度；开局后整局锁定，暂停设置不能更换。
- Continue 使用 Run 存档里的难度，不使用后来修改的“下一局偏好”。
- 同步 CSV 时如果 Unreal Editor 已打开，必须重启 Editor；只停止 PIE 再运行通常不会重载数据。
- 修改 XLSX/CSV 不需要重新编译 C++，但新增字段、Behavior、策略或资产 ID 需要程序支持。

正确验证顺序：

```text
同步最新分支
→ --check 确认基线干净
→ 编辑对应难度 XLSX
→ 保存并关闭 Excel/WPS
→ 生成 CSV
→ --check + validate_project.py
→ 重启 Unreal Editor
→ 设置对应难度
→ 新游戏验证
```

## 7. 常见报错

| 报错关键词 | 常见原因 | 处理方式 |
|---|---|---|
| `difficulty table set mismatch` | Sheet/Table 或 `_ExportMap` 被修改 | 恢复原结构，不要自行增删生产表 |
| `Columns do not match schema order` | 列名、列数或顺序变化 | 从 Git 恢复表头，只改数据行 |
| `Duplicate` | ID 或复合主键重复 | 根据报错 Sheet/Table/行修正唯一值 |
| `Unknown ... reference` | 引用了不存在或禁用的 Enemy、Stage、Encounter | 先补父表或修正引用 ID |
| `Cannot exceed maximum` | Min 大于 Max | 修正区间顺序 |
| `Anchor ratios must sum to one` | 两个锚点比例之和不是 1 | 重新填写两个比例 |
| `Generated CSV drift` / `difficulty CSV drift` | XLSX 和生产 CSV 不一致 | 若是刚改表，运行正式同步；否则先查是否有人手改 CSV |
| 游戏拒绝开局/Continue | 所选难度包缺失或校验失败 | 查看日志中的 Difficulty 错误，不要改成另一档绕过 |

生成器会尽量给出 Sheet、Table、行和列。先修第一条明确数据错误，再重新运行同步；不要阅读堆栈猜测，也不要直接手改 CSV。

## 8. 提交要求

- 同一次提交必须包含修改的 Difficulty XLSX 和对应生成的 `Content/Data/Difficulty/<Difficulty>/*.csv`。
- 不要只提交 XLSX，也不要只提交 CSV。
- 提交前必须通过 `sync_difficulty_xlsx_to_csv.py --check` 和 `validate_project.py`。
- 提交说明写明：修改了哪个难度、哪些 Sheet、哪些 Enemy/Encounter 稳定 ID。
- 推送前按项目规则检查远端是否有人修改同一工作簿；XLSX 二进制冲突不能直接选择 ours/theirs，应由人确认后重放数据修改。
