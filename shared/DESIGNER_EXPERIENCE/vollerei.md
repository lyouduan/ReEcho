# vollerei 的 ReEcho 策划经验

## 2026-08-28 - 角色武器与插槽局部富文本样式需求

- 结果：需要程序
- 事项与分支：角色、武器与武器插槽 Description 的局部加粗/斜体；对应 `request/vollerei/rich-text-copy-style`；提交号（待提交）
- 目标：让策划在既定文案标记语法中对 Description 局部片段应用加粗、斜体及二者组合，而不影响玩法数据。
- 生效映射：当前 `Description` 由普通 TextBlock 或商店动态 Tooltip 显示；仅改 Excel 文案无法产生局部字体样式，需程序提供统一富文本展示入口。
- 修改：新建 `issues/vollerei/requests/富文本-角色武器与插槽局部样式.md`，登记角色、武器与配件/核心说明的显示需求。
- 刷新方式：待程序实现后，由程序提供重启 Editor、新游戏及角色/武器/商店 Tooltip 的验证路径。
- 验证入口：角色与武器鼠标 Tooltip、键盘/手柄聚焦说明、商店配件/核心完整 Tooltip；检查中文、嵌套样式、长文案、低分辨率和无标记旧文案的兼容性。
- 策划确认：vollerei 于 2026-08-28 确认需求范围；等待对报告描述和验收标准的最终确认后推送。
- 限制与踩坑：不要将 Markdown/HTML 标记直接当作现有 TextBlock 的可用样式；不得复用仅限伤害数字或授权未确认的字体资源。
- 报告交接：`issues/vollerei/requests/富文本-角色武器与插槽局部样式.md`