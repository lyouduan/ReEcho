# Plan 157 - 程序 - 商店说明与属性浮窗蓝图化

## 协调

- Planner 负责人：Codex（当前程序对话内兼任）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`。
- 人工验收：`Passed`。
- 本地规划 / 实现基线：`origin/main@d62a088a8f732a90ff9b1b6ac4b6f1d681e44981`。
- 本地实现方式：独立分支 `plan/157-shop-tooltip-blueprints`、worktree `ReEcho-plan157-shop-tooltip-blueprints`。
- 依赖 / 阻塞：UE 资产操作及构建前确认没有交互式 Editor 使用本克隆；最终视觉与微调行为由用户验收。
- Writes: 本 Plan；`Source/ReEcho/{Public,Private}/UI/ReEchoInventoryShopWidget.*`；新增 `ReEchoShopTooltipWidget.*`、`ReEchoAttributeTooltipWidget.*`、`ReEchoAttributeRowWidget.*`；`Source/ReEcho/Private/Tests/ReEchoShopTooltipTests.cpp`；`Content/ReEcho/UI/WBP_ReEcho{ShopTooltip,AttributeTooltip,AttributeRow}.uasset`；`scripts/ue/{author,audit}_plan157_shop_tooltips.py`；`Design/UI/ReEcho_商店浮窗调整指南.md`；`Design/UI/ReEcho_UI修改指导.md`；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`；构建门禁精选 Win64 Editor 预构建包。
- Stable Reads: `ReEchoLoadoutTooltipWidget.*`、`ReEchoLoadoutEntryWidget.*`；`WBP_ReEchoLoadoutTooltip`、`WBP_ReEchoInventoryShopScreen`；`FReEchoShopOffer`、`FReEchoStatBlock`、CSV 属性目录和 Run 已生效结果投影；原商店聚焦测试及 UMG authoring 工具。
- 影响模式：`SharedContract`。
- Writes 补充：`Source/ReEcho/Private/Tests/ReEchoShopLogicBlockTests.cpp`（保留原断言语义，将旧动态树断言适配到新 Blueprint 控件契约）。
- 本轮用户追加 Writes：`ReEchoLoadoutSelectionWidget.*`、`ReEchoLoadoutSelectionTests.cpp`、`WBP_ReEchoLoadoutSelection.uasset`、`scripts/ue/repair_plan157_tooltip_backplates.py`。同一 Plan 内修复商店背板，并按用户明确要求修正选择页按钮状态；不改其布局或字体。用户已微调的 `WBP_ReEchoLoadoutTooltip.uasset` 只读保留，不运行旧整页生成器。
- 属性图标反馈追加 Writes：`scripts/ue/repair_plan157_attribute_icon_size.py`；用户确认修复已定位的 `AttributeIcon.Brush.ImageSize=0×0`，只补有效图片尺寸，保留最新位置、字号、字体、间距和外层尺寸。同步原生成器、审计及具名渲染测试防回归，不修改运行时布局权威。
- 兼容承诺 / 下游操作：保留所有原悬停命中面、Slate 原生定位避让、商品购买/装备/卡牌状态和属性格式；运行时只更新数据及显隐，不重置 Blueprint 字体、颜色、描边、宽度、间距和布局。
- 明确排除：不重建商店整页、不改写开场说明浮窗、不修改策划表或玩法数值、不推导卡牌实际效果、不改存档、点击选中规则、返回目的地或交易语义；本轮开场追加只改按钮常驻与明暗/可用性，不覆盖人工微调。

## 锁定目标

