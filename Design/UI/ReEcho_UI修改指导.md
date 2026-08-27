# ReEcho UI 修改指导

> 适用范围：Plan 29 全 UI UMG/C++ 混合迁移后的当前工程。本文面向 UI 设计、技术美术和负责 UI 的程序人员。

## 1. 总体原则

ReEcho UI 使用 UMG 与 C++ 混合架构：

- **UMG/WBP 负责**：布局、锚点、安全区、尺寸、间距、字体、颜色、贴图、按钮外观、动画和焦点表现。
- **C++ 负责**：数据读取、稳定 ID、合法性校验、按钮事件、购买/选择结果、存档、暂停、输入模式、页面生命周期和游戏状态切换。
- **数据表负责**：角色、武器、卡牌等生产数据。已迁移的数据从 `Design/Data/ReEchoData.xlsx` 生成到 `Content/Data/`，不要在 Widget 中复制一份数值。

修改 UI 外观时，优先只编辑 `/Game/ReEcho/UI/WBP_ReEcho*`。不要为了改尺寸、颜色或排版去修改 C++。只有需要新增数据绑定、事件或动态行为时才修改对应原生 Widget 类。

## 2. 当前 UI 清单

| UI | WBP 资产 | C++ 原生父类 | 运行层 | 当前架构与用途 |
|---|---|---|---|---|
| 玩家 HUD | `WBP_ReEchoPlayerHud` | `UReEchoPlayerHudWidget` | `PlayerHud` | 爱心、生命条/数值、时间碎片余额；生命变化由事件驱动，碎片只读 Run 权威 |
| 遭遇 HUD | `WBP_ReEchoEncounterHud` | `UReEchoEncounterHudWidget` | `GameplayHud` | `第 N 关`、`MM:SS`、右到左计时指针和真实小地图；最后 5 秒警示色由 C++ 状态驱动 |
| 敌人血条 | `WBP_ReEchoEnemyHealthBar` | `UReEchoHealthBarWidget` | 世界空间 Widget | 敌人头顶血条；生命变化由事件驱动 |
| 开始菜单 | `WBP_ReEchoStartMenu` | `UReEchoStartMenuWidget` | `Start` | 新游戏、继续、设置；继续按钮可用性由存档状态决定 |
| 初始配装 | `WBP_ReEchoLoadoutSelection` | `UReEchoLoadoutSelectionWidget` | `Loadout` | 角色/武器两阶段所见即所得条目、解释框、箭头和确认按钮 |
| 配装条目 | `WBP_ReEchoLoadoutEntry` | `UReEchoLoadoutEntryWidget` | 父页面内部 | 角色/武器共用的动态条目；UMG 控制切图、名称和条目尺寸 |
| 配装悬停说明 | `WBP_ReEchoLoadoutTooltip` | `UReEchoLoadoutTooltipWidget` | `WBP_ReEchoLoadoutEntry.SelectButton.ToolTip` | 鼠标 Tooltip 的宽度、边框、内边距、标题和说明文字 |
| 设置 | `WBP_ReEchoSettings` | `UReEchoSettingsWidget` | `Settings` | 图形、声音、控制分类和返回操作 |
| 暂停/死亡/胜利 | `WBP_ReEchoRestart` | `UReEchoRestartWidget` | `Pause` | 同一页面按明确模式显示暂停、死亡、胜利和退出确认状态 |
| 抽卡页面 | `WBP_ReEchoTraitCardChoice` | `UReEchoTraitCardChoiceWidget` | `BuildChoice` | 三个卡槽、标题、确认按钮和揭示动画 |
| 抽卡条目 | `WBP_ReEchoTraitCardEntry` | `UReEchoTraitCardEntryWidget` | 抽卡页面内部 | 单张卡的按钮、插图、标题、描述和图标 |
| 背包/商城 | `WBP_ReEchoInventoryShopScreen` | `UReEchoInventoryShopWidget` | `Screen` | 背包与商城共用页面；报价按钮由目录动态生成 |
| 属性页面 | `WBP_ReEchoStatsScreen` | `UReEchoStatsWidget` | `Screen` | 玩家和回响的两列属性展示 |
| 天气 | 无独立 WBP | `UReEchoWeatherWidget` | `Weather` | 明确保留的 C++ `NativePaint` 混合绘制面，负责雨和迷雾 |
| 伤害数字 | 无独立 WBP | `AReEchoDamageNumberActor` | 世界空间 | 使用 `UTextRenderComponent` 的世界空间反馈，不是菜单 UMG |
| 元素反应字 | `BP_ReEchoElementReactionPopup` | `AReEchoElementReactionPopupActor` | 世界空间 | 每次权威反应在主目标上方显示一次对应透明图片；参数见 [元素反应字调参指南](ReEcho_元素反应字调参指南.md) |

所有 WBP 位于：`Content/ReEcho/UI/`。

Plan45 的交互占位源图位于 `Content/SourceArt/UI/InteractionPlaceholder/`，运行时只使用按页面评审后导入的 `/Game/ReEcho/Textures/UI/InteractionPlaceholder/**`。`References` 中的整屏效果图只用于布局对照；效果图里出现但当前产品契约不存在的操作不能据此新增流程。`Fonts/PendingLicense` 中的 TTF 在授权确认前只留在本地隔离目录，不得导入、引用或提交分发。

## 3. UI 层级和输入

