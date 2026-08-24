# `MOD-ReEchoCombat` 详细设计

## 模块状态

- Runtime Module：`ReEchoCombat`。
- 代码根：`Source/ReEchoCombat/`。
- 架构标识：`MOD-ReEchoCombat`；功能检索标识：`AREA-AbilityCombat`。
- 当前状态：Plan41 已关闭并进入 `main`；Plan43 候选新增 `ReEchoEnemies` 作为只依赖 Combat 公共契约的怪物逻辑消费者。

## 存在原因

战斗曾分散在 Pawn、GAS、WeaponActor、Projectile、Enemy 和元素辅助函数中。一次攻击是否成立、伤害如何结算、谁发布命中/死亡事件，需要多个调用者共同遵守隐式约定；任何表现或载体代码漏掉一个判断都可能形成第二套规则。

本模块把“攻击请求与最终战斗裁决”收敛为独立逻辑边界：没有 UI、动画、VFX、音频和武器可见 Actor 时，生命、元素、命中和死亡仍能确定性运行与测试。它不是所有战斗相关 Actor 的容器，而是战斗真相和公共契约的所有者。

## 职责与排除项

**负责：**

- 自动/手动攻击请求的 held 状态、模式门控、统一释放和目标筛选；
- GAS Gameplay Tags、属性集、初始化/伤害/治疗 Effect 与基础/主动 Ability；
- Combatant 的生命、属性、元素状态和存活状态；
- `FReEchoAttackIdentity`、`FReEchoHitIntent`、`FReEchoHitResolved` 等跨模块值契约；
- `IReEchoCombatTarget` 的可选来源规则、反应/击杀/被击败通知接缝，以及 `bSourceRulesApplied` 单次变换门；
- HitResolver 对物理伤害、格挡、元素、生命、击杀与死亡的最终裁决；
- CombatEvents 和只读 Combatant/Attack Snapshot；
- 正式 `ElementCleanseCommand`：原子清除元素附着/灼烧并授予指定时长元素免疫，行为调用方不直接改 Combat 私有状态；
- 不依赖表现资源的纯规则与 World/GAS 自动化。

**不负责：**

- 武器 CSV/XLSX 读取、武器 Definition 编译、步骤游标、普通攻击间隔或逻辑投射物；
- Pawn 移动/相机/输入键位绑定，Enemy AI，Echo/Recording，Run/Save；
- Sprite、Mesh、动画、VFX、伤害数字、Widget、音频资产和播放；
- 商店、抽卡或局外流程。

## 权威状态

| 状态 | 唯一所有者 | 外部访问方式 |
|---|---|---|
| 生命、最大生命、战斗属性、元素附着/状态、存活 | `UReEchoCombatantComponent` + ASC/AttributeSet | `GetSnapshot()`、CombatEvents、受控 Effect/Resolver |
| 自动/手动模式、held 请求、当前目标 | `UReEchoAttackControllerComponent` / `UReEchoTargetingComponent` | 命令入口与 `FReEchoAttackSnapshot` |
| 元素规则集 | `ReEchoElementRuntime` 发布的不可变 `FReEchoElementRuleSet` | 主模块一次编译发布；Combat 只读 |
| 一次攻击的关联身份 | `FReEchoAttackIdentity` 值对象 | 全链按值传递，不由调用者拆分比较 |
| 阵营与可伤害关系 | `EReEchoCombatFaction`、`IReEchoCombatAffiliation`、`ReEchoCombatRelations` | Source 提交时快照阵营；候选层与 Resolver 共用同一判定 |
| 最终伤害、格挡、命中、击杀、死亡结果 | `ReEchoHitResolver` | `FReEchoHitResolved` 与 `UReEchoCombatEventsComponent` |

Weapon、Projectile、Enemy、UI 或表现适配器不得复制这些状态为可写真相。

`UReEchoCombatAttributeSet` 是 GAS 层的属性真相，保存 `Health`、`MaxHealth`、`Block`、攻击力等可被 GameplayEffect 修改的属性；`UReEchoCombatantComponent` 是 Combat 对外门面，负责绑定 ASC、同步只读快照、提供 `ApplyFinalDamage`/`ApplyHealing` 入口并广播生命/死亡/元素事件。有 ASC 时以 AttributeSet 为准，Combatant 不应成为第二套可写属性源。Development 的 `SetDebugInvulnerable` 仅在最终伤害入口返回零，不改写 ASC 属性、不消费格挡，并在 Shipping 固定关闭。

