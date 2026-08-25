# Plan 108 - 程序 - 商店卡组预付费与待选恢复

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`，需要用户确认商店卡组购买键、扣费反馈、继续选择入口和返回/读档后的可用性。
- 本地规划 / 实现基线：`origin/main@dc46a89a80bc67c738952374ae971cde5b69ddd3`。
- 本地实现方式：独立 worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan108-shop-card-pack-prepay`；分支 `plan/108-shop-card-pack-prepay`。
- 依赖 / 阻塞：依赖已关闭的 Plan97 分级卡组三选一、Plan101 卡组逐槽刷新与 Plan105 卡牌实际结果摘要；保留主线 SaveVersion 19 与 `OutcomeText`。现有远端任务分支已使用 Plan106/107，但尚未进入 `main`；当前程序用户已明确授权本任务使用 Plan108，避免覆盖或迫使进行中任务改号，并接受 `main` 暂时出现编号空档。
- Writes:
  - `plans/108-shop-card-pack-prepay.md`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardTypes.h`
  - `Source/ReEcho/{Public,Private}/Run/ReEchoRunSubsystem.*`
  - `Source/ReEcho/Public/Run/ReEchoRunSaveGame.h`
  - `Source/ReEcho/Public/Run/ReEchoShopCatalog.h`
  - `Source/ReEcho/{Public,Private}/ReEchoGameMode.*`
  - `Source/ReEcho/{Public,Private}/UI/ReEchoInventoryShopWidget.*`
  - `Source/ReEcho/{Public,Private}/UI/ReEchoTraitCardChoiceWidget.*`（仅实际需要移除逐卡付费表现或加入预付模式时）
  - `Source/ReEcho/Private/Tests/` 中实际受影响的 Shop、Trait、Save/UI 测试
  - `Source/ReEchoCards/Private/Tests/` 中实际受影响的状态/兼容测试
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - 最终 FullRebuild 刷新的 `Binaries/Win64/` 精选预构建包
- Stable Reads:
  - `Content/Data/shop_price_ranges.csv` 的 `Card_T1/Card_T2/Card_T3`
  - `Content/Data/shop_drop_levels.csv`
  - `Content/Data/shop_refresh_rules.csv`
  - `Design/Data/ReEchoData.xlsx`（本任务不修改卡牌、价格或刷新数值）
  - `plans/97-tiered-shop-card-pack-choice.md`
  - `plans/101-separated-shop-card-pack-refresh.md`
  - `plans/105-card-outcome-tooltips.md`
  - `shared/CODEBASE_MAP/{ARCHITECTURE,README}.md`
- 影响模式：`SharedContract`。改变商店卡组价格投影、购买时点、Run/Cards 可保存状态、SaveVersion、GameMode/UI 命令和卡牌购买事件时序。
- 兼容承诺 / 下游操作：保持逐关 `ShopTiers`、同级最多三张候选、一级可重复、二三级排除已拥有、逐卡槽刷新 `1 次 / 5 碎片`、确定性候选与价格、折扣和禁购规则；旧 SaveVersion 19 页面确定性迁移为未付费或已完成，不伪造待选付款。Plan105 `OutcomeText` 不回归。
- 明确排除：不修改卡牌数值/描述、价格区间、投放关次、刷新费用、武器/符文购买逻辑、免费过关选卡、卡面资源或 WBP 视觉资产；不把中文描述或 Widget 变成购买状态权威。

## 锁定目标

1. 商店仍固定展示一至三级卡组入口；未投放、售罄和已完成状态保持。可购买卡组在入口下方显示与武器/符文一致的购买键，显示该卡组的折后价格；点击后不弹二次确认，立即执行权威购买事务，成功才进入该等级原候选页。
2. 每个开放卡组只有一个稳定价格，不再让三张候选分别计价。基础价继续从 `Card_T1/Card_T2/Card_T3` 现有区间按 Run/关次/等级确定性生成，并随卡组页面状态保存；`G_2_16` 折扣在付款时应用并向上取整。读档、返回和重新打开不得重摇价格。
3. 卡组状态机固定为 `Available → PaidPendingChoice → Purchased`。付款成功后立即扣费、触发一次 `OnPurchase`、保存 Run，并进入三选一；最终选卡只授予卡牌，不再次扣费或触发购买事件。
4. `PaidPendingChoice` 允许返回商店，不退款。入口显示“继续选择”，重新进入不重复收费；候选、逐槽刷新用量与卡组价格保持。最终卡牌成功授予后才转为 `Purchased`。
5. 付款后立即保存待选状态。退出、崩溃恢复或继续游戏后，玩家可以从原卡组入口免费继续原候选页；不得重复扣费、重摇候选、刷新次数或价格。SaveVersion 升至 20，v19 及更早存档明确迁移：旧 `bPurchased=true` 视为已完成，其余卡组视为未付款。
6. 逐槽刷新保持现状：付款后每个实际候选槽仍可额外花费 5 时间碎片刷新一次，只替换同级当前槽，并排除当前页与所有已获得卡牌；返回、读档或继续选择不重置机会。
7. `OnPurchase` 只在卡组付款成功时触发。付款前已拥有的 `G_2_16` 正常获得本次购买成长；本次最终选择得到的 `G_2_16` 不追溯触发已经完成的卡组付款。`G_3_17` 的禁购代价在付款入口检查；若它是本次付款后选到的卡，只影响后续其他卡组。
8. 余额不足、禁购、卡组已付款/已完成、状态过期或数据缺失时，不扣费、不触发 `OnPurchase`、不进入选卡页。付款成功后的卡牌授予失败保留 `PaidPendingChoice`、余额和候选页，允许重试或返回，不退款、不重复收费、不静默售罄。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoCards`（`AREA-Cards` 可保存卡组状态）、`MOD-ReEcho`（`AREA-Run`、`AREA-UI`、`AREA-Tests`）、文档型 `MOD-ReEchoUI`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`、`MOD-ReEcho.md`、`MOD-ReEchoUI.md`，均已加入 Writes。
- 设计意图：把“购买卡组”和“领取卡牌”拆成两个可恢复的原子事务。Run/Cards 保存卡组付款事实，GameMode 只编排付款后开页和领取后刷新，UI 只显示状态与发送命令。
- 权威状态与依赖：`FReEchoShopCardPackRuntimeState` 继续是固定等级卡组候选、刷新和购买阶段的唯一可写事实；新增稳定基础价与待选付款状态。Run 独占货币扣除、折扣、`OnPurchase` 和保存编排；Cards 继续独占卡牌授予；UI 不持有可写余额或购买阶段。
- 决策记录：不复用候选 `ItemId` 作为付款凭据，而为等级卡组建立稳定购买 ID/窄命令；不在 UI 以“余额减少”推断已付款；不把付款与最终选择强绑成一个无法跨返回/存档恢复的长事务。武器/符文“成功购买后立即 SaveRun”作为付款保存时点参考，但卡组额外持久化 `PaidPendingChoice`，因为卡牌授予被延后。
- 相关文档同步范围：关闭前必审 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`modules/MOD-ReEcho.md`、`MOD-ReEchoCards.md`、`MOD-ReEchoUI.md`；模块拓扑和索引预计不变，若审阅确认不变则只在执行记录注明。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅依赖拓扑与跨模块不变量；
  - `README.md`：待审阅 AREA 路由；
  - `MOD-ReEcho.md`：待同步 Run/Save/Shop 的两阶段事务；
  - `MOD-ReEchoCards.md`：待同步卡组待选付款状态与最终授予边界；
  - `MOD-ReEchoUI.md`：待同步卡组购买键、继续选择和候选无价展示契约。

## 锁定验收

- [ ] 开放卡组入口显示单一卡组价格和购买键；点击后直接扣费并进入同级最多三选一，候选卡不再分别计价。
- [ ] 每个卡组基础价在配置区间内确定性生成，折扣、余额不足和 `NoExtraCardPurchase` 门禁准确；失败不改变余额、卡组状态或购买事件结果。
- [ ] 付款成功立即触发一次且仅一次 `OnPurchase`、保存并进入 `PaidPendingChoice`；返回后显示“继续选择”，重新进入不收费、不重摇。
- [ ] 待选状态、基础价、候选和逐槽刷新用量经 SaveVersion 20 保存/恢复；v19 及更早迁移不伪造已付款状态，旧已购卡组保持完成。
- [ ] 最终选择成功只授予卡牌并转为 `Purchased`，不二次扣费或触发购买；授予失败保留已付款待选状态并可重试。
- [ ] 卡组付款前已拥有的 `G_2_16` 本次触发，作为本次选择获得的 `G_2_16` 不追溯；本次选择获得 `G_3_17` 后只阻止后续卡组。
- [ ] 逐槽刷新仍为每槽 1 次、每次 5 碎片，返回/读档不重置；免费过关选卡和武器/符文购买/刷新不回归。
- [ ] Shop、Trait、Save、Cards/UI 聚焦自动化、Development Editor 构建、项目校验与 `git diff --check` 通过；最终发布候选 FullRebuild 与预构建检查通过。
- [ ] 用户在 PIE 验收购买键、即时扣费、返回/继续、读档恢复、刷新和最终选卡可用性。
- [ ] 未提交精选 `GIT_RULES.md` 允许列表外的 UE 生成物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@dc46a89a`；包含已关闭 Plan105、SaveVersion 19 与卡牌结果摘要。
- 引擎/构建可用性：UE 5.8 与仓库 Windows 构建入口预计可用；运行需要独占 Editor 的命令前检查并请用户关闭交互式 Editor，使用 Git common-dir Unreal 锁。
- 现有聚焦测试结果：规划期只读审计确认现有卡组为逐卡定价、选择时扣费；实现后重新运行 Shop/Trait/Save/Cards/UI 证据，不复用 Plan101/105 的旧构建结果。
- 共享契约 / 难合并资源风险：直接修改 Plan105 刚更新的 `ReEchoCardTypes.h`、Run/Save/ShopCatalog、Inventory Shop Widget、测试、模块文档和最终预构建包；实现以 `dc46a89a` 为组合基线保留 `OutcomeText`。Plan106/107 与本任务主要在工作簿、武器/Combat 和共享二进制耦合，进入 main 前必须审计实际远端并重新 FullRebuild。
- 基线损坏时的停止条件：未修改候选上的 Shop/Trait/Save/Cards 测试失败；必须改变已确认的退款、购买事件或待选恢复语义；无法保持失败原子性；远端先合入 Plan106/107 或其他修改相同 Run/UI/Save 契约的提交。

