# Plan 161 - 程序 - 符文背包所见即所得蓝图

## 协调

- Planner 负责人：JosephLE910 + Codex。
- Executor 负责人：JosephLE910 + Codex（合并模式）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main` / `531bd9cb08f8808f254de59dd0ff99fe36588a56`。
- 本地实现方式：独立 worktree，`plan/161-rune-backpack-blueprints`。
- 依赖 / 阻塞：沿用已发布 Plan160 符文份数与合成投影、Plan157 商店 Tooltip；无玩法阻塞。
- Writes:
  - `Source/ReEcho/{Public,Private}/UI/ReEchoRuneBackpack*Widget.*`
  - `Source/ReEcho/{Public,Private}/UI/ReEchoInventoryShopWidget.*`
  - `Source/ReEcho/Private/Tests/ReEcho*Backpack*Tests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopLogicBlockTests.cpp`
  - `Content/ReEcho/UI/WBP_ReEchoRuneBackpack.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoRuneBackpackEntry.uasset`
  - `scripts/ue/author_plan161_rune_backpack.py`
  - `scripts/ue/audit_plan161_rune_backpack.py`
  - `Design/UI/ReEcho_商店浮窗调整指南.md`
  - `Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/README.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - 本 Plan 与 Git 规则允许的匹配 Win64 Editor 预构建包。
- Stable Reads：Run 的 `FReEchoWeaponPartShopView` / `FReEchoShopOffer`、符文装备命令、`WBP_ReEchoInventoryShopScreen`、共用 Tooltip、已导入纹理和字体、UI Framework。
- 影响模式：`SharedContract`（限商店表现适配）。
- 兼容承诺 / 下游操作：保留槽位/武器兼容过滤、真实未装备份数、稳定条目索引与槽位 occurrence、装备而非购买命令、切换/关闭与余额订阅行为；不改变合成、存档、CSV/XLSX。
- 明确排除：武器背包重构、商店整页重建、已有用户蓝图修改、增加玩法、旧 Plan158 WIP 分支强制清理、实现自动发布 main。

## 锁定目标

符文背包主面板及单个条目使用真实独立 WBP，打开 Designer 就能看到完整背板、标题、滚动列表和符文示例。修改字体、字号、颜色、边框、Padding、图标尺寸、条目高度和面板尺寸后，编译保存及运行时数据刷新均不得被 C++ 固定值覆盖。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI`、`AREA-UI`。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `MOD-ReEchoUI.md`，均已列入 Writes。
- 设计意图：从商店大类中移出符文背包视觉构造，由 WBP 唯一拥有排版；商店只过滤只读投影、填充内容及转发原装备意图。
- 权威状态与依赖：不新增 Runtime Module，不改变 Run 所有权、公共玩法契约或依赖方向；新增 UI-only 的面板/条目组件。
- 决策记录：面板包含真实条目蓝图示例，运行时复用示例并按相同类扩展，隐藏多余行；示例只作设计期展示，不进入背包。使用正式边框/深底，保留共用 `WBP_ReEchoShopTooltip`。面板尺寸由 Designer 的 SizeBox 和 Desired Size 决定，C++ 仅按实际期望尺寸进行屏幕避让，不强写宽高。条目图标采用等比容器，资源更新不覆盖作者几何。
- 相关文档同步范围：审阅 `ARCHITECTURE.md` 的模块/状态拓扑（预计无变化）、`README.md` 索引，以及两份模块文档；调整指南登记明确的面板/条目入口与绑定节点。
- 关闭前逐项审阅结果：待实现后填写。

## 锁定验收

- [ ] 两个独立 WBP 在 Designer 中有可见背板、正确比例及真实嵌套示例。
- [ ] 面板/条目样式与尺寸不会在 Compile、Configure、重复打开、数量更新时被固定 C++ 值重置。
- [ ] 示例与实际数据隔离；0/1/多条及重复打开正确，不残留假数据或旧 Tooltip。
- [ ] 点击仍只装备已拥有且兼容的符文，双槽 occurrence 正确，数量/合成语义不变。
- [ ] 主线已有商店蓝图、武器背包及其他用户微调不被覆盖。
- [ ] 静态、编译、聚焦自动化和真实渲染证据通过；用户完成视觉与可编辑性验收。
- [ ] 未提交预构建允许列表之外的 UE 生成产物。

## Step 0 门禁

- 基线：`531bd9cb`；最大已发布 Plan160，本任务使用161。
- 远端审计：相对已交付 Plan158，主线新增 Plan160 背包份数/原子合成和弓击杀加速修复及 XLSX 同步。新任务从准确主线开始，无待合并旧实现，物理冲突为无；保留新的 `BackpackCount` 投影和原命令，逻辑兼容；耦合集中在 `BuildBackpackPopup` 与既有背包 UI 测试。
- 引擎：UE 5.8 安装版；当前未发现 UnrealEditor 进程，执行前复核。
- 基线检查：LFS 三个文件已还原；`python scripts/validate_project.py` 全部通过（含 XLSX/CSV）。
- 共享风险：仅新建两个资产，现有商店 WBP 只读；生成器不得覆盖人工调整过的既存新资产。
- 停止条件：出现玩法契约冲突、未保存 Editor 资产或真实绑定/构建失败时先解决/报告，不伪造通过。

## 实现提纲

1. 发布本 Plan；建立面板和条目原生窄数据接口。
2. 用 Editor 脚本一次性作者化两个 WBP、正式纹理和嵌套例子。
3. 商店复用面板，按真实投影配置内容并保留原有装备路由；移除符文背包硬编码视觉构造。
4. 聚焦测试作者值保持、滚动扩容、真实图标和边框、交互过滤及重复生命周期。
5. 完整交付编译、文档索引与调整指南；保留 `Review` 等待用户验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py`、`git diff --check`、clang-format | 无数据漂移或源码格式问题 |
| LFS | `python scripts/setup_lfs.py --check` | 真实资源可加载 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 成功、匹配预构建 |
| 资产 | author/audit Plan161 脚本、蓝图 Compile/Save、真实 Slate 渲染 | 预览/运行时相同层级与可见背板 |
| 自动化 | `ReEcho.UI.Shop`、Plan161 聚焦回归 | 过滤/份数/条目样式/尺寸/装备意图正确 |
| 人工 | 在两个 WBP 改字体和尺寸，再进商店打开符文背包 | 用户确认所见即所得 |

## 执行记录

### 变化

- 待实现。

### 证据

- 初始基线静态校验与 LFS 检查通过。

### 剩余风险

- Designer 与运行时不同分辨率需人工确认观感；不将自动化等同于人工作品验收。

### 人工验收结果/请求

- PendingBeforeClose：完成后提供准确工程与两个 WBP 入口。

### 架构文档审阅结果

- 待实现后逐项记录。