UI Framework 现在分为两个职责明确的 GameInstance Subsystem：

- `UReEchoUIManagerSubsystem`：Screen Class 注册表、类型化 Screen 实例、Viewport 层级和实例清理。
- `UReEchoUIFlowCoordinatorSubsystem`：页面打开/关闭、焦点、菜单输入、暂停和暂停状态下的安全页面替换。

GameMode 只保留玩法请求和页面 Delegate 处理，不再保存 WBP 资产路径，也不直接执行 `CreateWidget`、`AddToViewport` 或 `RemoveFromParent`。

`EReEchoUIScreen` 是页面生命周期的稳定标识；`EReEchoUILayer` 是显示层的稳定标识。当前层级从低到高为：

| 层 | ZOrder | 用途 |
|---|---:|---|
| `Weather` | 5 | 天气覆盖层 |
| `GameplayHud` | 10 | 遭遇信息 |
| `PlayerHud` | 12 | 玩家状态 |
| `BuildChoice` | 90 | 抽卡/构筑选择 |
| `Screen` | 95 | 商城、背包、属性页 |
| `Pause` | 100 | 暂停、死亡、胜利 |
| `Start` | 200 | 开始菜单 |
| `Loadout` | 210 | 初始配装 |
| `Settings` | 220 | 设置页 |

不要在 WBP 的蓝图脚本或 Gameplay 类中自行调用 `CreateWidget`、`AddToViewport`、`RemoveFromParent`，也不要另写数字 ZOrder。新增页面应注册 `EReEchoUIScreen` 和对应 Class，由 Flow Coordinator 统一打开和关闭。

## 4. WBP 绑定契约

WBP 的原生父类通过控件名称绑定 C++。修改层级和外观时可以移动控件，但不能随意重命名或删除绑定控件。

### 4.1 页面级可选绑定

以下名称使用 `BindWidgetOptional`。正常 WBP 路径应保留；缺失时部分原生类会进入 C++ fallback，但该 fallback 只用于资产失效保护，不是正式设计路径。

| WBP | 必须保留的正常路径控件名 |
|---|---|
| `WBP_ReEchoPlayerHud` | `PlayerPortrait`, `PlayerHealthProgress`, `PlayerHealthFill`, `PlayerHealthText`, `TimeShardText` |
| `WBP_ReEchoEncounterHud` | `EncounterText`, `CountdownText` |
| `WBP_ReEchoEnemyHealthBar` | `ProgressBar` |
| `WBP_ReEchoStartMenu` | `StatusText`, `ContinueButton`, `NewGameButton`, `GameSettingsButton`, `QuitButton` |
| `WBP_ReEchoLoadoutSelection` | `StatusText`, `TitleText`, `DescriptionText`, `DescriptionTextScale`, `ConfirmButtonLabel`, `StageSwitcher`, `CharacterStagePanel`, `WeaponStagePanel`, `DescriptionPanel`, `CharacterRow`, `WeaponRow`, `ConfirmButton` |
| `WBP_ReEchoSettings` | `GraphicsSettingsButton`, `AudioSettingsButton`, `ControlsSettingsButton`, `RestoreDefaultsButton`, `ApplyAndReturnButton`, `AudioPanel`, `GraphicsPanel`, `ControlsPanel`, `MasterVolumeSlider`, `MusicVolumeSlider`, `AmbienceVolumeSlider`, `CombatSfxVolumeSlider`, `UiSfxVolumeSlider`, `MasterMuteCheckBox`, `MusicMuteCheckBox`, `AmbienceMuteCheckBox`, `CombatSfxMuteCheckBox`, `UiSfxMuteCheckBox`, `DiagnosticToneCheckBox` |

`WBP_ReEchoSettings` 的声音页可见布局以 `AudioDesignerCanvas` 为位置权威。`MasterVolumeVisualOverlay`、`MusicVolumeVisualOverlay`、`CombatVolumeVisualOverlay` 分别代表完整滑条逻辑根；移动它们会同时移动轨道、填充和真实把手。各 `*Label`、`*Percent`、`*MuteCheckBox` 及 `AudioOutputField` 都是该 Canvas 的直接子项，可在 Designer 中独立拖动。运行时生成的下拉交互层会复制 `AudioOutputField` 的 Canvas 位置和尺寸，不应在 C++ 中另写坐标。
| `WBP_ReEchoRestart` | `TitleText`, `MessageText`, `ResumeButton`, `RestartButton`, `QuitButton`, `SettingsButton`, `QuitButtonText`；暂停样板另提供可选 `RootPanel`, `PauseSettingsButton`, `ResumeButtonLabel`, `RestartButtonLabel`, `ArtPauseDimmer`, `ArtPausePrimaryButton`, `ArtPauseSecondaryButton`, `ArtPauseTertiaryButton`, `ArtPauseSettings`；正式胜利样板提供可选 `VictoryCanvas`, `VictoryEncounterValue`, `VictoryTimeShardsValue`, `VictoryTraitCountValue`, `VictoryContinueButton`；正式失败样板提供可选 `DefeatCanvas`, `DefeatEncounterValue`, `DefeatTimeShardsValue`, `DefeatTraitCountValue`, `DefeatRestartButton`, `DefeatMainMenuButton` |
| `WBP_ReEchoTraitCardChoice` | `TraitCardContainer`, `TraitCardSlot0`, `TraitCardSlot1`, `TraitCardSlot2`, `TitleText`, `ConfirmButton`, `ConfirmButtonLabel` |
| `WBP_ReEchoInventoryShopScreen` | `BackgroundImage`, `InventoryPanel`, `ShopPanel`, `CurrencyText`, `InventoryText`, `CloseButton`, `OfferContainer` |
| `WBP_ReEchoStatsScreen` | `BackgroundImage`, `PlayerStatsText`, `EchoStatsText`, `CloseButton` |

