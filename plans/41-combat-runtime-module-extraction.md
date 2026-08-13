# Plan 41 - Combat - 独立模块与攻击系统重构

## 协调

- Planner 负责人：Gavyn-side Planner。
- Executor 负责人：Gavyn-side Planner（本对话 AI 直接执行）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。用户负责最终 PIE、攻速和手感验收。
- 本地规划 / 实现基线：`origin/main@748523644053a9dae4a2b4a7b113444a26f35e24`；执行前重新 fetch，并以包含本 Plan 的最新人工批准 `origin/main` 为准。
- 实现分支：本地 `plan/41-combat-runtime-module`，独立 worktree。
- 依赖 / 阻挡：Plan33 已关闭。Plan34 本地 WIP 保留但暂停占用 `ReEcho.Build.cs`/预构建包；Plan40 保持 Reserved 且不得在本 Plan 实施期间进入 Active。Plan41 合入后，两者必须在新模块边界上重新适配和验证。
- Writes：新增 `Source/ReEchoCombat/**`；`ReEcho.uproject`；`Source/ReEcho/ReEcho.Build.cs`；必要的 `Config/DefaultEngine.ini` Core Redirects；迁移/适配 `Source/ReEcho/{Public,Private}/{AbilitySystem,Combat,Core,Player,Weapons,Graybox}/**`；攻击/战斗/模块测试；校验脚本；本 Plan 执行记录；最终 `GIT_RULES.md` 允许的 Win64 Editor 预构建包。
- Stable Reads：`Data/**` 数据快照和生成契约、`Run/**` 攻击模式存档、`Recording/**` 录制语义、`ReEchoAudio` API、Plan28/38 攻击行为、Plan40 表现请求契约。
- 影响模式：战斗公共类型、反射路径、GAS、攻击组件和模块依赖为 `SharedContract`；模块描述符、Build.cs、Core Redirects 和最终预构建包为 `Exclusive`。
- 兼容承诺 / 下游操作：依赖必须为 `ReEcho -> ReEchoCombat`，禁止反向依赖。现有存档版本、Gameplay Tag/FName、伤害/元素结果、自动/手动模式、新局/Continue 规则和录制语义保持不变。普通攻击节奏按本 Plan 的唯一频率决定有意调整；UI、动画和音频只消费战斗事件，不反向控制战斗。
- 明确排除：不改战斗数值；不修改 XLSX/生成 CSV；不删除 `AttackIntervalSeconds` 兼容列；不重做敌人 AI、武器表现、动画、美术、UI、商店、抽卡、Echo 录制或音频播放；不创建 `ReEchoFrontend`/`ReEchoCore`；不网络化或插件化。

## 锁定目标

一步建立独立 `ReEchoCombat` Runtime Module，并把当前分散在 Pawn、GAS 和 WeaponActor 中的普通攻击重构成组件化单一管线：

```text
玩家输入 / 自动模式
→ AttackControllerComponent（是否持续请求攻击）
→ TargetingComponent（提供确定性合法目标）
→ GAS（能力状态与主动技能冷却）
→ WeaponRuntime（唯一普通攻击节拍、步骤与提交）
→ Combatant / ElementReaction（伤害结算）
→ CombatEvents（攻击、命中、受击、击杀、死亡）
→ 主模块适配 Audio / UI / Presentation
```

模块内部按职责选择载体：跟随 Actor 的状态用 Component/ASC，能力用 GAS，数值和步骤规则用普通 C++ 对象，跨模块使用窄接口和语义事件。不得把所有规则强行做成 ActorComponent，也不得因组件化复制状态真相源。

## 锁定产品与技术决定

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
- `WeaponRuntime` 独占步骤游标、唯一节拍、攻击提交和每次提交 ID；具体世界 Actor/VFX/投射物由主模块 Weapon 适配器执行。
- `CombatantComponent`/ASC 独占生命、属性和死亡状态；UI 只读或订阅事件。
- `CombatEvents` 只发布已发生的语义结果；Audio、UI、Presentation 不得通过回调改变结算结果。

