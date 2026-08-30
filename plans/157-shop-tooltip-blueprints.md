# Plan 157 - 程序 - 商店说明与属性浮窗蓝图化

## 协调

- Planner 负责人：Codex（当前程序对话内兼任）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@d62a088a8f732a90ff9b1b6ac4b6f1d681e44981`。
- 本地实现方式：独立分支 `plan/157-shop-tooltip-blueprints`、worktree `ReEcho-plan157-shop-tooltip-blueprints`。
- 依赖 / 阻塞：UE 资产操作及构建前确认没有交互式 Editor 使用本克隆；最终视觉与微调行为由用户验收。
- Writes: 本 Plan；`Source/ReEcho/{Public,Private}/UI/ReEchoInventoryShopWidget.*`；新增 `ReEchoShopTooltipWidget.*`、`ReEchoAttributeTooltipWidget.*`、`ReEchoAttributeRowWidget.*`；`Source/ReEcho/Private/Tests/ReEchoShopTooltipTests.cpp`；`Content/ReEcho/UI/WBP_ReEcho{ShopTooltip,AttributeTooltip,AttributeRow}.uasset`；`scripts/ue/{author,audit}_plan157_shop_tooltips.py`；`Design/UI/ReEcho_商店浮窗调整指南.md`；`Design/UI/ReEcho_UI修改指导.md`；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`；构建门禁精选 Win64 Editor 预构建包。
- Stable Reads: `ReEchoLoadoutTooltipWidget.*`、`ReEchoLoadoutEntryWidget.*`；`WBP_ReEchoLoadoutTooltip`、`WBP_ReEchoInventoryShopScreen`；`FReEchoShopOffer`、`FReEchoStatBlock`、CSV 属性目录和 Run 已生效结果投影；原商店聚焦测试及 UMG authoring 工具。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：保留所有原悬停命中面、Slate 原生定位避让、商品购买/装备/卡牌状态和属性格式；运行时只更新数据及显隐，不重置 Blueprint 字体、颜色、描边、宽度、间距和布局。
- 明确排除：不重建商店整页、不修改开场选择浮窗、不修改策划表或玩法数值、不推导卡牌实际效果、不改存档、输入和交易语义；不覆盖其他任务尚未集成的人工微调。

## 锁定目标

1. 商店角色被动、武器、配件、已获得卡牌和卡组说明统一使用真实可编辑的独立 Widget Blueprint，提供有代表性的示例标题和说明。
2. 已获得卡牌在说明下方的“实际效果”附加面板也必须在 Designer 中可见、可编辑，示例包含多项已生效结果。运行时仍只消费 `OutcomeText`；为空时折叠附加面板且不留空白。
3. 角色属性浮窗及其属性行由独立 Blueprint 持有样式，Designer 显示属性图标、名称、数值和完整列表示例；运行时按现有 CSV 顺序填充真实属性。
4. 所见即所得覆盖尺寸、字体/字号/颜色/描边、边框、Padding 与自动换行；编译、保存、重新打开、运行时填充都不能把人工样式改回程序常量。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI` / `AREA-UI`。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `MOD-ReEchoUI.md`，均在 Writes。
- 设计意图：消除商店 `BuildSlotTooltip` / `BuildAttributePanel` 内的临时布局与写死字体，将表现权威交给 Blueprint；商店仍为原有数据及原生 Tooltip 生命周期的宿主。
- 权威状态与依赖：Run/CSV 保持角色、物品与已生效效果的唯一来源；新 Widget 只消费展示文本、图标和属性行，不写回游戏状态。新增 Widget 在现有 ReEcho Runtime Module 内，不引入模块依赖。
- 决策记录：物品浮窗使用一个包含主说明与可选实际效果区的完整 WBP，保证同一 Designer 能预览两块组合；属性浮窗使用可编辑属性行 WBP，并在完整浮窗里放入真实行实例作为示例。保留现有白边近黑底、实际效果金边的视觉语义。
- 相关文档同步范围：审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoUI.md`；新增专门的控件微调指南并从总 UI 指南链接。

## 锁定验收

- [ ] 三个 WBP 均有非空的所见即所得示例；完整卡牌浮窗预览实际效果区，属性浮窗预览完整列表。
- [ ] 角色被动、武器、配件、卡组、已拥有卡牌的悬停说明全部复用新物品浮窗。
- [ ] 非空 OutcomeText 原样显示；空结果折叠；多行/多项结果和长正文能自动扩展，不被固定高度裁切。
- [ ] 真实属性顺序、图标、整数与百分比格式保持原契约，示例数值不泄漏到游戏。
- [ ] 改动蓝图样式后，运行时 Configure/刷新不覆盖；重复属性刷新无重复行或残余旧值。
- [ ] 编译、资产审计、聚焦自动化、项目校验与 diff 检查通过；只跟踪允许的预构建产物。
- [ ] 用户完成 Designer 与 PIE 视觉/微调验收。

## Step 0 门禁

- 远端最大 Plan 为 156；157 为新编号，与 Proposed 的 Plan156 独立。
- 基线新增的转场、回响法阵、怪物表现和预构建提交已做路径/逻辑审计，不改目标浮窗文件；新工作树直接采用准确远端基线，无物理冲突。与其他 UI 工作共享模块文档，发布时重新审计耦合。
- 引擎/构建：使用仓库 UE 5.8 安装版入口及同克隆 Unreal 锁；资产首次使用先执行 LFS 检查。
- 现有证据：开场独立浮窗只 SetText；商店物品与属性面板仍在 C++ ConstructWidget 并固定 16 号正文。
- 基线损坏时停止条件：缺失真实资产、无法加载目标类、同路径未保存修改或无法安全保留用户布局。

## 实现提纲

1. 发布本 Plan 并验证远端包含后，添加三个纯展示 Widget 与配套 Blueprint。
2. 将物品、实际效果、属性列表作者化为 Designer 默认样例，数据 Configure 与设计期严格分离。
3. 将商店现有 Tooltip 路由切换到新类；移除被替代的动态样式生成逻辑，保持原调用者。
4. 验证数据、显隐、自动高度、样式权威、资产引用及现有商店交互。
5. 写微调指南及执行证据，交付独立工程供用户验收；实现发布另走验收与发布门禁。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目与差异检查通过 |
| LFS | `python scripts/setup_lfs.py --check` | 真实资产可用 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 通过并刷新精选预构建包；最终实现发布使用 FullRebuild |
| 资产 | `Run-EditorPythonLocked.ps1` 执行 author/audit 脚本 | Compile/Save、真实控件、非空样例、Desired Size 及嵌套结构正确 |
| 聚焦 | `ReEcho.UI.Shop.TooltipBlueprints` 与 `ReEcho.UI.Shop.AuthoredLayoutHosts` | 真实数据填充、无数据折叠、样式不被重写、原商店路由不回归 |
| 人工 | 新浮窗 Designer 与商店 PIE | 字体微调、长说明/实际效果、属性列表、避让定位符合预期 |

## 执行记录

### 变化

- 已完成基线路由与范围核对，等待 Plan-only 发布后开始实现。

### 证据

- 规划基线 `d62a088a`；本次不改已有主工程 `ReEcho.uproject` 的未提交内容。

### 剩余风险

- 长文本高度随数据变化，Designer 示例只能代表示例内容；运行时仍需验证窗口避让。
- 尚未进行本次编译、资产和人工验证，不能复用前序 Plan 的成功结果。

### 人工验收结果/请求

- `PendingBeforeClose`：完成后提供具体 WBP/控件名及独立工程路径。

### 架构文档审阅结果

- 实现完成时逐项填写。
