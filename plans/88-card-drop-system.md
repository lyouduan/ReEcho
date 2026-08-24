# Plan 88 - 程序 - 按关次配置的卡牌投放系统

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner 与 Executor）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 最终集成基线：`origin/main` @ `e3a8701a66b1abe664736993a6e6c8f798baac3e`。
- 本地实现方式（可选，仅作交接说明）：独立 worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan88-card-drop-system`，分支 `plan/88-card-drop-system`。
- 依赖 / 阻塞：依赖 Plan47 的 `MOD-ReEchoCards` 目录/资格过滤，并与 Plan85 的商店所有权排除保持兼容。2026-08-24 用户锁定：商店有投放时固定显示 3 个卡牌槽，三个槽均从当前行 `ShopTiers` 指定 Tier 的合并牌池抽取；最新资格规则为1级卡可重复投放/叠加，2、3级卡及已获得武器、符文不得再次投放。该决定覆盖此前“所有已获得卡一律排除”及 Plan67“一配置 Tier 对应一槽”的旧解释。Plan87 已进入最终基线；Forge 已删除，本 Plan 只保留普通卡牌投放路由。
- Writes:
  - `plans/88-card-drop-system.md`
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`
  - `Source/ReEcho/Public/Run/ReEchoShopCatalog.h`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Public/UI/ReEchoInventoryShopWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoInventoryShopWidget.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoTraitTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoCharacterPromotionTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopLogicBlockTests.cpp`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardTypes.h`
  - `Source/ReEchoCards/Private/Cards/ReEchoCardRuntime.cpp`
  - `Source/ReEchoCards/Private/Tests/ReEchoCardRuntimeTests.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
- Stable Reads:
  - `策划数据源/【开普勒】回响数值与构筑体系.xlsx` 的可见 `投放系统!A1:C10`
  - `Design/Data/ReEchoData.xlsx` 的 `经济系统` 投放数据
  - `Content/Data/shop_drop_levels.csv`
  - `Content/Data/cards.csv`
  - `Content/Data/csv_schema.csv`
  - `Source/ReEcho/{Public,Private}/Data/ReEchoCsvDataRegistry.*`
  - `Source/ReEcho/Private/Data/ReEchoWeaponCsvReader.cpp`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardRuntime.h`
  - `Source/ReEcho/{Public,Private}/UI/ReEchoTraitCardChoiceWidget.*`
  - `plans/47-card-build-runtime-module.md`
  - `plans/67-shop-drop-table-overhaul.md`
  - `plans/85-shop-ownership.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：继续使用本局固定卡牌目录、现有 `OwnedCardIds`、`CanOffer` 和确定性种子。为保证同一商店页购买后不暗中重摇，在已随 BuildSnapshot 保存的卡牌运行态中增加当前关次/刷新序号及 3 个报价 CardId；不提升 SaveVersion，旧存档缺失字段按默认空页在首次打开时生成。商店购买和刷新入口保持不变；卡牌区为“有配置时固定 3 槽、牌池为 `ShopTiers` 合并集合”，并把“1级可重复、2/3级已获得即不可再投放”集中到 Cards 资格判断。WBP 继续消费现有扁平报价，不要求 UI 资产重做。
- 明确排除：不修改 39 张构筑卡的定义与效果，不调整卡牌数值，不重做商店 UI/资产，不修改武器/符文系统，不实现新的卡牌刷新机制，不修改策划源工作簿；如实现发现生产 XLSX/CSV 与本 Plan 锁定矩阵漂移，停止并回到策划真源审计，不静默覆表。

## 锁定目标

战斗结束后的免费卡牌组与商店卡牌槽统一由当前 `EncounterIndex` 对应的 `shop_drop_levels` 行决定，不再由 GameMode 固定写死全牌池三选一。

| 关次 | 免费投放数量与牌池 | 商店卡牌槽与牌池 |
|---|---|---|
| 1 | 0 组 | 0 槽 |
| 2 | 1 组，Tier 2 三选一 | 3 槽，均从 Tier 1 抽取 |
| 3 | 1 组，Tier 3 三选一 | 3 槽，均从 Tier 1 抽取 |
| 4 | 1 组，Tier 1 三选一 | 3 槽，均从 Tier 2 + Tier 3 合并牌池抽取 |
| 5 | 1 组，Tier 2 三选一 | 0 槽 |
| 6 | 1 组，Tier 3 三选一 | 3 槽，均从 Tier 1 + Tier 2 合并牌池抽取 |
| 7 | 1 组，Tier 2 三选一 | 3 槽，均从 Tier 1 + Tier 3 合并牌池抽取 |
| 8 | 0 组 | 0 槽 |

- `FreeTier` 为空表示本关战后不进入普通卡牌选择，直接进入战后商店/回响处理；有值时只从该 Tier 的 `Trait` 可投放牌池生成 3 张且选择 1 张。
- 免费与商店候选继续遵守 `MOD-ReEchoCards` 的 Enabled/Offerable、冲突和持有状态资格规则；1级卡即使已存在于 `OwnedCardIds` 仍可再次进入后续候选并叠加，2、3级卡一旦获得即不可再次进入候选。不得在 Run 或 UI 复制该规则。
- 所有角色遵循同一关次投放矩阵；勇者不再进入 Forge 阶段，有 `FreeTier` 时进入普通三选一，否则直接进入商店。
- 贤者每第 4 次普通选择触发的额外选择沿用当前关 `FreeTier`，且“不投放”关不会凭空创建普通选择或累计次数。
- 第 8 关 Boss 结算保持无免费选卡、无商店卡牌投放并进入胜利结算。
- 缺少关次行、Tier 非法或候选不足 3 张时不得回退到全等级牌池；数据校验应尽早报错，运行时记录明确错误并安全进入商店/后续结算，不能卡死流程。
- 商店有 `ShopTiers` 时固定生成 3 个互不重复卡牌槽，每个槽的候选池都是该行全部 Tier 的合并集合；空配置生成 0 槽。必须按真实 `EncounterIndex` 查表，不得把第 7/8 关钳制成第 6 关。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoCards`、`MOD-ReEchoUI`（直接修改）；`AREA-Data`、`AREA-Run`、`AREA-Cards`、`AREA-UI`（行为链受影响）。
- 对应模块文档：`MOD-ReEcho.md` 记录 Run 的固定三槽合并牌池；`MOD-ReEchoCards.md` 记录1级重复、2/3级持有排除的统一资格；`MOD-ReEchoUI.md` 关闭前具名审阅。
- 设计意图：让生产表已存在的 `FreeTier` 成为战后普通卡牌投放的唯一权威，并把“是否显示选卡、从哪个 Tier 抽取、选卡后进入哪里”集中在 Run 阶段策略中；GameMode 只按 Run phase 路由 UI，Cards 只负责牌池资格与授予。
- 权威状态与依赖：`Design/Data/ReEchoData.xlsx` → `shop_drop_levels.csv` → `FReEchoCsvDataSnapshot::ShopDropLevels` 是关次投放配置链；Run 拥有阶段与本局事务；Cards 拥有资格/牌池/授予；GameMode/UI 不保存第二份关次矩阵。状态所有者不变，不增加跨 Runtime Module 依赖。
- 决策记录：
  - 复用现有 `FReEchoCsvShopDropLevelRow { EncounterIndex, FreeTier, ShopTiers }`，不新建重复表或硬编码八关数组；当前数据同步 `--check` 已证明生产 XLSX 与 CSV 一致。
  - 免费投放数量由 `FreeTier` 的“空/有值”表达为 0/1 组，每组固定三选一；不为此把 Widget 改成任意数量候选。
  - 商店投放按用户 2026-08-24 的最新决定覆盖 Plan67 旧解释：`ShopTiers` 定义三个固定槽共同使用的合并牌池，而不是 Tier 数量或槽位数量；三个报价保持确定性且互不重复。
  - 持有状态资格属于 Cards 的统一契约：`CanOffer` 允许已拥有的1级卡再次投放并叠加，拒绝已拥有的2、3级卡；Run 的免费与商店路径复用该入口。
  - Run 提供单一的当前关免费投放解析/阶段推进入口，由 `CompleteEncounter` 使用；旧 Forge 存档阶段仍按主分支契约迁移为普通 `CardChoice`。
  - 确定性仍基于现有 `TraitOfferSeed + EncounterIndex + OwnedCardIds`；只把 Tier 作为候选池过滤条件，不引入时间或 UI 状态随机源。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：关闭前审阅；预期无 Runtime Module 拓扑或依赖方向变化，无需修改。
  - `shared/CODEBASE_MAP/README.md`：关闭前审阅；预期无新架构标识或阅读路由，无需修改。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：更新 `AREA-Data` / `AREA-Run` 的投放配置消费与阶段路由。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`：审阅 Tier 牌池与资格职责是否仍准确。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：审阅固定三候选 Widget 与 GameMode 路由是否仍准确。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：已审阅；Runtime Module 拓扑、依赖方向和权威状态所有者未变，无需修改。
  - `README.md`：已审阅；未增加或移动架构标识、模块和阅读路由，无需修改。
  - `MOD-ReEcho.md`：已更新 `AREA-Data` 的投放配置发布边界、`AREA-Run` 的逐关阶段/Tier 契约及 1..8 关测试覆盖。
  - `MOD-ReEchoCards.md`：已更新统一资格契约；1级卡可在免费或商店后续投放中重复并叠加，2、3级卡一旦获得即排除。
  - `MOD-ReEchoUI.md`：已审阅；免费选卡仍使用现有三候选 Widget，UI 只按 Run phase 被路由且不持有关次矩阵，无需修改。

## 锁定验收

- [x] 自动化逐行覆盖 1..8 关，证明免费投放组数/Tier 与商店固定 0/3 槽及合并 Tier 牌池精确匹配锁定矩阵。
- [x] 第 1 关结束不出现普通三选一，但仍进入战后商店和回响处理；第 2～7 关普通三选一的 3 张卡全部属于配置 Tier；第 8 关无普通选卡并正常结算。
- [x] 第 7 关商店精确生成 3 个卡牌槽，三个槽均只来自 Tier 1 + Tier 3 合并牌池，不再错误复用第 6 关 Tier 1 + Tier 2。
- [x] 勇者与其他角色遵循相同关次矩阵：第 1 关直接进入商店，有 `FreeTier` 的关次进入正确 Tier 的普通三选一，且不会出现 Forge 页面。
- [x] 贤者额外选择仍按每 4 次普通选择触发，额外候选与触发关的 `FreeTier` 一致。
- [x] 免费与商店后续投放允许已获得1级卡再次出现并叠加，排除所有已获得2、3级卡；同一组内没有重复 CardId。
- [x] 商店购买构筑卡后不重摇报价，并在同一商店会话内立即把真实卡牌图标显示到右侧卡牌槽。
- [x] 同一基线/种子/持有卡状态生成相同候选；非法或不足牌池不会回退全等级，也不会卡死战后流程。
- [x] `scripts\data\sync_xlsx_to_csv.py --check`、聚焦自动化、完整 Editor 构建、`python scripts/validate_project.py`、`git diff --check` 通过。
- [ ] 人工 PIE 验收以上 1、2、4、7、8 关的界面出现/跳过、卡牌等级与商店衔接。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：最初实现基于 `b8836ed1`；发布前先按用户确认变基到 `a4a72b98`，吸收 Plan87 角色能力数据化/Forge 删除、商店暂停与响应式布局；推送前远端又加入 Plan71 Echo Blueprint/空间表现，二次确认后组合适配到最终 `origin/main@e3a8701a`。
- 引擎/构建可用性：变基前完整构建与聚焦回归已通过；变基后的最终候选必须重新执行完整构建与全部相关自动化，旧二进制证据不沿用。
- 现有聚焦测试结果：变基前证据仅作历史记录；最终发布以本节后续追加的变基后结果为准。
- 共享契约 / 难合并资源风险：本 Plan 将编辑近期商店/Run 高频文件，尤其 `ReEchoRunSubsystem.cpp`、`ReEchoGameMode.cpp`、`ReEchoShopTests.cpp`；发布前必须再次 fetch 并审计 Plan85 之后的新提交。工作簿与 CSV 当前无需写入，避免与策划表二进制变更产生无意义冲突。
- 基线损坏时的停止条件：远端在 Plan-only 发布或实现集成前再次前进；策划源可见 `投放系统!A3:C10` 与生产 `shop_drop_levels.csv` 漂移；聚焦基线或预构建校验重新失败。命中任一条件时停止并重新审计，不静默合并或回退。

## 实现提纲

1. 在 Run 层增加当前关 `shop_drop_levels` 解析与免费投放策略的单一入口，返回“无普通投放”或有效 `FreeTier`；禁止默认全牌池回退。
2. `CompleteEncounter` 复用该策略推进到 `CardChoice` / `Planning`；不恢复已由 Plan87 删除的 Forge phase/API/UI。
3. `GenerateTraitCardOffers` 按当前关 `FreeTier` 调用 Cards 的 Tier 过滤牌池，保留现有资格、叠层优先级与确定性随机；为候选不足提供明确失败结果/日志和可继续流程。
4. GameMode 的战后 UI 路由按 Run phase 分发：需要卡牌时打开三选一，无普通投放时直接打开战后商店；选择完成后的现有商店衔接保持一致。
5. 商店卡牌槽直接使用真实 `EncounterIndex` 查 `ShopDropLevels`，删除 `1..6` 钳制；有 `ShopTiers` 时合并全部指定 Tier 的可投放牌池并确定性抽取 3 张互不重复卡，空配置返回 0 槽。
6. 在 Cards 统一资格入口实现1级可重复、2/3级持有排除；扩充 Cards、Trait 与 Shop 测试，覆盖叠加、逐关矩阵、Tier 纯度、确定性、Brave/Sage、无投放跳转、缺表/空池和第 7 关回归。
7. 更新 `MOD-ReEcho.md`、`MOD-ReEchoCards.md`，并完成所有相关 `CODEBASE_MAP` 文档的关闭前具名审阅。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 数据真源 | `scripts\data\sync_xlsx_to_csv.py --check` | 生产 XLSX 与 `shop_drop_levels.csv` 无漂移 |
| 格式 | 对修改的 `.h/.cpp` 运行仓库 `.clang-format` | 仅目标文件格式变化，diff 无语义噪声 |
| 聚焦自动化 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Traits`、`-Filter ReEcho.Shop` 与 `-Filter ReEcho.Characters` | 逐关投放、角色分支与商店第 7 关回归通过 |
| C++ 发布构建 | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新匹配候选的精选预构建包 |
| 静态 | `python scripts/validate_project.py` | 项目、数据与预构建不变量通过 |
| 差异卫生 | `git diff --check` | 无空白/冲突标记问题 |
| 人工 PIE | 1、2、4、7、8 关战后流程 | 记录卡牌 UI 是否出现、候选 Tier、商店衔接与 Boss 结算 |

