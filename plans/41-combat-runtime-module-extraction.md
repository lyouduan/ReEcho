# Plan 41 - Combat - 独立模块与攻击系统重构

## 协调

- Planner 负责人：Gavyn-side Planner。
- Executor 负责人：Gavyn-side Planner（本对话 AI 直接执行）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。用户负责最终 PIE、攻速和手感验收。
- 本地规划 / 实现基线：实现最初基于 `origin/main@f499a1b`；2026-08-13 经用户审计选择后已合入 `origin/main@24b9f08` 的远端规则权威提交。后续发布前仍重新 fetch 并执行外部提交审计。
- 实现分支：本地 `plan/41-combat-runtime-module`，独立 worktree。
- 依赖 / 阻挡：Plan33 已关闭。Plan34 本地 WIP 保留但暂停占用 `ReEcho.Build.cs`/预构建包；Plan40 保持 Reserved 且不得在本 Plan 实施期间进入 Active。Plan41 合入后，两者必须在新模块边界上重新适配和验证。
- Writes：新增 `Source/ReEchoCombat/**` 与 `Source/ReEchoWeapons/**`；`ReEcho.uproject`；匹配的 Build.cs；必要的 `Config/DefaultEngine.ini` Core Redirects；迁移/适配 `Source/ReEcho/{Public,Private}/{AbilitySystem,Combat,Core,Player,Weapons,Graybox}/**`；攻击/战斗/武器/模块测试；校验脚本；本 Plan 执行记录；`shared/CODEBASE_MAP/{ARCHITECTURE.md,README.md,modules/MOD-ReEcho.md,modules/MOD-ReEchoCombat.md,modules/MOD-ReEchoWeapons.md}`；最终 `GIT_RULES.md` 允许的 Win64 Editor 预构建包。
- Stable Reads：`Data/**` 数据快照和生成契约、`Run/**` 攻击模式存档、`Recording/**` 录制语义、`ReEchoAudio` API、Plan28/38 攻击行为、Plan40 表现请求契约。
- 影响模式：战斗公共类型、反射路径、GAS、攻击组件和模块依赖为 `SharedContract`；模块描述符、Build.cs、Core Redirects 和最终预构建包为 `Exclusive`。
- 兼容承诺 / 下游操作：依赖必须为 `ReEcho -> ReEchoWeapons -> ReEchoCombat`，且 `ReEcho` 可直接消费两个逻辑模块；禁止 Combat 反向依赖 Weapons/主模块，禁止两个逻辑模块依赖 Audio/UI/Presentation。现有存档版本、Gameplay Tag/FName、伤害/元素结果、自动/手动模式、新局/Continue 规则和录制语义保持不变。普通攻击节奏按本 Plan 的唯一频率决定有意调整；UI、动画、特效和音频只消费事件/快照，不反向控制逻辑。
- 明确排除：不改战斗数值；不修改 XLSX/生成 CSV；不删除 `AttackIntervalSeconds` 兼容列；不重做敌人 AI、武器表现、动画、美术、UI、商店、抽卡、Echo 录制或音频播放；不创建 `ReEchoFrontend`/`ReEchoCore`；不网络化或插件化。

## 锁定目标

一步建立独立的 `ReEchoCombat` 与 `ReEchoWeapons` Runtime Module，并把当前分散在 Pawn、GAS、WeaponActor、投射物和敌人中的战斗重构成“逻辑模块产生权威结果，表现层只读消费”的单一管线：

```text
玩家输入 / 自动模式
→ AttackControllerComponent（是否持续请求攻击）
→ TargetingComponent（提供确定性合法目标）
→ GAS（能力状态与主动技能冷却）
→ ReEchoWeapons / WeaponRuntime（唯一普通攻击节拍、步骤和攻击载体逻辑）
→ HitIntent（武器只报告命中候选和攻击参数）
→ ReEchoCombat / HitResolver（伤害、元素、生命和死亡最终裁决）
→ CombatEvents + 只读 Snapshots（唯一对外观察表面）
→ 主模块适配 Animation / VFX / Audio / UI / Presentation
```

逻辑模块内部按职责选择载体：跟随 Actor 的状态用 Component/ASC，能力用 GAS，数值和步骤规则用普通 C++ 对象，跨模块使用窄接口、同步请求/结果和语义事件。不得把所有规则强行做成 ActorComponent，也不得因组件化复制状态真相源。删除全部贴图、动画、特效、UI和音频后，战斗逻辑仍必须可完整运行和自动化验证。

## 锁定产品与技术决定

### 架构影响与设计意图

