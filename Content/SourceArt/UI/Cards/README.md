# Plan 69 卡牌美术资源约定（最终落地版）

## 卡牌底图（ArtImage，主视觉）
- 来源：`Downloads\卡牌` 的 3 张星级框：`1星灰色.png` / `2星金色.png` / `3星彩色.png`。
- 按 **Tier** 映射（非每张卡独立图）：`cards.csv` 的 `Tier` 列（1=灰色/2=金色/3=彩色；0=FORGE 隐藏）。
- 导入后运行时路径：`/Game/ReEcho/Textures/UI/Cards/Art/T_UI_CardTier{1|2|3}`。
- C++：`UReEchoTraitCardChoiceWidget::ResolveCardArtTexturePath(int32 Tier)`；`Tier<1` 返回空 → `ArtImage` 隐藏。
- 已暂存：`Content/SourceArt/UI/Cards/Art/T_UI_CardTier1.png|2.png|3.png`。

## 卡牌 icon（IconImage，角落小图标）
- 来源：既有图标最初来自《【开普勒】回响数值与构筑体系.xlsx》`构筑体系G` 表 `小icon` 列（内嵌于 `xl/drawings/media/`）；Plan128 以用户完整交付目录 `附件下载_【开普勒】回响数值与构筑体系/` 中按卡名导出的 PNG 为当前权威版本。
- 匹配方式：**按卡名称**（文档 `名称` 列 ↔ `cards.csv` DisplayName）。原因：设计表与游戏数据 **Id 与名称双重漂移**，不能按 Id 直接对应；同名精确匹配可靠。
- 抽取脚本：`scripts/plan69_extract_card_icons.py`（可复现；需本机存在该 xlsx）。
- 命名：`T_UI_CardIcon_{GAME_CARD_ID}.png` → 运行时 `/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_{GAME_CARD_ID}`。
- Plan128 后当前全部 64 张启用且可投放卡牌均拥有独立图标；未来新增且尚未交付独立图标的卡牌继续走通用回退 `T_UI_Shop_CardIcon`。
- 完整卡牌图标的导入与覆盖使用 `scripts/ue/import_plan128_complete_card_icons.py`，脚本会从权威 CSV 枚举全部启用且可投放卡牌，并逐项校验规范 SourceArt、运行时 Texture2D 对象路径和原生尺寸。

## 通用回退
- icon 缺失 → `/Game/ReEcho/Textures/UI/InteractionPlaceholder/InventoryShop/T_UI_Shop_CardIcon`。
- art（Tier<1）→ `ArtImage` 隐藏（保留既有 `ArtCardFrame`）。

## 后续导入（编辑器步骤 / 或脚本）
- `scripts/plan69_import_ue.py`：把 `SourceArt/UI/Cards/{Art,Icon}` 的 PNG 导入为 UTexture2D。
- `scripts/plan69_edit_wbp.py`：给 `WBP_ReEchoTraitCardEntry` 加 `ArtImage`/`IconImage` 两个 Image 控件（WBP 二进制编辑风险较高，失败则手动：编辑器拖两个 Image 改名即可）。
- 本地增量构建 `scripts/ue/Build-Editor.cmd -Configuration Development` + PIE 验收。