## 执行记录

### 变化

- 2026-08-24：完成只读现状审计并创建 Plan 88 与独立 worktree；尚未开始实现。
- 经用户确认采用组合适配，吸收 `c8769ba1`（精选 Editor 预构建包刷新）与 `b8836ed1`（Plan86 设置/暂停 UI 文档）；二者与本 Plan 无物理或玩法逻辑冲突。
- 策划源 `投放系统!A1:D47` 为可见工作表，未发现隐藏行/列；卡牌组投放矩阵位于 `A1:C10`。
- 生产 `Design/Data/ReEchoData.xlsx` 的 `经济系统` 已包含相同 `EncounterIndex/FreeTier/ShopTiers` 数据，`Content/Data/shop_drop_levels.csv` 与之同步。
- 现有运行时已读取并在商店消费 `ShopTiers`，但将关次钳制到 1..6，令第 7 关错误读取第 6 关；`FreeTier` 虽已解析进快照但没有任何运行时消费方。
- 现有战后流程在非 Boss 的第 1～7 关一律 `GenerateTraitCardOffers(3)`，候选来自未限制 Tier 的完整 `Trait` 可投放池；没有区分大关/小关，也没有按表区分关次。
- 2026-08-24：Run 已以当前 `EncounterIndex` 的 `FreeTier` 推进普通选卡阶段并生成同 Tier 三选一；空值、缺行、非法 Tier 或候选不足时不跨 Tier 回退，安全进入战后商店。所有角色共用该入口，贤者额外选择继续沿用触发关 Tier。
- 2026-08-24：GameMode 改为按 Run phase 路由选卡或直接进入战后商店/回响处理；商店按真实关次读取 `ShopTiers`，不再把第 7/8 关钳制到第 6 关。
- 2026-08-24：新增免费投放 1..8 关矩阵、商店 1..8 关槽位/Tier 矩阵及角色回归；同步把既有 CSV 效果测试从“调用公开抽牌接口索取全牌池”改为直接审计目录并通过调试授予验证效果，避免绕过新的关次投放契约。
- 2026-08-24：按用户补充规则把商店投放修正为“有配置固定三槽”，三槽共同从本关 `ShopTiers` 合并牌池确定性抽取；最初统一排除所有已获得卡，随后按最新规则收敛为1级卡可重复叠加、2/3级卡持有后排除。商店矩阵测试覆盖三槽、Tier 合集、组内去重及购买后资格。
- 2026-08-24：修复商店购买构筑卡后的即时表现：购买成功后 Widget 将该卡同步进当前 `OwnedCards` 快照并立即重绘右侧卡牌槽；卡牌槽加载真实卡牌图标，且不重摇当前商店报价。
- 2026-08-24：定位“同页第一张可买、剩余卡点击失败”为 Run 每次购买前重新按已拥有状态生成报价、而 Widget 仍显示旧页造成的前后端页面漂移。当前商店卡牌页改为按 `EncounterIndex + ShopRefreshSequence` 固定保存 3 个 CardId，各卡价格由卡 ID 与同一页面键独立确定；购买只把该报价标记为已购，不改变同页其余卡，显式刷新或进入下一关才按统一资格生成新页。
- 2026-08-24：按最新资格规则允许已拥有1级卡在后续免费/商店投放中再次出现并叠加；已拥有2、3级卡继续排除。同一商店页的同一1级报价仍只能购买一次，显式刷新后的新报价才可再次购买。
- 2026-08-24：商店卡牌报价实例 ID 纳入 `EncounterIndex + ShopRefreshSequence + CardId`，避免同一1级卡跨关再次出现时被上一关的已购记录误判；武器配件术语统一更正为“符文”，底层 `parts.csv` / `OwnedPartIds` 等兼容字段暂不做破坏性改名。
- 2026-08-24：按用户确认将 Plan88 变基到 `origin/main@a4a72b98`。冲突以主分支为基线：保留角色能力数据化、Forge 删除、商店暂停与响应式布局，再叠加逐关投放、稳定三槽商店页、1级重复规则和购买后即时卡槽显示。
- 2026-08-24：变基后角色回归发现两个组合问题：第 1 关按表进入 `Planning` 后重复结算回调会重复应用诗人增长；Plan87 的角色测试仍预期第 1 关进入普通选卡、且 Sage 测试未指定投放关次。Run 现对 `Planning/Shop` 等已结算阶段保持幂等，角色测试改为验证第 1 关跳过、第 2 关进入 Tier 2 普通选卡，并在第 2 关验证 Sage 节奏。
- 2026-08-24：推送前 fetch 发现 Plan71 Echo Gameplay Blueprint/空间表现进入远端。经用户再次确认组合适配后变基到 `origin/main@e3a8701a`；`ReEchoGameMode.cpp` 的 Echo 构造/生成入口与 Plan88 的战后卡牌阶段路由位于不同区段并自动合并，`MOD-ReEcho.md` 两项契约并存，精选二进制等待在最终组合源码上重新生成。

