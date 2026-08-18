# `MOD-ReEchoEnemies` 详细设计

## 模块状态

- Runtime Module：`ReEchoEnemies`。
- 代码根：`Source/ReEchoEnemies/`。
- 架构标识：`MOD-ReEchoEnemies`；功能检索标识：`AREA-Enemies`。
- 当前状态：通用逻辑边界、数据驱动 Boss，以及开普勒史莱姆/兔子/狐狸类型化行为均位于本模块；Encounter/Spawn 协调仍由主模块负责。

## 存在原因

旧 `AReEchoEnemyActor` 同时执行 AI、接触攻击、爆破引信、受击击退、Combat 接线、Sprite/动画、血条和特效。逻辑开发与资源/表现接入因此持续争用同一个 Actor 文件，而且动画生命周期、世界查询和伤害规则容易形成隐式耦合。

本模块把资源无关的怪物行为收敛为确定性逻辑边界：Host 显式提供一次世界感知快照，EnemyLogic 返回移动/攻击意图；Combat 仍独占生命、元素、伤害与死亡裁决；Presentation 未来只读事件和快照。没有 Sprite、动画、音频、UI 或 GameMode 时，怪物行为仍可编译和测试。

## 职责与排除项

**负责：**

- 怪物 Archetype、行为阶段、攻击冷却、攻击序号与存活行为门控；
- 兼容 Grunt/Shield/Bomber、开普勒 Slime/Ranged/Elite 和 Boss 的不可变 Definition；生产 Definition 由主模块从独立怪物工作簿生成的 CSV 编译后注入；
- Host 显式注入的目标感知到移动、朝向和攻击意图的确定性转换；
- Host 可在不改写 EnemyLogic 内部状态机的前提下应用外部眩晕门、移动倍率，并为当步 Sense 选择存活嘲讽 Echo；
- Bomber 不可取消引信、范围判定输入与一次性自毁提交；
- Slime 表驱动接触攻击、Ranged 锁点前摇/范围判定、Elite 正面防御/锁向突进与恢复；
- Combat Hurt 结果触发的游戏性击退状态，以及 Combat Death 后停止产出行为；
- 表现中立的 EnemyEvents 与只读 LogicSnapshot；
- 无世界纯逻辑回归。

**不负责：**

- 玩家或 Echo 武器、自动/手动攻击、最终伤害、格挡、元素、生命和死亡结算；
- GameMode、出生规则、遭遇结束、世界扫描、Actor Transform/Collision 的实际修改；
- XLSX/CSV 读取、资源路径、Sprite、动画、VFX、血条、伤害数字、音频和 UI；
- 表现播放完成、Actor 延迟销毁或资源加载结果。

## 权威状态

| 状态 | 唯一所有者 | 外部使用方式 |
|---|---|---|
| Archetype、Phase、SpawnIndex、攻击冷却、Fuse、受击剩余时间、击退速度、朝向、攻击序号 | `UReEchoEnemyLogicComponent` | `GetSnapshot()`；Host 不复制可写计时器 |
| 怪物不可变行为参数 | `FReEchoEnemyDefinition` | Host 初始化时按值注入；运行中不回读资源/配置对象 |
| 目标与世界位置样本 | EnemyHost 每步构造的 `FReEchoEnemySenseSnapshot` | Logic 只消费当步值，不自行搜索 PlayerController/GameMode |
| Actor Transform、Collision、实际 swept movement | 主模块 EnemyHost | 应用 `FReEchoEnemyActionIntent`，不能把结果回写成第二套 AI 状态 |
| 生命、元素、伤害、格挡、击杀与死亡 | `MOD-ReEchoCombat` | CombatEvents 与 CombatantSnapshot；Enemies 不直接扣血 |
| 当前动画、Sprite、VFX、血条与伤害数字 | 主模块 Presentation | 订阅事件、读取聚合快照；不控制 Logic |

完整保存由 `FReEchoEnemyRuntimeState` 聚合 `Transform + EnemyLogic 权威字段 + Combatant 生命/元素`；LogicSnapshot 不重复 Combat 的生命或元素状态，表现瞬时状态不进入保存。

