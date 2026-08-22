# Plan 67 — Shop — 投放系统重做（固定符文槽/卡牌按表/置灰/即装/分区刷新）

## 协调

- Planner 负责人：Gavyn-side AI（兼任秘书，按 shared/PLANNER_RULES.md 与 shared/SECRETARY_RULES.md）。
- Executor 负责人：Gavyn-side AI（程序身份，须经人工确认后动手）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Proposed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（商店交互、置灰手感、即装替换需 PIE 人工确认）。
- 本地规划 / 实现基线：worktree `ReEcho-plan67-shop-drop` @ `826f9bd`（plan/67-shop-drop，含 Plan67 完整交付；落后 origin/main，实现阶段再吸收最新 main）。
- 本地实现方式：独立 worktree `ReEcho-plan67-shop-drop`（避免在主工作树做 plan 类改动）。
- 依赖 / 阻塞：需先在 `ReEchoData.xlsx` 投放系统表与武器符文表落地两处新数据（见实现提纲 Step 1），再改运行时。
- **策划数据真源（2026-08-21 用户提供）**：`策划数据源/【开普勒】回响数值与构筑体系.xlsx` 的 **`投放系统` sheet**（A1:E47，全部可见行）。本 Plan 的所有投放数值（价格区间、关卡投放等级、刷新次数、三槽概率、怪物掉落时间碎片）以此表为唯一权威；实现时须导入 `ReEchoData.xlsx` → CSV，不得硬编码于 C++/JSON。分类依据：`武器插槽C` sheet 第 1 列 `武器类型`（`通用` / 具体武器名：匕首/弓/镰刀/长剑/枪/法杖/鞭），对应 parts.csv 的 `WeaponTypeId`（`Any`=通用 / `LongSword`/`Staff`/`Bow`/`Scythe`/`Gun`/`Whip`），支撑左槽"通用符文"与中/右槽"当前武器 vs 其他武器"判定。
- **数据真源纪律（2026-08-22 Gavyn 强调）**：**以策划文件夹的 xlsx 为准，CSV 随表改**。即 `Design/Data/ReEchoData.xlsx`（由 `sync_xlsx_to_csv.py` 导出）→ `Content/Data/*.csv` 是唯一发布链；不得以现网 CSV 反推覆盖策划表。凡是表里缺的字段/行，先改 xlsx 再 sync 出 CSV，不手改 CSV。
- **本地规划 / 实现基线（2026-08-22 更新）**：worktree `ReEcho-plan67-shop-drop` @ 当前分支头（plan/67-shop-drop，已吸收 origin/main a85f909；详见「执行记录」）。
- Writes：`Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`、`Public/Run/ReEchoShopCatalog.h`、`Private/UI/ReEchoInventoryShopWidget.cpp`、`Private/ReEchoGameMode.cpp`、`Private/Tests/ReEchoShopTests.cpp`、`scripts/data/sync_xlsx_to_csv.py`、`ReEchoData.xlsx` 相关 sheet 与衍生 CSV、WBP `ReEchoInventoryShop`。
- Stable Reads：`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoUI.md`（文档型）、`ARCHITECTURE.md`（`MOD-ReEchoUI` 非 Runtime Module 标注）。
- 影响模式：`SharedContract`（改变 `FReEchoWeaponPartShopView` 结构、刷新计数契约、配件分类字段，影响 UI/测试/数据三处消费方）。
- 兼容承诺 / 下游操作：旧的"整页洗牌 + 保存配置"契约被替换；`ReEcho-fix-shop-owned-parts` worktree 的 draft/save 逻辑**不并入**本 Plan（其"已拥有未装备可重装"意图以即装方式重做，不沿用 draft）。
- 明确排除：
  - 战斗结束免费三选一（Excel 列A）的等级限制**不在本 Plan 范围**（仅按用户需求改商店；免费投放沿用现有 `GenerateTraitCardOffers`，但其授予结果仍计入"获得过"排除集）。
  - 不新增背包系统（复用 `OwnedPartIds`）。
  - 不改武器本身槽位数量/映射规则（`SlotProfiles` 不变）。

## 锁定目标

玩家在商店看到且行为发生改变的结果：