`SetAdditiveAttackModifier(SourceId, Physical, Elemental)` 是通用、按来源替换的临时攻击修正入口：同一来源的新值覆盖旧值而非累加历史差值，最终写回 AttributeSet/兼容 StatBlock。Combat 不读取 CharacterId、能力表或缺血阈值；当前勇者能力由主模块 Player Host 在最终 `HealthChanged` 后计算，再发送这一窄命令。

## 输入、输出与公共契约

### 命令与输入

- Pawn/流程通过 `UReEchoAttackControllerComponent` 发送自动/手动模式、Begin/End manual、运行门控和统一 Release 请求；不直接写 GAS spec 的 `InputPressed`。
- `IReEchoAttackControllerHost` / `IReEchoAttackHost` 是主模块宿主与 Combat 的窄桥，Combat 不 include 具体 Pawn 或 WeaponActor。
- Weapons、敌人接触攻击或合法环境来源提交完整 `FReEchoHitIntent`；Intent 只描述候选，不宣称最终伤害或死亡。
- 来源宿主可在 Resolver 内通过 `ModifyOutgoingHit` 对候选执行一次类型化规则变换；元素内部伤害必须携带 `bSourceRulesApplied`，防止同一 Hit 重复扣资源或增伤。
- 主模块把已校验 CSV 编译为不可变 `FReEchoElementRuleSet` 后发布；Combat 不认识 CSV Row、工作簿或资源字段。

### 结果、事件与快照

- `FReEchoHitResolved` 是最终裁决结果；只有 Resolver 能决定实际伤害、格挡与死亡。对应的 `FReEchoDamageEvent::bFatal` 明确标记本次 Hurt 已把存活目标降至零血，表现消费者仍可显示伤害数字，但必须抑制普通受击动画与受击 VFX。
- `UReEchoCombatEventsComponent` 发布 AttackCommitted、Hit、Hurt、HealthChanged、ElementStateChanged、Kill、Death。
- `FReEchoCombatantSnapshot` 和 `FReEchoAttackSnapshot` 是调用瞬间的只读副本，不持久化，也不能被 UI 当成可写缓存。
- 事件 Payload 只包含稳定 ID、值、弱/受控对象句柄和世界信息，不携带 Widget、Sound、Animation、Texture 或 Material。

### 攻击身份

`FReEchoAttackIdentity` 集中封装来源弱引用与单调 CommitId。它解决三个问题：

1. 同一出手产生的 Commit、Projectile、Hit、Hurt、Kill、Death 能稳定关联；
2. 调用者不再各自组合 `SourceActor + CommitId`，避免遗漏或比较规则不一致；
3. 延迟载体命中前来源被销毁时，弱引用不会被解引用；已快照的目标伤害仍可结算，但跳过来源侧事件与组件查询。

身份相等使用对象索引/序列语义和 CommitId，不使用消息对象地址。消息和值对象可能被复制、序列化或跨帧保存，指针不能作为稳定业务身份。

### 阵营与友方伤害

玩家和回响显式实现 `IReEchoCombatAffiliation` 并返回 `PlayerSide`，EnemyHost 返回 `EnemySide`。武器或敌人提交攻击时把来源阵营快照进 `FReEchoAttackIdentity::SourceFaction`，所以延迟投射物在来源销毁后仍能稳定判断关系。`ReEchoCombatRelations::CanDamage` 是唯一关系规则：Weapons 在候选阶段调用它，避免友方碰撞提前消费攻击载体；HitResolver 在最终结算前再次调用，防止未来载体漏筛。未迁移的 `Unaligned` 来源保持兼容放行，但正式玩家、回响和敌人宿主必须显式声明阵营。

阵营与 `EReEchoDamageSource` 分工不同：阵营决定能否伤害，DamageSource 用于事件归因。回响武器必须发布 `Echo`，不能沿用 `Player`。同阵营伤害默认禁止；敌人自毁等真实例外只能在单次 `FReEchoHitIntent::bAllowSameFactionDamage` 上显式声明，禁止在 Resolver 外按 Actor 类型写特判。

## 依赖方向

```text
ReEchoWeapons ─────→ ReEchoCombat
ReEchoEnemies ─────→ ReEchoCombat
ReEchoCards ───────→ ReEchoCombat
ReEcho ────────────→ ReEchoCombat

ReEchoCombat ─/─→ ReEchoWeapons / ReEchoEnemies / ReEchoCards / ReEcho / ReEchoAudio / UI / Presentation
```