### 证据

- `scripts\data\sync_xlsx_to_csv.py --check`：通过。
- `scripts\ue\Build-Editor.cmd -Configuration Development`：通过；UHT/UBT 成功并刷新精选 Editor 预构建包。
- `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`：通过；98/98 actions 成功，最终候选预构建源指纹 `f31b2023197c`。
- `python scripts/validate_project.py`：通过（静态校验与预构建指纹匹配）。
- `git diff --check`：通过。
- `ReEcho.Traits` 首轮运行：新 `PostEncounterDropMatrix` 等 5 项通过；既有 `CsvEffectsApply` 暴露两条已漂移断言（J_SPADE 默认武器仍写旧值、启用 Trait 数仍写旧值），已按当前权威 CSV 修正。重跑时同克隆的 Plan86 Editor 与 Plan63 打包在锁外并发启动，聚焦自动化未完成，待共享 UE 进程退出后重跑。
- `scripts\ue\Run-Automation.cmd -Filter ReEcho.Traits`：通过，7/7；包含 1..8 免费投放矩阵与缺行/候选不足安全降级。
- `scripts\ue\Run-Automation.cmd -Filter ReEcho.Shop`：通过，8/8；包含 1..8 商店槽位/Tier 矩阵与第 7 关 Tier 1 + Tier 3 回归。
- `scripts\ue\Run-Automation.cmd -Filter ReEcho.Characters`：通过，3/3；覆盖 Brave 第 1/2 关锻造后分流与 Sage 第 4 次普通选择额外选择。
- 用户补充规则后的最终聚焦回归：`ReEcho.Cards` 5/5、`ReEcho.Traits` 7/7、`ReEcho.Shop` 8/8、`ReEcho.Characters` 3/3，全部通过。
- 用户补充规则后的 `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`：通过，97/97 actions 成功；精选 Editor 预构建源指纹 `fd19af302e07`。
- 最终 `scripts\data\sync_xlsx_to_csv.py --check`、`python scripts/validate_project.py` 与 `git diff --check`：全部通过。
- 即时卡牌槽修复聚焦回归：`ReEcho.UI.Shop` 2/2、`ReEcho.Shop` 8/8，全部通过；新增测试证明购买前卡牌槽为空，购买后立即获得 Tooltip 并加载对应真实卡牌图标。
- 即时卡牌槽修复后的 `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`：通过，96/96 actions 成功；精选 Editor 预构建源指纹 `6aade6b79629`。随后 XLSX/CSV 检查、`validate_project.py`、预构建校验与 `git diff --check` 全部通过。
- 最终1级重复规则与稳定商店页聚焦回归：`ReEcho.Cards` 5/5、`ReEcho.Traits` 8/8、`ReEcho.Shop` 8/8、`ReEcho.Characters` 3/3、`ReEcho.UI.Shop` 2/2，全部通过。新增覆盖已拥有1级卡免费再次选择、商店跨刷新/跨关再次购买并叠加、已拥有2/3级卡排除、同页三卡连续购买，以及稳定商店页存档恢复。
- 最终 `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`：通过，96/96 actions 成功；精选 Editor 预构建源指纹 `a9ebb6465c29`。商店套件曾暴露旧断言把三个装备报价错误限定为三个符文；按既有“武器或符文”投放设计修正断言后 8/8 通过，未修改产品投放权重。
- 最终 `scripts\data\sync_xlsx_to_csv.py --check`、`python scripts\validate_project.py`、`python scripts\ue\prebuilt_editor.py check` 与 `git diff --check`：全部通过；生产 XLSX/CSV 无漂移，精选 Editor 包与 `a9ebb6465c29` 源指纹匹配。
- 首次变基到 `a4a72b98` 后的聚焦回归：`ReEcho.Cards` 5/5、`ReEcho.Traits` 8/8、`ReEcho.Shop` 8/8、修正后的 `ReEcho.Characters` 3/3、`ReEcho.UI.Shop` 2/2、`ReEcho.Run` 14/14，全部通过；该证据随后由最终组合回归取代。
- 首次变基到 `a4a72b98` 后的 `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`：通过，96/96 actions 成功；精选 Editor 预构建源指纹 `502a05de5495`。该证据因随后吸收 Plan71 而失效，最终发布只采用 `e3a8701a` 组合候选上重新生成的证据。
- 最终 `origin/main@e3a8701a` 组合候选回归：`ReEcho.Cards` 5/5、`ReEcho.Traits` 8/8、`ReEcho.Shop` 8/8、`ReEcho.Characters` 3/3、`ReEcho.UI.Shop` 2/2、`ReEcho.Run` 14/14、`ReEcho.Presentation.EchoAppearance` 2/2，全部通过。
- 最终组合候选的 `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`：通过，93/93 actions 成功；精选 Editor 预构建源指纹 `03970a38b501`。

### 剩余风险

- 自动化已覆盖候选不足和缺关次行的安全降级；最终仍需用户在 PIE 确认页面出现/跳过、回响决策和商店关闭后的实际交互衔接。
- `ReEchoRunSubsystem.cpp`、`ReEchoGameMode.cpp` 与商店测试为近期高耦合路径；若发布前 `origin/main` 前进，需重新执行外部提交集成审计并使受影响证据失效。

### 人工验收结果/请求

- 待实现完成后请求用户 PIE 验收。

### 架构文档审阅结果

- `ARCHITECTURE.md`、`README.md` 已审阅，无拓扑或阅读路由变化，无需修改。
- `MOD-ReEcho.md` 已更新逐关免费投放与商店固定三槽/合并 Tier 牌池契约。
- `MOD-ReEchoCards.md` 已更新1级可重复、2/3级持有排除的统一投放资格契约。
- `MOD-ReEchoUI.md` 已更新构筑卡购买后即时填充右侧卡牌槽、且不重摇报价的表现契约。