1. 商店角色被动、武器、配件、已获得卡牌和卡组说明统一使用真实可编辑的独立 Widget Blueprint，提供有代表性的示例标题和说明。
2. 已获得卡牌在说明下方的“实际效果”附加面板也必须在 Designer 中可见、可编辑，示例包含多项已生效结果。运行时仍只消费 `OutcomeText`；为空时折叠附加面板且不留空白。
3. 角色属性浮窗及其属性行由独立 Blueprint 持有样式，Designer 显示属性图标、名称、数值和完整列表示例；运行时按现有 CSV 顺序填充真实属性。
4. 所见即所得覆盖尺寸、字体/字号/颜色/描边、边框、Padding 与自动换行；编译、保存、重新打开、运行时填充都不能把人工样式改回程序常量。
5. 用户验收追加：商店独立浮窗必须保留可见背板和边框（可参考开场选择浮窗），只修背景绘制，不重置已调字体和尺寸。选角色/武器两页的确认和返回始终显示；未选中时确认禁用且置灰；返回始终可用，非悬停置灰、悬停点亮，保留已有返回流程与防重复确认。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI` / `AREA-UI`。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `MOD-ReEchoUI.md`，均在 Writes。
- 设计意图：消除商店 `BuildSlotTooltip` / `BuildAttributePanel` 内的临时布局与写死字体，将表现权威交给 Blueprint；商店仍为原有数据及原生 Tooltip 生命周期的宿主。
- 权威状态与依赖：Run/CSV 保持角色、物品与已生效效果的唯一来源；新 Widget 只消费展示文本、图标和属性行，不写回游戏状态。新增 Widget 在现有 ReEcho Runtime Module 内，不引入模块依赖。
- 决策记录：物品浮窗使用一个包含主说明与可选实际效果区的完整 WBP，保证同一 Designer 能预览两块组合；属性浮窗使用可编辑属性行 WBP，并在完整浮窗里放入真实行实例作为示例。保留现有白边近黑底、实际效果金边的视觉语义。
- 相关文档同步范围：审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoUI.md`；新增专门的控件微调指南并从总 UI 指南链接。

## 锁定验收

- [x] 三个 WBP 均有非空的所见即所得示例；完整卡牌浮窗预览实际效果区，属性浮窗预览完整列表。
- [x] 角色被动、武器、配件、卡组、已拥有卡牌的悬停说明全部复用新物品浮窗。
- [x] 非空 OutcomeText 原样显示；空结果折叠；多行/多项结果和长正文采用自动高度与换行，无固定高度裁切（长文本视觉验收仍列于人工项）。
- [x] 真实属性顺序、图标、整数与百分比格式保持原契约，示例数值不泄漏到游戏。
- [x] 改动蓝图样式后，运行时 Configure/刷新不覆盖；重复属性刷新无重复行或残余旧值。
- [x] 编译、资产审计、聚焦自动化、项目校验与 diff 检查通过；只跟踪允许的预构建产物。
- [x] 用户完成微调及反馈修复后，于 2026-08-31 明确要求将本候选推送合入主分支，确认接受当前交付并授权发布；AI 不代称执行了人工 PIE。
- [x] 商店主说明、实际效果和属性浮窗的背景/边框使用真实可绘制 Brush，真实 GPU 渲染四边可见；保留文字样式和宽高，仅修正会遮住外框的零留边。
- [x] 两阶段的确认/返回常驻可见；未选中确认禁用置灰、悬停不选中；返回空闲暗、悬停亮且始终可返回（最终提交后防重复操作，待人工观感验收）。

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
| 渲染 | `ReEcho.UI.TooltipRendering.Backplates`（`-RenderOffscreen`，不可用 NullRHI） | 实际 GPU/Slate 截图及主说明、实际效果、属性面板四边像素断言通过 |
| 人工 | 新浮窗 Designer 与商店 PIE | 字体微调、长说明/实际效果、属性列表、避让定位符合预期 |

## 执行记录

### 变化