## 输入、输出与公共契约

### 输入

- `FReEchoEnemyDefinition`：资源无关的不可变行为定义，携带稳定但资源无关的 `PresentationId`。当前 `MakeLegacyEquivalent` 固定现有 Grunt/Shield/Bomber/Boss 数值和兼容 ID，生产定义由主模块从 CSV 编译后注入。
- `FReEchoEnemySenseSnapshot`：目标弱引用、Self/Target 位置、时间、目标存在/存活/无敌状态，以及 Encounter 注入的 `bSpecialActionPermitted`。Logic 不允许通过 `FindComponentByClass`、GameMode 或全世界扫描补输入。
- `BindEventSources(EnemyEvents, CombatEvents)`：由 Host 显式注入两个事件源。Logic 订阅 Combat Hurt/Death，不发现兄弟组件。
- `NotifyHurt`、`NotifyDeath`、`RestoreSnapshot`：窄命令入口，供 Host/保存适配与测试使用。

### 输出

- `FReEchoEnemyActionIntent`：单步朝向、移动距离、普通攻击候选和 `BossIntents`。Boss Intent 表达前摇、判定窗口、清洗和 EncounterPhase，仍不是最终命中结果。
- `FReEchoEnemyLogicSnapshot`：只读行为副本；除通用状态外保存 Boss 当前招式、阶段剩余时间、轮转索引、锁点、清洗/阶段门控与固定步累计，支持中途恢复。
- `FReEchoEnemyProjectileLogic`：资源无关的直线弹道 Advance/Snapshot；Host 只负责世界目标碰撞和 Combat 命中转发。
- `UReEchoEnemyEventsComponent`：发布 `FReEchoEnemyActionCommittedEvent` 与 `FReEchoEnemyFuseEvent`。事件只描述已经发生的行为状态，不携带表现资源。
- `UReEchoEnemyRosterComponent`：保存 Host/Logic 弱引用，以 SpawnIndex 稳定排序；存活状态即时读取 LogicSnapshot，不复制第二份 alive 标志。

普通接触攻击在进入范围且 cooldown ready 时提交；目标无敌仍消费 cooldown。兔子/狐狸只有获得 Encounter 的全局许可才可开始前摇，已开始的动作不被撤销；兔子锁点后按半径判断，狐狸锁向后按长度/宽度突进，正面防御沿用 Definition 的明确能力标志。全局窗口与并发令牌不保存在单个 EnemyLogic。Bomber 引信不可取消且只提交一次。

## 依赖方向

```text
ReEchoEnemies ─────────→ ReEchoCombat

ReEchoEnemies ─/─→ ReEcho / ReEchoWeapons / ReEchoAudio
ReEchoEnemies ─/─→ GameMode / UI / Paper2D / Presentation assets
```

模块公共依赖仅为 Core、CoreUObject、Engine 与 `ReEchoCombat`。`ReEchoCombat` 不反向依赖 Enemies；Cards 规则由主模块 Host 翻译成 Sense/推进门，不形成 `ReEchoEnemies -> ReEchoCards`。未来 EnemyHost 位于主模块，负责组合模块；不得为省接线把 Host 或资源类型下沉到 Enemies。

## 运行时流程

### 单步行为

```text
Encounter / GameMode 提供世界上下文
  → Encounter coordinator 统一判断远程窗口/精英并发许可
  → EnemyHost 构造 EnemySenseSnapshot（包含只读许可）
  → EnemyLogic::Advance(Sense, Delta)
  → EnemyActionIntent
      → Host 应用 Capsule swept movement
      → Host 将攻击候选转换为 Combat HitIntent
      → Combat ResolveHit 并更新唯一生命/元素状态
  → EnemyEvents / CombatEvents
      → Presentation 与 Audio 各自只读消费
```

逻辑事件的订阅者不位于 `Advance` 的返回路径，不能取消攻击、修改 cooldown、延长 Fuse 或阻止死亡。

### 受击与死亡

