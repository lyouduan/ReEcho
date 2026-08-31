# Plan 161 - 程序 - 符文背包所见即所得蓝图

## 协调

- Planner 负责人：JosephLE910 + Codex。
- Executor 负责人：JosephLE910 + Codex（合并模式）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`。
- 人工验收：`Passed`（用户微调后要求推送并合入主分支）。
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
  - `Content/ReEcho/UI/WBP_ReEchoInventoryShopScreen.uasset`（仅纳入用户本轮已保存微调，不由工具重建）。
  - `scripts/ue/author_plan161_rune_backpack.py`
  - `scripts/ue/audit_plan161_rune_backpack.py`
  - `Design/UI/ReEcho_商店浮窗调整指南.md`
  - `Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/README.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - 本 Plan 与 Git 规则允许的匹配 Win64 Editor 预构建包。
- Stable Reads：Run 的 `FReEchoWeaponPartShopView` / `FReEchoShopOffer`、符文装备命令、共用 Tooltip、已导入纹理和字体、UI Framework。
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
- 关闭前逐项审阅结果：见执行记录；人工验收仍待用户确认。

## 锁定验收

- [x] 两个独立 WBP 在 Designer 中有可见背板、正确比例及真实嵌套示例。
- [x] 面板/条目样式与尺寸不会在 Compile、Configure、重复打开、数量更新时被固定 C++ 值重置。
- [x] 示例与实际数据隔离；0/1/多条及重复打开正确，不残留假数据或旧 Tooltip。
- [x] 点击仍只装备已拥有且兼容的符文，双槽 occurrence 正确，数量/合成语义不变。
- [x] 主线已有商店蓝图、武器背包及其他用户微调不被覆盖。
- [x] 静态、编译、聚焦自动化和真实渲染证据通过。
- [x] 用户微调后接受当前候选并要求发布。
- [x] 未提交预构建允许列表之外的 UE 生成产物。

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

- 本 Plan 已以 `e69ae0fc` 单独发布到 main；实现在本地独立分支交付，不自行发布实现或关闭人工验收。
- 新增 `WBP_ReEchoRuneBackpack` 和 `WBP_ReEchoRuneBackpackEntry`：真实白边/深底、固定标题、滚动列表，三个独立条目示例包含原初/潮汐/森林之晶与不同数量。单条及整面板使用 Desired Size，不使用 Fill Screen 拉伸预览。
- 商店从 `BuildBackpackPopup` 移除固定尺寸/字体/边框/条目构造，改为实例化面板并复用行。尺寸使用面板期望大小和 Canvas AutoSize；标题、根 SizeBox、图标外层 SizeBox、文本样式、边框与间距全部由 WBP 持有。
- 条目 `DesignerPreview` 只在设计期生效，运行时填 `FReEchoRuneBackpackEntryView`，多份显示独立 `×N`；行数不足按同类追加并沿用首行列表 Slot 样式，多余行清空/禁用/折叠。原装备事件、稳定索引、双槽 occurrence、兼容过滤和数量权威不变。
- 首次渲染发现同资源 `SetBrushFromTexture` 会跳过 ImageSize 更新，导致首个样例图标的零尺寸不被修复；已采用明确原生纹理尺寸刷新，并修正一次性作者工具的 Brush 序列化。只修正本任务新建条目的初始 Brush，未修改其他资产。图标可见面积和比例已纳入 GPU 回归。
- 编写脚本只创建缺失资产，再次运行不改已有两份资产的哈希；审计脚本只读。已有商店、共用 Tooltip、数据源和远端新合成逻辑保持不变。

### 证据

- 初始基线及交付候选 `python scripts/validate_project.py` 全部通过，含 XLSX/CSV 同步；本任务没有沿用旧 Plan158 的数据豁免。
- `Build-Editor.cmd -Configuration Development`：UHT/UBT 成功，匹配 UE5.8 BuildId 55116800 的七模块精选预构建包；新 worktree 初次编译99个动作。后续修正与最终格式化后均重新构建，无 Live Coding。记录：`Saved/plan161-handoff-build.log`。
- `ReEcho.UI.Shop` 九项 + `ReEcho.UI.RuneBackpackRendering.DesignerParity` 一项，共10项通过。覆盖原商店布局、刷新文案、余额订阅、卡组选卡、逻辑区块、共用说明以及新背包作者值、0/1/8条、数量/零份过滤、双槽装备意图、重复打开和清空旧点击。记录：`Saved/plan161-handoff-tests-engine.log`。
- GPU Designer / runtime 使用相同内容分别绘制，322400个像素中差异0；同时验证外框留边可见、全部图标非零面积与原生宽高比、Desired Size 一致。截图：`Saved/Automation/Plan161/RuneBackpack-Designer.png` 与 `RuneBackpack-Runtime.png`。这是自动渲染核验，不等同于用户 PIE 视觉验收。
- `audit_plan161_rune_backpack.py` 通过；作者工具重复执行前后两份新资产 SHA256 一致。两份新 WBP 均在 Editor Compile/Save，未重建已有商店 WBP。
- 新 C++ 文件全文件 clang-format，旧文件仅修改区块检查；原主线商店头文件/实现已有的无关格式提示未扩张修复。`git diff --check`、Python 语法检查、LFS还原检查与预构建校验通过。

