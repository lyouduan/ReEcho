# `MOD-ReEchoUI`：UI 文档型逻辑模块

## 模块状态

- 当前状态：文档型逻辑模块入口；**不是** `ReEcho.uproject` 中的 Runtime Module。
- Runtime Module：无；当前代码仍属于 `MOD-ReEcho`。
- Build 文件：无；当前构建规则仍位于 `Source/ReEcho/ReEcho.Build.cs`。
- 主要目录：`Source/ReEcho/Public/UI/`、`Source/ReEcho/Private/UI/`、`Content/ReEcho/UI/`、`Content/ReEcho/Textures/UI/`、`Content/SourceArt/UI/`。
- UI 架构设计权威：[ReEcho UI 修改指导](../../../Design/UI/ReEcho_UI修改指导.md)。
- 相关 Plan：Plan29、Plan34、Plan45、Plan51、Plan70、Plan93、Plan99、Plan102、Plan110、Plan116、Plan118、Plan132、Plan145、Plan147、Plan149、Plan150、Plan154。

## 存在原因

为 UI 工作提供稳定的 `MOD-ReEchoUI` 阅读入口，把页面框架、Widget 代码和 WBP 设计规范路由到同一处。该标识只用于文档组织，不提前声明独立模块、源码迁移或新的依赖拓扑。

## 当前职责与边界

