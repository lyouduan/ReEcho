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

- 启动/继续、装载选择、竞技场、八场表驱动遭遇、局间构筑、商店、Echo 选择和结束流程编排。
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
| 卡牌目录、拥有/叠层、随机游标和事件状态 | `MOD-ReEchoCards` 的 Catalog/BuildState/RuntimeState | 整局并嵌入 Build/Recording | 主模块只调用纯命令并执行类型化结果 |
| 玩家/敌人生命与战斗属性 | GAS/`UReEchoCombatantComponent` | Actor/遭遇 | GameplayEffect、战斗命令、快照与委托 |
| 当前武器、攻击步骤与攻击载体 | `AReEchoWeaponActor` 及 Weapons 运行逻辑 | Actor/整局武器锁定 | 攻击请求、稳定 WeaponId、只读查询 |
| 怪物 Archetype、AI phase、攻击冷却、Fuse、受击位移与攻击序号 | `MOD-ReEchoEnemies` 的 `UReEchoEnemyLogicComponent` | Actor/单场遭遇 | EnemyHost 注入 Sense、应用 Intent；表现只读 Snapshot/Event |
| 当前战场怪物注册集合与稳定顺序 | `UReEchoEnemyRosterComponent` | Stage 连续战场 | GameMode 生成/按 Stage 策略清理，保存与全灭判断读取；同 Stage 跨 Encounter 保留原 Host，不扫描世界复制状态 |
| 遭遇时间与结束条件 | `AReEchoEncounterDirector` | 单场遭遇 | 表驱动时长、固定步推进与完成委托 |
| Stage/Wave 门、预警、出生候选与普通怪全局技能令牌 | WaveScheduler / SpawnResolver / GameMode Encounter coordinator | 单场遭遇 | 预警时锁定位置，Commit 时才创建并原子激活可受击 Enemy Host；Host 完成 Definition/Profile 装配后只尝试一次可选 Born 表现，不等待结果且不改变碰撞、受伤、AI 或 Commit；零秒首波在遭遇 0 秒预警并完整等待 SpawnProfile 的 WarningLeadSeconds 后 Commit；GameMode 统一限制远程窗口和精英并发；EnemyLogic 只消费许可 |
| 当前 Arena 场景与 SceneId 注册 | `AReEchoArenaSceneActor` 注册表；Stage CSV `SceneId` 为选择权威 | World/Stage | GameMode 在初始、恢复和跨 Stage 入口先应用场景；同 Stage 不重建，失败阻止 Encounter 开始 |
| 当前录制与历史 Playback | Recorder/Playback 组件 | 单场/存储录制 | 录制数据与播放接口 |
| 活跃屏幕、Viewport 层、焦点与输入模式 | UI Manager/Flow Coordinator | GameInstance/World | `EReEchoUIScreen` 与类型化 UI 命令 |
| Actor 可见状态 | 各 Presentation/Graybox Actor | Actor | 消费逻辑结果，不反写逻辑 |
| 首场表现资源预加载状态与驻留句柄 | `UReEchoRuntimeAssetPreloader` | GameInstance | Catalog 枚举软路径；完成/失败均以幂等终态释放开始门控 |

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
- 当前玩家 Combat 最终受伤与生命变化事件到 Player HUD 全屏反馈的只读装配；反馈失败不改变战斗或流程。
- 战斗常驻 HUD 的只读表现投影：Combatant 提供生命，Run 提供 TimeShards，Encounter Director 提供剩余/总时长，Player/Echo Presentation Profile 提供小地图头像；GameMode 只转发这些状态、进度事实及小地图视图，不复制或回写权威。
- 发往 `MOD-ReEchoAudio` 的语义音频请求。
- Development 编辑器启动时由 `FReEchoModule` 注册第二个只读日志输出设备，把普通 `UE_LOG` 同步写入 `Saved/Logs/ReEcho-session-<本地开始时间>-pid<进程号>.log`；`ReEcho.log` 仍是当前会话入口，独立会话文件不覆盖、不参与玩法状态，也不进入 Shipping。

### 稳定契约

- 稳定 `CharacterId`、`WeaponId`、Card/Part/Element/Reaction ID。
- `FReEchoBuildSnapshot`、录制样本/事件和 Run Save 版本迁移；v10 组合保存 `CardDomainRevision`/卡牌运行态、Encounter 波次/预警/全局令牌、EnemyLogic/Combatant/Transform 与独立武器配件所有权。
- SaveVersion 22 随 CardState 保存 Plan111 的债务、导电关内增幅、Echo三件套和已完成关卡武器历史，并保存当前免费三选一与各商店Tier卡组的已展示历史；v21及更早版本以当前候选重建最小展示历史，v20迁移采用零债务/零新历史并接纳追加式卡牌目录，不伪造已丢失的新机制收益。
- `EReEchoUIScreen`、Gameplay Tag/FName、CSV Schema 与 manifest。
- 对独立模块只暴露值类型、窄接口、同步请求/结果或语义事件，避免暴露主流程私有字段。

## 依赖方向

```text
MOD-ReEcho ──→ MOD-ReEchoAudio
           ├→ MOD-ReEchoCombat
           ├→ MOD-ReEchoCards ─→ MOD-ReEchoCombat
           ├→ MOD-ReEchoWeapons
           └→ MOD-ReEchoEnemies ─→ MOD-ReEchoCombat
```

`ReEcho` 可以调用并组合独立模块；Audio、Combat、Cards、Weapons、Enemies 均不得反向依赖 `ReEcho`。Cards 只依赖 Combat 的稳定值类型；主模块内部高层编排可以依赖领域契约，领域逻辑不应依赖具体 Widget、纹理、材质或 GameMode 私有实现。

### 音频语义装配

- `AReEchoGameMode` 从已经确认的菜单、遭遇、Boss、商店、天气、死亡、胜利、镜头切换和死亡重开生命周期发布状态或事件；普通遭遇把权威 `StageId` 作为 `Music.Encounter` 变体，`UReEchoRunSubsystem` 不保存第二份音乐状态。
- 基础敌人经济奖励以 `ReEchoData.xlsx/经济系统/tblEnemyShardDrops` 导出的 `enemy_shard_drops.csv` 为唯一数值权威。GameMode 在统一敌人装配点订阅最终死亡事实，只把稳定 `EnemyId/SpawnIndex` 路由给 Run；Run 按死亡时的当前 Encounter、Archetype、持久化 Run seed 和卡牌经济规则解析一次掉落数额并持久化已处理键。GameMode 是通用时间碎片世界实体的集中生成与关末清理边界，敌人死亡和武器符文均生成 `/Game/ReEcho/Gameplay/Pickups/BP_TimeShardPickup`，仅无 GameMode 的自动化世界回退原生类；Encounter 结束时所有尚未入账或仍在吸附的世界碎片立即销毁，不跨小关、Stage、商店或存档。Prefab 的继承 Sphere Collision 是唯一摄取范围权威，可在 Blueprint 组件详情直接调整；玩家进入范围后，Pickup 按“最小追速”与“玩家当前平面速度 + 可调优势”的较大值沿当前活动 Arena GameplayPlane 吸向玩家，到达捕获半径才通过 `GrantTimeShards` 一次性入账。活动地面只由 GameMode 当前 `ArenaScene` 的窄查询提供，禁止遍历世界猜测 Stage 切换期的 Arena。Prefab 暴露 `PresentationRoot -> {VisualRoot, GroundRoot -> GroundShadow}`，Class Defaults 拥有图标高度、材质、地面排序、贴地开关、吸附/捕获、落地弹跳与拾取上升参数，组件拥有局部 Transform/阴影缩放与材质。拾取图标使用真正参与透明排序的 `UMaterialBillboardComponent` 和支持 `Opacity` 的专用透明材质；材质忽略地面深度以免贴地中心以下被地面裁切，角色/怪物/战斗表现的遮挡仍由 Backdrop 与角色脚点之间的透明排序带决定。落地动画只移动 `VisualRoot`；捕获时先完成一次性入账与碰撞关闭，再隐藏地面阴影并让图标上升淡出。禁止改回忽略 `TranslucencySortPriority` 的 Masked `UBillboardComponent`，也禁止让表现动画延迟或重复货币事务、在调用点复制视觉常量。Enemy、Combat、攻击载体和表现均不直接写货币。
- `UReEchoUIFlowCoordinatorSubsystem` 为注册屏幕的按钮统一绑定 hover/基础 confirm，并通过无玩法状态的 `UReEchoButtonVisualFeedback` 缩放按钮完整视觉根；单按钮 Overlay 即使其 Button 自带 Content 仍优先作为完整根。Settings/Restart 的作者ing WBP Root 是正常表现权威，原生整页构建只在没有 Root 时降级；GameMode 仅在真实关闭、拒绝、购买和卡牌选择结果上追加专用 UI 事件。
- `UReEchoCombatAudioAdapterComponent` 消费 Combat 最终事件；玩家攻击提交把稳定 `WeaponId` 作为 `Combat.Attack` 变体，最终命中把 `FReEchoDamageEvent.Element` 转换为稳定 ElementId 作为 `Combat.Hit` 变体。Enemy/Boss Host 把已提交 Intent 翻译为其专用攻击事件，Enemy Archetype 决定 Enemy/Boss 路由，Echo Host 发布自身生命周期。
- 所有调用点只传 `FReEchoAudioEvents` 稳定 ID、位置、粗粒度来源和可选 VariantId，不加载 SoundWave，也不读取音频结果改变玩法。