## 模块边界

### 归属 `ReEchoCombat`

1. 最小战斗值类型：元素、伤害来源、`FReEchoStatBlock`、元素状态及必要的攻击请求/提交/结果类型；
2. Gameplay Tags、AttributeSet、GameplayEffects、基础/主动 GameplayAbility；
3. `UReEchoCombatantComponent`；
4. `UReEchoAttackControllerComponent`；
5. `UReEchoTargetingComponent` 与 Combat Target 接口；
6. 纯 `WeaponRuntime` 步骤/节拍状态及 Weapon Executor 接口；
7. 不依赖 Data Registry/具体 Enemy 的纯元素规则；
8. `CombatEvents` 公共事件契约及无设备/无世界单元测试。

### 保留在主 `ReEcho`

- Pawn 的移动、相机、输入绑定和视觉朝向适配；
- WeaponActor 的具体世界表现、投射物、近战查询和 VFX；
- CSV/XLSX Reader 以及将数据行编译为 Combat Weapon Definition 的适配器；
- Enemy、Echo、Encounter、Run、Recording、UI、Presentation 和 Audio 转发；
- 依赖具体世界遍历的元素反应应用层。

依赖方向：

```text
ReEcho ───────────────→ ReEchoCombat
   └──────────────────→ ReEchoAudio

ReEchoCombat ─/─→ ReEcho / ReEchoAudio
ReEchoAudio  ─/─→ ReEcho / ReEchoCombat
```

## 锁定验收

