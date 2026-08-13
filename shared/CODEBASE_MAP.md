# ReEcho 代码库地图

最后核验：2026-08-13。本文件是代码检索的最短权威路由索引，用于说明行为位于何处；约束仍以 `PROJECT_RULES.md` 为准。交付事实来自源码、测试、已关闭 Plan 和 Git，不另建重复状态快照。

## 最小检索流程

1. 先读 `AGENTS.md`，再读本文件。
2. 只读取**任务路由**中匹配请求的行。
3. C++ 工作先读匹配的 `Public/.../*.h`，再读对应 `Private/.../*.cpp`。
4. 编辑前读取 `shared/PROJECT_RULES.md`；调试时只读取 `shared/LESSONS.md` 的相关章节。
5. 除非诊断构建输出，不搜索 `Binaries/`、`Intermediate/`、`Saved/` 或生成文件。
6. 不得从旧 `Content/Data/*.json` 推断运行时行为；CSV 是目标运行时数据源，在后续领域 Plan 完成前 JSON 仅用于迁移。

常用首批命令：

```powershell
rg --files Source\ReEcho Config Content\Data docs scripts shared
rg -n "SymbolName" Source\ReEcho
```

## 仓库布局

| 路径 | 用途 | 读取时机 |
|---|---|---|
| `ReEcho.uproject` | UE 5.8 关联和启用插件 | 模块/插件/Editor 集成 |
| `Source/ReEcho/` | 主玩法运行时模块 | 玩法或 UI 代码 |
| `Source/ReEchoAudio/` | 独立音频运行时模块 | 语义音频 API、总线、状态通道、目录加载、播放策略或音频自动化 |
| `Config/` | 地图、GameMode、平衡和输入映射 | 启动、控制或调优 |
| `Design/Data/ReEchoData.xlsx` | 权威策划 XLSX 工作簿；machine Table 生成运行时 CSV | 数据编写、XLSX 迁移或 Plan25 检查 |
| `Design/Data/ReEchoData使用说明.md` | 可编辑 Table、字段规则、CSV 生成、错误和提交的中文策划指南 | 修改工作簿中的生产平衡/配置前 |
| `Design/Data/ReEchoData策划验收清单.md` | 策划 QA 接入、隔离克隆、XLSX/CSV/PIE 验收、恢复、反馈模板和 AI 设置提示 | 首次 Plan25 策划验收或将设置交给策划 AI 前 |
| `Content/Data/` | 运行时 CSV 契约及只读迁移 JSON | 数据契约、fixture、卡牌、角色、敌人、遭遇或平衡来源 |
| `Content/ReEcho/Materials/` | 序列化项目材质 | 视觉资产引用 |
| `Content/ReEcho/Textures/Characters/` | 已 cook 的 2D Actor 和阴影纹理 | 玩家/Echo/敌人 Billboard 视觉 |
| `Content/SourceArt/Characters/` | 可评审 PNG 源文件，包括 MushroomGirl 动画帧和 Sprite Sheet | 重新生成或扩展 2D Actor 资产 |
| `scripts/ue/` | 安装版引擎发现、构建、预构建 Editor 包校验、自动化、可复现 Windows Shipping 打包和资产导入 | 编译/测试/打包、拉取即开交付或资产导入流程 |
| `Binaries/Win64/ReEchoEditor.prebuilt.json` | 精选 Editor 模块包契约、源码指纹和二进制哈希 | 直接打开失败或程序发布评审 |
| `scripts/data/sync_xlsx_to_csv.py` | 确定性 XLSX Table 到 UTF-8 CSV 生成器、检查模式和事务发布 | 数据编写同步或生成 CSV 漂移 |
| `scripts/validate_project.py` | CSV、旧 JSON 和工作流静态校验 | 数据/工作流变更 |
| `docs/` | 面向人的架构和 MCP 指南 | 工具集成或熟悉项目 |
| `shared/` | AI 权威、规则、路由和协调 | 每个 AI 任务 |
| `plans/` | 有明确范围的实现 Plan 和执行记录 | Plan 专项工作 |