## 运行时流程

```text
DefaultEngine.ini
  → /Game/Level00
  → AReEchoGameMode::StartPlay
      → 校验并消费 Level00 唯一 Arena Scene / EncounterDirector / StartMenu
      → GameInstance 预加载器异步预热 Combat VFX、兔子代理、四武器首用表现与 MoonStaff 辅助表现
      → 新游戏：角色和初始武器选择 → RunSubsystem::StartRun
      → 继续：加载安全检查点或暂停遭遇
      → 预加载未完成时保留当前菜单；完成或失败后只进入一次 BeginSelectedRun
      → BeginNextEncounter / ResumeSavedEncounter
          → 按 Stage CSV SceneId 从 Arena Blueprint 注册表解析 SC01-SC04；只在 SceneId 变化时原位替换 Arena，并重绑 Player/Camera/Bounds
          → 玩家、Recorder、可用 Echo、EnemyHost + Roster
          → 60 Hz 固定步遭遇 → 0/10/20 秒 WaveScheduler（零秒首波先预警并等待配置 lead，后续波次仍提前预警、按 TriggerSeconds 生成）
          → 普通战按 30 秒完成；ReEchoStageTransition 统一解析下一场策略；Boss 按胜负
          → 完成录制与 RunSubsystem::CompleteEncounter
          → 局间停止玩家并冻结保留 Enemy Host，清理旧 Echo/瞬时攻击
          → 特质选择 → 商店 → Echo 管理
          → 同 Stage 原 Arena/Actor/Roster 与玩家位置继续；跨 Stage 清理并切换 Arena、解析入口 → 下一场
```

Arena 作者ing中，`BackdropHalfExtents` 只控制底图显示宽高，与 Camera/Player/Enemy Bounds 分离。SC01-SC04 Blueprint 默认关闭碰撞自动布局，Floor/四墙采用组件模板 Transform，construction 与作者ing重跑不得覆盖美术手调值。各 Backdrop 模板直接绑定对应 Profile 的 MI 以供 Blueprint Editor 预览，运行时仍由 `ApplySceneProfile` 创建动态实例并应用调色参数；Floor 不承载视觉底图。相机基础锁边范围读取当前 Arena 的 `CameraClampHalfExtents`，相机 Actor 另有左、右、下、上四个独立 inset；正交视锥 footprint 到达任一边缘时仅锁定对应轴，角色继续由 Player Bounds 与墙体限制。SC01 边缘插片是 Arena Blueprint 的直接组件；初始排序为 Backdrop < 角色/怪物 < Mid < Foreground，作者ing脚本只初始化缺失组件并保留已有美术 Transform、显隐、材质与透明排序。

Esc 进入暂停层；保存退出必须先成功捕获遭遇时钟、玩家、当前录制和存活敌人，保存失败不得退出。

Development 控制台命令统一由 `AReEchoGameMode` 的 `UFUNCTION(Exec)` 提供，完整列表见 `docs/GM_COMMANDS.md`。`UReEchoConsole` 只在控制台可见期间暂停游戏，并且仅恢复由自己触发的暂停；各 GM 命令不单独改变暂停状态。`GMSpawnFox <count> [distance]` 只用于快速表现验收：数量钳制为 `1..16`、距离钳制为 `150..1000 cm`，沿朝 Arena 中心的确定性弧线分散并钳制在 Enemy Spawn Bounds 内；它逐只复用生产 `M_FOX` Definition、EnemyHost、Roster 并具名报告成功/失败数，不建立第二套测试怪物。无参数仍生成 1 只、单个大于数量上限的参数按旧距离语法兼容。

`GMGod <On|Off|Toggle>` 只切换当前 Player Combatant 的 Development 最终伤害门禁；它不修改生命上限、格挡、元素规则或敌人结算，且 Shipping 中始终不可用。

`GMElement <None|Flame|Lightning|Grass|Water>` 在 Player 的最终出手修正末端持续覆盖每次攻击的元素，直至再次指定；`None` 关闭覆盖并恢复武器权威元素。`GMReaction` 是纯表现验收入口：它在最近存活敌人的受击表现根直接播放指定反应 Niagara，不写入元素附着、伤害、状态或正式反应事件；正式战斗反应仍只由 Combat 权威链路触发。

## 代码位置与阅读路线

| 目的 | Public 首读 | Private 实现 | 相关数据/资产 |
|---|---|---|---|
| 模块注册与编排 | `Source/ReEcho/Public/ReEchoGameMode.h` | `Private/ReEcho.cpp`、`Private/ReEchoGameMode.cpp` | `ReEcho.uproject`、`Config/DefaultEngine.ini` |
| 领域实现 | 下方匹配 `AREA-*` 的 Public 目录 | 同领域 Private 目录 | 对应 CSV、Config 或 Content 资产 |
| 跨领域修改 | 先读权威状态表和公共头 | 再读调用方与被调用方实现 | 对应自动化与 Plan |
| 首场表现资源预加载 | `Presentation/Loading/ReEchoRuntimeAssetPreloader.h` | 配对实现、`ReEchoGameMode.cpp` | VFX/Weapon Visual Catalog、`ReEchoRuntimeAssetPreloadTests.cpp` |

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
- 首读：`ReEchoCsvDataRegistry.*`、`ReEchoWeaponCsvReader.*`、`ReEchoCharacterBuildCsvReader.cpp` 和角色/卡牌/元素 Reader。
- 数据链：主策划、怪物、Encounter、音频四个独立 canonical 工作簿 → 统一 `scripts/data/sync_xlsx_to_csv.py` → `Content/Data/*.csv`；二进制工作簿保持独立所有权。已迁移玩法 JSON 已物理删除并由项目校验禁止回归，历史审阅只使用 Git。
- 符文投放资格以 `ReEchoData.xlsx/武器插槽C/tblParts` 的 `Enabled + ImplementationStatus + ShopEnabled + ShopPrice` 联合字段为权威，效果资格以同 Sheet 的 `tblPartEffects.Enabled` 为权威；禁用符文必须同时关闭父记录、商店资格并将价格归零，同时禁用其全部效果，不能只在 UI 隐藏。
- 角色能力链：`ReEchoData.xlsx/角色能力A/tblCharacterAbilities` → `character_abilities.csv` → `FReEchoCsvDataSnapshot::CharacterAbilities` → `Run/CharacterAbilities/ReEchoCharacterAbilityRuntime.*`。`characters.csv` 只保存角色实体和基础 StatBlock；描述文字与旧 JSON 不参与能力分派。
- 商店刷新链：`ReEchoData.xlsx/经济系统/tblShopRefreshRules` → `shop_refresh_rules.csv` → `FReEchoCsvDataSnapshot::ShopRefreshRules[Default]` → Run 的武器/符文页刷新与卡组逐槽刷新事务。刷新次数和价格只以该表为权威，UI 仅消费 Run 投影。
- 怪物编译：`ReEchoEnemyData.xlsx` 发布 `enemies`、`enemy_abilities`、`boss_phases`、`enemy_combat_stats`；Definition 编译以 `EnemyId + EncounterIndex` 读取不可变快照，按场次覆盖 MaxHealth、ContactDamage、AttackInterval，再把相对移速乘数以 `BaseMoveSpeed=210` 归一为运行时 cm/s。运行时不得回读 XLSX，也不得在 Host 复制成长或仇恨默认值。
- 权威：CSV Schema 校验、稳定 ID 引用、运行时快照发布和领域修订值。
- 武器装载顺序：`weapons.csv` 的 `InputSlot` 与 `LoadoutOrder` 支持 `1..6`；允许删除武器后保留稳定顺序空档，但可选武器的顺序值必须唯一且必须绑定非 `None` 输入槽。删除武器族时必须同时更新权威 XLSX/CSV 与 Reader 的精确部件审计计数，启动校验不得继续要求已删除武器的来源行或效果种类；未配置的可选效果种类不构成数据错误，但已配置行仍须通过 Schema/Behavior 校验。
- 武器伤害数据：`weapons.csv` 与 `attack_steps.csv` 只发布 `DamageCoefficient`；核心宝石通过 `part_effects.csv` 的 `DamageChannel` 选择物理、指定元素或确定性随机元素。原初之晶选择物攻，其他元素宝石与棱镜之晶选择元攻；数据层不得恢复物理/元素双倍率字段。
- 扩展：先改 XLSX/Schema/生成器，再扩 Reader 与验证；Behavior/Formula 等逻辑字段必须映射到注册实现。
- 禁止：运行时读取 XLSX、执行描述文本、把解析失败静默替换为默认逻辑、保存第二份平衡常量，或在 `Content/Data` 恢复已迁移玩法 JSON。

