# Plan 63 — 琐碎 Bug 修复长期积压（Trivial Bugfix Backlog）

- **计划 / Plan**：63
- **角色 / Role**：Programmer
- **秘书状态 / Secretary status**：InProgress（长期更新，living document）
- **工作树 / Worktree**：`ReEcho-plan63` → 分支 `plan/63-trivial-bugfix`
- **目标 / Goal**：收集并修复不构成独立 Plan 的小缺陷、可复现性增强（如调试 GM）、表现/手感微调等琐碎项；保持 main 稳定，每个修复以独立提交 fast-forward 到 main。

## 为什么需要这个 Plan

Demo 稳定化阶段（P0）持续暴露零散小问题：单个修复体量不足以开一个完整 Plan，但又需要编号、可追溯、可回滚。本 Plan 作为"长期积压"容器，持续接收这类改动，避免在主工作树随意打补丁、也避免为鸡毛蒜皮开一堆短命 Plan。

## 使用约定

- 所有实现落在 `plan/63-trivial-bugfix` 工作树/分支；主工作树 `ReEcho` 只接收 fast-forward。
- 每个修复在本文档「修复记录」追加一条，含：现象、根因、改动文件、验证方式。
- 发布前走标准门禁：`Build-Editor -FullRebuild` + `validate_project.py` + 提交 + fast-forward main + push origin/main。
- 若某个修复膨胀为独立系统级改动，应拆分出独立 Plan，不再计入本 Plan。

## 修复记录

### #1 — 新增 `GMGrantCard`：GM 直接获得指定卡牌（用于复现/验证静默刻度 G_2_17）

- **现象 / Symptom**：没有便捷手段在运行中直接拿到某张卡（如静默刻度 `G_2_17`）来观察其整条效果链路；只能靠正常商店/构筑流程，复现成本高。
- **根因 / Root cause**：现有 `ApplyTraitCard` 仅在 `Phase==CardChoice` 且卡牌在 `PendingTraitCardIds` 时生效，无法在任意时刻、对任意卡牌授予。权威构建变更封装 `TryMutateAuthoritativeBuild` 仅在该 .cpp 内部可见，外部（含 GM）无法复用。
- **改动 / Changes**：
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h` + `ReEchoRunSubsystem.cpp`：新增 `UReEchoRunSubsystem::DebugGrantCard(FName CardId)`，复用 `TryMutateAuthoritativeBuild` + `ReEchoCardRuntime::TryGrantCard` + `ReEchoCharacterPromotion::TryPromote`，忽略阶段/候选限制，可随时授予。
  - `Source/ReEcho/Public/ReEchoGameMode.h` + `ReEchoGameMode.cpp`：新增 `GMGrantCard(FName CardId)`（`UFUNCTION(Exec)`），沿用 `EnsureGMCommandAvailable`（`#ifndef _SHIPPING` 守卫）+ `PrintGMResult`（`[GM]` 前缀）。
  - `docs/GM_COMMANDS.md`：补充命令说明。
- **用法 / Usage**：PIE 或 Development 构建按 `~` 打开控制台，输入 `GMGrantCard G_2_17` 即可获得静默刻度；`GMGrantCard` 无参打印用法。Shipping 构建命令体被守卫跳过。
- **验证 / Verification**：`Build-Editor -FullRebuild` 通过；`validate_project.py` 静态校验通过；逻辑复用已验证的 `TryGrantCard` 路径，与正常特质卡授予行为一致。

### #2 — 修复六武器生产表触发启动期 CSV Fatal

- **现象 / Symptom**：Plan62 已把输入槽扩展到 `1..6`，且五个可选武器使用 `LoadoutOrder 1、2、4、5、6`；但 Editor/自动化在模块启动时依次因旧的 `1..3` 顺序上限、匕首删除前的 `78/16/62` 部件计数、以及“至少存在一个 AttackPatternReplacement/UniqueBehavior”而 Fatal。
- **根因 / Root cause**：`ReEchoWeaponCsvReader.cpp` 的输入槽解析和枚举已经扩展到 6，但三组生产数据审计仍绑定删除匕首前的内容；删除匕首后，当前生产表不再含由匕首提供的替换攻击模式和唯一行为部件效果。
- **改动 / Changes**：校验上限改为从 `EReEchoInputSlot::Slot6` 派生的具名常量；部件审计同步为当前权威表的 `70` 总行、`10` 具名行和 `60` 禁用空名行；保留两类效果的 Schema/Behavior 校验能力，但不再强制生产表必须至少配置一条。继续保留 `LoadoutOrder` 唯一性和 `StartSelectable` 必须绑定输入槽等既有规则，不重排生产表。
- **验证 / Verification**：Plan58 合入最新 main 的组合候选已通过启动期 CSV 加载、VFX 与兔子投射物聚焦自动化、Editor `-FullRebuild`、静态项目校验和 diff 检查。

### #3 — 启动崩溃排查（实为工作树落后 main）+ 保留 `GMGrantCard` 诊断日志

- **现象 / Symptom**：从 `plan/63-trivial-bugfix` 工作树直接打开编辑器启动即崩溃。
- **根因 / Root cause**：plan63 工作树当时落后 `origin/main` 共 6 个提交（Plan58 兔子投射物视觉/碰撞对齐等），其 `ReEchoEnemyActor` / VFX 的启动期改动修复了崩溃；plan63 未合入这些提交，故复现崩溃。并非 Plan63 自身代码引入缺陷。
- **改动 / Changes**：
  - 将 `origin/main`（`3164f52`）fast-forward 合并进 `plan/63-trivial-bugfix`，消除启动崩溃。
  - 保留并正式提交 `DebugGrantCard` / `GMGrantCard` 上的诊断日志（`[DebugGrantCard]` / `[GMGrantCard]` 前缀，`LogReEcho` Warning 级），用于后续任意卡牌授予的可追溯复现；日志仅打印 `Phase / EncounterIndex / TimeShards / OwnedCardIds.Num()` 等只读状态，不影响构建快照。
  - 二进制冲突（ReEcho.dll / prebuilt.json）按 main 版本解决后本地增量重建，不随本提交入库，待发布时由 `-FullRebuild` 重新生成。
- **验证 / Verification**：用户实机打开合并后的 plan63 工作树，启动崩溃消失；`GMGrantCard G_2_17` 诊断日志链路打印正常（enter → grant ok → done，卡数 +1）。本条目结束，Plan 保持开放等待下一琐碎 bug。

### #4 — 仅大关切换回血，小关保留残余血量

