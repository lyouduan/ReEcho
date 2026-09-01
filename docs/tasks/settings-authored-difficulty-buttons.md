# 设置页作者化难度按钮

## 目标与边界

- 用户于 2026-09-01 要求删除设置页难度下拉框，增加与“画面 / 声音 / 键位”并列的“难度”页签，并在独立页面放置蓝图内可见、可拖动、可缩放的“派对 / 常规 / 噩梦”三个按钮；按钮视觉与纵向构图沿用暂停页三按钮风格。
- 基线：`origin/main@f306fe3dd739afa65f7589b9d6b4b0ef63b2130d`；独立工作树 `ReEcho-settings-authored-difficulty-buttons` / `fix/settings-authored-difficulty-buttons`。
- 保留 Plan162 的权威逻辑：未开局时编辑下一局偏好，恢复默认选择“常规”，应用后持久化；开局或继续游戏后只读展示本局锁定难度。不得修改难度数据包、Run/Save、开局或继续流程。

## 设计与影响面

- `WBP_ReEchoSettings/Panel` 直接拥有第四个 `Overlay_Difficulty` 页签根；`SettingsLayoutCanvas` 直接拥有独立 `DifficultyPanel`，其三个 Overlay 根各包含透明命中按钮、暂停按钮图和默认中文标签，Designer 中即可预览和微调全部几何与样式。
- `UReEchoSettingsWidget` 只按稳定名称绑定点击，切换当前选中图和可用状态；不再创建下拉框、标签或写入位置和尺寸。
- 三个按钮继续复用通用按钮悬停反馈，中心等比缩放并在移出时恢复作者值。
- Writes：`WBP_ReEchoSettings`、窄作者/审计脚本、`ReEchoSettingsWidget.*`、聚焦测试、`MOD-ReEchoUI.md`、本记录和匹配 Editor 预构建包。
- 已审阅 `ARCHITECTURE.md` / `README.md`：模块、依赖、稳定 ID 和阅读入口均不变化，无需修改。

## 验证目标

- 蓝图中可直接看见顶部“难度”页签及其独立页面的三个纵向按钮；页签与页面按钮根均是作者化 Canvas 子项，位置与尺寸不由 C++ 覆盖。
- 默认蓝图样例为“常规”选中；运行时三按钮正确切换 PendingDifficulty，恢复默认与应用语义不变；本局锁定后全部禁用且当前档仍可辨认。
- 作者脚本幂等、蓝图编译保存；Development 构建、设置/难度聚焦自动化、项目静态、LFS 与 diff 检查通过后交给用户 PIE 验收。

## 当前交付（2026-09-01）

- 已删除正常 WBP 路径中的运行时 `DifficultyPanel` / `DifficultyComboBox` 创建和固定坐标写入；第四页签、独立 `DifficultyPanel` 与三个 Canvas Overlay 根均由 WBP 保存。
- 三个根分别保存透明 `UReEchoIndexedButton`、暂停按钮浅/深图和中文样例；默认“常规”使用浅图，另外两档使用深图。运行时仅根据 Pending/Run 难度切换图资源并启用或禁用按钮。
- 初版误把“难度”理解为键位页内标签；用户截图纠正后已改为真正的第四页签，并把三按钮移到独立页面纵向排列。纠正后的作者脚本成功编译保存 WBP，专项审计确认第四页签、独立页面、三个 Canvas 根、样例素材与旧控件清理均通过。
- 纠正后的 Development 构建通过并刷新 7 模块精选 Editor 包，源码指纹 `e1b395ae5d38`；`ReEcho.UI.SettingsInteraction` 1/1 通过；`validate_project.py`、预构建检查、LFS hydration 与 `git diff --check` 通过。
- `ReEcho.Difficulty` 中 `PackagesAndCache` 通过；既存 `RunLockAndSaveMigration` 因测试在 Package Outer 下直接 `NewObject<UReEchoRunSubsystem>` 触发 UE 5.8 `ClassWithin=GameInstance` ensure 而失败。该测试及 Run/Save 源码均未被本任务修改，不将其记为本交付通过，也不扩大本 UI 任务修复范围。
- 用户已在蓝图中完成最终微调并确认发布；保留该 `WBP_ReEchoSettings` 作者结果，不再运行作者脚本覆盖。发布前仅执行只读蓝图审计、最终组合构建及仓库门禁，并按主分支发布锁规则合入 `main`。
