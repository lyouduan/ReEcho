# Plan 101 - 程序 - 商店武器符文与卡组独立刷新

## 协调

- Planner / Executor：Codex（按用户要求合并）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@6b3262fa574d9ea684cad1c0ce8d6c8c96f25fc6`。
- 本地实现方式：`plan/101-separated-shop-card-refresh`；`C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan101-separated-refresh`。
- 依赖 / 阻塞：Plan 97 的固定一至三级卡组入口与付费三选一；`策划数据源/【开普勒】回响数值与构筑体系.xlsx` 的可见 `投放系统!A27:C30` 是本任务原始数值依据，进入运行时前迁移到生产权威 `Design/Data/ReEchoData.xlsx`。
- Writes:
  - `plans/101-separated-shop-card-pack-refresh.md`
  - `Design/Data/ReEchoData.xlsx`
  - `Content/Data/shop_refresh_rules.csv`
  - `Content/Data/{reecho_data_manifest,csv_schema}.csv`
  - `scripts/data/sync_xlsx_to_csv.py` 及相关数据验证/测试
  - `Source/ReEchoCards/Public/Cards/ReEchoCardTypes.h`
  - `Source/ReEcho/{Public,Private}/Data/ReEcho{CsvDataRegistry,WeaponCsvReader}.*`
  - `Source/ReEcho/{Public,Private}/Run/ReEcho{RunSaveGame,RunSubsystem,ShopCatalog}.*`
  - `Source/ReEcho/{Public,Private}/UI/ReEcho{InventoryShop,TraitCardChoice}Widget.*`
  - `Source/ReEcho/{Public,Private}/ReEchoGameMode.*`
  - `Source/ReEcho/Private/Tests/ReEcho{Shop,ShopLogicBlock,SaveGame,TraitCardChoice}Tests.cpp` 中实际存在且受影响的文件
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
- Stable Reads:
  - `策划数据源/【开普勒】回响数值与构筑体系.xlsx`（只读原始设计依据）
  - `Content/Data/shop_drop_levels.csv`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp` 的现有商店页与统一购买事务
  - `Source/ReEcho/Private/UI/ReEchoInventoryShopWidget.cpp`
  - `Source/ReEcho/Private/UI/ReEchoTraitCardChoiceWidget.cpp`
- 影响模式：`SharedContract`。拆分现有共享 `ShopRefreshSequence`，新增刷新规则数据、Run/Save 状态、卡组三选一命令与 UI 投影。
- 兼容承诺 / 下游操作：武器、符文、卡牌购买价格、投放等级、初次投放资格、购买审计、卡牌效果 `FreeShopRefresh` 与 `NoShopRefresh` 保持；刷新替换结果额外排除所有已获得卡牌。旧存档安全迁移，新状态不允许通过读档重置次数。
- 明确排除：不接入新的正式 UI 图片，不改变卡组投放等级表、不改变免费过关选卡、不调整商品/卡牌价格、不修改外部原始开普勒工作簿。

## 锁定目标