- Plan-only 已发布：`83cb1e0e`，远端核验包含准确 Plan；随后开始本地实现。
- 已添加三个数据展示类及一次性 Blueprint 创建脚本，保留已存在资产不重置其样式；未改原商店和开场 WBP。
- 商店物品及属性生成入口已改接新类；原聚焦测试保留语义并适配作者化控件，新增样式保留、示例/真实数据隔离、结果显隐和属性列表刷新测试。
- 已补充微调指南和模块契约；三个 WBP 已编译保存并在新进程中审计，可以交付独立工程验收。
- 三个新 WBP 分别包含物品双面板、完整八行属性列表与可复用属性行。默认 Desired Size，中文字体为已入库的方正黑体，正文自动换行、容器自动高度。仅更新数据和必要显隐，不写字体/颜色/宽度/Padding。
- 两处测试适配：预览测试持有 Slate 强引用后再做 Layout Prepass，避免弱引用缓存失效误报零尺寸；基线刷新按钮已是 Overlay（图片 + 次数文本），旧测试仍假定直接图片，改为验证 Overlay 中原正式贴图及归属，未修改刷新按钮行为或削弱贴图断言。
- 对修改的 C++ 文件运行仓库格式化，原商店文件同时包含纯排版差异；语义 diff 已审查，未改变交易、选择、已有微调和卡牌结果计算。

### 证据

- 规划基线 `d62a088a`；本次不改已有主工程 `ReEcho.uproject` 的未提交内容。
- `python scripts/setup_lfs.py --check` PASS（3 个 LFS 文件）；author/audit Python 语法检查 PASS；最终 `python scripts/validate_project.py` 与 `git diff --check` PASS。
- 初次 Editor 构建 C4458 已改名 RowSlot 修复。最终 `Saved/Logs/Plan157-Build3.log`：UBT Succeeded，预构建 7 模块刷新，build_id `55116800`，源码指纹前缀 `bb2c752c14f1`。预构建清单校验通过，不是 Live Coding 热补丁。
- 资产创建：`Saved/Logs/ReEcho-session-20260831-005618-pid8944.log` 中 `[Plan157] PASS`；独立进程审计 `Saved/Logs/ReEcho-session-20260831-005708-pid47912.log` 中 `[Plan157Audit] PASS`，验证三蓝图真实层级/绑定、非空中文样例、多行实际效果、自动高度、等比例图标、八行属性实例和运行时行类引用。
- `ReEcho.UI.Shop.` 五项全部 Success，退出码 0：`AuthoredLayoutHosts`、`CardPackChoicePresentation`、`LogicBlocks`、`RuneBackpackFiltersWeaponType`、`TooltipBlueprints`。日志 `Saved/Logs/Plan157-Automation2-Engine.log`（同 `ReEcho-session-20260831-010321-pid47240.log`）。
- 新测试覆盖 Designer 非零尺寸/Desired Size、实际文本与多行结果、空/空白结果折叠与重现、字号/描边/颜色/宽度/Padding 保留、属性行复用/增减/清空、CSV 顺序、42 血和 123% 等真实值隔离。既有测试确认配件、卡组和卡牌 Tooltip 路由无回归。
- 额外的生成脚本二次运行哈希检查未执行：用户另一个工程 `ReEcho-fix-remove-spawn-debug-spheres` 的交互式 Editor 已打开，入口安全阻止启动。未关闭该会话；此项为额外检查，不替代上述已通过的必需资产审计/自动化。代码已审阅：已有资产直接返回，不重新 Compile/Save。日志 `Saved/Logs/Plan157-Idempotence.log`。

### 剩余风险

- 长文本高度随数据变化，Designer 示例只能代表示例内容；运行时仍需验证窗口避让。
- 必需客观验证已通过；没有由 AI 代做 PIE 视觉验收，长文本、屏幕边缘避让以及用户微调后的观感仍待用户检查。
- 本地实现尚未发布；后续发布须保留用户微调，重新审计当时 main 并执行 FullRebuild、预构建及发布锁门禁。

### 人工验收结果/请求