- `UReEchoUIManagerSubsystem` 管理 Screen Class、实例和 Viewport 层级。
- `UReEchoUIFlowCoordinatorSubsystem` 管理页面开关、焦点、输入模式和暂停策略，并在屏幕创建后递归遍历嵌套 `UUserWidget`，为已有 `UButton` 统一绑定 `UI.Hover` / 基础 `UI.Confirm` 与 `1.05` 倍中心悬停缩放；具体 Widget 可通过 `BindButtonAudioFeedback` 把稳定语义覆盖为 `UI.CardSelect` 等专用反馈，绑定对象幂等更新而不重复注册 Delegate。父级若是只有一个 Button 且有视觉兄弟的 `Overlay`，即使 Button 已有 Content 也优先缩放该完整根，否则缩放 Button，移出后恢复作者ing Scale/Pivot。设置页的 `ComboBox` 选择与 `Slider` 交互完成由 `UReEchoSettingsWidget` 补发一次基础 `UI.Confirm`。所有注册页面的创建/复用/关闭/聚焦结果与按钮点击统一写入 Shipping 可用的 `Saved/Logs/UIInteractionAudit.log`；`UReEchoIndexedButton` 自审计以覆盖页面打开后才生成的动态条目，卡组入口另记录命令接收、拒绝原因、打开结果和候选 ID。
- 交付版 Settings 保持“总音量 / 背景音乐 / 音效音量”三滑条；第三条是非音乐聚合控制，同时预览并保存 `Ambience`、`CombatSfx`、`UiSfx`，因此 `Ambience_Rain` 等环境循环不需要额外第四条滑条。
- 通关后商店的回响存储主弹窗与未决离店确认层由独立 `WBP_ReEchoStoragePopup` 作者ing，`WBP_ReEchoInventoryShopScreen` 只放一个全屏子控件；正式底板、按钮四态、当前锚点/本场回响文字和按钮几何均可在弹窗 Designer 中预览及微调。`UReEchoInventoryShopWidget` 只按稳定名称投影 G_3_02 的单一时间锚点状态与待处理回响，并绑定“设为/替换锚点”和“跳过”窄事件；不展示容量、三槽或逐槽操作，也不写回作者几何。回响摘要以“第 N 关 | 角色正式名 | 武器正式名”展示，名称读取 CSV `DisplayName`；旧记录的武器类型 ID 允许回退到 `weapon_types.csv`，未知项不显示内部 ID。无完整 WBP 时仍保留最低可用的动态 fallback。
- `WBP_ReEchoSettings` 与 `WBP_ReEchoRestart` 的作者ing Root 是正常表现权威；原生 `BuildWidgetTree()` 只在完全没有 Root 时建立最低可用 fallback，不得因单一可选绑定缺失而覆盖整页。Settings 的运行时 ComboBox/Slider 按稳定名称幂等复用。设置页外层 `SettingsDimmer` 保持作者化全屏纯黑遮罩（黑色、Alpha 1.00），不得改成彩色底板或由 C++ 重建其几何。
- `WBP_ReEchoStartMenu` 的 `SaveRollbackPanel` 是正式三槽存档页的布局权威；标题、关闭按钮、三行深/浅底板、截图、便签、名称、摘要和时间文本都由 WBP Canvas 直接拥有并可在 Designer 中调整。`UReEchoStartMenuWidget` 只消费 Run 提供的三条只读摘要，切换占用/空槽美术、载入真实截图并广播稳定槽号；它不选择物理 SaveGame 名、不写存档，也不在 C++ 中重建几何。占用槽点击立即读取，空槽意图由 GameMode 解析为第一个空槽，新旧存档兼容和全槽策略归 Run。
- 三选一卡牌页的三个单槽刷新按钮与样例文案由 `WBP_ReEchoTraitCardChoice` 的 `ShopCardRefreshButton0..2` / `ShopCardRefreshText0..2` 持有，Designer 中可直接预览、拖动与缩放；`UReEchoTraitCardChoiceWidget` 运行时复用这些控件并只更新索引、次数、费用、显隐、启用状态和点击广播。按钮四态统一复用 `T_UI_Pause_ButtonLight`；无 WBP 的原生兜底仍动态创建同款按钮。
- 商店装配树的卡牌槽框由 `DesignerCardSlot0..11` 的 Button 四态持有，子图层 `DesignerCardSlotArt0..11` 只承载已拥有卡牌内容；装配树固定提供两页，第一页投影第 1–12 张、第二页投影第 13–24 张，第二页继续复用同一组作者化槽位及其位置，不复制第二套几何或卡牌状态。`DesignerPageCounter` 展示 `1/2`、`2/2`，`ArtFormalPageLeft` / `ArtFormalPageRight` 负责切页；透明命中层只接收点击，箭头图本身按统一按钮契约在可用时以中心 `1.05` 倍悬停缩放、移出恢复作者尺寸。`T_UI_Shop110_EmptyCardSlotIcon` 锁图用于左侧 `DesignerPackOfferIcon0..2` 卡组商品位，不进入右侧装配树。
- `WBP_ReEchoSettings` 的三个分类内容容器、文字字体/颜色与 Slot 布局均由 WBP 作者ing；C++ 只切换容器并绑定设置值/交互。运行时生成的下拉选项从 WBP `GraphicsValue0` 读取字体、颜色和渲染位移，不另设一套程序样式。
- `WBP_ReEchoRestart` 复用既有 Pause 层承载普通暂停、退出到主菜单确认、退出游戏确认和 Death/Victory 结算状态，不新增独立 Pause WBP；交付切图只负责表现，透明真实按钮继续发出继续、设置、保存/不保存退出、返回等类型化 Delegate，目标关卡切换和程序退出由 `AReEchoGameMode` 执行。正式结算页的透明按钮与可见底图/文字是 Canvas 兄弟节点，因此 `UReEchoRestartWidget` 以 visual-only 绑定把统一 `1.05` 倍中心悬停缩放同步到对应底图和标签；该绑定不接管 WBP 几何，也不重复点击审计或音效。正式 `VictoryCanvas` / `DefeatCanvas` 是结算表现的唯一权威；被其替代的旧结果标题、摘要、角色、卡牌底板和按钮素材已经删除，不得重新导入。保存失败仍使用 `ArtRestartDialogPanel`，普通暂停与退出确认层保持不变。
- 正式 Victory 由同一 `WBP_ReEchoRestart` 内的可选 `VictoryCanvas` 作者ing；其全部视觉与文字保持可独立编辑，C++ 只投影真实关卡总数、时间碎片和构筑数量，并把 `VictoryContinueButton` 转发到既有 `OnRestartRequested`，不新增结算或推进逻辑。`ArtVictoryCharacterFormal` 继续拥有位置和尺寸，运行时只按 `CurrentBuild.CharacterId` 把 Brush 替换为选角页对应的 `Selected` 角色纹理；其 Canvas ZOrder 固定高于同 Canvas 其他兄弟，避免角色局部被装饰或文字遮住。
- 正式 Death 由同一 `WBP_ReEchoRestart` 内的可选 `DefeatCanvas` 作者ing；视觉、文字和五个空槽底板均保持可独立编辑，C++ 只投影真实到达关卡、时间碎片、构筑数量和只读卡牌展示。`DesignerDefeatCardIcon0..4` 是对应空槽上方的正方形作者化覆盖层：按权威 `Tier` 降序展示当前构筑最多五张卡牌，同 Tier 保持获得顺序；图标路径和缺图时的通用卡图回退与商店一致，未占用覆盖层隐藏而不改变空槽 Brush、作者几何、Tooltip 或交互。`DefeatRestartButton` 转发既有重开请求，`DefeatMainMenuButton` 转发既有返回主菜单请求，不伪造击败数、金币或修改结算状态所有权。`ArtDefeatCharacterFormal` 同样只在运行时替换 Brush 且保持 Canvas 最前层；四个已知角色复用 LoadoutSelection `Selected` 纹理，未知 ID 或缺图回退 J_HEART，WBP 作者几何不被 C++ 覆盖。
- 商店/背包页允许 `WBP_ReEchoRestart` 以更高 Pause 层覆盖：按 `P` 不关闭商店，不触发战后推进或回响存储门禁；关闭 Pause 后必须重新聚焦商店并继续保持世界暂停与菜单能力阻挡。
- 结算统计文字由 `WBP_ReEchoRestart` 的 `VictoryCanvas` / `DefeatCanvas` 独立作者化：每页关卡、构筑数量、碎片、回响伤害、本体伤害、反应次数、最高单次伤害与击杀数的标签/数值，其位置、尺寸、字体、字号、颜色、对齐、换行和裁切全部以 WBP 为准。`UReEchoRestartWidget` 不再在构造时回填固定三列坐标或 18/20 字号，只更新真实数值；保留控件名与 `Is Variable` 绑定，标签文案可以直接编辑。`ReEcho.UI.RestartWidgetAuthoredStats` 对保存值和人工微调值验证首次构造、切换结果页与再次构造均不覆盖表现。
- WBP/UMG 管理布局、尺寸、样式、动画和焦点表现。
- 商店刷新按钮的 `ShopRefreshCountText` 使用 `BindWidgetOptional`，由 `WBP_ReEchoInventoryShopScreen` 保存示例、字体、字号、颜色、换行、裁切与对齐；其 `DesignerRefreshTextBox` 为 Canvas 子控件，可以直接拖动和改变文字区域。C++ 的兼容内容重建只在缺少此文字时运行，正常路径仅更新实时文案、按钮可用状态和既有刷新委托。`ReEcho.UI.Shop.AuthoredRefreshText` 检查保存值、任意微调值、构造前数据更新、再次构造以及免费/付费/无限/禁用状态。
- C++ Widget 管理只读展示状态、类型化绑定、事件转发和页面生命周期。
- `UReEchoEncounterTransitionWidget` 是默认 ZOrder 10000 的非交互视口最上层 Screen。第一关结算后通过 WmfMedia/HAP、MediaTexture 直绘播放无音轨 `Stage01To02.mov`，并在首个有效视频表面出现后独立启动 `S_Stage01To02`；GameMode使用既有局间门停止玩法并显式停止Music State，但不暂停World媒体时钟。该路径不先播放 Hap Alpha、不打开 CardChoice 或 Shop。第二关及以后在权威时间到 00 后通过动态材质从第0帧播放 `EncounterEndToCardChoiceV2.mov`（Hap Alpha、RGBA、120帧、40FPS）：0–57完成一次入场和首轮摆动，再追加两遍27–57，使钟表共来回摆动三轮；后两轮跳过重复中心帧。第一段与 `CardChoiceToShop.mov` 共用 `(0.17,0.0806)–(0.83,0.8006)` 的16:9 Fit区域，顶部对齐 `ArtClockNeedle` 的作者位置Y=87。第2–7关全部免费抽卡完成后播放第二段：立即关闭已确认的TraitChoice以露出游戏场景，再创建正常Enabled、`HitTestInvisible`、初始透明的商店；源58–78帧用0.5秒SmoothStep将商店从0渐入到1，实现游戏场景到商店的平滑过渡，不使用纯黑Backdrop。商店创建后解除其菜单Pause但保持Run阶段和能力阻挡，使媒体时钟继续推进。商店从第58帧起提供 `ArtFormalLoadoutTree` 实际几何；第68帧起媒体缩放、移动到小面具人锚点并淡出，此时商店渐入进度约50%，终点保留初始媒体57.5%尺寸。媒体完成后恢复商店 `Visible`、焦点和World Pause。失败时幂等放行；两段透明视频无音轨且不循环，`Music.Shop`连续。03→00场景重影仍由相机后处理负责。
- Stage01To02 播放时媒体画面保持 `HitTestInvisible`，仅右上角 `StageCgSkipButton` 接收点击。按钮资格由 GameMode 根据 RunSubsystem 的账号级已观看状态注入；首次播放、失败路径和非 CG 状态均折叠。Widget 点击后立即禁用按钮并广播一次请求，不直接写存档或推进关卡。
- Stage01To02 的独立 `S_Stage01To02` 组件使用 `Master × Music Bus × 1.5` 的有效增益，不继承 `Music.Encounter` 的 `0.5` BaseVolume，也不响应 Master/Music 静音开关。独立组件仍由 Widget 在首个有效视频帧启动，并在跳过、失败或媒体结束时精确停止。
- Player HUD 显式接收当前玩家的 Combatant 与 CombatEvents：`OnHurt` 仅触发瞬时受击红光，生命变化仅更新可配置的低血量底色。`UReEchoPlayerScreenFeedbackWidget` 独占合成、钳制、重触发和死亡清理等表现状态；`WBP_ReEchoPlayerScreenFeedback` 的 Class Defaults 是阈值、强度、曲线指数、呼吸和材质参数的 Editor 调参表面。HUD 硬引用并构造该 WBP Class，Widget 硬引用 `/Game/ReEcho/Materials/UI/M_UI_PlayerHurtVignette`，保证 cook 收集；材质缺失时使用不影响玩法的原生左右边缘 fallback。
- Plan93/102/158 战斗常驻 HUD 以两个 WBP 为视觉权威：Player HUD 用 `PlayerHealthFill` 映射真实生命比例并显示 Run 的只读 TimeShards；免费选卡暂停期扣费成功后由 GameMode 立即刷新投影。Encounter HUD 显示 `第 N 关`、`MM:SS` 与真实小地图；普通关指针按 Encounter Director 剩余/总时长从左经下半圆逆时针转到右，不显示倒计时黑底块。Boss 关保留 `ArtClockFrame` 与 `ArtClockNeedle`，只隐藏倒计时文字；`BossHealthArc` 是专用 `UReEchoBossHealthArcWidget`，通过硬引用 UI 材质 `M_UI_BossHealthArc` 将原钟面的下半刻度环显示为醒目紫红剩余段及近黑灰空血段，`Tick Contrast` 默认 0.25，弱化刻度以凸显连续填充。独立作者化 `BossHealthPercentText` 在原倒计时区域显示整数百分比，运行时不改其字体、颜色或几何；真实空/满血才显示 0%/100%，中间限制为 1%–99%。弧形与指针消费同一个当前阶段生命比例，满/半/空分别为指针 `90/0/-90` 度；阶段回血恢复比例，不累计各阶段总血量。正式 WBP 已删除旧横条三节点。`Boss Health Preview` 提供设计期模式和血量样例，弧形颜色/半径/几何由 WBP 拥有，临时 MID/Slate Brush 不替换序列化作者资产；原生无 WBP 兜底仍保留最小横条。Minimap Canvas 保持透明且无内层竞技场边框，以 Player/Echo Presentation Profile 头像绘制实时位置，缺图才回退色点；废弃底部技能栏不回归。所有 Widget 仅消费只读状态，不写 Run、时钟或生命。调参见 [Boss 弧形血条调整指南](../../../Design/UI/ReEcho_Boss弧形血条调整指南.md)。
- Plan118 将 Minimap Canvas 的 Echo 轨迹从 2px 纯色折线改为沿同一投影路径确定性盖印经典墨水笔尖。`UReEchoMinimapCanvasWidget` 硬引用 UI 材质以进入 Cook；笔尖尺寸、间距、角度/透明度抖动、Grain 强度和固定六项 `Ink Trail Colors` 调色板均在该控件实例的 `Minimap | Ink Trail` 分类调整。控件按 Echo 顺序为每个调色板项维护独立动态材质实例，把颜色写入 `TrailColor` 向量参数；Slate Tint 仅乘单次盖印 Alpha，因此不依赖 UI Material 的 Vertex Color RGB。缺项回退视图颜色且不染色头像。盖印跨折线段保持间距，并受单 Echo 最大数量约束；材质缺失时才回退旧折线，不改变录制采样、路径数据、头像或坐标投影。
- 世界空间伤害跳字由 `AReEchoDamageNumberActor` 硬引用 `/Game/ReEcho/Fonts/DamageNumbers/F_DamageNumber_MFYuYue_Font`，保证 Cook 收集；对应 OTF 与授权说明归档在 `Content/SourceArt/UI/CombatHud/DamageNumbers/`，只允许用于非商用伤害数字，不得当作全局 UI 字体复用。Enemy Presentation 只读 `FReEchoDamageEvent::ReactionBehaviorId` 选择反应色：Vaporize `#A5CBF3`、Conduct `#EBC02C`、Burn `#E86A12`、Growth `#92C039`、Enhance `#F1B84C`；无反应标识时回退到既有元素色/物理白色，不重新推导反应。跳字数值读取同一事件中生命钳制前、已经过伤害规则修正的 `RawDamage`，而不是实际扣血 `AppliedDamage`；因此低血击杀仍显示本次原本应造成的完整伤害，生命和死亡结算不变。暴击只放大数字本体，不创建下划线或其他附属字形；视觉倍率由 `/Game/ReEcho/UI/CombatHud/BP_ReEchoDamageNumber` 的 `Damage Number|Critical > Critical Size Scale` 独立配置，不读取玩法 `CriticalMultiplier`。该蓝图的 `Damage Number|Animation` Class Defaults 继续负责持续时间、上漂速度和起止缩放，缺失时才回退原生 Actor 默认值。跳字生命周期内保持不透明，只通过缩放表现变化，到期直接销毁。Actor 继续使用专用 `M_DamageNumberTextOpacity` 保留 UE 默认文字材质的距离场字形重建；`scripts/ue/author_damage_number_material.py` 可幂等重建该材质并以真实 SM5 编译错误为失败，`scripts/ue/author_damage_number_blueprint.py` 负责创建并验证调参蓝图。该动态加载蓝图目录必须由 Packaging AlwaysCook 收集。
- 世界空间元素反应字由 `AReEchoElementReactionPopupActor` 消费既有 `FReEchoElementReactionResolvedEvent`：Burn/Vaporize/Growth/Conduct/Enhance 分别映射灼烧/蒸发/生长/导电/强化透明图，只在权威 `PrimaryTarget` 上方生成一次，不按 Growth 影响列表或 Conduct 连锁节点重复。`BP_ReEchoElementReactionPopup` 的 `Element Reaction Popup|Animation/Layout` Class Defaults 调整持续时间、世界尺寸、上浮高度、渐隐曲线和缩放；独立半透明材质动态接收 `ReactionTexture` 与 `Opacity`。纹理、材质或蓝图缺失只跳过该次表现或回退原生 Actor，不改变元素反应结算。
- Plan110 正式商店以 `WBP_ReEchoInventoryShopScreen > DesignerShopPresentationCanvas` 和根级 `DesignerLoadoutCanvas` 为 `1920×1080` 位置权威。前者直接持有正式木框、货币/刷新、3 个配件报价、3 个卡组报价、中部纸张、装配树和保存离店美术；后者直接持有 `DesignerWeaponPanel`、`DesignerShopClock`、`DesignerWeaponInteractionButton`、3 个符文槽和 12 个卡牌槽。报价卡、文字、按钮与全部装配槽均是 Canvas 可调节点；`UReEchoInventoryShopWidget` 只按稳定名称写真实文本、价格、图标、Tooltip、显隐和启用状态，并把最多 24 张已拥有卡牌按两页切片投影到同一组 12 个作者化槽位，不覆盖位置/尺寸。配件/符文说明的命中面是整个 `DesignerPartOfferCard*`，卡组说明的命中面是整个 `DesignerPackOfferCard*` / `DesignerPackOfferBase*` 范围；购买按钮仍只负责点击命令，不得缩小说明的悬浮命中区。旧 `Overlay_0` / `Overlay_1` 只保留必要逻辑契约，其旧底板、标题、店员、时钟和槽位素材已从 WBP 层级删除。
- 进入商店时，`ReEchoGameMode` 将 `CurrentBuild.CharacterId` 显式传给 `UReEchoInventoryShopWidget`；Widget 在本地展示副本中静默合成 `CHARACTER_CARD_<CharacterId>`，使用角色 CSV 的正式名称、被动描述和角色 Icon，仅投影到右侧第一个已拥有卡槽，不写入 Run 背包、不触发购买或领取事务。
- 武器/符文贴图 Brush 使用源纹理尺寸；商品图标、已装备武器和槽内内容均放在居中的 `ScaleBox(ScaleToFit)` 中，外层 Button/Canvas 只持有作者化槽框几何，禁止用 Fill 把非同宽高比纹理压扁或拉长，也禁止退回默认 32px 中央小图。点击后用独立浮层列出 Run 投影的全部已拥有武器，当前项禁用，其他项只广播装备请求。武器与符文背包统一挂在响应式设计面的根级 `BackpackPopupLayer`，其 ZOrder 高于商店和回响弹层；武器背包保持原动态构造；符文背包使用 Plan161 的独立面板/条目 WBP，同样保留白边、近黑底、滚动区和等比图标，具体排版由 Designer 持有。符文槽继续填充纹理、置灰状态与 Tooltip；普通容量使用三个作者化槽，Run 投影任一非核心槽容量扩为 2 时切换到 `DesignerDualAttachmentLayout`，按 `(SlotTypeId, OccurrenceIndex)` 映射为一个核心长槽和四个独立方槽，完整呈现 `1+2+2` 而不复制卡牌状态或改写容量。构筑卡购买成功后把真实卡牌图标立即填入卡牌槽，但不重摇当前商店卡组候选。
- 商店角色被动、武器、配件、卡牌和卡组的原生 Tooltip 统一实例化 `WBP_ReEchoShopTooltip`；名称与正文分别绑定 `TitleText` / `DescriptionText`，Run 投影的 `OutcomeText` 原样填入同名文字控件，非空显示整个作者化 `OutcomePanel`，空/纯空白折叠且不留间距。主框和独立描边的“实际效果”框在同一 Designer 默认可见并有示例；C++ 不改写字体、宽度、边框、Padding 或对齐，也不按 ID/描述反推玩法结果。
- 商店角色/时钟属性 Tooltip 由 `WBP_ReEchoAttributeTooltip > AttributeRows` 及实际嵌套的 `WBP_ReEchoAttributeRow` 实例作者化；完整预览提供当前八项属性，每行 `DesignerPreview` 只供设计期使用。商店适配层保持 CSV 顺序和整数/百分比格式，投影 `FReEchoAttributeRowView` 名称/值/图标；运行时复用现有行、折叠并清空剩余行，目录扩展时才按同一行类追加，沿用首行 Slot 样式。图标只替换 Brush Resource，不重设几何。新类仍在 ReEcho 模块中；入口与说明见 `Design/UI/ReEcho_商店浮窗调整指南.md`。
- 商店/背包页以 `1920×1080` 为作者设计面：`BackgroundImage` 保持直接铺满实际视口，其余根控件及运行时商店/回响弹层由 `ResponsiveContentScale > ResponsiveContentSize > ResponsiveContentCanvas` 统一 `ScaleToFit`。低分辨率按比例缩小完整页面，超宽屏只延展背景；运行时迁移必须保留原 Canvas Slot 的锚点、偏移、自动尺寸和 ZOrder。
- `UReEchoInventoryShopWidget` 的无资产 fallback 展示 Run 提供的 `EffectivePrice/bCanPurchase`、武器/符文免费与付费剩余刷新次数、刷新禁用和额外卡牌组禁用状态；Widget 不再根据原价、折扣、碎片余额或免费商店卡牌自行重算内容物价格和购买资格。免费次数优先消费且不占用每关付费次数。主刷新只广播武器/符文刷新命令，Run 成功消费并保存后才更新这三个报价槽，卡组候选和已购状态保持不变。诅咒银行仍由 Run 分开保存非负现金与正债务，商店和玩家 HUD 只读展示二者净值，因此赊账后显示负数但购买判断不读取该展示值。
- 商店逻辑按稳定区块拆分：`ShopLogicScrollBox > ShopLogicPanel` 依次承载 `WeaponPartOfferPanel`（配件购买）、`RunItemOfferPanel`（普通商品）、`ShopControlPanel`（规则/刷新）和 `WeaponLoadoutPanel`（槽位草稿/保存）。现有 WBP 由 C++ 在根 Canvas 上提供有界、显式滚动条的商品视口，战后模式止于底部回响托盘上方；`EchoPanel` 使用独立缩放托盘与显式高 ZOrder。这些名称是后续 WBP 接入的逻辑契约，C++ 不依赖任何美术占位节点。
- 同一商店页展示数据驱动的武器槽组、兼容符文报价和装配草稿；商品购买按钮一律发送购买事务，不再兼做已拥有物品的免费重装；I 级符文允许跨刷新页重复购买。正式背包装配走独立的 `OnOwnedPartEquipRequested` 命令，备用草稿页的“保存配置”仍发送完整 PartId 集合给 Run。购买由 Run 自动填充兼容的对应空槽；已有同级装备时优先原槽合成，满槽不挤出旧件。UI不执行二次装配，沿既有购买成功后的完整只读投影更新符文图与背包份数。符文背包在 Run 投影基础上仍按当前武器类型与所点槽位做防御性过滤，长剑和镰刀共享槽名时不展示对方专属符文；关闭页面不提交草稿，已成功购买的符文所有权仍保留。
- Plan160 背包条目读取 Run 投影的 `BackpackCount`，仅展示未装备份数大于零的候选，多份显示数量；兼容旧手工视图时才按装备集合过滤。合成判断、材料消耗和原槽升级均由 Run 完成，成功后整页重取投影，未购报价保持稳定。
- UI 只消费只读摘要或事件并发送受控命令，不直接写 Run、Combat、Weapons、Enemies 或存档权威状态。
- 战后角色能力不建立专用 UI 分支：旧 Forge 标题、候选和提交命令已删除；旧存档的 Forge 阶段由 Run 迁移到普通卡牌选择，Widget 只显示正式卡牌候选。
- 构筑三选一卡面只保留正式卡牌插图、名称、说明、图标和选择交互；旧标签条、候选/选择提示、标签文字及纯色/运行时染色占位卡底已从 `WBP_ReEchoTraitCardEntry` 和 C++ 注入契约中删除。`WBP_ReEchoTraitCardEntry` 的默认正文/图片是 Designer 样例，根 `CardRootScaleBox` 等比缩放内部与正式底图一致的 `420×593` `CardRootSizeBox` 设计面；`WBP_ReEchoTraitCardChoice` 三个槽位内各放一个仅供所见即所得预览的样例实例，运行时以真实条目替换并写入数据，不改变槽位几何。Entry 的 `NameText` / `DescriptionText` 位于 `CardDesignerCanvas`，Choice 的 `TitleText` / `ConfirmButtonLabel` 位于根 Canvas，均可在 Designer 中独立拖动；运行时只写内容。底部 `ConfirmButton` 的按钮美术由 Choice WBP 的 Button Style 直接引用 `T_UI_Pause_ButtonLight`，独立标签为 `HitTestInvisible`，C++ 只管理选择索引、启用状态与确认委托；无 WBP fallback 的卡牌按钮底色保持透明。
- Plan132 把开场 Loadout 从角色/武器同页一次确认改为同一 `WBP_ReEchoLoadoutSelection` 内的两个连续阻塞阶段。Widget 只拥有当前阶段、未提交的两个稳定 ID 和键盘/手柄 Focus 候选：角色确认只切换到武器阶段，武器确认才沿既有 `OnLoadoutConfirmed(CharacterId, WeaponId)` 向 GameMode 广播一次最终组合；Run、预加载和首关切换权威不变。Plan138 将鼠标瞬时 Hover 与持久候选分离：Hover 只显示原生 Tooltip、全局缩放并隐藏 Focus 回退说明，不写 Selected ID；鼠标点击才更新 Selected/Unselected 图、条目箭头和确认对象，已点击 A 后 Hover B 不会偷换为 B。Plan154 增加两个阶段始终可用的 Designer-owned `BackButton` / `BackButtonLabel`：武器页返回只在 Widget 内切回角色页、保留角色并清空武器；角色页返回才通过 `OnBackRequested` 让 GameMode 关闭 Loadout、重开 Start Menu，返回不会启动或保存 Run。`BackButton` 与 `ConfirmButton` 均使用正式浅/深按钮资源族；用户验收后的 WBP 分别保存其明暗搭配、位置、尺寸、文字和间距，运行时 C++ 不回写几何或 Style。键盘/手柄 Focus 继续更新候选并显示页面内锚定说明。角色/武器资格与说明仍来自 CSV 快照，交付构图顺序仅是稳定 ID 的页面表现排序。Selection WBP 固定承载 `CharacterEntry0..3` 与 `WeaponEntry0..3` 八个实际 Entry 实例，运行时复用并配置它们；`StageSwitcher` 直接承载两个真实 Stage Panel，Designer 通过原生 `Active Widget Index=0/1` 即时切换角色/武器页，运行时也只切换该索引。`WBP_ReEchoLoadoutEntry` 用成对 Selected/Unselected Texture2D 表达“初始全亮 / 当前候选亮其余暗”；Entry 独立 Designer 使用 Desired Size 并默认展示箭头。`EntryRootSizeBox` 与 `PortraitSize` 的高度覆盖值由 Entry WBP 资产唯一拥有，设计期 `NativePreConstruct` 和运行时 `Configure` 均不得回填固定高度；角色/武器横向布局所需的 `390/280` 宽度仍由 Selection 注入。不同源画布的比例由 `PortraitScale=ScaleToFit`、Portrait ScaleBox Slot 双轴 Center 及每次从 Texture `GetImportedSize()` 刷新的 Brush 尺寸共同保证。每个 Entry 自己拥有 `EntrySelectionArrow`，它与 `SelectButton` 同属 `EntryVisualOverlay`，因此全局 Hover 缩放会覆盖图片、名称与箭头整个视觉组，运行时只切换箭头可见性。鼠标 Hover 复用商店的原生 Tooltip 定位机制，跟随当前条目并由 Slate 做屏幕避让。`WBP_ReEchoLoadoutTooltip` 只拥有鼠标说明框的宽度、边框、内边距、字体、字号、颜色和换行，C++ 只写入 CSV 标题/说明。整屏合成稿不作为交互贴图。正式纹理目录由 Packaging AlwaysCook 收集，单项缺图只回退既有角色/武器纹理，不改变资格或事务。
- 卡牌图标继续按稳定路径 `/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_{CardId}` 动态解析，整个 Cards 纹理目录由 AlwaysCook 收集。Plan128 后当前全部 64 张启用且可投放卡牌均有独立图标，源图保持各自交付原生尺寸；未来新增卡牌缺少独立资源时继续安全回退 `T_UI_Shop_CardIcon`，UI 不据图标存在性改变卡牌资格。
- 关闭、返回、事务拒绝、购买成功、卡牌揭示开始与装备/卸下成功的专用声音由命令结果宿主发布；普通/商店卡牌在有效点选时由卡面按钮发布一次 `UI.CardSelect`，后续确认或领取事务不重复。初次打开或成功逐槽刷新在 Widget 接收权威候选并开始揭示时发布一次 `UI.CardReveal`，装备集合未变化时不发布 `UI.Equip`。按钮基础反馈不代替事务结果，也不得让音频失败改变 UI 行为。

