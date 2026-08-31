# 商店刷新文本蓝图作者化

- 基线：`fad6d0813fdd84d05cad0bba06769bf2af5b10b5`。
- 工作区：`ReEcho-shop-refresh-authored-text`；分支：`fix/shop-refresh-authored-text`。
- 类型：单按钮表现绑定的小型修复；不发布远端，不迁移数据表。
- 目标：`WBP_ReEchoInventoryShopScreen` 内直接预览并编辑刷新文本的位置、字体、字号、颜色、对齐与换行；运行时只投影刷新次数/费用等文字内容、按钮可用状态和既有点击事件。
- 影响：`MOD-ReEcho` / `AREA-UI`，控件表现由 WBP 持有；Run 的刷新状态、扣费和随机数逻辑、公共数据契约均不变。
- Writes：`ReEchoInventoryShopWidget.h/.cpp`、刷新文本聚焦测试、目标 WBP、窄范围作者化脚本、匹配的精选预构建包、本记录、`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoUI.md`、相关 UI 使用说明。
- Stable reads：现有刷新文案分支、商店报价结构、`ShopRefreshButton` 点击委托、其他按钮与用户已发布的两份蓝图。
- 实现原则：优先复用完整作者化内容，禁止 legacy fallback 的 `SetContent` 覆盖；无 WBP 时保留最低可用兜底。蓝图只迁移目标按钮内部，其他几何/样式保持原样。
- 验证：构建、目标蓝图编译/保存、作者值及任意微调值在构造/刷新/重建后保持、免费/付费/无限/禁用状态、原生 fallback、`git diff --check`、LFS/预构建检查。
- 已知基线问题：项目校验中 `Card.EasterShardThreshold` 白名单缺失；XLSX 与 `cards.csv` / `card_effects.csv` 有差异。上轮一次性发布豁免不复用于本任务；不为此修改表或放宽校验。
- 状态：本地实现完成；人工视觉验收待用户测试，未推送远端。

## 实现与证据

- `ShopRefreshCountText` 改为可选蓝图绑定；完整作者化路径不再创建旧文字或重建按钮 Content。动态 `RefreshLabel` 的全部文案分支未改，次数、费用、随机数、扣款与 Run 状态均未修改。
- WBP 只新增 `DesignerRefreshOverlay`、`DesignerRefreshTextCanvas`、`DesignerRefreshTextBox`、`ShopRefreshCountText` 四个控件。按钮原始 Canvas 位置与尺寸保留，原底图仅重挂父容器，文字默认复现 9 号黑色居中样式并提供样例；迁移检查保护其他 146 个控件的父级、几何和主要表现属性。
- 初次迁移在保存前被保护检查拒绝，磁盘原资产哈希仍为 `5b20559ca7a6c87d28eb1e1fe78000c5d9dd783f`。原因：手动移出已有变量控件，再调用 AddWidget，会触发编辑器中间编译，破坏该控件的 GUID/Outer。改用 UMGToolSet `WrapWidgets` 原子包裹并重新取得控件引用后成功。未保存失败状态，也未修改引擎。
- 保存后资产哈希：`27cb2c934c65ac5d6443cb3c4808759b9a220008`。再次运行迁移脚本报告 `Already authored; preserved edits, no save`，哈希不变。
- `Build-Editor.cmd -Configuration Development`：成功，98 actions，42.49 秒；预构建包 7 modules，UE Build ID `55116800`，source fingerprint `22c1fcf22440`。仅有基线 `FImageUtils::CompressImageArray` 弃用警告。本次没有执行发布所需的额外 `-FullRebuild`，也没有打包。
- `Run-Automation.cmd -Filter ReEcho.UI.Shop`：6/6 成功。包括 `AuthoredLayoutHosts`、新增 `AuthoredRefreshText`、`CardPackChoicePresentation`、`LogicBlocks`、`RuneBackpackFiltersWeaponType`、`TooltipBlueprints`。新增测试覆盖保存样式与任意微调样式，构造前传入数据、首次构造、刷新、重建，免费/付费/无限次数及禁用状态，并确认重建后一次点击只广播一次既有刷新事件。
- 本地证据：`Saved/Publication/build.log`、`Saved/Publication/shop-tests.log`；蓝图迁移成功日志时间 `2026.08.31-05.53.00`，幂等检查时间 `05.55.06`（UE 日志时间）。
- `python -m py_compile scripts/ue/author_shop_refresh_text.py`、LFS checkout/fsck、预构建指纹检查、`git diff --check` 均通过。
- 全局 `validate_project.py` 仍因基线行为白名单失败；`sync_xlsx_to_csv.py --check` 仍报告同两份 CSV 漂移。这两项未通过，也没有为本任务重新豁免。未改动任何 CSV/XLSX 或上轮发布的 About/TraitCardChoice 蓝图。

## 文档审阅

- `MOD-ReEcho.md`、`MOD-ReEchoUI.md`：已更新，记录刷新文本表现权威与兼容边界。
- `Design/UI/ReEcho_UI修改指导.md`：已更新，提供文字与可拖动外框的准确层级。
- `CODEBASE_MAP/ARCHITECTURE.md`：已审阅，无需修改；模块拓扑、Run 状态所有权和刷新命令契约未变。
- `CODEBASE_MAP/README.md`：已审阅，无需修改；仍归既有 `AREA-UI` / `MOD-ReEcho`，未新增架构路由。

## 人工测试入口

打开本任务 `ReEcho-shop-refresh-authored-text/ReEcho.uproject`，在 `WBP_ReEchoInventoryShopScreen` Designer 选中 `ShopRefreshCountText` 改字体/颜色/对齐，选中 `DesignerRefreshTextBox` 调整位置和尺寸。进入商店核对实际文案、可读性及悬停/点击；这是视觉验收，不用重新运行迁移脚本。
