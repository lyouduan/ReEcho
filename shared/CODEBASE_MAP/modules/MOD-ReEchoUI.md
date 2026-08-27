# `MOD-ReEchoUI`：UI 文档型逻辑模块

## 模块状态

- 当前状态：文档型逻辑模块入口；**不是** `ReEcho.uproject` 中的 Runtime Module。
- Runtime Module：无；当前代码仍属于 `MOD-ReEcho`。
- Build 文件：无；当前构建规则仍位于 `Source/ReEcho/ReEcho.Build.cs`。
- 主要目录：`Source/ReEcho/Public/UI/`、`Source/ReEcho/Private/UI/`、`Content/ReEcho/UI/`、`Content/ReEcho/Textures/UI/`、`Content/SourceArt/UI/`。
- UI 架构设计权威：[ReEcho UI 修改指导](../../../Design/UI/ReEcho_UI修改指导.md)。
- 相关 Plan：Plan29、Plan34、Plan45、Plan51、Plan70、Plan93、Plan99、Plan102、Plan116、Plan118。

## 存在原因

为 UI 工作提供稳定的 `MOD-ReEchoUI` 阅读入口，把页面框架、Widget 代码和 WBP 设计规范路由到同一处。该标识只用于文档组织，不提前声明独立模块、源码迁移或新的依赖拓扑。

## 当前职责与边界