- **现象 / Symptom**：原 `BeginNextEncounter` 无条件 `InitializeFromStats(Stats, true)`，小关（同 Stage 内连续遭遇）与大关（跨 Stage 切换）进入时都回满血，与"小关连续作战应保留血量"的设计意图不符。
- **改动 / Changes**：`ReEchoGameMode.cpp` `BeginNextEncounter`（约 L1144）将 `bFillHealth` 由硬编码 `true` 改为 `!Transition.bSameStage`——仅跨 Stage（大关切换）回满，小关进入保留 `FMath::Min(当前血, HpMax)`（见 `ReEchoGameplayEffects::ApplyInitialization`：bFillHealth=false 时 `Health = FMath::Min(Attributes->GetHealth(), Stats.HpMax)`）。同时加 `[StageTransition] enter encounter ... fillHealth=...` 诊断日志，供 PIE 验证。
- **依据 / Basis**：`FReEchoStageTransitionDecision.bSameStage` 在同 Stage 连续遭遇为 `true`（小关）、跨 Stage 为 `false`（大关）；测试 `ReEchoStageTransitionTests` Cases 表（0→1 大关 / 1→2 小关 / … / Boss 边界大关）印证切换结构。
- **验证 / Verification**：待用户 PIE 实机确认——小关连续（如第2→3场）血量不回满、大关切换（如第1→2场、Boss 边界）回满。本条目待验收。

### #5 — 狐狸（Elite）正面方向护盾：待策划确认是否调整（记录挂起）

- **现象 / Symptom**：测试反馈 `GMSpawnFox` 生成的狐狸（敌人定义 `M_FOX`，kind=Elite）从正面打"不吃伤害"。
- **根因 / Root cause**：方向护盾是 **所有 Elite 敌人的统一机制**，非狐狸专属。`ReEchoEnemyDefinitionCompiler.cpp:124-125` 将 `bUsesDirectionalShield` 设为 `true`（Archetype 为 Shield 或 Elite）；`AReEchoEnemyActor::ModifyIncomingRawDamage`（`ReEchoEnemyActor.cpp:592-601`）对正面来向伤害返回 `0.0f`、背面返回 `RawDamage*2.0f`。故狐狸正面攻击归零、背面双倍——属设计行为，不是专属 bug 也不是不吃伤害。
- **待策划确认 / Open question**：① 是否预期（Elite 正面防御+背面弱点）；② 若改，方向是否反了、或想从"正面完全免疫"改为"正面减伤"（如 ×0.5）、或处理自动攻击（FIX-8/FIX-9 锁定正面时序）永远打正面的问题。
- **状态 / Status**：记录挂起（PendingDesigner），未实现改动；等策划确认后再决定改为代码修复或关闭。

### #6 — 主角受伤害来源日志（shipping 可用）

- **需求 / Need**：记录主角每次受到的伤害来自哪个对象，用于线上/发布包排查误伤与伤害来源（计划于 shipping 包保留）。
- **改动 / Changes**：`AReEchoPlayerPawn::ModifyIncomingRawDamage`（`ReEchoPlayerPawn.cpp:496`，玩家受伤最后一道关，每次受击必调）增加 `[PlayerDamage]` 诊断日志，打印 `RawDamage / 来源 Actor 名+类名 / EReEchoDamageSource 枚举 / EReEchoElement 枚举`；来源 Actor 取自 `Intent.Attack.Source`（TWeakObjectPtr，失效时记 `from=None`）。
- **shipping 兼容 / Shipping**：使用 `UE_LOG(LogReEcho, Warning, ...)`，**不包裹 `#ifndef _SHIPPING`**，故编译进 Shipping 二进制并写入日志文件（控制台不显示，但存档可查）。与项目既有 `LogReEcho` Warning 日志一致。
- **验证 / Verification**：PIE 实机受伤时 Output Log 出现 `[PlayerDamage]` 行；来源应仅为 `Enemy`/环境类（Echo=PlayerSide 同阵营不可互伤，见 #5 结论）。待验收。

### #7 — 商店购买 BuildCard 类商品导致时间碎片变负数

- **现象 / Symptom**：在商店购买 `G_2_15 时砂豪赌`（Tier2 / Trait / Economy 卡）后，时间碎片变为负数（如 `-售价`）。用户用 `GMSetShards <售价>` 把碎片设为刚好等于售价、再购买该卡可稳定复现。
- **根因 / Root cause**：`PurchaseShopItem`（`ReEchoRunSubsystem.cpp`）的 BuildCard 分支先以外部 `TimeShards` 做 `TimeShards < EffectivePrice` 校验（通过），再调用 `TryMutateAuthoritativeBuild` 内 `ReEchoCardRuntime::TryGrantCard`。`G_2_15` 的 `Card.NextShardDrop` 在 `OnGrant`（`ReEchoCardRuntime.cpp:324`）将 `Result.TimeShards = 0`（清零全部碎片），闭包随后 `PendingTimeShards = Grant.TimeShards`（=0）覆盖了校验基准；最后再 `PendingTimeShards -= EffectivePrice` → `0 - 售价 = 负数`。即"校验用买前余额、扣费用被卡效果覆盖后的余额"两基准不一致，且卡牌 OnGrant 的碎片清零与售价扣费是两个独立操作叠加成负。
- **改动 / Changes**：
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp` `PurchaseShopItem`：扣费改为**原子**——以购买前余额 `TimeShards` 为基准，叠加卡牌 `OnGrant` 对碎片的净改变 `GrantShardDelta = PendingTimeShards - TimeShards`，并对结果 `FMath::Max(0, TimeShards - EffectivePrice + GrantShardDelta)` 夹紧到 ≥0。这样 `G_2_15` 购买后碎片为 0（符合"失去所有碎片"语义）、`G_3_17 代价丰收`（+3000）等正向卡仍正常加碎片、普通卡正常扣售价，且永远不会出现负数。
  - 配套调试：`[ShopDebug]` 系列日志（enter / BuildCard grant 的 inTimeShards-outTimeShards / before-deduct / NEGATIVE RISK blocked 告警 / after-deduct），验证修复前后行为对比。
- **复现 / Repro**：`GMSetShards <G_2_15 售价>` → 商店购买 `G_2_15` → 修复后碎片应为 0（不再为负）；Output Log 过滤 `ShopDebug` 可见 `NEGATIVE RISK blocked` 告警（已被夹紧拦截）。
- **验证 / Verification**：待用户 PIE 实机确认——购买 `G_2_15` 后碎片 ≥0（预期 0），`[ShopDebug]` 日志 `after deduct timeShards=0`；并尝试其它 BuildCard/普通商品确认扣费正常。本条目待验收。

### #8 — UI 数值展示（战斗血量上限 / 商店主角属性面板 / 持有碎片）不在数值变化时即时刷新

- **现象 / Symptom**：在商店内购买会改变属性的卡（如 `FORGE_MEDIUM_HP` 减 HpMax / `FORGE_EXTREME` 减 HpMax+加攻击）后，商店主角属性面板仍显示购买前数值；用 GM（`GMAddShards` / `GMSetShards`）改动持有碎片时，商店内持有碎片显示不刷新。根因：属性面板 `SetPlayerStats` 仅在商店**打开时**被调用一次，且读取的是上一场战斗的 `Player->Combatant->Stats`（非"下一场将带着的属性"）；持有碎片显示同样只在商店内购买路径刷新，外部 GM 改动不触发。
- **根因 / Root cause**：UI 采用"打开时拉取一次"的快照式更新，而非"数值变更事件驱动"。商店主角属性面板来源应为权威构建 `CurrentBuild.Stats`（购买属性卡后立即反映），而非 `Combatant->Stats`（上一场残留）。
- **改动 / Changes**（`Source/ReEcho/Private/ReEchoGameMode.cpp`）：
  - `#8-A` 打开商店 `ShowInventoryShopMenu`：属性面板改读 `RunSubsystem ? RunSubsystem->CurrentBuild.Stats : Player->Combatant->Stats`（展示"下一场将带着的属性"）。
  - `#8-A` `RefreshShopPresentation` 末尾：每次刷新（打开/购买/刷新）都调用 `InventoryShopWidget->SetPlayerStats(RunSubsystem->CurrentBuild.Stats)`，确保已购属性卡即时反映。
  - `#8-B` `GMAddShards` / `GMSetShards`：若商店已打开（`InventoryShopWidget` 非空），在改完碎片后调用 `RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode())`，即时刷新持有碎片显示。
