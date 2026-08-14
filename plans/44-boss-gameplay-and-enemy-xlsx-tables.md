# Plan 44 - 程序 - Boss 可玩闭环与怪物 XLSX 配表

## 协调

- Planner 负责人：Gavyn-side Planner。
- Executor 负责人：Gavyn-side AI。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。Boss 招式可读性、躲避手感、30 秒强化后的战斗节奏与最终胜负必须由用户 PIE 验收。
- 本地规划 / 实现基线：`origin/main@348d2a84232676164a64f8cfb5a49f836b65ba91`；执行授权前已 fetch，本地与远端一致。
- 本地实现方式：独立工作树 `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan44`，本地分支 `plan/44-boss-gameplay-and-enemy-tables`；不设远端任务分支。
- 依赖 / 阻挡：
  - Plan43 已关闭并提供 `MOD-ReEchoEnemies`、EnemyHost、Roster、Combat 单一结算路径和只读 Enemy Presentation。
  - 策划案《时间回响_Demo关卡与怪物设计说明书_v1.0》是本 Plan 的产品来源；其中 Boss 四个主动招式缺少完整数值，30 秒玩家强化也缺少精确定义，必须先完成本 Plan“审核决策”后才能设为 `Ready`。
  - `怪物体系M` 当前是 `ReferenceOnly`，没有 Excel Table；现有生产运行时仍使用 `Content/Data/enemies.json` 和 C++ `MakeLegacyEquivalent` 硬编码。
  - 用户已确认怪物体系由另一位策划负责配置，必须使用独立工作簿，不能继续写入主 `ReEchoData.xlsx`；独立二进制文件用于隔离策划所有权和 Git 冲突。
- Writes：
  - Plan：`plans/44-boss-gameplay-and-enemy-xlsx-tables.md`。
  - 怪物权威工作簿与生成单元：新增独立 `Design/Data/ReEchoEnemyData.xlsx`，以及 `Content/Data/reecho_data_manifest.csv`、`Content/Data/csv_schema.csv`、新增 `Content/Data/enemies.csv`、`Content/Data/enemy_abilities.csv`、`Content/Data/boss_phases.csv`，并删除迁移完成后的 `Content/Data/enemies.json`。主 `Design/Data/ReEchoData.xlsx` 不承载怪物生产表。
  - 表格工具与说明：`scripts/data/sync_xlsx_to_csv.py`、`scripts/data/test_sync_xlsx_to_csv.py`、新增 `Design/Data/ReEchoEnemyData使用说明.md`、`Design/Data/ReEchoEnemyData策划验收清单.md`；同步入口需显式读取两个权威工作簿，但怪物策划日常只编辑 `ReEchoEnemyData.xlsx`。
  - 数据适配：`Source/ReEcho/{Public,Private}/Data/**`、`Source/ReEcho/ReEcho.Build.cs`、数据 Registry/Schema/运行时快照测试。
  - Boss 逻辑：`Source/ReEchoEnemies/{Public,Private}/Enemies/**`、`Source/ReEchoEnemies/Private/Tests/**`。
  - Combat 窄契约：`Source/ReEchoCombat/{Public,Private}/Combat/**`、必要的聚焦测试；只新增“清除元素附着/灼烧并授予指定时长免疫”的正式命令，不把 Boss 行为下沉到 Combat。
  - 世界宿主与流程：`Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*`、新增 Boss 敌方投射物 Host/逻辑接线、`Source/ReEcho/{Public,Private}/Encounter/ReEchoEncounterDirector.*`、`Source/ReEcho/{Public,Private}/ReEchoGameMode.*`、Boss/Encounter/Save 测试。
  - 保存：`Source/ReEcho/Public/Run/ReEchoRunSaveGame.h`、`Source/ReEcho/Public/Core/ReEchoTypes.h` 及对应恢复测试（仅在运行时状态确需新增字段时修改）。
  - 最小表现接线：`Source/ReEcho/{Public,Private}/Presentation/Enemy/**`，只增加资源中立的预警/事件消费与当前资产 fallback；不制作正式动画或美术资产。
  - 架构与门禁：`shared/CODEBASE_MAP/ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`、`MOD-ReEcho.md`、`MOD-ReEchoCombat.md`、`scripts/validate_project.py`、最终精选 Win64 Editor 预构建包。