### `AREA-Cards`：卡牌构筑适配

**设计意图：** 卡牌定义、抽取资格、授予事务和事件状态由独立 [`MOD-ReEchoCards.md`](MOD-ReEchoCards.md) 负责；主模块只把 CSV 编译成目录，并把 Run、Combat、Echo、Enemies、Weapons、Shop/UI 接到类型化命令和规则快照。

- 主模块适配：`Data/ReEchoCsvDataRegistry.*`、`Run/ReEchoRunSubsystem.*`、玩家/Echo/Enemy/GameMode/WeaponRuntime 接缝。
- 权威：Cards 模块拥有 CardState 与规则计算；Run 拥有整局提交、货币、录制和 Save IO。
- 禁止：Run、Widget 或 Actor 再按中文说明/卡牌 ID 编写第二套通用分派，或绕过 Cards 直接改卡牌运行态。

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
- 伤害倍率适配：静态符文修改只写入同一个 `Weapon.DamageCoefficient` 并同步到实际 AttackStep；宝石只写 `Weapon.DamageChannel`，不参与倍率兼容选择。最终属性来源和公式由 `MOD-ReEchoWeapons` 权威执行。
- 武器符文装载（Plan75）：每把武器 3 槽（1 核心 Core + 2 武器专属命名槽，如弓=弓弦+箭头），符文语义即 `parts.csv` 配件。`AReEchoWeaponActor` 持有局内活权威 `EquippedRunes`（`TArray<FReEchoEquippedPartSnapshot>`）并提供 `EquipRune`/`UnequipRune`；装配/卸下复用 `ReEchoWeaponRuntime::TryEquipParts` 的统一校验与 Effect 编译，并触发 `RebuildEffectiveDefinition`，使"装了符文的武器"对外是半径统一的有效武器（"一体"）。持久真值仍为 `BuildSnapshot.EquippedParts`，`EquippedRunes` 每次变动后镜像之，存档/回放由 `BuildSnapshot` 重建。符文行为数据驱动（特殊行为走 `BehaviorId`），不继承子类；装配/Effect 不进入 `ReEchoWeapons` 逻辑模块。
- 武器符文执行（Plan76）：47 个可见生产符文全部由 `part_effects` 编译。静态修改会同时作用于武器 Definition 和实际 AttackStep，避免步骤值覆盖符文；19 种动态 `Part.*` 行为编译为每次 Commit 快照化的 `FReEchoWeaponRuneEffectSpec`，由 `AReEchoWeaponActor` 在 Combat 最终 `FReEchoHitResolved` 后执行命中、暴击、击杀、群攻阈值、临时叠层、陨星、外圈、分裂和镰刀投掷。碎片符文与基础敌人掉落共用 `AReEchoTimeShardPickupActor` 的正式纹理和拾取后入账边界，Echo 结果不产出经济收益。描述文字不参与分派。
- 攻击表现路由（Plan76/104）：长剑与镰刀的稳定 AttackPattern 由 Combat VFX Catalog 分别映射专用一次性 Niagara，不再生成旧平面刀光；弓与枪的飞行 Niagara 附着到 `AReEchoProjectileActor`，首次权威 `OnProjectileImpacted` 播放各自命中特效，表现不积分位移、不决定命中。四者不再读取 `SlashCrescent/ScytheSweep/BowProjectile/GunProjectile`；鞭和正式法杖表现已退役，`MoonStaff/StaffLightWave` 仅保留为贤者动画辅助。任何表现缺失都不得影响逻辑 Commit、飞行和伤害。
- 武器表现 Profile（Plan77）：`FReEchoWeaponVisualCatalog` 以 `WeaponVisualKey` 一对一解析本体资源、武器动作模式和专属攻击 VFX 能力；Profile 不包含角色动画或玩法规则。Player/Echo 可启用 Weapon Track，普通怪物强制无武器，Boss 只有显式配置武器表现 ID 时才允许启用。
- 武器表现资源由 `/Game/ReEcho/DataAsset/Weapon` 下的一武器一 `UReEchoWeaponPresentationProfile` 配置，并由 `DA_WeaponPresentationCatalog` 按 VisualKey 唯一解析；预加载器只聚合已启用的 Soft Reference。Profile 提供 Charge、Travel、DamageApplied 三个可选阶段槽，并保留明确的 AttackCommitted 槽承载长剑/镰刀现有提交斩击。
- 手持武器装配（Plan81/Plan100）：唯一 Weapon Presentation Catalog 以 `RightHandAnchorRatio` / `LeftHandAnchorRatio` 提供所有人物和武器共用的左右挂点，角色 Animation2D Profile 只提供稳定 `WorldHeight`，每个 Weapon Presentation Profile 以相对 `HeldOffsetRatio`、尺寸主轴、旋转及 `HeldLengthOverrideCm` 描述单武器差异。六个生产 Profile 使用绝对世界长度，因此同一 WeaponVisualKey 在 Player/Echo 上不再被宿主 Actor Scale 二次改变；Player/Echo 按朝向直接选择共享挂点，不读取当前动画帧 Bounds，也不把受击临时形变传给武器。
- Boss 法杖装配（Plan100）：Enemy Presentation 的 TimeGuard/MoonStaff 路径以 Gameplay Blueprint 作者配置的 `BossWeaponRoot` 为不变基准挂点；唯一子层 `BossWeaponFacingRoot` 根据屏幕朝向直接选择 MoonStaff Profile 的 `HeldRightFacingOffsetRatio` / `HeldLeftFacingOffsetRatio`，两侧可在武器 DA 中独立调节。Boss 不消费 Player/Echo 的共享挂点、通用 `HeldOffsetRatio` 或相机前置位置计算；法杖尺寸仍来自 MoonStaff Profile，Boss 技能、摆动和事件时序不变。
- Echo 空间表现（Plan71 返工）：Echo Catalog/Profile 只提供独立回响动画与材质差异；运行时按 Recording `CharacterId` 查询 CSV `AppearanceId`，把 Player Profile 的 `WorldHeight` 与脚点策略组合进瞬态 Echo Profile，武器左右挂点统一来自 Weapon Presentation Catalog。Echo 初始化从 live Player 读取稳定的 Presentation/Foot/Flipbook/AttackVfx/HurtVfx 作者 Transform 和 Actor Scale，确保 Gameplay Blueprint 实例上的 FlipbookRoot Rotation 被准确继承；会随当前帧变化的 Motion/Effects/Ground/Shadow 则从该 Player Class CDO 读取安全作者基准，避免复制受击形变、脚点对齐或动态阴影。缓存后的 Effects 与阴影基准使角色尺寸、手持武器、阴影及战斗 VFX 根不再依赖 Echo 专用空间常量。复制只影响表现树与 Actor 总体缩放，不改变 Playback、Combat 或攻击时序；开发自动化可通过只在 `WITH_DEV_AUTOMATION_TESTS` 下存在的空间快照读取最终 Transform、归一化高度和 Bounds 宽度，不形成 Shipping 公共状态。
- Echo Gameplay Blueprint：`/Game/ReEcho/Gameplay/CharacterPrefabs/BP_EchoGameplay` 是 `AReEchoEchoActor` 的薄表现子类，允许美术编辑继承的 PresentationRoot、FlipbookRoot、GroundShadow、AttackVfxRoot、HurtVfxRoot 与 EchoAuraVfxRoot。GameMode 构造期以硬类引用保证 Cook 收录，新遭遇与存档恢复只调用统一 `SpawnEchoActor()`；资产不可加载时回退原生 Class。幂等作者脚本只创建缺失资产，已有 Blueprint 永不重写作者 Transform。
- 边界：Actor 可以创建表现和转发 Commit/HitIntent，但不能拥有第二个攻击频率门或自行扣血。
- 测试：逻辑模块 `Source/ReEchoWeapons/Private/Tests/`；主模块保留数据编译、构筑、Actor 装配和跨域回归。

