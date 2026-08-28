# ReEcho 当前全局架构

本文只描述当前 `main` 已存在的全局拓扑、依赖方向和跨模块不变量。各模块的详细设计意图与代码位置从 [`README.md`](README.md) 进入。

## 项目形态

ReEcho 是 Unreal Engine 5.8 的 2.5D 时间回响肉鸽原型。角色、敌人与 Echo 使用 2D 表现，移动、碰撞、攻击和世界流程运行于 3D 场景；`/Game/Level00` 当前承载运行时生成的有界竞技场与八场表驱动遭遇。

## 当前 Runtime Module

| 架构标识 | Runtime Module | 存在原因 | 详细文档 |
|---|---|---|---|
| `MOD-ReEcho` | `ReEcho` | 作为 UE 玩法装配根，组合世界生命周期、主流程、尚未独立的玩法领域、表现和 UI | [`modules/MOD-ReEcho.md`](modules/MOD-ReEcho.md) |
| `MOD-ReEchoPresentation` | `ReEchoPresentation` | 隔离角色与怪物共享的 2D Profile、状态机、渲染和逐帧 Query/Debug，使表现资产不反向进入玩法模块 | [`modules/MOD-ReEchoPresentation.md`](modules/MOD-ReEchoPresentation.md) |
| `MOD-ReEchoAudio` | `ReEchoAudio` | 隔离音频目录、策略、总线和播放后端，使缺资源或无设备不影响玩法 | [`modules/MOD-ReEchoAudio.md`](modules/MOD-ReEchoAudio.md) |
| `MOD-ReEchoCombat` | `ReEchoCombat` | 集中攻击控制、战斗参与者、GAS、命中/元素/生命/死亡裁决和只读结果契约 | [`modules/MOD-ReEchoCombat.md`](modules/MOD-ReEchoCombat.md) |
| `MOD-ReEchoCards` | `ReEchoCards` | 集中卡牌定义、构筑状态、抽取资格与无世界规则计算 | [`modules/MOD-ReEchoCards.md`](modules/MOD-ReEchoCards.md) |
| `MOD-ReEchoWeapons` | `ReEchoWeapons` | 集中武器不可变定义、唯一普通攻击节拍、步骤和无表现的攻击载体逻辑 | [`modules/MOD-ReEchoWeapons.md`](modules/MOD-ReEchoWeapons.md) |
| `MOD-ReEchoEnemies` | `ReEchoEnemies` | 集中怪物 AI、攻击节奏、爆破引信、受击位移与行为快照，避免表现资源成为玩法前置 | [`modules/MOD-ReEchoEnemies.md`](modules/MOD-ReEchoEnemies.md) |

```text
MOD-ReEcho ─────────────→ MOD-ReEchoAudio
    ├───────────────────→ MOD-ReEchoPresentation
    ├───────────────────→ MOD-ReEchoCombat
    ├───────────────────→ MOD-ReEchoCards ──────→ MOD-ReEchoCombat
    ├───────────────────→ MOD-ReEchoWeapons ─────→ MOD-ReEchoCombat
    └───────────────────→ MOD-ReEchoEnemies ─────→ MOD-ReEchoCombat

MOD-ReEchoCombat  ─/─→ MOD-ReEchoWeapons / MOD-ReEcho / MOD-ReEchoAudio
MOD-ReEchoCards   ─/─→ MOD-ReEcho / MOD-ReEchoWeapons / MOD-ReEchoEnemies / MOD-ReEchoAudio / UI
MOD-ReEchoWeapons ─/─→ MOD-ReEcho / MOD-ReEchoAudio
MOD-ReEchoAudio   ─/─→ MOD-ReEcho / MOD-ReEchoCombat / MOD-ReEchoWeapons
MOD-ReEchoEnemies ─/─→ MOD-ReEcho / MOD-ReEchoWeapons / MOD-ReEchoAudio / Presentation
MOD-ReEchoPresentation ─/─→ MOD-ReEcho / MOD-ReEchoEnemies / MOD-ReEchoCombat / MOD-ReEchoWeapons
```

依赖必须保持单向。Cards 计算构筑状态、候选资格和类型化规则结果，但不接触世界；Weapons 和 Enemies 可以产生攻击提交或命中候选，但只有 Combat 能形成最终伤害、元素、生命与死亡结果；主模块负责把结果装配到世界 Actor、表现、UI、音频、Run 与 Recording。`ReEchoPresentation` 只消费稳定表现 ID 与命令，不引用玩法 Actor；音频和表现只消费结果，不决定攻击、命中或流程是否成功。

战斗 VFX 仍属于 `MOD-ReEcho/AREA-Presentation`，以[战斗 VFX 文档入口](modules/MOD-ReEchoVFX.md)维护。Combat/Weapons/Enemies 只发布资源中立的提交、最终伤害、特殊动作阶段和逻辑投射物生命周期；主模块 VFX 适配器选择 Niagara、纹理等表现资产并管理实例。粒子碰撞、播放完成、加载成败、朝向轴和透明层级均不得反向影响玩法。

## 主运行流程

```text
启动 / Continue
  → 角色与初始武器选择
  → AReEchoGameMode 创建竞技场、玩家、EncounterDirector 与可用 Echo
  → 60 Hz 暂停感知固定步推进遭遇
  → Encounter Catalog 在 0/10/20 秒发布预警和 Spawn Intent
  → Spawn Resolver 以玩家预测/Echo 录制路径双锚确定出生点
  → 玩家位置与成功主动技能按 20 Hz 录制
  → 普通战 30 秒完成；同 Stage 保留存活敌人，跨 Stage 清理；Boss 由胜负完成
  → 特质构筑 → Time Shard 商店 → Echo 存储/单一时间锚点
  → 下一场遭遇；最终 Boss 结束整局
```

