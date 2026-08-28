# Plan 147 - 程序 - 回响存储正式弹窗 UI

## 协调

- Planner 负责人：当前程序用户与 Gavyn-side AI。
- Executor 负责人：Gavyn-side AI。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@050bffdfc7a37c4e047a02b719fb168bc6c30518`。
- 本地实现方式（可选，仅作交接说明）：按当前对话已确认的“一任务一 worktree”模式，从发布本 Plan 后的准确 `origin/main` 创建专属 worktree。
- 依赖 / 阻塞：正式参考 `C:/Users/gavynqiu/Downloads/重开弹窗.svg`；正式切图 `暂停按钮浅.png`、`暂停按钮深.png`、`通用弹窗.png`；实现前必须通过 Git LFS 检出检查并确认 Unreal Editor 已关闭。与 Plan 146 的数据资产迁移并行时，本任务只消费稳定 `G_3_02`/回响摘要契约，不修改其数据权威或迁移表面。
- Writes:
  - `plans/147-formal-echo-storage-popup-ui.md`
  - `Content/SourceArt/UI/InventoryShop/EchoStorage/`
  - `Content/ReEcho/Textures/UI/InventoryShop/EchoStorage/`
  - `Content/ReEcho/UI/WBP_ReEchoInventoryShopScreen.uasset`
  - `Source/ReEcho/Public/UI/ReEchoInventoryShopWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoInventoryShopWidget.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopLogicBlockTests.cpp`
  - `scripts/ue/import_plan147_echo_storage_popup.py`
  - `scripts/ue/author_plan147_echo_storage_popup.py`
  - `scripts/ue/audit_plan147_echo_storage_popup.py`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - 适用的精选 `Binaries/Win64` Editor 预构建包及 manifest
- Stable Reads:
  - `Content/Data/cards.csv`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `scripts/ue/author_plan110_formal_shop_ui.py`
  - `scripts/ue/audit_plan110_formal_shop_ui.py`
  - `shared/CODEBASE_MAP/README.md`
- 影响模式：`SharedContract`；修改同一个商店 WBP 和 `UReEchoInventoryShopWidget` 的作者ing/运行时绑定边界，进入 main 前必须审计并适配其他商店 UI 变更。
- 兼容承诺 / 下游操作：保留 `G_3_02`、`PostTraitIntermission`、待存回响、三槽容量、存储/跳过/替换/指定回放 Delegate 与 Run/GameMode 权威语义；保留无 WBP 时的最低可用 C++ fallback。已有存档与卡牌数据不迁移。
- 明确排除：不修改回响录制/回放结算、卡牌数值与文案真源、商店商品/刷新逻辑、暂停/重开弹窗本身、其他页面视觉和玩法状态所有权。

## 锁定目标

把商店内现有纯 C++ 白盒“回响存储”弹窗替换为可在 `WBP_ReEchoInventoryShopScreen` Designer 中直接预览和微调的正式弹窗。视觉以 `重开弹窗.svg` 的弹窗构图为参考，使用提供的 `通用弹窗.png` 作为正式底板，并以 `暂停按钮浅.png` / `暂停按钮深.png` 承载按钮正常/悬停按下状态；标题和正文改为现有回响存储内容。

玩家点击已拥有的 `G_3_02「时空锚点」` 卡牌槽后，仍可在正式弹窗中查看容量、待存回响、已存回响和回放选择，并执行关闭、存储、跳过、满仓替换、取消替换及指定回放。未处理待存回响时尝试离店的确认层也复用同一套正式弹窗与按钮视觉。进入商店不自动打开主弹窗，原有开启时机不变。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI`、`AREA-UI`、`AREA-Tests`。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 已加入 `Writes`；前者关闭前审阅 Runtime Module 事实，后者同步回响弹窗从动态白盒转为 WBP 作者ing 的职责边界与验证入口。
- 设计意图：WBP 管理布局、尺寸、正式纹理、按钮状态和可读性；C++ 只投影真实状态、更新文字/显隐/启用状态并广播既有类型化事件。避免再次由 C++ 固化一套不可所见即所得的视觉。
- 权威状态与依赖：不改变 Run/GameMode 权威状态或公开玩法契约。`UReEchoInventoryShopWidget` 继续消费 `FReEchoEchoStorageSummary`，但优先复用 WBP 中的稳定命名控件；缺少完整作者ing 绑定时才构建最低可用 fallback。
- 决策记录：
  - 不把整张 1920×1080 SVG/截图导入成运行时贴图；SVG 只提供位置与构图参考，实际运行时使用三张透明正式切图，避免把 HUD、背景和示例文字烘死。
  - 主弹窗在同一商店 WBP 中作者ing，而不是新增拥有玩法状态的独立 Widget；这样保留现有父 Widget Delegate 和生命周期，同时让美术可直接调整弹层。
  - 已存三槽区域允许在通用底板内容区内紧凑排列；文字内容来自现有真实数据，不把示例 Encounter、角色或武器写进 WBP。
- 相关文档同步范围：关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `shared/CODEBASE_MAP/README.md`；预计拓扑和稳定索引不变，无需正文修改。维护 `MOD-ReEchoUI.md` 的商店回响作者ing事实；审阅 `MOD-ReEcho.md` 的代码位置与测试说明。
- 关闭前逐项填写审阅结果：待执行。

## 锁定验收