- **验证 / Verification**：`Build-Editor -FullRebuild` 通过；`validate_project.py` 静态校验通过；用户 PIE 实机确认——商店内购买属性卡后主角属性面板即时更新、GM 改碎片后持有碎片显示即时刷新。本条目待验收（用户已确认"感觉没问题"）。

### #9 — 遭遇结束时倒计时仍显示"剩余 1 秒"就进入了选卡/结算界面

- **现象 / Symptom**：玩家在倒计时还显示"剩余 1 秒"时就已经无法操作、进入了"选择构筑卡牌"界面（截图证实 `CountdownText=1` 与选卡面板同屏）。`[EncounterTimer][END] reason=duration Remaining=0.000` 表明真实计时在 `Remaining=0` 时正确结束，问题不在真实计时早停，而在**显示端**：HUD 用 `CeilToInt(RemainingTime)` 取整，最后整秒（remaining∈(0,1]）都显示"1"，而选卡由 `Director::Tick` 内 `OnEncounterEnded` 触发，发生在 `GameMode::UpdateEncounterHud` 把"0"帧绘制出来**之前**，于是 HUD 冻结在"1"、选卡页弹出覆盖其上。
- **根因 / Root cause**：结束本局 / 弹出选卡的触发时机与"显示倒计时归零"没有对齐——直接隐藏 HUD 只是掩盖症状，正确的行为是**倒计时显示到 0 才停止本局并弹出选卡**。
- **改动 / Changes**（`Source/ReEcho/Private/ReEchoGameMode.cpp` + `Public/ReEchoGameMode.h`）：
  - 撤销原先"遭遇结束立即 `SetVisibility(Collapsed)`"的掩盖式修法。
  - `HandleEncounterEnded`：先 `EncounterHudWidget->SetEncounterStatus(Idx, Total, 0.0f)` 把 HUD 强制刷成"剩余 0 秒"（确保显示到 0 而非冻结在"1"）；幸存分支不再立即弹卡，而是用 `FTimerHandle EncounterEndSettleTimerHandle` + `FTimerDelegate` 延时 `EncounterEndSettleSeconds=0.5f` 调用新增回调 `ProceedToPostEncounterUI()`；死亡分支直接收起 HUD 返回。
  - 新增 `ProceedToPostEncounterUI()`：收起 HUD（`Collapsed`），再按原逻辑 `ShowRestartScreen(false,true)`（Boss 击杀）或 `PrepareEncounterIntermission()`+`ShowTraitCardChoice()`（普通/锻造选卡）。
  - 头文件新增 `void ProceedToPostEncounterUI();`（`UFUNCTION`）与 `FTimerHandle EncounterEndSettleTimerHandle;`。
- **验证 / Verification**：`Build-Editor -Development` 通过；用户 PIE 实机确认——倒计时清晰显示"0"后约半秒才弹出选卡/结算，且选卡时不再残留"CoundownText=1"。本条目待验收。

<!-- 后续修复继续在此处追加 #10、#11…… -->

### #12 — 死亡/胜利「重新开始」进入角色/武器选择界面（不再重载关卡）