- `UReEchoUIManagerSubsystem` 管理 Screen Class、实例和 Viewport 层级。
- `UReEchoUIFlowCoordinatorSubsystem` 管理页面开关、焦点、输入模式和暂停策略，并在屏幕创建后为已有 `UButton` 统一绑定 `UI.Hover` / 基础 `UI.Confirm` 与 `1.05` 倍中心悬停缩放；父级若是只有一个 Button 且有视觉兄弟的 `Overlay`，即使 Button 已有 Content 也优先缩放该完整根，否则缩放 Button，移出后恢复作者ing Scale/Pivot。设置页的 `ComboBox` 选择与 `Slider` 交互完成由 `UReEchoSettingsWidget` 补发一次基础 `UI.Confirm`。所有注册页面的创建/复用/关闭/聚焦结果与按钮点击统一写入 Shipping 可用的 `Saved/Logs/UIInteractionAudit.log`；`UReEchoIndexedButton` 自审计以覆盖页面打开后才生成的动态条目，卡组入口另记录命令接收、拒绝原因、打开结果和候选 ID。
- 交付版 Settings 保持“总音量 / 背景音乐 / 音效音量”三滑条；第三条是非音乐聚合控制，同时预览并保存 `Ambience`、`CombatSfx`、`UiSfx`，因此 `Ambience_Rain` 等环境循环不需要额外第四条滑条。
- 通关后商店的回响存储控件由 `UReEchoInventoryShopWidget` 动态生成，使用底部紧凑缩放托盘承载，避免遮挡商店/装配室主体；存储、跳过、替换与指定回放事件语义保持不变。
- `WBP_ReEchoSettings` 与 `WBP_ReEchoRestart` 的作者ing Root 是正常表现权威；原生 `BuildWidgetTree()` 只在完全没有 Root 时建立最低可用 fallback，不得因单一可选绑定缺失而覆盖整页。Settings 的运行时 ComboBox/Slider 按稳定名称幂等复用。
- `WBP_ReEchoSettings` 的三个分类内容容器、文字字体/颜色与 Slot 布局均由 WBP 作者ing；C++ 只切换容器并绑定设置值/交互。运行时生成的下拉选项从 WBP `GraphicsValue0` 读取字体、颜色和渲染位移，不另设一套程序样式。
- `WBP_ReEchoRestart` 复用既有 Pause 层承载普通暂停、退出到主菜单确认、退出游戏确认和 Death/Victory 结算状态，不新增独立 Pause WBP；交付切图只负责表现，透明真实按钮继续发出继续、设置、保存/不保存退出、返回等类型化 Delegate，目标关卡切换和程序退出由 `AReEchoGameMode` 执行。
- 商店/背包页允许 `WBP_ReEchoRestart` 以更高 Pause 层覆盖：按 `P` 不关闭商店，不触发战后推进或回响存储门禁；关闭 Pause 后必须重新聚焦商店并继续保持世界暂停与菜单能力阻挡。
- WBP/UMG 管理布局、尺寸、样式、动画和焦点表现。
- C++ Widget 管理只读展示状态、类型化绑定、事件转发和页面生命周期。
- `UReEchoEncounterTransitionWidget` 是 ZOrder 10000 的非交互视口最上层 Screen：普通计时关卡在 HUD 显示 03→01 时只消费 GameMode 投影的规范化强度绘制屏幕空间叠色；权威时间到 00 后才通过 FileMediaSource/MediaTexture/UI Material 从第 0 帧播放 `Content/Movies/EncounterTransition/EncounterTransitionAlpha.mov`（Hap Alpha、RGBA、121 帧、40 FPS）。视频非循环并保留原 PNG Alpha，以 Fill 等比居中裁切；到达末帧后覆盖层 0.4 秒淡出，露出 TraitChoice 或 Shop。Widget 只报告媒体完成/失败，不生成卡牌、不推进 Run、不拥有 Encounter 时间。
- Player HUD 显式接收当前玩家的 Combatant 与 CombatEvents：`OnHurt` 仅触发瞬时受击红光，生命变化仅更新可配置的低血量底色。`UReEchoPlayerScreenFeedbackWidget` 独占合成、钳制、重触发和死亡清理等表现状态；`WBP_ReEchoPlayerScreenFeedback` 的 Class Defaults 是阈值、强度、曲线指数、呼吸和材质参数的 Editor 调参表面。HUD 硬引用并构造该 WBP Class，Widget 硬引用 `/Game/ReEcho/Materials/UI/M_UI_PlayerHurtVignette`，保证 cook 收集；材质缺失时使用不影响玩法的原生左右边缘 fallback。
- Plan93/102 战斗常驻 HUD 以两个 WBP 为视觉权威：Player HUD 用 `PlayerHealthFill` 映射真实生命比例并显示 Run 的只读 TimeShards；免费选卡暂停期的单槽刷新扣费成功后，GameMode 立即更新该 HUD 投影，不依赖暂停中不运行的普通 Tick；Encounter HUD 显示 `第 N 关`、零补齐 `MM:SS`，并把现有真实 `UReEchoMinimapCanvasWidget` 包进交付回响框。倒计时不保留 `ArtTimeReadout` 黑色半透明底块；普通关 `ArtClockNeedle` 的 WBP Pivot 位于源图顶部轴心，C++ 按 Encounter Director 剩余/总时长把它从左经下半圆逆时针转到右。Boss 关保留 `ArtClockFrame` 钟背板，只隐藏倒计时文字和指针；同一中心区域的 `BossHealthPanel` 复用玩家血条底板纹理并使用紫色 Tint，暗紫 Fill 位于不透明底板内部上层，只读 EnemyRoster 中存活 Boss Combatant 的当前/最大生命并从左向右缩放；关卡文字与小地图继续显示。Minimap Canvas 底板透明且不绘制内层竞技场边框，保留轨迹，并用 Player/Echo Presentation Profile 的对应头像绘制实时位置；缺图时才降级为旧色点。参考图底部技能栏已被产品废弃，不进入 WBP 或运行时纹理；Widget 不写 Run、不复制遭遇时钟或 Boss 生命，也不伪造技能状态。
- Plan118 将 Minimap Canvas 的 Echo 轨迹从 2px 纯色折线改为沿同一投影路径确定性盖印经典墨水笔尖。`UReEchoMinimapCanvasWidget` 硬引用 UI 材质以进入 Cook；笔尖尺寸、间距、角度/透明度抖动、Grain 强度和固定六项 `Ink Trail Colors` 调色板均在该控件实例的 `Minimap | Ink Trail` 分类调整。控件按 Echo 顺序为每个调色板项维护独立动态材质实例，把颜色写入 `TrailColor` 向量参数；Slate Tint 仅乘单次盖印 Alpha，因此不依赖 UI Material 的 Vertex Color RGB。缺项回退视图颜色且不染色头像。盖印跨折线段保持间距，并受单 Echo 最大数量约束；材质缺失时才回退旧折线，不改变录制采样、路径数据、头像或坐标投影。
- 世界空间伤害跳字由 `AReEchoDamageNumberActor` 硬引用 `/Game/ReEcho/Fonts/DamageNumbers/F_DamageNumber_MFYuYue_Font`，保证 Cook 收集；对应 OTF 与授权说明归档在 `Content/SourceArt/UI/CombatHud/DamageNumbers/`，只允许用于非商用伤害数字，不得当作全局 UI 字体复用。Enemy Presentation 只读 `FReEchoDamageEvent::ReactionBehaviorId` 选择反应色：Vaporize `#A5CBF3`、Conduct `#EBC02C`、Burn `#E86A12`、Growth `#92C039`、Enhance `#F1B84C`；无反应标识时回退到既有元素色/物理白色，不重新推导反应。跳字数值读取同一事件中生命钳制前、已经过伤害规则修正的 `RawDamage`，而不是实际扣血 `AppliedDamage`；因此低血击杀仍显示本次原本应造成的完整伤害，生命和死亡结算不变。运行时统一生成 `/Game/ReEcho/UI/CombatHud/BP_ReEchoDamageNumber`；其 `Damage Number|Animation` Class Defaults 是持续时间、上漂速度和起止缩放的美术调参入口，缺失时才回退原生 Actor 默认值。跳字生命周期内保持不透明，只通过缩放表现变化，到期直接销毁。Actor 继续使用专用 `M_DamageNumberTextOpacity` 保留 UE 默认文字材质的距离场字形重建；`scripts/ue/author_damage_number_material.py` 可幂等重建该材质并以真实 SM5 编译错误为失败，`scripts/ue/author_damage_number_blueprint.py` 负责创建并验证调参蓝图。该动态加载蓝图目录必须由 Packaging AlwaysCook 收集。
- 世界空间元素反应字由 `AReEchoElementReactionPopupActor` 消费既有 `FReEchoElementReactionResolvedEvent`：Burn/Vaporize/Growth/Conduct/Enhance 分别映射灼烧/蒸发/生长/导电/强化透明图，只在权威 `PrimaryTarget` 上方生成一次，不按 Growth 影响列表或 Conduct 连锁节点重复。`BP_ReEchoElementReactionPopup` 的 `Element Reaction Popup|Animation/Layout` Class Defaults 调整持续时间、世界尺寸、上浮高度、渐隐曲线和缩放；独立半透明材质动态接收 `ReactionTexture` 与 `Opacity`。纹理、材质或蓝图缺失只跳过该次表现或回退原生 Actor，不改变元素反应结算。
- 商店装配室的武器面板、时钟、3 个符文槽和 12 个卡牌槽以 `WBP_ReEchoInventoryShopScreen > Overlay_0 > DesignerLoadoutCanvas` 为位置权威；这些 authored 控件都是 Canvas 直接子项，可在 UMG Designer 中修改 Position/Size。`UReEchoInventoryShopWidget` 在武器面板的 authored 矩形上叠加当前武器图与透明点击入口，位置跟随面板而不是另立坐标；武器贴图 Brush 使用源纹理尺寸，按钮 Content Slot 填充 authored 矩形，再由 ScaleBox 等比放大，禁止退回默认 32px 中央小图。点击后用独立浮层列出 Run 投影的全部已拥有武器，当前项禁用，其他项只广播装备请求。武器与符文背包统一挂在响应式设计面的根级 `BackpackPopupLayer`，其 ZOrder 高于商店和回响弹层；符文背包复用武器背包的边框、滚动区、图标尺寸和文字行对齐。符文槽继续填充纹理、置灰状态与 Tooltip；构筑卡购买成功后把真实卡牌图标立即填入卡牌槽，但不重摇当前商店卡组候选。
- 已拥有卡牌的 Tooltip 保留原“名称 + 策划描述”面板；当 Run 投影的 `OutcomeText` 非空时，同一 Tooltip 根在其正下方再生成独立描边的“实际效果”面板。UI 不按卡牌 ID、中文描述或当前属性反推玩法结果，空结果不生成第二面板。
- 商店/背包页以 `1920×1080` 为作者设计面：`BackgroundImage` 保持直接铺满实际视口，其余根控件及运行时商店/回响弹层由 `ResponsiveContentScale > ResponsiveContentSize > ResponsiveContentCanvas` 统一 `ScaleToFit`。低分辨率按比例缩小完整页面，超宽屏只延展背景；运行时迁移必须保留原 Canvas Slot 的锚点、偏移、自动尺寸和 ZOrder。
- `UReEchoInventoryShopWidget` 的无资产 fallback 展示实际折扣价、武器/符文免费与付费剩余刷新次数、刷新禁用和额外卡牌组禁用状态；免费次数优先消费且不占用每关付费次数。主刷新只广播武器/符文刷新命令，Run 成功消费并保存后才更新这三个报价槽，卡组候选和已购状态保持不变。诅咒银行仍由 Run 分开保存非负现金与正债务，商店和玩家 HUD 只读展示二者净值，因此赊账后显示负数但购买判断不读取该展示值。
- 商店逻辑按稳定区块拆分：`ShopLogicScrollBox > ShopLogicPanel` 依次承载 `WeaponPartOfferPanel`（配件购买）、`RunItemOfferPanel`（普通商品）、`ShopControlPanel`（规则/刷新）和 `WeaponLoadoutPanel`（槽位草稿/保存）。现有 WBP 由 C++ 在根 Canvas 上提供有界、显式滚动条的商品视口，战后模式止于底部回响托盘上方；`EchoPanel` 使用独立缩放托盘与显式高 ZOrder。这些名称是后续 WBP 接入的逻辑契约，C++ 不依赖任何美术占位节点。
- 同一商店页展示数据驱动的武器槽组、兼容配件报价和装配草稿；未拥有配件点击后发送购买命令，已拥有配件点击后只编辑草稿，“保存配置”才发送完整 PartId 集合给 Run。关闭页面不提交草稿，已成功购买的配件所有权仍保留。
- UI 只消费只读摘要或事件并发送受控命令，不直接写 Run、Combat、Weapons、Enemies 或存档权威状态。
- 战后角色能力不建立专用 UI 分支：旧 Forge 标题、候选和提交命令已删除；旧存档的 Forge 阶段由 Run 迁移到普通卡牌选择，Widget 只显示正式卡牌候选。
- 构筑三选一卡面只保留正式卡牌插图、名称、说明、图标和选择交互；旧标签条、候选/选择提示、标签文字及纯色/运行时染色占位卡底已从 `WBP_ReEchoTraitCardEntry` 和 C++ 注入契约中删除。`WBP_ReEchoTraitCardEntry` 的默认正文/图片是 Designer 样例，根 `CardRootScaleBox` 等比缩放内部与正式底图一致的 `420×593` `CardRootSizeBox` 设计面；`WBP_ReEchoTraitCardChoice` 三个槽位内各放一个仅供所见即所得预览的样例实例，运行时以真实条目替换并写入数据，不改变槽位几何。Entry 的 `NameText` / `DescriptionText` 位于 `CardDesignerCanvas`，Choice 的 `TitleText` / `ConfirmButtonLabel` 位于根 Canvas，均可在 Designer 中独立拖动；运行时只写内容。底部 `ConfirmButton` 的按钮美术由 Choice WBP 的 Button Style 直接引用 `T_UI_Pause_ButtonLight`，独立标签为 `HitTestInvisible`，C++ 只管理选择索引、启用状态与确认委托；无 WBP fallback 的卡牌按钮底色保持透明。
- 卡牌图标继续按稳定路径 `/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_{CardId}` 动态解析，整个 Cards 纹理目录由 AlwaysCook 收集。Plan128 后当前全部 64 张启用且可投放卡牌均有独立图标，源图保持各自交付原生尺寸；未来新增卡牌缺少独立资源时继续安全回退 `T_UI_Shop_CardIcon`，UI 不据图标存在性改变卡牌资格。
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
| 修改玩家受伤/低血量屏幕反馈 | `ReEchoPlayerHudWidget.*` | `WBP_ReEchoPlayerScreenFeedback`、`M_UI_PlayerHurtVignette`、两个 `scripts/ue/author_plan70_*` 作者ing脚本 |
| 修改战斗常驻 HUD | `WBP_ReEchoPlayerHud`、`WBP_ReEchoEncounterHud` | `ReEchoPlayerHudWidget.*`、`ReEchoEncounterHudWidget.*`、`ReEchoMinimapCanvasWidget.*`、`Content/SourceArt/UI/CombatHud/Plan{93,102}/`、`Content/SourceArt/UI/CombatHud/MinimapInkBrush/` |
| 修改关末倒计时/序列过渡 | `ReEchoEncounterTransitionWidget.*` | `ReEchoGameMode` 结算状态、`Content/Movies/EncounterTransition/EncounterTransitionAlpha.mov`、MediaAssets/HAPMedia |
| 修改商店卡牌规则展示 | `ReEchoInventoryShopWidget.*` | `ReEchoGameMode::RefreshShopPresentation`、`UReEchoRunSubsystem` 商店命令 |
| 修改页面创建、层级和实例 | `ReEchoUIManagerSubsystem.*` | `UI/Framework/ReEchoUIScreenTypes.h` |
| 修改焦点、输入或暂停流程 | `UI/Framework/ReEchoUIFlowCoordinatorSubsystem.*` | `ReEchoGameMode` 类型化端点 |
| 修改暂停/退出确认表现 | `WBP_ReEchoRestart`、`ReEchoRestartWidget.*` | `ReEchoGameMode` 的暂停退出端点 |
| 修改音频设置页 | `WBP_ReEchoSettings` 的绑定契约 | `ReEchoSettingsWidget.*`、`MOD-ReEchoAudio.md` |
| 修改通用按钮反馈 | `UI/Framework/ReEchoButtonVisualFeedback.*` | `ReEchoUIFlowCoordinatorSubsystem.*`、`FReEchoAudioEvents`、具体结果宿主 |