### 4.2 动态条目必需绑定

以下条目使用严格的 `BindWidget`。缺失、类型不匹配或重命名会导致 Blueprint 编译错误，或在错误资产版本下造成运行时问题。

| WBP | 必需控件名 |
|---|---|
| `WBP_ReEchoLoadoutEntry` | `SelectButton`, `PortraitImage`, `NameText`, `EntrySelectionArrow`, `EntryRootSizeBox`（可选尺寸宿主） |
| `WBP_ReEchoLoadoutTooltip` | `TitleText`, `DescriptionText` |
| `WBP_ReEchoTraitCardEntry` | `SelectButton`, `NameText`, `DescriptionText`；可选图片绑定为 `ArtImage`, `IconImage` |

`SelectButton` 必须是 `UReEchoIndexedButton`，不能替换成普通 `UButton`。它把动态数组索引送回父页面，再由 C++ 解析为稳定的 CharacterId、WeaponId 或 CardId。

`WBP_ReEchoTraitCardChoice` 初次打开时仍播放页面指针和全部候选的逐张揭示。免费/付费三选一的单槽刷新成功后，只将被替换槽位的卡牌置为透明并重播它的揭示；其他卡牌和指针不重置。刷新失败不播放揭示。

### 4.3 商店逻辑块的后续 WBP 接口

动态商品内容统一位于 `ShopLogicScrollBox > ShopLogicPanel`；WBP 未提供有界宿主时，C++ 将滚动区挂到根 Canvas 的固定视口并显式显示滚动条，战后模式的视口下沿必须停在回响存储托盘上方。区块顺序固定为武器配件、普通商品、规则/刷新、槽位草稿。WBP 可按同名 `BindWidgetOptional` 提供这两个宿主、`RunItemOfferPanel`、`ShopControlPanel`、`WeaponPartOfferPanel`、`WeaponLoadoutPanel`，以及 `ShopRefreshButton`、`ShopRefreshText`、`ShopRuleText`、`WeaponLoadoutText`、`SaveLoadoutButton`。这些控件只负责容器和表现，不得改变购买、刷新、草稿或保存的事件语义。

装配室的位置权威位于 `WBP_ReEchoInventoryShopScreen > Overlay_0 > DesignerLoadoutCanvas`。其中 `DesignerWeaponPanel`、`DesignerShopClock`、`DesignerAttachmentSlot0..2` 和 `DesignerCardSlot0..11` 都是 Canvas 直接子项，可在 Designer 中直接拖动或在 Slot 面板修改 Position X/Y、Size X/Y。配件槽和卡牌槽的 `*Art*` 子控件只负责图片，不应单独移动；C++ 只更新图片、置灰状态、Tooltip 和点击逻辑。不要重新运行旧式位置写入脚本覆盖人工布局；`configure_shop_designer_layout.py` 只在控件缺失时写入初始坐标，已存在控件不会被重置。

## 5. 各页面的修改边界

### 5.1 玩家 HUD、遭遇 HUD、敌人血条

Plan93/102 战斗 HUD 以 1920×1080 为作者设计面。`WBP_ReEchoPlayerHud` 保留旧头像/ProgressBar 绑定作为兼容入口，但正式构图折叠头像和旧 ProgressBar，由 `PlayerHealthFill` 接收真实生命比例、`TimeShardText` 显示 `UReEchoRunSubsystem::TimeShards` 的只读投影。免费选卡页在暂停世界时刷新成功，GameMode 必须将扣费后余额立即重投影到 `TimeShardText`，不得等待普通 Tick。`WBP_ReEchoEncounterHud` 不保留倒计时文字后的 `ArtTimeReadout` 黑色半透明底块；普通关的 `ArtClockNeedle` Render Pivot 固定在源图顶部轴心，C++ 按权威剩余/总时长从左经下半圆逆时针连续转到右（满/半/零时为 `90/0/-90` 度）。Boss 关保留 `ArtClockFrame` 钟背板，只隐藏 `CountdownText` 与 `ArtClockNeedle`，同一顶部中心位置显示 `BossHealthPanel`：`BossHealthFrame` 复用玩家 `ArtHealthFrame` 的血条底板纹理并使用紫色 Tint，`BossHealthFill` 使用暗紫色并按 EnemyRoster 中存活 Boss Combatant 的当前/最大生命从左向右缩放；Fill 必须绘制在底板纹理的不透明黑色内部之上，缩短后由黑色内部显示空血区域。`第 N 关` 与小地图保持显示。Encounter HUD 继续复用唯一的 `UReEchoMinimapCanvasWidget`，只用交付回响框包裹它；Minimap Canvas 自身保持透明，不绘制内层竞技场边框，保留 Echo 轨迹，并用当前 Player/Echo Presentation Profile 的对应头像替代实时方点。参考图底部技能栏已被产品明确废弃，WBP 与运行时纹理均不保留；不得据此伪造按钮、冷却或输入。

