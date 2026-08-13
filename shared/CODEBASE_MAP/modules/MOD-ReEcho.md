# `MOD-ReEcho`：`ReEcho`

## 模块状态

- 当前状态：当前 `main` 的 Runtime Module，也是项目的 UE 玩法装配根。
- 描述符：`ReEcho.uproject`。
- Build 文件：`Source/ReEcho/ReEcho.Build.cs`。
- 注册入口：`Source/ReEcho/Private/ReEcho.cpp` 中的 `FReEchoModule`。
- 主要目录：`Source/ReEcho/Public/`、`Source/ReEcho/Private/`。
- 启动地图：`/Game/Level00`，默认 GameMode 为 `AReEchoGameMode`。

## 存在原因

`ReEcho` 把 UE 世界生命周期、主流程、玩法领域、数据适配、Actor 装配、UI 和表现连接成可运行游戏。当前很多领域仍处于同一个编译模块，但通过 `AREA-*` 维持职责边界，以便功能稳定后按真实依赖进一步拆分，而不是仅按目录形式拆模块。

它是组合根，不是所有状态的总控制器：GameMode 可以决定“下一步进入哪个领域”，但不能复制 Run、Combat、Recording 或 UI 内部的权威状态。

## 职责与排除项

### 负责

- 启动/继续、装载选择、竞技场、六场遭遇、局间构筑、商店、Echo 选择和结束流程编排。
- 玩家、敌人、Echo、武器、投射物、世界 UI 与表现 Actor 的创建和生命周期装配。
- 将 XLSX 生成的 CSV 编译为类型化运行时快照。
- 本局阶段、构筑、存档、Echo 存储与回放选择。
- 当前 GAS/Combat、Weapons、Recording 和 UI Framework 的宿主与跨领域适配。
- 把玩法语义转换为 `ReEchoAudio` 请求以及 Animation/VFX/UI 的只读表现输入。

### 不负责

- 不让 `AReEchoGameMode` 保存各领域的第二份可写状态。
- 不让 Widget、动画、VFX、音频或资产加载结果决定命中、伤害、死亡、攻击节奏或流程成功。
- 不在 C++ 中复制已经进入 XLSX/CSV 权威链的平衡常量。
- 不从运行时读取 XLSX 或执行表格自由文本。
- 不要求底层独立模块反向 include `ReEcho` 具体类型。

## 权威状态

| 状态 | 权威拥有者 | 生命周期 | 合法访问方式 |
|---|---|---|---|
| 主流程当前屏幕/阶段编排 | `AReEchoGameMode` + `UReEchoUIFlowCoordinatorSubsystem` | World/屏幕切换 | 类型化流程命令与屏幕 ID |
| 本局阶段、构筑、背包、货币、Echo 存储/回放 | `UReEchoRunSubsystem` | GameInstance/整局 | 窄命令、只读摘要、SaveGame |
| 玩家/敌人生命与战斗属性 | GAS/`UReEchoCombatantComponent` | Actor/遭遇 | GameplayEffect、战斗命令、快照与委托 |
| 当前武器、攻击步骤与攻击载体 | `AReEchoWeaponActor` 及 Weapons 运行逻辑 | Actor/整局武器锁定 | 攻击请求、稳定 WeaponId、只读查询 |
| 遭遇时间与结束条件 | `AReEchoEncounterDirector` | 单场遭遇 | 固定步推进与完成委托 |
| 当前录制与历史 Playback | Recorder/Playback 组件 | 单场/存储录制 | 录制数据与播放接口 |
| 活跃屏幕、Viewport 层、焦点与输入模式 | UI Manager/Flow Coordinator | GameInstance/World | `EReEchoUIScreen` 与类型化 UI 命令 |
| Actor 可见状态 | 各 Presentation/Graybox Actor | Actor | 消费逻辑结果，不反写逻辑 |

## 输入、输出与公共契约

### 输入

