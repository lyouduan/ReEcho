# Plan 105 - 程序 - 卡牌实际结果浮窗

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Closed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`Passed`；用户于 2026-08-25 确认手测无误并授权发布到远端 `main`。
- 最终集成基线：`origin/main@e7491d2dc28fe80253cc5536e3f25c7d081331ce`；已吸收 Plan104 与其后的主分支更新。
- 本地实现方式：`plan/105-card-outcome-tooltips`；`C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan105-card-outcome-tooltips`。
- 依赖 / 阻塞：Plan104 正在并行收敛武器集合；两者逻辑独立，但可能共同修改 `ReEchoRunSubsystem.cpp`、Run/Save 测试、模块文档和最终预构建包，发布实现前必须审计并组合实际远端结果。
- Writes:
  - `plans/105-card-outcome-tooltips.md`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardTypes.h`
  - `Source/ReEchoCards/Private/Cards/ReEchoCardRuntime.cpp`
  - `Source/ReEcho/{Public,Private}/Run/ReEcho{RunSaveGame,RunSubsystem,ShopCatalog}.*`
  - `Source/ReEcho/{Public,Private}/UI/ReEchoInventoryShopWidget.*`
  - `Source/ReEcho/Private/Tests/ReEcho{Trait,Shop,SaveGame}Tests.cpp` 中实际受影响文件
  - `Source/ReEchoCards/Private/Tests/` 中实际受影响文件
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `Design/Data/ReEchoData.xlsx`
  - `Content/Data/{parts,part_effects}.csv`
  - 最终 FullRebuild 刷新的 `Binaries/Win64/` 精选预构建包
- Stable Reads:
  - `Content/Data/{cards,card_effects}.csv`
  - `plans/104-four-weapon-roster.md`
  - `shared/CODEBASE_MAP/{ARCHITECTURE,README}.md`
- 影响模式：`SharedContract`。扩展 Cards 可保存运行态、Run 商店只读投影和 SaveVersion，并改变商店已拥有卡牌 Tooltip 契约。
- 兼容承诺 / 下游操作：不改变卡牌抽取、授予、结算、属性、货币或随机序列；结果摘要与同一原子玩法事务一起更新。旧存档继续可读，不伪造无法从旧状态反推的历史收益；仍可从现有 `EconomyPenalty` 和未完成追踪状态恢复可证明的摘要。
- 明确排除：不修改卡牌数值/描述、卡牌 icon、三选一页面、武器/符文 Tooltip、每次攻击或每次元素反应等瞬时事件日志；不为展示结果在 Widget 复制卡牌 ID 玩法分派。用户追加要求仅禁用 `群攻陨星剑刃` 与 `投掷召回链` 的目录、商店资格和效果记录，不改写其他符文逻辑。

## 锁定目标

1. 商店装配室中已拥有卡牌的小 icon 保留现有“名称 + 策划描述”浮窗；若该卡存在已解析的随机、派生或累计实际结果，在原浮窗正下方紧贴显示第二个独立描边面板。没有实际结果的卡不显示空面板。
2. `代价丰收 G_3_17` 明确显示本局随机到的代价：禁止商店刷新、禁止购买额外卡牌组或敌人不再掉落时间碎片，且与真正生效的 `EconomyPenalty` 一致。
3. `猎杀淬炼 G_2_05`、`元素虹吸 G_2_06` 在等待目标关时显示待结算状态；对应下一关完成后分别显示本次实际永久增加的物攻/元攻数值，零收益也必须显示 `+0`，不得在清空追踪计数时丢失结果。
4. 建立统一的类型化“卡牌实际结果摘要”，覆盖当前生产卡中其他同类稳定结果：随机属性取舍、随机赠卡、血肉铸锋一次性生命上限收益、战利回流实际刷新次数，以及商店契约/殉身回响/灵魂共振/万象共鸣产生的累计永久收益。随机赠卡摘要使用实际获得的卡牌显示名；瞬时每击/每脉冲结果不进入持久浮窗。
5. Cards 是结果状态唯一权威：随机选择、延迟结算和累计成长在原事务内写入类型化状态；Run 只把目录显示名和状态格式化为只读 `OutcomeText`，UI 只决定第二面板是否显示及排版。任何层都不得解析中文卡牌描述决定结果。
6. 结果摘要随 SaveVersion 19 保存/恢复。v18 及更早存档保持兼容：可由现有字段证明的代价/待结算状态确定性迁移，无法反推的已结算历史保持“无摘要”而不是伪造数值；发生下一次对应事件后开始准确记录。
7. 在权威 `ReEchoData.xlsx` 中禁用 `群攻陨星剑刃` 与 `投掷召回链`：父符文必须 `Enabled=false`、`ImplementationStatus=Disabled`、`ShopEnabled=false`、`ShopPrice=0`，其全部 `part_effects` 同步禁用；生成 CSV 必须与工作簿一致。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoCards`（`AREA-Cards`）、`MOD-ReEcho`（`AREA-Run`、`AREA-Tests`）、文档型 `MOD-ReEchoUI`（`AREA-UI`）。
- 对应模块文档：维护 `MOD-ReEchoCards.md`、`MOD-ReEcho.md`、`MOD-ReEchoUI.md`，均已加入 Writes。
- 设计意图：把“规则描述”和“本局真实解析结果”分成稳定的两层展示；真实结果继续由 Cards 事务产生和持有，避免 Widget 根据卡牌 ID 或描述猜测当前效果。
- 权威状态与依赖：在 `FReEchoCardRuntimeState` 增加可序列化的类型化结果记录；Cards 的依赖方向不变。Run 将记录与 Card Catalog 组合成展示文本并放入商店只读 Offer；UI 不获得可写 CardState。
- 决策记录：采用一个 Tooltip 根中的上下两个有独立边框的面板，以满足“紧贴当前浮窗下面”并继续使用 UE 单一 `SetToolTip` 生命周期；采用类型化结果而非预先保存中文，保证存档不固化 UI 文案；生产二、三级动态卡当前不可重复投放，重复一级随机赠卡按同一来源卡汇总实际赠卡历史。
- 相关文档同步范围：必审 `ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoCards.md`、`MOD-ReEchoUI.md`；模块拓扑和索引预计不变，若审阅确认不变则只在执行记录注明。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅依赖拓扑与状态所有权；
  - `README.md`：待审阅 AREA 路由；
  - `MOD-ReEcho.md`：待同步 Run/Save/Shop 投影事实；
  - `MOD-ReEchoCards.md`：待同步结果状态与事务边界；
  - `MOD-ReEchoUI.md`：待同步卡牌双层 Tooltip 契约。

