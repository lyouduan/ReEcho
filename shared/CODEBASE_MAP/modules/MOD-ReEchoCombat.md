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
- HitResolver 对物理伤害、格挡、元素、生命、击杀与死亡的最终裁决；
- CombatEvents 和只读 Combatant/Attack Snapshot；
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
| 最终伤害、格挡、命中、击杀、死亡结果 | `ReEchoHitResolver` | `FReEchoHitResolved` 与 `UReEchoCombatEventsComponent` |

Weapon、Projectile、Enemy、UI 或表现适配器不得复制这些状态为可写真相。

## 输入、输出与公共契约

### 命令与输入

- Pawn/流程通过 `UReEchoAttackControllerComponent` 发送自动/手动模式、Begin/End manual、运行门控和统一 Release 请求；不直接写 GAS spec 的 `InputPressed`。
- `IReEchoAttackControllerHost` / `IReEchoAttackHost` 是主模块宿主与 Combat 的窄桥，Combat 不 include 具体 Pawn 或 WeaponActor。
- Weapons、敌人接触攻击或合法环境来源提交完整 `FReEchoHitIntent`；Intent 只描述候选，不宣称最终伤害或死亡。
- 主模块把已校验 CSV 编译为不可变 `FReEchoElementRuleSet` 后发布；Combat 不认识 CSV Row、工作簿或资源字段。

### 结果、事件与快照

- `FReEchoHitResolved` 是最终裁决结果；只有 Resolver 能决定实际伤害、格挡与死亡。
- `UReEchoCombatEventsComponent` 发布 AttackCommitted、Hit、Hurt、HealthChanged、ElementStateChanged、Kill、Death。
- `FReEchoCombatantSnapshot` 和 `FReEchoAttackSnapshot` 是调用瞬间的只读副本，不持久化，也不能被 UI 当成可写缓存。
- 事件 Payload 只包含稳定 ID、值、弱/受控对象句柄和世界信息，不携带 Widget、Sound、Animation、Texture 或 Material。

### 攻击身份

`FReEchoAttackIdentity` 集中封装来源弱引用与单调 CommitId。它解决三个问题：

1. 同一出手产生的 Commit、Projectile、Hit、Hurt、Kill、Death 能稳定关联；
2. 调用者不再各自组合 `SourceActor + CommitId`，避免遗漏或比较规则不一致；
3. 延迟载体命中前来源被销毁时，弱引用不会被解引用；已快照的目标伤害仍可结算，但跳过来源侧事件与组件查询。

身份相等使用对象索引/序列语义和 CommitId，不使用消息对象地址。消息和值对象可能被复制、序列化或跨帧保存，指针不能作为稳定业务身份。

## 依赖方向

```text
ReEchoWeapons ─────→ ReEchoCombat
ReEchoEnemies ─────→ ReEchoCombat
ReEcho ────────────→ ReEchoCombat

ReEchoCombat ─/─→ ReEchoWeapons / ReEchoEnemies / ReEcho / ReEchoAudio / UI / Presentation
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

### 命中与结算

```text
Weapons/接触攻击产生 HitIntent
  → ReEchoHitResolver::ResolveHit
  → Target 接口修正原始输入（如格挡）
  → 物理/元素规则更新 Combatant 唯一状态
  → 形成 HitResolved
  → 发布 Hit/Hurt/HealthChanged/ElementStateChanged/Kill/Death
  → 主模块表现、UI、音频只读消费
```

一次 Intent 只能结算一次。Projectile、WeaponActor 与 Enemy 不得在 Resolver 外再次扣血或重算元素。

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
| 主模块接线 | `Source/ReEcho/Private/Player/ReEchoPlayerPawn.cpp`、`Source/ReEcho/Private/Graybox/ReEchoEnemy.cpp` | 只负责宿主/世界适配，不是规则权威 |
| 怪物逻辑消费 | `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyLogicComponent.h` | 显式订阅 CombatEvents 处理 Hurt/Death；不复制生命、元素或伤害结算 |

## 扩展方式

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
- 命令：`scripts/ue/Run-Automation.cmd -Filter ReEcho.Combat`，并回归 `ReEcho.AttackMode`、Element、Recording/Save。
- 用户 PIE：自动/手动切换、连续攻击、无目标、菜单/死亡释放、命中/击杀和手感。

## 不变量与常见错误

- Combat 是最终结算权威；Weapons、Actor、Widget 与表现不得直接扣血或决定死亡。
- `FReEchoAttackIdentity::Source` 必须保持弱引用；延迟逻辑不得凭非空裸地址判断 Actor 存活。
- Combat 不依赖 Weapons、主模块、Audio、UI 或任何表现资源。
- 自动/手动共享 AttackController/GAS/Weapons 路径；自动攻击不进入 Recording。
- 事件描述已发生结果，回调不能反向更改本次结果；Snapshot 也不是命令。
- 不为兼容旧调用保留第二套生命、元素、held、目标或伤害算法。