Plan118 的 Echo 轨迹使用 `Content/SourceArt/UI/CombatHud/MinimapInkBrush/` 归档的经典墨水笔交付，运行时只消费 512px 透明笔尖、900px Grain 和 `/Game/ReEcho/Materials/UI/M_UI_MinimapInkTrail`；`.abr`、256px 备选笔尖及 Photoshop 参数文件仅供追溯。选择 `WBP_ReEchoEncounterHud` 内的 `ReEchoMinimapCanvasWidget` 实例后，在 Details 的 `Minimap | Ink Trail` 分类调节：`Ink Trail Stamp Size Px` 控制线宽，`Ink Trail Stamp Spacing Px` 控制连续度与绘制数量，`Ink Trail Angle Jitter Degrees` 控制边缘方向变化，`Ink Trail Opacity Jitter` 控制盖印深浅变化，`Ink Trail Grain Strength` 控制纸张颗粒缺口；`Ink Trail Colors` 的 Element 0–5 按 Echo 顺序控制六条轨迹颜色，修改后由控件同步到各自动态材质的 `TrailColor` 参数，缺少对应项时回退运行时调色板，头像始终保持原图颜色。建议先调 Size/Spacing，再微调 Colors/Jitter/Grain；Spacing 越小盖印越密、成本越高。`Max Ink Trail Stamps Per Echo` 是高级安全上限，除非明确验证长轨迹成本，不应随意放大。所有参数仅影响小地图表现；不要通过修改材质 Vertex Color、Recording 采样率来调整笔触颜色或形态。

Plan93 源图与参考图归档在 `Content/SourceArt/UI/CombatHud/Plan93/`；Plan102 的八张 Player/Echo 透明头像与语义清单归档在 `Content/SourceArt/UI/CombatHud/Plan102/`。运行时 HUD 切图位于 `/Game/ReEcho/Textures/UI/CombatHud/`，小地图头像位于其 `Minimap/` 子目录并由 Character/Echo Profile 硬引用。整屏参考图不导入运行时；生命、碎片、关卡、倒计时和小地图状态仍由 C++ 提供，WBP 只拥有锚点、尺寸、层级和贴图。

可以在 UMG 中修改：

- 头像大小、裁切方式、边框和锚点。
- 血条尺寸、背景、填充图、颜色和文本排版。
- 遭遇信息的位置、字体、对齐和安全区。

不要在 Tick 蓝图中轮询生命值。玩家 HUD 和敌人血条已经订阅 `OnHealthChanged`；新增状态也应优先使用已有事件。

敌人世界空间伤害跳字显示本次经过攻击方/目标方规则修正后、目标剩余生命钳制前的伤害值。例如目标剩余 7 血、本次最终伤害 20，生命只扣 7 并死亡，但跳字显示 20。该数值来自 `FReEchoDamageEvent::RawDamage`；`AppliedDamage` 继续只表示实际扣血，不要在 WBP、材质或动画中根据当前生命重新计算跳字。字体、颜色、取整、缩放和生命周期仍由既有 Damage Number 表现链负责。

### 5.2 Start Menu、Settings、Restart

可以修改面板尺寸、按钮布局、按钮 Style、文本、焦点高亮和转场动画。不要在 WBP 中直接开始游戏、读写存档、重启关卡或退出程序；按钮只应把请求交给 C++ Delegate。

`WBP_ReEchoStartMenu` 的 Plan45 正式样板使用 `T_UI_Start_Background`、`T_UI_Start_TitleLogo`、`T_UI_Start_PrimaryActions`、`T_UI_Start_SettingsIcon` 和 `T_UI_Start_Quit`。这些 Image 都是命中测试不可见的表现层，既有 `NewGameButton`、`ContinueButton`、`GameSettingsButton` 和 `QuitButton` 作为透明交互层覆盖对应图案。`存档回溯` 固定映射到继续当前存档；无存档时仍显示，但必须置灰并禁用。关于入口当前隐藏。开始页退出只发送 C++ Delegate，由 GameMode 直接退出程序；WBP 不得自行调用退出 API。设置继续打开既有 Settings 页面，不能在 WBP 中复制设置逻辑。

项目将 `UserInterfaceSettings.RenderFocusRule` 设为 `Never`，不绘制 Unreal 默认的紫色虚线焦点框；这只隐藏默认 Focus Brush，不移除键盘/手柄焦点和导航。需要焦点反馈时，应使用与页面美术一致的按钮 Normal/Hovered/Pressed/Disabled 状态，不要重新启用默认紫色描边。

所有交互 WBP 的按钮由 Flow Coordinator 统一提供 `1.05` 倍中心悬停缩放。按钮的底图、文字、图标、选中装饰和真实点击区必须属于同一个按钮视觉根：父级若为只含一个 Button 且另有底图/装饰兄弟的 `Overlay`，即使 Button 自身已有文字 Content，也优先缩放该 Overlay；没有这种单按钮 Overlay 时才缩放 Button。不要把多个按钮放进同一个候选悬停根，也不要在各 WBP 的 Blueprint Graph 复制 Hover/Unhover 逻辑。移出时系统会恢复进入前的 Render Scale 和 Pivot，WBP 作者可继续调整原始尺寸与布局。

