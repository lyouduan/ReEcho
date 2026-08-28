# Plan 147 - 程序 - 回响存储正式弹窗 UI

## 协调

- Planner 负责人：当前程序用户与 Gavyn-side AI。
- Executor 负责人：Gavyn-side AI。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Closed`。
- 人工验收：`Passed`。
- 本地规划基线：`origin/main@050bffdfc7a37c4e047a02b719fb168bc6c30518`；Plan 发布后的实现基线：`origin/main@fb0538d65a99defab073b857b71c31aa0607a4e6`。
- 本地实现方式（可选，仅作交接说明）：按当前对话已确认的“一任务一 worktree”模式，从发布本 Plan 后的准确 `origin/main` 创建专属 worktree。
- 依赖 / 阻塞：正式参考 `C:/Users/gavynqiu/Downloads/重开弹窗.svg`；正式切图 `暂停按钮浅.png`、`暂停按钮深.png`、`通用弹窗.png`；实现前必须通过 Git LFS 检出检查并确认 Unreal Editor 已关闭。与 Plan 146 的数据资产迁移并行时，本任务只消费稳定 `G_3_02`/回响摘要契约，不修改其数据权威或迁移表面。
- Writes:
  - `plans/147-formal-echo-storage-popup-ui.md`
  - `Content/SourceArt/UI/InventoryShop/EchoStorage/`
  - `Content/ReEcho/Textures/UI/InventoryShop/EchoStorage/`
  - `Content/ReEcho/UI/WBP_ReEchoInventoryShopScreen.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoStoragePopup.uasset`
  - `Content/SourceArt/UI/WeaponParts/Icons/`
  - `Content/ReEcho/Textures/UI/WeaponParts/Icons/`
  - `Content/SourceArt/UI/Cards/Icon/T_UI_CardIcon_G_1_01.png`
  - `Content/SourceArt/UI/InventoryShop/Plan110/Elements/`
  - `Content/ReEcho/Textures/UI/InventoryShop/Plan110/`
  - `Content/SourceArt/UI/IconCatalog/`
  - `Content/ReEcho/Textures/UI/IconCatalog/`
  - `Source/ReEcho/Public/UI/ReEchoInventoryShopWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoInventoryShopWidget.cpp`
  - `Source/ReEcho/Public/Core/ReEchoTypes.h`
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Source/ReEcho/Public/Run/ReEchoRunSaveGame.h`
  - `Source/ReEcho/Public/Run/ReEchoShopCatalog.h`
  - `Source/ReEcho/Public/ReEchoGameMode.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Public/UI/ReEchoEchoManagementWidget.h`（删除）
  - `Source/ReEcho/Private/UI/ReEchoEchoManagementWidget.cpp`（删除）
  - `Source/ReEcho/Public/UI/ReEchoStoredEchoEntryWidget.h`（删除）
  - `Source/ReEcho/Private/UI/ReEchoStoredEchoEntryWidget.cpp`（删除）
  - `Source/ReEcho/Private/Tests/ReEchoEchoReplayRuntimeTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoEchoStorageTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoSaveGameTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopEchoSelectionTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopLogicBlockTests.cpp`
  - `scripts/ue/import_plan147_echo_storage_popup.py`
  - `scripts/ue/author_plan147_echo_storage_popup.py`
  - `scripts/ue/audit_plan147_echo_storage_popup.py`
  - `scripts/ue/migrate_plan147_echo_storage_popup_widget.py`
  - `scripts/ue/migrate_plan147_single_time_anchor_popup.py`
  - `scripts/ue/import_weapon_part_icons.py`
  - `scripts/ue/import_plan147_shared_icon_catalog.py`
  - `scripts/ue/audit_plan147_complete_shop_icons.py`
  - `docs/UI_ICON_REPLACEMENT_GUIDE.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`
  - 适用的精选 `Binaries/Win64` Editor 预构建包及 manifest
- Stable Reads:
  - `策划数据源/icon/`
  - `Content/Data/cards.csv`
  - `scripts/ue/author_plan110_formal_shop_ui.py`
  - `scripts/ue/audit_plan110_formal_shop_ui.py`
  - `shared/CODEBASE_MAP/README.md`