- **现象 / Symptom**：玩家死亡（或通关胜利）后弹窗点「重新开始」，会回到主菜单（StartMenu），而不是直接进入角色/武器选择界面；与选卡暂停重载回归（#11 同类）问题同源——都是「重载关卡 → 新 `StartPlay` 无条件 `ShowStartMenu`」。
- **根因 / Root cause**：`ReEchoGameMode.cpp` 的 `HandleRestartRequested()` 用 `UGameplayStatics::OpenLevel(this, CurrentLevelName)` 重开，会销毁并重建整个 World（含 GameMode），于是新的 `StartPlay()` 末尾无条件调用 `ShowStartMenu()`，把玩家送回主菜单，而非预期的 Loadout 选择界面。
- **改动 / Changes**：
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`（`HandleRestartRequested`）：不再 `OpenLevel`，改为就地清理并复用「新游戏」后续流程——`RunSubsystem->DeleteSavedRun()` + 复位 `bBeginSelectedRunRequested`/`bBeginSelectedRunStarted`（否则 `RequestBeginSelectedRun` 会直接 return、无法再次开局）+ `ClearCombatants()`（清残留敌人/Echo）+ `SetGamePaused(false)` + `ShowLoadoutSelection()`。
  - 玩家与战场的最终复位交给 `BeginNextEncounter` 在开局时统一完成（回出生点、满血复活、清残留敌人/Echo、重启 Director），避免就地手动复位的不完整风险。
  - 顺带移除死亡分支的 `QueueEventForNextWorld(Revive)`（不再重载世界，该队列事件永不触发）。
- **验证 / Verification**：Development 增量构建 + `-FullRebuild` 通过；`validate_project.py` 静态校验通过；编辑器实测：死亡→「重新开始」直接进入角色/武器选择界面；胜利→「重新开始」同理；「退出游戏」仍回主菜单（路径未改）；选择后新一局战斗/自动攻击/再次阵亡均正常。

### #13 — 怪物扣血浮动数字去掉负号

- **现象 / Symptom**：怪物受伤时跳出的浮动伤害数字带负号（如 `-123`），体验上希望只显示数值。
- **根因 / Root cause**：`ReEchoDamageNumberActor::SpawnDamageNumber` 用 `FString::Printf(TEXT("-%d"), DisplayDamage)` 拼接负号前缀。
- **改动 / Changes**：
  - `Source/ReEcho/Private/UI/ReEchoDamageNumberActor.cpp`：改为 `FString::Printf(TEXT("%d"), DisplayDamage)`，只显示数值。
  - 该生成路径仅被敌人受伤数字使用（`ReEchoEnemyActor.cpp`、`ReEchoEnemyPresentationComponent.cpp` 调用），不影响玩家受伤表现。
- **验证 / Verification**：Development 增量构建通过；编辑器实测攻击怪物时伤害数字不再带负号。

### #14 — Shipping 商店在笔记本分辨率下显示不完整

- **现象 / Symptom**：同一 Shipping 包在 1920×1080 台式机上可完整显示商店，但在 1366×768 等笔记本分辨率下，商店下半部分和右侧装配室被视口裁切。
- **根因 / Root cause**：`WBP_ReEchoInventoryShopScreen` 的商店、装配室和关闭按钮按 `1920×1080` 固定设计坐标保存，内容最下沿约为 `Y=961`；根 Canvas 只把背景图锚定到视口，没有对固定坐标内容做整体等比缩放。低于设计分辨率时，Canvas 仍按实际视口排版，超出的固定像素内容被裁掉。
- **改动 / Changes**：
  - `UReEchoInventoryShopWidget` 保留 `BackgroundImage` 直接铺满实际视口；其余 WBP 根控件与运行时商店/回响弹层统一迁入固定 `1920×1080` 的 `ResponsiveContentCanvas`。
  - 使用全屏 `ResponsiveContentScale` 按 `ScaleToFit`、`Both` 等比缩放设计面；16:9 笔记本缩小整页，超宽屏由背景延展并保持内容比例与点击坐标一致。
  - 原 WBP Canvas Slot 的锚点、偏移、自动尺寸和 ZOrder 在迁移后原样保留；不改商品、购买、装配、回响或关闭语义。
  - `ReEcho.UI.Shop.LogicBlocks` 与 `ReEcho.UI.Shop.AuthoredLayoutHosts` 增加响应式宿主、设计尺寸、背景分层和控件归属断言。
- **影响面 / Impact**：`MOD-ReEcho` / `AREA-UI`，仅商店/背包共用页面布局；无 Schema、存档、玩法或资产内容变化。
- **验证 / Verification**：`ReEcho.UI.Shop.AuthoredLayoutHosts`、`ReEcho.UI.Shop.LogicBlocks` 自动化通过；Development Editor `-FullRebuild`、`validate_project.py`、`git diff --check` 通过；Win64 Shipping 完整 Build/Cook/Stage/Pak/Archive 成功，补齐项目规定的 31 张运行时 CSV 后，以 `1366×768` 窗口启动并稳定运行 12 秒。最终仍需在问题笔记本上进入商店做人工视觉与点击验收。

### #15 — 商店界面按 P 无法打开暂停菜单

- **现象 / Symptom**：商店/背包页面已经使世界暂停，但按 `P` 不会显示暂停菜单，而是执行商店关闭路径。
- **根因 / Root cause**：玩家的 `PauseMenu` 输入已设置 `bExecuteWhenPaused=true`，但 `AReEchoGameMode::TogglePauseMenu()` 遇到 `InventoryShopWidget` 时调用 `HandleInventoryShopClosed()` 后直接返回，把暂停键错误复用成了商店关闭键。
- **改动 / Changes**：商店打开时按 `P` 改为在更高的 Pause 层打开 `WBP_ReEchoRestart`，不关闭商店、不触发结算推进或回响存储门禁；再次按 `P`/点击继续时关闭 Pause，并恢复商店焦点、菜单能力阻挡和暂停状态。显式记录“Pause 从商店打开”的返回目标，避免普通战斗暂停受影响。
- **影响面 / Impact**：`MOD-ReEcho` / `AREA-UI`，仅暂停与商店页面切换；无商品、存档、战斗、Schema 或资产变化。
- **验证 / Verification**：Development Editor 增量构建与 `-FullRebuild` 通过；`ReEcho.UIManagerSubsystem.PauseOverInventoryShop`、`ReEcho.UIManagerSubsystem.ResetOnTravel` 自动化通过；`validate_project.py`、`git diff --check` 通过。商店内按 `P` 打开 Pause、再次按 `P`/点击继续返回商店仍需人工验收。

### #16 — 怪物实际出生位置与预警位置不一致

- **现象 / Symptom**：策划反馈怪物出现时，动画显示位置与此前预警位置不重合，导致玩家依据预警判断的位置与实际出生位置不符。
- **根因 / Root cause**：`FReEchoSpawnResolver` 将所有候选点的 `Z` 固定为 `50`，预警球直接使用该缓存点；怪物生成后 `AReEchoEnemyActor::AlignToGameplayPlane` 又按玩法平面与各自 `CollisionHalfHeightCm` 重算 Actor 中心高度。当前生产怪物半高并不统一（史莱姆 `90`、兔子 `85`、狐狸 `130`），斜视角投影下预警球中心与最终 Actor/动画脚点产生明显屏幕空间错位。
- **目标 / Acceptance**：预警与生成提交必须消费同一个最终世界位置事实；怪物生成完成后的 Actor 中心位置应与其对应预警位置一致，动画脚点仍贴合玩法平面，不改变现有 XY 解算、距离环、间距、波次时序或策划表数值。
- **实现范围 / Writes**：`Source/ReEcho/Private/ReEchoGameMode.cpp`、聚焦测试、本文档，以及 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；审阅 `MOD-ReEchoEnemies` 与 `MOD-ReEchoPresentation`，若其所有权/契约未变化则只在本条记录“无需修改”。
- **改动 / Changes**：`FReEchoSpawnResolveRequest` 新增显式 `SpawnCenterWorldZ`；SpawnResolver 的正常候选和确定性回退都保留该高度，不再写死 `Z=50`。GameMode 在准备预警批次时从当前 Arena 玩法平面与 `Enemies.CollisionHalfHeightCm` 计算最终 Actor 中心高度，随后预警和生成提交原样复用同一缓存位置。
- **验证 / Verification**：修改的 C++ 已按仓库 `.clang-format` 格式化；`ReEcho.Encounter.DeterministicSpawnResolver` 自动化通过，锁定确定性位置与最终中心高度；Development Editor `-FullRebuild` 通过并刷新预构建包；`validate_project.py` 与 `git diff --check` 通过。第一次完整构建曾因测试期望值误用 `float` 与 UE 5.8 `FVector::Z` 的 `double` 重载冲突而失败，改为 `double` 后重新完整构建通过。
- **文档审阅 / Documentation review**：`MOD-ReEcho` 已补充“预警/提交共享最终 Actor 中心位置”契约；`MOD-ReEchoEnemies` 已审阅、无需修改，因为 EnemyLogic 和 Host 变换所有权未变化；`MOD-ReEchoPresentation` 已审阅、无需修改，因为动画、脚点与表现输入未变化。
- **状态 / Status**：Review。技术门禁已通过；仍需用户在 PIE 中观察史莱姆、兔子和狐狸的预警球与出生动画是否重合，作为视觉验收。

### #17 — 狐狸死亡脚点代码导致 Shipping 无法编译

- **现象 / Symptom**：最新 `origin/main` 执行 Win64 Shipping clean build 时，`ReEchoEnemyPresentationComponent.cpp` 两处调用 `UPaperSprite::GetPivotMode`，UBT 报 `C2039: GetPivotMode 不是 UPaperSprite 的成员`，项目无法进入 Cook 与归档。
- **根因 / Root cause**：狐狸死亡落地修复通过 PaperSprite 的 `PivotMode == Custom` 选择逐帧作者脚点；但 `PivotMode`、`CustomPivotPoint` 与 `GetPivotMode` 都位于 Paper2D 的 `WITH_EDITORONLY_DATA` / `WITH_EDITOR` 区域，Shipping 会裁掉该 API。运行时代码错误依赖了编辑器专用资产元数据。
- **目标 / Acceptance**：Shipping 与 Editor 均从可 Cook 的项目资产契约选择狐狸死亡脚点；不得仅用条件编译跳过 Shipping 表现逻辑。Win64 Shipping 必须完成 clean build、cook、pak、archive 和启动烟测，狐狸与 BadFox 的死亡动画仍使用已制作的逐帧脚点。
- **实现范围 / Writes**：`UReEcho2DCharacterPresentationProfile`、敌人表现组件、狐狸 Profile 配置脚本与资产、聚焦自动化、`MOD-ReEchoPresentation` 和本文档；不改变玩法死亡、碰撞、数值或其他敌人 Profile。
- **改动 / Changes**：Profile 新增会进入 Cook 的 `bUseAuthoredDeathPivot`；狐狸 Profile 显式开启。敌人死亡表现只读取该运行时策略，不再调用 PaperSprite 编辑器 API；未开启的 Profile 继续使用当前帧 Bounds 底边中心。自动化锁定生产狐狸 Profile 必须开启该策略。
- **验证 / Verification**：修改的 C++ 已按仓库 `.clang-format` 格式化；狐狸 Profile 配置脚本通过 UE 命令行幂等执行并记录 `authored_pivot=True`；新增 `ReEcho.Presentation.Animation2D.CookedDeathPivotPolicy` 聚焦自动化通过；Development Editor `-FullRebuild` 通过并刷新预构建包；Win64 Shipping clean build、cook、pak、archive、33 张运行时 CSV 投递及 10 秒启动烟测通过；`validate_project.py` 与 `git diff --check` 通过。既有 `ReEcho.Presentation.Animation2D.AssetProfiles` 仍被无关的 TimeGuard Phase2 资产断言阻塞，本次新增狐狸断言未报错。
- **状态 / Status**：Review。本地技术候选已完成，等待用户验证狐狸与 BadFox 死亡动画脚点；未经用户认可不提交、不推送。

### #18 — 出生预警消失后没有生成怪物

- **现象 / Symptom**：部分红色出生预警球正常显示并消失，但对应位置没有生成怪物。
- **根因 / Root cause**：GameMode 在预警阶段为完整配表批次显示红球，却到提交阶段才按 `ActiveUnitLimit` 裁剪。同一波多个角色批次先后提交时，较早批次占满上限，后续批次已显示的预警位置会被裁掉。现有运行日志已记录 `Ranged truncated 5->0` 等直接证据。
- **目标 / Acceptance**：每个可见出生预警都必须在同一锁定位置提交一个怪物；单位上限只能阻止预警位置被创建，不能在红球显示后取消该位置。不改变配表数量、单位上限、波次时序、位置解算或 Stage 连续性。
- **实现范围 / Writes**：`ReEchoEncounterRuntime.*`、`ReEchoGameMode.cpp`、`ReEchoEncounterRuntimeTests.cpp`、本文档及 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`。
- **改动 / Changes**：预警阶段按存活单位和全部待提交批次统一预留容量，只解析并显示实际获准的红球；提交阶段完整消费预留位置，不再二次裁剪。生产敌人生成显式使用 `AlwaysSpawn`，确保确定性解算后的承诺位置不会被 Actor 默认碰撞策略静默拒绝；失败仍输出具名错误日志。
- **影响面 / Impact**：`MOD-ReEcho` / `AREA-Encounter`；只收紧预警与出生提交的一致性，无 Schema、存档格式、配表、敌人 AI、伤害或表现资产变化。`MOD-ReEchoEnemies` 与 `MOD-ReEchoPresentation` 已审阅，无所有权或契约变化，无需修改。
- **验证 / Verification**：修改的 C++ 已按仓库 `.clang-format` 格式化；组合 `origin/main@e34f8f5f` 后 Development Editor `-FullRebuild` 94/94 通过并刷新 7 个预构建模块；`ReEcho.Encounter` 自动化 4/4 通过，其中新增 `SpawnWarningCapacityReservation` 锁定跨角色批次预留；`validate_project.py`、预构建一致性和 `git diff --check` 通过。曾额外生成 Shipping 候选并通过 Build/Cook/Stage/Pak/Archive 与 10 秒烟测；后续仅在用户明确要求时打包。用户已手测确认红球与怪物提交一致。
- **状态 / Status**：Closed。用户已完成手测验收并授权合并发布。