- `PendingBeforeClose`：请在 `C:/Users/gavynqiu/Documents/miniGame/ReEcho-plan157-shop-tooltip-blueprints/ReEcho.uproject` 验收。物品说明/实际效果：`WBP_ReEchoShopTooltip` 的 `DescriptionText` / `OutcomeText`；完整属性：`WBP_ReEchoAttributeTooltip`；属性行字体：`WBP_ReEchoAttributeRow` 的 `AttributeNameText` / `AttributeValueText`。操作说明位于 `Design/UI/ReEcho_商店浮窗调整指南.md`。

### 架构文档审阅结果

- `MOD-ReEcho.md`、`MOD-ReEchoUI.md`：已更新并复核只读展示数据、Blueprint 表现权威、样例/真实数据边界及代码落点，与最终候选一致。
- `Design/UI/ReEcho_UI修改指导.md`：已添加入口链接；新微调指南列出稳定控件和实际效果/属性预览边界。
- `ARCHITECTURE.md`、`README.md`：已审阅，现有 AREA-UI 路由和 Runtime Module 拓扑未改变，无需修改；关闭/集成基线变化时再次复核。

### 本轮反馈修正：背板与按钮常驻

- 后续用户仍看不到边框：本轮增加 `ReEchoShopTooltipTests.cpp` 中具名 GPU 离屏渲染诊断，保存真实 Slate PNG 检查边框是否被覆盖；不再以 Brush 资源存在代替可见性证据。只修浮窗绘制，不再改按钮，不动字体/尺寸。诊断代码仍在既有测试 Writes 内。

- 用户已调好字体和大小，保留当前 worktree 的全部已保存微调，包括用户改动的 `WBP_ReEchoLoadoutTooltip.uasset` 与 `ReEcho.uproject` 的 EngineAssociation=5.8。修复前快照在 `Saved/Plan157-UserSnapshot-20260831-012508/`；不进入 Git，不替代用户资产。
- 只读资产检查发现六个 Border 的 Background 均为 `IMAGE`、`resource=None`，仅 BrushColor 非空；这解释了控件存在、数据正常而没有实际背板。补上开场说明框同族的真实九宫格外框与白色纹理着深色内板，维持原边框色（实际效果金色）、字体与布局。
- `repair_plan157_tooltip_backplates.py` 默认仅检查，显式 `REECHO_PLAN157_APPLY=1` 才在 UE 内修改指定 Brush。Compile/Save 前后保护字段核对：物品浮窗 125、属性浮窗 89、选择页 175，共 389 项文字/字体/布局值相等；属性行与开场说明 WBP 的 SHA256 和用户快照一致。未运行旧整页生成器。
- 确认原先在设计期/运行期 `RefreshSelection` 中按选中状态 Collapsed。新增内部 `RefreshActionButtons`，两按钮和独立标签常驻，确认无候选禁用，返回仅视觉灰态不禁用；Hover/Unhover 更新返回标签灰显，WBP 四状态 Brush 控制按钮图片明暗。末次提交后仍禁用两按钮，后续刷新不能重新启用确认。
- 第一次选择页资产测试拦住了 UE Python 嵌套 Struct 视图别名使 Normal 也被染暗的问题；修复为先 `.copy()` 再设各状态 Tint，恢复确认正常态 1.0、禁用态 0.5，返回空闲/禁用 0.5、悬停/按下 1.0。未删除或放宽测试断言。
- 最终构建：`Saved/Logs/Plan157-Build4.log`，UBT Succeeded、7 模块预构建刷新、源码指纹 `e10c860c965c`；现有 GameMode 弃用 API 警告与本轮无关。
- 最终自动化：`Saved/Logs/Plan157-Final-Shop-Engine.log` 5/5 Success；`Saved/Logs/Plan157-Final-LoadoutSelection-Engine.log` 2/2 Success；均退出码 0。测试覆盖六块真实背板资源、按钮常驻/禁用、标签灰显/悬停、正式按钮资源及 Tint、旧点击选择/返回与防重复提交契约。
- 独立进程重新加载审计 PASS：`Saved/Logs/Plan157-Audit2-Engine.log`；生成脚本再次运行后五个相关 WBP SHA256 全部不变，见 `Saved/Logs/Plan157-Idempotence2-Engine.log` 和命令输出。最终 Python 语法、项目校验及 `git diff --check` PASS。
- 模块文档和微调指南补充背景资源要求、按钮四态及独立标签灰显；同一 ReEcho 模块内的私有实现调整，没有公共 API、Run 权威、存档或架构拓扑变化。
- 人工状态仍为 `PendingBeforeClose`：用户字体/大小已经调好，本轮背板和按钮观感需在同一 Plan157 工程复测；未发布实现。