1. **配件区改为 Excel 左/中/右 三固定槽**：
   - 左槽只出"通用符文"（PartCategory=Universal）。
   - 中/右槽按 **70% 当前武器符文 + 15% 其他武器（非符文）+ 15% 其他武器符文** 三态加权抽取（分类见下，依赖新增 `PartCategory`）。
   - 每个槽固定展示**一条**报价；购买后该槽**原地置灰**（不可再点），其余槽不动。
2. **卡牌区按当前关卡 xlsx 投放表决定槽数与等级**：每关列出几个等级就有几个槽，每槽**钉死一个等级**（如第4关列 2/3 级 → 2 个槽，一槽必出 2 级、一槽必出 3 级；第1关不投放 → 0 槽）。
3. **获得过的配件/卡牌不再出现**（任何来源：商店购买、战斗结束三选一、奖励、GM 等）；**一级卡牌例外**——可反复获得、可反复在商店出现。
4. **购买即装备、无保存配置**：买配件直接装进对应武器槽，被替换下的旧配件回落 `OwnedPartIds`（背包）；点"已拥有未装备"区的配件直接重装（同样即装即替）。删除现有 draft/保存配置全流程。
5. **两个独立刷新**：卡牌区刷新（限 1 次，耗 5 时间碎片）、符文/配件区刷新（限 2 次，耗 5 时间碎片）；各自独立计数、各自只重洗本区。

## 策划数据真源（开普勒 · `投放系统` sheet，A1:E47）

> 来源：`策划数据源/【开普勒】回响数值与构筑体系.xlsx` → `投放系统` sheet（2026-08-21 用户提供，全部可见行）。以下为权威数值，实现时导入 `ReEchoData.xlsx` → CSV。

### 表1 · 卡牌组投放配置（关卡 → 战斗后免费三选一等级 / 商店卡牌组三选一等级）

| 关卡 | 战斗后免费投放(三选一) | 商店卡牌组投放(三选一) |
|---|---|---|
| 第1关 | 不投放 | 不投放 |
| 第2关 | 2级 | 1级 |
| 第3关 | 3级 | 1级 |
| 第4关 | 1级 | 2级、3级 |
| 第5关 | 2级 | 不投放 |
| 第6关 | 3级 | 1级、2级 |
| 第7关 | 2级 | 1级、3级 |
| 第8关 | 不投放 | 不投放 |

> 注：本 Plan **仅落地"商店卡牌组投放"列**（D1）；"战斗后免费三选一"列不在本 Plan 范围（见协调·明确排除），但授予结果仍计入"获得过"排除集。

### 表2 · 商店物品售价配置（按区间随机）

| 售卖物品 | 价格区间 |
|---|---|
| 1级卡牌组 | 30–50 |
| 2级卡牌组 | 100–120 |
| 3级卡牌组 | 150–200 |
| 原初之晶 | 10–20 |
| 潮汐/森林/焚绝/鸣雷/棱镜之晶 | 20–30 |
| 棱镜水晶 | 100–120 |
| 其他武器符文 | 30–40 |
| 其他武器 | 10–20 |

> 实现要点：售价由现有固定值 `Part.ShopPrice` / `Tier*10` 改为**按物品类别取区间随机**。类别映射依赖 `武器符文C` 的 `武器类型` 列 + 卡牌等级。

### 表3 · 刷新次数限制

| 项 | 卡牌组刷新 | 武器符文刷新 |
|---|---|---|
| 限制刷新次数 | 1 次 | 2 次 |
| 刷新消耗时间碎片 | 5 | 5 |

> 对应 D3：拆为 `CardShopRefreshCount`(限1) / `RuneShopRefreshCount`(限2)，各扣 5 时间碎片。

### 表4 · 商店武器符文刷新概率与占位（左/中/右三槽）

| 槽位 | 内容逻辑 |
|---|---|
| 左槽位 | 必定刷新【通用】武器符文；当所有【通用】武器符文都已被购买后，改为与中槽位一致 |
| 中槽位 | 70% 当前持有武器对应符文(除通用)；15% 一把其他武器；15% 其他武器对应符文 |
| 右槽位 | 与中槽位一致 |

