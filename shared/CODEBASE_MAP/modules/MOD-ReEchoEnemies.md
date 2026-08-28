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
- Host 负责应用外部眩晕门和移动倍率，并为当步 Sense 选择存活嘲讽 Echo；首次进入眩晕时通过窄命令取消旧目标锁定动作；`M_SHEEP` 在配置边界启用 Combat 眩晕免疫，并拒绝 Host 卡牌眩晕计时；
- Bomber 不可取消引信、范围判定输入与一次性自毁提交；
- Slime 表驱动接触攻击、Ranged 锁点前摇/范围判定、Elite 正面防御/锁向突进与恢复；
- Combat Hurt 结果触发的游戏性击退状态，以及 Combat Death 后停止产出行为；
- 表现中立的 EnemyEvents 与只读 LogicSnapshot；
- 表现中立的特殊动作阶段事件和逻辑投射物 Spawned/Moved/Ended 生命周期；
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
| Actor Transform、Collision、实际 swept movement | 主模块 EnemyHost | 应用 `FReEchoEnemyActionIntent`；普通移动不回写，Elite Active 冲撞只通过 `ResolveSpecialDashStep` 回报遇阻与首次接触门控 |
| 生命、元素、伤害、格挡、击杀与死亡 | `MOD-ReEchoCombat` | CombatEvents 与 CombatantSnapshot；Enemies 不直接扣血 |
| 当前动画、Sprite、VFX、血条与伤害数字 | 主模块 Presentation | 订阅事件、读取聚合快照；不控制 Logic |

完整保存由 `FReEchoEnemyRuntimeState` 聚合 `Transform + EnemyLogic 权威字段 + Combatant 生命/元素`；LogicSnapshot 不重复 Combat 的生命或元素状态，表现瞬时状态不进入保存。

## 输入、输出与公共契约

- Enemy Host 的 `ConfigureFromDefinition` 是出生 Commit 的原子激活边界：成功返回时 Combatant 必须存活，Actor 与根玩法碰撞必须启用且可受击。出生预警期间不创建 Host；表现资源加载或 Blueprint 默认值不得延长不可受击阶段。

### 输入

- `FReEchoEnemyDefinition`：资源无关的不可变行为定义，携带稳定但资源无关的 `PresentationId`。当前 `MakeLegacyEquivalent` 固定现有 Grunt/Shield/Bomber/Boss 数值和兼容 ID，生产定义由主模块从 CSV 编译后注入。
- `FReEchoEnemySenseSnapshot`：目标弱引用、Self/Target 位置、时间、目标存在/存活/无敌状态，以及 Encounter 注入的 `bSpecialActionPermitted`。Logic 不允许通过 `FindComponentByClass`、GameMode 或全世界扫描补输入。
- `BindEventSources(EnemyEvents, CombatEvents)`：由 Host 显式注入两个事件源。Logic 订阅 Combat Hurt/Death，不发现兄弟组件。
- `NotifyHurt`、`NotifyDeath`、`RestoreSnapshot`：窄命令入口，供 Host/保存适配与测试使用。狐狸 Active 冲撞另由 Host 调用 `ResolveSpecialDashStep`，只反馈实际 Sweep 是否遇阻与本次路径是否已消费首次接触，不传世界对象或伤害结果。
- `CancelActiveActionsForStun`：Host 仅在首次进入眩晕时调用；取消普通特殊技或 Boss 当前动作、发布对应终止事件并清空旧锁点，保留普通冷却、Bomber Fuse、受击状态、身份与生命。
- `ResetEncounterTransientState`：同 Stage 局间边界的窄命令。取消尚未提交的 Fuse/特殊行动/Boss 行动、受击位移和瞬时计时，但保留 Actor 身份、生命、普通攻击冷却、攻击序号及其他持久逻辑；Enemy Host 负责在调用后冻结自己的 World Tick 和逻辑投射物。

### 输出