## 实现提纲

1. 为卡组状态和只读投影增加稳定基础价、`PaidPendingChoice` 与迁移不变量。
2. 将当前“候选卡付费授予”拆为“卡组付款”和“已付款候选领取”两个窄事务，建立准确审计、折扣、`OnPurchase` 和失败回滚。
3. 升级 SaveVersion 20，覆盖 v19 页面迁移、付款后保存、返回/继续和读档恢复。
4. 改造商店卡组入口的购买/继续按钮和 GameMode 流程；候选页移除逐卡价格/余额门禁但保留逐槽刷新费用。
5. 补充 Cards、Shop、Trait、Save/UI 自动化，维护模块文档和执行记录，完成格式化、构建、静态门禁与用户 PIE 验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Cards/状态 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Cards` | 卡组阶段、迁移和最终授予不变量通过 |
| Shop/事务 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Shop` | 单一卡组价格、付款、折扣、禁购、一次购买事件、失败原子性通过 |
| Trait/Save | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Traits` 与实际 Save 过滤器 | 预付候选领取、返回/读档恢复、v19→v20 迁移通过 |
| UI | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.Shop` | 购买/继续按钮、候选无价展示、刷新费用与状态路由通过 |
| C++ | `.clang-format`；`scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0并刷新开发预构建包 |
| 静态 | `python scripts/validate_project.py`；`git diff --check` | 项目、模块文档和文本不变量通过 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`；`python scripts/ue/prebuilt_editor.py check` | 最终集成源码与精选预构建包匹配 |
| 人工 | PIE 中购买、返回、继续、读档、刷新、领取与失败重试 | 用户确认交互、反馈和可恢复性正确 |