完整页面清单、WBP/C++ 分工、绑定控件名称、动态条目规则和人工验收要求，统一以 [ReEcho UI 修改指导](../../../Design/UI/ReEcho_UI修改指导.md) 为准；本文件不复制第二份控件契约。

### 三选一作者表现

三选一作者修订：`UReEchoTraitCardChoiceWidget` 首次写入前按控件实例捕获 `TitleText`、三个 `ShopCardRefreshText` 的作者模板；普通整数样例或显式 `{Count}/{Tier}/{Remaining}/{Cost}` 接收真实数据，其余文案/换行保持。标题不再移动，刷新按钮样式仅在新建原生 fallback 时设置。NativeConstruct、报价刷新和 Slate 重建都不能反向捕获已经填过的数字作为新模板。玩法/随机数/扣费/逐槽揭示不变；回归 `ReEcho.UI.TraitChoice.AuthoredPresentation` 覆盖作者值、动态文字、双选、重建和禁用状态。

### Plan158 血条表面

Plan158 Boss 表面补充：弧形颜色/透明度以 WBP 中用户当前微调值为准，不要求沿用初版紫红色。`BossHealthArc → Surface` 的 `Vein Strength/Width/Spacing`、`Relief Strength` 直接传入 MID；分叉血管仅是静态平面颜色纹理，整体圆润截面法线与固定左上高光/背光模拟**整条血条**凸起，两者解耦。只改剩余段 RGB，保留 Alpha、空槽和血量边界。关闭血管仍有整体凸起，双强度归零恢复平面。无新增贴图/几何/战斗状态，GPU 回归检查纹理不影响法线、参数生效、逐像素 Alpha 一致及扣血不滑纹理。