- `FReEchoEnemyActionIntent`：单步朝向、移动距离、普通攻击候选和 `BossIntents`。Boss Intent 表达前摇、判定窗口、清洗和 EncounterPhase，仍不是最终命中结果。
- `FReEchoEnemyLogicSnapshot`：只读行为副本；除通用状态外保存普通特殊技能的下一轮转索引与当前活动 AbilityId。Elite 冲撞还保存 Active 剩余时间、剩余距离、同一次 `AttackIdentity` 与首次接触是否已消费；Boss 保存当前招式、阶段剩余时间、轮转索引、锁点、清洗/阶段门控与固定步累计，支持中途恢复。
- `FReEchoEnemyProjectileLogic`：资源无关的直线弹道 Advance/Snapshot；Host 只负责世界目标碰撞和 Combat 命中转发。
- `UReEchoEnemyEventsComponent`：除提交/Fuse 外，发布 `FReEchoEnemySpecialActionEvent`（WindupStarted、ActionCommitted、RecoveryStarted、ActionEnded、ActionCancelled）与 `FReEchoEnemyProjectileEvent`（Spawned、Moved、Ended）。事件只描述已经发生的行为状态和空间上下文，不携带表现资源。
- `UReEchoEnemyRosterComponent`：保存 Host/Logic 弱引用，以 SpawnIndex 稳定排序；存活状态即时读取 LogicSnapshot，不复制第二份 alive 标志。

普通接触攻击在进入范围且 cooldown ready 时提交；目标无敌仍消费 cooldown。兔子/狐狸只有获得 Encounter 的全局许可才可开始前摇，已开始的动作不被撤销；同一普通怪的多个启用特殊能力按 `SequenceOrder + AbilityId` 稳定排序并确定性循环，前摇、提交、恢复和快照恢复全程按 `SpecialAbilityId` 绑定同一能力。兔子因此依次执行移动散射与站定连发；兔子锁点后按半径判断。狐狸在 Windup 结束时提交一次身份，随后在 `ActiveSeconds` 内沿锁向分步积分 `LengthCm`；每步最多消费剩余距离，完成或 Host 回报世界阻挡后进入只含 `RecoverySeconds` 的 Recovery。首次路径接触门在无敌零伤害时也消费，保证同次冲撞不重试；正面防御沿用 Definition 的明确能力标志。全局窗口与并发令牌不保存在单个 EnemyLogic。Bomber 引信不可取消且只提交一次。

远程敌人提交不再直接按锁点范围结算，而是由 EnemyHost 用 `FReEchoEnemyProjectileLogic` 按能力表的 `ProjectileCount` 与 `SpreadAngleDegrees` 展开确定性直线投射物；单发沿锁定方向，有散射角的多发以中心方向对称齐射。`ProjectileCount>1`、零散射角且 `ActiveSeconds>0` 的直线多发在该 Active 窗口内等间隔进入世界：第一发随提交生成，其余已提交球保存剩余延迟并依次发布 Spawned，避免同帧同位置重叠。Host 使用每条已生成球的移动线段与“按单球半径扩张后的目标碰撞盒”做连续扫掠，每球向 Combat 提交至多一次命中；兔子球命中玩家后立即发布 `Ended` 并从逻辑数组移除，视觉代理随事件结束。长剑还可用其提交时的 180°近战扇区结束弧内兔子球；EnemyHost 仍是移除权威，Niagara 不参与斩弹裁决。兔子能力的单球半径固定为 `RadiusCm / 3`（沿用原移动三球散射的作者基准），不因站定四连发的 `ProjectileCount` 改变；羊 Boss 等通用齐射仍把 `RadiusCm` 作为整组碰撞预算并按实际发数平分。单球半径不得从敌人本体碰撞尺寸推导，也不得让 Niagara 粒子参与裁决。各逻辑轨迹分别发布 Spawned/Moved/Ended，事件携带共享 AttackIdentity、稳定 `VolleyBallIndex`、逻辑位置/方向和只读碰撞半径；Presentation 必须以 `(AttackIdentity, VolleyBallIndex)` 一一投影。当前保存数组沿用兼容字段名 `BossProjectiles`，但承载已生成和已提交待生成的通用敌方逻辑投射物；旧存档默认把已有条目视为已生成，重命名需要独立存档迁移。显式正数表内投射物速度优先，兼容数据才按 `MaxRangeCm / CooldownSeconds` 推导。

