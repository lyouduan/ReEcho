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