- UE 生命周期：`StartPlay`、World Tick、输入绑定、Actor/Subsystem 生命周期。
- `Content/Data/*.csv` 与 `Config/*.ini`。
- 用户输入、Widget 命令和 GM/调试命令。
- SaveGame、录制历史和当前世界碰撞/目标信息。

### 输出

- 世界 Actor、确定性遭遇推进、战斗请求与结果。
- 本局只读摘要、保存文件、录制与 Echo Playback。
- UI 屏幕命令、只读展示数据和表现事件。
- 发往 `MOD-ReEchoAudio` 的语义音频请求。

### 稳定契约

- 稳定 `CharacterId`、`WeaponId`、Card/Part/Element/Reaction ID。
- `FReEchoBuildSnapshot`、录制样本/事件和 Run Save 版本迁移。
- `EReEchoUIScreen`、Gameplay Tag/FName、CSV Schema 与 manifest。
- 对独立模块只暴露值类型、窄接口、同步请求/结果或语义事件，避免暴露主流程私有字段。

## 依赖方向

```text
MOD-ReEcho ──→ MOD-ReEchoAudio
```

`ReEcho` 可以调用独立服务模块；`ReEchoAudio` 不得反向依赖 `ReEcho`。主模块内部高层编排可以依赖领域契约，领域逻辑不应依赖具体 Widget、纹理、材质或 GameMode 私有实现。

## 运行时流程

```text
DefaultEngine.ini
  → /Game/Level00
  → AReEchoGameMode::StartPlay
      → CreateArena / EncounterDirector / StartMenu
      → 新游戏：角色和初始武器选择 → RunSubsystem::StartRun
      → 继续：加载安全检查点或暂停遭遇
      → BeginNextEncounter / ResumeSavedEncounter
          → 玩家、Recorder、可用 Echo、敌人
          → 60 Hz 固定步遭遇
          → 全部敌人死亡或超时
          → 完成录制与 RunSubsystem::CompleteEncounter
          → 特质选择 → 商店 → Echo 管理 → 下一场
```

Esc 进入暂停层；保存退出必须先成功捕获遭遇时钟、玩家、当前录制和存活敌人，保存失败不得退出。

## 代码位置与阅读路线

| 目的 | Public 首读 | Private 实现 | 相关数据/资产 |
|---|---|---|---|
| 模块注册与编排 | `Source/ReEcho/Public/ReEchoGameMode.h` | `Private/ReEcho.cpp`、`Private/ReEchoGameMode.cpp` | `ReEcho.uproject`、`Config/DefaultEngine.ini` |
| 领域实现 | 下方匹配 `AREA-*` 的 Public 目录 | 同领域 Private 目录 | 对应 CSV、Config 或 Content 资产 |
| 跨领域修改 | 先读权威状态表和公共头 | 再读调用方与被调用方实现 | 对应自动化与 Plan |

不要先全文搜索所有 `Source/ReEcho`。先从下方选择一个领域，再沿 Public 契约 → Private 实现 → 数据/测试扩展。

## 内部领域

### `AREA-Core`：`Core`公共类型与兼容契约

**设计意图：** 保存主模块中需要跨多个领域共享、且尚未归入独立模块的稳定值类型、枚举、快照和全局原型设置。它是契约层，不是业务逻辑杂物箱。

- 代码：`Source/ReEcho/Public/Core/`、`Source/ReEcho/Private/Core/`。
- 首读：`ReEchoTypes.h`、`ReEchoBalanceSettings.h`。
- 权威：类型布局、枚举语义、兼容字段和全局配置入口；具体实例状态仍归所属领域。
- 扩展：只有三处以上领域真正共享、语义稳定且没有更合适模块所有者的类型才进入 Core。
- 禁止：资产加载、流程跳转、Widget 操作、世界查询和领域内部算法。

### `AREA-Data`：`Data`生产数据适配

**设计意图：** 把生成 CSV 校验并编译为类型化、不可变、可按稳定 ID 查询的运行时快照；隔离工作簿格式与玩法执行。

