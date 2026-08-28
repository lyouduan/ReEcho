# vollerei 的 ReEcho 策划经验

## 2026-08-28 - 角色武器插槽与卡牌文案修正

- 结果：成功
- 事项与分支：角色、武器、武器插槽与卡牌文案修正；对应 `merge/vollerei/weapon-and-slot-copy`；提交号（待提交）
- 目标：修正角色、武器、武器插槽配件/核心及卡牌的展示文案，不改变玩法属性或效果契约。
- 生效映射：在 `Design/Data/ReEchoData.xlsx` 的角色、武器、武器插槽和构筑生产 Table 修改 `Description`，保存关闭表格后运行全量同步，会分别更新 `Content/Data/characters.csv`、`weapon_types.csv`、`parts.csv`、`cards.csv` 的对应展示文案。
- 修改：`ReEchoData.xlsx`；生成 CSV 为 `characters.csv`、`weapon_types.csv`、`parts.csv`、`cards.csv`。未修改 ID、数值、元素、效果、启用状态或生成器。
- 刷新方式：保存并关闭 Excel/WPS，运行 `python scripts/data/sync_xlsx_to_csv.py`、`--check` 与 `python scripts/validate_project.py`；游戏内验证需重启 Unreal Editor 后使用新游戏。
- 验证入口：角色与武器选择说明、商店配件/核心 Tooltip 和抽卡页文案。策划确认文案效果正确并同意推送。
- 策划确认：vollerei 于 2026-08-28 确认效果正确并明确同意推送。
- 限制与踩坑：不要手改生成 CSV；工作簿是二进制文件，编辑前确认无他人同时修改。局部加粗/斜体属于独立 Request `request/vollerei/rich-text-copy-style`，不能直接在纯文本 Description 中实现。
- 报告交接：无

## 2026-08-28 - 羊皮纸统一 Description 说明框需求

- 结果：需要程序
- 事项与分支：角色、武器、卡牌、武器插槽与符文背包的统一羊皮纸说明框；对应 `request/vollerei/parchment-description-panel`；提交号（待提交）
- 目标：以透明羊皮纸面板统一承载动态 `DisplayName` 与 `Description`，不改变数据或玩法逻辑。
- 生效映射：透明源图位于 `Content/SourceArt/UI/DescriptionPanel/Elements/T_UI_ParchmentDescriptionPanel.png`；程序需在 Unreal Editor 导入运行时 Texture2D 并接入 Description Tooltip/Panel，策划侧仅提供源图和视觉验收。
- 修改：新建羊皮纸说明框 Request、透明运行源图与原始参考图；未导入 `.uasset`、未修改 WBP、C++、数据表或生成 CSV。
- 刷新方式：待程序候选提供后，重启 Unreal Editor、新游戏并在角色、武器、卡牌、配件/核心和符文背包说明入口验证。
- 验证入口：检查短/长 Description 的自动换行、边框、文字安全边距、点击穿透，以及 1280×720、1920×1080、超宽比例；标题外轮廓与羊皮纸上部内边框在 1920×1080 设计面至少保留 24px 可见间距。
- 策划确认：vollerei 于 2026-08-28 确认需求范围、统一说明框产品选择及标题顶部安全间距；待确认报告后推送。
- 限制与踩坑：不要把文字烘焙进图；需要自适应高度时不能非等比拉伸手绘边框；局部加粗/斜体属于独立 `request/vollerei/rich-text-copy-style`。
- 报告交接：`issues/vollerei/requests/羊皮纸-统一说明框.md`