Plan68 的生产阵容由独立怪物工作簿驱动：普通怪为 `M_SLIME`、`M_RABBIT`、`M_FOX`，Boss 为 `M_SHEEP`。普通怪的 `AttackCountOrRange` Phase2 是纯表现转换，转换期间继续接受正常伤害；`M_SHEEP` 第一阶段生命使用怪物定义，第一次致命伤由 Combatant 的窄委托交给 EnemyLogic 转为 `HealthDepleted` Phase2 过渡，Host 发布变身事件，完成后按 Phase2 定义刷新最大生命与当前生命；第二次致命伤沿正常 Combat 死亡路径。未进入仇恨范围的普通怪执行可保存的确定性 IdleWander，一旦进入战斗后不恢复游走；Host 只注入 `bInCombat`、`HateRangeCm` 和当前生命比率，Logic 不读取 GameMode 或 Combatant。

Plan96 补齐羊 Boss 阶段战斗倍率的消费边界：一阶段仍直接使用 `EnemyAbilities.Damage/CooldownSeconds`；进入 `CurrentPhaseIndex=2` 后，EnemyLogic 在提交 Intent 时把物理伤害乘当前 `BossPhases.PhysicalAttackMultiplier`，并用 `CooldownSeconds / AttackSpeedMultiplier` 设置技能冷却。Host 仍只把解析后的 `RawDamage` 交给投射物或矩形/圆形/光束空间判定，最终扣血只进入 `FReEchoHitIntent -> ReEchoHitResolver`；Niagara 不参与范围或伤害裁决。

羊 Boss 的所有蓄力 Niagara 均挂接 `BossWeaponTipRoot`；该节点在 `BossWeaponFacingRoot` 局部 +Z 方向偏移 MoonStaff 最终世界长度的一半，落在中心 Pivot 法杖贴图的顶部，并继承 `BossWeaponRoot` 的基础挂点/挥舞旋转和 MoonStaff DA 最终左右偏移；该顶部挂点缺失时才回退通用 `AttackVfxRoot`。站定四连弹与移动三向散射复用通用敌方逻辑投射物链：前者在 Recovery 窗口内依次进入世界，后者同帧按三向扇形进入世界；每颗独立连续扫掠并提交单弹伤害，Skill02 的 Ability `RadiusCm` 不触发一次性 AOE。BlinkSlam 的落点、预警和圆形伤害判定统一以 Intent 锁定中心为权威，圆半径直接使用 Ability `RadiusCm`，不再回读闪现后的 Actor 位置；AttackWindow 先把 gameplay Host 闪现到锁定落点，Presentation 根节点再从上方向落点缓入 0.5 秒，完成后才结算伤害并生成地裂，地裂保留锁定落点 XY、只对齐蓄力开始时目标 `GroundRoot` 的世界 Z，不生成移动拖尾。MeleeSweep 以羊的 `BossWeaponRoot` 世界位置为圆心、锁定朝向为中轴、Ability `LengthCm` 为半径覆盖前方 180° 半圆，伤害与挥杖特效共享挂点；PrayerBeam 在 WindupStart 保留技能 `LockedTargetLocation` 的 XY，只快照目标阴影使用的 `GroundRoot` 世界 Z 作为高度（缺失时依次回退 `FootRoot`、Arena `GameplayPlaneWorldZ` 和原锁定 Z），后续预警与光束复用该不可变组合位置，光束沿世界 +X 向上延伸，不在蓄力结束回读角色实时位置；其 `ActiveSeconds` 同时定义光束、落点预警和伤害检测窗口（当前为 3 秒），窗口内持续检测进入光束的目标，但每次释放最多结算一次伤害。Development 的 `GMBossDamageRange` 只读绘制这些同源几何，不参与命中裁决。

血条耗尽转换由 Combat 致命伤拦截启动；同一命中随后发布的 Hurt 不得把 `Transforming` 覆盖为 `HitReaction`。转换期间 Host 的入伤修正统一返回零，避免临时保活的 1 HP 被多段攻击击杀；完成事件再应用 Phase2 最大生命与回满策略，之后第二次致命伤恢复正常死亡。

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
      → Host 应用 Box swept movement
      → Host 将攻击候选或狐狸 Active 实际路径首次接触转换为 Combat HitIntent
      → Host 用窄命令回报狐狸遇阻/接触门
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
  → Host 关闭玩法碰撞并让 Roster 立即视为死亡
  → Presentation 按伤害来源方向，在死亡动画开始的 0.3 秒内施加独立的 90 cm 缓出位移
  → Host 停止后续 AI/攻击/表现推进，但继续推进已经脱离出手阶段的逻辑投射物
  → Presentation 清除普通 Hit/Attack/VFX/阴影，只独占播放一次非循环 Death
  → Death 播放完成由 Host 销毁 Actor；没有有效 Death Clip 时在当前死亡广播栈退出后的下一安全帧销毁
