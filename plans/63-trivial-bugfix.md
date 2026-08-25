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