### 再次反馈修正：边框被内层底板覆盖

- 用户反馈“没有看到边框”后，用非 NullRHI 的 `FWidgetRenderer` 渲染实际 WBP 并读取 GPU 像素。修复前 ShopTooltip / Outcome / Attribute 三块框的 12 个边缘采样全部失败，而开场 LoadoutTooltip 参考框通过；日志 `Saved/Logs/Plan157-RenderRed-Engine.log`，修复前截图保存在 `Saved/Automation/Plan157/Before/`。上一轮真实 Brush 检查不足以证明最终边框可见。
- 根因：生成器 `add()` 在添加子控件后统一清零 `UBorderSlot.Padding`，引擎将该值同步写回父 Border，覆盖预设的 3px 留边。内层实心底板因而铺满外框并遮住全部边缘。生成器已排除 BorderSlot，既有资产不重建；专用 `REECHO_PLAN157_FRAME_INSETS=1` 模式只修正三块外框与其内容 Slot 的 Padding 为至少 3px。
- 修复前两目标资产快照：`Saved/Plan157-BeforeInsets-20260831-014641/`。首次保护性比较发现 Unreal Struct 副本不能用对象相等判断内容，操作在保存前中止；改用 `export_text()` 冻结字段值后重试通过。`Saved/Logs/Plan157-InsetRepair2-Engine.log` 记录物品浮窗 121 项、属性浮窗 87 项受保护的文字/样式/布局值逐项相等；只排除明确要修复的外框 Padding 和对应内容 Slot Padding。用户字体、字号、宽高覆盖值不变，属性行和开场说明 WBP 的 SHA256 与原用户快照一致。
- 最终真实渲染通过：`Saved/Logs/Plan157-RenderAfter-Engine.log`，`ReEcho.UI.TooltipRendering.Backplates` Success、退出码 0。已人工查看生成的 `Saved/Automation/Plan157/ShopTooltip.png` 与 `AttributeTooltip.png`，主说明浅色框、实际效果金框及属性浅色框均有清晰四边；参考 `LoadoutTooltip.png` 保留。渲染用短测试数据，不是用户 PIE 验收。
- 最新代码构建 `Saved/Logs/Plan157-Build8.log`：Succeeded，7 模块精选预构建刷新，engine build `55116800`、source fingerprint 前缀 `01af7c09fa26`。此后仅修改资产、Python 和文档，未再改 C++。
- 修复后 `Saved/Logs/Plan157-Inset-Shop-Engine.log` 商店 5/5 Success、退出码 0；新增正值留边/父子 Slot 同步断言通过。独立进程加载资产审计 `Saved/Logs/Plan157-Inset-Audit-Engine.log` PASS。Python 语法、项目校验、预构建清单及 `git diff --check` 均通过。选择页本轮未改，沿用上轮 Flow/Assets 2/2 证据。
- 微调指南与 `MOD-ReEchoUI.md` 已补充父 Border/子 BorderSlot 的 Padding 同步语义、3px 留边及真实 GPU 渲染入口。复核 `MOD-ReEcho.md`，未改变模块拓扑或数据权威，无需追加结构变更。
- 本地交付仍在原 Plan157 工程，未推送。保留 `PendingBeforeClose`，请用户复测商店实际悬停效果。

