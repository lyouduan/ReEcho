# Plan85 — 商店「已拥有 / 已购」追踪

- **状态**：InProgress（实现于工作树 `ReEcho-plan85-shop-ownership`，分支 `plan/85-shop-ownership`）
- **作者**：[PROGRAMMER]（由程序 gavynqiu 委托，Planner/Secretary 起草）
- **基线**：origin/main `ea3c34d`（含 Plan76 符文 GM / Plan68 敌人 / Plan82 死亡表现 / Plan84 场景切换）

## 目标
商店购买后，被购买的槽位不再自动刷出新的内容，而是保持显示为「已购 / 已获得」；只有在玩家主动点击「刷新」后，已购槽位才会重摇出新内容。武器 / 符文 / 卡牌一经获得，就标记「已获得」状态，之后（含手动刷新）不再刷出同类已拥有项。

## 需求拆解（三个小目标，各自独立提交）
1. **购买后槽位不自动刷新，显示「已购」**：点击购买后，对应槽位保留刚买的商品并显示为「已购」（置灰、不可再点），不触发重摇；仅手动刷新才重摇该槽位。
2. **武器购买后标记「已获得」且不再刷出**：买一把武器（如剑）即加入「已拥有武器」集合，UI 标「已获得」，刷新时从候选中排除，不会再次刷出该武器。
3. **符文 / 卡牌购买后标记「已获得」且不再刷出**：符文（配件）、卡牌获得后标「已获得」；刷新时从候选中排除，不再刷出已拥有项。

## 现状分析（已读代码确认）
- 报价生成：`UReEchoRunSubsystem::GetWeaponPartShopView()` 用 `BuildShopOfferSeed(WeaponId, EncounterIndex, ShopRefreshSequence)` 确定性随机。
- `PickPart` 按「候选池下标」取配件：`IsPartExcluded` 排除 `OwnedPartIds` / `EquippedPartIds` / `InventoryItems`。
- `AReEchoGameMode::HandleShopPurchaseRequested` 购买后调用 `RefreshShopPresentation` → 再次 `GetWeaponPartShopView()`（**ShopRefreshSequence 不变**）。但因购买把配件加入 `OwnedPartIds`、卡牌加入 `InventoryItems`，候选池缩小，同一 seed 的下标落到**另一个**配件 → 槽位被「重摇」成新内容（即当前未满足需求 1 的根因）。
- 购买处理 `PurchaseShopItem`：配件→`OwnedPartIds`+装备；武器→切换 `CurrentBuild.WeaponId`（仅排除"当前武器"，未维护"已拥有武器"集合，需求 2 缺口）；卡牌→`InventoryItems`+`OwnedCardIds`。
- 已有排除：`IsPartExcluded`（符文/配件）与 `ReEchoCardRuntime::CanOffer`（卡牌，基于 `OwnedCardIds`）已能在刷新时排除已拥有项；widget 对 `OwnedParts` 显示「已拥有」、对 `InventoryItems` 中卡牌显示「已购买」。需求 3 大部分已具备，主要是统一「已获得」标记与确认刷新排除。

## 实现方案

### Step 1 — 购买后槽位不自动刷新（显示「已购」）
- widget 增加「已购槽位」标记：渲染期对 `CurrentView.Offers[i]` 维护 `PurchasedOfferIndices`（或给 offer 结构加 `bPurchased`）。
- 新增 widget 方法 `MarkOfferPurchased(int32 OfferIndex)`：将该槽位渲染为「已购」（置灰、禁用点击、显示「已购」标签），**不重建整个报价**。
- `HandleShopPurchaseRequested` 购买成功后：调用 `MarkOfferPurchased(对应下标)` 更新该槽位，并仅刷新不重摇的部分（TimeShards / 持有配件 / 主角属性面板），**不再**调用会重生成报价的 `ShowShop`。
- 手动刷新（`HandleShopRefreshRequested`）仍递增 `ShopRefreshSequence` → 全量重摇（已购槽位恢复为新内容），符合需求 1。
- 提交：仅此改动。

### Step 2 — 武器「已获得」且不再刷出
- `UReEchoRunSubsystem` 新增 `TSet<FName> OwnedWeaponIds`（与 `OwnedPartIds` 同生命周期/持久化）。
- `PurchaseShopItem` 武器分支：购买后 `OwnedWeaponIds.Add(SlotOffer.WeaponId)`。
- `GetWeaponPartShopView` 的 `OtherWeaponCandidates`：在排除 `CurrentBuild.WeaponId` 基础上，再排除 `OwnedWeaponIds`。
- widget：`MakeWeaponSlotOffer`/渲染武器槽位时，若 `OwnedWeaponIds.Contains(WeaponId)` 显示「已获得」（置灰、不可再买）。
- 提交：仅此改动。

### Step 3 — 符文 / 卡牌「已获得」且不再刷出
- 符文（配件）：确认 `IsPartExcluded` 已含 `OwnedPartIds`/`InventoryItems`（已购即装备，刷新排除）。统一 UI 标记为「已获得」（替代/并列「已拥有」文案）。
- 卡牌：确认 `CanOffer` 基于 `OwnedCardIds` 已排除已拥有；widget 对 `OwnedCards` / `InventoryItems` 中卡牌显示「已获得」。
- 回归检查：刷新时三类已拥有项均不再出现。
- 提交：仅此改动（含必要的标签/注释一致性）。

## 验证
- 每步提交后增量构建 `scripts/ue/Build-Editor.cmd -Configuration Development` 确认可编译。
- 商店相关自动化测试：`ReEchoShopTests.cpp` / `ReEchoShopLogicBlockTests.cpp` / `ReEchoShopEchoSelectionTests.cpp`（聚焦购买后槽位保持、刷新重摇、已拥有项排除）。
- `python scripts/validate_project.py` 静态校验（发布前全量重构后跑）。
- 用户本地开编辑器实测：买符文/武器/卡牌后槽位显示「已购/已获得」、不重摇；点刷新后重摇且已拥有项不出现。

## 风险
- Step 1 改动购买后刷新路径，需确保 TimeShards / 持有 / 属性面板仍正确刷新（不重摇报价即可）。
- 确定性随机依赖候选池大小，Step 2 排除已拥有武器会改变后续武器候选池，需确认不影响其它槽位确定性预期。
- 改动仅在 plan 工作树，最终经用户本地实测 + 「可以推 main」后 fast-forward 到 origin/main。
