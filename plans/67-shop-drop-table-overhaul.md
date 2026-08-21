# Plan 67 — Shop — 投放系统重做（固定符文槽/卡牌按表/置灰/即装/分区刷新）

## 协调

- Planner 负责人：Gavyn-side AI（兼任秘书，按 shared/PLANNER_RULES.md 与 shared/SECRETARY_RULES.md）。
- Executor 负责人：Gavyn-side AI（程序身份，须经人工确认后动手）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Proposed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（商店交互、置灰手感、即装替换需 PIE 人工确认）。
- 本地规划 / 实现基线：`origin/main` @ af9b6e47（已 `git fetch` 确认远端无更新）。
- 本地实现方式：独立 worktree `ReEcho-plan67-shop-drop`（避免在主工作树做 plan 类改动）。
- 依赖 / 阻塞：需先在 `ReEchoData.xlsx` 投放系统表与武器符文表落地两处新数据（见实现提纲 Step 1），再改运行时。
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
    - **2026-08-21 修订（用户要求）**：D5 推翻"无独立背包 UI"——改为**点击武器配件槽位（`DesignerAttachmentSlot{i}`）弹出对应槽位（SlotTypeId）的浮层背包**，列出该槽位下"拥有但未装备"的配件，点击即装（走 `OnPurchaseRequested` → GameMode `TryEquipPurchasedPart`）。空槽也弹（用 `View.Slots[i].SlotTypeId`）。数据源复用 `FReEchoWeaponPartShopView.OwnedParts`/`Slots`/`EquippedParts`，未新增背包数据结构。原"明确排除：不新增背包系统"中"不新增背包**数据结构**"仍成立，仅 UI 层新增浮层面板。
  - **D6 获得范围/刷新（q5）**：任何来源获得均计入排除集；刷新限次按 Excel。
- 相关文档同步范围：`ARCHITECTURE.md`（若提及商店报价结构需更新）、`modules/MOD-ReEcho.md`、`modules/MOD-ReEchoUI.md`；其余 MOD 不相关，省略并说明。
- 关闭前逐项填写审阅结果：（实现后回填）

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

- 基线分支/提交：`origin/main` af9b6e47（已 fetch 确认一致）。
- 引擎/构建可用性：worktree 内 `scripts/ue/Build-Editor.cmd -Configuration Development` 可跑（编辑器关闭时）。
- 现有聚焦测试结果：`ReEcho.Shop.*` 当前通过（将随结构变更更新）。
- 共享契约 / 难合并资源风险：`FReEchoWeaponPartShopView` 结构变更影响 UI/测试；WBP `ReEchoInventoryShop` 需同步重排（UMG 资源，合并注意）。
- 基线损坏时的停止条件：若 `origin/main` 出现基线外提交，先报告冲突再等用户选择。

## 实现提纲

1. **数据层（先落地，再改代码）**
   - `ReEchoData.xlsx`：
     - 武器符文表新增列 `PartCategory`（值：Universal / CurrentWeaponRune / OtherWeapon / OtherWeaponRune），按 Excel 投放系统语义回填现有符文。
     - 投放系统表新增可被运行时读取的"商店卡牌组投放等级"结构化列（关卡 → 等级列表，如 第4关=`2,3`；第1关=空）。复用现有《投放系统》sheet，明确各行语义，使 `sync_xlsx_to_csv.py` 能导出。
   - `sync_xlsx_to_csv.py`：将 `PartCategory` 加入 parts CSV 导出；新增 shop-drop 等级表 → CSV（如 `Content/Data/ShopDropTable.csv`）；更新 `LOCKED_REFERENCE_SHEETS`。
   - `FReEchoCsvPartRow`（`ReEchoCsvDataRegistry.h`）新增 `FName PartCategory`；`ReEchoCsvDataRegistry` 解析新列与新 CSV。
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

- 2026-08-21（背包浮层，用户要求）：`UReEchoInventoryShopWidget`(.h/.cpp) 新增槽位点击 → 对应槽位背包浮层。
  - `.h`：新增 `BackpackPopupPanel`(UCanvasPanel*)、`ActiveBackpackSlotIndex`、`CachedBackpackItemIds`；新增 `GetSlotTypeIdForIndex`、`FindOwnedPartByContentId`、`HandleAttachmentSlotClicked`、`HandleAttachmentSlot0/1/2Clicked`（无参 UFUNCTION wrapper）、`BuildBackpackPopup`、`HideBackpackPopup`、`HandleBackpackItemClicked`。
  - `.cpp`：`RebuildAttachmentHoverSlots` 末尾为 3 个 `DesignerAttachmentSlot{i}` 绑无参 OnClicked；`BuildBackpackPopup` 按 SlotTypeId 筛"拥有未装备"配件，用 `UReEchoIndexedButton`+`OnIndexedClicked` 生成列表项（避免无参 OnClicked 委托带参绑定失败），浮层定位到被点槽位旁；`Refresh()` 开头 `HideBackpackPopup()` 防残留；购买走既有 `OnPurchaseRequested`。
  - 门禁：增量 Development 构建通过（Result: Succeeded，prebuilt bundle 刷新）、`validate_project.py` 全绿。待 PIE 人工验收。

### 证据

### 剩余风险

### 人工验收结果/请求

### 架构文档审阅结果