生成目录（`Intermediate`、`Saved`、Derived Data 和大部分 `Binaries`）属于本地输出。只有 `Binaries/Win64/ReEchoEditor.prebuilt.json` 声明的文件为“拉取即开”交付而跟踪。

## 运行时组成

`/Game/Level00` 是 Editor 和打包游戏启动地图。`AReEchoGameMode` 在该世界中创建运行时竞技场和玩法 Actor。

```text
DefaultEngine.ini
  -> /Game/Level00
  -> AReEchoGameMode::StartPlay
       -> CreateArena（有边界的隐藏碰撞竞技场、相机对齐 unlit 背景、灯光）
       -> 生成 AReEchoEncounterDirector
       -> 显示 UReEchoStartMenuWidget
            -> 新游戏：UReEchoRunSubsystem::StartRun
            -> 继续：加载安全检查点或暂停的遭遇
       -> BeginNextEncounter 或 ResumeSavedEncounter
            -> 重置玩家并启动 UReEchoRecorderComponent
            -> 有历史时生成先前 AReEchoEchoActor
            -> 生成敌人组合
            -> 启动确定性遭遇时钟

遭遇固定步长
  -> 记录玩家位置
  -> 推进 Echo 回放

全部敌人死亡或计时结束
  -> 完成录制
  -> UReEchoRunSubsystem::CompleteEncounter
  -> 下一遭遇（共六个）

Esc -> 暂停菜单 -> 退出
  -> 确认：继续游戏或保存并退出
  -> 遭遇存档捕获时钟、玩家、活跃录制和存活敌人
```

## 运行时模块地图