- 影响模式：`SharedContract`；修改同一个商店 WBP 和 `UReEchoInventoryShopWidget` 的作者ing/运行时绑定边界，进入 main 前必须审计并适配其他商店 UI 变更。
- 兼容承诺 / 下游操作：保留 `G_3_02`、`PostTraitIntermission`、待处理回响、设为锚点/跳过与 Run/GameMode 权威语义；持久状态只保留 `CardState.Runtime.AnchorRecordingId` 指向的一条时间锚点记录。旧存档 `StoredEchoes` 仍可反序列化，但恢复时只折叠为当前锚点；`StorageCapacity`、`SelectedReplayIds` / `SpecificReplayLimit` 仅保留字段兼容并在新存档中写零/空；保留无 WBP 时的最低可用 C++ fallback。
- 明确排除：不修改回响录制数据、普通自动回放的 Latest/Previous 规则、卡牌数值与文案真源、商店刷新逻辑、暂停/重开弹窗本身、其他页面视觉和玩法状态所有权。

## 锁定目标

把商店内现有纯 C++ 白盒“回响存储”弹窗替换为独立的 `WBP_ReEchoStoragePopup`，完整主弹窗与离店确认层都能在该蓝图 Designer 中直接预览和微调；`WBP_ReEchoInventoryShopScreen` 只保留一个全屏子控件，不再混放弹窗内部节点。视觉以 `重开弹窗.svg` 的弹窗构图为参考，使用提供的 `通用弹窗.png` 作为正式底板，并以 `暂停按钮浅.png` / `暂停按钮深.png` 承载按钮正常/悬停按下状态；标题和正文改为现有回响存储内容。同时以 `策划数据源/icon` 的 101 张 PNG 为最终校对源：复核活动卡牌，规范化并导入 48 张已交付配件（45 个当前可售、3 个禁用预留）、8 张商店属性 icon、4 张元素 icon 与 4 张角色 icon，修复“连射移速枪机”等条目落回空槽占位图的问题。