## 锁定验收

- [x] 代价丰收浮窗第二面板准确显示实际三选一代价，并在存读档后保持一致。
- [x] 猎杀淬炼、元素虹吸在等待期显示待结算，完成目标关后显示实际 `物攻 +N` / `元攻 +N`，包括 `+0`，且数值与 StatBlock 的真实变化一致。
- [x] 审计列出的其他随机/派生/累计稳定结果均通过统一类型化摘要展示；没有结果的普通卡不生成第二面板，UI/Run 无按中文描述分派。
- [x] 旧存档迁移、原子失败、随机确定性、CardState/Recording/Save 恢复与既有玩法数值无回归。
- [x] 群攻陨星剑刃与投掷召回链及其四条效果已从权威表和生成 CSV 禁用，且不再具备商店投放资格。
- [x] 第二面板紧贴原浮窗下方，描边、宽度、自动换行和 DPI 下可读性已由用户 PIE 验收。
- [x] 聚焦 Cards/Trait/Shop/Save 自动化、Development Editor 构建、项目校验和 `git diff --check` 通过；发布前最终集成候选 FullRebuild 与预构建检查通过。
- [x] 未提交精选 `GIT_RULES.md` 允许列表外的 UE 生成物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@f0801c5a`；包含已关闭 Plan101、Plan103 实现与已发布 Plan104 规格。
- 引擎/构建可用性：UE 5.8 可用；当前存在交互式 Editor，任何需要独占的构建/自动化前先请用户关闭并使用 Git common-dir 锁。
- 现有聚焦测试结果：本轮只读审计确认玩法数值路径存在；实现后必须重新运行聚焦证据，不复用旧构建。
- 共享契约 / 难合并资源风险：Plan104 的本地实现正修改 Run/Save 测试、`ReEchoRunSubsystem.cpp`、模块文档和最终预构建包；本 Plan 可并行开发，但进入 main 前必须组合适配并重跑受影响证据。
- 基线损坏时的停止条件：若未修改候选上的 Card grant/encounter end、Save v18 或商店已拥有卡牌投影测试失败，先区分基线问题，不以删断言或重写结果掩盖。

## 实现提纲

1. 在 Cards 定义通用结果类型与查询所需状态，让随机授予/属性取舍/经济代价、延迟结算和累计永久成长在原事务内记录结果。
2. 升级 SaveVersion 19 与兼容迁移，覆盖 Run、Recording 中嵌入的 CardState，验证读档不重摇随机结果、不丢失待结算或已结算摘要。
3. 扩展 `FReEchoShopOffer` 的只读结果文本，由 Run 根据类型化结果和目录显示名生成，不在 Widget 解释卡牌行为。
4. 将现有 `BuildSlotTooltip` 改为可选双面板组合；第二面板只在非空结果文本时创建，并保持现有卡牌/武器/符文调用兼容。
5. 补充 Cards、Trait、Shop、Save/UI 测试，维护模块文档和执行记录，完成构建、静态检查与用户 PIE 验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Cards 纯规则 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Cards` 与相关 `ReEcho.Traits` | 随机、延迟、累计结果与玩法事务一致 |
| Run/Save | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Run` | Save v19、旧版本迁移、Recording 嵌入状态恢复通过 |
| Shop/UI | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Shop`、`ReEcho.UI.Shop` | Offer 携带结果文本；空结果单层、非空结果双层 |
| C++ | `.clang-format`；`scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0并刷新开发预构建包 |
| 静态 | `python scripts/validate_project.py`；`git diff --check` | 项目、文档和文本不变量通过 |
| 数据 | `python scripts/data/sync_xlsx_to_csv.py --check`；核对两条 `parts.csv` 与四条 `part_effects.csv` | 工作簿/CSV 无漂移，父符文、商店资格与全部效果均禁用 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`；`python scripts/ue/prebuilt_editor.py check` | 最终集成源码与精选预构建包匹配 |
| 人工 | 商店悬停代价丰收、猎杀淬炼、元素虹吸及至少一张其他动态结果卡 | 第二面板位置、内容、换行和 DPI 可读性正确 |