- 代码：`Source/ReEcho/Public/Data/`、`Source/ReEcho/Private/Data/`。
- 首读：`ReEchoCsvDataRegistry.*`、`ReEchoWeaponCsvReader.*`、角色/卡牌/元素 Reader。
- 数据链：`Design/Data/ReEchoData.xlsx` → `scripts/data/sync_xlsx_to_csv.py` → `Content/Data/*.csv`。
- 权威：CSV Schema 校验、稳定 ID 引用、运行时快照发布和领域修订值。
- 扩展：先改 XLSX/Schema/生成器，再扩 Reader 与验证；Behavior/Formula 等逻辑字段必须映射到注册实现。
- 禁止：运行时读取 XLSX、执行描述文本、把解析失败静默替换为默认逻辑、保存第二份平衡常量。

### `AREA-AbilityCombat`：`AbilitySystem` / `Combat`战斗执行

**设计意图：** 权威战斗逻辑已迁入 `MOD-ReEchoCombat`；主模块只负责把世界 Actor、Run/Recording、音频和表现接到其命令、事件与快照上。

- 逻辑代码与完整意图：[`MOD-ReEchoCombat.md`](MOD-ReEchoCombat.md)。
- 主模块适配：`Source/ReEcho/Public/Combat/`、`Source/ReEcho/Private/Combat/`，当前包括元素 CSV 编译/兼容 facade 与 `ReEchoCombatAudioAdapterComponent`。
- 边界：适配器可以翻译数据和订阅结果，不得保存第二份生命、元素、held 或最终伤害状态。
- 测试：逻辑模块 `Source/ReEchoCombat/Private/Tests/`；主模块保留跨 Run、Recording、世界 Actor 和兼容加载测试。

### `AREA-Weapons`：`Weapons`武器执行

**设计意图：** 武器规则和逻辑载体已迁入 `MOD-ReEchoWeapons`；主模块保留 CSV→Definition 编译、世界宿主和武器可见表现。

- 逻辑代码与完整意图：[`MOD-ReEchoWeapons.md`](MOD-ReEchoWeapons.md)。
- 主模块适配：`ReEchoWeaponRuntime.*` 把策划数据编译为资源无关 Definition；`ReEchoWeaponActor.*` 组合逻辑对象与 Sprite/Mesh/VFX Actor。
- 边界：Actor 可以创建表现和转发 Commit/HitIntent，但不能拥有第二个攻击频率门或自行扣血。
- 测试：逻辑模块 `Source/ReEchoWeapons/Private/Tests/`；主模块保留数据编译、构筑、Actor 装配和跨域回归。

### `AREA-Encounter`：`Encounter`遭遇时钟

**设计意图：** 提供可暂停、确定性的 60 Hz 固定步遭遇时钟与完成信号，让 Recording/Echo/敌人生成共享同一时间语义。

- 代码：`Source/ReEcho/Public/Encounter/`、`Source/ReEcho/Private/Encounter/`。
- 首读：`ReEchoEncounterDirector.*`。
- 权威：本场准备/运行时间、固定步累计与超时完成。
- 输入：开始、恢复、暂停和 World Tick。
- 输出：固定步事件、剩余时间和完成委托。
- 禁止：持有构筑、货币、存档或 Widget 状态。

### `AREA-Run`：`Run`本局状态与存档

**设计意图：** 将跨遭遇但限于本局的阶段、构筑、背包、货币、商店、Echo 存储/回放选择和安全保存集中在 GameInstance Subsystem。