1. 商店主页面的刷新按钮只刷新当前武器/符文三个报价槽，不得改变任一卡组候选、卡组已购状态或卡组刷新次数。
2. 每次进入新的商店关次，武器/符文共有 2 次刷新机会；按钮显示剩余次数。每次成功刷新消耗 5 时间碎片；已有免费商店刷新次数优先抵扣价格，但仍占用本关 2 次上限。
3. 卡组三选一界面的每个实际卡位在该卡组本关各有 1 次刷新机会；刷新按钮位于对应卡牌下方，并显示该卡位剩余次数。每次成功刷新只替换被点击卡位，消耗 5 时间碎片；其他两个卡位保持原样。
4. 刷新替换严格使用当前卡组等级，并排除页面当前其他卡牌、刚被替换的卡牌和玩家所有已获得卡牌（包括已获得的 1 级卡）。若不存在合法替换，按钮禁用且不扣碎片、不耗次数。刷新不改变其他等级卡组、武器/符文报价和已购状态；武器/符文刷新不改变卡组。余额不足、次数耗尽、禁用刷新或无效界面请求同样原子失败。
5. `NoShopRefresh` 继续阻止所有商店内刷新。成功购买卡组后仍关闭三选一并将入口置为已购；刷新不会把已购入口重新开放。
6. 刷新上限、刷新价格从生产 XLSX→CSV→类型化运行时快照读取：卡组 `1 / 5`，武器符文 `2 / 5`。运行时不得继续以 C++ 魔法常量复制这些数值。
7. 当前关次的武器/符文刷新次数与序列、三个卡组各自的刷新次数与序列随存档恢复；进入下一商店关次重置为表中上限。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`（`AREA-Data`、`AREA-Run`、`AREA-UI`、`AREA-Tests`）、`MOD-ReEchoCards`、文档型 `MOD-ReEchoUI`。`MOD-ReEchoWeapons` 不改：武器逻辑模块不拥有商店或刷新状态。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoCards.md`、`MOD-ReEchoUI.md`，均已列入 Writes；审阅 `MOD-ReEchoWeapons.md`，若只读商店边界不变则在执行记录注明无需修改。
- 设计意图：Run 继续是商店事务和货币的唯一权威；Cards 只保存卡组运行态；UI 只显示只读剩余次数并发出类型化刷新请求；GameMode 只编排页面更新。
- 权威状态与依赖：新增单行 `shop_refresh_rules` 数据表，由 Data Registry 编译为不可变规则；Run 保存当前关次武器/符文已用次数和三个卡组各自已用次数/随机序列。依赖方向仍是 UI/GameMode → Run → Cards/Data，不新增反向依赖。
- 决策记录：
  - 不再复用一个全页 `ShopRefreshSequence`，避免两个刷新入口互相重摇。
  - 武器/符文刷新保留 `FreeShopRefresh` 的既有经济价值，但免费次数不能绕过 2 次上限。
  - “三张分别分配”按三选一页面的每个实际候选卡位分别计数；不是每个等级卡组共享一次，也不是三个等级入口分别一次。
  - 初次投放继续遵守既有规则（1 级可重复，2/3 级排除已拥有）；用户明确要求刷新替换结果更严格，所有等级都排除已获得卡牌。
  - 刷新已购卡组无入口且不允许；刷新其他卡组不清除该已购状态。