公共依赖仅为 UE Core/Engine、GameplayAbilities、GameplayTags、GameplayTasks。需要世界装配、数据、音频或表现时，由 `ReEcho` 主模块适配。

## 运行时流程

### 自动/手动普通攻击

```text
Pawn 输入或 Run 模式
  → AttackController 更新唯一 held/模式状态
  → Targeting 返回确定性合法目标
  → BasicAttack Ability 请求宿主尝试攻击
  → ReEchoWeapons 判断唯一 readiness 并返回 Commit/剩余等待
  → 成功 Commit 发布 AttackCommitted；临时未就绪则等待，不结束 held 循环
```

Combat 管理“是否持续请求”，Weapons 管理“何时可提交”。两者不能各持一个攻击频率计时器。

Plan68 为血条清空型 Boss 提供一个可选、默认未绑定的致命伤拦截委托。Combatant 仍是生命与死亡唯一权威：只有宿主显式绑定且 EnemyLogic 接受 Phase2 过渡时，第一次致命伤才暂缓死亡并把 GAS/兼容生命同步夹到可存活值；宿主随后在变身完成时通过 `InitializeFromStats` 把 Phase2 最大生命和当前生命统一刷新。普通敌人和已经进入 Phase2 的 Boss 不绑定或拒绝该入口，继续走既有死亡路径。该委托不能承载 Boss 规则、表现或世界查询。

### 命中与结算

```text
Weapons/接触攻击产生 HitIntent
  → ReEchoCombatRelations 最终校验来源/目标阵营
  → ReEchoHitResolver::ResolveHit
  → 来源接口执行一次 outgoing rule（并设置 guard）
  → Target 接口修正原始输入（如格挡）
  → 物理/元素规则更新 Combatant 唯一状态
  → 形成 HitResolved
  → 发布 Hit/Hurt（致命时携带 bFatal）/HealthChanged/ElementStateChanged/Kill/Death
  → 来源接口接收已裁决的 Reaction/Kill 通知，目标接口接收 Defeated 通知
  → 主模块表现、UI、音频只读消费
```

一次 Intent 只能结算一次。Projectile、WeaponActor 与 Enemy 不得在 Resolver 外再次扣血或重算元素。

## 带读：一次战斗怎么发生

本节是对上文架构契约的“跑通调用链”补充。它描述跨模块运行路径，但不改变“职责与排除项”：`PlayerPawn`、`WeaponActor`、`Projectile`、`Enemy`、UI、音频仍是宿主/适配器/消费者，不是 `ReEchoCombat` 的规则权威。

### 0. 背景知识与对照关系

#### 0.1 GAS 在 Combat 中的最简原理

```text
ASC = GAS 管家
├─ Ability：能做什么
│   ├─ BasicAttack
│   └─ ActiveAttack
├─ AttributeSet：当前属性是多少
│   ├─ Health
│   ├─ Attack
│   └─ Speed
├─ GameplayEffect：怎么改属性
│   ├─ Damage
│   ├─ Heal
│   └─ Cooldown
└─ GameplayTag：当前状态/输入标记
    ├─ Input.Attack.Basic
    ├─ Cooldown.Attack.Active
    └─ State.Dead
```

在本模块中，GAS 负责能力激活、属性存储、属性修改与标签状态；Combat 负责命中是否成立、最终伤害、元素/死亡事件。

GAS 在本模块中主要做三件事：

| GAS 职责 | 对应原理 | 本项目例子 |
|---|---|---|
| 存战斗属性 | `AttributeSet` 是 GAS 属性表 | `UReEchoCombatAttributeSet` 保存 `Health`、`MaxHealth`、`Block`、攻击力等。 |
| 改战斗属性 | `GameplayEffect` 是属性修改请求 | 伤害/治疗先写入 `IncomingDamage` / `IncomingHealing`，再由 `PostGameplayEffectExecute` 扣血或回血。 |
| 管攻击能力 | `GameplayAbility` 是可激活的攻击/技能逻辑 | 输入激活 `UReEchoBasicAttackAbility` / `UReEchoActiveAttackAbility`，Ability 再调用宿主接口出手。 |

#### 0.2 先建立对照关系

阅读下面流程时，把每一步对照回本文档这些位置：