### `AREA-Enemies`：怪物逻辑、宿主与表现接线

**设计意图：** `MOD-ReEchoEnemies` 独占怪物行为状态；主模块只提供世界感知、Transform/Collision 应用、Combat 转发和资源表现，避免逻辑与美术继续争用同一份实现。

- 逻辑代码与完整意图：[`MOD-ReEchoEnemies.md`](MOD-ReEchoEnemies.md)。
- 世界宿主：`Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*`，只组合 Logic/Combat/Presentation、构造 Sense、应用 Intent、维护 Actor 生命周期，并把敌方逻辑载体的连续路径与世界接触转为 Combat。狐狸 Active 每步继续用根 Box `AddActorWorldOffset(..., sweep=true)`，只按实际起终点与锁定目标碰撞盒的首次相交提交同一 `AttackIdentity` 的 `FReEchoHitIntent`；无敌接触消费一次门但不扣血，世界阻挡立即反馈 Logic 进入 Recovery，禁止穿透或补偿传送。兔子移动散射一次 Commit 展开三条扇形轨迹，站定连发则按 `ActiveSeconds` 依次发布四条同向轨迹，每条最多结算一次；两种兔子技能的单球碰撞半径固定沿用原移动三球基准 `RadiusCm / 3`，不因发数改变。Combat Death 后 Host 立即关闭 Logic/碰撞和新行为，保留已发射逻辑投射物推进；死亡阶段只继续轻量表现推进，使 GroundShadow 可跟随 Death 当前帧；Death Clip 独占播放一次并在实际完成时销毁 Host，缺失 Death 时下一安全帧销毁，实际时长加短宽限只作为防卡死 watchdog。
- 表现适配：`Source/ReEcho/{Public,Private}/Presentation/Enemy/ReEchoEnemyPresentationComponent.*` 只把稳定 `PresentationId` 和表现事件转发给 `MOD-ReEchoPresentation`；同时只读 Host 的 Combat 伤害/反应结果生成伤害数字和一次性元素反应字。反应字严格使用事件的 `ReactionBehaviorId` 与 `PrimaryTarget`，不从元素状态重算，也不按 Growth/Conduct 的受影响目标重复生成。主模块的 `UReEchoEnemyGameplayClassRegistry` 独立解析敌人 Gameplay Blueprint Class；血条、动画、命中特效、元素光环、反应字与死亡残留均不反向控制玩法。
- 主流程：`AReEchoGameMode` 从不可变 Run 数据快照编译并注入 Enemy Definition，通过 Roster 管理生命周期；`SetEncounterSimulationSuspended` 在同 Stage 局间冻结原 Host，并在进入下一 Encounter 前恢复。Boss 房由 Boss 死亡结束，30 秒 EncounterPhase 只编排 Echo 退场和配表倍率强化。
- Plan68 接线：GameMode 用当前 `EncounterIndex` 编译每个出生 Enemy Definition；Host 注入表驱动 `HateRangeCm`、当前生命比率和特殊行动许可。普通怪的距离/受击次数 Phase2 仅改变表现，转换期间继续接受伤害；`M_SHEEP` 的第一次致命伤由 Combat 窄委托转为 Phase2 变身事件，完成后把生命上限和当前生命切到 650；普通怪未战斗时的 IdleWander 仍由 EnemyLogic 独占状态。
- 群体移动：EnemyLogic 的追踪/攻击意图保持权威；主模块 `FReEchoEnemyCrowdSteering` 在 Host 应用 Transform 前，根据 SpawnIndex 稳定攻击槽位、Roster 邻居 Separation、切向绕行和短时受阻恢复修正普通追踪位移。普通敌人之间互相加入 MoveIgnore，避免 Pawn Sweep 形成静止队列；玩家、场景与 Boss 仍保持硬碰撞，击退和特殊动作不进入普通 Crowd 修正。
- 保存：v9 `FReEchoEnemyRuntimeState` 使用稳定 EnemyId 聚合 Transform、完整 EnemyLogicSnapshot、Combatant 生命/元素，以及通用敌方已生成/已提交待生成投射物；狐狸 Active 快照包含剩余时间/距离、锁向、攻击身份和一次接触门，旧快照缺失这些瞬时字段时安全落入 Recovery，既不补走终点也不重复伤害。待生成直线连发球保存剩余延迟和 Spawned 发布状态，旧存档条目默认视为已生成。EncounterRuntimeState 另存波次游标、预警已解析位置、Spawn序号及普通怪全局技能令牌剩余时间，表现临时状态不保存。
- 禁止：EnemyActor 再持有攻击/引信/击退计时器，Presentation 调用伤害/AI 命令，GameMode 每帧 `TActorIterator<AReEchoEnemyActor>` 扫描。
- 测试：`ReEcho.Enemies.*`（其中 `ReEcho.Enemies.Host.FoxDashCollision` 锁定分帧 Sweep、首次路径伤害、躲避/无敌/撞墙）、`ReEcho.StageTransition.*`、`ReEcho.Run.SaveSnapshot`、Combat ElementReaction 与完整回归。

### `AREA-Encounter`：`Encounter`遭遇时钟

**设计意图：** 提供可暂停、确定性的 60 Hz 固定步遭遇时钟、表驱动波次门、单一出生解析和纯值 Stage 过渡决策，让 Recording/Echo/敌人生成共享同一时间语义，同时不让 GameMode 保存刷怪平衡常量。Stage 是连续战场，Encounter 只界定时钟、波次、录制和 Echo；结束一场不等于重建同 Stage 的 Actor。

- 代码：`Source/ReEcho/Public/Encounter/`、`Source/ReEcho/Private/Encounter/`。
- 首读：`ReEchoEncounterDirector.*`、`ReEchoEncounterRuntime.*`、`ReEchoEncounterCsvReader.*`。
- 权威：Director 独占本场运行时间；WaveScheduler 独占已触发事件游标；SpawnResolver 只做纯确定性计算；`ReEchoStageTransition::Resolve` 只消费当前/下一 Encounter 与 Stage 行，统一输出同 Stage、Roster 保留和玩家位置保留策略；GameMode 的 Encounter coordinator 独占远程窗口/精英并发令牌。
- 输入：开始、恢复、暂停、World Tick、不可变 Stage/Encounter/Wave/Spawn 数据、玩家运动样本和录制路径样本。
- 输出：固定步事件、剩余时间、完成委托、预警/提交事件、确定性出生位置和 `FReEchoStageTransitionDecision`。预警先按存活单位与其他待提交批次预留 `ActiveUnitLimit` 容量，只为真正获准提交的位置显示；每个预警位置都是出生承诺。普通怪与 Boss 的最终 Actor 中心位置 XY 都来自各自 `SpawnProfiles` 和统一 SpawnResolver，Z 为当前玩法平面加该敌人的碰撞半高；提交必须原样、完整复用预留位置，不得二次按单位上限裁剪，也禁止表现预警与生成提交各自重算。非法 Stage/Encounter 引用必须失败关闭。
- 生产波次结构：`Encounter.1` 至 `Encounter.8` 都按 `WaveIndex=1/2/3` 在 `0/10/20` 秒通过同一通用 WaveScheduler 触发；`Encounter.8` 只有 Wave.1 携带 `BossEnemyId=M_SHEEP`，Wave.2/3 是不含 Boss 的增援波。Boss 通过 `Spawn.Boss` 的双锚距离环解析出生位置，不再使用 GameMode 固定世界坐标；Boss 不计入该关 `ActiveUnitLimit`，其预留位置也不挤占普通增援容量。关卡仍以 `BossOrPlayerDeath` 结束，禁止在后两波重复配置或生成 Boss。
- 出生参数：SpawnResolver/Host 保留准确 `EncounterIndex`，使同一 EnemyId 在编译 Definition 时选择 `enemy_combat_stats` 的对应场次覆盖；不得把波次序号或数组下标误作 EncounterIndex。
- 连续性：同 Stage 保留存活 Enemy Host 的对象身份、EnemyId、SpawnIndex、Transform、生命和持久逻辑状态，并保留玩家位置；局间显式冻结 Host、取消旧攻击阶段和逻辑投射物，不消耗玩法冷却。跨 Stage 清理旧 Roster，并用 Arena Scene 的中心与玩法平面解析入口。玩家生命/属性在下一 Encounter 初始化时的既有语义不由此契约改变。
- 测试：`Source/ReEcho/Private/Tests/ReEchoStageTransitionTests.cpp` 的 `ReEcho.StageTransition.*` 覆盖生产矩阵、非法边界与原 Host 局间连续性。
- 禁止：持有构筑、货币、存档或 Widget 状态。