### 追加：名称完整单行

- 用户要求加宽后，在已关闭的 Plan161 工程中仅修改两个新背包 WBP 的宽度与名称换行配置，面板 320→520、条目 286→474；不修改字号、图标、行高、颜色或其他商店资产。一次性作者工具的新建默认值同步；已有资产仍不会被重复生成覆盖。
- GPU 回归增加临时长名称 `【握柄】镰刃命中攻速握柄III` 和 `×999`，验证关闭自动换行且文字期望宽度小于实际分配宽度；该压力样例不写入资产，不改变用户的 Designer 示例。
- Development 构建、资产加载审计、商店及 GPU 共10项测试、静态项目校验、预构建与格式检查通过。加宽后的 Designer/runtime 差异为 0 / 396800 像素。日志为 `Saved/plan161-width-{build,audit-engine,tests-engine,static}.log`，已查看实际渲染截图，最长名称与数量完整同排。
- 架构复核：`MOD-ReEcho` / `MOD-ReEchoUI` 与索引已审阅无需修改；只是既有作者尺寸调整和测试增强，不改变 Runtime API、状态所有权、模块依赖或代码位置。商店浮窗指南同步当前宽度和换行入口。

### 剩余风险

- Designer 与运行时不同分辨率需人工确认观感；不将自动化等同于人工作品验收。
- 本轮为本地技术交付；正式 main 发布需要用户验收与届时最新远端审计、发布锁、最终 FullRebuild，不能复用本地开发构建冒充发布门禁。

### 用户微调与发布审计

- 用户明确要求纳入微调并合入 main。磁盘仅有商店主蓝图修改，已确认 Editor 关闭；`WBP_ReEchoInventoryShopScreen.uasset` SHA256 为 `CF9DDB70FE02F17F68641B67AC88B649129FA4A2AA500FDA6A60C38C1EC70143`，原样纳入，不重建该资产。
- 最新远端 `428b83fd` 相对本任务发布基线新增 `e895eb9b` 的符文购买空槽自动装备、对应回归和模块文档，以及发布合并/构建记录。无 Plan 编号变化；代码与 UI 资产无物理冲突，只有精选预构建 DLL/manifest 双方更新冲突，获锁合并后由最终 FullRebuild 重新生成。
- 语义与耦合审计：保留远端 Run 的购买填空槽/原槽合成/满槽保留背包行为；Plan161 只消费更新后的 `BackpackCount` 并发送手动装备请求，两者契约兼容。远端源码和回归逐字保留，文档同时保留远端玩法描述与本任务 Designer 入口。最终重跑商店 UI、符文库存与购买相关测试。
- 架构关闭复核：`ARCHITECTURE.md` 已审阅无需修改（无新模块/拓扑/所有权变化）；`README.md` 已有调参索引；`MOD-ReEcho.md`、`MOD-ReEchoUI.md` 保留本任务组件条目并合入远端自动装备描述；两份 UI 指南保留调整入口与新宽度。最终发布证据待完成后登记。

### 人工验收结果/请求

- PendingBeforeClose：打开本 Plan 独立工程，编辑 `WBP_ReEchoRuneBackpack > BackpackRootSizeBox` 与 `WBP_ReEchoRuneBackpackEntry > NameText/CountText/RuneIconSize/EntryRootSizeBox`，编译保存后进商店验证可读性、屏幕边缘与重复开关。具体入口已登记到商店浮窗调整指南。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md` 已审阅、无需修改：仍为 ReEcho 内部 UI 组件，Run 所有权、UI Flow 屏幕生命周期与模块依赖不变。
- `shared/CODEBASE_MAP/README.md` 已更新：登记商店浮窗调参指南，包含两个新背包 WBP 入口，不新增 Runtime Module。
- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已更新：记录新 UI-only 视图/面板/条目以及原装备事件边界。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 已更新：替换符文动态构造描述，登记作者尺寸/实例复用/样例隔离/生成与回归入口；保留武器背包事实。
- `Design/UI/ReEcho_UI修改指导.md` 与 `ReEcho_商店浮窗调整指南.md` 已更新：登记新蓝图、必需绑定节点、尺寸/字体/背板/图标调参及预览刷新步骤。