| 带读问题 | 对照本文档位置 | 关键判断 |
|---|---|---|
| 这一步是不是 Combat 模块职责？ | “职责与排除项” | 只要涉及武器 CSV、可见 Actor、UI、音频、Enemy AI，就属于外部适配或消费者。 |
| 哪个状态是唯一真相？ | “权威状态” | 生命/元素看 `Combatant + ASC`；held/目标看 `AttackController/Targeting`；最终伤害看 `HitResolver`。 |
| 外部应该如何调用 Combat？ | “输入、输出与公共契约” | 上游提交命令或 `HitIntent`，下游只读事件或 Snapshot。 |
| 结算顺序是否正确？ | “运行时流程 / 命中与结算” | 一次 Intent 必须只进一次 Resolver，不能在外部重复扣血。 |

### 1. 玩家被装配成 Combat 宿主

入口：`Source/ReEcho/Private/Player/ReEchoPlayerPawn.cpp`。

`AReEchoPlayerPawn` 创建并持有：

- `UAbilitySystemComponent` / `UReEchoCombatAttributeSet`：GAS 权威属性；对应本文档“权威状态 / 生命、最大生命、战斗属性”。
- `UReEchoCombatantComponent`：生命、属性、元素状态门面；对应“生命与元素状态”。
- `UReEchoAttackControllerComponent` / `UReEchoTargetingComponent`：自动/手动 held 与目标；对应“自动/手动模式、held 请求、当前目标”。
- `UReEchoCombatEventsComponent`：事件出口；对应“结果、事件与快照”。
- `AReEchoWeaponActor`：主模块武器适配器，不属于 Combat 规则权威；对应“职责与排除项 / 不负责武器”。

阅读顺序：先看 `AReEchoPlayerPawn` 的构造函数和 `BeginPlay()`，只确认“宿主如何把 ASC、Combatant、Weapon、Ability 接起来”，不要在这里寻找最终伤害规则。

### 2. 输入或自动攻击只产生“持续攻击请求”

| 类型 | 入口 | 谁主动触发 | 触发时机 |
|---|---|---|---|
| 手动攻击 | `SetupPlayerInputComponent` 绑定的输入事件 | 玩家 | 按下 / 松开瞬间 |
| 自动攻击 | `PlayerPawn.Tick` | 系统 | 每帧检查目标 |

手动输入路径：

```text
SetupPlayerInputComponent
  → ManualBasicAttack / ManualStopBasicAttack
  → AttackController.BeginManualAttack / EndManualAttack
  → Host.PressBasicAttackInput / ReleaseBasicAttackInput
```

这里先停在 `Host.PressBasicAttackInput` / `ReleaseBasicAttackInput`：它们只表示“开始/停止持续攻击请求”，进入 GAS Ability 的部分放到下一节。

自动攻击路径：

```text
PlayerPawn.Tick
  → AttackController.UpdateAutomaticAttack
  → Targeting.FindNearestTarget
  → Host.FaceAutomaticTarget
  → Host.PressBasicAttackInput
```

对应本文档“运行时流程 / 自动/手动普通攻击”：Combat 管理“是否持续请求”，Weapons 管理“何时可提交”。这里常见错误是让自动攻击绕过 GAS 或单独维护一套攻击频率。

### 3. GAS Ability 把输入转成宿主攻击尝试

入口：`Source/ReEchoCombat/Private/AbilitySystem/ReEchoPlayerAbilities.cpp`。

普通攻击 Ability 的核心职责：

```text
UReEchoBasicAttackAbility.ActivateAbility
  → WaitInputRelease
  → AttemptOrWait
  → IReEchoAttackHost.TryCommitBasicAttack
  → 若 Waiting，则按 Host.GetBasicAttackWaitRemaining 继续等待
```

主动攻击 Ability 的核心职责：

```text
UReEchoActiveAttackAbility
  → IReEchoAttackHost.ExecuteActiveAttack
  → 使用 Cooldown GameplayEffect
```

对应本文档“输入、输出与公共契约 / 命令与输入”：Combat 通过 `IReEchoAttackHost` 这个窄接口调用宿主，不 include 具体 Pawn 或 WeaponActor。

### 4. Pawn 宿主把攻击尝试交给 Weapon

入口：`AReEchoPlayerPawn::TryCommitBasicAttack()`、`ExecuteBasicAttackAbility()`、`ExecuteActiveAttackAbility()`。