## 执行记录

### 变化

- 2026-08-25：策划反馈已拥有卡牌 icon 的浮窗只显示规则描述，未显示本局实际随机/派生结果；明确要求新增紧贴原浮窗下方的结果面板，并覆盖所有同类卡牌而非只硬编码三张。
- 2026-08-25：只读审计确认 `G_3_17` 的 `EconomyPenalty` 已持久化但未投影；`G_2_05/G_2_06` 结算时直接修改属性后清空计数，没有保存实际增量；商店 `FReEchoShopOffer` 与 `BuildSlotTooltip` 当前也只有单一 `EffectText`。
- 2026-08-25：Cards 已增加可保存的类型化结果状态，原子记录随机代价/属性取舍/赠卡、延迟结算和累计永久收益；Run 投影 `OutcomeText`，UI 在旧面板下按需生成第二面板。
- 2026-08-25：SaveVersion 升至 19，v18 只迁移可证明的代价/待结算状态；已增加实际代价、物攻/元攻结算、随机属性取舍、存读档/旧版迁移与双面板 Widget 结构自动化。
- 2026-08-25：按用户追加要求在权威工作簿禁用 `群攻陨星剑刃` 与 `投掷召回链`，同步关闭两条父符文的商店资格/价格和四条运行时效果，并重新生成 `parts.csv`、`part_effects.csv`。