| 区域 | 主要类型 | 文件 | 职责 |
|---|---|---|---|
| 编排 | `AReEchoGameMode` | `Public/ReEchoGameMode.h`、`Private/ReEchoGameMode.cpp` | 运行时有界竞技场/相机对齐 unlit 背景、六个遭遇、可配置确定性边缘敌人生成、清理、录制交接和 HUD 文本 |
| UI 框架 | `UReEchoUIManagerSubsystem`、`UReEchoUIFlowCoordinatorSubsystem`、`EReEchoUIScreen` | `UI/Framework/*`、`UI/ReEchoUIManagerSubsystem.*`、`ReEchoGameMode.*` | 中央 WBP 类注册、类型化活跃屏幕生命周期、viewport 层、焦点/输入策略、暂停安全屏幕切换；GameMode 保留玩法决定和委托端点 |
| 遭遇时钟 | `AReEchoEncounterDirector` | `Encounter/ReEchoEncounterDirector.*` | 可暂停 60 Hz 固定步、准备阶段、超时/结束委托 |
| Billboard 屏幕尺寸 | `ReEchoBillboardScreenScale` | `Private/Graybox/ReEchoBillboardScreenScale.h` | 共享相机深度补偿，保持玩家/Echo/敌人投影尺寸稳定 |
| 玩家 | `AReEchoPlayerPawn` | `Player/ReEchoPlayerPawn.*` | 地图边缘限制的跟随相机、较宽玩家竞技场限制、WASD、中心对齐根 Capsule、跟随鼠标的水平 Sprite 朝向、四种可配置 NewCast 角色纹理、手动攻击和纯视觉 2D 运动 |
| GAS 战斗 | `UAbilitySystemComponent`、`UReEchoCombatAttributeSet`、原生 GameplayEffect/tag 和玩家能力 | `AbilitySystem/*`、`Player/ReEchoPlayerPawn.*`、`Combat/ReEchoCombatantComponent.*`、`Graybox/ReEchoEnemyActor.*` | 玩家/敌人运行时属性、基于 Effect 的伤害/治疗、tag 驱动激活、冷却提交和 AbilityTask 持续攻击；CombatantComponent 是面向旧代码的适配器 |
| 投射物 | `AReEchoProjectileActor` | `Graybox/ReEchoProjectileActor.*` | 可见球体飞行、扫掠路径与 Capsule 命中检测、伤害交付 |
| 玩家武器 | `AReEchoWeaponActor`、`FReEchoCsvWeaponRow`、`EReEchoInputSlot` | `Weapons/ReEchoWeaponActor.*`、`Player/ReEchoPlayerPawn.*`、`UI/ReEchoLoadoutSelectionWidget.*`、`Data/ReEchoWeaponCsvReader.*` | CSV 驱动具体 WeaponId、仅兼容 InputSlot 映射、有序攻击步骤、开局可选装载，以及按稳定 WeaponId 锁定本局 recorder/Echo 初始化 |
| 剑弧 VFX | `AReEchoSwordArcActor` | `Graybox/ReEchoSwordArcActor.*`、`Weapons/ReEchoWeaponActor.cpp` | 每次挥剑生成分层半透明月牙，并在短生命周期内淡出 |
| 敌人 | `AReEchoEnemyActor` | `Graybox/ReEchoEnemyActor.*` | Grunt/shield/bomber/final-Boss 属性、六种轮换 2D grunt 变体和 2D final-Boss Billboard、Capsule 伤害体积、追击/接触伤害及纯视觉攻击/受击/死亡运动 |
| Echo | `AReEchoEchoActor` | `Graybox/ReEchoEchoActor.*` | 历史移动和已录武器变化回放、自动共享武器攻击、半透明脉冲幽灵材质及纯视觉 2D 运动 |
| Echo 轨迹 | `AReEchoTrajectoryActor` | `Graybox/ReEchoTrajectoryActor.*` | 仅将活跃 Echo 的简化历史位置投影到地面，形成一条世界固定半透明路线 |
| 战斗 | `UReEchoCombatantComponent`、`ReEchoElementReaction` | `Combat/ReEchoCombatantComponent.*`、`Combat/ReEchoElementReaction.*` | 共享属性、HP、格挡、伤害/死亡委托和 CSV 驱动元素反应执行 |
| 命中 VFX | `ReEchoAttackEffects`、`AReEchoHitImpactActor` | `Graybox/ReEchoAttackEffects.*`、`Graybox/ReEchoHitImpactActor.*` | 仅在实际伤害非零后生成透明 HitStarburst 平面 |
| 伤害数字 | `AReEchoDamageNumberActor` | `UI/ReEchoDamageNumberActor.*`、伤害调用者 | 面向相机的浮动 `-N` 文本，显示玩家/敌人实际承受伤害 |
| 录制 | `UReEchoRecorderComponent` | `Recording/ReEchoRecorderComponent.*` | 20 Hz 位置和成功主动技能事件 |
| 回放 | `UReEchoPlaybackComponent` | `Recording/ReEchoPlaybackComponent.*` | 插值历史位置和跨过的技能事件 |
| 局内状态/存档 | `UReEchoRunSubsystem`、`UReEchoRunSaveGame` | `Run/ReEchoRunSubsystem.*`、`Run/ReEchoRunSaveGame.h` | 局内阶段、遭遇索引、CSV 支持 build、背包、待定/最新/存储 Echo 状态、稳定 GUID 回放选择、v4 迁移、安全检查点和显式暂停遭遇持久化 |
| CSV 数据注册表 | `FReEchoCsvDataRegistry` | `Data/ReEchoCsvDataRegistry.*`、`Data/ReEchoWeaponCsvReader.*`、`Content/Data/*.csv`、`Design/Data/ReEchoData.xlsx`、`scripts/data/sync_xlsx_to_csv.py` | 版本化 CSV manifest 加载、XLSX 编写同步、验证、行为/Effect/公式/攻击模式允许列表和不可变运行时快照 |
| 共享类型 | `FReEcho*`、`EReEcho*` | `Core/ReEchoTypes.*` | 属性、build 快照、录制样本/事件、元素、阶段和暂停遭遇/敌人运行时状态 |
| 平衡配置 | `UReEchoBalanceSettings` | `Core/ReEchoBalanceSettings.h`、`Config/DefaultGame.ini` | 遭遇/固定步/录制/全局原型值 |
| 生命 UI | `UReEchoPlayerHudWidget`、`AReEchoHealthBarActor`、`UReEchoHealthBarWidget` | `UI/ReEchoPlayerHudWidget.*`、`Graybox/ReEchoHealthBarActor.*`、`UI/ReEchoHealthBarWidget.*` | 左上角玩家头像/实时生命 HUD；面向相机的世界血条仅用于敌人 |
| 遭遇 HUD | `UReEchoEncounterHudWidget` | `UI/ReEchoEncounterHudWidget.*`、`ReEchoGameMode.*` | 右上角当前遭遇和剩余时间；最后五秒变红 |
| 背包/商店 | `UReEchoInventoryShopWidget`、`UReEchoRunSubsystem` | `UI/ReEchoInventoryShopWidget.*`、`Run/ReEchoShopCatalog.h`、`Run/ReEchoRunSubsystem.*`、`ReEchoGameMode.*` | 使用提供的全屏美术显示 B/M 菜单；Time Shard 购买进入本局背包并立即修改 build 快照 |
| 玩家/Echo 属性 | `UReEchoStatsWidget` | `UI/ReEchoStatsWidget.*`、`ReEchoGameMode.*`、`Graybox/ReEchoEchoActor.*` | Tab 暂停双列实时属性，使用导入的模糊发条背景；支持没有活跃 Echo 的局 |
| 天气场景 | `UReEchoWeatherWidget` | `UI/ReEchoWeatherWidget.*`、`ReEchoGameMode.*`、`ReEchoBalanceSettings.h` | 可按遭遇配置雨线和以玩家/Echo 为中心的战争迷雾，作为输入透明屏幕空间表现渲染 |
| GM/调试命令 | `AReEchoGameMode` exec 函数 | `ReEchoGameMode.*`、`docs/GM_COMMANDS.md` | 开发控制台状态、治疗、Time Shard、天气覆盖和清敌命令；Shipping 中拒绝 |
| 开始/继续/装载 UI | `UReEchoStartMenuWidget`、`UReEchoLoadoutSelectionWidget` | `UI/ReEchoStartMenuWidget.*`、`UI/ReEchoLoadoutSelectionWidget.*`、`Run/ReEchoRunSaveGame.h`、`ReEchoGameMode.*` | 阻塞式启动选择，随后在遭遇 1 前选择 CSV 支持的角色和初始武器；继续会恢复已保存当前装载 |
| 暂停/重启 UI | `UReEchoRestartWidget` | `UI/ReEchoRestartWidget.*`、`ReEchoGameMode.*` | Esc 暂停层、恢复、全关卡重启、两步保存退出确认、死亡屏幕和最终 Boss 胜利操作 |
| 特质选择 UI | `UReEchoTraitCardChoiceWidget` | `UI/ReEchoTraitCardChoiceWidget.*`、`Run/ReEchoRunSubsystem.*`、`ReEchoGameMode.*` | 居中动画三选一及当前 Time Shard；只有待定 offer 修改 build，然后在下一遭遇前打开现有商店 |
| 测试 | 录制/GAS/局内/存档/商店/特质自动化 | `Private/Tests/*` | 录制插值/时间线、GAS 结构、最终 Boss 门禁、安全/暂停存档快照、商店购买和确定性待定特质回归 |