### #19 — Boss 不再固定在场景上方出生

- **现象 / Symptom**：策划反馈 Boss 每局固定刷新在场景上方，缺少与玩家/回响位置关联的变化。
- **根因 / Root cause**：`AReEchoGameMode::SpawnScheduledBatch` 对 `EnemyRole=Boss` 单独使用硬编码 `FVector(800,0,50)` 并提前返回，完全绕过 `SpawnProfiles`、双锚 SpawnResolver、最终碰撞中心高度和预警位置承诺链。
- **目标 / Acceptance**：Boss 使用与普通怪相同的表驱动 Warning/Commit、双锚位置解析和位置承诺；`EncounterWaves.BossEnemyId` 继续拥有 Boss 身份，Boss 不计入单位上限时不得挤占普通增援容量；删除固定世界坐标，不改变 Boss AI、技能、数值、阶段或关卡结束规则。
- **实现范围 / Writes**：`ReEchoEncounterRuntime.*`、`ReEchoGameMode.cpp`、`ReEchoEncounterRuntimeTests.cpp`、Encounter 权威工作簿与 `spawn_profiles.csv`、Schema/项目校验、本文档及 `MOD-ReEcho`。现有 `ReEcho-plan63` 工作树含未提交 UI/素材修改且严重落后 main，本条按用户指令在独立 `ReEcho-plan63-boss-spawn-location-final` / `plan/63-boss-spawn-location-final` 实施，不触碰原工作树。
- **改动 / Changes**：新增 `Spawn.Boss`（`M_SHEEP`、700–950 距离环、220 间距）；WaveScheduler 为 Boss 生成同一位置事实的 Warning/Commit；GameMode 取消 Boss 特例并通过 SpawnResolver 预留、提交。`BossCountsTowardUnitLimit=false` 时 Boss 仍获得自身位置，但不进入普通怪容量计算。
- **验证 / Verification**：工作簿由 artifact-tool 编辑并完成六个工作表的前后渲染、目标表检查和公式错误扫描；最终文件保留原工作表保护、解锁数据行、表范围及扩展后的数据验证。C++ 聚焦测试锁定 Boss 事件、动态距离环、旧固定坐标消失和单位上限豁免。Development Editor `-FullRebuild` 97/97 通过并刷新 7 个预构建模块（源码指纹 `b768fa629799`）；`ReEcho.Encounter` 自动化 4/4 通过；XLSX/CSV 同步测试 18/18、生产数据同步检查、`validate_project.py`、预构建一致性与 `git diff --check` 均通过。本条未宣称 PIE 人工视觉验收。
- **状态 / Status**：Closed。按用户指令完成技术验收并提交发布。

