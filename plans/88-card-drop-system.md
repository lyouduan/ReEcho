# Plan 88 - 程序 - 按关次配置的卡牌投放系统

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner 与 Executor）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main` @ `b8836ed1d05847e1a340de9c89148dd802b16393`。
- 本地实现方式（可选，仅作交接说明）：独立 worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan88-card-drop-system`，分支 `plan/88-card-drop-system`。
- 依赖 / 阻塞：依赖 Plan47 的 `MOD-ReEchoCards` 目录/资格过滤，沿用 Plan67 已确认的商店卡牌“一配置 Tier 对应一固定购买槽”语义，并与 Plan85 的商店所有权排除保持兼容。已吸收 `c8769ba1` 的精选 Editor 预构建包刷新；远端 Plan86 设置/暂停 UI 与本 Plan 无 Writes 重叠，可并行实现。Plan87 已由用户说明为另一项尚未发布的本地任务，本 Plan 不占用其编号。
- Writes:
  - `plans/88-card-drop-system.md`
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoTraitTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopTests.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
- Stable Reads:
  - `策划数据源/【开普勒】回响数值与构筑体系.xlsx` 的可见 `投放系统!A1:C10`
  - `Design/Data/ReEchoData.xlsx` 的 `经济系统` 投放数据
  - `Content/Data/shop_drop_levels.csv`
  - `Content/Data/cards.csv`
  - `Content/Data/csv_schema.csv`
  - `Source/ReEcho/{Public,Private}/Data/ReEchoCsvDataRegistry.*`
  - `Source/ReEcho/Private/Data/ReEchoWeaponCsvReader.cpp`
  - `Source/ReEchoCards/{Public,Private}/Cards/ReEchoCardRuntime.*`
  - `Source/ReEcho/{Public,Private}/UI/ReEchoTraitCardChoiceWidget.*`
  - `plans/47-card-build-runtime-module.md`
  - `plans/67-shop-drop-table-overhaul.md`
  - `plans/85-shop-ownership.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：不增加存档字段或 SaveVersion；继续使用本局固定卡牌目录、现有 `OwnedCardIds`、`CanOffer` 和确定性种子。既有商店固定槽/价格/购买/刷新契约保持不变，只纠正第 7 关错误读取第 6 关 `ShopTiers` 的关次索引。WBP 仍只承载三张免费候选，不要求 UI 资产重做。
- 明确排除：不修改 39 张构筑卡的定义与效果，不调整卡牌平衡，不重做商店 UI/资产，不改变 Plan67 已确认的商店卡牌固定槽语义，不修改武器/符文系统，不实现新的卡牌刷新机制，不修改策划源工作簿；如实现发现生产 XLSX/CSV 与本 Plan 锁定矩阵漂移，停止并回到策划真源审计，不静默覆表。

## 锁定目标

战斗结束后的免费卡牌组与商店卡牌槽统一由当前 `EncounterIndex` 对应的 `shop_drop_levels` 行决定，不再由 GameMode 固定写死全牌池三选一。

| 关次 | 免费投放数量与牌池 | 商店卡牌槽与牌池 |
|---|---|---|
| 1 | 0 组 | 0 槽 |
| 2 | 1 组，Tier 2 三选一 | 1 槽，Tier 1 |
| 3 | 1 组，Tier 3 三选一 | 1 槽，Tier 1 |
| 4 | 1 组，Tier 1 三选一 | 2 槽，Tier 2 + Tier 3 |
| 5 | 1 组，Tier 2 三选一 | 0 槽 |
| 6 | 1 组，Tier 3 三选一 | 2 槽，Tier 1 + Tier 2 |
| 7 | 1 组，Tier 2 三选一 | 2 槽，Tier 1 + Tier 3 |
| 8 | 0 组 | 0 槽 |

- `FreeTier` 为空表示本关战后不进入普通卡牌选择，直接进入战后商店/回响处理；有值时只从该 Tier 的 `Trait` 可投放牌池生成 3 张且选择 1 张。
- 免费候选继续遵守 `MOD-ReEchoCards` 的 Enabled/Offerable、唯一性、冲突和持有状态资格规则；不得在 Run 或 UI 复制卡牌规则。
- Plan87 已删除勇者 Forge；所有角色统一按当前关 `FreeTier` 决定进入普通三选一或直接进入商店。角色能力不得改变这条投放阶段路由。
- 贤者每第 4 次普通选择触发的额外选择沿用当前关 `FreeTier`，且“不投放”关不会凭空创建普通选择或累计次数。
- 第 8 关 Boss 结算保持无免费选卡、无商店卡牌投放并进入胜利结算。
- 缺少关次行、Tier 非法或候选不足 3 张时不得回退到全等级牌池；数据校验应尽早报错，运行时记录明确错误并安全进入商店/后续结算，不能卡死流程。
- 商店继续遵循 Plan67 的固定槽解释，但必须按真实 `EncounterIndex` 查表；不得把第 7/8 关钳制成第 6 关。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`（直接修改）；`AREA-Data`、`AREA-Run`、`AREA-Cards`、`AREA-UI`（行为链受影响）。`MOD-ReEchoCards` 与 `MOD-ReEchoUI` 为稳定消费方，预期不改其源码/公共契约。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已加入 `Writes`，需记录 `AREA-Run` 以 `shop_drop_levels` 决定战后阶段和 Tier 牌池的契约。`MOD-ReEchoCards.md` 与 `MOD-ReEchoUI.md` 关闭前具名审阅；若实现保持现有 `BuildOfferPool(..., Tier)` 和三槽 Widget 契约，则记录“已审阅、无需修改”。
- 设计意图：让生产表已存在的 `FreeTier` 成为战后普通卡牌投放的唯一权威，并把“是否显示选卡、从哪个 Tier 抽取、选卡后进入哪里”集中在 Run 阶段策略中；GameMode 只按 Run phase 路由 UI，Cards 只负责牌池资格与授予。
- 权威状态与依赖：`Design/Data/ReEchoData.xlsx` → `shop_drop_levels.csv` → `FReEchoCsvDataSnapshot::ShopDropLevels` 是关次投放配置链；Run 拥有阶段与本局事务；Cards 拥有资格/牌池/授予；GameMode/UI 不保存第二份关次矩阵。状态所有者不变，不增加跨 Runtime Module 依赖。
- 决策记录：
  - 复用现有 `FReEchoCsvShopDropLevelRow { EncounterIndex, FreeTier, ShopTiers }`，不新建重复表或硬编码八关数组；当前数据同步 `--check` 已证明生产 XLSX 与 CSV 一致。
  - 免费投放数量由 `FreeTier` 的“空/有值”表达为 0/1 组，每组固定三选一；不为此把 Widget 改成任意数量候选。
  - 商店投放沿用 Plan67 的既有人工决定：`ShopTiers` 每个 Tier 对应一个购买槽；本 Plan 只修正逐关读取和全矩阵回归，不重新解释为弹窗三选一。
  - Run 提供单一的当前关免费投放解析/阶段推进入口，由 `CompleteEncounter` 和普通卡牌选择完成路径共用，避免角色特例绕过 `FreeTier`。
  - 确定性仍基于现有 `TraitOfferSeed + EncounterIndex + OwnedCardIds`；只把 Tier 作为候选池过滤条件，不引入时间或 UI 状态随机源。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：关闭前审阅；预期无 Runtime Module 拓扑或依赖方向变化，无需修改。
  - `shared/CODEBASE_MAP/README.md`：关闭前审阅；预期无新架构标识或阅读路由，无需修改。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：更新 `AREA-Data` / `AREA-Run` 的投放配置消费与阶段路由。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`：审阅 Tier 牌池与资格职责是否仍准确。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：审阅固定三候选 Widget 与 GameMode 路由是否仍准确。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅。
  - `README.md`：待审阅。
  - `MOD-ReEcho.md`：待更新。
  - `MOD-ReEchoCards.md`：待审阅。
  - `MOD-ReEchoUI.md`：待审阅。

## 锁定验收

- [ ] 自动化逐行覆盖 1..8 关，证明免费投放组数/Tier 与商店槽数/Tier 精确匹配锁定矩阵。
- [ ] 第 1 关结束不出现普通三选一，但仍进入战后商店和回响处理；第 2～7 关普通三选一的 3 张卡全部属于配置 Tier；第 8 关无普通选卡并正常结算。
- [ ] 第 7 关商店精确生成 Tier 1 + Tier 3 两个卡牌槽，不再错误复用第 6 关 Tier 1 + Tier 2。
- [ ] 勇者与其他角色遵循相同关次矩阵：第 1 关直接进入商店，有 `FreeTier` 的关次进入正确 Tier 的普通三选一，且不会出现 Forge 页面。
- [ ] 贤者额外选择仍按每 4 次普通选择触发，额外候选与触发关的 `FreeTier` 一致。
- [ ] 同一基线/种子/持有卡状态生成相同候选；非法或不足牌池不会回退全等级，也不会卡死战后流程。
- [ ] `scripts\data\sync_xlsx_to_csv.py --check`、聚焦自动化、完整 Editor 构建、`python scripts/validate_project.py`、`git diff --check` 通过。
- [ ] 人工 PIE 验收以上 1、2、4、7、8 关的界面出现/跳过、卡牌等级与商店衔接。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main` @ `b8836ed1d05847e1a340de9c89148dd802b16393`。远端 Plan86 已占用，用户明确说明 Plan87 也已由另一项本地任务占用，因此本任务使用 Plan88。
- 引擎/构建可用性：尚未为本 Plan 启动 UE；`c8769ba1` 已对当前源码刷新精选 Editor 预构建包，使 Plan-only 静态发布候选具备重新校验条件。后续 C++ 实现仍必须在最终候选上执行完整构建。
- 现有聚焦测试结果：`python scripts\data\sync_xlsx_to_csv.py --check` 通过，证明 `ReEchoData.xlsx` 导出与生产 CSV 字节一致；尚未运行 UE 自动化。
- 共享契约 / 难合并资源风险：本 Plan 将编辑近期商店/Run 高频文件，尤其 `ReEchoRunSubsystem.cpp`、`ReEchoGameMode.cpp`、`ReEchoShopTests.cpp`；发布前必须再次 fetch 并审计 Plan85 之后的新提交。工作簿与 CSV 当前无需写入，避免与策划表二进制变更产生无意义冲突。
- 基线损坏时的停止条件：远端在 Plan-only 发布或实现集成前再次前进；策划源可见 `投放系统!A3:C10` 与生产 `shop_drop_levels.csv` 漂移；聚焦基线或预构建校验重新失败。命中任一条件时停止并重新审计，不静默合并或回退。