### Plan157 背板和选择页按钮状态

- 商店主说明、实际效果和属性 Tooltip 的外 Border 使用开场 `T_UI_Loadout_DescriptionPanel` 同族九宫格，内 Border 使用真实白色纹理着深色；生成器/资产审计验证真实 Brush Resource，不能仅验证颜色或控件存在。
- 外框 Content Padding 持有内层底板的留边（默认四边 3px）；`UBorderSlot::SetPadding` 会同步覆盖父 Border 的 Padding，生成器不得在挂子控件后统一清零 BorderSlot。资产审计验证父子 Padding 同步且正值；`ReEcho.UI.TooltipRendering.Backplates` 用非 NullRHI 的真实 Slate/GPU 渲染检查三块面板的四边像素，避免仅有 Brush 引用但边框被实心内板遮盖的回归。
- 属性行的 `AttributeIcon.Brush.ImageSize` 也由 WBP 持有且须非零（默认 28×28），不等同于外层 SizeBox 的占位。运行时仅换资源、不强写尺寸；生成器显式设置非零期望尺寸，资产审计和真实商店适配测试检查图片尺寸/贴图，GPU 测试填充全部八项真实属性图标并验证每张图的屏幕面积与非纯色像素。
- `ReEchoLoadoutSelectionWidget::RefreshActionButtons` 统一运行时与 Designer 的按钮状态：确认和返回以及独立标签始终可见，确认仅在当前阶段有选择且尚未提交时启用；返回在未最终提交前保持启用。确认标签跟随确认禁用，返回标签仅在 Hover 时启用其正常亮态、离开灰显；这不修改字体、颜色、几何或游戏候选。
- `WBP_ReEchoLoadoutSelection` 的 Button Style 持有明暗：确认 Disabled 暗，返回 Normal/Disabled 暗、Hovered/Pressed 亮；C++ 不改写 Style。原有角色页回主菜单、武器页回角色选择、防重复提交和点击选中契约不变。
- 验证入口为 `ReEcho.UI.Shop.TooltipBlueprints`（真实背板引用、数据及样式保留）、`ReEcho.UI.LoadoutSelection.Flow` / `.Assets`（常驻显示、禁用与 Hover 灰显、WBP 明暗状态）。

