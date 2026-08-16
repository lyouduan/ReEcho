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
| 玩家 HUD | `WBP_ReEchoPlayerHud` | `UReEchoPlayerHudWidget` | `PlayerHud` | 玩家头像、生命条、生命文本；生命变化由事件驱动 |
| 遭遇 HUD | `WBP_ReEchoEncounterHud` | `UReEchoEncounterHudWidget` | `GameplayHud` | 当前遭遇和倒计时；最后 5 秒警示色由 C++ 状态驱动 |
| 敌人血条 | `WBP_ReEchoEnemyHealthBar` | `UReEchoHealthBarWidget` | 世界空间 Widget | 敌人头顶血条；生命变化由事件驱动 |
| 开始菜单 | `WBP_ReEchoStartMenu` | `UReEchoStartMenuWidget` | `Start` | 新游戏、继续、设置；继续按钮可用性由存档状态决定 |
| 初始配装 | `WBP_ReEchoLoadoutSelection` | `UReEchoLoadoutSelectionWidget` | `Loadout` | 角色和武器动态容器、状态文本、确认按钮 |
| 配装条目 | `WBP_ReEchoLoadoutEntry` | `UReEchoLoadoutEntryWidget` | 父页面内部 | 角色/武器共用的动态条目；UMG 控制头像和按钮尺寸 |
| 设置 | `WBP_ReEchoSettings` | `UReEchoSettingsWidget` | `Settings` | 图形、声音、控制分类和返回操作 |
| 暂停/死亡/胜利 | `WBP_ReEchoRestart` | `UReEchoRestartWidget` | `Pause` | 同一页面按明确模式显示暂停、死亡、胜利和退出确认状态 |
| 抽卡页面 | `WBP_ReEchoTraitCardChoice` | `UReEchoTraitCardChoiceWidget` | `BuildChoice` | 三个卡槽、标题、货币和揭示动画 |
| 抽卡条目 | `WBP_ReEchoTraitCardEntry` | `UReEchoTraitCardEntryWidget` | 抽卡页面内部 | 单张卡的按钮、标题、描述和选择提示 |
| 背包/商城 | `WBP_ReEchoInventoryShopScreen` | `UReEchoInventoryShopWidget` | `Screen` | 背包与商城共用页面；报价按钮由目录动态生成 |
| 属性页面 | `WBP_ReEchoStatsScreen` | `UReEchoStatsWidget` | `Screen` | 玩家和回响的两列属性展示 |
| 天气 | 无独立 WBP | `UReEchoWeatherWidget` | `Weather` | 明确保留的 C++ `NativePaint` 混合绘制面，负责雨和迷雾 |
| 伤害数字 | 无独立 WBP | `AReEchoDamageNumberActor` | 世界空间 | 使用 `UTextRenderComponent` 的世界空间反馈，不是菜单 UMG |

所有 WBP 位于：`Content/ReEcho/UI/`。

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
| `WBP_ReEchoPlayerHud` | `PlayerPortrait`, `PlayerHealthProgress`, `PlayerHealthText` |
| `WBP_ReEchoEncounterHud` | `EncounterText`, `CountdownText` |
| `WBP_ReEchoEnemyHealthBar` | `ProgressBar` |
| `WBP_ReEchoStartMenu` | `StatusText`, `ContinueButton`, `NewGameButton`, `GameSettingsButton` |
| `WBP_ReEchoLoadoutSelection` | `StatusText`, `CharacterRow`, `WeaponRow`, `ConfirmButton` |
| `WBP_ReEchoSettings` | `DetailText`, `CategoryTitleText`, `GraphicsSettingsButton`, `AudioSettingsButton`, `ControlsSettingsButton`, `RestoreDefaultsButton`, `ApplyAndReturnButton`, `AudioPanel`, `MasterVolumeSlider`, `MusicVolumeSlider`, `AmbienceVolumeSlider`, `CombatSfxVolumeSlider`, `UiSfxVolumeSlider`, `MasterMuteCheckBox`, `MusicMuteCheckBox`, `AmbienceMuteCheckBox`, `CombatSfxMuteCheckBox`, `UiSfxMuteCheckBox`, `DiagnosticToneCheckBox` |
| `WBP_ReEchoRestart` | `TitleText`, `MessageText`, `ResumeButton`, `RestartButton`, `QuitButton`, `SettingsButton`, `QuitButtonText` |
| `WBP_ReEchoTraitCardChoice` | `TraitCardContainer`, `TraitCardSlot0`, `TraitCardSlot1`, `TraitCardSlot2`, `TitleText`, `SubtitleText`, `CurrencyText`, `NeedleWidget` |
| `WBP_ReEchoInventoryShopScreen` | `BackgroundImage`, `InventoryPanel`, `ShopPanel`, `CurrencyText`, `InventoryText`, `CloseButton`, `OfferContainer` |
| `WBP_ReEchoStatsScreen` | `BackgroundImage`, `PlayerStatsText`, `EchoStatsText`, `CloseButton` |

### 4.2 动态条目必需绑定

以下条目使用严格的 `BindWidget`。缺失、类型不匹配或重命名会导致 Blueprint 编译错误，或在错误资产版本下造成运行时问题。

| WBP | 必需控件名 |
|---|---|
| `WBP_ReEchoLoadoutEntry` | `SelectButton`, `PortraitImage`, `NameText` |
| `WBP_ReEchoTraitCardEntry` | `SelectButton`, `KickerText`, `NameText`, `DescriptionText`, `SelectHintText` |