## 实现提纲

1. 在 Run 层增加当前关 `shop_drop_levels` 解析与免费投放策略的单一入口，返回“无普通投放”或有效 `FreeTier`；禁止默认全牌池回退。
2. `CompleteEncounter` 与普通卡牌选择完成路径复用该策略推进到 `CardChoice` / `Planning`；保持 Sage 额外选择语义，不增加 Brave 特殊阶段。
3. `GenerateTraitCardOffers` 按当前关 `FreeTier` 调用 Cards 的 Tier 过滤牌池，保留现有资格、叠层优先级与确定性随机；为候选不足提供明确失败结果/日志和可继续流程。
4. GameMode 的战后 UI 路由按 Run phase 分发：需要卡牌时打开三选一，无普通投放时直接打开战后商店；选择完成后的现有商店衔接保持一致。
5. 商店卡牌槽直接使用真实 `EncounterIndex` 查 `ShopDropLevels`，删除 `1..6` 钳制；无配置或空 `ShopTiers` 均返回 0 槽。
6. 扩充 `ReEchoTraitTests.cpp` 与 `ReEchoShopTests.cpp`：逐关矩阵、Tier 纯度、确定性、Sage、全角色一致的无投放跳转、缺表/空池和第 7 关回归。
7. 更新 `MOD-ReEcho.md`，并完成所有相关 `CODEBASE_MAP` 文档的关闭前具名审阅。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 数据真源 | `scripts\data\sync_xlsx_to_csv.py --check` | 生产 XLSX 与 `shop_drop_levels.csv` 无漂移 |
| 格式 | 对修改的 `.h/.cpp` 运行仓库 `.clang-format` | 仅目标文件格式变化，diff 无语义噪声 |
| 聚焦自动化 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Trait` 与 `-Filter ReEcho.Shop` | 逐关投放、角色分支与商店第 7 关回归通过 |
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

### 证据

- `scripts\data\sync_xlsx_to_csv.py --check`：通过。
- `python scripts/validate_project.py`：通过（`static verified only`，未声称 UHT/UBT/PIE）。
- `git diff --check`：通过。
- UE 构建/自动化：未运行。

### 剩余风险

- Plan86 的设置/暂停 UI 后续若扩张到 `ReEchoGameMode.cpp`，实现集成时需要重新审计潜在同行编辑；其当前已发布 Writes 不包含该文件。
- 免费候选不足 3 张的运行时错误路径需要在实现中保持可继续，不得把数据错误伪装成跨 Tier 回退。
- `ReEchoRunSubsystem.cpp`、`ReEchoGameMode.cpp` 与商店测试为近期高耦合路径，后续 main 更新需要重新执行外部提交集成审计。

### 人工验收结果/请求

- 待实现完成后请求用户 PIE 验收。

### 架构文档审阅结果

- 待实现与评审阶段填写。