- 代码：`Source/ReEcho/Public/Run/`、`Source/ReEcho/Private/Run/`。
- 首读：`ReEchoRunSubsystem.*`、`ReEchoRunSaveGame.h`、`ReEchoShopCatalog.h`。
- 权威：Run phase/index、BuildSnapshot、Inventory、Time Shard、Pending/Latest/Stored Echo、稳定回放 ID、SaveVersion。
- 输入：Start/CompleteEncounter、购买、特质选择、Echo 命令、保存/继续。
- 输出：只读摘要、确定性 offer、保存结果和下一阶段。
- 扩展：通过窄事务命令校验后一次更新；失败必须不产生部分状态。
- 禁止：返回可写内部容器、让 Widget 直接改字段、用数组索引充当持久 Echo 身份。
- 测试：Save、Shop、Trait、EchoStorage、EchoReplayRuntime 和 Run parity 测试。

### `AREA-Recording`：`Recording`录制与回放

**设计意图：** 以 20 Hz 捕获玩家位置和成功主动技能事件，并对历史位置插值、跨越技能事件；记录玩家意图历史，不记录当前世界的命中结果。

- 代码：`Source/ReEcho/Public/Recording/`、`Source/ReEcho/Private/Recording/`。
- 首读：`ReEchoRecorderComponent.*`、`ReEchoPlaybackComponent.*`。
- 权威：活跃录制时间线、样本顺序、完成录制内容和 Playback 游标。
- 规则：自动普通攻击不进入 Recording；Echo 回放时重新依据当前世界目标和结算。
- 扩展：新增可录事件必须定义稳定序列化语义、时间戳和旧存档兼容。
- 测试：`ReEchoRecordingTests.cpp` 及 Echo/Save 连续性测试。

### `AREA-Player`：`Player`玩家宿主

**设计意图：** 承接 Pawn 输入、移动、相机、鼠标朝向和玩家侧组件装配，把输入翻译成领域命令，不复制 Combat/Run/Weapon 权威状态。

- 代码：`Source/ReEcho/Public/Player/`、`Source/ReEcho/Private/Player/`。
- 首读：`ReEchoPlayerPawn.*`。
- 输入：WASD、攻击、暂停/模式命令和世界边界。
- 输出：移动、相机/朝向表现、攻击请求和只读玩家状态。
- 扩展：新输入先确定命令所有者；菜单、死亡与模式切换必须统一释放 held 请求。
- 禁止：在 Pawn 再建一套攻击间隔、生命或 Run phase。

### `AREA-Presentation`：`Graybox` / `Presentation`世界表现

**设计意图：** 组装玩家、敌人、Echo、投射物、血条、伤害数字和命中特效等世界对象，消费逻辑状态与事件形成可见反馈。

- 代码：`Source/ReEcho/Public/Graybox/`、`Private/Graybox/`、`Public/Presentation/`、`Private/Presentation/`。
- 首读：`ReEchoEnemyActor.*`、`ReEchoEchoActor.*`、`ReEchoProjectileActor.*`、命中/伤害数字适配器。
- 权威：只拥有表现实例与表现生命周期；逻辑生命、伤害、攻击节奏和录制不归这里。
- 输入：Combat/Weapon/Recording 结果、只读快照、稳定视觉 ID。
- 输出：Sprite/Mesh/材质、动画、VFX、世界文本和镜头反馈。
- 扩展：表现缺失、提前结束或加载失败必须不改变玩法；碰撞若承担玩法命中必须由逻辑契约明确，而非视觉组件偶然状态。

### `AREA-UI`：`UI`屏幕与交互

**设计意图：** 通过稳定屏幕 ID、中央 Widget 注册、统一焦点/输入/暂停策略管理开始、装载、HUD、特质、商店、Echo 管理、属性和暂停等界面。

- 代码：`Source/ReEcho/Public/UI/`、`Source/ReEcho/Private/UI/`。
- 首读：`UI/Framework/*`、`ReEchoUIManagerSubsystem.*`、各屏幕 Widget、`ReEchoGameMode` 的类型化端点。
- 权威：活跃屏幕实例、Viewport 层、焦点、输入模式和屏幕暂停策略。
- 输入：玩法只读摘要、用户选择和稳定 ID。
- 输出：类型化命令，不直接写 Run/Combat/Weapon 内部状态。
- 扩展：新增屏幕先注册 `EReEchoUIScreen` 与生命周期策略；GameMode 不直接管理 Widget Viewport。
- 人工验收：布局、可读性、焦点、点击区域和视觉效果由用户验收，Executor 不做高 token 视觉遍历。