`AReEchoGameMode` 只负责编排跨领域步骤；每个领域的状态仍由其权威组件或 Subsystem 持有，GameMode 不保存第二份真相。

## 跨模块状态流

| 状态/结果 | 权威拥有者 | 其他系统如何使用 |
|---|---|---|
| 本局阶段、构筑、背包、Echo 存储与保存 | `UReEchoRunSubsystem` | 发送窄命令、读取摘要，不直接改字段 |
| 当前生命、属性、格挡、元素状态 | `MOD-ReEchoCombat` 的 Combatant/GAS/ElementRuntime | UI/表现读取 Snapshot 或订阅 CombatEvents |
| 角色能力配置与跨遭遇永久进度 | `MOD-ReEcho/AREA-Data` 的 `CharacterAbilities` 快照与 `AREA-Run/CharacterAbilities` | Run 在类型化生命周期触发；不解释角色描述文本 |
| 当前缺血角色能力的临时攻击修正 | `MOD-ReEchoCombat` 的 Combatant/GAS | Player Host 将最终 HealthChanged 适配为按来源替换的通用攻击修正；Combat 不认识角色 ID |
| 自动/手动 held、目标与攻击请求 | `MOD-ReEchoCombat` 的 AttackController/Targeting | Pawn、菜单和 Run 只发送受控命令 |
| 武器定义、攻击步骤、唯一节拍与逻辑载体 | `MOD-ReEchoWeapons` | Player/Echo 发请求；Weapons 只产生 Commit/HitIntent |
| 怪物类型、行为阶段、攻击冷却、爆破引信、受击位移与攻击序号 | `MOD-ReEchoEnemies` 的 EnemyLogic | EnemyHost 显式提供 Sense、应用移动并把攻击候选交给 Combat；表现只读事件/快照 |
| Stage、Encounter、Wave 门、确定性出生候选与普通怪全局技能令牌 | `MOD-ReEcho` 的 Encounter Catalog / WaveScheduler / SpawnResolver / GameMode coordinator | 预警锁定位置；远程窗口与精英并发统一授权；Enemies 只消费许可 |
| 最终命中、伤害、元素、击杀与死亡 | `MOD-ReEchoCombat` 的 HitResolver | Weapons/Enemy 提交 HitIntent；其余系统消费结果 |
| 玩家历史与 Echo Playback | Recording/Playback | 世界流程启动/停止，当前世界重新选目标与结算 |
| 屏幕实例、焦点、输入模式与暂停策略 | UI Manager/Flow Coordinator | GameMode 发送屏幕命令，不直接管理 Viewport |
| 音频目录、总线、音乐/环境状态与播放实例 | `MOD-ReEchoAudio` | 主模块发送语义请求，不读取播放内部状态决定玩法 |
| 战斗 VFX 实例、语义资产映射与视觉跟随索引 | `MOD-ReEcho/AREA-Presentation` 的 VFX 适配器 | 只读消费 Combat/Enemy 事件；逻辑投射物位置、命中和有效性仍归 Enemies/Host；逐球可见代理只投影逻辑位置 |

## 数据权威流

```text
Design/Data/ReEchoData.xlsx + ReEchoEnemyData.xlsx + ReEchoEncounterData.xlsx + ReEchoAudioEvents.xlsx
  → scripts/data/sync_xlsx_to_csv.py
  → Content/Data/*.csv
  → 类型化 CSV Reader / FReEchoCsvDataRegistry
  → 本局固定的不可变运行时快照
```

- XLSX 是已迁移领域的策划编辑源；CSV 是可 diff、可打包的运行时源。
- 角色基础行与角色能力子表分离：`tblCharacters → characters.csv`，`tblCharacterAbilities → character_abilities.csv`；能力逻辑只允许注册行为，旧 Forge 不属于生产契约。
- Unreal 运行时不读取 XLSX，也不执行表格自由文本。
- Behavior、Formula、Effect 与 AttackPattern 通过稳定 ID 映射到注册实现。
- 已迁移领域的旧玩法 JSON 已从 `Content/Data` 删除并由校验器禁止回归；历史快照只从 Git 读取，不能成为第二事实来源。

## 跨模块不变量

- 依赖指向权威逻辑或底层服务，底层模块不得反向依赖主流程或表现。
- 每种运行时状态只有一个权威拥有者；其他模块只读快照、订阅事件或发送受控命令。
- 逻辑产生结果；Animation、VFX、Audio 和 UI 只消费结果，表现完成回调不控制玩法。
- 稳定 ID、存档格式、CSV Schema、公共 API 和生成器属于共享契约，破坏性修改必须显式协调。
- `.uasset`/`.umap` 承载宿主地图和资产引用，不保存可以由 C++ 或权威数据明确表达的第二份玩法规则。
- 模块缺失可选资源或外部设备时必须安全降级，不得导致确定性玩法失败。

## 当前候选状态

Plan47 新增第六个 Runtime Module `ReEchoCards`、39 张构筑和版本化 CardState；Cards 只依赖 Combat，Weapons/Enemies 不反向依赖 Cards，主模块负责世界、商店与存档适配。Plan48 的 Encounter Catalog、WaveScheduler 与 SpawnResolver 仍属于 `MOD-ReEcho/AREA-Encounter`；它们只向 EnemyHost 提供数据和 Spawn Intent，没有让 `ReEchoEnemies` 反向依赖主模块、工作簿或表现资源。

## 维护规则

Plan 中形成的局部方案、替代方案和验证历史留在 Plan。通过验收并合入的模块存在原因、职责、状态所有权、公共契约、依赖方向、代码落点和跨领域不变量必须晋升到本目录。