表中路径相对于 `Source/ReEcho/Public` 或 `Source/ReEcho/Private`。

## 当前玩法契约

- 玩家：局前角色与 CSV 武器选择、本局锁定武器装载、2D Billboard 角色、WASD 移动、鼠标左键/J 普攻和 Q/Space 强力射击。
- Echo：2D Billboard 回放 Actor，按历史移动，并通过共享武器实现攻击，包括已录武器变化回放。
- 投射物飞行没有 Niagara。只有实际伤害大于零时才显示透明 HitStarburst 命中效果。
- 所有敌人类型使用 2D Billboard 视觉；旧 cube/plane/cone 降级渲染已移除。Shield 敌人仍有实现，但暂时排除在遭遇组合外。
- 敌人受击反馈：短硬直、背离来源击退和衰减横向抖动；被格挡命中不触发。
- 玩家、Echo 和 2D 敌人使用现有静态纹理做 Sprite 本地 bob、squash、lunge 和 recovery；敌人死亡会在销毁前短暂缩小/倒下。
- 玩家和敌人使用紧凑、面向相机、锚定在 2D 视觉上方的世界血条。
- Esc 打开恢复/重启/退出暂停菜单。退出需确认且保存成功；Continue 或 Esc 取消并恢复。死亡和 Boss 胜利显示重启/退出；Shipping 调用平台 QuitGame。
- 清除全部存活敌人立即结束遭遇；超时作为兜底。
- 自动 Echo 攻击不序列化。录制保存玩家位置及成功主动技能事件。