### `AREA-Run`：`Run`本局状态与存档

**设计意图：** 将跨遭遇但限于本局的阶段、构筑、背包、货币、商店、Echo 存储/回放选择和安全保存集中在 GameInstance Subsystem；卡牌内部状态与规则计算委托给 `MOD-ReEchoCards`。

- 代码：`Source/ReEcho/Public/Run/`、`Source/ReEcho/Private/Run/`。
- 角色能力：`Run/CharacterAbilities/ReEchoCharacterAbilityRuntime.*` 是统一类型化入口。猎手静态能力在新 Run 与角色晋升时作用于有效 StatBlock；诗人在成功 Encounter 完成时永久增长；智者在普通选卡计数达到配置间隔时追加选择。旧 Forge 阶段只作为旧存档迁移输入，并确定性转为普通 `CardChoice`，不再生成、展示或授予 Forge。
- 勇者缺血阶梯不写入 Run Save：Player Host 订阅 Combat 最终 `HealthChanged`，用当前/最大生命和能力表重算物攻/元攻加值，再通过 Combat 通用来源修正入口替换旧值。治疗、恢复和重生自然回退，不累计历史损血。
- 首读：`ReEchoRunSubsystem.*`、`ReEchoRunSaveGame.h`、`ReEchoShopCatalog.h`。
- 权威：Run phase/index、BuildSnapshot 提交、普通 Inventory、武器符文 OwnedPartIds、武器背包 OwnedWeaponIds、Time Shard/卡牌债务事务、Pending/Latest/Previous/Stored Echo、稳定回放 ID、SaveVersion 22；BuildSnapshot 内的 CardState 语义由 Cards 定义。
- 输入：Start/CompleteEncounter、购买、特质选择、Echo 命令、保存/继续。
- 输出：只读摘要、确定性 offer、保存结果和下一阶段。
- 卡牌授予成功并完整提交 `CurrentBuild` 后，Run 通过 `OnCardGrantCommitted` 发布只读 StatBlock 与类型化生命调整；
  GameMode 只把该命令转交 Player Combatant。失败事务不得发布，Widget 不订阅该事件反向改生命。
- 战后卡牌投放：Run 按当前 `EncounterIndex` 查询 `shop_drop_levels`；空 `FreeTier` 直接进入战后商店，有效 Tier 只从 Cards 提供的同 Tier `Trait` 资格池生成三选一。数据缺失或候选不足不得跨 Tier 回退，并安全转入商店。商店始终投影 `[1级卡组, 2级卡组, 3级卡组]` 三个固定入口，按真实关次读取 `ShopTiers` 逐级启用；每个启用卡组从同 Tier 资格池确定性缓存最多三张候选和一个同 Tier 基础价。入口底部购买键展示折扣后的卡组总价；付款事务一次扣费、对付款前已拥有卡牌触发一次 `OnPurchase`，立即保存并转为 `PaidPendingChoice` 后才进入三选一。返回商店不退款，入口变为“继续选择”，重进不收费、不重摇；最终领取只执行 `TryGrantCard`，成功才转 `Purchased`，不再次扣费或触发 `OnPurchase`。领取失败保留已付款状态、余额、候选和刷新用量以便重试。候选不足只显示实际 1/2 张，零张售罄，禁止跨级补位。初始投放页按 `EncounterIndex` 稳定；若页面生成后经其他效果新获得二、三级候选，则投影即时剔除该卡但不补抽、不重摇。免费和已付款三选一的每个实际候选槽都独立拥有 `shop_refresh_rules` 配置的刷新次数和价格；免费选卡及已付候选领取本身不收费，只有刷新扣款。成功刷新只原位替换所点槽位，并只重播该槽位的卡牌揭示动画；其他候选保持可见且不重播页面指针。替换保持同级，并排除全部已获得卡、当前候选以及当前这一组三选一自生成起曾展示过的全部卡；各商店Tier卡组和免费三选一分别拥有独立展示历史，新开一组才重置。无合法替代、余额不足、次数耗尽或禁刷新均原子失败。商店主刷新不得重建卡组或清除付款/已购状态。初次免费与商店投放仍允许已拥有的 1 级卡重复叠加，并排除已拥有的 2、3 级卡。SaveVersion 22 保存免费页和各Tier卡组的展示历史；旧版本只从当前候选重建最小历史。SaveVersion 20 新增卡组基础价与付款待选状态；v19 已购卡组迁移为已付款并按原页面身份惰性恢复稳定价格。SaveVersion 18 持久化当前免费投放页及逐槽用量，v17 持久化商店卡组和刷新用量；v16 自动补齐商店零用量，v15 及更早的单卡页缓存显式丢弃并确定性重建。
- 扩展：通过窄事务命令校验后一次更新；失败必须不产生部分状态。
- 商店武器/符文：`parts.csv` 的 `ShopEnabled/ShopPrice` 生成报价；三个槽以独立的 `WeaponRuneRefreshSequence` 缓存稳定 ContentId 与确定性价格，购买只改变已购/已装备状态，不得用缩小后的资格池重算其余槽。主刷新每个商店关次受 `shop_refresh_rules` 限制并显示剩余次数，只推进武器/符文页；卡牌 `FreeShopRefresh` 可优先抵扣价格但仍占用关次上限，`NoShopRefresh` 阻断两类刷新。下一关重置该预算，SaveVersion 17 持久化当前关次、序列和已用次数，读档不得重置。符文购买立即进入 `OwnedPartIds` 并装入对应槽，满槽时最早符文回背包。每次购买成功后 GameMode 都重取完整商店只读投影，使 `OwnedParts`、`EquippedParts`、武器、卡牌、货币和背包同步更新；稳定缓存保证该重取不会重摇未购买报价。初始武器和购买武器进入 `OwnedWeaponIds`；购买整把武器与背包换装都复用 `ReEchoWeaponRuntime::TrySelectWeapon`，只保留兼容符文，不兼容符文仍拥有但卸下。`TryEquipOwnedWeapon` 是不扣费、不刷新页面的窄事务，未知、禁用或未拥有武器不得改变构筑。SaveVersion 14 起持久化武器背包与稳定武器/符文商店页；v12 及更早存档以当前装备武器迁移最小拥有集合，v13 存档保留武器背包并重建商店页。
- 商店购买事务：武器、符文和兼容旧商品继续由 `PurchaseShopItemDetailed` 处理；卡组改由 `PurchaseShopCardPackDetailed(Tier)` 付款、`ClaimPaidShopCardChoice(ItemId)` 领取，候选 ItemId 不能绕过付款直接购买。三个接口都返回交易 ID、结果码、说明与实际价格，并以同一交易 ID 输出 `BEFORE / RESULT / AFTER`；快照包含碎片、当前武器、已装备符文、已生效卡牌、武器背包、符文背包和普通背包。统一审计出口直接追加 `Saved/Logs/ShopPurchaseAudit.log`，不依赖 Shipping 会裁剪的 `UE_LOG`；Development 另镜像到普通 UE 日志，玩家可见反馈仍由 UI 负责。
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

**设计意图：** 承接 Pawn 输入、移动、鼠标朝向和玩家侧组件装配，把输入翻译成领域命令，不复制 Combat/Run/Weapon 权威状态；相机由关卡 Arena Scene 拥有，Pawn 只读取实际 ViewTarget 的相机方向。