- [ ] `WBP_ReEchoInventoryShopScreen` Designer 中能直接看到并微调完整回响存储弹窗和离店确认层，运行时不重新写其位置与尺寸。
- [ ] 弹窗使用 `通用弹窗` 正式底板；交互按钮使用浅/深正式按钮状态，透明点击区与可见按钮一致，图片不拉伸变形。
- [ ] 标题为“回响存储”，正文保留容量、回放模式、待存信息、三条已存回响、选择状态及存储/跳过/替换等原有动态内容。
- [ ] `G_3_02` 点击开启、关闭弹窗、存储、跳过、满仓替换、取消替换、选择回放和未决离店门禁语义均不变。
- [ ] 无完整 WBP 绑定时仍有最低可用 fallback，且不会因单一可选节点缺失覆盖完整 authored 商店根。
- [ ] 聚焦 UI 自动化、资产审计、Blueprint 编译、Editor FullRebuild、项目校验、LFS 检查及 `git diff --check` 通过。
- [ ] 由用户在 PIE 验收 1920×1080 及至少一个非 16:9 分辨率下的构图、文字可读性、按钮状态和全部交互。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@050bffdfc7a37c4e047a02b719fb168bc6c30518`；该基线相对初始审计新增/更新 Plan 142、144、145、146 文档，均未修改商店 WBP/C++。Plan 146 的数据资产迁移为 `Exclusive` 大任务，但本任务不触碰其数据 Writes，只稳定读取既有 `G_3_02` 和回响摘要契约。
- 引擎/构建可用性：Git LFS `--check` 已通过；计划发布前确认没有 `UnrealEditor` / `UnrealEditor-Cmd` 进程。实现与最终发布仍执行准确候选 FullRebuild。
- 现有聚焦测试结果：基线 `ReEcho.UI.Shop.AuthoredLayoutHosts` 已在仓库中覆盖正式商店、`G_3_02` 点击入口及现有动态回响层；实现后扩展为正式弹窗作者ing和样式断言。
- 共享契约 / 难合并资源风险：`WBP_ReEchoInventoryShopScreen.uasset` 是二进制共享热点，无法文本合并；开始实现及发布前都必须 fetch 并审计该资产和商店 Widget 的外部变化。
- 基线损坏时的停止条件：LFS 未还原、Blueprint 编译失败、正式切图无法导入/透明通道异常、WBP 绑定不能保留 fallback，或远端出现需要产品取舍的同弹窗修改时停止越界部分并报告。

## 实现提纲

1. 将 SVG 参考与三张正式 PNG 归档到窄范围 SourceArt，通过幂等 Unreal Python 脚本导入三张 UI 纹理并校验尺寸、透明度和 TextureGroup。
2. 在 `WBP_ReEchoInventoryShopScreen` 的响应式 1920×1080 设计面上作者ing主弹窗和离店确认层，建立稳定命名的底板、标题、动态文字、三槽控件与按钮，按钮四态使用浅/深纹理且不保留历史内容 Padding。
3. 将 `UReEchoInventoryShopWidget` 改为优先绑定/复用这些 authored 控件，只更新真实文本、显隐、启用与事件；保留完整性门控的动态 fallback，且不覆盖 authored 几何。
4. 扩展商店 UI 自动化与 Python 资产审计，覆盖作者ing层级、纹理绑定、按钮状态、初始隐藏、`G_3_02` 点击开启及既有回响事件。
5. 更新模块文档和执行记录，完成格式化、Blueprint 编译、聚焦自动化、FullRebuild、静态/LFS 门禁，再交付 PIE 人工验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| LFS | `python scripts/setup_lfs.py --check`、`git lfs status`、`git lfs fsck` | 基线及候选资产均为真实文件，对象健康 |
| 纹理导入 | `scripts/ue/Run-EditorPythonLocked.ps1 -ScriptPath scripts/ue/import_plan147_echo_storage_popup.py` | 三张正式纹理存在、透明通道/TextureGroup/压缩设置正确 |
| WBP 作者ing | `scripts/ue/Run-EditorPythonLocked.ps1 -ScriptPath scripts/ue/author_plan147_echo_storage_popup.py` | 稳定命名节点创建或幂等更新，Blueprint Compile/Save 成功 |
| 资产审计 | `scripts/ue/Run-EditorPythonLocked.ps1 -ScriptPath scripts/ue/audit_plan147_echo_storage_popup.py` | 层级、几何、正式纹理、按钮状态和 Designer 可调性符合契约 |
| C++ 格式 | 对修改的 `.h/.cpp` 运行仓库 `.clang-format` | 格式化 diff 仅限目标文件 |
| C++ 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 退出码 0，并刷新匹配候选的精选 Editor 包 |
| UI 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.Shop.AuthoredLayoutHosts` | 主/确认弹层、纹理与既有回响交互断言通过 |
| Blueprint | `CompileAllBlueprints` | 目标 WBP 及全项目 Blueprint 编译无错误 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目与差异检查通过 |
| 人工 | PIE：获得 `G_3_02` 后完成战斗并进入战后商店，逐项操作主弹窗及未决离店确认 | 用户确认视觉、可读性、DPI 和交互 |

## 执行记录

### 变化

- 待执行。

### 证据

- 规划基线：`origin/main@050bffdfc7a37c4e047a02b719fb168bc6c30518`；已审计传入为 Plan 142 扩展及 Plan 144/145/146 文档，无物理冲突。Plan 145 存档语义与 Plan 146 数据权威仅作为 Stable Read 耦合关注，不进入本任务实现范围。
- 规划前 `python scripts/setup_lfs.py --check`：通过。

### 剩余风险

- 正式底板内容区高度需容纳三条真实回响记录与多状态按钮；最终字号和密度需要 PIE 人工验收。
- `WBP_ReEchoInventoryShopScreen.uasset` 为二进制共享热点，远端同资产变化会使此前资产/构建证据失效。

### 人工验收结果/请求

- `PendingBeforeClose`：等待实现候选后由用户 PIE 验收。

### 架构文档审阅结果

- 待实现完成后逐项填写。