### #20 — 玩家受伤后短暂忽略敌人碰撞挤压

- **现象 / Symptom**：玩家被敌人包围时，受伤后仍持续被多个 Pawn 碰撞体阻挡，难以从包围中移动脱身，可能被挤压连续击杀。
- **目标 / Acceptance**：每次真实扣血后，玩家根碰撞在 1 秒内忽略 `Pawn` 移动碰撞；重复受伤从最新一次重新计时。该窗口只改变物理挤压，不提供伤害无敌，不影响场景墙体或敌人伤害判定；致死时立即恢复碰撞。
- **实现范围 / Writes**：`ReEchoPlayerPawn.*`、`ReEchoPlayerCollisionTests.cpp`、本文档及 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`。影响 `MOD-ReEcho / AREA-Player`；`MOD-ReEchoCombat` 只提供既有 Hurt 事实，已审阅、无需修改；`MOD-ReEchoEnemies` 的 AI/伤害/碰撞所有权不变，已审阅、无需修改。
- **改动 / Changes**：Player Pawn 订阅自身类型化 `OnHurt`；首次真实扣血时保存 Blueprint 作者的 `ECC_Pawn` 响应并切换为 `Ignore`，在 Pawn Tick 中推进可重触发的一秒剩余时间。Blocked/零伤害不触发，致死和 EndPlay 立即清理窗口并恢复响应；暂停时窗口与玩法时间一起停止推进。
- **验证 / Verification**：修改的 C++ 已按仓库 `.clang-format` 格式化；Development Editor 构建 96/96 通过并刷新 7 个预构建模块（源码指纹 `177f9e0297fd`）；`ReEcho.Player.HurtCollisionIgnore` 自动化 1/1 通过，锁定窗口内仍可继续扣血、重复受伤顺延计时及一秒后恢复作者响应；`validate_project.py`、预构建一致性与 `git diff --check` 通过。用户授权按当前完整候选发布。
- **文档审阅 / Documentation review**：`MOD-ReEcho` 已更新玩家 Hurt 后短时 Pawn 碰撞忽略契约；`MOD-ReEchoCombat` 已审阅、无需修改，因为 Hurt 事实与伤害结算所有权不变；`MOD-ReEchoEnemies` 已审阅、无需为本项修改，因为敌人 AI、命中和碰撞所有权不变；`ARCHITECTURE.md` 与 `CODEBASE_MAP/README.md` 已审阅、无需修改，因为模块拓扑和路由标识未变化。
- **状态 / Status**：Closed。用户授权将本工作树完整候选发布并清理。

### #21 — 兔子站定连发相邻弹距调整为约 70cm

- **需求 / Need**：兔子站定直线连发的相邻子弹飞行间距调整为约 `70cm`。
- **换算 / Calculation**：`M_RABBIT_RangedBurst` 当前为 4 发；未填写固定弹速，因此运行时按 `MaxRangeCm / CooldownSeconds = 1000 / 1.4 = 714.2857cm/s` 推导。连发间隔由 `ActiveSeconds / (ProjectileCount - 1)` 决定，故 `ActiveSeconds = 70 / 714.2857 × 3 = 0.294s`。
- **改动 / Changes**：权威工作簿 `Design/Data/ReEchoEnemyData.xlsx` 的 `EnemyAbilities!H11`（`M_RABBIT_RangedBurst.ActiveSeconds`）由 `0.1` 改为 `0.294`，并同步生成 `Content/Data/enemy_abilities.csv`；未改移动散射技能或其他敌人能力。
- **验证 / Verification**：工作簿目标单元格、样式与公式错误扫描通过，五个工作表渲染复核通过；保留原工作表保护、数据验证及其余 XLSX 包内容；XLSX/CSV 同步及项目静态校验通过。用户已在 PIE 确认当前约 `70cm` 相邻弹距符合预期；`Design/Data/ReEchoEnemyData使用说明.md` 已补充策划填表公式、当前示例及常见误填提醒。
- **状态 / Status**：Closed。用户已完成手感验收并授权发布。

### #22 — 兔子站定连发与移动散射使用相同单发尺寸

- **现象 / Symptom**：兔子移动散射为 3 发、站定连发为 4 发；两者 `RadiusCm` 都是 `100`，旧逻辑却按各自 `ProjectileCount` 平分半径，导致散射单发半径为 `33.33cm`、连发仅 `25cm`，且表现直径跟随碰撞半径，连发视觉和判定都小约 25%。
- **目标 / Acceptance**：兔子每颗子弹尺寸固定，不随同一技能一次发射的数量改变；以现有移动散射为基准，站定连发同样使用 `33.33cm` 碰撞半径和约 `66.67cm` 视觉直径。不改变羊 Boss 的投射物分配规则。
- **改动 / Changes**：兔子 Ranged 投射物统一按原始 3 发移动散射基准解析单发半径，不再按当前技能发数重新平分；羊 Boss 仍保留显式按发数解析的既有路径。聚焦测试新增移动/站定两种兔子技能半径相等断言，并将站定连发时序断言改为读取配表间隔。
- **验证 / Verification**：修改的 C++ 已按仓库 `.clang-format` 格式化；Development Editor 增量构建通过并刷新 7 个预构建模块（源码指纹 `e649ef829fec`）；`ReEcho.Enemies.Host` 自动化 5/5 通过，其中 `RabbitProjectilePipeline` 锁定移动/站定两种技能单球半径相等，`SheepProjectilePipeline` 同时回归羊 Boss 路径；`validate_project.py`、预构建一致性与 `git diff --check` 通过。
- **文档审阅 / Documentation review**：`MOD-ReEcho` 与 `MOD-ReEchoEnemies` 已更新兔子双技能轨迹、固定单发半径和 EnemyHost 所有权；`MOD-ReEchoVFX` 已更新逐球事件数量与固定视觉尺寸契约；`ARCHITECTURE.md` 与 `CODEBASE_MAP/README.md` 已审阅、无需修改，因为模块拓扑、依赖方向和稳定路由标识未变化。
- **状态 / Status**：Closed。用户已确认两种兔子技能的单发尺寸一致并授权发布。
- **发布集成 / Release integration**：取得 `main-publish-lock` 后合入 `origin/main@402b6d22`；传入的伤害数字、元素反应来源、狐狸冲撞、首波预警和 Plan116 与本候选无源码/数据逻辑冲突，只有精选预构建包发生预期二进制冲突并由最终组合源码完整重生。Development Editor `-FullRebuild` 100/100 通过，精选包源码指纹 `aae735c39d73`；最终组合上的 `ReEcho.Player.HurtCollisionIgnore` 1/1、`ReEcho.Enemies.Host` 5/5、XLSX 同步测试 18/18、生产数据一致性、项目校验、预构建一致性和 `git diff --check` 全部通过。

### #23 — `GMGotoBoss` 进入第八关后立即获胜

- **现象 / Symptom**：在正常遭遇中执行 `GMGotoBoss`，界面进入第八关后未与羊 Boss 战斗即直接显示胜利。
- **根因 / Root cause**：Plan115 将零秒首波的 Commit 延后到完整 `WarningLeadSeconds`；第八关 Boss 因此先进入预警 Pending。实测日志证明 `GMGotoBoss` 后 Boss 的 Commit 时间为 `0.9s`，但旧胜利门在 `0.0s` 仅因 Roster 中暂无存活 Boss 就结束遭遇，将“尚未生成”误判为“已被击败”，导致后续 Commit 永远无法执行。
- **改动 / Changes**：GameMode 新增遭遇内权威状态 `bBossSuccessfullySpawnedThisEncounter`，仅当 Boss Host 完整生成并配置成功后置真；新遭遇清零，读档时从成功恢复的 Boss 重建。Boss 胜利门现在要求“本关 Boss 曾成功生成且当前无存活 Boss”；预警 Pending 或生成失败均不判胜。保留 `[BossVictoryTrace]` 供本轮人工验证。
- **验收 / Acceptance**：`GMGotoBoss` 进入第八关后先显示 Boss 出生预警，Boss 成功生成后关卡继续；只有击杀已成功生成的 Boss 才显示胜利。Boss 生成失败时输出错误但不显示胜利。
- **验证 / Verification**：取得 `main-publish-lock` 后合入 `origin/main@8017c6d4`，传入的卡牌与敌人伤害诊断改动和本修复无文本逻辑冲突；7 个精选预构建模块由最终组合源码执行 Development Editor `-FullRebuild` 94/94 成功重生（源码指纹 `230b07c834f2`）。最终组合上的 `ReEcho.GameMode` 2/2、`ReEcho.Encounter` 4/4、`ReEcho.Run.FinalBossRequiresKill` 1/1 自动化通过；`validate_project.py`、预构建一致性与 `git diff --check` 通过。
- **状态 / Status**：Closed。用户已明确授权推送并合入远端主分支。

### #24 — Esc 与 P 共用暂停菜单动作

- **需求 / Need**：`Esc` 应与 `P` 产生完全相同的暂停菜单效果，包括打开暂停、再次按键恢复，以及复用暂停菜单对设置、属性页和商店覆盖层的既有处理。
- **根因 / Root cause**：玩家代码已经把 `PauseMenu` Action 统一绑定到 `AReEchoGameMode::TogglePauseMenu()`，并允许暂停期间继续响应；但 `Config/DefaultInput.ini` 只把 `P` 映射到该 Action，缺少 `Escape` 映射，与 `MOD-ReEcho` 已声明的“Esc 进入暂停层”契约不一致。
- **改动 / Changes**：仅在 `DefaultInput.ini` 为同一个 `PauseMenu` Action 增加 `Escape`；不新增输入回调，不修改暂停、恢复、菜单焦点、商店覆盖或存档逻辑。影响 `MOD-ReEcho / AREA-Player / AREA-UI`。
- **验证 / Verification**：`python scripts/validate_project.py`、`git diff --check` 通过；额外静态断言确认 `PauseMenu` 恰有 `P` 与 `Escape` 两条目标映射。取得发布锁后的最终候选按程序发布门禁完成 Development Editor `-FullRebuild`（95/95），精选预构建包刷新为源码指纹 `6ee071709d39`；仍未宣称 PIE 人工按键测试，发布后可继续确认战斗中 `P`/`Esc` 均可打开和关闭暂停菜单，以及商店页面按 `Esc` 时沿用 `P` 的暂停层覆盖行为。
- **文档审阅 / Documentation review**：`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已审阅且无需修改，因为正文已准确声明 `Esc` 进入暂停层；`MOD-ReEchoUI.md` 已审阅且无需修改，因为屏幕层级、焦点和生命周期契约未改变；`ARCHITECTURE.md` 与 `CODEBASE_MAP/README.md` 无模块拓扑或稳定路由变化，无需修改。
- **状态 / Status**：Review。客观发布门禁已通过并按用户授权发布；PIE 人工按键确认尚未执行。