玩家点击已拥有的 `G_3_02「时空锚点」` 卡牌槽后，可在正式弹窗中查看当前唯一时间锚点与本场待处理回响，并执行关闭、将本场设为时间锚点（已有锚点时直接替换）或跳过。弹窗不再展示容量、三条存储槽、逐槽替换/选择或取消替换。未处理待存回响时尝试离店的确认层继续复用同一套正式弹窗与按钮视觉。进入商店不自动打开主弹窗，原有开启时机不变。已不可购买的 `SHOP_REPLAY_UNLOCK「指定回放解锁」` 及其多选状态/API/测试和两套无引用旧 C++ 面板一并删除。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI`、`AREA-UI`、`AREA-Tests`。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 已加入 `Writes`；前者关闭前审阅 Runtime Module 事实，后者同步回响弹窗从动态白盒转为 WBP 作者ing 的职责边界与验证入口。
- 设计意图：WBP 管理布局、尺寸、正式纹理、按钮状态和可读性；C++ 只投影真实状态、更新文字/显隐/启用状态并广播既有类型化事件。避免再次由 C++ 固化一套不可所见即所得的视觉。
- 权威状态与依赖：Run 以 `CurrentBuild.CardState.Runtime.AnchorRecordingId` 作为唯一时间锚点权威；`FReEchoEchoStorageSummary` 只投影一个有效锚点，不再暴露 `SpecificReplayLimit` / `SelectedReplayIds`。`UReEchoInventoryShopWidget` 优先复用 WBP 中的稳定命名控件；缺少完整作者ing 绑定时才构建最低可用 fallback。
- 决策记录：
  - 不把整张 1920×1080 SVG/截图导入成运行时贴图；SVG 只提供位置与构图参考，实际运行时使用三张透明正式切图，避免把 HUD、背景和示例文字烘死。
  - 主弹窗和离店确认层作者ing在独立 `WBP_ReEchoStoragePopup` 中；它只是商店 Widget 的视觉子控件，不拥有玩法状态。父 `UReEchoInventoryShopWidget` 继续持有 Delegate 和生命周期，并按稳定名称递归绑定子蓝图控件。
  - 内容区只保留当前时间锚点、本场待处理回响与两个主操作；文字内容来自现有真实数据，不把示例 Encounter、角色或武器写进 WBP。
- 相关文档同步范围：关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `shared/CODEBASE_MAP/README.md`；预计拓扑和稳定索引不变，无需正文修改。维护 `MOD-ReEchoUI.md` 的商店回响作者ing事实；审阅 `MOD-ReEcho.md` 的代码位置与测试说明。
- 关闭前逐项填写审阅结果：已完成；运行时单锚点职责、独立 authored 弹窗职责、架构总览与稳定索引均已同步或审阅。

## 锁定验收

- [x] 完整弹窗已抽为独立 `WBP_ReEchoStoragePopup`；商店 WBP 只包含一个 `EchoStoragePopupWidget` 子控件，打开独立蓝图即可所见即所得地调整主弹窗和离店确认层，运行时不重写其几何。
- [x] 弹窗使用 `通用弹窗` 正式底板；交互按钮使用浅/深正式按钮状态，透明点击区与可见按钮一致，图片不拉伸变形。
- [x] 标题为“回响存储”，正文只保留当前时间锚点、本场待处理回响和直接替换说明；旧容量、三槽、逐槽替换/选择控件不存在。
- [x] `G_3_02` 点击开启、关闭弹窗、设为/替换时间锚点、跳过和未决离店门禁语义均由窄命令承载。
- [x] `SHOP_REPLAY_UNLOCK`、多选回放运行时状态/API/购买分支/测试及无资产引用的旧回响管理 C++ Widget 已清理；旧存档字段仅作读取兼容。
- [x] 无完整 WBP 绑定时仍有最低可用 fallback，且不会因单一可选节点缺失覆盖完整 authored 商店根。
- [x] 聚焦 UI 自动化、资产审计、Blueprint 编译、Editor FullRebuild、项目校验、LFS 检查及 `git diff --check` 通过。
- [x] 用户已完成当前弹窗候选的 Designer 微调并要求发布；当前可见构图、正式称呼与交互按该人工结论验收。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。
- [x] 64 张当前可选卡牌、48 张已交付武器配件、8 张商店属性、4 张元素和 4 张角色 icon 均有规范化 SourceArt 与运行时纹理；45 个当前可售配件只使用精确交付映射，`连射移速枪机` 不再显示空槽占位图。

## Step 0 门禁

- 基线分支/提交：`origin/main@050bffdfc7a37c4e047a02b719fb168bc6c30518`；该基线相对初始审计新增/更新 Plan 142、144、145、146 文档，均未修改商店 WBP/C++。Plan 146 的数据资产迁移为 `Exclusive` 大任务，但本任务不触碰其数据 Writes，只稳定读取既有 `G_3_02` 和回响摘要契约。
- 引擎/构建可用性：Git LFS `--check` 已通过；计划发布前确认没有 `UnrealEditor` / `UnrealEditor-Cmd` 进程。实现与最终发布仍执行准确候选 FullRebuild。
- 现有聚焦测试结果：基线 `ReEcho.UI.Shop.AuthoredLayoutHosts` 已在仓库中覆盖正式商店、`G_3_02` 点击入口及现有动态回响层；实现后扩展为正式弹窗作者ing和样式断言。
- 共享契约 / 难合并资源风险：`WBP_ReEchoInventoryShopScreen.uasset` 是二进制共享热点，无法文本合并；开始实现及发布前都必须 fetch 并审计该资产和商店 Widget 的外部变化。
- 基线损坏时的停止条件：LFS 未还原、Blueprint 编译失败、正式切图无法导入/透明通道异常、WBP 绑定不能保留 fallback，或远端出现需要产品取舍的同弹窗修改时停止越界部分并报告。

## 实现提纲

1. 将 SVG 参考与三张正式 PNG 归档到窄范围 SourceArt，通过幂等 Unreal Python 脚本导入三张 UI 纹理并校验尺寸、透明度和 TextureGroup。
2. 在独立 `WBP_ReEchoStoragePopup` 的响应式 1920×1080 设计面上作者ing主弹窗和离店确认层，建立稳定命名的底板、标题、当前锚点/本场回响文字与两个主按钮，按钮四态使用浅/深纹理且不保留历史内容 Padding。
3. 将 `UReEchoInventoryShopWidget` 改为优先绑定/复用这些 authored 控件，只更新真实文本、显隐、启用与事件；保留完整性门控的动态 fallback，且不覆盖 authored 几何。
4. 扩展商店 UI 自动化与 Python 资产审计，覆盖作者ing层级、纹理绑定、按钮状态、初始隐藏、`G_3_02` 点击开启及既有回响事件。
5. 更新模块文档和执行记录，完成格式化、Blueprint 编译、聚焦自动化、FullRebuild、静态/LFS 门禁，再交付 PIE 人工验收。
6. 以 `策划数据源/icon` 为最终视觉交付，对照 `cards.csv`、`parts.csv` 和现有 UI 契约建立精确清单：归一化 64 张活动卡牌、48 张已交付配件、8 张属性、4 张元素和 4 张角色 icon；全量重导入并用 Python 审计。3 个禁用配件只导入资产，不改变玩法 Enabled/ShopEnabled。
7. 从用户当前已微调的商店 WBP 原样克隆 `EchoPanelScale` 与 `CloseConfirmWidget` 子树到独立 `WBP_ReEchoStoragePopup`，然后以一个全屏子 Widget 替换商店内嵌节点；C++ 递归绑定独立子蓝图中的稳定控件名，fallback 保持不变。
8. 按现行 G_3_02 卡牌语义收敛回响选择：删除弹窗三槽、容量和逐槽按钮；Run 只持有一条时间锚点记录，本场记录直接设为或替换锚点。移除不可购买的 `SHOP_REPLAY_UNLOCK`、多选回放能力和无引用旧 UI 类，保留旧 SaveGame 字段的最小反序列化兼容。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| LFS | `python scripts/setup_lfs.py --check`、`git lfs status`、`git lfs fsck` | 基线及候选资产均为真实文件，对象健康 |
| 纹理导入 | `scripts/ue/Run-EditorPythonLocked.ps1 -ScriptPath scripts/ue/import_plan147_echo_storage_popup.py` | 三张正式纹理存在、透明通道/TextureGroup/压缩设置正确 |
| WBP 抽取 | `scripts/ue/Run-EditorPythonLocked.ps1 -ScriptPath scripts/ue/migrate_plan147_echo_storage_popup_widget.py` | 用户现有几何/样式原样迁入独立 WBP，商店只剩一个子控件，两个 Blueprint Compile/Save 成功 |
| 资产审计 | `scripts/ue/Run-EditorPythonLocked.ps1 -ScriptPath scripts/ue/audit_plan147_echo_storage_popup.py` | 层级、几何、正式纹理、按钮状态和 Designer 可调性符合契约 |
| icon 全量审计 | 重导入活动卡牌、Plan110 商店纹理、`scripts/ue/import_weapon_part_icons.py`、`scripts/ue/import_plan147_shared_icon_catalog.py` 后运行 `scripts/ue/audit_plan147_complete_shop_icons.py` | 64 张活动卡牌、48 张交付配件、8 张属性、4 张元素、4 张角色 icon 均有稳定 ID 正式纹理；活动配件无语义 fallback |
| C++ 格式 | 对修改的 `.h/.cpp` 运行仓库 `.clang-format` | 格式化 diff 仅限目标文件 |
| C++ 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 退出码 0，并刷新匹配候选的精选 Editor 包 |
| UI 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.Shop.AuthoredLayoutHosts` | 主/确认弹层、纹理与既有回响交互断言通过 |
| Blueprint | `CompileAllBlueprints` | 目标 WBP 及全项目 Blueprint 编译无错误 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目与差异检查通过 |
| 人工 | PIE：获得 `G_3_02` 后完成战斗并进入战后商店，逐项操作主弹窗及未决离店确认 | 用户确认视觉、可读性、DPI 和交互 |