### 证据

- `scripts/ue/Build-Editor.cmd -Configuration Development`：通过，UHT/UBT 退出 0，并刷新 7 个精选 Editor 模块。
- `ReEcho.Cards`：5/5 通过；覆盖精确赠卡、累计刷新/购买成长和原有原子性。
- `ReEcho.Traits`：10/10 通过；新增 `ResolvedOutcomesPersistAndProject` 通过，覆盖代价丰收、孤注一掷、猎杀淬炼、元素虹吸、SaveVersion 19 存读档和 v18 严格迁移。
- `ReEcho.UI.Shop`：3/3 通过；已验证无结果单面板、有结果双面板与结果文本。
- `ReEcho.Shop` 与 `ReEcho.Run`：通过，后者 18/18，含 SaveSnapshot 及 v8/v10/v15 迁移回归。
- `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check`：通过。
- `python scripts/data/sync_xlsx_to_csv.py`：通过工作簿保护、Schema、引用和生成字节一致性检查；两条父符文均为 `Enabled=false / Disabled / ShopEnabled=false / ShopPrice=0`，四条效果均为 `Enabled=false`。
- 数据变更后 `ReEcho.Shop`：11/11 通过，含武器符文投放、购买、稳定页面、背包切换与独立刷新回归。
- 最终集成候选 `ReEcho.Data`：6 项中 4 项通过；另外两项命中本任务外的既有数据/旧断言（生产卡池断言 39、实际 37；Rabbit 双形态 presentation-only 断言与当前权威数据不符）。本次 diff 不修改 `cards.csv` 或敌人数据，`parts.csv` 仅切换两条既有命名符文的启用/商店字段；权威同步与项目 Schema 校验均通过，未借本任务扩改无关基线测试。
- 最终集成候选已完成 `Development -FullRebuild`；`ReEcho.Cards` 5/5、`ReEcho.Traits` 10/10、`ReEcho.UI.Shop` 3/3、`ReEcho.Shop` 11/11、`ReEcho.Run` 18/18 通过；项目校验、XLSX/CSV 同步检查、预构建检查与 `git diff --check` 通过。
- 首次 Development 编译暴露 `FReEchoShopOffer` 聚合初始化兼容问题，已将可选字段移至结构体末尾并重跑通过；首次 UI 自动化发现一处旧根节点断言，已改为单/双面板明确断言并重跑通过。

### 剩余风险

- Plan104 与其后的主分支更新已在 `origin/main@e7491d2d` 上完成重放、冲突审计和 FullRebuild；不再作为待处理集成风险。
- 旧存档中已完成且已清空的历史派生收益无法可靠反推，兼容迁移只能保留可证明状态，不能伪造归因。

### 人工验收结果/请求

- `Passed`：用户于 2026-08-25 确认手测无误；代价丰收、猎杀淬炼、元素虹吸及通用实际结果副浮窗的内容与排版通过，禁用符文不再投放。

### 架构文档审阅结果

- `ARCHITECTURE.md`：已审阅；Cards 状态所有权与现有 `ReEcho → ReEchoCards` 依赖不变，无需修改拓扑。
- `README.md`：已审阅；`AREA-Cards` / `AREA-Run` / `AREA-UI` 路由不变。
- `MOD-ReEchoCards.md`：已同步 `ResolvedOutcomes` 权威与原子事务边界。
- `MOD-ReEcho.md`：已同步 SaveVersion 19 与严格旧版迁移。
- `MOD-ReEchoUI.md`：已同步可选双面板 Tooltip 及 UI 只读边界。