- Stable Reads：`Source/ReEchoWeapons/**`、`Source/ReEchoAudio/**`、现有玩家/回响武器路径、Plan29-31 回响存储/回放、现有动画与 Boss 资产。
- 影响模式：`SharedContract`。Boss 行为仍在 `ReEchoEnemies`，伤害/元素仍在 `ReEchoCombat`，XLSX/CSV 由主模块数据适配编译后注入，主流程只编排 Boss 房间和 30 秒阶段事件。
- 兼容承诺 / 下游操作：保留 `M_Grunt | M_Shield | M_Bomber | M_TimeGuard` 稳定 ID、现有非 Boss 行为、Combat HitIntent 单一结算、Plan31 默认/指定回响选择、保存继续与表现模块只读边界。策划只编辑 XLSX；运行同步脚本后生成 CSV，不手改 CSV。
- 明确排除：
  - 不在本 Plan 中把总流程从当前 6 场改成策划案的 8 房间。
  - 不实现 0/10/20 秒三波投放、回响/本体双锚比例、出生镜像、攻击令牌或全关怪物数量/1.3 数值成长；这些属于后续 Encounter/Spawn Plan。
  - 不把史莱姆、兔子、狐狸外观直接重定义为新的玩法 Archetype；普通怪行为改版另立 Plan。
  - 不制作正式 Boss 动画、VFX、音频资产、UI 排版或关卡美术；只提供稳定语义事件、预警数据和 fallback 可玩反馈。
  - 不把 Boss 行为塞进 `ReEchoGameMode`、Presentation、WBP 或音频回调。

## 锁定目标

1. 把当前只会追踪/接触攻击的 Boss 完善为确定性四招 Boss：近战挥击、远程投射、闪身打击、祷告光束；所有招式具有明确前摇、锁定点、命中窗口、后摇和冷却。
2. Boss 每 9 秒清除自身元素附着和灼烧，并获得 1 秒元素免疫；该变化通过 Combat 正式命令完成并发布元素状态事件。
3. Boss 战不再在 30 秒时自动判负。玩家死亡则失败，Boss 死亡则胜利；30 秒只触发一次“回响退场 + 玩家强化”阶段，随后战斗持续到一方死亡。
4. Boss 的攻击选择、锁定目标/位置、招式阶段、计时器、清洗计时和 30 秒阶段必须确定性、可保存恢复，不依赖动画完成、资源加载或帧率。
5. 将现有怪物基础数值和 Boss 行为参数迁入独立权威工作簿 `ReEchoEnemyData.xlsx`，同步为 CSV；生产运行时移除 `enemies.json` 与 `MakeLegacyEquivalent` 的隐式 fallback。启用但缺失/非法的敌人或招式配置必须使加载失败并给出工作簿/表/行/列错误，不得静默使用默认值。
6. 保持模块边界：`ReEchoEnemies` 产生行为与空间攻击意图，EnemyHost 应用世界移动/投射物/范围查询，`ReEchoCombat` 独占最终伤害与元素状态，Presentation/Audio/UI 只读消费事件。
7. 给怪物策划提供独立工作簿、可编辑下拉、字段说明、单位和校验；策划只改 `ReEchoEnemyData.xlsx`，运行统一同步脚本即可更新三个 CSV 并进入游戏验证。

## 策划案与当前实现差异