## 配置、数据与资产

| 事项 | 来源 |
|---|---|
| 默认地图/GameMode | `Config/DefaultEngine.ini`；运行时全局/遭遇平衡在 `Config/DefaultGame.ini` |
| WASD、鼠标/J 和 Q/Space | `Config/DefaultInput.ini` |
| MCP 端点/Editor 偏好 | `Config/DefaultEditorPerProjectUserSettings.ini`、`.codex/config.toml` |
| Windows 覆盖 | `Config/Windows/WindowsEngine.ini` |
| 运行时 CSV 契约 | `Content/Data/*.csv`、`Source/ReEcho/Data/*` |
| 仅迁移策划注册表 | `Content/Data/*.json` |
| Echo 幽灵材质 | `Content/ReEcho/Materials/M_EchoGhost.uasset` |
| 模块依赖 | `Source/ReEcho/ReEcho.Build.cs` |

CSV 当前包含模块启动时由 `FReEchoCsvDataRegistry` 加载的运行时基础 manifest/schema/smoke 表、权威角色/build 表、元素/状态/反应表和武器领域表。当前可玩角色基础属性/默认武器、六卡特质抽取池、锻造选择、晋升角色桶、卡牌数值 Effect、战斗元素、必要状态、六种有序反应、具体武器、有序攻击步骤、槽位 profile、部件和部件 Effect 均从 CSV 读取；burn DOT 状态、vaporize 平方伤害、按 ReactionEfficiency 缩放半径的 growth 附着、配置半径 conduct 连锁和 enhancement 阻塞通过 `ReEchoElementReaction` 执行。旧 JSON 仍覆盖遭遇、敌人和全局平衡；已迁移角色/卡牌/元素/状态/反应/武器 JSON 仅作迁移评审材料。

## 任务路由