`SelectButton` 必须是 `UReEchoIndexedButton`，不能替换成普通 `UButton`。它把动态数组索引送回父页面，再由 C++ 解析为稳定的 CharacterId、WeaponId 或 CardId。

## 5. 各页面的修改边界

### 5.1 玩家 HUD、遭遇 HUD、敌人血条

可以在 UMG 中修改：

- 头像大小、裁切方式、边框和锚点。
- 血条尺寸、背景、填充图、颜色和文本排版。
- 遭遇信息的位置、字体、对齐和安全区。

不要在 Tick 蓝图中轮询生命值。玩家 HUD 和敌人血条已经订阅 `OnHealthChanged`；新增状态也应优先使用已有事件。

### 5.2 Start Menu、Settings、Restart

可以修改面板尺寸、按钮布局、按钮 Style、文本、焦点高亮和转场动画。不要在 WBP 中直接开始游戏、读写存档、重启关卡或退出程序；按钮只应把请求交给 C++ Delegate。

`WBP_ReEchoSettings` 的 Graphics、Audio、Controls 是固定页面结构。固定的音频 Slider 和 Checkbox 必须由 WBP 正常路径静态提供，布局、间距、样式和焦点表现归 UMG；`UReEchoSettingsWidget` 只绑定控件、刷新状态并把预览/提交/撤销请求交给 `UReEchoAudioService`。不要依赖 C++ `BuildAudioPanel()` 向正常 WBP 动态注入整套布局，该路径只用于设计资产缺失时的最低可用 fallback。所有绑定控件必须勾选 `Is Variable`，并严格使用第 4 节列出的名称和类型。

`WBP_ReEchoRestart` 是多状态页面。隐藏某个按钮或改文案前，必须检查四种显示场景：

1. 普通暂停。
2. 退出确认。
3. 玩家死亡。
4. Boss 胜利。

`QuitButtonText` 必须保持为 `QuitButton` 的可见内容层级，否则 C++ 更新确认文案时玩家可能看不到变化。

### 5.3 Loadout 动态容器

- `CharacterRow` 与 `WeaponRow` 只提供动态容器，不要在设计器中手工放置固定数量的角色/武器按钮。
- 条目外观统一修改 `WBP_ReEchoLoadoutEntry`。
- 头像尺寸、缩放方式、按钮高度和标签排版全部由条目 WBP 控制。
- 角色和武器数量来自 CSV 快照，不要假设永远是固定数量。
- 不要用按钮文字反查 ID；C++ 使用动态索引映射到稳定 ID。

### 5.4 Trait 抽卡

- 卡片尺寸由 `WBP_ReEchoTraitCardChoice` 中的 `TraitCardSlot0..2` 决定，不由 C++ 正常路径设置。
- 单卡内部的标题、说明、提示、Padding 和按钮 Style 修改 `WBP_ReEchoTraitCardEntry`。
- 三个 Slot 必须全部存在，并保持可容纳动态创建的 Entry Widget。
- `NeedleWidget` 的位置和视觉可由 UMG 调整，但揭示时序和是否可选择由 C++ 控制。
- 不要把 CardId 写进蓝图文本或按钮 Tag；选择仍由父 Widget 的索引到稳定 ID 映射完成。
- 不要在卡片按钮事件中自行移除父页面或打开商城。抽卡结算、下一帧关闭、商城创建和暂停恢复由 GameMode 管理。

### 5.5 Inventory/Shop

- `InventoryPanel` 与 `ShopPanel` 是同一屏幕的两种展示模式。
- `OfferContainer` 是动态容器，不要在里面放固定报价按钮并依赖它们的序号。
- C++ 会根据 `ReEchoShopCatalog` 创建报价条目并处理价格、已拥有状态和购买请求。
- 可以调整容器宽度、间距、滚动方案、背景和按钮视觉；不可在 WBP 中扣除时间碎片或直接写 Inventory。
- 页面数据可能先于 `NativeConstruct` 到达。新增刷新逻辑时必须允许“数据已到、动态控件尚未创建”的生命周期状态，禁止直接对未校验数组使用 `[0]`。

### 5.6 Stats

属性页当前以两个动态文本块展示玩家和回响快照。可以调整两列布局、字体、背景和无回响时的占位表现。属性值必须继续由 C++ 快照提供，不要在 WBP 中重新计算攻击、防御或武器规则。

### 5.7 Weather 和伤害数字

这两项目前不是常规菜单 WBP：

- Weather 使用 `NativeTick` 更新雨滴状态，并在 `NativePaint` 绘制雨和迷雾。这是 Plan 29 明确允许的混合 Paint Surface。
- Damage Number 是世界空间 TextRender Actor，跟随世界位置和生命周期。

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
- 原生 UI 头文件：`Source/ReEcho/Public/UI/`
- 原生 UI 实现：`Source/ReEcho/Private/UI/`
- UI 层和输入协调：`ReEchoUIManagerSubsystem.*`
- 页面生命周期和安全转场：`UI/Framework/ReEchoUIFlowCoordinatorSubsystem.*`
- Screen/Layer 类型：`UI/Framework/ReEchoUIScreenTypes.h`
- 页面创建和流程切换：`ReEchoGameMode.*`
- UI 迁移范围与验收：`plans/29-ui-umg-migration.md`
- UI 数据来源说明：`Design/Data/ReEchoData使用说明.md`

修改前先从目标 WBP 找到原生父类，再检查绑定名称、数据入口和 Delegate。只要遵守“UMG 管表现、C++ 管状态和流程、数据表管生产数据”，大部分视觉改动都可以独立完成而不影响玩法逻辑。