Plan45 的运行时美术消费保持在 WBP 表现层：Start Menu、Settings、Restart、Trait Card、Inventory/Shop、Player/Encounter HUD 和 Stats 页面引用分页纹理目录；原生 Widget 仍拥有状态、Delegate、显隐和生命周期。

## 屏幕生命周期：关卡 travel 重置

`UReEchoUIManagerSubsystem` 是 `UGameInstanceSubsystem`，其 `ActiveScreens` / `ManagedWidgets` 跨 `OpenLevel`（non-seamless 整图重载）保留——GameInstance 在 travel 时不被销毁，而 UWorld / GameMode / PlayerController 会被重建。引擎在 `LoadMap` 前会 `RemoveAllViewportWidgets` 把旧 widget 从视口摘掉，但旧 UObject 仍 `IsValid` 并留在 `ActiveScreens` 中，导致新世界 `SetupArena` 经 `CreateScreen` 命中"已存在则短路返回"分支、跳过 `AddToLayer`（`AddToViewport`），于是重载后 HUD 不显示。

修复把"travel 后屏幕必须重建"变为子系统的不变量：`Initialize` 中绑定 `FCoreUObjectDelegates::PreLoadMap` → `HandlePreLoadMap` → `ResetScreens()`（先 `RemoveFromParent` 再清空两个容器）；`Deinitialize` 中解绑并调用同一 `ResetScreens()`。新世界总走"新建 + `AddToViewport`"分支，旧世界对象尽快不可达、可被干净 GC。`PreLoadMap` 首次进游戏也会触发，但此时容器为空、`ResetScreens` 为 no-op，无副作用。本修复覆盖 Restart / 退出到主菜单 / Continue 所有 `OpenLevel` 路径，不修改 HUD Widget 视觉或 `OpenScreen` / `CloseScreen` 公共契约。