| 任务关键词 | 优先读取 | 通常还需读取 |
|---|---|---|
| 启动、竞技场、轮次、生成、清敌进下一关 | `ReEchoGameMode.*` | `EncounterDirector.*`、`RunSubsystem.*`、`DefaultEngine.ini` |
| 输入、移动、玩家攻击 | `Player/ReEchoPlayerPawn.*` | `DefaultInput.ini`、`ReEchoProjectileActor.*` |
| 2D Sprite 动画、帧导入、攻击/受击/死亡反馈 | `Presentation/Animation2D/*`、`Player/ReEchoPlayerPawn.*`、`Graybox/ReEchoEnemyActor.*`、`Graybox/ReEchoEchoActor.*` | `scripts/ue/repair_2d_animation_flipbooks.py`、`scripts/ue/import_mushroomgirl_frames.py`、`Content/2DAnim/`、`Content/SourceArt/Characters/MushroomGirl/` |
| GAS、能力、属性、Effect、冷却、tag | `AbilitySystem/*`、`Player/ReEchoPlayerPawn.*` | `docs/GAS_ONBOARDING.md`、`Combat/ReEchoCombatantComponent.*`、`Graybox/ReEchoEnemyActor.*`、`Weapons/ReEchoWeaponActor.*`、GAS 自动化测试 |
| 鼠标瞄准/玩家朝向/相机 | `Player/ReEchoPlayerPawn.*` | `GameMode::RestoreGameInput`、`DefaultInput.ini` |
| 初始武器选择/本局锁定武器/剑/近战/元素反应 | `UI/ReEchoLoadoutSelectionWidget.*`、`Weapons/ReEchoWeaponActor.*`、`Weapons/ReEchoWeaponRuntime.*`、`Data/ReEchoWeaponCsvReader.*`、`Combat/ReEchoElementReaction.*`、`Data/ReEchoElementReactionCsvReader.*` | `Graybox/ReEchoProjectileActor.*`、`Graybox/ReEchoEnemyActor.*`、`Player/ReEchoPlayerPawn.*`、`Content/Data/weapons.csv`、`Content/Data/weapon_types.csv`、`Content/Data/attack_steps.csv`、`Content/Data/slot_profiles.csv`、`Content/Data/parts.csv`、`Content/Data/part_effects.csv`、`RunSubsystem::StartRun`、`RunSubsystem::TryEquipParts`、`FReEchoBuildSnapshot::EquipmentBaseStats/EquipmentBaseRuleFlags/EquippedParts`、`FReEchoBuildSnapshot::WeaponDomainRevision` |
| 子弹速度/尺寸/颜色/命中 | `Graybox/ReEchoProjectileActor.*` | 玩家/Echo 调用者、`EnemyActor::ReceiveGrayboxDamage` |
| 敌人 AI、类型、Shield、Bomber、Boss | `Graybox/ReEchoEnemyActor.*`、`Graybox/ReEchoBomberRules.*` | `CombatantComponent.*`、`ReEchoBalanceSettings.h`、`DefaultGame.ini`、命中效果 |
| 伤害、HP、格挡、死亡 | `Combat/ReEchoCombatantComponent.*` | 伤害调用者和生命 UI |
| 命中 VFX 或打击感 | `Graybox/ReEchoAttackEffects.*`、`Graybox/ReEchoEnemyActor.*` | Niagara 插件/材质路径 |
| 浮动伤害文本 | `UI/ReEchoDamageNumberActor.*` | 每个 `ApplyFinalDamage` 调用者，目前为 `Graybox/ReEchoEnemyActor.cpp` |
| Echo 外观/攻击/回放/本局锁定武器 | `Graybox/ReEchoEchoActor.*`、`Recording/ReEchoRecorderComponent.*`、`Recording/ReEchoPlaybackComponent.*` | 固定的 `RunSubsystem::GetRunDataSnapshot()`、`Recording.BuildSnapshot.WeaponId`、仅初始化 `Weapons/ReEchoWeaponActor::SelectWeaponById`、`M_EchoGhost.uasset` |
| Echo 路线/轨迹/拖尾 | `Graybox/ReEchoTrajectoryActor.*` | `Core/ReEchoTypes.h`、`Graybox/ReEchoEchoActor.*`、`M_EchoGhost.uasset` |
| 录制定性/插值 | `Core/ReEchoTypes.*`、`Recording/*` | `EncounterDirector.*`、录制测试 |
| Echo 存储/回放选择、局内阶段、存档和商店 | `Run/ReEchoRunSubsystem.*`、`Run/ReEchoRunSaveGame.h` | `Core/ReEchoTypes.*`、GameMode |
| CSV 运行时数据、Schema、fixture 和 XLSX 编写 | `Data/ReEchoCsvDataRegistry.*`、`Private/Data/*CsvReader.*` 下领域 reader | `Design/Data/ReEchoData.xlsx`、`Design/Data/ReEchoData.migration.md`、`scripts/data/sync_xlsx_to_csv.py`、`scripts/data/test_sync_xlsx_to_csv.py`、`Content/Data/README.md`、`Content/Data/*.csv`、`validate_project.py`、数据自动化测试 |
| 玩家头像/生命 HUD、敌人血条 | `UI/ReEchoPlayerHudWidget.*`、`UI/ReEchoHealthBarWidget.*`、`Graybox/ReEchoHealthBarActor.*` | `Player/ReEchoPlayerPawn.*`、`ReEchoGameMode.*`、`CombatantComponent.*` |
| 遭遇倒计时/当前关卡 HUD | `UI/ReEchoEncounterHudWidget.*` | `ReEchoGameMode.*`、`EncounterDirector.*`、`RunSubsystem.*` |
| 雨、雾、天气场景 | `UI/ReEchoWeatherWidget.*`、`ReEchoGameMode.*` | `Core/ReEchoBalanceSettings.h`、`DefaultGame.ini` |
| 背包、商店、存储、Time Shard | `UI/ReEchoInventoryShopWidget.*`、`Run/ReEchoShopCatalog.h`、`Run/ReEchoRunSubsystem.*` | `ReEchoGameMode.*`、`Player/ReEchoPlayerPawn.*`、`DefaultInput.ini`、导入 UI 纹理 |
| 玩家属性、Echo 属性、Tab 面板 | `UI/ReEchoStatsWidget.*`、`ReEchoGameMode.*` | `Graybox/ReEchoEchoActor.*`、`Combat/ReEchoCombatantComponent.*`、`Player/ReEchoPlayerPawn.*`、`DefaultInput.ini` |
| 暂停/死亡/重启/退出 UI | `UI/ReEchoRestartWidget.*` | `ReEchoGameMode.*`、`PlayerPawn::TogglePauseMenu`、`DefaultInput.ini` |
| 特质卡/卡牌选择/角色晋升/角色 build | `UI/ReEchoTraitCardChoiceWidget.*`、`Run/ReEchoRunSubsystem.*`、`Run/ReEchoCharacterPromotion.*` | `Content/Data/cards.csv`、`Content/Data/card_effects.csv`、`Content/Data/characters.csv`、`Content/Data/character_aliases.csv`、`ReEchoGameMode.*`、`Core/ReEchoTypes.*`、`Weapons/ReEchoWeaponActor.*` |
| 卡牌/角色/元素/反应/敌人/平衡数据 | 匹配的 `Content/Data/*.csv` 和仅迁移 JSON | `Content/Data/README.md`、`validate_project.py` |
| GM、调试命令、作弊、控制台 | `ReEchoGameMode.*`、`docs/GM_COMMANDS.md` | 匹配玩法子系统或 Actor API |
| 构建或直接打开模块失败 | `scripts/ue/Build-Editor.*`、`scripts/ue/prebuilt_editor.py` | 最新 UBT 日志、预构建 manifest 和匹配源码 |
| Windows 打包/cook/资源缺失 | `scripts/ue/package_windows.py`、`ReEchoGameMode.*`、硬资产引用、UAT Cook manifest | `Content/ReEcho/Textures/Characters/`、`Saved/Cooked/Windows`、Shipping 烟测 |
| 自动化 | `scripts/ue/Run-Automation.*` | `Private/Tests/*`、`Saved/Logs/ReEcho.log` |
| UE MCP | `docs/UE_MCP.md` | `ReEcho.uproject`、Editor 设置、`.codex/config.toml` |
| RenderDoc 图形调试 | `docs/RENDERDOC_MCP.md` | `scripts/mcp/Codex-With-RenderDoc.cmd` |
| AI 工作流/规则 | `AGENTS.md`、`shared/PROJECT_RULES.md` | 首次接入路由到 `PROGRAMMER_RULES.md`、`DESIGNER_RULES.md` 或 `ARTIST_RULES.md`；程序任务再使用匹配职责规则；`PLANNER_EXCHANGE.md` 记录实时范围/所有权；`WORKFLOW.md` 只说明原因 |