| 项目 | 策划案 | 当前实现 | 本 Plan 处理 |
|---|---|---|---|
| Boss 基础值 | 650 HP、0.90m/s、半径1.2m、奖励30 | 650 HP、45uu/s、接触18、2秒间隔；半径未由数据驱动 | 以策划案为权威迁表；速度按项目现有 `50uu=1m` 换算为45uu/s |
| Boss 招式 | 挥击、远程、闪身打击、祷告光束 | 只有普通接触攻击 | 实现四招确定性状态机 |
| 元素清洗 | 每9秒；免疫1秒 | 无 | 增加 Combat 正式命令和 Boss 调度 |
| 30秒后 | 回响消失；玩家战斗属性大幅提高；继续战斗 | EncounterDirector 30秒结束，Boss未死则整局失败 | 改为 Boss 死亡条件结束；30秒阶段只触发一次 |
| 数据来源 | 怪物体系由另一位策划独立配置 | `enemies.json` + C++ 硬编码 | 新建独立 `ReEchoEnemyData.xlsx`，内部多 Table 同步 CSV；不写主 `ReEchoData.xlsx` |
| 总房间 | 8个房间，Boss为第8场 | 当前6场，Boss为第6场 | 本 Plan 保持6场；8房间另立 Encounter Plan |

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoEnemies`、`MOD-ReEcho`、`MOD-ReEchoCombat`；关联 `AREA-Enemies`、`AREA-Data`、`AREA-Encounter`、`AREA-Run`、`AREA-Presentation`、`AREA-Tests`。
- 对应模块文档：维护 `MOD-ReEchoEnemies.md`、`MOD-ReEcho.md`、`MOD-ReEchoCombat.md`；审阅 `MOD-ReEchoWeapons.md`、`MOD-ReEchoAudio.md`，若契约/代码位置不变则在关闭记录中说明无需修改。
- 设计意图：Boss 是 Enemy 逻辑的一种策略，不是 GameMode 特判集合。表格只提供数值和允许的 `BehaviorId`；不同技能语义由注册行为实现，不在单元格中写脚本、case 名或自由文本表达式。
- 数据依赖方向：`ReEcho` 数据适配读取独立 `ReEchoEnemyData.xlsx` 生成的 CSV，编译为资源无关 `FReEchoEnemyDefinition / FReEchoEnemyAbilityDefinition / FReEchoBossPhaseDefinition`，由 Host 按值注入 `ReEchoEnemies`。`ReEchoEnemies` 不反向依赖主模块、工作簿或 CSV Reader。主工作簿与怪物工作簿可由同一同步入口统一校验，但二者保持独立二进制文件和策划所有权。
- 逻辑/表现分离：Boss Logic 发布 `TelegraphStarted / AbilityCommitted / AbilityEnded / Cleanse / PhaseChanged` 等语义事件；Presentation、Audio 与未来动画各自订阅，不能回写技能进度。
- 相关文档同步范围：`ARCHITECTURE.md`、CODEBASE_MAP 索引、上述三份模块文档、XLSX 使用说明与策划验收清单。

### 目标运行时拓扑

```text
ReEchoEnemyData.xlsx（怪物策划独立权威工作簿）
  → enemies.csv / enemy_abilities.csv / boss_phases.csv
  → ReEchoCsvDataRegistry（跨工作簿统一校验 + Run 快照）
  → 主模块 EnemyDefinitionCompiler
  → AReEchoEnemyActor（世界 Host）
      → UReEchoEnemyLogicComponent / BossBehaviorPolicy（ReEchoEnemies）
          → Move / Telegraph / Attack / Cleanse / Phase Intent
      → Host 应用 Transform、投射物和空间查询
      → FReEchoHitIntent → ReEchoCombat（唯一伤害/元素裁决）
      → EnemyEvents / CombatEvents
          → EnemyPresentation、Audio Adapter、UI（只读）