## 执行记录

### 变化

- 2026-08-25：用户确认卡组从“选中候选时付费”改为“入口购买键先付费再进入”；卡组使用现有等级价格区间的单一稳定价格，不弹二次确认。
- 2026-08-25：用户确认付款后可返回且不退款，入口可免费继续选择；付款后保存/恢复参考武器符文的即时保存时点，但增加可持久化待选阶段；逐槽刷新保持 `1 次 / 5 碎片`。
- 2026-08-25：用户确认 `OnPurchase` 在卡组付款时触发，最终领取不再触发；付款后授予失败保留已付款待选状态。
- 2026-08-25：fetch 发现主线从初始审计的 `061083d5` 前进至 `dc46a89a`，已审计 Plan105 对 Cards/Run/Save/UI 的直接重叠并由用户确认采用远端组合基线。用户同时确认因现有远端 Plan106/107 任务分支而使用 Plan108 的编号例外。

### 证据

- 规划期源码审计确认当前 `FReEchoShopCardChoiceOffer` 为逐卡价格，`PurchaseShopItemDetailed` 在候选领取时扣费、授予、触发 `OnPurchase` 并把卡组设为 `bPurchased`；GameMode 成功后 `SaveRun`。
- `shop_price_ranges.csv` 当前为 `Card_T1=30..50`、`Card_T2=100..120`、`Card_T3=150..200`；`shop_refresh_rules.csv` 当前卡槽刷新为 `1 次 / 5 碎片`。

### 剩余风险

- 两阶段事务新增一个跨页面/存档生命周期状态；必须覆盖重复点击、付款后返回、读档、候选授予失败和旧存档迁移，避免重复扣费或免费领取。
- Plan105 刚修改相同卡牌状态与 Tooltip 投影；实施和集成时必须保留结果摘要字段与 SaveVersion 19 迁移。
- Plan106/107 后续进入 main 会使共享工作簿、模块文档和预构建包证据失效；最终发布前重新审计并 FullRebuild。

### 人工验收结果/请求

- `PendingBeforeClose`：实现完成后请在 PIE 验收卡组购买键、直接扣费、返回/继续、读档恢复、刷新与最终领取。

### 架构文档审阅结果

- 待实现后填写。