- 受影响架构标识：直接修改 `MOD-ReEchoCombat` / `AREA-AbilityCombat`、`MOD-ReEchoWeapons` / `AREA-Weapons` 和主模块装配/表现边界 `MOD-ReEcho`；`MOD-ReEchoAudio` 是公共契约消费者关系的只读审阅对象。
- 对应模块文档：创建并维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`、`MOD-ReEchoWeapons.md`，维护 `MOD-ReEcho.md`；`MOD-ReEchoAudio.md` 已列入关闭审阅但不加入 Executor Writes，因为其服务、目录、策略和后端契约未修改。
- 设计意图：把攻击请求、武器执行和最终战斗裁决从 Pawn/Actor/表现回调中分离成无资源也可运行的逻辑模块；表现、UI 和音频只消费类型化事件与只读快照。
- 权威状态与依赖：Combat 独占 held/目标/生命/元素/最终结算，Weapons 独占普通攻击唯一 readiness/步骤/逻辑载体；依赖固定为 `ReEcho -> ReEchoWeapons -> ReEchoCombat`，主模块可直接装配两者，逻辑模块不反向依赖主流程/表现/音频。
- 相关文档同步范围：更新 `shared/CODEBASE_MAP/ARCHITECTURE.md` 的四模块拓扑、状态流和不变量；更新 `README.md` 的模块/AREA 路由；逐项更新上述三个模块文档并审阅 `MOD-ReEchoAudio.md`。

### 设计决策记录

- **2026-08-13，攻击身份集中化：** 不再要求调用者自行组合和比较 `SourceActor + CommitId`。`ReEchoCombat` 导出统一 `FReEchoAttackIdentity`，封装来源、序号、有效性和比较语义；武器、攻击载体、元素、命中和事件只传完整身份。
- **2026-08-13，攻击来源生命周期安全：** `FReEchoAttackIdentity` 中的来源必须是 `TWeakObjectPtr<AActor>`，不能让延迟投射物以非空裸地址判断来源存活。来源离开世界后，攻击序号和已快照的伤害数据仍可随载体完成目标结算，但跳过来源侧 Hit/Kill 事件与来源组件查询。
- **2026-08-13，战斗与武器分域：** `ReEchoCombat` 拥有攻击控制、目标契约、Combatant、伤害/元素/生命/死亡裁决和结果事件；`ReEchoWeapons` 拥有武器配置运行态、唯一攻击频率门、连段步骤、近战查询、逻辑投射物和法杖光波。Weapons 产生 `AttackCommitted`/`HitIntent`，Combat 返回 `HitResolved`，Weapons 不直接扣血或决定死亡。
- **2026-08-13，逻辑与表现单向分层：** 逻辑模块维护唯一真实状态并产生事件/快照；Animation、VFX、UI、Audio 和武器视觉只读消费，不得用动画结束、特效碰撞、资源加载或回调控制 readiness、命中和结算。连续位置等状态用只读 Snapshot，出手/命中/死亡等边沿变化用事件。
- **2026-08-13，策划数据编译边界：** XLSX/CSV 的读取、校验和显示字段保留在主 `ReEcho` 数据适配层；加载成功后，由适配层一次性编译并发布不含资源引用的不可变 Combat/Weapons Definition/RuleSet。逻辑模块只消费这些模块原生快照，不反向 include CSV Row、工作簿路径、颜色、VisualKey 或其他表现数据，避免数据加载器成为第二套运行时逻辑。
- **2026-08-13，元素状态归属：** 附着、免疫、强化、状态期限和燃烧运行态归 `CombatantComponent` 唯一持有，由 Combat HitResolver 修改；Enemy/存档只能读取快照或走明确恢复入口，Enemy 的光环、标签与伤害数字订阅 Combat 事件刷新。禁止 Enemy、Projectile、Weapon 或表现适配器再保存第二份可写元素状态。
- **执行中决策纪律：** 本 Plan 后续形成或改变的架构、权威数据、公共契约、玩法语义和关键取舍，先追加到本节并说明原因，再实施；不得只记录在对话、提示或 AI 摘要。

### 唯一普通攻击频率

- 自动和手动普通攻击的唯一权威数据为当前武器的 `weapons.AttackIntervalSeconds / AttackSpeed`。
- `WeaponRuntime` 独占 `NextAttackReadyTime`（或等价单一状态）以及当前步骤游标；其他对象只能查询剩余时间或提交请求。
- GAS 不再拥有基础攻击 Cooldown Tag 或第二个定时门；held ability 只等待 WeaponRuntime 根据 `AttackIntervalSeconds` 返回的剩余时间。
- `attack_steps.DurationSeconds` 只表示当前步骤的行为/表现持续时间，可继续驱动移动、无敌或后续动画时间线，但不得拒绝或延后下一次普通攻击提交。
- `StepLockRemaining` 不再作为攻击门；应删除、改名为非阻塞步骤行为时间，或收敛为只读表现状态。它不得与 `NextAttackReadyTime` 并列控制攻击频率。
- 现有 `part_effects` 对 `AttackIntervalSeconds` 的修改继续按原字段语义生效，并由同一个 WeaponRuntime readiness 计算消费，不得形成额外计时器。
- 该决定会改变现有两门配置中被 `DurationSeconds` 压慢的武器，例如长剑由当前实际约 `0.8s` 一击变为约 `0.28s` 一击；这是用户明确选择后的预期变化，必须由用户 PIE 验收，而不是在实现中暗中恢复 `max(Interval, Duration)`。
- 主动技能的独立技能冷却仍属于 GAS；不得借本 Plan 把主动技能冷却与普通攻击节拍混为一个状态。当前占位主动技能若没有独立配置，保持现有可执行结果并记录后续数据契约缺口。

### 权威状态

- `AttackControllerComponent` 独占攻击模式下的请求/held 状态，接收 Pawn 输入、菜单、死亡和恢复模式命令；它不计算攻击间隔、不结算伤害。
- `TargetingComponent` 独占自动目标筛选与确定性最近目标规则；目标必须实现 Combat Module 的最小 Target 接口，组件不得 include 具体 Enemy 类。
- `ReEchoWeapons` 的 `WeaponRuntime` 独占步骤游标、唯一节拍和攻击载体逻辑；它使用统一 `FReEchoAttackIdentity` 提交攻击，只产生 `HitIntent`，不得直接修改目标生命或决定死亡。
- `CombatantComponent`/ASC 独占生命、属性和死亡状态；UI 只读或订阅事件。
- `ReEchoCombat` 的 HitResolver 独占伤害、格挡、元素、生命和死亡最终裁决；同一 `HitIntent` 只结算一次。
- `CombatEvents` 只发布已发生的语义结果；Animation、VFX、Audio、UI、Presentation 不得通过回调改变结算结果。

### 事件、只读数据与命令契约

跨模块交互必须分成三种表面，不得用一个可写 Component 指针同时承担通知、查询和修改：

1. **`CombatEvents`：本次已经发生的结果。** 事件使用 `ReEchoCombat` 导出的只读 `USTRUCT` Payload；订阅者不得根据事件回调反向改变本次结算。
2. **`CombatSnapshots`：当前权威状态的只读副本。** UI 在收到事件后通过只读 provider 查询；Snapshot 由权威组件一次构造，不暴露内部容器、计时器或可写引用。
3. **`CombatCommands`：外部合法请求。** UI/流程只能调用窄命令入口；命令由 Combat/主模块适配层验证后改变状态，再发布事件。Widget 不得直接设置生命、属性、held、目标、步骤游标或 readiness。

#### 类型化事件 Payload

实现名称可按 UE 规范微调，但至少锁定以下语义和数据：

- `AttackCommitted`：统一 `FReEchoAttackIdentity`、可选 Target 句柄、`WeaponId`、`AttackPatternId`、`AttackStepId/StepIndex`、攻击起点/方向和世界位置。只在 WeaponRuntime 成功通过唯一频率门并推进步骤一次后发布。
- `HitIntent`：Weapons 产生的待裁决命中候选，包含完整 AttackIdentity、Target、武器伤害参数和命中位置/方向；不得包含最终实际伤害、死亡或表现资源。
- `HitResolved` / `Hit`：对应完整 AttackIdentity、Target、原始伤害、最终实际伤害、伤害来源、元素、`bCritical`、`bBlocked` 和命中世界位置。只有 Combat 可构造最终结果，未造成实际命中不得伪造 Hit。
- `Hurt`：目标视角的同一次伤害结果，必须复用同一 AttackIdentity/Damage Result，不能重新计算一遍伤害。
- `HealthChanged`：Combatant 句柄、变化前/后生命、最大生命和变化原因；UI 不需要读取内部 AttributeSet 才能刷新常用血条。
- `Kill` / `Death`：同一次终结结果的 AttackIdentity/Target、伤害来源和世界位置；没有攻击来源的环境/调试变化可使用无效身份，每次死亡只发布一次。

Payload 可以携带通用 Actor/Component 弱句柄、稳定运行时 ID、Gameplay Tag/FName、标量和世界坐标，但不得携带 Widget、音频、动画、纹理、材质或其他资源引用。订阅者需要资源时，使用自己的目录把语义 ID 映射为资源。

#### 只读 Snapshot / Provider

- `FReEchoCombatantSnapshot` 至少提供只读 Combatant 标识、`CurrentHealth`、`MaxHealth`、`FReEchoStatBlock`、元素/状态摘要和 `bAlive`。
- `FReEchoAttackSnapshot` 至少提供自动/手动模式、是否 held、当前只读 Target 句柄、当前 `WeaponId/AttackStepId`、下一次可攻击剩余时间和当前有效 `AttackSpeed`。
- `FReEchoProjectileSnapshot`（法杖光波若只是不同形状则复用）至少提供稳定 ProjectileId、AttackIdentity、逻辑位置/方向、归一化生命周期和有效状态；表现层不得拥有第二份飞行真相。
- `UReEchoCombatantComponent` 和 `UReEchoAttackControllerComponent` 分别提供一次性 `GetSnapshot()`；UI 不拼接多个裸字段形成自己的第二份状态。
- Snapshot 只表示调用瞬间，不被持久保存，也不进入 Recording；需要长期显示时由 UI 在事件后重新查询，不能把旧 Snapshot 当权威缓存。

#### 受控 Command

- 攻击模式选择通过 `SetAttackMode`（或等价类型化命令）进入，先由现有 Run/流程规则持久化，再同步给 `AttackControllerComponent`。
- 手动输入只发送 `BeginManualAttack` / `EndManualAttack`；自动循环只发送目标存在/失效和运行门控命令。两者不能直接操作 GAS spec 的内部 `InputPressed` 字段。
- 菜单、死亡、切换模式统一调用 `ReleaseAttackRequests`；不得让 UI 分别清理多份 held 标志。
- 治疗、伤害、调试和复活若由 UI/GM 发起，必须走现有 GameplayEffect/经过验证的 Combat Command；不得公开 `SetCurrentHealth` 等无约束写入口。

UI 可以同时消费事件和 Snapshot；音频通常只消费事件 Payload，由主模块把它翻译成 `ReEchoAudio` 语义请求；Animation/VFX/Presentation 消费 `AttackCommitted`、Projectile 事件/快照、`Hurt` 和 `Death`。技术依赖仍由 `ReEcho` 主模块适配：两个逻辑模块与 `ReEchoAudio` 不直接互相依赖。

## 模块边界

### 归属 `ReEchoCombat`

1. 最小战斗值类型：元素、伤害来源、`FReEchoStatBlock`、元素状态、统一 `FReEchoAttackIdentity` 及攻击请求/命中意图/结算结果类型；
2. Gameplay Tags、AttributeSet、GameplayEffects、基础/主动 GameplayAbility；
3. `UReEchoCombatantComponent`、`UReEchoAttackControllerComponent`、`UReEchoTargetingComponent` 与 Combat Target/HitResolution 接口；
4. 伤害、格挡、元素、生命、击杀和死亡的唯一 HitResolver/裁决入口；
5. `CombatEvents`、只读 Snapshot/provider 和受控 Command 公共契约及无设备/无世界单元测试。

Combat 不认识具体武器、Projectile/Wave Actor、Enemy、UI、Audio、动画、材质或纹理。它只接受 Weapons 产生的 `HitIntent` 并返回/发布最终 `HitResolved`。

### 归属 `ReEchoWeapons`

1. 武器不可变运行时 Definition、构筑后的有效参数和装备/步骤只读 Snapshot；
2. `WeaponRuntime` 的唯一普通攻击频率门、连段步骤游标、提交和 AttackIdentity 传递；
3. AttackStep 解释器与近战、Projectile、Wave 等可注册逻辑执行器；
4. 逻辑投射物/光波的位置、速度、范围、穿透、生命周期、已命中目标集合和 `HitIntent`；
5. Weapon/Projectile 公共事件和只读 Snapshot；不依赖资源的单元/世界自动化测试。

Weapons 可以依赖 Combat 导出的窄契约，但不得直接修改 Combatant/ASC 生命，不得决定格挡、最终伤害、击杀或死亡，也不得包含 UI、Audio、动画、材质和纹理资源引用。

### 保留在主 `ReEcho` / Presentation 适配层

- Pawn 的移动、相机、输入绑定和视觉朝向适配；
- CSV/XLSX Reader 以及将数据行编译为 Weapons Definition 的适配器；
- Player、Enemy、Echo、Encounter、Run、Recording 的装配与世界生命周期；
- Weapon/Sprite/Mesh、Projectile/Wave 视觉、动画、VFX、伤害数字、UI、Audio 和镜头反馈适配器；
- 对逻辑事件安全绑定/解绑，并把语义 ID 映射到具体资产。表现适配器只读事件和 Snapshot，不产生命中、不修改逻辑时间或状态。

依赖方向：

```text
ReEcho ───────────────→ ReEchoCombat
   ├──────────────────→ ReEchoWeapons ─────────→ ReEchoCombat
   └──────────────────→ ReEchoAudio