- 代码：`Source/ReEcho/Public/Player/`、`Source/ReEcho/Private/Player/`。
- 首读：`ReEchoPlayerPawn.*`。
- 输入：WASD、攻击、暂停/模式命令和世界边界。
- 输出：移动、相机/朝向表现、攻击请求和只读玩家状态；真实扣血的 Hurt 事件会让根 Box 在 1 秒内忽略 `Pawn` 移动碰撞，重复受伤顺延窗口但不提供伤害无敌，结束后恢复 Blueprint 作者碰撞响应。
- 扩展：新输入先确定命令所有者；菜单、死亡与模式切换必须统一释放 held 请求。
- 禁止：在 Pawn 再建一套攻击间隔、生命或 Run phase。

### `AREA-Presentation`：`Graybox` / `Presentation`世界表现

**设计意图：** 组装玩家、敌人、Echo、投射物、关卡场景、血条、伤害数字和命中特效等世界对象，消费逻辑状态与事件形成可见反馈。`Presentation/Animation2D` 通过 Profile、Catalog、Controller 与 Driver，把玩家 `AppearanceId` 和敌人 `PresentationId` 统一解析为 Gameplay Blueprint + Flipbook；通用运行时代码不按具体角色、敌人、Boss 或生成序号硬加载资源。