## 符文背包作者化（Plan161）

- `WBP_ReEchoRuneBackpack` 由 `ReEchoRuneBackpackWidget.*` 驱动，`BackpackRootSizeBox` 持有期望宽高；商店根级 `BackpackPopupLayer` 只挂载同一实例、自动尺寸并按期望大小避让屏幕边缘，不再向符文弹层写固定尺寸。武器背包保持原实现。
- 背板、固定标题和滚动列表都来自 WBP。`EntryList` 内置三个真实的 `WBP_ReEchoRuneBackpackEntry`，其 `ReEchoRuneBackpackEntryWidget.*` 只填写 `NameText`、`CountText`、`RuneIcon` 和 `SelectButton` 索引；图片使用作者化 ScaleToFit，原生纹理宽高比随资源变化，显示区域由 `RuneIconSize` 拥有。
- `DesignerPreview` 仅供设计期，独立条目/整个面板均使用 Desired Size。运行时复用已有条目，按同一 Entry 类追加、复制首行列表 Slot 的 Padding/Alignment/Size；多余条目清空、禁用并折叠，防止样例或旧 Tooltip 泄露。Configure/PreConstruct 不改字体、边框、Padding、行高或面板尺寸。
- 商店适配仍按当前武器、槽型、`BackpackCount` 过滤，用共用 `WBP_ReEchoShopTooltip` 填入真实说明；点击继续发出 `OnOwnedPartEquipRequested(ItemId, OccurrenceIndex)`，不购买、不消耗材料、不计算合成。关闭和切换槽位清空旧索引，重复打开复用实例且只绑定一次。
- 作者工具：`scripts/ue/author_plan161_rune_backpack.py` 仅创建缺失资产，绝不覆盖已保存微调；只读审计为 `audit_plan161_rune_backpack.py`。调参入口登记于 [商店浮窗调整指南](../../../Design/UI/ReEcho_商店浮窗调整指南.md)。回归 `ReEcho.UI.Shop.RuneBackpackAuthored` / `.RuneBackpackRouting` / `.RuneBackpackFiltersWeaponType`，真实 GPU 对比 `ReEcho.UI.RuneBackpackRendering.DesignerParity`。

