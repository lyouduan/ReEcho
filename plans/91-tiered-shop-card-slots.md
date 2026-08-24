# Plan 91 - 程序 - 商店分级固定卡牌槽

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner 与 Executor）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main` @ `f8e8a40bf8ce7974d24b7b296c51bdc4cb35344b`。
- 本地实现方式（可选，仅作交接说明）：独立 worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan91-tiered-shop-card-slots`，分支 `plan/91-tiered-shop-card-slots`。
- 依赖 / 阻塞：基于 Plan88 已发布的逐关 `ShopTiers`、稳定商店页和统一卡牌资格；不改变策划 XLSX/CSV 字段或逐关配置。
- Writes:
  - `plans/91-tiered-shop-card-slots.md`
  - `Source/ReEcho/Public/Run/ReEchoShopCatalog.h`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Source/ReEcho/Private/UI/ReEchoInventoryShopWidget.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopLogicBlockTests.cpp`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardTypes.h`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
- Stable Reads:
  - `Content/Data/shop_drop_levels.csv`
  - `Design/Data/ReEchoData.xlsx`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardRuntime.h`
  - `Source/ReEchoCards/Private/Cards/ReEchoCardRuntime.cpp`
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：继续以 `ShopTiers` 决定本关哪些等级槽投放；继续复用 Cards 的“1级可重复、2/3级持有排除”资格。缓存数组改为固定 `[Tier1, Tier2, Tier3]` 位置并允许 `None` 空位；读取旧的混合池缓存页时按等级形状失配自动重建，不提升 SaveVersion。扁平 `Offers` 兼容投影保留三个卡牌位置，空位置不可购买。
- 明确排除：不修改战斗结束免费三选一；不修改卡牌效果、价格区间、刷新成本、逐关 XLSX 配置或商店美术资产；空槽不跨级补卡。

## 锁定目标

- 商店卡牌区域始终存在三个固定位置：第1槽只允许1级卡，第2槽只允许2级卡，第3槽只允许3级卡。
- `ShopTiers` 仍是逐关投放开关：未配置的等级槽保持空；配置等级但已无合格候选时显示空/售罄。
- 任一等级槽都不得从其他等级回退或补位；刷新只在各槽自己的等级牌池内重抽。
- 保持1级卡可重复投放/叠加，已获得2、3级卡不得再投放；同一稳定页面购买后不重摇其他槽。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` 的 `AREA-Run`、`AREA-UI`，以及 `MOD-ReEchoCards` / `AREA-Cards`。
- 对应模块文档：维护 `MOD-ReEcho.md`、`MOD-ReEchoCards.md`、`MOD-ReEchoUI.md`，均已列入 `Writes`。
- 设计意图：把“投放等级”固化为槽位身份，避免 UI 或调用方把 `ShopTiers` 再解释为共享混合牌池；Run 继续负责编排页面，Cards 继续唯一负责候选资格。
- 权威状态与依赖：不改变权威拥有者和模块依赖方向。Run 持有 `EncounterIndex + ShopRefreshSequence` 页面缓存；Cards 的运行态只保存固定位置 CardId，UI 只消费只读槽投影。
- 决策记录：三个槽始终构造，Tier 固定为1/2/3；未投放或候选耗尽以不可购买的空槽表示。放弃“只返回已填槽”的方案，因为它会让数组位置随关次变化，重新把等级语义泄漏给 UI。放弃跨级补卡，因为用户已明确禁止。
- 相关文档同步范围：更新三个模块文档的商店投放契约；`ARCHITECTURE.md` 已审阅，本 Plan 不新增模块、不改变拓扑或依赖方向；`README.md` 已审阅，稳定标识与阅读路由不变。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：待实现后记录。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`：待实现后记录。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：待实现后记录。
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅、无需修改；模块拓扑和依赖方向不变。
  - `shared/CODEBASE_MAP/README.md`：已审阅、无需修改；稳定标识与路由不变。

## 锁定验收

- [ ] 每个商店页的 `CardSlotOffers` 精确为3项，索引0/1/2的 `Tier` 精确为1/2/3。
- [ ] 第4关 `ShopTiers=2|3` 时1级槽为空，2级槽只出2级卡，3级槽只出3级卡；第1/5/8关三个槽均为空。
- [ ] 对应等级合格牌池耗尽时仅该槽为空/售罄，其他槽不受影响且不跨级补位。
- [ ] 1级重复、2/3级持有排除、页面刷新、购买稳定性和存读档行为有自动化证据。
- [ ] 商店表现层固定构造三个卡牌位置；空槽不可点击、不可广播购买 ID，并能显示“未投放”或“售罄”。
- [ ] C++ 格式化、聚焦自动化、Editor 构建、项目校验和 `git diff --check` 通过。
- [ ] 用户在 PIE 确认第4关三个槽的等级/空槽表现以及第1关全空表现。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@f8e8a40b`，包含 Plan88 最终投放实现；Plan91 为其公开商店槽契约的后续修正。
- 引擎/构建可用性：Plan88 最终候选已完成 UE 5.8 完整构建，Plan91 仍需在自身最终候选重新构建，旧二进制证据不可直接用于发布。
- 现有聚焦测试结果：Plan88 `ReEcho.Shop` 8/8、`ReEcho.UI.Shop` 2/2；其旧断言是“0或3个混合池报价”，需按本 Plan 新契约替换。
- 共享契约 / 难合并资源风险：`ReEchoRunSubsystem.cpp`、卡牌运行态、商店 Widget 和商店测试均为近期高频路径；Plan 发布后、实现发布前都必须 fetch 审计远端变化。
- 基线损坏时的停止条件：远端新增提交；`ShopTiers` 或卡牌资格事实来源变化；旧缓存无法安全自愈；构建或聚焦测试失败。