- 相关 Plan：Plan51（Restart 后战斗 HUD 跨关卡屏幕重置）。
- 自动化：`ReEcho.UIManagerSubsystem.ResetOnTravel`（`Source/ReEcho/Private/Tests/ReEchoUIManagerSubsystemTests.cpp`）验证 `ResetScreens` 后 `ActiveScreens` 清空、再次 `OpenScreen` 得全新实例；`ReEchoRestartWidgetTests` 只测 `WBP_ReEchoRestart` 表现，不覆盖子系统级重置。

## 验证

- C++ 变更：`scripts/ue/Build-Editor.cmd -Configuration Development`。
- 按钮根悬停回归：`scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.ButtonVisualFeedback`；全部交互 WBP 的缩放幅度、裁切和页面流程仍需 PIE 人工验收。
- 玩家屏幕反馈回归：`scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.PlayerScreenFeedback`；视觉曲线、超宽屏、暂停层级和死亡切换仍需 PIE 人工验收。
- WBP 变更：在 Unreal Editor 中 Compile/Save，并运行 `CompileAllBlueprints`。
- Plan99 构筑卡牌表现回归：`scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.TraitCard`；验证正式卡图绑定保留、六个废弃标签/占位节点不存在、全部正式文字为可拖动 Canvas 子项、确认按钮使用浅色暂停按钮纹理且文案为“确定”。
- 静态检查：`python scripts/validate_project.py`、`git diff --check`。
- Plan47 商店回归：折扣显示与实际扣款同舍入、免费刷新优先消费且无零价无限刷新、永久代价禁用状态可见；不修改 Plan45 WBP/纹理资产。
- Plan47 配件回归：兼容配件可购买、普通背包与配件所有权分离、三类槽位从 `slot_profiles.csv` 生成、必需 Core 不可留空、保存前后装备效果与存档一致。
- 卡牌商店投影固定为1/2/3级三个卡组入口；入口不显示具体卡牌 icon，底部按钮按状态显示“购买 · 折扣后卡组价 / 继续选择 / 已购”。可购买入口发布 Tier 命令后，GameMode 先调用 Run 的卡组付款事务并立即 `SaveRun`，成功才在 ZOrder 98 的 `BuildChoice` 层打开复用的选择页；已付款待选入口不检查余额或额外购卡门槛，直接继续同一候选页且不重复收费。页内最多三张同级候选，不显示单卡价格，并提供“返回商店”；最终选择调用独立领取事务。商店已付款与战后免费三选一都在每张实际候选下方显示独立刷新按钮、该槽剩余次数和价格，并通过统一 `OnCardSlotRefreshRequested(SlotIndex)` 发送稳定槽位；GameMode 按当前页面调用对应 Run 原子命令后重注入同一层，Widget 不自行抽牌或扣费。成功逐槽刷新只将该稳定槽位映射到当前可见卡牌并重播它的揭示，另外两张卡与页面指针保持已揭示状态。领取/刷新失败恢复原选择；取消仅关闭页面，已付款状态不退款且可继续。
- Plan91 武器背包扩展：Widget 不持有武器所有权，也不把换装伪装成购买；`OnWeaponEquipRequested` 交给 GameMode 调用 Run 事务。成功后整页重取只读投影，使当前武器图、武器名和兼容符文槽同时更新，但独立的武器/符文与卡牌刷新序列都不变，因此报价不重摇。
- Plan91 稳定页中的已拥有武器仍保留原槽身份和价格，但 Widget 必须以 Run 的 `OwnedWeapons` 投影为“已获得”并禁用购买；不得依赖本次页面内的临时点击记录判断所有权。
- Plan91 的任意商品购买成功后由 GameMode 重新注入完整商店只读投影，Widget 不再局部拼接 `EquippedParts`。这保证新购符文立即进入 `OwnedParts` 背包视图，同时依赖 Run 的稳定页缓存保持其余报价不变。
- 武器/符文购买按钮由 GameMode 调用 Run 的 `PurchaseShopItemDetailed`；卡组入口发送 Tier 并分别消费 `PurchaseShopCardPackDetailed` / `ClaimPaidShopCardChoice` 的结构化结果。Widget 只根据只读投影控制购买与继续按钮，不自行修改扣费、付款或所有权状态。
- 人工检查：按 UI 修改指导执行页面导航、焦点、DPI、可读性和交互验收。