### `AREA-Tests`：`Tests`验证边界

**设计意图：** 证明数据、领域算法和跨领域契约的确定性，防止目录重构或模块拆分改变行为。

- 主位置：`Source/ReEcho/Private/Tests/`；独立模块测试位于各自 `Private/Tests/`。
- 分层：纯规则单元测试 → World/GAS 集成 → Save/CSV/兼容 → 完整 `ReEcho.*` 自动化。
- 构建：`scripts/ue/Build-Editor.cmd -Configuration Development`。
- 自动化：`scripts/ue/Run-Automation.cmd -Filter <focused>`。
- 静态：`python scripts/validate_project.py`、`git diff --check`、预构建包检查。
- 边界：自动化不能替代用户对手感、视觉、音频可听性和可用性的判断。

## 常见任务阅读路线

| 任务 | 先读 | 再读 |
|---|---|---|
| 启动/继续/遭遇切换 | `ReEchoGameMode.*`、`RunSubsystem.*` | UI Framework、EncounterDirector、SaveGame |
| 自动/手动攻击 | `PlayerPawn.*`、AbilitySystem、`WeaponActor.*` | AttackMode/Weapon/Combat 测试与 `weapons.csv` |
| 武器/部件/元素 | Data Reader、`WeaponRuntime.*`、`ElementReaction.*` | 生产 CSV、Run build snapshot、世界载体 |
| 商店/特质/Echo 管理 | `RunSubsystem.*`、Shop/Trait/Echo 摘要 | 对应 Widget、GameMode 类型化端点和测试 |
| 保存/继续 | `RunSaveGame.h`、`RunSubsystem.*` | Encounter、Recording、存活敌人快照测试 |
| 新 UI 屏幕 | UI Framework、UI Manager | GameMode 快照/命令端点、WBP 注册 |
| Actor/动画/VFX 接入 | 逻辑结果契约、Presentation/Graybox Actor | 资产路径、Plan40 等表现 Plan |

## 扩展原则

- 先确定新状态的唯一所有者，再选择 Component、Subsystem、普通 C++ 对象、GAS 或 Actor。
- 跨领域优先使用值类型、稳定 ID、只读快照、语义事件和窄命令；不要暴露可写内部对象。
- 只有依赖边界和独立验证价值明确时才拆 Runtime Module；目录数量不是拆模块理由。
- 当某 `AREA-*` 独立成 Runtime Module 时，保留或迁移稳定标识，新增独立 `MOD-*` 文档，并在全局架构中明确新依赖。

## 验证与测试

- 修改单一领域时，先运行该领域的 focused automation，再运行受公共契约影响的相邻领域测试。
- 修改模块装配、公共类型或跨领域消息时，必须完成 Editor Development 构建和完整 `ReEcho.*` 自动化。
- 修改 CSV/XLSX 数据契约时，同时运行静态数据校验、导入/往返测试和对应运行时测试。
- UI、视觉、音频可听性和战斗手感由用户在 PIE 中验收；自动化负责证明命令、快照和领域结果，不替代人工体验判断。
- 统一静态门禁为 `python scripts/validate_project.py` 与 `git diff --check`。

## 不变量与常见错误

- `AReEchoGameMode` 是编排器，不是各领域状态仓库。
- Run/Combat/Weapon/Recording/UI 各自只有一个权威写入口。
- 自动攻击不进入 Recording；Echo 在当前世界重新选目标与命中。
- 保存失败不得退出；购买/装备/选择失败不得产生部分状态。
- 表现资源和完成回调不能控制确定性逻辑。
- 不从旧 JSON、描述文本、Widget 缓存或 Actor 表现字段恢复第二份事实来源。