`WBP_ReEchoSettings`、`WBP_ReEchoRestart`、`WBP_ReEchoTraitCardEntry`、`WBP_ReEchoInventoryShopScreen`、`WBP_ReEchoPlayerHud`、`WBP_ReEchoEncounterHud` 和 `WBP_ReEchoStatsScreen` 已接入 Plan45 对应面板、卡框、立绘和 HUD 装饰。新增 Image 均不参与命中测试；原 `RootPanel`、`InventoryPanel`、`ShopPanel`、`OfferContainer`、按钮和文本绑定名称/类型保持不变。商店与装配室装饰必须继续放在各自原面板内部，使 `UReEchoInventoryShopWidget::Refresh()` 的显隐切换同时覆盖内容和美术层。

`WBP_ReEchoSettings` 的 Graphics、Audio、Controls 是固定页面结构。固定的音频 Slider 和 Checkbox 必须由 WBP 正常路径静态提供，布局、间距、样式和焦点表现归 UMG；`UReEchoSettingsWidget` 只绑定控件、刷新状态并把预览/提交/撤销请求交给 `UReEchoAudioService`。只要 WBP 已有作者ing Root，C++ 就不得用 `BuildWidgetTree()` 覆盖整页；该路径只用于设计资产完全没有 Root 时的最低可用 fallback。运行时补充的 ComboBox/Slider 使用稳定名称查找并幂等复用，重复构造不得叠加交互层。所有绑定控件必须勾选 `Is Variable`，并严格使用第 4 节列出的名称和类型。

设置页正式作者ing树不再保留早期迁移期的折叠兼容节点：旧 `CategoryTitleText` / `DetailText`、旧页签白底与重复标签、旧画面占位行、占位提示文字及按钮内重复文字均已删除。页面标题、页签文字、字段和按钮表现以当前 `Panel`、三个页签 Overlay、三个分类 Panel 及两个 Canvas 直属按钮为准；不要为兼容旧脚本重新创建这些废弃节点。C++ 中同名可选成员只服务无 Root 的最低可用 fallback。

`WBP_ReEchoRestart` 是多状态页面。隐藏某个按钮或改文案前，必须检查六种显示场景：

该 WBP 是普通 Pause、两种退出确认、Death 与 Victory 的唯一作者ing表现入口；只要资产已有 Root，`UReEchoRestartWidget` 不得用原生 fallback 覆盖它，也不要新增独立 Pause WBP。`RootPanel`、`TitleText`、`MessageText` 和关键按钮/切图绑定必须保持 `Is Variable`。

正式 Victory 的布局权威是 `VictoryCanvas`。标题、三组标签/数值、五个样例卡槽、角色、花饰、底板、按钮底图、按钮文字和透明真实按钮均为 Canvas 直属子项，因此可在 Designer 中独立选择、拖动和缩放。运行时只覆盖 `VictoryEncounterValue`、`VictoryTimeShardsValue`、`VictoryTraitCountValue` 并切换显隐；`VictoryContinueButton` 仍调用既有重开委托。不要把这些节点重新合并为一张整页截图，也不要在 C++ 中写布局坐标。

正式 Death 的布局权威是 `DefeatCanvas`。标题、到达关卡/构筑数量/时间碎片三组标签与数值、五个样例卡槽、角色、枯花、底板、两个按钮底图、按钮文字和透明真实按钮同样是 Canvas 直属子项，可在 Designer 中独立移动和缩放。运行时只覆盖 `DefeatEncounterValue`、`DefeatTimeShardsValue`、`DefeatTraitCountValue` 并切换显隐；`DefeatRestartButton` 复用既有整局重开委托，`DefeatMainMenuButton` 复用既有返回主菜单委托。不要在 WBP 中复制重开、存档或开关关卡逻辑。

正式结算接入后，旧 `ArtRestartCharacter`、`ArtResultSummaryPanel`、`ArtSelectedCardsPanel`、`ArtVictoryTitle`、`ArtDefeatTitle` 及其占位纹理/源图已经删除。不要为兼容旧脚本重新创建这些节点，也不要恢复旧的“时间线收束”“回响中断”、Boss 摘要或死亡提示文本；Victory/Death 只使用上述两个正式 Canvas。`ArtRestartDialogPanel` 不属于废弃结算层，它仍服务保存失败兜底弹窗，必须保留。

1. 普通暂停。
2. 退出到主菜单确认。
3. 退出游戏确认。
4. 玩家死亡。
5. Boss 胜利。
6. 重开确认/结算状态。

`QuitButtonText` 必须保持为 `QuitButton` 的可见内容层级，否则 C++ 更新确认文案时玩家可能看不到变化。

Plan45 普通暂停使用交付切图组装为命中测试不可见的表现层，实际交互继续由 `ResumeButton`、`RestartButton`、`QuitButton` 和右上 `PauseSettingsButton` 承担。普通状态依次表示继续、退出到主菜单、退出游戏；进入确认后，同一组按钮切换为保存并退出、不保存并退出、返回。退出到主菜单与退出游戏必须保留不同目标，保存/不保存语义由 GameMode 执行，WBP 不得直接写存档、开关关卡或退出程序。存在暂停切图时旧的 Attack Mode 动态区隐藏，避免偏离暂停效果图。

### 5.3 Loadout 两阶段所见即所得页面