### #25 — 羊 Boss 第一条血耗尽后卡在原地且持续免伤诊断

- **现象 / Symptom**：羊 Boss 第一条血耗尽后停在原地，不移动也不攻击；玩家后续攻击无法造成伤害。
- **初步判断 / Initial assessment**：该组合现象与 Boss 长时间停留在 `Transforming` 完全一致。致命伤拦截会把羊保留为可存活状态，转换期间 Logic 不产出移动/攻击，而 Host 的入伤修正会把后续伤害改为 0；正常应在 `TransformSeconds` 后进入 Phase2 并回满第二条血。当前最可疑路径是 Host 仅在 `!bStunned` 时推进 EnemyLogic，导致清空血条时附带或持续刷新的眩晕冻结 `PhaseTransitionRemainingSeconds`；次要候选为 Encounter suspension、World Pause 或 Actor Tick 停止。
- **诊断改动 / Diagnostics**：`ReEchoEnemyActor` 增加 `[SheepPhase2Trace]` 日志，覆盖致命伤拦截的接受/拒绝原因、Transform 开始与完成事件、Transform 期间每秒心跳（Delta、World Pause、Host Tick、Encounter suspension、Born gate、卡牌/Status 眩晕、剩余转换时间、HP/MaxHP、可受击状态）及 Phase2 回血结果；既有 `[EnemyDamageGate]` 同步补充暂停、Tick、Suspension 与眩晕门字段。只增加观测，不修改阶段、免伤、眩晕、AI、伤害或回血行为。
- **修复 / Fix**：按用户确认的玩法规则，`M_SHEEP` 在 Host 配置边界启用 Combat 眩晕免疫。Combat 权威入口拒绝所有 `Z_Vertigo` 并在启用免疫时清除活动状态、GAS 标签及存档恢复残留；Host 卡牌眩晕在写入独立计时前查询同一免疫策略。其他敌人的眩晕行为不变。
- **影响面 / Impact**：`MOD-ReEcho / AREA-Enemies / AREA-AbilityCombat`；诊断日志与卡牌门位于 Enemy Host，通用免疫策略位于 `MOD-ReEchoCombat` 的 Combatant 权威入口，EnemyLogic 阶段算法未改。
- **测试方式 / Repro**：进入 Boss 关并打空第一条血，异常出现后停留至少 3 秒，再继续攻击数次；退出 PIE 后读取会话日志并过滤 `SheepPhase2Trace|EnemyDamageGate`。
- **文档审阅 / Documentation review**：`MOD-ReEchoEnemies`、`MOD-ReEchoCombat` 与 `MOD-ReEcho` 已同步羊全来源眩晕免疫的 Host/Combat 边界；同时把既有羊生命说明改为读取当前 Phase 定义，避免文档固化过期表值。`ARCHITECTURE.md` 与 `CODEBASE_MAP/README.md` 无模块拓扑或稳定路由变化，无需修改。
- **验证 / Verification**：取得发布锁并合并 `origin/main@ae548af1` 后，最终集成候选完成 Development Editor `-FullRebuild`（104/104），精选 Win64 Editor 预构建包刷新为源码指纹 `31d717a1509a`；`ReEcho.Combat.Runes.TimedStatusAndIndependentStacks` 与 `ReEcho.Enemies.Host.SheepStunImmunity` 在集成版本上均找到 1 项并返回 `Result={Success}` / `EXIT CODE: 0`。实现阶段自动化曾准确捕获短路表达式未删除状态表残留，拆分计时器与状态清理后复跑通过。最终静态、预构建与 LFS 发布门禁见发布提交记录。
- **状态 / Status**：Review。用户已授权保留诊断日志并直接发布远端主分支；PIE 二阶段与持续战斗人工复验仍待后续执行。