```

### Boss 行为策略

- `UReEchoEnemyLogicComponent` 保留共同生命周期、受击、击退、死亡和快照权威；Boss 专属选择与阶段状态放在 `ReEchoEnemies` 内的资源无关 Boss policy/runtime，不放进 Host。
- Boss policy 使用配置的稳定 `BehaviorId` 解析为注册行为：`Boss.MeleeSweep`、`Boss.Projectile`、`Boss.BlinkSlam`、`Boss.PrayerBeam`、`Boss.ElementCleanse`。
- 主动招式按 `SequenceOrder` 确定性轮转；若当前距离/空间条件不满足，从后续技能中选择第一个合法项，不使用不可复现的全局随机。连续跳过一轮时回到可执行的远程招式，不能原地卡死。
- 前摇开始时锁定目标或落点；后续移动不改变已锁定空间，确保预警可躲。技能提交后才消耗 cooldown；受击不会由动画取消已经锁定的逻辑动作，除非后续策划另加明确打断规则。
- 每个空间命中都带唯一 AttackIdentity；一个招式的同一命中窗口对同一目标最多结算一次。
- 投射物使用 `ReEchoEnemies` 的资源无关 projectile logic + 主模块轻量 projectile Host，沿用 Weapons 的“逻辑/表现分离”原则，但不引入 `ReEchoEnemies -> ReEchoWeapons` 依赖。

### 独立怪物工作簿中的三张生产表

`Design/Data/ReEchoEnemyData.xlsx` 是怪物体系唯一策划真源，由怪物策划独立维护；主 `ReEchoData.xlsx` 不镜像这些行。工作簿内含以下三个 Excel Table，并通过统一同步入口生成生产 CSV。因为 XLSX 是二进制文件，同一时刻只允许怪物策划所有者编辑该工作簿，程序侧结构迁移需先协调交接。

#### `Enemies` / `tblEnemies` / `enemies.csv`

每个玩法敌人一行。至少包含：

- `Id`、`Archetype`、`BehaviorProfileId`、`PresentationId`、`Enabled`；
- `MaxHealth`、`MoveSpeedCmPerSecond`、`CollisionRadiusCm`、`CollisionHalfHeightCm`；
- `ContactDamage`、`AttackIntervalSeconds`、`ContactRangeCm`、`MovementStopDistanceCm`；
- `HitReactionDurationSeconds`、`KnockbackSpeedCmPerSecond`、`KnockbackDrag`；
- Bomber 的 `TriggerRadiusCm`、`DamageRadiusCm`、`FuseSeconds`；
- `Reward`、`Boss`、`SourceSheet`、`SourceRow`、`Notes`。

迁移 `M_Grunt`、`M_Shield`、`M_Bomber`、`M_TimeGuard`。非适用数字字段必须使用明确中性值并由 `Archetype` 条件校验，不能依赖空白解析成0。

#### `EnemyAbilities` / `tblEnemyAbilities` / `enemy_abilities.csv`

每个 Boss 主动/被动技能一行。至少包含：

- `Id`、`OwnerEnemyId`、`BehaviorId`、`SequenceOrder`、`Enabled`；
- `Damage`、`WindupSeconds`、`ActiveSeconds`、`RecoverySeconds`、`CooldownSeconds`；
- `MinRangeCm`、`MaxRangeCm`、`RadiusCm`、`WidthCm`、`LengthCm`；
- `ProjectileSpeedCmPerSecond`、`TeleportOffsetCm`；
- `TargetingMode`、`LockTiming`、`CleanseIntervalSeconds`、`ImmunitySeconds`；
- `SourceSheet`、`SourceRow`、`Notes`。

字段按 `BehaviorId` 做条件校验。例如 `Boss.Projectile` 必须有正弹速，`Boss.BlinkSlam` 必须有预警和半径，`Boss.ElementCleanse` 的伤害/空间字段必须为中性值。

#### `BossPhases` / `tblBossPhases` / `boss_phases.csv`

每个 Boss 阶段一行。至少包含：

- `Id`、`BossEnemyId`、`PhaseIndex`、`TriggerSeconds`、`EchoPolicy`；
- `PhysicalAttackMultiplier`、`ElementalAttackMultiplier`、`AttackSpeedMultiplier`、`MovementSpeedMultiplier`；
- `RefillHealthPolicy`、`Enabled`、`SourceSheet`、`SourceRow`、`Notes`。

首版只配置 `M_TimeGuard` 的 30 秒阶段：销毁本场 Echo，强化当前玩家且不改变 Run 永久 Build。该临时强化需要进入遭遇保存快照，恢复时不得重复乘算。

### 失败策略

- 三张表进入 manifest/required table 集，缺表、重复 ID、未知外键、未知 `BehaviorId`、非法范围、Boss 没有至少一个可执行主动招式或阶段重复时，Registry 加载失败并报告文件/行/字段。
- 生产 Host 配置未知/禁用 `EnemyId` 时使用项目既有 Fatal 配置策略；不得回退到 `MakeLegacyEquivalent`、JSON 或 C++ 默认值。
- 旧 JSON 删除后，校验器禁止 `Content/Data/enemies.json` 回归，并禁止生产路径重新调用 `MakeLegacyEquivalent`；纯测试 fixture 可显式构造 Definition。

## 审核决策：策划案缺失参数的建议默认值

以下数值不是策划案原文。用户在完成独立工作簿与模块解耦审核后授权执行，首版按本节建议值实施；最终数值仍可由怪物策划在独立工作簿中调整。

| 技能 | 建议参数 | 设计意图 |
|---|---|---|
| `Boss.MeleeSweep` | 伤害18；范围260cm；宽220cm；前摇0.65s；判定0.12s；后摇0.75s；冷却2.8s | 近身基础招，轮廓明确、可横向躲避 |
| `Boss.Projectile` | 伤害14；弹速700cm/s；命中半径55cm；前摇0.60s；后摇0.55s；冷却3.5s；合法距离300~1100cm | 中远程压制，锁定前摇结束时位置，不追踪 |
| `Boss.BlinkSlam` | 伤害22；落点半径180cm；消失0.35s；地面预警0.90s；后摇0.90s；冷却7.0s；出现在目标180cm外 | 强制位移和明显范围躲避，不直接贴脸无预警命中 |
| `Boss.PrayerBeam` | 伤害26；长度1200cm；宽160cm；前摇1.20s；判定0.15s；后摇1.0s；冷却9.0s | 高威胁长直线；最后一帧锁点，随后不追踪 |
| `Boss.ElementCleanse` | 每9.0s；免疫1.0s | 完全采用策划案 |
| 30秒玩家强化 | 物理攻击×2.0、元素攻击×2.0、攻速×1.5、移速×1.25；不改最大生命、不回血 | “战斗属性大幅提高”具体化；避免中途改血量语义 |

已确认：

- 怪物体系由另一位策划配置，使用独立 `ReEchoEnemyData.xlsx`；不在主 `ReEchoData.xlsx` 中建立怪物生产表。
- Boss 实现不要求代码物理上只修改 `ReEchoEnemies`；验收标准是保持怪物模块解耦初衷：行为决策、状态和确定性计时归 `ReEchoEnemies`，世界执行归 EnemyHost，伤害/元素归 `ReEchoCombat`，Encounter/Run 负责编排，Presentation/Audio/UI 只读消费事件。
- 首版采用上表技能数值、固定顺序轮转加条件跳过、30 秒四项强化倍率且不回血/不提高最大生命。
- 本 Plan 保持当前 6 场且 Boss 为第 6 场；策划案的 8 房间与完整刷怪系统另立 Plan。
- 用户于 2026-08-14 授权开始执行，并要求使用独立 worktree 文件夹。

## 实现提纲

1. **固定基线**：新增当前 Boss 30秒失败、接触攻击和 JSON/C++ 数据来源的回归测试；记录策划案差异。
2. **独立怪物工作簿与生成器**：新建 `ReEchoEnemyData.xlsx`，建立三个 Excel Table、下拉/说明/样例；扩展跨工作簿 ExportMap、Schema、manifest、同步测试和字节一致性校验；生成三张 CSV，且不改主 `ReEchoData.xlsx` 的怪物生产内容。
3. **Registry 与编译适配**：实现行类型、Reader、条件校验、稳定查找和 Run snapshot；Host 通过主模块 adapter 把 CSV 行编译为 Enemies 公共 Definition，移除生产 legacy fallback 和旧 JSON。
4. **Boss 纯逻辑**：在 `ReEchoEnemies` 实现四招选择/阶段/计时/锁点/唯一 AttackIdentity、9秒清洗意图、30秒阶段意图和确定性快照；用无世界测试覆盖节拍与边界。
5. **Combat 清洗命令**：提供窄的清除附着/灼烧并授予指定免疫时长 API，发布元素状态事件；Boss 不直接编辑 Combat 私有状态。
6. **世界攻击 Host**：实现挥击、光束和砸地的空间查询，投射物 logic/Host，闪身位置求解和碰撞安全；全部命中只进入 `FReEchoHitIntent -> ResolveHit`。
7. **Boss 房间阶段**：EncounterDirector 支持 `TimeoutSurvive` 与 `DefeatBoss` 结束模式；Boss 房30秒不结束，只触发 Echo 退场和一次性临时玩家强化，直到玩家或 Boss 死亡。
8. **保存恢复**：保存当前 Boss 技能阶段/计时/锁点/清洗/30秒阶段、临时玩家倍率和存活敌方投射物；恢复后不重放已提交伤害、不重复强化、不重置轮转。
9. **只读表现事件**：为现有 Boss fallback 增加清晰预警和语义事件；资源失败不影响逻辑。正式动画/VFX/音频由资源侧后续消费同一契约。
10. **文档、回归与发布**：同步 CODEBASE_MAP 和策划说明，运行聚焦/完整测试；用户 PIE 通过后 FullRebuild、提交并推送 main，清理可安全删除的本地工作。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| XLSX/CSV | `sync_xlsx_to_csv.py --check`、同步测试、`validate_project.py` | 独立怪物工作簿与三表字节一致；跨工作簿 manifest、下拉/Schema/外键/条件字段正确；主工作簿无怪物生产镜像；旧 JSON 不再生产加载 |
| 数据 Registry | `ReEcho.Data.Enemies.*` | 四敌人定义、五技能/被动、Boss阶段可查；缺表/坏字段/未知行为按字段失败 |
| Boss 纯逻辑 | `ReEcho.Enemies.Boss.*` | 四招轮转/跳过、锁点、冷却、清洗、阶段、暂停和快照恢复确定性 |
| Combat | `ReEcho.Combat.*` + Boss focused | 清洗移除附着/灼烧、1秒免疫、事件发布；所有伤害只走 HitResolver |
| 空间攻击 | Boss Host/projectile tests | 挥击/光束/砸地/投射物边界、同招同目标一次命中、AttackIdentity 稳定 |
| Encounter | Boss phase/termination tests | 30秒不结束；Echo 恰好退场一次；玩家强化一次；玩家死失败/Boss死胜利 |
| Save | Boss mid-windup/mid-projectile/post-30s round trip | 恢复后计时、锁点、投射物和倍率一致，无重复伤害/强化 |
| 非 Boss 回归 | `ReEcho.Enemies.*`、Run、Echo、Weapons focused | Grunt/Shield/Bomber 行为与稳定 ID 无回归 |
| 构建 | `Build-Editor.cmd -Configuration Development` | UE 5.8 UHT/UBT 成功 |
| 完整自动化 | `Run-Automation.cmd -Filter ReEcho` | 发现套件通过；既有失败单独记录，不伪装通过 |
| 人工 | 用户 PIE | 四招预警可读且可躲；清洗、30秒退场/强化、死亡/胜利、保存继续手感正确 |
| 发布 | `Build-Editor.cmd -Configuration Development -FullRebuild` | 最新最终候选五模块与精选预构建包一致 |

## 执行记录

### 变化

- 2026-08-14：读取策划案和当前主线。确认 Boss 当前只有 legacy 接触攻击；`怪物体系M` 无生产 Table；Boss 房当前30秒未击杀即失败，与策划案冲突。
- 2026-08-14：本 Plan 以 `Proposed` 写入本地，等待用户审核缺失参数和范围；尚未发布、尚未开始实现。
- 2026-08-14：用户确认怪物体系由另一位策划独立配置；Plan 改为新增 `ReEchoEnemyData.xlsx` 作为怪物唯一策划真源，不再向主 `ReEchoData.xlsx` 写入怪物生产表。
- 2026-08-14：用户确认 Boss 跨模块窄适配可以接受，只要求符合怪物模块解耦初衷；不以“所有代码只能写在 `ReEchoEnemies`”作为验收条件。
- 2026-08-14：用户授权执行，接受建议首版参数、固定轮转加条件跳过、30 秒倍率且不改生命，以及当前 6 场范围；指定使用 `ReEcho-plan44` 独立 worktree 开发。

### 证据

- 执行授权前 `git fetch --prune origin`：本地/远端 main 均为 `348d2a84232676164a64f8cfb5a49f836b65ba91`，无外部提交耦合。
- 策划案结构化读取成功：Boss 基础数值、四个招式描述、9秒清洗/1秒免疫、30秒 Echo 退场和玩家强化已确认；原文没有提供四招完整数值或强化倍率。
- `ReEchoData.xlsx`：`怪物体系M` 仅7行参考文字，标记 `ReferenceOnly`，没有 Excel Table；ExportMap 当前17张表且无敌人表。
- 当前生产路径：`Content/Data/enemies.json`、`ReEchoEnemyDefinitions::MakeLegacyEquivalent`、`AReEchoEnemyActor::Configure`；`AReEchoEncounterDirector` 在30秒广播结束，Boss未击杀进入 Failed。
- DOCX 的 OOXML/正文/15张表可结构化读取；本机缺少 LibreOffice，Word 对该文件报“文件可能已经损坏”，因此未取得可靠页面渲染证据。本 Plan 只依据成功解析的结构化内容，不声明视觉版式验收。

### 剩余风险

- 首版参数已获执行授权，但属于程序建议初值；怪物策划后续可在独立工作簿中调参，不能把这些值重新硬编码到 C++。
- 策划案同时要求8房间和全新刷怪体系；本 Plan 明确不实现，Boss 暂时仍是当前第6场。
- Boss 投射物与阶段保存会扩展当前遭遇快照；实现时必须以旧存档兼容和不重复提交伤害为优先。

### 人工验收结果/请求

- 审核决策已完成；状态保持 `PendingBeforeClose`。实现交付后由用户执行 Boss 四招、躲避手感、30 秒阶段、胜负和保存继续的 PIE 验收。

### 架构文档审阅结果

- 规划阶段已审阅 `ARCHITECTURE.md`、CODEBASE_MAP 索引、`MOD-ReEchoEnemies.md`、`MOD-ReEcho.md` 与 `MOD-ReEchoCombat.md`；正文更新将在实现与真实代码落点确定后完成。