- 代码：`Source/ReEcho/Public/Graybox/`、`Private/Graybox/`、`Public/Presentation/`、`Private/Presentation/`。
- 首读：`Presentation/Animation2D/*`、`ReEchoPlayerPawn.*`、`ReEchoEnemyActor.*`、`ReEchoEchoActor.*`、`docs/2D_SEQUENCE_ANIMATION.md`。
- 权威：只拥有表现实例与表现生命周期；逻辑生命、伤害、攻击节奏和录制不归这里。
- 输入：Combat/Weapon/Recording 结果、只读快照、`AppearanceId`、武器视觉 Key 和稳定视觉 ID。
- 输出：Sprite/Mesh/材质、动画、VFX、世界文本和镜头反馈。
- 战斗 VFX：详细设计见 [`MOD-ReEchoVFX.md`](MOD-ReEchoVFX.md)。`UReEchoCombatVfxComponent` 装配在 Player、Echo 与 Enemy Host，只读订阅 Combat/Enemy 类型化事件；三类 Host 的 `EffectsRoot` 下分别暴露 Blueprint 可编辑的 `AttackVfxRoot` / `HurtVfxRoot`，Echo 另有按 Flipbook 稳定视觉 Bounds 居中并处于角色下一排序层的 `EchoAuraVfxRoot`。Run/GameMode 只在 Cards 的同一权威 `EchoAuraPulseCount` 到达时播放一次水草 Aura，不新增视觉计时器，2 秒/4m 元素玩法链不变；`FReEchoCombatVfxCatalog` 是语义到完整 Niagara/材质/纹理路径的唯一映射。兔子每颗可见子弹以 `(AttackIdentity, VolleyBallIndex)` 一一跟随 EnemyHost 逐球快照，并按事件碰撞半径绘制相同尺寸的核心球与红色柔光材质；表现不拥有轨迹、碰撞、伤害或存档。
- 战斗表现协调（Plan77）：Enemy Host 上的 `UReEchoCombatPresentationCoordinator` 将特殊动作归一为由来源、序列和 AbilityId 标识的有序阶段。动画与 VFX 不再各自订阅并解释原始 SpecialAction，而是共同消费同一 Windup/Committed/Ended/Cancelled 广播；Coordinator 只拥有去重和可丢弃的表现生命周期。战斗主体 DA 从 Plan78 起统一位于 `/Game/ReEcho/DataAsset/{Character,Enemy,Weapon,Common}`，角色、怪物和武器 Catalog 不再混放。
- 比例契约：人物 `100 UU`；Grunt/史莱姆 `80 UU`；Rabbit `140 UU`；Fox `200 UU`；Goat/Boss `220 UU`。Animation2D Profile 拥有外观目标高度；Gameplay Blueprint 独立拥有可编辑 Box Collision 的尺寸和变换，运行时代码不得按外观高度覆盖碰撞；同一 Profile 的不同语义序列必须同尺寸。
- Arena Scene：`Presentation/Scene/ReEchoArenaSceneActor.*` 是 `/Game/Level00` 中 Editor authored 的场景契约，独立拥有可替换 `MapMaterial`、相机安全边界、玩家活动边界和敌人出生边界；不再持有或采样地图 AO。`UReEchoArenaSceneProfile` 只拥有地图材质、Ground 调色和插片白名单/密度/固定种子/中央安全区等表现默认值；Actor 的 `SceneProfile` 在 Construction 中建立地图 MID 并应用 `GroundTint/GroundBrightness/GroundSaturation/GroundContrast`，但 Profile 不拥有玩法边界、碰撞、遭遇或关卡流程。`ArenaCamera` 独立直属 `SceneRoot`，直接拥有可编辑的位置、旋转、正交宽度和宽高比，不继承地图 Transform。默认中心锁定模式在地图安全区内让相机地面焦点严格跟随玩家脚点，边缘/角落仍以地图 Clamp 为先。`MapRoot` 是地图、碰撞和玩法范围的 Editor 平移/缩放节点；Backdrop、Floor 和四墙是其直接子节点，`OnConstruction()` 会修复旧关卡实例遗留的 Attachment。`PlantRoot` 位于 `GroundDetailRoot` 下，Level00 中烘焙的植物卡片可单独选择、移动、缩放、替换材质或删除；Plan52 工具使用 Profile 固定种子并只清理带 `ReEchoGeneratedDecoration_Plan52` 标签的卡片。GameMode、相机 Clamp、脚点排序与视差读取 MapRoot 世界中心，玩家/出生/相机范围消费其 XY Scale。`GameplayPlaneZ` 是 MapRoot 局部脚底平面，统一转换为世界高度；玩家和怪物 Actor 中心始终为该高度加各自 Box 半高，GroundRoot/阴影因此贴合相同平面。`ReEcho2DSceneLightingComponent` 只维护脚点透明排序，不修改角色明暗。Backdrop 与 Collision 各有独立 Auto Layout 开关；关闭后 Construction 保留关卡实例中直接编辑的组件 Transform。GameMode 只验证唯一实例并消费边界；相机按组件当前倾斜正交视锥投影到同一玩法平面的实际 footprint 跟随/钳制，不在运行时生成备用场地或覆盖 Editor 相机参数。
- Animation2D 状态：Profile 只选择角色真实拥有的 Flipbook 语义 Clip，并以目标 `WorldHeight` 统一归一化同一外观所有序列；共享状态机包含 Move（Walk）、Attack.Charge、Attack.Basic、Hit、Transform.Phase2、Death。Move 是可选基础循环，其余状态也是可选能力；缺失语义为合法 no-op，保持当前有效表现。Charge 是由玩法事件显式结束的循环动作，Basic/Hit/Transform/Death 为一次性动作；Transform 的锁定优先级高于 Hit/Attack，Death 是终结状态，非终结动作结束后尝试返回 Move。敌人 Actor Rotation 始终保持 Identity，玩法朝向只读 `EnemyLogic` 的 `FacingDirection`，表现仅由相机面片和 Renderer 左右镜像解释。玩家和敌人的 `GroundRoot` 都直属稳定的 `FootRoot`；两类 `PresentationMotionRoot` 都按 `AuthoredMotionLocation + CalculatedFootAlignmentOffset + TransientMotionOffset` 定位，共用当前 Flipbook 完整 RenderBounds、Renderer 缩放/镜像和 FlipbookRoot 相机面片角度的脚点计算，使 authored 非零时底边中心仍严格落在 `FootRoot + Profile.FootpointOffset`。两类 GroundShadow 共用 `UReEcho2DAnimationComponent` 的表现宽度计算：保留 Blueprint authored 的纵深、Z、材质、透明度和地面旋转，只将 XY 同步到当前 Flipbook 底边中心，并将屏幕横向宽度按实际变换匹配；阴影不继承 FlipbookRoot、瞄准或 Actor 的旋转。`FlipbookRoot` 根据当前实际 ViewTarget 相机旋转对齐，并以相机 Right 轴解释左右朝向。玩家和敌人各自只保留一个可继承编辑的 `GroundShadow`；玩家、Grunt、Rabbit、Fox、Goat 分别绑定 `MI_Shadow_*` 材质实例，通过实例的 `ShadowOpacity` 调整强度。运行时表现更新不创建 MID；阴影不进入碰撞或动画状态。Renderer 统一使用 Paper2D 透明 Unlit Sprite 材质消费源纹理 Alpha。
- Echo 动画映射：`AReEchoEchoActor` 复用 Player 的 `PresentationRoot → FootRoot → PresentationMotionRoot/GroundRoot` 组件树以及 `Catalog/Profile/Controller/UReEcho2DAnimationComponent` 执行链。独立 `DA_EchoPresentationCatalog` 按录制快照中的规范角色 ID 将 `J_HEART/J_SPADE/J_CLOVER/J_DIAMOND` 一一解析到对应 Echo Profile；默认与移动使用各自 `Walk`，默认近战攻击使用 `Attack`，Bow/Gun 与非生产辅助 `MoonStaff` 的 `WeaponVisualKey` 动画集覆盖为 `Attack_Arrow`。Echo Host 只提交移动、朝向和成功攻击语义，Controller 负责单次播放结束回到当前基础状态；原 Billboard 仅在 Profile/Flipbook 缺失时作为不影响玩法的回退。Echo 的武器瞄准与 Player 一样使用独立 `AttackAimDirection`，不再通过旋转 Actor 根节点驱动武器或 Flipbook。
- 碰撞边界：根 Box Collision 始终拥有移动 Sweep、阻挡、导航和位置记录权威；`BP_PlayerGameplay` 与四个 `BP_EnemyGameplay_*` 是尺寸/Transform 的 Editor 权威，GameMode 实际生成这些类。匹配序列的 PaperFlipbook 可使用 `EachFrameCollision` 提供 `QueryOnly` 身体轮廓，但不得推动 Actor 或替代根 Box。
- 命中边界：语义 Body Hurtbox 与 Weapon AttackHitbox 由独立帧轨道表达。只有已提交攻击的只读身份和轨道 active frame 能开放攻击查询；动画时间、像素 alpha、Paper2D 内建碰撞和播放完成都不能产生或裁决伤害。
- 资产：运行时 Texture2D、PaperSprite、Flipbook 与同目录源图位于 `Content/ReEcho/Art/Animation2D/{Players,Echos,Enemies}/`；Character/Enemy Profile 与 Common FSM 位于 `Content/ReEcho/DataAsset/{Character,Enemy,Common}/`。导入与只读审计工具位于 `scripts/ue/`。
- Gameplay Blueprint 表现树：四个玩家共享 `BP_PlayerGameplay`；Grunt、Shield、Bomber、Slime、Rabbit、Fox、TimeGuard 分别使用 Catalog 绑定的 `BP_EnemyGameplay_*`，但全部继承同一原生 Host/组件契约。根 `Collision` 保持移动碰撞权威；其下统一为 `PresentationRoot → FootRoot → {PresentationMotionRoot, GroundRoot}`，MotionRoot 下再挂 `FlipbookRoot` 与 `EffectsRoot`，GroundRoot 独立拥有 GroundShadow。子 Blueprint 只调整碰撞、比例、Tint、阴影、脚点和挂点，不拥有 AI、伤害或动画状态机真相。
- Transform 权威：Gameplay Blueprint 的 `CharacterScale` 是玩家和怪物整个 Actor 的统一尺寸入口，等比作用于根 Box Collision、表现、阴影、特效和未来新增子节点。全部组件使用相对父节点的 Transform，不设置绝对位置、旋转或缩放。`PresentationRoot -> FootRoot` 是共享脚点根；MotionRoot 只驱动 Flipbook/Effects，GroundRoot 独立保持地面旋转并从 Flipbook 底边中心同步平面位置和宽度。Move 与 Attack 完全由 Flipbook 表现；Hit 可在 MotionRoot/Flipbook 分支播放短暂受击反馈，阴影只同步平面结果而不参与高度、面片旋转或形变。敌人血条是独立表现 Actor，但只消费 `FootRoot` 世界脚点、Profile 高度和宿主 `CharacterScale`，其位置与显示尺寸随 Gameplay BP 总缩放同步。玩家瞄准只更新独立逻辑 `AttackAimDirection` 与 Flipbook 横向镜像，不再旋转整个 Actor；武器读取该逻辑方向，碰撞与阴影不会因瞄准旋转。
- 比例边界：Profile `WorldHeight` 只负责同一外观各 Flipbook 序列之间的基础归一化；Gameplay Blueprint 的 `CharacterScale` 负责碰撞、角色、阴影、特效及全部挂点的最终整体比例。局部 `FlipbookRoot`、`GroundRoot`、`EffectsRoot` 只作分层位置/朝向微调，不得再用局部 Scale 调整角色整体尺寸。场景整体比例由 Arena Blueprint 的 `MapRoot` 独立拥有，相机不继承 MapRoot。
- 环境融合：每个 Gameplay Blueprint 的 `FlipbookRenderer.CharacterTint` 是角色/怪物与地图、植物卡片匹配色调的轻量入口；只影响 Paper2D 颜色，不改变源纹理、动画、碰撞或玩法状态。
- Editor 构图预览：`ReEcho2DEditorPreviewActor` 是 EditorOnly 的单一 ChildActor 宿主，直接实例化真实 Gameplay Blueprint 类；目标类不变时不重建 ChildActor，避免 Construction 累积重复实例。ChildActor 标记为 Visualization Component，并在 BeginPlay 显式销毁，保证不会把碰撞、AI 或表现带入 PIE/Cook。
- VFX 测试构图：`ReEchoVfxPreviewActor` 与 `/Game/ReEcho/Testing/VFX/L_VFXAuthoring` 只读组合生产 Catalog/Profile；Editor 模式提供单特效和 `NOT APPLIED` 多目标校准。Conduct Production 仅在 PIE 临时生成真实 Enemy/Combatant，并走正式元素命中/事件/VFX 链路；退出 PIE 清理宿主。测试地图不属于默认地图或生产引用链。
- Gameplay Blueprint 编辑器：玩家 Blueprint 与七个敌人 Blueprint 必须维持同一继承组件契约，并可使用挂在 `PresentationMotionRoot` 下的 `ArtAuthoringRoot` 作为美术扩展入口；完整编辑器中仍可选择继承的 Collision、FlipbookRoot、GroundShadow 与 EffectsRoot。
- 角色/存档边界：生产角色只有 `J_SPADE/J_DIAMOND/J_CLOVER/J_HEART`。SaveVersion 11 把旧版本 CurrentBuild、所有 Echo Recording 和局中 ActiveRecording 内的 `J_CAT/J01` 迁移为 `J_SPADE`；新存档只写规范 ID。
- Boss 表现边界：`Enemy.TimeGuard` 使用普通敌人相同的 Catalog/Profile/Gameplay Blueprint/FSM 路径；不存在 `Boss2D` 静态贴图特例。专属动画未交付时仅由 `DA_Enemy_TimeGuard` 显式复用 Goat 动画。
- 阴影渲染层：GroundShadow 与 Flipbook 共享 MotionRoot 只解决位置同步，前后遮挡由整数 `TranslucencySortPriority` 明确控制。玩家与全部怪物阴影固定为 `-10`，角色 Flipbook 使用 Profile/Clip 的非负表现层，禁止使用会被截断为零的小数排序值，确保阴影始终绘制在角色下层。
- 旧架构清理：`ReEcho2DVisualPrefabActor` 类型与 `/Game/ReEcho/Animation2D/VisualPrefabs/**` 蓝图资产已删除。角色表现只能从真实 Gameplay Blueprint 扩展，禁止重新引入运行时第二 Actor 或另一套表现组件树。
- 场景 Prefab：通用 `/Game/ReEcho/Scene/Prefabs/BP_ArenaScene` 继承 `ReEchoArenaSceneActor` 稳定契约，`BP_ArenaScene_SC01..04` 是只选择对应 `DA_ArenaScene_SC01..04` 的薄子 Blueprint；Level00 当前放置 SC02 子类。四张唯一权威地图源图/Texture2D 为 `/Game/ReEcho/Art/Scene/Map/sc01..04`，统一由 `M_ArenaGround` 与 `MI_SC01..04` 消费。正式 Level00 的 Arena Actor 与 `MapRoot` 位于世界原点，`GameplayPlaneZ=0`；Backdrop 位于地面下方极小偏移，Floor 顶面与地面平面对齐但不阻挡 Pawn，角色高度由玩法平面保持，只有四面墙承担移动阻挡。`author_plan52_decorations.py` 从 ArenaCamera 计算卡片朝向，按 Profile 中央安全区拒绝高卡片落点，并以三档尺寸和脚点排序烘焙可单独编辑的 StaticMeshActor；Bake/Clear 只替换 Plan52 专用标签集合，不触碰手工作品。美术可在 Blueprint 的可选视觉层继续新增组件；相机、MapRoot、玩法范围和碰撞仍由类型化原生接口供 GameMode 消费。
- 2D表现FSM：`ReEcho2DAnimationStateMachineAsset` 保存完整七态语义、可中断优先级和播放完成去向；`ReEcho2DPresentationController` 是纯 Flipbook 执行器并保留 Gameplay 宿主的稳定意图 API。敌人特殊动作 `WindupStarted -> Charge`、`ActionCommitted -> Basic`、`ActionEnded -> CancelAttackAction`；Boss 阶段事件以锁定 Transform 过渡后切换 Phase2 AnimationSet。Profile 负责绑定 FSM 与 Appearance/WeaponVisualSet Clip，状态机只选择 Flipbook 和表现状态，不通过动画帧或完成回调反向驱动伤害、移动、AI 或根 Box Collision。
- 扩展：表现缺失、提前结束或加载失败必须不改变玩法；Animation2D 不依赖具体角色枚举，也不通过回调反向控制 Combat/Weapons。新增关卡场景应复用类型化 Arena Scene 契约，并由 Editor 维护关卡资产，不在 GameMode 增加路径或 Actor Label 分支。
- 测试：`ReEchoArenaSceneTests.cpp` 覆盖正交视锥地面 footprint、中心跟随、四边/四角 Clamp 与地图小于视野时的中心锁定；场景 Actor 唯一性和资产绑定由 Editor 自动化与人工 PIE 验收。
- 本地表现验收入口：`scripts/ue/Verify-And-Open-Editor.cmd` 顺序执行 Editor 构建、项目静态校验和 `git diff --check`；只有全部通过才启动 ReEcho Editor，供用户继续 Editor/PIE 人工检查。发布仍使用独立的 `Build-Editor.cmd -FullRebuild` 门禁。