- Plan132 后页面固定按“角色确认 → 武器确认”两阶段展示；第一次确认只切阶段，第二次确认才向 GameMode 广播一次最终 `(CharacterId, WeaponId)`。WBP 不得自行启动 Run、切关或写存档。
- `CharacterStagePanel > CharacterRow` 内固定放置 `CharacterEntry0..3`，`WeaponStagePanel > WeaponRow` 内固定放置 `WeaponEntry0..3`；八项都是实际运行使用的 `WBP_ReEchoLoadoutEntry` 实例，不是占位图。运行时从 CSV 写入名称、说明和资格，但复用这些 Designer 控件，不再清空后另建一套。不要删除或改名；可直接调整 Row 的 1920×1080 位置、每个 Entry 的 HorizontalBox Slot Padding，以及 Entry 蓝图内部尺寸与样式。
- 初次进入每个阶段时没有 Preview：全部条目使用明亮 `Selected` 切图，说明、箭头和确认按钮隐藏。鼠标 Hover 使用与商店一致的原生 Tooltip 定位机制：Slate 跟随当前条目并自动避让屏幕边缘，而框体视觉由 `WBP_ReEchoLoadoutTooltip` 完全管理。键盘/手柄 Focus 使用页面内 `DescriptionPanel` / `DescriptionTextScale` 大号回退框；点击与 Hover/Focus 都会把当前项保持为 `Selected`、其余项切到 `Unselected`，并显示箭头和确认按钮。
- 条目外观统一修改 `WBP_ReEchoLoadoutEntry`。`EntryRootSizeBox` 是当前阶段为角色/武器写入不同设计宽度的唯一尺寸宿主；`PortraitImage` 必须位于 `PortraitScale` 内并保持 `Stretch=Scale To Fit`，不能改为 Fill，否则枪这类近方形素材会被强行拉成长图。
- 1920×1080 位置权威位于 `WBP_ReEchoLoadoutSelection > LoadoutDesignCanvas`。角色与武器页都由同一 Designer 内的 `StageSwitcher` 承载：在层级中选中它，直接将 Details 的 `Active Widget Index` 设为 `0`（角色）或 `1`（武器），Designer 会立即切换到对应的真实运行页面；Class Defaults 的 `Designer Preview Index` 只负责 `-1` 初始全亮态或 `0..3` 选中态。`TitleText`、八个 Entry、两个 Stage Panel、键盘 Focus 回退用的 `DescriptionPanel` / `DescriptionTextScale > DescriptionText`、八张箭头、`ConfirmButton` 和 `ConfirmButtonLabel` 都应在同一 Designer 中按实际运行控件调整。鼠标悬停说明框本身才在 `WBP_ReEchoLoadoutTooltip` 中调整；`TooltipRootSizeBox.Width Override` 控制总宽，`TooltipFrame` 使用 `T_UI_Loadout_DescriptionPanel` 的九宫格 Brush 并控制边框，`TooltipSurface` 控制内边距与内底色，`TitleText` / `DescriptionText` 控制字体和换行。
- 选中箭头位于 `WBP_ReEchoLoadoutEntry.EntrySelectionArrow`，与 `SelectButton` 同属 `EntryVisualOverlay`。全局 Hover 反馈会识别该 Overlay，因此角色/武器图、名称与箭头会作为一个整体放大缩小。箭头位置在 Entry 蓝图的 Overlay Slot 中统一调整；运行时只切换可见性，不写位置。不要把箭头重新放回 Selection 的公共 Canvas，否则它不会跟随单个条目的 Hover 缩放。
- 正式源图位于 `Content/SourceArt/UI/LoadoutSelection/Plan132/`，运行时位于 `/Game/ReEcho/Textures/UI/LoadoutSelection/`。四张 `1-*.png` 是合成构图参考，不得作为整屏点击贴图；实际交互使用 16 张选中/未选中切图、解释框和箭头组合。
- 角色说明读取 `characters.csv.Description`，武器说明读取对应 `weapon_types.csv.Description`；不要在 WBP/C++ 复制中文玩法文案，也不要用按钮文字反查 ID。C++ 只使用动态索引映射稳定 ID。
- 当前交付构图顺序为勇者、智者、诗人、猎手，以及镰刀、枪、剑、弓。该顺序只属于本页面的表现映射；未知未来选项追加到已有项后，不修改全局 CSV 排序。

### 5.4 Trait 抽卡