```

死亡击退只移动表现根节点，不改变 Actor 世界位置、碰撞或死亡时序。

### 当前候选接线状态

`AReEchoEnemyActor` 已成为轻量 Host：显式构造 Sense、推进 Logic、应用 swept movement、把攻击候选交给 Combat，并聚合保存；不再保存 AI cooldown、Fuse、AttackSequence、击退或表现计时器。Plan47 在 Host 层增加卡牌眩晕/移动倍率，并由 GameMode 为普通攻击与 Boss 投射物统一选择最近存活嘲讽 Echo；EnemyLogic 仍不读取 Cards。`AReEchoGameMode` 通过 Roster 生成、恢复、按 Stage 策略清理、捕获存档和判断全灭；同 Stage 局间由主模块 Host 的显式 suspension 停止 World 推进，并通过 `ResetEncounterTransientState` 取消旧 Encounter 的瞬时动作，普通攻击冷却等持久状态不消耗 UI 时间。Host 仅把 Definition 的 `PresentationId` 传给 `UReEchoEnemyPresentationComponent`；后者在主模块 Catalog 中解析 Profile，EnemyLogic 不依赖 Blueprint、Paper2D 或资产路径。

敌人死亡只发布类型化最终事实，不拥有时间碎片区间、随机种子、拾取物或余额。主模块在出生与读档恢复共用的装配函数中订阅死亡事件，由 Run 按死亡时 Encounter 解析一次掉落数额，再由 GameMode 在死亡位置生成通用时间碎片拾取物；因此同 Stage 留存敌人不会把出生关次误当奖励关次，玩家与 Echo 击杀也不会形成两条经济路径。

Plan79 在主模块 Host 世界移动层增加纯值 Crowd Steering：只修正 `Pursuing` 阶段的普通追踪位移，使用 SpawnIndex 稳定槽位、Roster 稳定邻居顺序、软分离和切向恢复。普通怪物互相忽略 swept movement 硬碰撞，Boss、玩家和场景仍硬阻挡；EnemyLogic 的 Phase/攻击/击退/特殊位移、Combat 命中与保存格式均不改变。短时受阻计时属于 Host 可丢弃世界协调状态，不进入 Snapshot。

## 代码位置与阅读路线

| 目的 | 先读代码 | 说明 |
|---|---|---|
| Definition、Sense、Intent、Snapshot | `Public/Enemies/ReEchoEnemyTypes.h` | 全部为资源无关值契约 |
| 行为组件 API | `Public/Enemies/ReEchoEnemyLogicComponent.h` | 显式初始化、推进、事件注入和快照命令 |
| 追踪、接触、锁点远程、精英突进、Fuse、击退 | `Private/Enemies/ReEchoEnemyLogicComponent.cpp` | 当前行为规则唯一实现 |
| 行为事件 | `Public/Enemies/ReEchoEnemyEventsComponent.h` | Presentation-neutral 的行为总线 |
| 直线投射物逻辑 | `Public/Enemies/ReEchoEnemyProjectileLogic.h` → Private 实现 | 位置、速度、最大射程与可保存 Snapshot；无世界/资源依赖 |
| 本场敌人集合 | `Public/Enemies/ReEchoEnemyRosterComponent.h` → `Private/Enemies/ReEchoEnemyRosterComponent.cpp` | 代替 GameMode 重复世界扫描的单一弱引用注册表 |
| 规则回归 | `Private/Tests/ReEchoEnemyLogicTests.cpp` | 旧数值、节拍、无敌语义、Fuse、快照与死亡 |
| 模块入口 | `Public/ReEchoEnemies.h`、`Private/ReEchoEnemies.cpp` | Runtime Module 注册 |
| 世界装配 | `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*` | 轻量 Host、Sense/Intent/Combat/保存适配 |
| 敌人表现 | `Source/ReEcho/{Public,Private}/Presentation/Enemy/ReEchoEnemyPresentationComponent.*` | 只读快照/事件、Profile/贴图、动画/VFX/血条 |
| 主流程 Roster/全局技能令牌接线 | `Source/ReEcho/{Public,Private}/ReEchoGameMode.*` | 敌人生成、恢复、清理、保存及远程窗口/精英并发单一协调 |
| Host 集成回归 | `Source/ReEcho/Private/Tests/ReEchoEnemyHostTests.cpp` | Logic/Combat/Transform/Roster 保存组合 |
| 群体移动纯值与碰撞策略 | `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyCrowdSteering.*`、`Private/Tests/ReEchoEnemyCrowdSteeringTests.cpp` | 槽位、分离、绕行、确定性、速度预算和 Boss 硬碰撞策略 |

## 扩展方式

- 新怪物 Archetype：先在 `ReEchoEnemyData.xlsx` 注册稳定 ID/Behavior，再扩资源无关 Definition/状态机与聚焦测试，最后由 Host/Presentation 分别增加数据编译和外观映射；不要在 Logic 引入资产类。
- 新感知条件：在 `FReEchoEnemySenseSnapshot` 增加稳定值字段，由 Host 采样；不要让 Logic 查询 GameMode、PlayerController 或世界 Actor。
- 新攻击类型：EnemyLogic 只产生动作身份和候选参数，Host 转为 `FReEchoHitIntent`，最终裁决仍只进 Combat Resolver。
- 新表现反馈：订阅 EnemyEvents/CombatEvents 或读取聚合 PresentationSnapshot；详细 Niagara 接法见 [`MOD-ReEchoVFX.md`](MOD-ReEchoVFX.md)，不向 Logic 添加动画完成回调。
- Boss GM 调试：`DebugQueueBossAbility` 只保存一个不可持久化的待触发 AbilityId；下一个合法固定步仍通过正式 Telegraph、锁点、提交、恢复和结束链执行，不直接生成伤害或表现。
- 新保存字段：仅保存权威状态，并提供版本化迁移；不要把派生 UI/表现状态或 Combat 生命复制进 LogicSnapshot。

## 验证与测试

### 当前远程伤害权威（2026-08-25）

敌方远程能力仍由 EnemyLogic 正常产生意图、由 Host 转换并交给 Combat；数值只消费 `ReEchoEnemyData.xlsx → enemy_abilities.csv`，不得在 Logic、Host、Combat 或 VFX 复制伤害常量或增加第二份禁伤开关。Plan109 当前两条兔子能力每球伤害均为 `1`；羊 Boss Skill02/03/04 一阶段分别为 `4/24/16`，二阶段继续由 `BossPhases.PhysicalAttackMultiplier=1.5` 得到 `6/36/24`。这些当前值是配表审计事实而非模块常量，后续平衡只更新工作簿并重新发布 CSV；空间判定和最终扣血边界不变。

- `ReEcho.Enemies.Logic.LegacyDefinitions`：四类怪物现有数值等价。
- `ReEcho.Enemies.Logic.ContactCadence`：移动与同帧攻击意图可共存，冷却按现有语义推进。
- `ReEcho.Enemies.Logic.InvulnerableTargetConsumesAttack`：无敌只阻止伤害候选，不回滚动作与 cooldown。
- `ReEcho.Enemies.Logic.BomberFuse`：Fuse 不可取消、范围语义正确且只自毁一次。
- `ReEcho.Enemies.Logic.HurtAndSnapshot`：击退、快照恢复与死亡门控。
- `ReEcho.Enemies.Logic.Roster`：去重注册、稳定顺序、无复制存活查询与清理。
- `ReEcho.Enemies.Logic.RangedAndEliteBehaviors`：兔子锁点可躲避；狐狸在 0.15 秒 Active 内分步积分 650 cm、复用一次 AttackIdentity，并安全保存/恢复一次接触门。
- `ReEcho.Enemies.Logic.RangedAbilityRotation`：普通远程多能力按 SequenceOrder 循环、活动 AbilityId 跨快照绑定，以及两种施法移动门。
- `ReEcho.Enemies.Host.StunRetarget`：首次眩晕只取消一次旧锁定动作且保留冷却；解除后的第一步朝向当前目标，冷却结束后新动作锁定当前目标。
- `ReEcho.Enemies.Host.SheepStunImmunity`：`M_SHEEP` 同时拒绝 Combat 状态眩晕与 Host 卡牌眩晕，且不记录 `Z_Vertigo`。
- 命令：`scripts/ue/Build-Editor.cmd -Configuration Development`；`scripts/ue/Run-Automation.cmd -Filter ReEcho.Enemies.Logic`。
- `scripts/validate_project.py` 固定模块依赖和 include 边界，并拒绝 World 扫描、隐式兄弟组件发现、直接伤害调用及 Content 资源路径。
- `ReEcho.Enemies.Host.CompositionAndSave`、`ReEcho.Enemies.Host.RabbitProjectilePipeline`、`ReEcho.Run.SaveSnapshot` 与 Combat ElementReaction World 测试覆盖 Host/Combat/Roster/Save 接缝；Rabbit 测试额外锁定三球方向、四球跨帧连发及恢复、逐球事件身份、单球表驱动半径、路径内/外、命中玩家立即结束、长剑弧内结束事件和当前表值下单球实际扣血 `1`。
- Plan47 跨域回归覆盖 10/20 秒眩晕、Echo 400cm 减速/元素光环、0.5 秒敌方元素免疫上限和嘲讽目标；规则与持久状态仍由 Cards/Run 测试负责。
- 怪物攻击、受击、爆破、Boss、动画和遭遇结束仍由用户 PIE 验收。

## 不变量与常见错误

- Enemies 拥有行为，Combat 拥有伤害/生命/元素，Host 拥有世界 Transform，Presentation 拥有可见反馈；任何一方不得复制另一方的可写真相。
- 卡牌眩晕与减速是 Host 当步输入；首次进入眩晕时，Host 调用 `CancelActiveActionsForStun` 取消尚未结束的普通特殊技或 Boss 技能并清空旧锁点，但不得改写普通攻击冷却、Bomber Fuse、受击状态或为 Cards 增加 Enemies 反向依赖。眩晕期间 Host 停止推进 EnemyLogic，并把同一只读眩晕事实交给 Presentation 暂停当前动画帧；解除眩晕的第一帧必须以当前玩家或存活嘲讽 Echo 重新构造 Sense 并立即决策，不得恢复旧目标锁定动作。
- EnemyHost 显式声明 `EnemySide`，攻击身份在提交时快照该阵营；敌人不得在自身 Logic 中复制玩家/回响类型判断。Bomber 自毁通过单次 HitIntent 的 `bAllowSameFactionDamage` 明确放行自身伤害，不能为此全局开启敌人互伤。
- Host 必须显式注入 Sense 和事件组件；组件内部 `FindComponentByClass` 会重新引入隐式装配和悬空 Actor 风险。
- 同一步可以同时产生移动和攻击；提交攻击不得用新空对象覆盖已计算的移动/朝向意图。
- Bomber Fuse 到期只提交一次；等待 Host/Combat 销毁的间隙不能再次爆炸。
- 事件表示已发生结果，订阅回调没有玩法否决权；表现缺失、资源加载失败和动画结束都不能改变行为。
- Snapshot 是只读副本，不是命令；外部不得修改副本后假定 Logic 状态已变化。
- `FReEchoEnemySenseSnapshot::bPhase2TransitionPermitted` 是 Host 注入的单步许可：为 false 时 Logic 仅跳过新 Phase2 Transform 的启动判定，不停止普通 AI、移动、攻击、碰撞或受伤；许可恢复后的首个合格逻辑步重新评估攻击次数、距离与血量条件，已开始的 Transform 不受影响。
- `FReEchoEnemySenseSnapshot::bMovementPermitted` 是 Host 注入的单步许可：为 false 时 Logic 仍推进普通 AI、目标与冷却，但输出的普通移动、特殊冲刺和 Boss 传送请求均被清零；恢复后的首个逻辑步重新允许移动。
- `FReEchoEnemySenseSnapshot::bAttackPermitted` 是 Host 注入的单步许可：为 false 时 Logic 不开始或提交普通攻击、特殊技和 Boss 攻击窗口，但继续采样目标并推进既有冷却、引信、Boss encounter 等计时；已到提交边界的动作保持待提交，许可恢复后的首个合格逻辑步提交。Host 同时在世界副作用边界拒绝 Gate 内的攻击窗口、传送、投射物和伤害。
- Encounter 局间重置只允许清理具名瞬时字段；不得重建 Logic、重置普通攻击冷却/序号或用快照重生 Host 冒充同 Stage 连续性。
- 不引入 `ReEchoEnemies -> ReEchoWeapons`：敌人攻击节拍由 EnemyLogic 拥有，玩家武器节拍由 Weapons 拥有。