```text
Combat OnHurt
  → EnemyLogic 启动游戏性击退状态
  → 下一步产生击退 MovementDelta
  → Presentation 独立播放抖动/受击特效

Combat OnDeath
  → EnemyLogic 立即停止所有行为
  → Host 关闭玩法碰撞并更新 Roster
  → Presentation 可继续播放死亡残留，但不阻挡遭遇结束
```

### 当前候选接线状态

`AReEchoEnemyActor` 已成为轻量 Host：显式构造 Sense、推进 Logic、应用 swept movement、把攻击候选交给 Combat，并聚合保存；不再保存 AI cooldown、Fuse、AttackSequence、击退或表现计时器。Plan47 在 Host 层增加卡牌眩晕/移动倍率，并由 GameMode 为普通攻击与 Boss 投射物统一选择最近存活嘲讽 Echo；EnemyLogic 仍不读取 Cards。`AReEchoGameMode` 通过 Roster 生成、恢复、清理、捕获存档和判断全灭。Host 仅把 Definition 的 `PresentationId` 传给 `UReEchoEnemyPresentationComponent`；后者在主模块 Catalog 中解析 Profile，EnemyLogic 不依赖 Blueprint、Paper2D 或资产路径。

## 代码位置与阅读路线

| 目的 | 先读代码 | 说明 |
|---|---|---|
| Definition、Sense、Intent、Snapshot | `Public/Enemies/ReEchoEnemyTypes.h` | 全部为资源无关值契约 |
| 行为组件 API | `Public/Enemies/ReEchoEnemyLogicComponent.h` | 显式初始化、推进、事件注入和快照命令 |
| 追踪、接触、锁点远程、精英突进、Fuse、击退 | `Private/Enemies/ReEchoEnemyLogicComponent.cpp` | 当前行为规则唯一实现 |
| 行为事件 | `Public/Enemies/ReEchoEnemyEventsComponent.h` | Presentation-neutral 的行为总线 |
| 本场敌人集合 | `Public/Enemies/ReEchoEnemyRosterComponent.h` → `Private/Enemies/ReEchoEnemyRosterComponent.cpp` | 代替 GameMode 重复世界扫描的单一弱引用注册表 |
| 规则回归 | `Private/Tests/ReEchoEnemyLogicTests.cpp` | 旧数值、节拍、无敌语义、Fuse、快照与死亡 |
| 模块入口 | `Public/ReEchoEnemies.h`、`Private/ReEchoEnemies.cpp` | Runtime Module 注册 |
| 世界装配 | `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*` | 轻量 Host、Sense/Intent/Combat/保存适配 |
| 敌人表现 | `Source/ReEcho/{Public,Private}/Presentation/Enemy/ReEchoEnemyPresentationComponent.*` | 只读快照/事件、Profile/贴图、动画/VFX/血条 |
| 主流程 Roster/全局技能令牌接线 | `Source/ReEcho/{Public,Private}/ReEchoGameMode.*` | 敌人生成、恢复、清理、保存及远程窗口/精英并发单一协调 |
| Host 集成回归 | `Source/ReEcho/Private/Tests/ReEchoEnemyHostTests.cpp` | Logic/Combat/Transform/Roster 保存组合 |

## 扩展方式

- 新怪物 Archetype：先在 `ReEchoEnemyData.xlsx` 注册稳定 ID/Behavior，再扩资源无关 Definition/状态机与聚焦测试，最后由 Host/Presentation 分别增加数据编译和外观映射；不要在 Logic 引入资产类。
- 新感知条件：在 `FReEchoEnemySenseSnapshot` 增加稳定值字段，由 Host 采样；不要让 Logic 查询 GameMode、PlayerController 或世界 Actor。
- 新攻击类型：EnemyLogic 只产生动作身份和候选参数，Host 转为 `FReEchoHitIntent`，最终裁决仍只进 Combat Resolver。
- 新表现反馈：订阅 EnemyEvents/CombatEvents 或读取聚合 PresentationSnapshot；不向 Logic 添加动画完成回调。
- 新保存字段：仅保存权威状态，并提供版本化迁移；不要把派生 UI/表现状态或 Combat 生命复制进 LogicSnapshot。

