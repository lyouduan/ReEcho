# Plan 110 - 程序 - 正式商店 UI 接入

## 协调

- Planner 负责人：Codex（当前程序路线，规划执行合一）。
- Executor 负责人：Codex（当前程序路线，规划执行合一）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@42624169981adb9256321426584f781f66651cb4`。
- 本地实现方式（可选，仅作交接说明）：一任务一 worktree：`ReEcho-plan110-formal-shop-ui`，本地分支 `plan/110-formal-shop-ui`。
- 依赖 / 阻塞：依赖 Plan108 已发布的商店卡组预付费与继续选择契约；实现不得回退或绕过该契约。
- Writes:
  - `plans/110-formal-shop-ui.md`
  - `Content/SourceArt/UI/InventoryShop/Plan110/**`
  - `Content/ReEcho/Textures/UI/InventoryShop/Plan110/**`
  - `Content/ReEcho/UI/WBP_ReEchoInventoryShopScreen.uasset`
  - `Source/ReEcho/Public/UI/ReEchoInventoryShopWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoInventoryShopWidget.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopLogicBlockTests.cpp`（仅在绑定契约需要新增自动化证据时）
  - `scripts/ue/import_plan110_formal_shop_ui.py`
  - `scripts/ue/author_plan110_formal_shop_ui.py`
  - `scripts/ue/audit_plan110_formal_shop_ui.py`
  - `docs/ART_ASSET_ORGANIZATION.md`
  - `Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `Binaries/Win64/ReEchoEditor.prebuilt.json` 及其清单允许的精选 Win64 Editor 预构建产物
- Stable Reads:
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Source/ReEcho/Public/Run/ReEchoShopCatalog.h`
  - `Source/ReEcho/Private/Tests/ReEchoShopTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopLogicBlockTests.cpp`
  - `Design/Data/ReEchoData.xlsx` 及同步生成的商店相关 CSV（只读，不改变内容/价格/概率）
  - `C:/Users/gavynqiu/Documents/miniGame/正式-UI视觉/正式-UI视觉/商店/**`（交付源资产）
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：保持现有商品投影、折扣价、刷新次数与费用、配件购买/拥有/草稿装备/保存配置、卡组 1/2/3 级预付费/继续选择/领取、逐槽刷新、武器切换、回响存储、关闭与保存离店委托语义不变；保留现有稳定 Widget/Delegate 名称，新增 authored 控件使用稳定名称并由 C++ 幂等绑定。
- 明确排除：不修改商店经济数值、报价生成、抽卡池、卡组付款/退款规则、武器/配件兼容规则、存档 Schema、回响规则或关卡流程；不把整张效果图作为运行时页面；不导入未提供授权的字体；不为效果图中的 `1/2` 擅自新增玩法分页。

## 锁定目标

将 `正式-UI视觉/商店/1-商店.png` 的正式视觉资产接入 `WBP_ReEchoInventoryShopScreen`，在 `1920×1080` 作者设计面上实现左侧时间商店、中部武器装配室、右侧卡牌装配区以及保存离店入口的正式构图。所有可调位置、尺寸和主要样式由 WBP Designer 持有；运行时 C++ 只把真实商品、价格、图标、拥有/禁用状态和装备状态填入 authored 槽位，并继续转发现有类型化命令。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI`、`AREA-UI`。代码仍编译在 `MOD-ReEcho`，不新建 Runtime Module。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 已加入 `Writes`；若 C++ 绑定实现改变 `MOD-ReEcho` 的模块级公共事实，则同步维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，否则在关闭记录中具名说明无需修改。
- 设计意图：把正式商店的空间布局和美术样式从运行时 `BuildTargetShopPresentation()` 的坐标生成收敛到 WBP；C++ 保持状态投影、按钮绑定和委托转发职责，避免视觉微调必须改代码，也避免 WBP 直接写 Run 权威状态。
- 权威状态与依赖：Run/GameMode 继续拥有购买、扣费、刷新、所有权、装备草稿提交、卡组预付费/领取和存档权威；Widget 只消费只读 `FReEchoPartShopView` / offer projection 并发出既有命令。Plan 不改变公共 gameplay 契约或依赖方向，但会扩展/整理 WBP 与 C++ 间的 authored 控件绑定契约。
- 决策记录：
  - 正式切图按源图归档到 `Content/SourceArt/UI/InventoryShop/Plan110`，运行时 Texture2D 导入 `Content/ReEcho/Textures/UI/InventoryShop/Plan110`；效果参考图只归档，不作为整屏运行时纹理。
  - WBP 以具名 Canvas/槽位提供设计权威和 Designer 样例；运行时只替换文本、Brush、可见性和启用状态，不重排 authored 几何。
  - 现有 `DesignerLoadoutCanvas` 的武器面板、时钟、3 个配件槽和 12 个卡牌槽稳定名称继续保留，必要时仅在正式父面板内重新定位。
  - 商品三列和卡组三列继续映射现有稳定报价/卡组入口；Plan108 的 `PaidPendingChoice` 必须显示为继续选择且不得重复收费。
  - 目标图中的页码/箭头仅在已有真实分页状态时反映它；没有对应 gameplay 状态时保持非交互装饰或禁用，不创建第二套隐藏库存。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：更新正式商店 WBP/C++ 分工、稳定控件和验证路线。
  - `Design/UI/ReEcho_UI修改指导.md`：更新商店 Designer 可调节点、运行时绑定与人工验收步骤。
  - `docs/ART_ASSET_ORGANIZATION.md`：登记 Plan110 商店源图与运行时纹理路径。
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：关闭前审阅；预计依赖拓扑不变。
  - `shared/CODEBASE_MAP/README.md`：关闭前审阅；预计稳定模块路由不变。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：关闭前审阅；只有模块级契约事实变化才修改正文。
- 关闭前逐项填写审阅结果：
  - `<文档>` 已更新：列出与验收实现一致的具体变化；
  - `<文档>` 已审阅、无需修改：说明该实现为何不改变此文档中的事实。

## 锁定验收

- [ ] `WBP_ReEchoInventoryShopScreen` 在 `1920×1080` 下与 `1-商店.png` 的主要构图一致：左侧商店木框、货币/刷新区、配件与卡组三列，中部装配室/武器/配件槽，右侧卡牌槽与角色区域，以及底部保存离店按钮均使用正式资产。
- [ ] 正式页面的主要面板、商品槽、文字、按钮、武器面板、时钟、3 个配件槽、12 个卡牌槽和保存离店按钮可在 UMG Designer 中调整位置/尺寸；C++ 刷新不覆盖 authored 几何。
- [ ] 所有现有商店行为保持一致：购买、折扣扣费、已拥有/已装备状态、主刷新、配件草稿与保存配置、武器切换、卡组预付费/继续选择/领取/逐槽刷新、回响存储和离店流程均可用。
- [ ] WBP 提供可读的 Designer 样例内容，PIE 时由真实投影替换；样例不得参与运行时状态或消费命令。
- [ ] 正式源图、运行时纹理和 WBP 通过幂等导入/authoring/audit 脚本验证，引用无丢失，参考整图没有被当作运行时页面。
- [ ] 功能结果有可观察证据。
- [ ] 必需自动化/构建检查通过。
- [ ] 仅在手感、可读性、视觉质量或可用性需要判断时请求人工验收。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@42624169981adb9256321426584f781f66651cb4`。
- 引擎/构建可用性：UE 5.8 安装版与现有 `scripts/ue` 入口沿用主线；执行 Editor/Commandlet 前检查项目进程并取得 Git common-dir Unreal 锁。
- 现有聚焦测试结果：基线未在本 Plan 创建时重跑；主线自带 Plan108 商店测试与精选预构建包。实现阶段先运行静态审计和聚焦基线测试，再以最终候选重建。
- 共享契约 / 难合并资源风险：`WBP_ReEchoInventoryShopScreen.uasset` 是难合并二进制；`ReEchoInventoryShopWidget.cpp` 与 Plan108 直接耦合。实现以当前主线为唯一基线，不恢复旧 Plan45/91/99 的动态布局版本。
- 基线损坏时的停止条件：WBP 无法经 Unreal 加载/编译、现有 Plan108 商店测试失败、正式资产缺少可用透明/切图信息，或必须改变 gameplay 事务语义才能完成视觉接入时，停止越界部分并报告。

## 实现提纲

1. 审计正式商店切图的尺寸、透明通道、用途及与目标图对应关系，建立 ASCII 稳定命名清单并归档源图。
2. 通过 Unreal 导入运行时纹理，设置 UI 过滤/压缩/透明属性并验证资产可加载。
3. 在 WBP 中建立正式设计面与 Designer 样例，保留既有稳定逻辑节点和 loadout 槽名称，使全部主要几何可手调。
4. 将 `UReEchoInventoryShopWidget` 收敛为绑定 authored 控件、填充真实投影与转发既有委托；无 WBP 时保留最低可用 fallback，但不得覆盖正常 authored 页面。
5. 增加资产/WBP 结构审计与必要的商店绑定回归，逐项验证 Plan108 预付费及其他既有流程未回退。
6. 更新相关 UI/资产组织文档、执行记录和人工 PIE 验收清单；最终候选完成全量 Editor 重建与精选预构建包刷新。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | 项目/源码不变量通过 |
| 格式与差异 | `git diff --check`；修改 C++ 时对目标文件执行仓库 `.clang-format` | 无格式/空白错误，C++ diff 可审阅 |
| 资产导入 | `scripts/ue/Run-PythonScript.cmd scripts/ue/import_plan110_formal_shop_ui.py` | 正式 Texture2D 导入成功且可加载 |
| WBP authoring | `scripts/ue/Run-PythonScript.cmd scripts/ue/author_plan110_formal_shop_ui.py` | WBP 编译/保存成功，稳定节点和 Designer 几何存在 |
| WBP 审计 | `scripts/ue/Run-PythonScript.cmd scripts/ue/audit_plan110_formal_shop_ui.py` | 正式纹理引用、稳定绑定、可调槽位和无整屏参考图消费均通过 |
| C++ 最终构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新准确精选预构建包 |
| 商店行为回归 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Shop` 与受影响 UI 聚焦测试 | 购买、刷新、装备、预付费/领取与页面绑定通过 |
| 蓝图 | `CompileAllBlueprints` 或等价项目入口 | `WBP_ReEchoInventoryShopScreen` 无编译错误 |
| 人工 | PIE 检查正式商店 1920×1080、低分辨率/超宽缩放、悬停/点击/禁用状态及全部既有流程 | 用户确认视觉与交互通过 |

## 执行记录

### 变化

- 已建立正式 Plan；尚未开始实现。

### 证据

- 远端编号审计：`origin/main` 最大正式编号为 Plan109，本任务使用 Plan110。
- 外部提交审计后，用户确认采用 `origin/main@42624169981adb9256321426584f781f66651cb4` 作为新基线。

### 剩余风险

- 正式商店目标图包含大量合成区域，需根据提供切图重建，不允许直接使用整屏效果图；最终像素级观感需要人工 PIE 验收。
- WBP 为二进制难合并资产；发布前必须再次审计远端是否有同资产修改。

### 人工验收结果/请求

- `PendingBeforeClose`：实现完成后由用户在 PIE 中验证正式视觉和全部商店交互。

### 架构文档审阅结果

- 待实现完成后填写。