## 执行记录

### 变化

- 2026-08-28：Plan 147 已以 `fb0538d65a99defab073b857b71c31aa0607a4e6` 单独发布到 `origin/main`，实现工作区已从该准确基线建立。
- 2026-08-28：归档完整 SVG 参考与三张正式 PNG，运行时只导入 `PopupFrame`、`ButtonLight`、`ButtonDark` 三张透明 UI 切图；未导入整屏 SVG。
- 2026-08-28：`WBP_ReEchoInventoryShopScreen` 初版新增 Designer 可见的 `EchoPanelScale > EchoPanel > EchoPopupCanvas` 主弹窗，以及默认折叠、可手动预览的 `CloseConfirmWidget` 确认层；随后整体迁入独立弹窗蓝图并删除废案三槽。
- 2026-08-28：`UReEchoInventoryShopWidget` 改为可选绑定 authored 控件并按稳定名称补齐数组/事件；运行时只更新真实状态，不改写 Canvas 几何。无资产原生 Widget 继续走既有动态 fallback。
- 2026-08-28：用户将完整开普勒 icon 交付并入同一任务；审计确认卡牌 64/64 已覆盖，但可售武器配件仅 10/45 有规范化源图，`P_GUN_MOVESTACK_GUNACTION` 既无稳定 ID 纹理也不在旧行号兼容表中，因而回退为空槽图。Plan 扩展为 45 个可售配件精确映射与全量重导入。
- 2026-08-28：按用户反馈将主弹窗与离店确认层进一步抽成独立 `WBP_ReEchoStoragePopup`；迁移脚本以当前资产为源复制 Designer 几何和样式，不用旧默认值覆盖用户微调，旧 author 脚本在独立资产存在后主动拒绝重建。
- 2026-08-28：将工作区 `策划数据源/icon` 作为最终 icon 校对源。101 张中 81 张已与 SourceArt 像素一致；补入 1 张变化的活动卡牌、8 张属性、4 张元素、4 张角色和 3 张禁用预留配件，并将配件清单从 45 个活动项扩展为全部 48 个精确交付项。该目录缺少另外 27 张现役卡牌，但这些卡牌已由此前完整交付保存在规范化 SourceArt 中，继续纳入 64 张活动卡牌运行时审计。
- 2026-08-28：迁移完成后，`WBP_ReEchoInventoryShopScreen` 只保留一个全屏 `EchoStoragePopupWidget`；主弹窗和确认层的现有几何、样式与用户微调位于独立 `WBP_ReEchoStoragePopup`，运行时递归绑定其稳定控件名。
- 2026-08-28：重导全部 64 张活动卡牌并统一 UI 纹理属性；导入 48 张精确配件纹理、8 张商店属性纹理及 8 张共享角色/元素纹理。3 个禁用预留配件仅有资产，未修改 `Enabled` / `ShopEnabled`。
- 2026-08-28：新增 `docs/UI_ICON_REPLACEMENT_GUIDE.md`，向策划提供单张与整批 icon 替换、稳定 ID 查找、重新导入、日志审计、人工检查及需转交程序的边界。
- 2026-08-28：按策划现行卡牌语义将回响回放收敛为 `G_3_02「时空锚点」` 的单一锚点；不再保留任何运行时存储容量或三格展示。本场回响可直接设为锚点，已有锚点时原子替换。
- 2026-08-28：清理已废弃的 `SHOP_REPLAY_UNLOCK「指定回放解锁」` 商品、购买结果、容量/多选运行时 API 与状态、对应旧测试，以及无资产引用的 `UReEchoEchoManagementWidget` / `UReEchoStoredEchoEntryWidget` 两套 C++ UI。旧存档中的 `SelectedReplayIds` / `SpecificReplayLimit` 仅保留字段布局用于反序列化兼容，新存档固定写空/零。
- 2026-08-28：用户手测发现此前“单一锚点”仍保留了废案三槽外观。重新以 `cards.csv` 中 G_3_02 的正式语义为准：Run 由三条 Stored Echo 收敛为一条锚点记录；`WBP_ReEchoStoragePopup` 已删除容量、三条槽位、逐槽替换/选择和取消替换，只显示当前锚点与本场回响。
- 2026-08-28：按用户反馈将锚点与本场回响摘要从内部 `CharacterId` / `WeaponId` 改为正式 CSV `DisplayName`，并把“遭遇 #N”改为“第 N 关”。旧回响可能保存武器类型 ID，显示层在具体武器查找失败时继续查询 `weapon_types.csv`，因此 `J_HEART | LongSword` 正式显示为“勇者 | 长剑”；无法识别时只显示“未知角色/未知武器”，不泄露内部 ID。本轮未写入 WBP，保留用户刚完成的 Designer 微调。