## 商店最终余额订阅（Plan159）

- Run 仍独占现金与债务。运行时 GameMode 使用 `RefreshShopFromRun` 初始化/刷新商店商品、装备与规则投影；该入口首次绑定 `OnTimeShardBalanceChanged` 并读取余额初值，重复调用复用同一个 DelegateHandle，不重新写入现金缓存，也不重置已选择的卡牌页。
- 余额事件按完整外层事务提交后发布的 `Cash/Debt` 更新缓存。`RefreshCurrencyText` 是余额文字唯一写入点；保持 `TimeShards - TimeShardDebt` 文案，只在内容不同时 SetText，不改 WBP 字体、颜色、对齐或几何。
- `RefreshPurchaseAvailability` / `RefreshShopControlState` 从 Run 最新只读投影更新既有购买与刷新按钮。该局部路径不调用整页 `Refresh`、不重建控件/Tooltip、不重抽报价、不关闭武器/符文背包、不切卡牌页。现金与债务改变但净额相同仍需要同步资格。
- 商店上方打开已付款选卡层时保留订阅，所以付款、逐槽付费刷新和还债都能立即反映到底层商店。关闭选卡层不靠补刷新余额；商店经 `RemoveFromParent` 关闭时立即解绑（即使 Slate 仍持有引用），`NativeDestruct` 再作幂等兜底，重新打开读取最新初值并只订阅一次。旧 `ShowInventory` / `ShowShop` 数值快照入口只保留独立初始化兼容，不作为 GameMode 的商店余额推送路径。
- GM 加/设碎片改走 Run 窄命令，不再触发商店整页刷新。购买、换装、主刷新、授卡后的内容更新仍保留；卡包付款后也更新待选状态（包括零价无余额事件的情况），不能因统一余额事件而删掉非余额投影。
- 验证：`ReEcho.Shop.BalanceTransactions`、`ReEcho.Shop.BalanceOfferCommands` 与 `ReEcho.UI.Shop.BalanceSubscription`，源码为 `Source/ReEcho/Private/Tests/ReEchoShopBalanceTests.cpp`；同时回归既有 `ReEcho.Shop` / `ReEcho.UI.Shop`、Save、Trait。人工检查已付款选卡返回、第二页和弹窗不被纯余额更新干扰。

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
| 修改关末倒计时/序列过渡 | `ReEchoGameMode` 结算状态、`Presentation/Scene/ReEchoArenaCameraActor.*`、`ReEchoEncounterTransitionWidget.*` | `M_PP_EncounterCountdownGhost_V4`、`Content/Movies/EncounterTransition/{EncounterEndToCardChoiceV2.mov,CardChoiceToShop.mov,Stage01To02.mov}`、`S_Stage01To02`、MediaAssets/HAPMedia |
| 修改商店卡牌规则展示 | `ReEchoInventoryShopWidget.*` | `ReEchoGameMode::RefreshShopPresentation`、`UReEchoRunSubsystem` 商店命令 |
| 修改正式商店构图 | `WBP_ReEchoInventoryShopScreen` 的两个 `Designer*Canvas` | `Content/SourceArt/UI/InventoryShop/Plan110/README.md`、`scripts/ue/audit_plan110_formal_shop_ui.py` |
| 修改页面创建、层级和实例 | `ReEchoUIManagerSubsystem.*` | `UI/Framework/ReEchoUIScreenTypes.h` |
| 修改焦点、输入或暂停流程 | `UI/Framework/ReEchoUIFlowCoordinatorSubsystem.*` | `ReEchoGameMode` 类型化端点 |
| 修改暂停/退出确认表现 | `WBP_ReEchoRestart`、`ReEchoRestartWidget.*` | `ReEchoGameMode` 的暂停退出端点 |
| 修改正式存档回溯页 | `WBP_ReEchoStartMenu` 的 `SaveRollbackPanel` | `ReEchoStartMenuWidget.*`、`ReEchoRunSubsystem` 三槽摘要、`scripts/ue/audit_plan145_start_menu_tree.py` |
| 修改音频设置页 | `WBP_ReEchoSettings` 的绑定契约 | `ReEchoSettingsWidget.*`、`MOD-ReEchoAudio.md` |
| 修改通用按钮反馈 | `UI/Framework/ReEchoButtonVisualFeedback.*` | `ReEchoUIFlowCoordinatorSubsystem.*`、`FReEchoAudioEvents`、具体结果宿主 |