- 卡片尺寸由 `WBP_ReEchoTraitCardChoice` 中的 `TraitCardSlot0..2` 决定，不由 C++ 正常路径设置。
- 三个 `TraitCardSlot` 内的 `DesignerTraitCardSample0..2` 仅用于 Designer 所见即所得预览；进局后 C++ 会以真实条目替换，不参与玩法数据。
- 单卡使用 `WBP_ReEchoTraitCardEntry > CardRootScaleBox > CardRootSizeBox`：ScaleBox 负责在外部卡槽中等比缩放，SizeBox 固定为卡牌底图原始 `420×593` 设计面，避免底图变形，也避免 Canvas 尺寸随文字包围盒变化。内部的位置权威位于 `SelectButton > Overlay_0 > CardDesignerCanvas`；`NameText`、`DescriptionText` 与 `IconImage` 都是该 Canvas 的直接子项，可在 Designer 画布中直接拖动或修改 Canvas Slot 的 Position/Size。不要修改或删除 ScaleBox，亦不要修改 SizeBox 的 Width/Height Override；默认文本与图片是设计期样例，进局后 `Configure` 只覆盖内容，不覆盖位置。若单独打开 Entry 时 Designer 仍显示横向全屏预览，请将右上角预览模式从 `Fill Screen` 改为 `Desired`，它不代表运行时卡牌比例。
- 选择页的 `TitleText` 与 `ConfirmButtonLabel` 都是 `WBP_ReEchoTraitCardChoice > RootPanel` 的 Canvas 直接子项，可独立拖动。`ConfirmButtonLabel` 为 `HitTestInvisible`，不会阻挡按钮；需要移动整个确认交互时，应同时移动 `ConfirmButton` 与 `ConfirmButtonLabel`。
- 三个 Slot 必须全部存在，并保持可容纳动态创建的 Entry Widget。
- 不要把 CardId 写进蓝图文本或按钮 Tag；选择仍由父 Widget 的索引到稳定 ID 映射完成。
- 正式 icon 源图统一归档为 `Content/SourceArt/UI/Cards/Icon/T_UI_CardIcon_{CardId}.png`，运行时导入到 `/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_{CardId}`。Plan128 使用 `scripts/ue/import_plan128_complete_card_icons.py` 按权威 CSV 批量导入当前全部启用且可投放卡牌；替换图标只改同名 SourceArt/Texture2D，不要在 WBP 或 C++ 新增按中文名称分支。源图保留交付原生尺寸，不要为了统一规格重采样。
- 不要在卡片按钮事件中自行移除父页面或打开商城。抽卡结算、下一帧关闭、商城创建和暂停恢复由 GameMode 管理。

### 5.5 Inventory/Shop

- `InventoryPanel` 与 `ShopPanel` 是同一屏幕的两种展示模式。
- 商店逻辑固定拆为普通商品、规则/刷新、武器配件购买、装备槽位草稿、回响管理五块；不要再把不同类型的报价合并到同一索引数组。
- `OfferContainer` 承载 `RunItemOfferPanel`；普通商品按钮由 `ReEchoShopCatalog` 生成。`WeaponPartOfferPanel` 单独承载 `parts.csv` 中兼容当前武器且允许出售的配件。
- 两类报价各自解析稳定 ItemId；商店刷新只轮换普通商品，不改变配件按钮与配件 ID 的映射。
- `ShopLogicScrollBox` 必须有独立固定边界并显示滚动条，`WeaponPartOfferPanel` 必须排在普通商品之前；回响管理使用独立底部 `ScaleBox` 托盘和高于商城美术的 Canvas ZOrder，不得重新塞回商品纵向列表或使用默认 ZOrder。
- 装配室武器面板、时钟、三个配件槽与十二个卡牌槽的位置必须由 `DesignerLoadoutCanvas` 内的同名 Designer 控件保存；不得在 C++ 中新增固定坐标或在每次刷新时回写 Canvas Slot。
- 可以调整容器宽度、间距、滚动方案、背景和按钮视觉；不可在 WBP 中扣除时间碎片或直接写 Inventory。
- 页面数据可能先于 `NativeConstruct` 到达。新增刷新逻辑时必须允许“数据已到、动态控件尚未创建”的生命周期状态，禁止直接对未校验数组使用 `[0]`。
- `ShopPanel` 内的装配室由 C++ 根据 `slot_profiles.csv` 动态生成槽组和容量，当前主要显示 Core/Grip/Blade 三类但不得硬编码具体名称；`G_3_22` 可增加非 Core 容量。
- 点击已拥有配件只调整页面草稿；同槽满时由明确点击的新配件替换最早草稿项。购买立即保存所有权且不自动装备，“保存配置”才提交整套 `EquippedParts`；直接离开商店丢弃未保存草稿。

### 5.6 Stats

属性页当前以两个动态文本块展示玩家和回响快照。可以调整两列布局、字体、背景和无回响时的占位表现。属性值必须继续由 C++ 快照提供，不要在 WBP 中重新计算攻击、防御或武器规则。

### 5.7 Weather 和伤害数字

这两项目前不是常规菜单 WBP：

- Weather 使用 `NativeTick` 更新雨滴状态，并在 `NativePaint` 绘制雨和迷雾。这是 Plan 29 明确允许的混合 Paint Surface。
- Damage Number 是世界空间 TextRender Actor，跟随世界位置和生命周期。
- Damage Number 仅使用 `/Game/ReEcho/Fonts/DamageNumbers/F_DamageNumber_MFYuYue_Font`；源文件归档在 `Content/SourceArt/UI/CombatHud/DamageNumbers/`，授权边界为非商用，不得扩散到其他 UI 或商业交付。
- 反应伤害颜色按权威 `ReactionBehaviorId` 映射：蒸发 `#A5CBF3`、导电 `#EBC02C`、灼烧 `#E86A12`、生长 `#92C039`、强化 `#F1B84C`。强化被下一次伤害反应消费时，该次跳字使用强化金色；无反应来源时继续回退到元素色或物理白色。
- 跳字使用支持顶点 Alpha 的半透明 TextRender 材质；在 `0.9s` 上漂生命周期内从完全不透明连续淡出到完全透明，随后销毁。颜色 Alpha 必须乘入淡出曲线，不能用不透明材质或固定 Alpha 覆盖。

若要把它们迁移成 UMG，必须先评估世界/屏幕空间转换、对象池、绘制性能、输入穿透和分层，不能只创建一个空 WBP 替换。

## 6. 推荐修改流程

### 6.1 仅修改外观

