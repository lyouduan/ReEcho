# `MOD-ReEchoUI`：UI 文档型逻辑模块

## 模块状态

- 当前状态：文档型逻辑模块入口；**不是** `ReEcho.uproject` 中的 Runtime Module。
- Runtime Module：无；当前代码仍属于 `MOD-ReEcho`。
- Build 文件：无；当前构建规则仍位于 `Source/ReEcho/ReEcho.Build.cs`。
- 主要目录：`Source/ReEcho/Public/UI/`、`Source/ReEcho/Private/UI/`、`Content/ReEcho/UI/`、`Content/ReEcho/Textures/UI/`、`Content/SourceArt/UI/`。
- UI 架构设计权威：[ReEcho UI 修改指导](../../../Design/UI/ReEcho_UI修改指导.md)。
- 相关 Plan：Plan29、Plan34、Plan45、Plan51。

## 存在原因

为 UI 工作提供稳定的 `MOD-ReEchoUI` 阅读入口，把页面框架、Widget 代码和 WBP 设计规范路由到同一处。该标识只用于文档组织，不提前声明独立模块、源码迁移或新的依赖拓扑。

## 当前职责与边界

- `UReEchoUIManagerSubsystem` 管理 Screen Class、实例和 Viewport 层级。
- `UReEchoUIFlowCoordinatorSubsystem` 管理页面开关、焦点、输入模式和暂停策略，并在屏幕创建后为已有 `UButton` 统一绑定 `UI.Hover` / 基础 `UI.Confirm`；设置页的 `ComboBox` 选择与 `Slider` 交互完成由 `UReEchoSettingsWidget` 补发一次基础 `UI.Confirm`。
- 交付版 Settings 保持“总音量 / 背景音乐 / 音效音量”三滑条；第三条是非音乐聚合控制，同时预览并保存 `Ambience`、`CombatSfx`、`UiSfx`，因此 `Ambience_Rain` 等环境循环不需要额外第四条滑条。
- 通关后商店的回响存储控件由 `UReEchoInventoryShopWidget` 动态生成，使用底部紧凑缩放托盘承载，避免遮挡商店/装配室主体；存储、跳过、替换与指定回放事件语义保持不变。
- `WBP_ReEchoRestart` 复用既有 Pause 层承载普通暂停、退出到主菜单确认、退出游戏确认和结算状态；交付切图只负责表现，透明真实按钮继续发出继续、设置、保存/不保存退出、返回等类型化 Delegate，目标关卡切换和程序退出由 `AReEchoGameMode` 执行。
- WBP/UMG 管理布局、尺寸、样式、动画和焦点表现。
- C++ Widget 管理只读展示状态、类型化绑定、事件转发和页面生命周期。
- 商店装配室的武器面板、时钟、3 个配件槽和 12 个卡牌槽以 `WBP_ReEchoInventoryShopScreen > Overlay_0 > DesignerLoadoutCanvas` 为位置权威；这些控件都是 Canvas 直接子项，可在 UMG Designer 中修改 Position/Size。`UReEchoInventoryShopWidget` 只向槽位填充纹理、置灰状态、Tooltip 和存储卡点击事件，不创建或重置其坐标。
- `UReEchoInventoryShopWidget` 的无资产 fallback 展示实际折扣价、免费刷新余量、刷新禁用和额外卡牌组禁用状态；刷新只广播命令，Run 成功消费并保存后才更新页面和确定性报价顺序。
- 商店逻辑按稳定区块拆分：`ShopLogicScrollBox > ShopLogicPanel` 依次承载 `WeaponPartOfferPanel`（配件购买）、`RunItemOfferPanel`（普通商品）、`ShopControlPanel`（规则/刷新）和 `WeaponLoadoutPanel`（槽位草稿/保存）。现有 WBP 由 C++ 在根 Canvas 上提供有界、显式滚动条的商品视口，战后模式止于底部回响托盘上方；`EchoPanel` 使用独立缩放托盘与显式高 ZOrder。这些名称是后续 WBP 接入的逻辑契约，C++ 不依赖任何美术占位节点。
- 同一商店页展示数据驱动的武器槽组、兼容配件报价和装配草稿；未拥有配件点击后发送购买命令，已拥有配件点击后只编辑草稿，“保存配置”才发送完整 PartId 集合给 Run。关闭页面不提交草稿，已成功购买的配件所有权仍保留。
- UI 只消费只读摘要或事件并发送受控命令，不直接写 Run、Combat、Weapons、Enemies 或存档权威状态。
- 关闭、返回、事务拒绝、购买成功与卡牌选择成功的专用声音由命令结果宿主发布；按钮基础反馈不代替事务结果，也不得让音频失败改变 UI 行为。

完整页面清单、WBP/C++ 分工、绑定控件名称、动态条目规则和人工验收要求，统一以 [ReEcho UI 修改指导](../../../Design/UI/ReEcho_UI修改指导.md) 为准；本文件不复制第二份控件契约。

## 依赖方向