### `AREA-UI`：`UI`屏幕与交互

**设计意图：** 通过稳定屏幕 ID、中央 Widget 注册、统一焦点/输入/暂停策略管理开始、装载、HUD、特质、商店、Echo 管理、属性和暂停等界面。

- 代码：`Source/ReEcho/Public/UI/`、`Source/ReEcho/Private/UI/`。
- 首读：`UI/Framework/*`、`ReEchoUIManagerSubsystem.*`、各屏幕 Widget、`ReEchoGameMode` 的类型化端点。
- 权威：活跃屏幕实例、Viewport 层、焦点、输入模式和屏幕暂停策略。
- 输入：玩法只读摘要、用户选择和稳定 ID。
- 输出：类型化命令，不直接写 Run/Combat/Weapon 内部状态。
- 扩展：新增屏幕先注册 `EReEchoUIScreen` 与生命周期策略；GameMode 不直接管理 Widget Viewport。
- 商店/背包页保留全屏背景，并把固定 `1920×1080` 作者坐标的交互内容放入统一等比缩放设计面；WBP 控件和运行时弹层必须共享同一缩放坐标系，避免低分辨率裁切或点击区域错位。武器背包和符文背包共用该设计面的根级高层浮层，不能继续嵌在装配室局部 Canvas 下被兄弟表现层遮挡。
- 商店/背包页按 `P` 时由更高 Pause 层覆盖，商店保持打开；恢复后重新聚焦商店并保持暂停，不得复用商店关闭路径或触发战后推进。
- 战斗世界空间元素反应字由主模块 Enemy Presentation 订阅 Combat 的权威反应完成事件后生成；五类透明图及渐隐材质属于 UI 表现资产，`BP_ReEchoElementReactionPopup` 是尺寸、上浮、渐隐与缩放的调参入口，缺失时不得阻断玩法。
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
| 2D 序列动画/逐帧 Query 轮廓 | `Presentation/Animation2D/*`、`PlayerPawn.*`、`EnemyActor.*` | `docs/2D_SEQUENCE_ANIMATION.md`、`docs/ART_ASSET_ORGANIZATION.md`、`Content/ReEcho/Art/Animation2D/`、Profile/Catalog 与导入工具 |
| Actor/VFX 接入 | 逻辑结果契约、Presentation/Graybox Actor | 资产路径与对应表现 Plan |

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

- PlayerPawn、EchoActor 和 EnemyActor 是 Combat 阵营的世界适配器：玩家/回响返回 `PlayerSide`，敌人返回 `EnemySide`；主模块只声明身份和事件来源，不复制 `CanDamage` 规则。回响武器归因为 `DamageSource::Echo`，但与玩家共享玩家阵营。

- `AReEchoGameMode` 是编排器，不是各领域状态仓库。
- Run/Combat/Weapon/Recording/UI 各自只有一个权威写入口。
- 自动攻击不进入 Recording；Echo 在当前世界重新选目标与命中。
- 保存失败不得退出；购买/装备/选择失败不得产生部分状态。
- 表现资源和完成回调不能控制确定性逻辑。
- Enemy Host 在 Profile 装配后尝试 Born，并把 Presentation 的只读 Born 活跃状态转换为 Enemy Sense 的 Phase2 启动许可；该许可不参与出生提交或普通行为。致命伤发生于 Born 时不截获为 Transform，使 Combat 正常死亡并由 Death 表现立即抢占。
- 不从旧 JSON、描述文本、Widget 缓存或 Actor 表现字段恢复第二份事实来源。

## 运行时 CSV 松散文件与 Shipping 打包契约

`Content/Data/*.csv`（由 `reecho_data_manifest.csv` 列示的 28 张运行时表，含 `stages/attributes/encounters/encounter_waves/spawn_policy/spawn_profiles` 等）是**纯松散文件**，运行时由 `ReEchoCsvDataRegistry` 经 `FFileHelper` 按物理路径直接读取，不经由 UE 资产系统。这带来一项打包约束：

- **cook 只枚举被资产引用的文件**。`Content/Data/*.csv` 没有任何 `.uasset`/`.umap` 引用（已用全量二进制扫描确认，连已进包的 `characters.csv`/`weapons.csv` 也不被任何资产内嵌），因此 cook 不会把它们作为资产依赖暂存进包。
- 历史打包中出现了"部分 csv 进包、部分不进"的偶发结果（22 进、6 缺），这是 cook 内部行为的非确定性副作用，**不能作为数据齐备的保证**；缺表会在启动期触发 `ReEcho.cpp` 的 `LowLevelFatalError`（`...: File could not be read`，退出码 3）。
- **确定化投递**：`scripts/ue/package_windows.py` 在 `BuildCookRun` 的 archive 完成后，显式把 `Content/Data/*.csv` 拷贝进最终包 `ReEcho/Content/Data/`（见其 `stage_runtime_csvs`）。该函数排除 `Engine` 目录下的 `Content`，只写入游戏模块目录，保证 manifest 列示的 28 张 csv 全部就位，与已进包的 22 张走同一松散文件机制。
- 修改 csv 集合时：保持 `reecho_data_manifest.csv` 为真源；`package_windows.py` 不写死表名，按目录通配拷贝，因此新增/删除运行时 csv 无需改脚本。
- 此契约仅影响打包投递，不改变 Development/PIE 既有的松散文件读取路径，也不改变 CSV schema、稳定 ID 或 `ReEcho.cpp` 的启动校验逻辑。
# Plan73 元素表现装配

运行时 Host 通过 `UReEchoCombatVfxComponent` 订阅元素状态与反应完成事件；玩法保持权威，Host 只按敌人 Definition 的稳定 `PresentationId` 选择、生成和清理 Niagara。旧元素状态表现三件套 `ElementAuraRing + ElementAttachmentLabel + ElementAuraLight` 及其更新、朝向和脉冲契约已整体删除，元素附着与反应表现只走 Niagara。
元素附着为 Encounter 作用域：同 Stage 保留的敌人在进入局间暂停时、Player 在进入局间时均重置元素战斗态并广播最终状态，确保绑定 Niagara 同步清理。