```text
IReEchoAttackHost.TryCommitBasicAttack
  → 检查 Weapon / Combatant
  → 查询 Weapon.GetAttackCooldownRemaining
  → Weapon.TryBasicAttack(Combatant)
```

这里 Pawn 只是宿主适配层。对应本文档“职责与排除项”：Pawn 不拥有攻击间隔、不拥有伤害公式、不拥有命中裁决。

### 5. Weapons 生成 Commit 与攻击载体

入口：

- `Source/ReEchoWeapons/Private/Weapons/ReEchoWeaponLogic.cpp`
- `Source/ReEcho/Private/Weapons/ReEchoWeaponActor.cpp`

`FReEchoWeaponLogic` 负责：

```text
TryCommitBasicAttack / TryCommitActiveAttack
  → 检查 readiness
  → 生成 FReEchoAttackIdentity
  → 选择 AttackStep
  → 计算 RawDamage / Element / Carrier / Range
  → 输出 FReEchoWeaponAttackCommit
```

`AReEchoWeaponActor` 负责把 Commit 转成世界表现载体：

```text
Commit.Carrier == Melee      → SwingMelee
Commit.Carrier == Projectile → FireProjectile
Commit.Carrier == Wave       → FireStaffLightWave
```

对应本文档“职责与排除项 / 不负责武器”和“扩展方式 / 新攻击载体不放在 Combat”：武器可以决定“这次攻击是什么载体、候选伤害是多少”，但不能决定最终扣血和死亡。

### 6. 载体命中后提交 HitIntent

近战路径：`AReEchoWeaponActor::ApplyDamageToTarget()` 直接组装 `FReEchoHitIntent`。

投射物/光波路径：

```text
AReEchoProjectileActor / AReEchoStaffLightWaveActor
  → UReEchoProjectileLogicComponent.InitializeProjectile
  → Advance
  → 遍历 IReEchoCombatTarget
  → IntersectsCombatPath
  → ResolveIntent
  → ReEchoHitResolver.ResolveHit
```

对应本文档“输入、输出与公共契约 / 命令与输入”：Weapons、敌人接触攻击或合法环境来源提交完整 `FReEchoHitIntent`。Intent 只表示“候选命中”，不宣称最终伤害。

### 7. HitResolver 做唯一最终裁决

入口：`Source/ReEchoCombat/Private/Combat/ReEchoHitResolver.cpp` 与 `ReEchoElementHitResolver.cpp`。

```text
ReEchoHitResolver.ResolveHit
  → Element == None：ResolvePhysicalHit
  → Element != None：ResolveElementHit
  → Target.ModifyIncomingRawDamage
  → Combatant.ApplyFinalDamage
  → 形成 FReEchoHitResolved
  → 发布 Hit / Hurt / Kill / Death / ElementStateChanged
```

对应本文档“权威状态 / 最终伤害、格挡、命中、击杀、死亡结果”：只有 Resolver 能决定 `AppliedDamage`、`bBlocked`、`bKilled`。

如果排查“打中了但没掉血”，优先按这个顺序看：

1. `HitIntent.Target` 是否实现 `IReEchoCombatTarget`；
2. `Target->IsCombatTargetAlive()` 是否为 true；
3. `Target->ModifyIncomingRawDamage()` 是否把伤害改成 0，例如盾兵；
4. `Combatant.ApplyFinalDamage()` 是否被 GAS 的 Block 消耗；
5. `UReEchoCombatAttributeSet::PostGameplayEffectExecute()` 是否真正改了 `Health`。

### 8. Combatant/ASC 改写生命与元素状态

入口：

- `UReEchoCombatantComponent::ApplyFinalDamage()`
- `ReEchoGameplayEffects::ApplyDamage()`
- `UReEchoCombatAttributeSet::PostGameplayEffectExecute()`

```text
Combatant.ApplyFinalDamage
  → ReEchoGameplayEffects.ApplyDamage
  → GameplayEffect 写 IncomingDamage
  → AttributeSet.PostGameplayEffectExecute
  → Block 优先消耗，否则扣 Health
  → Combatant.HandleHealthChanged
  → OnHealthChanged / OnDeath / CombatEvents.HealthChanged
```

对应本文档“权威状态 / 生命、最大生命、战斗属性、元素附着/状态、存活”：外部不要直接写 `CurrentHealth` 或自己广播死亡。

### 9. Enemy 既是目标，也是攻击者

入口：