## 验证与测试

### 当前远程伤害安全状态（2026-08-18）

敌方远程能力仍由 EnemyLogic 正常产生意图、由 Host 转换并交给 Combat；目前仅因投射物/预警表现不可见，在权威 `ReEchoEnemyData.xlsx / EnemyAbilities` 中把 `M_TimeGuard_Projectile`、`M_TimeGuard_BlinkSlam`、`M_TimeGuard_PrayerBeam` 和 `M_RABBIT_RangedBurst` 的 `Damage` 临时设为 `0`。不要在 Logic、Host 或 Combat 增加第二份禁伤开关。美术接入并通过 PIE 可读性验收后，直接恢复表格数值并重新发布 CSV。近战、突进和接触伤害不受影响。

- `ReEcho.Enemies.Logic.LegacyDefinitions`：四类怪物现有数值等价。
- `ReEcho.Enemies.Logic.ContactCadence`：移动与同帧攻击意图可共存，冷却按现有语义推进。
- `ReEcho.Enemies.Logic.InvulnerableTargetConsumesAttack`：无敌只阻止伤害候选，不回滚动作与 cooldown。
- `ReEcho.Enemies.Logic.BomberFuse`：Fuse 不可取消、范围语义正确且只自毁一次。
- `ReEcho.Enemies.Logic.HurtAndSnapshot`：击退、快照恢复与死亡门控。
- `ReEcho.Enemies.Logic.Roster`：去重注册、稳定顺序、无复制存活查询与清理。
- `ReEcho.Enemies.Logic.RangedAndEliteBehaviors`：兔子锁点可躲避、狐狸锁向突进与表驱动伤害/时序。
- 命令：`scripts/ue/Build-Editor.cmd -Configuration Development`；`scripts/ue/Run-Automation.cmd -Filter ReEcho.Enemies.Logic`。
- `scripts/validate_project.py` 固定模块依赖和 include 边界，并拒绝 World 扫描、隐式兄弟组件发现、直接伤害调用及 Content 资源路径。
- `ReEcho.Enemies.Host.CompositionAndSave`、`ReEcho.Run.SaveSnapshot` 与 Combat ElementReaction World 测试覆盖 Host/Combat/Roster/Save 接缝。
- Plan47 跨域回归覆盖 10/20 秒眩晕、Echo 400cm 减速/元素光环、0.5 秒敌方元素免疫上限和嘲讽目标；规则与持久状态仍由 Cards/Run 测试负责。
- 怪物攻击、受击、爆破、Boss、动画和遭遇结束仍由用户 PIE 验收。

## 不变量与常见错误

- Enemies 拥有行为，Combat 拥有伤害/生命/元素，Host 拥有世界 Transform，Presentation 拥有可见反馈；任何一方不得复制另一方的可写真相。
- 卡牌眩晕与减速是 Host 当步输入；不得写回 EnemyLogic cooldown/Fuse 或为 Cards 增加 Enemies 反向依赖。
- EnemyHost 显式声明 `EnemySide`，攻击身份在提交时快照该阵营；敌人不得在自身 Logic 中复制玩家/回响类型判断。Bomber 自毁通过单次 HitIntent 的 `bAllowSameFactionDamage` 明确放行自身伤害，不能为此全局开启敌人互伤。
- Host 必须显式注入 Sense 和事件组件；组件内部 `FindComponentByClass` 会重新引入隐式装配和悬空 Actor 风险。
- 同一步可以同时产生移动和攻击；提交攻击不得用新空对象覆盖已计算的移动/朝向意图。
- Bomber Fuse 到期只提交一次；等待 Host/Combat 销毁的间隙不能再次爆炸。
- 事件表示已发生结果，订阅回调没有玩法否决权；表现缺失、资源加载失败和动画结束都不能改变行为。
- Snapshot 是只读副本，不是命令；外部不得修改副本后假定 Logic 状态已变化。
- 不引入 `ReEchoEnemies -> ReEchoWeapons`：敌人攻击节拍由 EnemyLogic 拥有，玩家武器节拍由 Weapons 拥有。