- [ ] `ReEchoCombat` 是独立 Runtime Module，可在 Editor/Game Target 构建；`ReEcho` 单向依赖它，Combat Module 不 include/依赖主模块、Audio、UI、Run、Recording、Presentation 或具体 Player/Weapon/Enemy 类。
- [ ] `AttackControllerComponent` 同时承接自动/手动攻击请求；物理输入只在手动模式有效，自动模式只在存活、无菜单且存在合法目标时保持同一 GAS 普攻输入。
- [ ] `TargetingComponent` 保留当前范围、最近距离与 SpawnIndex tie-break；无目标立即释放自动请求，不产生空挥。
- [ ] 基础攻击 Ability 不再 cast/include `AReEchoPlayerPawn`，通过 Combat Host/Weapon Runtime 契约工作；自动与手动继续共用同一 `Input_Attack_Basic` spec。
- [ ] 普通攻击只有一个频率状态：`AttackIntervalSeconds / AttackSpeed` 写入 WeaponRuntime 的下一次可提交时间。GAS、Pawn、WeaponActor 不存在第二个基础攻击 cooldown/interval/lock 计时器。
- [ ] Plan38 的“持续按住时，暂时未就绪不会永久结束循环”保持；每次成功提交只产生一次步骤推进、伤害执行和唯一 AttackCommitId。
- [ ] WeaponActor 世界适配只在成功提交后执行攻击表现/命中；失败/等待请求不播放攻击、不发射投射物、不推进步骤、不发布 Combat.Attack。
- [ ] CombatEvents 至少覆盖 AttackCommitted、Hit、Hurt、Kill、Death；事件带稳定提交/来源/目标标识和必要世界位置，但不携带 UI/音频/动画资源路径。
- [ ] 现有伤害、格挡、治疗、死亡、元素、OnKill、Echo 武器攻击和主动技能结果不回归；自动普通攻击仍不写入 Recording。
- [ ] `DurationSeconds` 不再控制或阻止自动/手动普通攻击；`AttackIntervalSeconds` 及其构筑修改只通过 WeaponRuntime 的唯一 readiness 生效，并有明确诊断/测试。
- [ ] 移动的 `UCLASS/USTRUCT/UENUM` 有精确 Core Redirect 或无路径变化兼容层；旧 `/Script/ReEcho.*` Blueprint/WBP/DataAsset/SaveGame 引用实际加载成功，RunSave 版本不变。
- [ ] `FReEchoBuildSnapshot`、Recording 字节语义、稳定 Gameplay Tag/FName/CSV ID 不变；无第二份战斗状态或兼容分叉。
- [ ] Plan34 的 AudioEvents/XLSX/设置和 Plan40 的 Presentation/Animation2D/资产不被修改；主模块只提供事件适配接口。
- [ ] 聚焦攻击/Combat/Weapon/Element/Echo/Save/Recording 测试、完整 `ReEcho.*` 自动化、UE 5.8 Editor 构建、项目校验和 `git diff --check` 通过。
- [ ] 最终候选 FullRebuild，且只提交 `GIT_RULES.md` 允许的三个 Runtime Module Editor 预构建包及 manifest 跟踪文件。
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
2. 创建 `ReEchoCombat` 空模块并证明构建、加载和单向依赖。
3. 迁移最小战斗值类型、GAS tags/attributes/effects/abilities 和 Combatant；同步 API 宏/Core Redirect。
4. 新建 Target/Weapon Executor/Combat Host/Events 公共契约，不暴露具体主模块类型。
5. 实现 `TargetingComponent`，迁移当前确定性自动选敌规则。
6. 实现 `AttackControllerComponent`，迁移自动/手动 held、模式门控以及菜单/死亡释放。
7. 实现 Combat WeaponRuntime：将攻击步骤定义从 CSV Adapter 编译为模块自有数据，使用 `AttackIntervalSeconds / AttackSpeed` 维护唯一 readiness 和步骤提交；`DurationSeconds` 只进入非阻塞步骤行为数据。
8. 改写 BasicAttack Ability：等待 WeaponRuntime readiness，删除基础攻击 GAS cooldown和步骤锁第二道门；主动技能冷却保持独立。
9. 将 WeaponActor/Enemy/Pawn 改为世界适配器；成功提交后才执行命中、表现与 CombatEvents。
10. 加载旧资产/存档并验证 Core Redirect、CDO、序列化往返和稳定 ID。
11. 运行聚焦与完整回归；格式化、构建、静态校验；提交本地 Review 候选。
12. 用户 PIE 通过后，Planner 合入 main、FullRebuild、刷新三模块预构建包、关闭并发布。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 依赖 | Build.cs/include 审计与校验器 | `ReEcho -> ReEchoCombat` 单向，Combat 无具体主模块依赖 |
| 组件 | AttackController/Targeting/WeaponRuntime 单元测试 | held、模式、确定性目标、唯一 readiness、步骤推进正确 |
| 攻击 | Plan28/38 与新 SingleCadence 测试 | 自动/手动共享路径，等待不丢循环，频率只读 AttackIntervalSeconds |
| 结算 | Combatant/Weapon/Element/Events 测试 | 伤害、格挡、元素、击杀与事件一一对应 |
| 兼容 | Core Redirect、旧资产/旧保存加载与往返 | 旧 `/Script/ReEcho.*` 引用和 RunSave 正常 |
| 跨域 | Echo/Recording/Save/Presentation contract 测试 | 无录制、存档、表现调用语义回归 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | 三个 Runtime Module UHT/UBT 成功 |
| 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | 完整套件通过 |
| 静态 | `python scripts/validate_project.py`; `git diff --check` | 项目不变量和路径审计通过 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | manifest/DLL/源码指纹匹配 |
| 人工 | 用户 PIE 清单 | 攻速与手感、模式、武器、菜单/死亡/继续均通过 |

## Executor 交接

本 Plan 由当前 Gavyn-side AI 直接执行，不再生成长启动 prompt。实现必须先发布 Plan，再从 freshly fetched、人工批准的 `origin/main` 建独立 worktree。若任何设计需要 `ReEchoCombat` include 主模块类型，停止该方向并用窄接口或主模块适配器重做。不得要求 Executor 做视觉/手感验收。

## 执行记录

### 变化

### 证据

### 剩余风险

### 人工验收结果/请求

等待用户 PIE 验收。