- 相关文档同步范围：必审 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoCards.md`、`MOD-ReEchoUI.md`、`MOD-ReEchoWeapons.md`。只有运行时状态/契约事实变化的正文才更新。
- 关闭前逐项填写审阅结果：见执行记录“架构文档审阅结果”。

## 锁定验收

- [ ] 开普勒表的可见 `1/2/5/5` 值已迁移到生产 XLSX 和生成 CSV，权威同步检查无漂移。
- [ ] 商店主刷新最多成功 2 次且每次只刷新武器/符文；显示剩余次数，第三次原子拒绝。
- [ ] 三选一页面每个实际卡位各自最多成功刷新 1 次；按钮位于卡牌下方并显示该卡位剩余次数，刷新只替换所点卡位。
- [ ] 刷新结果不重复当前页面卡牌、不返回被替换卡牌，也不出现任何已获得卡牌；无合法替换时按钮禁用且状态不变。
- [ ] 每次付费刷新实际扣 5 碎片；余额不足、次数耗尽、禁用刷新不改变任何状态；免费商店刷新只抵扣武器/符文刷新价格且仍消耗上限。
- [ ] 购买、取消、失败、读档和下一关重置行为符合锁定目标；候选保持确定性且二、三级继续排除已拥有卡牌。
- [ ] 聚焦自动化、Development Editor 构建、XLSX/CSV 同步检查、项目校验与 `git diff --check` 通过。
- [ ] 用户在 PIE/Shipping 可见界面确认两个刷新按钮、剩余次数和实际交互可用。

## Step 0 门禁

- 基线分支/提交：`origin/main@6b3262fa574d9ea684cad1c0ce8d6c8c96f25fc6`。用户确认吸收 Plan63 的怪物出生预警修复；其 `ReEchoGameMode.cpp` 改动位于出生批次代码，与本 Plan 商店端点区段分离，模块文档和最终预构建包需要组合适配。
- 引擎/构建可用性：当前未检测到 `UnrealEditor` 进程；执行构建/自动化前使用 Git common-dir 锁。
- 现有聚焦测试结果：沿用基线已发布 Plan 97/99 的证据仅作定位，不作为本 Plan 最终证据；实现后重跑聚焦检查。
- 共享契约 / 难合并资源风险：生产 XLSX 是二进制共享源，必须与新增 CSV/schema/manifest 同候选发布；推送前重新 fetch 并审计远端 XLSX 差异。
- 基线损坏时的停止条件：若权威同步、Editor 构建或既有聚焦商店测试在未修改候选上失败，先区分基线问题与本 Plan 回归，不以削弱断言绕过。

## 实现提纲

1. 将刷新规则迁移进生产 XLSX 单行表，扩展同步映射、manifest/schema、CSV Reader 和不可变快照。
2. 拆分武器/符文与各卡组候选卡位的刷新序列/次数，升级保存版本并实现旧存档迁移、跨关重置和单卡位原子刷新命令。
3. 改造商店主按钮投影，只刷新武器/符文并显示剩余次数；保留免费刷新优先抵扣和禁刷新规则。
4. 为付费卡组三选一的每张实际卡牌增加独立刷新按钮、剩余次数、价格和带候选槽位索引的类型化请求；GameMode 成功后只原位重注入该槽的新同级候选。
5. 补充数据、Run、Save、UI 和跨域测试，维护模块文档，完成格式化、构建、静态检查和人工手测说明。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| XLSX/CSV | `python scripts/data/sync_xlsx_to_csv.py --check` | 生产工作簿、生成 CSV、schema/manifest 无漂移 |
| 数据/运行时 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Shop` | 独立计数、扣费、失败原子性、候选隔离通过 |
| UI | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.Shop` 与卡牌选择聚焦测试 | 两类按钮、剩余次数、命令路由通过 |
| Save | 对应 `ReEcho.Run.Save*` 聚焦测试 | 新旧存档迁移、同页恢复、跨关重置通过 |
| C++ | `.clang-format`；`scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0 |
| 静态 | `python scripts/validate_project.py`；`git diff --check` | 项目/源码/文档不变量通过 |
| 人工 | PIE 第2/4关：分别消耗两个刷新入口 | 可见次数、扣费、候选隔离和禁用表现正确 |

## 执行记录

### 变化

- 2026-08-25：用 Spreadsheet 技能按 Excel 实际可见视图核对外部源表；`投放系统!A27:C30` 行列均未隐藏，值为卡组卡位限制 1、武器符文限制 2、两类刷新价格均 5。现代码仍用共享 `ShopRefreshSequence`，且 `ReEchoShopRefreshPrice` 为 10，需按本 Plan 拆分与迁移。
- 2026-08-25：策划临时明确“三张分别分配”是三选一页面每张卡下方各有一个刷新按钮，每个卡位一次；只替换所点卡位，刷新结果排除当前页面卡牌、被替换卡牌和全部已获得卡牌，无替换候选时禁用且不消费。
- 2026-08-25：远端发布 `plans/100-weapon-vfx-placement-and-shared-sizing.md` 占用 Plan100。用户确认吸收 `df419ebc` 并将本任务顺延为 Plan101；当前只存在 Plan 文档编号冲突，无商店代码冲突。

### 证据

- 待实现后填写。

### 剩余风险

- 生产 XLSX 为二进制共享文件；发布前必须重新审计远端是否变化。

### 人工验收结果/请求

- `PendingBeforeClose`：实现后由用户手测可见刷新按钮、剩余次数与候选隔离。

### 架构文档审阅结果

- 待实现后逐项填写。