1. 确认目标 WBP 和对应原生父类。
2. 检查是否有人正在编辑同一 `.uasset`；同一资产必须串行修改。
3. 在 Unreal Editor 中编辑 WBP，不要用文本或十六进制工具修改 `.uasset`。
4. 保留本文列出的绑定控件名和控件类型。
5. 修改锚点、Safe Zone、SizeBox、Padding、Style、字体、贴图或动画。
6. Compile 并 Save 目标 WBP。
7. 运行 `CompileAllBlueprints`。
8. 在 PIE 中走到真实页面，检查鼠标、键盘焦点、返回流程和暂停状态。

### 6.2 新增动态 UI 内容

1. 优先创建可复用 Entry WBP，而不是在父页面复制固定按钮。
2. Entry 用索引事件通知父 Widget。
3. 父 Widget 维护“索引 → 稳定 ID”映射。
4. Gameplay/RunSubsystem 验证稳定 ID 并执行结果。
5. Entry 只接收显示数据，不保存权威游戏状态。
6. 动态容器刷新前校验容器、数组数量和 Entry 对象有效性。

### 6.3 新增页面

1. 创建继承 `UUserWidget` 的原生 C++ 父类，公开最小的数据初始化函数和 Delegate。
2. 用 `BindWidget`/`BindWidgetOptional` 明确 WBP 契约。
3. 创建以该类为 Parent Class 的 `WBP_ReEcho*`。
4. 在 `EReEchoUIScreen` 中增加页面身份，并在 UI Manager 注册 Class 和语义 Layer。
5. 由 Flow Coordinator 负责创建、加入 Layer、配置输入、暂停和关闭；GameMode 只绑定玩法 Delegate。
6. 为资产缺失或加载失败提供安全降级；不得让运行卡在暂停且无可操作页面的状态。

## 7. 禁止事项

- 不在 WBP 中直接修改 RunSubsystem、存档、货币、背包、卡牌或武器权威状态。
- 不手工修改 `.uasset` 文件内容。
- 不在多个类中散写 `AddToViewport(数字)`。
- 不在设计器中硬编码动态角色、武器、卡牌或商城报价数量。
- 不通过显示文本、按钮名称或数组位置充当长期稳定 ID。
- 不删除或重命名绑定控件而不同时更新原生父类并重新编译 Blueprint。
- 不让页面在 Slate/UMG 点击 Delegate 尚在分发时同步删除祖先 Widget 并立即抢占焦点；跨页面切换应由协调层安全延迟。
- 不把 C++ fallback 当成正式 UI。Fallback 只保证 WBP 缺失时仍有最低可用界面。

## 8. DPI、安全区和可访问性

每次较大的 UI 修改至少检查：

- 1280×720。
- 1920×1080。
- 2560×1440。
- 一种 21:9 超宽分辨率。

检查项目：

- 关键按钮不超出 Safe Zone。
- 文本不裁切、不重叠，中文字体可正常 Cook。
- 动态列表数量增加后仍可访问；必要时使用 ScrollBox。
- 键盘初始焦点清晰，鼠标点击和返回键路径正常。
- 禁用、选中、悬停、按下状态有足够视觉差异。
- HUD 不遮挡核心战斗区域，天气层不接收输入。

## 9. 验证清单

### 客观检查

```text
scripts/ue/Build-Editor.cmd
CompileAllBlueprints
scripts/ue/Run-Automation.cmd -Filter ReEcho
python scripts/validate_project.py
git diff --check
```

### PIE 流程检查

1. Start Menu：新游戏、继续、设置、返回。
2. Loadout：切换角色、切换武器、确认、动态条目焦点。
3. 战斗 HUD：玩家血量、敌人血条、遭遇倒计时和最后 5 秒警示。
4. Tab Stats：玩家/回响两列、无回响状态、关闭后恢复输入。
5. 背包/商城：打开、购买、余额更新、已拥有状态、关闭。
6. 抽卡：三卡揭示、选择、额外抽卡、抽卡后自动进入商城、商城关闭后进入下一遭遇。
7. Esc：暂停、设置、退出确认、取消、继续。
8. 死亡与胜利：标题、按钮集合、重启和退出路径。
9. Weather/Damage：雨、迷雾、伤害数字不阻塞输入且层级正确。

## 10. 代码与资产定位

- WBP：`Content/ReEcho/UI/`
- UI 源图/参考：`Content/SourceArt/UI/InteractionPlaceholder/`
- 已消费的运行时 UI 纹理：`Content/ReEcho/Textures/UI/InteractionPlaceholder/`
- 原生 UI 头文件：`Source/ReEcho/Public/UI/`
- 原生 UI 实现：`Source/ReEcho/Private/UI/`
- UI 层和输入协调：`ReEchoUIManagerSubsystem.*`
- 页面生命周期和安全转场：`UI/Framework/ReEchoUIFlowCoordinatorSubsystem.*`
- Screen/Layer 类型：`UI/Framework/ReEchoUIScreenTypes.h`
- 页面创建和流程切换：`ReEchoGameMode.*`
- UI 迁移范围与验收：`plans/29-ui-umg-migration.md`
- UI 数据来源说明：`Design/Data/ReEchoData使用说明.md`

修改前先从目标 WBP 找到原生父类，再检查绑定名称、数据入口和 Delegate。只要遵守“UMG 管表现、C++ 管状态和流程、数据表管生产数据”，大部分视觉改动都可以独立完成而不影响玩法逻辑。