## 不变量与陷阱

- 只使用 UE 5.8 安装版/发行版；独立源码检出不在范围内。
- 构建 C++ DLL 前关闭 ReEcho Unreal Editor。
- 除非明确策划变更同步更新全部契约，否则模拟为 60 Hz、录制为 20 Hz、遭遇时长为 30 秒。
- 不得将自动攻击序列化进录制。
- 不得手改 `.uasset` 或 `.umap`；在 `PLANNER_EXCHANGE.md` 认领序列化资产，并通过 UE 修改。
- `Content/Data` 下 CSV 是目标运行时数据源；只改 JSON 不会改变玩法。
- 竞技场在运行时生成，不要搜索不存在的项目地图。
- `AllToolsets` 可能在 commandlet 中输出无关 GameFeatureData/Niagara Python 警告；依据具名自动化结果判断 ReEcho 测试。
- 遵循 `.clang-format` 和 `PROJECT_RULES.md` 的强制 Unreal C++ 章节。

## 验证路线

```powershell
python scripts\validate_project.py
scripts\ue\Build-Editor.cmd -Configuration Development
git diff --check
```

仅在 Plan 或用户明确要求时选择执行功能自动化（`scripts\ue\Run-Automation.cmd`）。

只在调用时传入 `-EngineRoot <path>`，或使用机器本地 `RE_ECHO_UE_ROOT`；绝不提交机器路径。