Plan45 的运行时美术消费保持在 WBP 表现层：Start Menu、Settings、Restart、Trait Card、Inventory/Shop 和 Player/Encounter HUD 引用分页纹理目录；原生 Widget 仍拥有状态、Delegate、显隐和生命周期。

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
- Plan110 正式商店回归：`scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.Shop.AuthoredLayoutHosts` 验证正式 authored 报价/装配槽绑定、现有背包与回响入口、购买后即时投影，以及第 13 张卡进入第二页首槽并可用左右箭头往返 `1/2`、`2/2`；`scripts/ue/Run-EditorPythonLocked.ps1 -ScriptPath scripts/ue/audit_plan110_formal_shop_ui.py` 验证正式控件仍可在 Designer 调整且整屏参考图未成为运行时纹理。
- Plan147 回响弹窗回归：同一 `ReEcho.UI.Shop.AuthoredLayoutHosts` 聚焦自动化验证 `G_3_02` 点击入口、正式弹窗开启和单一时间锚点语义；`scripts/ue/Run-EditorPythonLocked.ps1 -ScriptPath scripts/ue/audit_plan147_echo_storage_popup.py` 验证独立主/确认层级、1920×1080 作者面、当前锚点/本场回响 Designer 默认文案、正式底板和浅/深按钮状态，并断言旧容量、三槽、逐槽替换/选择控件不存在；整屏 SVG 只保留为 SourceArt 参考。
- Plan149 双重武器槽回归：`ReEcho.UI.Shop.AuthoredLayoutHosts` 同时验证扩容后五个 authored 槽分别显示各 occurrence，`scripts/ue/audit_plan149_dual_weapon_slot_ui.py` 验证一个 `89x175` 核心长槽与四个 `89x89` 非核心方槽仍存在于 WBP 且可在 Designer 调整。
- 卡牌商店投影固定为1/2/3级三个卡组入口；入口不显示具体卡牌 icon，底部按钮按状态显示“购买 · 折扣后卡组价 / 继续选择 / 已购”。可购买入口发布 Tier 命令后，GameMode 先调用 Run 的卡组付款事务并立即 `SaveRun`，成功才在 ZOrder 98 的 `BuildChoice` 层打开复用的选择页；已付款待选入口不检查余额或额外购卡门槛，直接继续同一候选页且不重复收费。页内最多三张同级候选，不显示单卡价格，并提供“返回商店”；最终选择调用独立领取事务。商店已付款与战后免费三选一都在每张实际候选下方显示独立刷新按钮、该槽剩余次数和价格，并通过统一 `OnCardSlotRefreshRequested(SlotIndex)` 发送稳定槽位；GameMode 按当前页面调用对应 Run 原子命令后重注入同一层，Widget 不自行抽牌或扣费。成功逐槽刷新只将该稳定槽位映射到当前可见卡牌并重播它的揭示，另外两张卡与页面指针保持已揭示状态。领取/刷新失败恢复原选择；取消仅关闭页面，已付款状态不退款且可继续。
- Plan91 武器背包扩展：Widget 不持有武器所有权，也不把换装伪装成购买；`OnWeaponEquipRequested` 交给 GameMode 调用 Run 事务。成功后整页重取只读投影，使当前武器图、武器名和兼容符文槽同时更新，但独立的武器/符文与卡牌刷新序列都不变，因此报价不重摇。
- Plan91 稳定页中的已拥有武器仍保留原槽身份和价格，但 Widget 必须以 Run 的 `OwnedWeapons` 投影为“已获得”并禁用购买；不得依赖本次页面内的临时点击记录判断所有权。
- Plan91 的任意商品购买成功后由 GameMode 重新注入完整商店只读投影，Widget 不再局部拼接 `EquippedParts`。这保证新购符文立即进入 `OwnedParts` 背包视图，同时依赖 Run 的稳定页缓存保持其余报价不变。
- 武器/符文购买按钮由 GameMode 调用 Run 的 `PurchaseShopItemDetailed`；卡组入口发送 Tier 并分别消费 `PurchaseShopCardPackDetailed` / `ClaimPaidShopCardChoice` 的结构化结果。Widget 只根据只读投影控制购买与继续按钮，不自行修改扣费、付款或所有权状态。
- 人工检查：按 UI 修改指导执行页面导航、焦点、DPI、可读性和交互验收。

## Plan152 彩蛋卡牌表现

- 彩蛋卡玩法 Tier 为 0，但 Run 向三选一和已拥有卡槽显式投影 `PresentationTier=3`；Widget 只据展示 Tier 复用三级卡底，不能把彩蛋重新归入三级玩法池。
- 九张图标固定解析 `/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_G_4_1`～`G_4_9`，源图位于对应 `Content/SourceArt/UI/Cards/Icon/`；缺图只走既有安全回退，不改变投放资格。
- 随机、累计和多项效果继续使用现有 Tooltip 第二块“实际效果”面板；`RandomDetails` 的目标/数值数组由 Run 统一翻译，UI 不按 `G_4_*` ID 或中文描述自行推断。