- Host：`Source/ReEcho/Private/Graybox/ReEchoEnemyActor.cpp`
- 逻辑：`Source/ReEchoEnemies/Private/Enemies/ReEchoEnemyLogicComponent.cpp`
- 表现：`Source/ReEcho/Private/Presentation/Enemy/ReEchoEnemyPresentationComponent.cpp`

作为目标：

- `AReEchoEnemyActor` 实现 `IReEchoCombatTarget`，返回自己的 `Combatant`；
- Host 通过 `ModifyIncomingRawDamage()` 保留盾兵正面格挡/背刺加伤；
- Combat 继续独占生命、元素、伤害和死亡裁决；
- `EnemyLogic` 订阅 Hurt/Death 更新受击、击退和存活状态，`EnemyPresentation` 独立订阅同一结果驱动可见反馈，二者互不调用。

作为攻击者：

```text
EnemyActor.Tick
  → Host 构造 FReEchoEnemySenseSnapshot
  → EnemyLogic.Advance
  → FReEchoEnemyActionIntent
  → Host 应用朝向/扫掠移动
  → Host 把 AttackIntent 翻译成 FReEchoHitIntent
  → ReEchoHitResolver.ResolvePhysicalHit
```

距离、冷却、Bomber 引信和一次性自毁由 `ReEchoEnemies` 决定；Host 只采样世界、应用 Intent 并适配 Combat。对应本文档“扩展方式 / 新目标类型”：新目标应该实现 `IReEchoCombatTarget`，而不是让 Resolver include 具体 Actor；新的 Enemy 行为也不应重新塞回 Host。

### 10. UI、音频、表现只读消费结果

UI 常见路径：

```text
Combatant.OnHealthChanged
  → PlayerHudWidget / HealthBarWidget
```

音频路径：

```text
UReEchoCombatAudioAdapterComponent
  → 订阅 CombatEvents.OnAttackCommitted / OnHit / OnHurt / OnKill / OnDeath
  → 转成 ReEchoAudio 事件
```

敌人表现路径：

```text
EnemyEvents.Action/Fuse + CombatEvents.Hurt/Death/ElementStateChanged
  → UReEchoEnemyPresentationComponent
  → 动画、伤害数字、受击、死亡、引信和元素附着显示
```

关闭碰撞与 `LifeSpan` 属于 Host 的世界生命周期收尾，不由表现组件决定。

对应本文档“结果、事件与快照”：事件描述已发生结果，回调不能反向更改本次结果。

### 11. 一次完整普攻的最短调用图

```text
Input / Auto held
  → AttackController
  → AbilityInputPressed(Input.Attack.Basic)
  → UReEchoBasicAttackAbility.AttemptOrWait
  → IReEchoAttackHost.TryCommitBasicAttack
  → AReEchoPlayerPawn.ExecuteBasicAttackAbility
  → AReEchoWeaponActor.TryBasicAttack
  → FReEchoWeaponLogic.TryCommitBasicAttack
  → FReEchoWeaponAttackCommit
  → WeaponActor.ExecuteAttack
  → Melee / Projectile / Wave
  → FReEchoHitIntent
  → ReEchoHitResolver.ResolveHit
  → Target.ModifyIncomingRawDamage
  → UReEchoCombatantComponent.ApplyFinalDamage
  → ReEchoGameplayEffects.ApplyDamage
  → UReEchoCombatAttributeSet.PostGameplayEffectExecute
  → FReEchoHitResolved
  → CombatEvents + OnHealthChanged
  → UI / Audio / VFX 只读消费
```

用这张图定位问题时，始终回到本文档“不变量与常见错误”：不要在 Resolver 外创建第二套生命、元素、held、目标或伤害算法。

## 代码位置与阅读路线