## 实现提纲

1. 将卡牌商店视图固定为三个带 Tier 身份的槽，并为未投放/售罄增加明确不可购买状态。
2. 将稳定页面缓存改为三个位置的 CardId；每个配置 Tier 独立建立资格池、独立确定性抽取一张，空池写入 `None`。
3. 读取旧缓存时校验位置等级与本关配置；不匹配则在当前页面键下重建，购买后继续保留页面。
4. 扁平兼容投影和 Widget 固定渲染三个位置；空槽禁用购买并显示状态，不加载不存在的卡图标。
5. 更新逐关矩阵、资格耗尽、刷新、购买和存档自动化，并同步模块文档。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 数据静态 | `python scripts/data/sync_xlsx_to_csv.py --check` | XLSX 与生产 CSV 无漂移 |
| C++ 格式 | 仓库 `.clang-format` 仅格式化本 Plan 修改的 `.h/.cpp` | diff 无无关格式噪声 |
| 商店逻辑 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Shop` | 固定等级槽、资格、刷新、购买、存档通过 |
| 商店 UI | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.Shop` | 三槽及空槽不可购买通过 |
| Run/卡牌回归 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Run`、`-Filter ReEcho.Cards`、`-Filter ReEcho.Traits` | 页面缓存与统一资格无回归 |
| C++ 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 成功并刷新精选预构建包 |
| 发布构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终集成候选完整构建并刷新指纹 |
| 静态 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 项目与精选包一致 |
| 人工 | PIE 检查第1、4关商店 | 固定三个槽、等级和空槽状态符合锁定目标 |

## 执行记录

### 变化

- 待实现。

### 证据

- 待执行。

### 剩余风险

- 空槽的最终美术样式仍需用户在 PIE 判断；本 Plan 只保证状态、交互和基础文字正确。

### 人工验收结果/请求

- `PendingBeforeClose`：第1关全空槽和第4关1级空、2/3级有卡的商店表现。

### 架构文档审阅结果

- 待实现后补齐。