当前 UI 代码仍编译在 `ReEcho` 中，因此全局 Runtime Module 拓扑没有变化。未来若计划创建真实 `ReEchoUI` Runtime Module，必须另立 Plan，先移除 Widget 对主模块内部玩法类型和权威 Subsystem 的直接依赖，再同步 `.uproject`、Build 文件、全局架构、索引和模块验证。

## 代码位置与阅读路线

| 目的 | 首读 | 后续 |
|---|---|---|
| 修改 UI 外观或布局 | `Design/UI/ReEcho_UI修改指导.md` | `Content/ReEcho/UI/WBP_ReEcho*` |
| 接入 UI 美术源图 | `Content/SourceArt/UI/InteractionPlaceholder/README.md` | `Content/ReEcho/Textures/UI/InteractionPlaceholder/`、对应 WBP |
| 修改页面行为或绑定 | 对应 `Source/ReEcho/Public/UI/*Widget.h` | 配对的 `Source/ReEcho/Private/UI/*Widget.cpp` |
| 修改商店卡牌规则展示 | `ReEchoInventoryShopWidget.*` | `ReEchoGameMode::RefreshShopPresentation`、`UReEchoRunSubsystem` 商店命令 |
| 修改页面创建、层级和实例 | `ReEchoUIManagerSubsystem.*` | `UI/Framework/ReEchoUIScreenTypes.h` |
| 修改焦点、输入或暂停流程 | `UI/Framework/ReEchoUIFlowCoordinatorSubsystem.*` | `ReEchoGameMode` 类型化端点 |
| 修改暂停/退出确认表现 | `WBP_ReEchoRestart`、`ReEchoRestartWidget.*` | `ReEchoGameMode` 的暂停退出端点 |
| 修改音频设置页 | `WBP_ReEchoSettings` 的绑定契约 | `ReEchoSettingsWidget.*`、`MOD-ReEchoAudio.md` |
| 修改通用按钮反馈 | `UI/Framework/ReEchoUIFlowCoordinatorSubsystem.*` | `FReEchoAudioEvents`、具体结果宿主 |

Plan45 的运行时美术消费保持在 WBP 表现层：Start Menu、Settings、Restart、Trait Card、Inventory/Shop、Player/Encounter HUD 和 Stats 页面引用分页纹理目录；原生 Widget 仍拥有状态、Delegate、显隐和生命周期。

## 屏幕生命周期：关卡 travel 重置

`UReEchoUIManagerSubsystem` 是 `UGameInstanceSubsystem`，其 `ActiveScreens` / `ManagedWidgets` 跨 `OpenLevel`（non-seamless 整图重载）保留——GameInstance 在 travel 时不被销毁，而 UWorld / GameMode / PlayerController 会被重建。引擎在 `LoadMap` 前会 `RemoveAllViewportWidgets` 把旧 widget 从视口摘掉，但旧 UObject 仍 `IsValid` 并留在 `ActiveScreens` 中，导致新世界 `SetupArena` 经 `CreateScreen` 命中"已存在则短路返回"分支、跳过 `AddToLayer`（`AddToViewport`），于是重载后 HUD 不显示。

修复把"travel 后屏幕必须重建"变为子系统的不变量：`Initialize` 中绑定 `FCoreUObjectDelegates::PreLoadMap` → `HandlePreLoadMap` → `ResetScreens()`（先 `RemoveFromParent` 再清空两个容器）；`Deinitialize` 中解绑并调用同一 `ResetScreens()`。新世界总走"新建 + `AddToViewport`"分支，旧世界对象尽快不可达、可被干净 GC。`PreLoadMap` 首次进游戏也会触发，但此时容器为空、`ResetScreens` 为 no-op，无副作用。本修复覆盖 Restart / 退出到主菜单 / Continue 所有 `OpenLevel` 路径，不修改 HUD Widget 视觉或 `OpenScreen` / `CloseScreen` 公共契约。

- 相关 Plan：Plan51（Restart 后战斗 HUD 跨关卡屏幕重置）。
- 自动化：`ReEcho.UIManagerSubsystem.ResetOnTravel`（`Source/ReEcho/Private/Tests/ReEchoUIManagerSubsystemTests.cpp`）验证 `ResetScreens` 后 `ActiveScreens` 清空、再次 `OpenScreen` 得全新实例；`ReEchoRestartWidgetTests` 只测 `WBP_ReEchoRestart` 表现，不覆盖子系统级重置。

## 验证

- C++ 变更：`scripts/ue/Build-Editor.cmd -Configuration Development`。
- WBP 变更：在 Unreal Editor 中 Compile/Save，并运行 `CompileAllBlueprints`。
- 静态检查：`python scripts/validate_project.py`、`git diff --check`。
- Plan47 商店回归：折扣显示与实际扣款同舍入、免费刷新优先消费且无零价无限刷新、永久代价禁用状态可见；不修改 Plan45 WBP/纹理资产。
- Plan47 配件回归：兼容配件可购买、普通背包与配件所有权分离、三类槽位从 `slot_profiles.csv` 生成、必需 Core 不可留空、保存前后装备效果与存档一致。
- 人工检查：按 UI 修改指导执行页面导航、焦点、DPI、可读性和交互验收。