ReEchoCombat  ─/─→ ReEchoWeapons / ReEcho / ReEchoAudio / Presentation
ReEchoWeapons ─/─→ ReEcho / ReEchoAudio / Presentation
ReEchoAudio   ─/─→ ReEcho / ReEchoCombat / ReEchoWeapons
```

## 锁定验收

- [x] `ReEchoCombat` 与 `ReEchoWeapons` 均为独立 Runtime Module，可在 Editor/Game Target 构建；依赖方向符合模块边界，两个逻辑模块都不依赖主模块、Audio、UI、Run、Recording、Presentation 或具体资源类。
- [x] `AttackControllerComponent` 同时承接自动/手动攻击请求；物理输入只在手动模式有效，自动模式只在存活、无菜单且存在合法目标时保持同一 GAS 普攻输入。
- [x] `TargetingComponent` 保留当前范围、最近距离与 SpawnIndex tie-break；无目标立即释放自动请求，不产生空挥。
- [x] 基础攻击 Ability 不再 cast/include `AReEchoPlayerPawn`，通过 Combat Host/Weapon Runtime 契约工作；自动与手动继续共用同一 `Input_Attack_Basic` spec。
- [x] 普通攻击只有一个频率状态：`AttackIntervalSeconds / AttackSpeed` 写入 WeaponRuntime 的下一次可提交时间。GAS、Pawn、WeaponActor 不存在第二个基础攻击 cooldown/interval/lock 计时器。
- [x] Plan38 的“持续按住时，暂时未就绪不会永久结束循环”保持；每次成功提交只产生一次步骤推进和统一 AttackIdentity，所有后续载体/命中/结算复用完整身份。
- [x] Weapons 只在成功提交后执行逻辑近战/Projectile/Wave；失败/等待请求不播放攻击、不生成逻辑载体、不推进步骤、不发布 AttackCommitted。
- [x] Weapons 只产生 `HitIntent`，Combat 是最终伤害/格挡/元素/生命/击杀/死亡唯一裁决者；任何攻击载体和表现代码都不能直接扣血。
- [x] CombatEvents 至少覆盖 AttackCommitted、HitResolved/Hit、Hurt、Kill、Death；事件带统一 AttackIdentity、目标标识和必要世界位置，但不携带 UI/音频/动画资源路径；延迟载体不得查询已销毁来源 Actor。
- [x] `HealthChanged` 和伤害/攻击事件使用类型化 Payload；Hit/Hurt/Kill/Death 对同一结算复用同一个 AttackIdentity/结果，不重复计算。
- [x] `FReEchoCombatantSnapshot` 与 `FReEchoAttackSnapshot` 由权威组件一次构造并通过只读 provider 返回；UI 无需 include 私有 AttributeSet/WeaponRuntime，也不保存第二份权威战斗状态。
- [x] 外部修改只经过类型化 Combat Command；自动/手动输入、菜单/死亡释放、攻击模式、伤害/治疗均没有供 Widget 直接写内部字段的公共 API。
- [x] 逻辑 Projectile/Wave 有稳定 ID 和只读 Snapshot；视觉对象只跟随逻辑快照，视觉缺失、延迟、提前销毁或资源加载失败不改变命中和伤害。
- [x] UI、动画、VFX、音频与 Presentation 订阅生命周期可安全解绑；Actor/World 销毁、暂停、重新开始或继续游戏后不存在悬空回调、重复绑定或重复反馈。
- [x] 删除/禁用全部表现消费者后，自动/手动、近战、投射物、光波、元素、死亡和 Echo 攻击仍可由逻辑自动化完整运行。
- [x] 现有伤害、格挡、治疗、死亡、元素、OnKill、Echo 武器攻击和主动技能结果不回归；自动普通攻击仍不写入 Recording。
- [x] `DurationSeconds` 不再控制或阻止自动/手动普通攻击；`AttackIntervalSeconds` 及其构筑修改只通过 WeaponRuntime 的唯一 readiness 生效，并有明确诊断/测试。
- [x] 移动的 `UCLASS/USTRUCT/UENUM` 有精确 Core Redirect 或无路径变化兼容层；旧 `/Script/ReEcho.*` Blueprint/WBP/DataAsset/SaveGame 引用实际加载成功，RunSave 版本不变。
- [x] `FReEchoBuildSnapshot`、Recording 字节语义、稳定 Gameplay Tag/FName/CSV ID 不变；无第二份战斗状态或兼容分叉。
- [x] Plan34 的 AudioEvents/XLSX/设置和 Plan40 的 Presentation/Animation2D/资产不被修改；主模块只提供只读事件/Snapshot表现适配接口。
- [ ] 聚焦攻击/Combat/Weapon/Element/Echo/Save/Recording 测试、完整 `ReEcho.*` 自动化、UE 5.8 Editor 构建、项目校验和 `git diff --check` 通过。
- [ ] 最终候选 FullRebuild，且只提交 `GIT_RULES.md` 允许的四个 Runtime Module Editor 预构建包及 manifest 跟踪文件。
- [ ] 用户在 PIE 验收自动/手动切换、不同武器连续攻击、攻速变化、无目标、菜单/死亡释放、命中/击杀和继续游戏后模式恢复。

## 反射与序列化兼容

- 实施前列出所有移动反射类型及旧 `/Script/ReEcho.*` 路径。
- 使用精确 `ClassRedirects`、`StructRedirects`、`EnumRedirects`；禁止模糊整包重定向。
- 自动化必须实际加载受影响的旧 SaveGame 和现有 Blueprint/WBP/DataAsset，不能只检查配置字符串。
- 若某类型无法安全迁移，允许保留反射壳在主模块、把纯实现委托给 Combat Module；不得为了“文件都搬走”破坏兼容。

## Step 0 门禁

- fetch 并确认 `origin/main` 包含本 Plan；若 main 前进，先报告物理/Git 冲突、逻辑冲突和耦合，等待用户选择。
- 保留 Plan34 worktree，不合并其旧工作簿；将其重叠所有权退回 Reserved。Plan40 保持 Reserved，不覆盖其动画 WIP。
- 构建/命令行 Editor 前请用户保存并关闭 Editor；确认无有效 Unreal lock。
- 迁移前记录 Plan38 held-repeat、AttackMode、GAS/Combat/Weapon/Element、Echo、Save/Recording 聚焦基线。
- 停止条件：出现本 Plan 明确频率变更之外的伤害/元素/存档/录制语义改变；无法消除 Combat -> ReEcho 反向依赖；需要编辑 Plan34/40 独占资产；反射引用无法兼容加载。

## 实现提纲

1. 清点 include 图、反射路径、当前攻击状态和聚焦基线，建立模块边界静态测试。
2. 创建 `ReEchoCombat` 与 `ReEchoWeapons` 空模块并证明构建、加载和依赖方向。
3. 迁移最小战斗值类型、GAS tags/attributes/effects/abilities 和 Combatant；同步 API 宏/Core Redirect。
4. 新建 Target/HitResolution/Weapon Executor/Combat Host、统一 AttackIdentity、类型化 Events、只读 Snapshots/providers 和受控 Commands 公共契约，不暴露具体主模块类型。
5. 实现 `TargetingComponent`，迁移当前确定性自动选敌规则。
6. 实现 `AttackControllerComponent`，迁移自动/手动 held、模式门控以及菜单/死亡释放。
7. 实现 Weapons WeaponRuntime：将攻击步骤定义从 CSV Adapter 编译为模块自有数据，使用 `AttackIntervalSeconds / AttackSpeed` 维护唯一 readiness 和步骤提交；`DurationSeconds` 只进入非阻塞步骤行为/表现数据。
8. 改写 BasicAttack Ability：等待 WeaponRuntime readiness，删除基础攻击 GAS cooldown和步骤锁第二道门；主动技能冷却保持独立。
9. 把近战、Projectile/Wave 迁为 Weapons 逻辑执行器，只产生 HitIntent；Combat HitResolver 统一裁决并产生 HitResolved。将 WeaponActor/Enemy/Pawn/Echo 改为装配或表现适配器。
10. 加载旧资产/存档并验证 Core Redirect、CDO、序列化往返和稳定 ID。
11. 运行聚焦与完整回归；格式化、构建、静态校验；提交本地 Review 候选。
12. 用户 PIE 通过后，Planner 合入 main、FullRebuild、刷新四模块预构建包、关闭并发布。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 依赖 | Build.cs/include 审计与校验器 | `ReEcho -> ReEchoWeapons -> ReEchoCombat`，逻辑模块无主模块/表现依赖 |
| 组件 | AttackController/Targeting/WeaponRuntime/Projectile 单元测试 | held、模式、确定性目标、唯一 readiness、步骤、载体生命周期正确 |
| 公共数据 | AttackIdentity/Event/Intent/Result/Snapshot/Command 自动化 | 身份集中、结果字段完整、同一攻击关联一致、Snapshot 只读、非法命令不改状态 |
| 攻击 | Plan28/38 与新 SingleCadence 测试 | 自动/手动共享路径，等待不丢循环，频率只读 AttackIntervalSeconds |
| 结算 | Combatant/Weapons/HitResolver/Element/Events 测试 | Weapons 不扣血，Combat 对伤害、格挡、元素、击杀与事件唯一裁决 |
| 兼容 | Core Redirect、旧资产/旧保存加载与往返 | 旧 `/Script/ReEcho.*` 引用和 RunSave 正常 |
| 跨域 | Echo/Recording/Save/Presentation contract 测试 | 无录制、存档、表现调用语义回归 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | 四个 Runtime Module UHT/UBT 成功 |
| 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | 完整套件通过 |
| 静态 | `python scripts/validate_project.py`; `git diff --check` | 项目不变量和路径审计通过 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | manifest/DLL/源码指纹匹配 |
| 人工 | 用户 PIE 清单 | 攻速与手感、模式、武器、菜单/死亡/继续均通过 |

## Executor 交接

本 Plan 由当前 Gavyn-side AI 直接执行，不再生成长启动 prompt。实现必须先发布 Plan，再从 freshly fetched、人工批准的 `origin/main` 建独立 worktree。若任何设计需要逻辑模块 include 主模块/表现类型、Combat include Weapons，或表现回调控制逻辑，停止该方向并用窄接口或主模块适配器重做。任何新设计决定先回写本 Plan 的设计决策记录。不得要求 Executor 做视觉/手感验收。

## 执行记录

### 变化

- 2026-08-13 用户审核后扩大既有 Review 候选：新增 `ReEchoWeapons` 逻辑模块，并锁定逻辑/表现单向分层；原“具体 Weapon/Projectile/Wave 逻辑保留主模块”的边界已废止，旧构建/自动化证据不再作为最终候选证据。
- 将分散的 `SourceActor + CommitId` 调用约定收敛为统一 `FReEchoAttackIdentity`；武器提交、Projectile/Wave、元素状态、Hit/Hurt/Kill/Death 全链复用同一身份。
- 新增独立 `ReEchoWeapons` Runtime Module：`FReEchoWeaponLogic` 独占 readiness、步骤、暴击/元素游标和提交事务；逻辑 Projectile/Wave 共用 `UReEchoProjectileLogicComponent`、稳定 ID、Snapshot 和命中意图。
- 武器提交采用确认/回滚事务；载体创建或世界执行失败时完整恢复 readiness、步骤、暴击累积和元素游标，但不复用已分配的攻击序号。
- `ReEchoCombat` 新增统一 `ResolveHit`：Weapons 和视觉 Actor 不再注入结算回调或直接扣血；物理、元素、格挡、生命、击杀、死亡和类型化事件均由 Combat 裁决。
- CSV 只在主模块读取并编译为不可变 `FReEchoElementRuleSet`；元素附着、免疫、强化与燃烧运行态迁入 `CombatantComponent`，Enemy 只保存/恢复快照并订阅事件刷新表现。
- Player/Enemy 均实现最小 CombatTarget；敌人接触攻击也通过统一 HitResolver，`HealthChanged` 携带 Reason、AttackIdentity 与 DamageSource。

- Added the standalone `ReEchoCombat` Runtime Module and moved GAS tags, attributes, effects, abilities, combat values, and `UReEchoCombatantComponent` behind a one-way `ReEcho -> ReEchoCombat` dependency.
- Added typed attack/target host interfaces, `UReEchoAttackControllerComponent`, `UReEchoTargetingComponent`, combat events, read-only snapshots, and typed attack-mode commands.
- Replaced the dual basic-attack gate with one weapon cadence: `AttackIntervalSeconds / AttackSpeed`. `DurationSeconds` now drives only non-blocking step behavior, movement, invulnerability, and presentation timing.
- 旧单模块候选曾适配 Pawn、WeaponActor、Enemy、projectile、staff wave、immediate element damage 和测试；该结果现作为迁移起点，不是新的双逻辑模块最终边界或证据。
- Added a main-module audio adapter component that safely binds/unbinds Combat events and translates only semantic IDs into `ReEchoAudio`; neither runtime module depends on the other.
- Added exact Core Redirects for reflected types moved from `/Script/ReEcho` to `/Script/ReEchoCombat` and a static module-boundary validator.
- 已按统一架构文档库维护候选结构：更新 `shared/CODEBASE_MAP/ARCHITECTURE.md` 的四模块拓扑与状态流，更新 `README.md` 的稳定路由，并新增 `modules/MOD-ReEchoCombat.md`、`MOD-ReEchoWeapons.md`。两份模块文档分别记录存在原因、权威状态、公共契约、运行流程、扩展边界、测试和真实代码位置；在 Plan41 合入前只随本候选分支存在，不提前冒充远端 `main` 架构。

### 当前客观证据（2026-08-13）

- 用户已手工通过合入 Plan40 前的 Plan41 PIE，并授权发布。外部审计发现 `origin/main@7ae6738` 已包含 Plan40 表现和后续相机提交；用户选择 combined adaptation：保留远端 PresentationController/Profile/逐帧 Query 碰撞和资产，以本 Plan AttackController/Combat/Weapons 为逻辑权威。
- 用户明确采用远端 `CameraOrthoWidth = SceneWorldHeight * SceneAspectRatio * 2.0f`：接受同一值写入 `ArenaSceneWorldWidth`，即相机视野、玩家横向移动边界和敌人横向生成范围一起扩大两倍；不实施“仅扩大视野”的拆分适配。
- 组合冲突已确定性解决：`PlayerPawn` 不再恢复 `bAutoAttackMode`/两份 held 字段，所有查询和命令继续委托 AttackController；Plan40 的 PresentationController 与 FrameCollisionDriver 同时保留。Enemy/WeaponActor 的表现增量与 CombatTarget/WeaponLogic 自动合并后通过源码审阅。
- 组合候选 UE 5.8 Win64 Development `-FullRebuild` PASS，四个 Runtime Module 全部从源编译并刷新预构建包，源码指纹 `41ad26014266`。
- 完整 `ReEcho` 自动化入口在进入测试队列前被引擎平台预检阻止：Win64 SDK 有效，但本机缺少 LinuxArm64/VisionOS `SDK.json MainVersion`；与 Plan40 已记录环境限制一致，不计为断言失败或通过。
- 用户 PIE 发现延迟投射物命中时，`FReEchoAttackIdentity::Source` 的失效 `TObjectPtr` 在 `ResolvePhysicalHit` 中触发访问冲突。已改为 `TWeakObjectPtr<AActor>`，来源离开世界后只跳过来源侧反馈，已快照的目标伤害仍完成。
- 修复后 Development Editor 构建 PASS；`ReEcho.Combat.*` 7/7、`ReEcho.Weapons.*` 9/9、`ReEcho.AttackMode.*` 7/7 PASS。
- 新增两级回归：`ReEcho.Combat.AttackIdentity.SourceLifetime` 证明失效来源不会被解析；`ReEcho.Weapons.ProjectilesUseSingleShotSpreadCountAndExplosion` 现在覆盖“发射后销毁来源、延迟命中仍结算且不崩溃”。

- Development Editor build: PASS，四个 Runtime Module（ReEcho、ReEchoCombat、ReEchoWeapons、ReEchoAudio）均由 UHT/UBT 构建并刷新预构建清单。
- `ReEcho.AttackMode.*`: 7/7 PASS；自动/手动 held、输入源、无录制、存档迁移和确定性目标均通过。
- `ReEcho.Combat.Element*`: 4/4 PASS；纯规则、Burn 刷新、保存连续性、Growth/Conduct 世界行为均通过。
- `ReEcho.Weapons.*`: 9/9 PASS；唯一频率门、步骤、装备数值、OnKill、Projectile/爆炸与 Run 快照均通过。
- 完整 `ReEcho.*`: 本 Plan 影响范围全部 PASS；唯一失败仍为 Plan41 前已存在且未触碰的 `ReEcho.Run.EchoReplayResolver.EmptyStale`（Plan31 replay resolver 基线缺陷）。
- clang-format 已对 42 个本次修改/新增 C++ 文件执行；最终 validator、diff-check 在显式暂存预构建文件后重跑。

### 当前剩余工作与风险

- 合入 Plan40 后，原纯 Plan41 PIE 证据对玩家/敌人/武器表现接缝部分失效；最终推送前仍需用户用组合候选做最小 PIE：连续攻击、自动/手动切换、攻击动画和至少一个敌人命中/死亡。
- The unrelated Plan31 replay-resolver baseline defect is not changed by Plan41 and needs a separately owned fix.
- Final FullRebuild, merge to main, remote publication, and worktree cleanup remain gated on user PIE approval.

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md` 已更新：四模块拓扑、Combat/Weapons 状态所有权、HitIntent→ResolveHit→Events 流向和表现只读不变量与组合候选一致。
- `shared/CODEBASE_MAP/README.md` 已更新：`MOD-ReEchoCombat`、`MOD-ReEchoWeapons` 及迁移后的 `AREA-AbilityCombat` / `AREA-Weapons` 路由指向真实代码与独立文档，并吸收最新模块文档强制门禁。
- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已更新：主模块的数据编译、世界装配、Plan40 PresentationController/逐帧 Query 碰撞以及 Combat/Weapons 适配位置均与组合候选一致。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md` 已创建并更新：记录攻击控制、Combatant、GAS、AttackIdentity、HitResolver、事件/快照、依赖和测试位置。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md` 已创建并更新：记录唯一 cadence、步骤事务、逻辑 Projectile/Wave、HitIntent、主模块数据/表现适配边界和测试位置。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoAudio.md` 已审阅、无需修改：本 Plan 只在主模块新增 Combat→Audio 语义适配，未改变 Audio 模块的目录、策略、总线、服务、后端或公共请求契约。

### 人工验收结果/请求

请用户从 `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan41\ReEcho.uproject` 启动 PIE，按上面的人工验收清单测试；通过前不合入 main。