| 目的 | 先读代码 | 说明 |
|---|---|---|
| 公共值与身份 | `Public/Combat/ReEchoCombatTypes.h` | AttackIdentity、HitIntent/Resolved、StatBlock、ElementState |
| 事件与快照 | `Public/Combat/ReEchoCombatContracts.h` | 类型化事件、Combatant/Attack Snapshot、EventsComponent |
| 生命与元素状态 | `Public/Combat/ReEchoCombatantComponent.h` → `Private/Combat/ReEchoCombatantComponent.cpp` | 状态唯一写入口和 ASC 同步 |
| 最终命中结算 | `Public/Combat/ReEchoHitResolver.h` → `Private/Combat/ReEchoHitResolver.cpp` | 物理/元素/死亡裁决中心 |
| 元素规则 | `ReEchoElementRuntime.*`、`ReEchoElementHitResolver.cpp`、`ReEchoElementRules.*` | 不可变 RuleSet 与 Combatant 状态修改 |
| 攻击请求 | `ReEchoAttackControllerComponent.*`、`ReEchoCombatTarget.*` | held、模式、目标与宿主窄接口 |
| GAS | `Public/AbilitySystem/` → `Private/AbilitySystem/` | Ability、Tags、Effects、AttributeSet |
| 模块回归 | `Private/Tests/ReEchoCombatRuntimeTests.cpp` | 身份生命周期、元素、Resolver、契约 |
| 主模块接线 | `Source/ReEcho/Private/Player/ReEchoPlayerPawn.cpp`、`Source/ReEcho/Private/Graybox/ReEchoEnemyActor.cpp` | 只负责宿主/世界适配，不是规则权威 |
| 怪物逻辑消费 | `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyLogicComponent.h` | 显式订阅 CombatEvents 处理 Hurt/Death；不复制生命、元素或伤害结算 |

## 扩展方式

### 武器符文状态宿主

Plan76 的晕眩、流血、短暂无敌和临时攻速/移速均通过 `UReEchoCombatantComponent` 的窄命令进入 Combat。`Z_Vertigo` 维护动作禁止边界；`Z_Bleeding` 每层每秒结算最大生命 0.5%，每层独立保存到期时间；临时属性层通过独立 GAS Effect Handle 应用和移除，非 GAS 兼容路径保持相同乘数语义。Weapons/主模块只能提交命令并读取查询，不能直接改生命、状态标签或最终属性。

- 新伤害类型：先扩 Intent/Resolved 的稳定枚举和值字段，再只在 Resolver 增加裁决分支和 focused tests。
- 新元素/状态：由主模块 Data Adapter 编译 RuleSet；Combat 扩纯规则与 Combatant 状态，不读取 CSV。
- 新 Combat 消费者：优先订阅事件或读取 Snapshot；若需要改变状态，新增窄 Command，不公开可写组件字段。
- 新目标类型：实现 `IReEchoCombatTarget`，不要让 Targeting/Resolver include 具体 Actor 类。
- 新攻击载体不放在 Combat；载体属于 Weapons，只向 Combat 提交 Intent。
- 新表现反馈不放在 Combat；在主模块建立事件适配器并映射资源。

## 验证与测试

- 纯规则：身份、伤害、元素、RuleSet 与 Snapshot 的无世界测试。
- World/GAS：Combatant、ASC、AttackController、Targeting、Ability held 重试。
- 生命周期：来源销毁后延迟命中仍安全结算，且不发布来源侧反馈。
- 跨模块：Weapons 只产生 Intent；Combat 才改变生命并发布最终事件。
- 卡牌接缝：来源规则只执行一次；元素二次分派不重复变换；Reaction/Kill/Defeated 仅在权威结果后通知。
- 命令：`scripts/ue/Run-Automation.cmd -Filter ReEcho.Combat`，并回归 `ReEcho.AttackMode`、Element、Recording/Save。
- 用户 PIE：自动/手动切换、连续攻击、无目标、菜单/死亡释放、命中/击杀和手感。

## 不变量与常见错误

- Combat 是最终结算权威；Weapons、Actor、Widget 与表现不得直接扣血或决定死亡。
- `FReEchoAttackIdentity::Source` 必须保持弱引用；延迟逻辑不得凭非空裸地址判断 Actor 存活。
- Combat 不依赖 Weapons、主模块、Audio、UI 或任何表现资源。
- Combat 不认识具体卡牌；它只提供可选宿主钩子和 guard，`ReEchoCards`/主模块负责规则语义与状态。
- 自动/手动共享 AttackController/GAS/Weapons 路径；自动攻击不进入 Recording。
- 事件描述已发生结果，回调不能反向更改本次结果；Snapshot 也不是命令。
- 不为兼容旧调用保留第二套生命、元素、held、目标或伤害算法。
# Plan73 元素反应表现契约

`UReEchoCombatEventsComponent::OnElementReactionResolved` 只在有效反应完整结算后发布一次资源中立结果，携带 ReactionId、ReactionBehaviorId、RadiusCm、反应前/进入/结算后元素以及玩法确定的受影响目标顺序；表现消费者不得重新计算半径或连锁拓扑。