> 对应 D2/D4：配件区改左/中/右三固定槽；严格 4 类 `PartCategory` = Universal / CurrentWeaponRune / OtherWeapon / OtherWeaponRune。分类依据 `武器符文C` sheet `武器类型` 列（`通用` / 具体武器名）。

### 表5 · 怪物掉落时间碎片配置（关卡 × 怪物类型）

| 关卡 | 近战小怪 | 远程小怪 | 精英怪 |
|---|---|---|---|
| 第1关 | 1–2 | 1–2 | / |
| 第2关 | 1–2 | 1–2 | / |
| 第3关 | 3–5 | 3–5 | 5–8 |
| 第4关 | 3–5 | 3–5 | 5–8 |
| 第5关 | 5–7 | 5–7 | 8–10 |
| 第6关 | 5–7 | 5–7 | 8–10 |
| 第7关 | 7–9 | 7–9 | 10–12 |
| 第8关 | 7–9 | 7–9 | 10–12 |

> 说明：此表属掉落模块（另一消费方），本 Plan 仅记录其为权威来源；刷新消耗的"时间碎片"货币与此掉落同源，供表3引用。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`（Runtime 商店/构筑数据）、`MOD-ReEchoUI`（文档型，商店 Widget 重写）。`MOD-ReEchoAudio`/`MOD-ReEchoCombat` 不受影响。
- 对应模块文档：`modules/MOD-ReEcho.md`（需更新商店报价生成与刷新契约）、`modules/MOD-ReEchoUI.md`（需更新商店交互契约）。已加入 Writes。
- 设计意图：把"商店投放"从"随机洗牌池"改造为"按 Excel 投放表的确定性固定槽"，使投放可策划、可复现、可验收；并去掉 draft/保存配置这一层间接，装备结果即时可见。
- 权威状态与依赖：本 Plan 改变 `FReEchoWeaponPartShopView` 的报价结构（由扁平 `Offers` 改为 `SlotOffers`+`CardSlotOffers`）与刷新计数（`ShopRefreshSequence` 单计数拆为卡牌/符文双计数）。数据权威为 `ReEchoData.xlsx` 投放系统表 + 武器符文表。
- 决策记录：
  - **D1 卡牌槽结构（q0）**：选"以 Excel 三选一表为准"，但落地为"3 个固定槽、每槽只随机表中等级"——即每关按表列等级数量决定槽数、每槽钉死一个等级（非三选一弹窗）。理由：用户明确"第四关两个槽一必2级一必3级、第一关零槽"。
  - **D2 配件置灰结构（q1）**：改造成 Excel 左/中/右 三固定槽（非保持动态列表）。理由：用户明确选"改造成左/中/右三固定槽"。
  - **D3 刷新（q2）**：两个独立刷新按钮（卡牌1/符文2，各 5 碎片）。理由：最贴合 Excel；满足用户"刷新让整页重洗"的字面（各区内整页重洗）。
  - **D4 符文两 15%（q3）**：严格 4 类 `PartCategory` = Universal / CurrentWeaponRune / OtherWeapon / OtherWeaponRune，加数据字段。理由：用户明确要"严格区分为 当前武器符文+其他武器(不是符文)+其他武器符文"。
  - **D5 背包（q4）**：复用 `OwnedPartIds`，不新建系统；UI 暴露"已拥有未装备"区可即装重装备。
  - **D6 获得范围/刷新（q5）**：任何来源获得均计入排除集；刷新限次按 Excel。
- 相关文档同步范围：`ARCHITECTURE.md`（若提及商店报价结构需更新）、`modules/MOD-ReEcho.md`、`modules/MOD-ReEchoUI.md`；其余 MOD 不相关，省略并说明。
- 关闭前逐项填写审阅结果：（实现后回填）

## Q5–Q8 澄清决策（2026-08-22 Gavyn 回复，实现前必读）

> 背景：Step 1 数据层落地前向 Gavyn 提出的 4 个确认问题。以下为答复，构成对上文 D1–D6 / Step 1 的补充与修正；与上文冲突处以本节为准。

### Q5 — 开普勒表里"武器符文分类"是怎么写的？

- **核查结论**：开普勒表 `武器插槽C` sheet（parts 真源）表头为 `武器类型 | 插槽位 | 宝石/配件名称 | 效果(对程序) | icon | 符文介绍【文案】 | 词条`。第 1 列 `武器类型` 取值为 `通用` 或具体武器名（匕首/弓/镰刀/长剑/枪/法杖/鞭）。
- 这与已导出的 parts.csv `WeaponTypeId` 完全对应：`通用`→`Any`，具体武器名→`LongSword`/`Staff`/`Bow`/`Scythe`/`Gun`/`Whip`。
- **决策**：**不再新增 `PartCategory` 列**（撤回 Step 1 中"新增 PartCategory 字段"的写法）。三槽分类直接复用现有 `WeaponTypeId`：
  - `Any` = 通用符文 → 左槽候选。
  - 具体武器类型 = 该武器专属符文 → 用于判定"当前武器符文"vs"其他武器符文"。
- D4 的 4 类语义（Universal / CurrentWeaponRune / OtherWeapon / OtherWeaponRune）仍保留，但改为**运行时按 `WeaponTypeId` + 当前武器动态计算**，不落存储字段。

### Q6 — 售价区间是否需要列，且随机生成？

- **决策**：**需要**。在 xlsx 投放系统表落地"售价区间"结构化列（物品类别 → 最低价 / 最高价两列），运行时按类别取 `[Min,Max]` **均匀随机**生成实际售价。
- 映射策划表表2 的 8 个品类（1/2/3 级卡牌组、原初之晶、元素之晶、棱镜水晶、其他武器符文、其他武器）。
- 取代现有固定值 `Part.ShopPrice` / `Tier*10`（`ReEchoShopRefreshPrice` 刷新费另由表3 权威，见 D3）。
- CSV 侧需新增对应列（如 `ShopPriceMin` / `ShopPriceMax`），由 sync 从 xlsx 导出；**价格随机逻辑在 C++ 运行时**，不在 CSV 存结果。

### Q7 — 数据真源与"其他武器"含义

- **真源纪律**（同协调段新增条）：**以策划文件夹 xlsx 为准，CSV 随表改**。现网 CSV 与 xlsx 不一致时，以 xlsx 为准重新 sync，不手改 CSV 覆盖表。
- **"其他武器"语义**：指**开局未选的那几把武器**对应的配件。即中/右槽 15% "一把其他武器" = 从玩家本局未选择的武器类型中抽一把，再从其配件池取一件。
- **决策**：因此"其他武器/其他武器符文"的候选集 = `WeaponTypeId != Any && WeaponTypeId != 当前武器类型`（其他武器符文）与 `WeaponTypeId != Any && WeaponTypeId == 某非当前武器类型`（其他武器本体件）。需确认 parts.csv 是否含"武器本体件"行；若当前只有符文件，则"其他武器"项可能需策划补行或降级为"其他武器符文"。**实现前先与 Gavyn/策划确认"其他武器"是否有独立配件行**（开放项，见交接说明）。

### Q8 — cards 现状核查

- **核查结论**：`Content/Data/cards.csv` 共 42 行，表头含 `Id, SourceWorkbookId, Tier, DisplayName, Description, Tags, PromotionRoleId, OfferGroup, Enabled, Offerable, StackPolicy, ConflictPolicy, ReviewStatus, DisabledReason`。
  - `Tier` 分布：1→8、2→13、3→18、0→3（共 42）。
  - `OfferGroup` 分布：`Trait`→39、`Forge`→3。
- **决策**：卡牌等级/分组字段**已就绪**，无需新增卡牌字段。D1 的"商店卡牌组投放等级"只需在 xlsx 投放系统表新增结构化"关卡→等级列表"列（如第4关=`2,3`、第1关=空），sync 出新 CSV（如 `ShopDropTable.csv`）；运行时按当前 `EncounterIndex` 查表得等级列表 → 每级一槽。一级卡（`Tier==1`）可重复获得/出现（D3 排除例外）。

## 现状（2026-08-22 交接快照）

- **分支/工作树**：`ReEcho-plan67-shop-drop`（分支 `plan/67-shop-drop`），已吸收最新 `origin/main`（a85f909）。主工作树 `ReEcho/main` 保持干净。
- **已完成**：
  - Plan67 文档完善 + 策划数据源文件夹入库并已合入远端 main（纯文档，fast-forward，豁免 FullRebuild）。
  - 商店代码现状已摸清：`GetWeaponPartShopView()`（`ReEchoRunSubsystem.cpp` ~790-903）产出**扁平 `Offers`** + `ShuffleOffers` 随机池；`MakeWeaponPartOffer`（Price=`Part.ShopPrice`）、`MakeBuildCardOffer`（Price=`Tier*10`）；`PurchaseShopItem`（~1466+）；UI `ReEchoInventoryShopWidget` 用 `WeaponPartOfferButtons[3]` 平铺。
  - 三槽分类依据已就绪：parts.csv `WeaponTypeId`（`Any`=通用 / 具体武器名），**无需新增 PartCategory 列**（Q5）。
  - 卡牌字段已就绪（Q8）：cards.csv 已有 `Tier`/`OfferGroup`。
- **尚未开始（下一步 = Step 1 数据层）**：
  1. xlsx 投放系统表新增"售价区间"两列（Min/Max，Q6）+ "关卡→商店卡牌投放等级列表"结构化列（D1/Q8）。
  2. `sync_xlsx_to_csv.py`：导出售价区间列 + 新 `ShopDropTable.csv`；注意 `LOCKED_REFERENCE_SHEETS` 与 AUTHORING_LIST_VALIDATION_COLUMNS。
  3. `FReEchoCsvPartRow` / `ReEchoCsvDataRegistry`：售价区间字段（可选，亦可运行时查表）；`ShopDropTable` 读取结构。
  4. 构建（增量 Development）使 Binaries 与源码一致后，sync 才会把 xlsx 新列真正发布进 CSV（见记忆 13038418）。
- **待确认开放项**：
  - **"其他武器"是否有独立配件行**（Q7）：当前 parts.csv 是否含"武器本体件"（非符文件）？若无，需向策划确认"其他武器"项如何落地（补行 or 并入"其他武器符文"）。
  - 售价区间的 8 个品类与 `WeaponTypeId`/卡牌 Tier 的确切映射规则（尤其"棱镜水晶"vs"棱镜之晶"、元素之晶按元素还是统一）。

## 交接说明（给下一个模型）

- **你是谁**：继续 Plan67 的实现者。Gavyn（程序身份）准备换模型继续，故此处做交接。角色规则见 `shared/PLANNER_RULES.md` / `EXECUTOR` / `SECRETARY_RULES.md`；提交身份 `JosephLE910 + Codex`，标签 `[PROGRAMMER]`。
- **工作树纪律**：所有 plan 类改动只在 `ReEcho-plan67-shop-drop` 内做；主工作树 `ReEcho/main` 只 fast-forward。本地 debug 循环用增量构建 `scripts/ue/Build-Editor.cmd -Configuration Development`（编辑器关闭时），推 origin/main 前才需 `-FullRebuild`。
- **数据真源铁律**：xlsx → sync → CSV。永远以策划 xlsx 为准，CSV 随表改，绝不手改 CSV 反推（Q7）。改了 cpp 必须先增量构建刷新 Binaries，sync 才会把 xlsx 新列真正发布进 CSV。
- **下一步建议顺序**：
  1. 先与 Gavyn 敲定 Q7 开放项（"其他武器"是否有独立配件行）——这会决定中/右槽 15% 的候选来源。
  2. 落地 Step 1 数据层：xlsx 加售价区间(Min/Max)列 + 关卡投放等级列表列；`sync_xlsx_to_csv.py` 导出；新增 `ShopDropTable.csv`。
  3. 增量构建 → sync 发布 CSV → `validate_project.py`。
  4. 再进入 Step 2 运行时报价重写（扁平 Offers → 左/中/右三槽 + 卡牌钉死等级槽，见上文实现提纲）。
- **关键文件**：plan 文档（本文件）、`Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`（`GetWeaponPartShopView`/`MakeWeaponPartOffer`/`MakeBuildCardOffer`/`ShuffleOffers`/`PurchaseShopItem`）、`Public/Run/ReEchoShopCatalog.h`（`FReEchoWeaponPartShopView`）、`Private/UI/ReEchoInventoryShopWidget.cpp`、`scripts/data/sync_xlsx_to_csv.py`、`Content/Data/parts.csv`/`cards.csv`、`Design/Data/ReEchoData.xlsx`、`策划数据源/【开普勒】回响数值与构筑体系.xlsx`（`投放系统` sheet = 权威数值）。
- **勿踩坑**：openpyxl `iter_rows` 含隐藏行，须查 `row_dimensions[r].hidden`；中文路径在 cmd 下易乱码，git add 用 `./*`；PowerShell 内嵌中文 here-string 易解析失败，脚本输出尽量英文。
- **本次交接提交**：仅 plan 文档更新（Q5–Q8 决策 + 现状 + 交接），纯 Markdown，豁免 FullRebuild；**本地提交，未推送**（按 Gavyn 指示，推送合入待后续）。

## 锁定验收

- [ ] 配件区为左/中/右 三固定槽；左槽仅通用符文，中/右按 70/15/15 三态抽取（可由日志/数据断言验证分布）。
- [ ] 卡牌区槽数=当前关卡投放表列等级数；每槽等级与表一致；第1关 0 槽、第4关恰 2 槽（2级+3级）。
- [ ] 已拥有的配件/非一级卡不出现在商店；一级卡可重复出现并可重复购买。
- [ ] 购买配件即装入对应武器槽，被替下的旧配件进入 `OwnedPartIds`；点"已拥有未装备"配件即装（无保存配置步骤）。
- [ ] 购买后置灰对应槽，不整页重洗；仅点该区刷新才重洗本区。
- [ ] 卡牌刷新限 1 次、符文刷新限 2 次，各扣 5 时间碎片；超限按钮禁用。
- [ ] 自动化 `ReEcho.Shop.*` 更新后通过；`validate_project.py` 与增量构建通过。
- [ ] 人工 PIE 确认：置灰手感、即装替换、两刷新按钮计数与扣费。

## Step 0 门禁

- 基线分支/提交：worktree `ReEcho-plan67-shop-drop` @ `826f9bd`（plan/67-shop-drop）。实现阶段前须 `git fetch` 并吸收最新 `origin/main`。
- 引擎/构建可用性：worktree 内 `scripts/ue/Build-Editor.cmd -Configuration Development` 可跑（编辑器关闭时）。
- 现有聚焦测试结果：`ReEcho.Shop.*` 当前通过（将随结构变更更新）。
- 共享契约 / 难合并资源风险：`FReEchoWeaponPartShopView` 结构变更影响 UI/测试；WBP `ReEchoInventoryShop` 需同步重排（UMG 资源，合并注意）。
- 基线损坏时的停止条件：若 `origin/main` 出现基线外提交，先报告冲突再等用户选择。

## 实现提纲

1. **数据层（先落地，再改代码）**（Q5/Q6/Q7/Q8 修正 2026-08-22）
   - `ReEchoData.xlsx`：
     - **不新增 `PartCategory` 列**（Q5）：三槽分类复用现有 `WeaponTypeId`（`Any`=通用 / 具体武器名），运行时动态判定 Universal / CurrentWeaponRune / OtherWeapon / OtherWeaponRune。
     - 投放系统表新增**售价区间两列**（`ShopPriceMin` / `ShopPriceMax`，按策划表2 的 8 个品类回填）——价格随机在运行时做（Q6）。
     - 投放系统表新增可被运行时读取的"商店卡牌组投放等级"结构化列（关卡 → 等级列表，如 第4关=`2,3`；第1关=空）。复用现有《投放系统》sheet，明确各行语义，使 `sync_xlsx_to_csv.py` 能导出。
   - `sync_xlsx_to_csv.py`：新增售价区间列导出 + shop-drop 等级表 → CSV（如 `Content/Data/ShopDropTable.csv`）；按需更新 `LOCKED_REFERENCE_SHEETS` / AUTHORING_LIST_VALIDATION_COLUMNS。
   - `FReEchoCsvPartRow`（`ReEchoCsvDataRegistry.h`）：**不加 PartCategory**；如需可在运行时用 `WeaponTypeId` 直接分类。新增 `ShopDropTable` 读取结构（若需要）。
2. **运行时报价生成（`ReEchoRunSubsystem.cpp` 的 `GetWeaponPartShopView`）**
   - 重写：不再产出扁平 `Offers`，改为：
     - `TArray<FReEchoWeaponSlotOffer> SlotOffers`（长度=武器槽数 3）：每槽一条报价。
       - 槽0（假定=Core/左）：候选=PartCategory==Universal 且兼容当前武器 且未拥有未装备。
       - 槽1/2（中/右）：按权重 70/15/15 从 {CurrentWeaponRune, OtherWeapon, OtherWeaponRune} 候选中抽取一条（兼容判定沿用 `IsPartCompatibleWithWeapon`）。
       - 排除集 = `OwnedPartIds` ∪ 已装备 `EquippedParts`；卡牌不混入此视图。
     - `TArray<FReEchoCardSlotOffer> CardSlotOffers`：读当前 `EncounterIndex` 的 shop-drop 等级表；每级一槽，槽内从该等级可投放卡中抽一张（排除非一级已拥有卡；一级卡不受排除限制）。等级列表空→空数组。
   - 刷新种子分两套：`CardShopRefreshCount`、`RuneShopRefreshCount`（替换原 `ShopRefreshSequence` 单计数），各驱动本区确定性洗牌。
3. **购买即装（`PurchaseShopItem` / `HandleShopPurchaseRequested`）**
   - 配件：直接 `TryEquipParts(已装备+新件)`；成功则被替下的旧件保持/回 `OwnedPartIds`（确认 `TryEquipParts` 已处理回落）；删除 draft/保存配置相关调用。
   - 卡牌：直接 `TryGrantCard`/等效授予（一级卡可重复叠加）。
   - 购买后调用仅刷新"该槽置灰"状态（不再整页 `RebuildTargetOfferRows`）。
4. **刷新拆分（`HandleShopRefreshRequested` → `HandleCardShopRefreshRequested` / `HandleRuneShopRefreshRequested`）**
   - 卡牌刷新：`CardShopRefreshCount < 1` 且 `TimeShards >= 5` → 扣 5、`++CardShopRefreshCount`、重洗卡牌区。
   - 符文刷新：`RuneShopRefreshCount < 2` 且 `TimeShards >= 5` → 扣 5、`++RuneShopRefreshCount`、重洗符文区。
   - `ReEchoShopRefreshPrice` 由 10 改为 5（Excel 权威）。
5. **UI 重写（`ReEchoInventoryShopWidget`）**
   - 配件区：3 个固定槽位 widget（左/中/右），各绑定一条 `SlotOffers`；购买置灰（`SetIsEnabled(false)` + 灰色背景）。
   - 卡牌区：动态生成 N 个槽（按 `CardSlotOffers.Num()`），每槽钉死等级、显示抽中的卡；购买置灰。
   - 删除 draft/保存配置按钮与流程；新增"已拥有未装备"区（遍历 `OwnedPartIds` 中未装备者），点击即装（即时）。
   - 两个刷新按钮（卡牌/符文）显示剩余次数与 5 碎片消耗。
   - WBP `ReEchoInventoryShop` 相应重排（UMG 资源变更，合并注意）。
6. **测试更新（`ReEchoShopTests.cpp`）**
   - 断言改为：配件=3 个固定槽报价（非随机前3）；卡牌槽数随关卡表（第4关=2）；刷新分两套计数；已拥有排除；一级卡可重复。
7. **文档同步**：更新 `MOD-ReEcho.md`、`MOD-ReEchoUI.md` 商店契约段；`ARCHITECTURE.md` 若提及则更新。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | CSV schema/UTF-8 不变量通过 |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码 0 |
| 运行时自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Shop` | 更新后报告通过 |
| 数据 | `python scripts/data/sync_xlsx_to_csv.py` + 校验 | 新 CSV 含 PartCategory 与 ShopDrop 表 |
| 人工 | 具名 PIE 任务 | 置灰/即装/两刷新计数与扣费记录 |

## 执行记录

### 变化

- 2026-08-21（完善 plan · 固化策划数据真源，用户要求）：
  - 用户提供策划数据源 `策划数据源/【开普勒】回响数值与构筑体系.xlsx`，其 `投放系统` sheet（A1:E47，全部可见行）为本 Plan 投放数值的唯一权威。
  - 本 Plan 文档新增「策划数据真源」节，固化 5 张表：卡牌组投放配置（关卡→等级）、商店售价区间、刷新次数限制（卡牌1/符文2，各扣5碎片）、三槽概率与占位（左通用/中右70·15·15）、怪物掉落时间碎片（关卡×怪类型）。
  - 协调段补充数据来源说明与分类依据（`武器符文C` sheet `武器类型` 列支撑 Universal/CurrentWeapon/OtherWeapon 判定）。
  - 修订 Step 0 / 本地基线为 worktree @ `826f9bd`。
  - **本次提交仅含 plan 文档 + 策划数据源文件夹，不含代码、不动 `ReEchoData.xlsx`**（xlsx 新增投放表与代码实现为后续独立步骤）。纯 Markdown + 数据源文件变更，豁免 FullRebuild。
- 2026-08-21（背包浮层，用户要求）：`UReEchoInventoryShopWidget`(.h/.cpp) 新增槽位点击 → 对应槽位背包浮层。
  - `.h`：新增 `BackpackPopupPanel`(UCanvasPanel*)、`ActiveBackpackSlotIndex`、`CachedBackpackItemIds`；新增 `GetSlotTypeIdForIndex`、`FindOwnedPartByContentId`、`HandleAttachmentSlotClicked`、`HandleAttachmentSlot0/1/2Clicked`（无参 UFUNCTION wrapper）、`BuildBackpackPopup`、`HideBackpackPopup`、`HandleBackpackItemClicked`。
  - `.cpp`：`RebuildAttachmentHoverSlots` 末尾为 3 个 `DesignerAttachmentSlot{i}` 绑无参 OnClicked；`BuildBackpackPopup` 按 SlotTypeId 筛"拥有未装备"配件，用 `UReEchoIndexedButton`+`OnIndexedClicked` 生成列表项（避免无参 OnClicked 委托带参绑定失败），浮层定位到被点槽位旁；`Refresh()` 开头 `HideBackpackPopup()` 防残留；购买走既有 `OnPurchaseRequested`。
  - 门禁：增量 Development 构建通过（Result: Succeeded，prebuilt bundle 刷新）、`validate_project.py` 全绿。待 PIE 人工验收。
- 2026-08-22（Q5–Q8 决策 + 现状/交接，Gavyn 要求换模型继续）：
  - 核查 Q5：开普勒表 `武器插槽C` sheet 第 1 列 `武器类型` = `通用`/具体武器名，与 parts.csv `WeaponTypeId`（Any/LongSword/Staff/Bow/Scythe/Gun/Whip）一一对应 → **撤回"新增 PartCategory 列"，改用现有 WeaponTypeId 运行时动态分类**。
  - 核查 Q8：cards.csv 42 行，`Tier`(0-3)/`OfferGroup`(Trait/Forge) 字段已就绪，卡牌侧无需新增字段。
  - Gavyn 决策：Q6 售价区间需 Min/Max 两列、运行时随机；Q7 以策划 xlsx 为准 CSV 随表改、"其他武器"=开局未选武器；协调段补数据真源纪律条。
  - 文档新增「Q5–Q8 澄清决策」「现状（交接快照）」「交接说明」三节；更新 Step 1 数据层提纲与基线段。
  - 开放项遗留：Q7"其他武器"是否有独立配件行（当前 parts.csv 是否含非符文的武器本体件），实现前先与 Gavyn/策划确认。
  - **本次提交仅含 plan 文档更新，纯 Markdown，豁免 FullRebuild；本地提交未推送**（推送合入待后续）。

### 证据

### 剩余风险

- Q7 开放项："其他武器"（中/右槽 15%）候选来源取决于 parts.csv 是否含武器本体件行；若无可降级为"其他武器符文"或策划补行。
- 售价区间 8 品类与 `WeaponTypeId`/Tier 的精确映射（棱镜水晶 vs 棱镜之晶、元素之晶是否按元素分）待落地时与策划对齐。

### 人工验收结果/请求

### 架构文档审阅结果