### 属性图标零尺寸修复

- 用户在微调位置后反馈属性左侧图标缺失，并明确要求修复。只读诊断 `Saved/Logs/Plan157-IconSizeInspect-Engine.log` 确认八张正式贴图均能加载、可见性/透明度正常、外层为 28×28，但内部 `AttributeIcon.Brush.ImageSize` 为 0×0；居中 ScaleBox 无有效图片期望面积。运行时只替换资源，所以保留了这个无效尺寸。不是丢失素材，也不需要回退用户位置调整。
- 保存前备份用户最新属性行资产到 `Saved/Plan157-BeforeIconSize-20260831/`。在 UE 内只把图片 Brush 的 ImageSize 修为 28×28，保留全部 62 项文字/字体/插槽/变换/外层尺寸保护值；编译保存成功，见 `Saved/Logs/Plan157-IconRepair-Engine.log`。属性完整浮窗、物品浮窗、开场浮窗三资产哈希修复前后不变。
- 生成器显式作者化非零图标期望尺寸；修复脚本默认只读，显式启用才修零尺寸，已有非零人工值直接保留。资产审计新增非零尺寸及纹理检查；运行时仍只写数据，不新增强制回填布局常量。
- 扩展原 GPU 测试填入 CSV 全部八项正式图标；验证每张图非零屏幕面积、区域内存在非纯色美术像素。`Saved/Logs/Plan157-IconRender-Engine.log` 的 Backplates Success、退出码 0；已查看 `Saved/Automation/Plan157/AttributeTooltip.png`，八张图与边框全部可见。渲染使用测试属性值，不是实际存档属性或用户 PIE 验收。
- 最终构建 `Saved/Logs/Plan157-Build10-Icons.log` Succeeded，7 模块精选预构建刷新，build_id `55116800`、source `ed2ed7f7f0f5`。商店 `Saved/Logs/Plan157-Icons-Shop-Engine.log` 5/5 Success（含真实商店属性图标绑定/尺寸检查），退出码 0。Python 语法、项目校验、预构建清单和 diff 检查通过。
- 微调指南与 `MOD-ReEchoUI.md` 补充 ImageSize 与外层 SizeBox 的职责区分及回归检查；`MOD-ReEcho.md` 拓扑与只读契约未变。保持原工程、原 Plan，未提交或发布本地实现。
- 独立 UE 进程重新加载保存资产审计通过：`Saved/Logs/Plan157-Icons-Audit-Engine.log` 的 `[Plan157Audit] PASS`，确认非零尺寸已落盘。交回用户在同一 Plan157 工程复测，人工验收继续 `PendingBeforeClose`。

### 发布前验收与外部审计（2026-08-31）

- 用户在属性图标恢复后明确要求“推送到远端合入主分支”，本次人工状态更新为 `Passed`。全部最新已保存 UI 微调纳入候选，保持 EngineAssociation=5.8；发布前没有运行生成器重置用户布局。
- 已 fetch 并重读远端 AGENTS / Git / Planner 权威规则；当前 main `efe05ad3`，相对 Plan-only 基线新增刷怪调试球移除、攻速 300% 上限、一级卡牌数值及配套 XLSX/CSV、属性图标 AlwaysCook。只读 diff 审计：无新增 Plan 编号冲突；不触碰本任务 UI 源码或蓝图，保留外部 Gameplay/表格/Cook 变更。预构建包存在双边更新，需要最终 FullRebuild；`MOD-ReEcho.md` 的波次段与本任务 UI 段可组合保留，无产品取舍。
- 先保存完整本地正式候选，再按发布锁协议合入最新 main，重跑最终构建、资产/自动化/静态/LFS 门禁。此处不复用本地增量构建作为发布证据。