### #26 — Boss 免疫受击硬直与 Echo 嘲讽

- **需求 / Need**：所有 Boss 免疫玩家攻击造成的受击硬直/击退，并在 Echo 嘲讽规则生效时继续以玩家为目标；普通敌人的受击和嘲讽行为保持不变。
- **实现范围 / Writes**：`ReEchoEnemyLogicComponent.cpp`、`ReEchoEnemyActor.*`、EnemyLogic/Host 聚焦自动化、本文档及 `MOD-ReEchoEnemies`、`MOD-ReEcho`。影响 `MOD-ReEchoEnemies` 的 Hurt 权威入口和 `MOD-ReEcho / AREA-Enemies` 的世界目标解析；不改变伤害数值、生命、眩晕/减速/元素状态、卡牌规则、数据 Schema、XLSX/CSV 或存档格式。
- **改动 / Changes**：EnemyLogic 的通用 Boss Archetype 在权威 `NotifyHurt` 入口忽略硬直和击退，不取消当前技能；恢复旧快照时清除 Boss 可能残留的硬直时间、击退速度与 `HitReaction` phase。Enemy Host 把 AI Sense 与已出手投射物的 Echo 选择统一到同一目标解析函数，普通敌人在嘲讽开启时继续选择最近存活 Echo，Boss 始终保留玩家目标。
- **验证 / Verification**：Visual Studio LLVM `clang-format` 已按仓库样式格式化 5 个改动 C++ 文件；取得发布锁并合并 `origin/main@1273f930` 后，最终集成候选完成 Development Editor `-FullRebuild`（95/95），刷新 7 个精选预构建模块（源码指纹 `7567dd78cd8b`）。`ReEcho.Enemies.Boss.HitReactionImmunity` 与 `ReEcho.Enemies.Host.BossEchoTauntImmunity` 均找到 1 项并返回 `Result={Success}` / `EXIT CODE: 0`；`validate_project.py`、预构建一致性、LFS checkout/status/fsck 与 `git diff --check` 均通过。
- **文档审阅 / Documentation review**：`MOD-ReEchoEnemies` 与 `MOD-ReEcho` 已同步 Boss 控制免疫及 Host 目标选择契约；`MOD-ReEchoCombat` 已审阅、无需修改，因为最终伤害、生命与状态权威未变；`ARCHITECTURE.md` 与 `CODEBASE_MAP/README.md` 已审阅、无需修改，因为模块拓扑、依赖与稳定路由标识未变化。
- **状态 / Status**：Review。技术候选与客观门禁已完成；用户已明确授权按规则发布，PIE 的 Boss 连续受击与 Echo 嘲讽表现验收作为 `PendingFollowUp` 保留。

### #27 — 终局重新开始时完整重建 World

- **现象 / Symptom**：死亡或胜利结算后点击「重新开始」采用同一 World 就地清理；`ClearCombatants()` 只销毁敌人、Echo 和掉落物，未销毁独立的玩家/Echo 弹道，因此暂停时冻结的旧局弹道可能在新一局解除暂停后继续移动并命中新目标。
- **根因 / Root cause**：#12 为避免 `OpenLevel` 后 `StartPlay()` 固定显示主菜单，取消了原有 World 重载，并假定 `BeginNextEncounter` 会完整复位战场。该假定无法覆盖所有 World 级瞬态 Actor，且后续新增系统会持续扩大人工清理名单。
- **目标 / Acceptance**：结算界面继续暂停当前 World；只有点击「重新开始」时才删除当前存档并完整重载当前关卡。新 World 必须直接进入角色/武器选择，普通启动仍进入主菜单；跨 World 启动意图只能消费一次，旧局 Actor、计时器和弹道不得进入新局。
- **改动 / Changes**：UI Flow Coordinator 以 GameInstance 生命周期保存一次性 Loadout 旅行意图；终局重开在 `OpenLevel` 前写入意图并恢复跨 World 复活音频队列。新 GameMode 的 `StartPlay()` 消费意图后选择 Loadout，否则保持主菜单路径。删除 #12 的就地 `ClearCombatants` 和本 GameMode 标志复位。
- **影响面 / Impact**：`MOD-ReEcho / AREA-UI / AREA-Encounter`；不改变结算内容、存档格式、角色/武器选择、普通启动、退出到主菜单或遭遇内局间连续性。
- **验证 / Verification**：实现完成后的 Development Editor `-FullRebuild` 98/98 通过；撤回两处无关排版后，本地验收候选再以增量构建 4/4 通过。取得发布锁并合入 `origin/main@4b6c57d3` 后，最终组合候选完成 Development Editor `-FullRebuild`（95/95），刷新 7 个精选预构建模块（源码指纹 `0e7fa61eefdb`）。`ReEcho.UIFlow.TerminalRestartTravelRoute` 找到 1 项并以 `Result={Success}` 完成；`validate_project.py`、LFS checkout/status/fsck 与 `git diff --check` 均通过。传入的 Boss 控制免疫和暴击伤害数字与启动/重开链路无逻辑冲突。
- **文档审阅 / Documentation review**：`MOD-ReEcho` 同步终局重载与一次性 UI 旅行路由；`ARCHITECTURE.md`、`CODEBASE_MAP/README.md` 无模块拓扑或稳定标识变化，已审阅、无需修改。
- **状态 / Status**：Closed。用户已完成人工验收并授权发布，最终组合门禁已通过。