### 证据

- 规划基线：`origin/main@050bffdfc7a37c4e047a02b719fb168bc6c30518`；已审计传入为 Plan 142 扩展及 Plan 144/145/146 文档，无物理冲突。Plan 145 存档语义与 Plan 146 数据权威仅作为 Stable Read 耦合关注，不进入本任务实现范围。
- 规划前 `python scripts/setup_lfs.py --check`：通过。
- 正式纹理导入：`[Plan147EchoImport] imported 3 formal UI textures`。
- WBP 作者ing：`[Plan147EchoAuthor] authored formal echo-storage modal surfaces`；目标资产 Compile/Save 成功。
- WBP 抽取：`[Plan147EchoWidgetMigration] PASS standalone WYSIWYG popup created and shop reduced to one child host`。
- 单锚点迁移：`[Plan147SingleTimeAnchorPopup] PASS retired three-slot UI removed`。
- 资产审计：`[Plan147EchoAudit] PASS standalone single-time-anchor modal, one-child shop host, formal slices, and no retired three-slot controls`。
- icon 审计：`[Plan147ShopIconAudit] PASS cards=64 active_shop_parts=45 delivered_parts=48 shop_stats=8 shared=8 raw_delivery=101`；101 张原始交付均有逐文件哈希命中的规范化 SourceArt。
- `CompileAllBlueprints`：`0 errors / 0 warnings / 0 blueprints failed to load`。
- `ReEcho.UI.Shop` 聚焦自动化：`AuthoredLayoutHosts`、`CardPackChoicePresentation`、`LogicBlocks`、`RuneBackpackFiltersWeaponType` 全部 `Success`。
- `ReEcho.Run.Echo`：6/6 `Success`；覆盖普通 Latest 回退、单时间锚点替换、锚点存档恢复、待处理生命周期、旧存档折叠迁移与记录负载不变。
- `ReEcho.Shop.EchoSelection`：3/3 `Success`；其中 `DeprecatedReplayUnlockRemoved` 证明商品目录不再暴露旧商品，旧 ID 购买失败且不扣时间碎片。
- `ReEcho.Run.SaveSnapshot`：1/1 `Success`；单一锚点可随运行存档恢复。
- 最终 `scripts/ue/Build-Editor.ps1 -Configuration Development -FullRebuild`：105/105 actions 成功，预构建清单 build id `55116800`、source hash `9c8f26af130b`。
- 最终 `CompileAllBlueprints`：`0 errors / 0 warnings / 0 blueprints failed to load`；命令行仅报告 4 条既有启动警告。
- `python scripts/validate_project.py`、`python scripts/setup_lfs.py --check`、`git lfs fsck`、`git diff --check`：通过。

### 剩余风险

- `WBP_ReEchoInventoryShopScreen.uasset` 为二进制共享热点；发布时必须保留最新主线的双武器商店布局，并在最终组合候选上重新挂接独立弹窗、重跑 Blueprint/自动化/FullRebuild 门禁。

### 人工验收结果/请求

- `Passed`：用户完成弹窗微调并要求将当前候选发布到远端主分支；正式称呼已改为 CSV 展示名，不再显示内部角色/武器 ID。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 已同步 Plan147 authored 弹窗、C++ 状态边界和验证入口。
- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已同步单条时间锚点、旧存档折叠兼容和废弃商品清理后的运行时事实。
- `shared/CODEBASE_MAP/ARCHITECTURE.md` 已将商店职责更新为“回响存储/单一时间锚点”；`shared/CODEBASE_MAP/README.md` 已审阅，未新增 Runtime Module 或稳定索引。
