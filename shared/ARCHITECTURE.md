# ReEcho 项目架构与模块设计意图

本文件是项目当前有效架构和模块设计意图的权威说明。它回答“系统为什么这样拆、谁拥有状态、依赖应指向哪里”。

- [`CODEBASE_MAP.md`](CODEBASE_MAP.md) 使用与本文件相同的稳定架构标识，回答每个模块/领域的“代码在哪里”，不重复设计理由。
- `plans/<id>-*.md` 记录一次任务中的决策过程、替代方案和验证历史。
- 源码、测试和运行时数据仍是具体行为事实来源；本文件不得描述尚未合入 `main` 的目标架构。
- 每个 Plan 关闭时必须审阅本文件：架构变化应同步更新；没有变化则在 Plan 中记录已审阅、无需修改及原因。

## 项目形态

ReEcho 是 Unreal Engine 5.8 的 2.5D 时间回响肉鸽原型：角色、敌人和回响使用 2D 表现，移动、碰撞、攻击与世界流程运行在 3D 场景中。`/Game/Level00` 承载运行时生成的有界竞技场和六场遭遇流程。

当前 `main` 声明两个 Runtime Module：

| 架构标识 | Runtime Module | 依赖角色 |
|---|---|---|
| `MOD-ReEcho` | `ReEcho` | UE 玩法装配根，依赖 `ReEchoAudio` |
| `MOD-ReEchoAudio` | `ReEchoAudio` | 独立音频服务，不得反向依赖 `ReEcho` |

```text
ReEcho ─────────→ ReEchoAudio
  │
  ├─ 主流程、世界装配、战斗、武器、数据、存档、录制与 UI
  └─ 把语义音频请求适配给独立音频模块

ReEchoAudio ─/─→ ReEcho
```

依赖必须保持单向。音频失败不得阻塞或改变玩法。Plan41 正在本地 Review `ReEchoCombat` 与 `ReEchoWeapons` 的进一步拆分；在其通过人工验收并合入 `main` 前，它不是本文件中的当前架构。

## Runtime Module 设计意图

### `MOD-ReEcho`：`ReEcho`

**存在原因：** 作为当前玩法装配根，将 UE 世界生命周期、主流程和尚未独立成模块的玩法领域组合起来，并把数据、逻辑与表现接到具体 Actor、Subsystem 和 Widget。

**当前职责：**

- `AReEchoGameMode` 编排开始/继续、装载选择、竞技场、遭遇、局间构筑、商店和结束流程。
- `UReEchoRunSubsystem` 持有本局阶段、构筑、存档、Echo 存储与回放选择。
- GAS/Combatant、武器和元素反应执行当前战斗规则。
- Recording/Playback 捕获玩家历史并驱动 Echo。
- UI Framework 管理屏幕身份、创建/关闭、焦点、输入模式和暂停策略。
- Data 层读取和校验生成的 CSV，发布运行时不可变快照。
- 主模块把玩法语义翻译成音频、动画、VFX、UI 和资产引用。

**边界意图：** `ReEcho` 可以依赖独立逻辑或服务模块；独立模块不得反向依赖它。GameMode 负责跨领域编排，不应成为各领域内部状态的第二事实来源。Widget 与表现对象不得拥有玩法权威状态。

### `MOD-ReEchoAudio`：`ReEchoAudio`

**存在原因：** 音频资源、播放实例、总线、并发、冷却和混音策略变化频繁，但不应与战斗或主流程互相阻塞，因此作为独立服务模块。

**输入：** 来自主模块的语义事件或状态请求，例如 `Combat.Attack`、`Combat.Hit`、`UI.Confirm`、音乐和环境状态。

**权威状态：** 音频目录、播放实例、音乐/环境状态通道、音量与静音设置、冷却、并发和优先级。

**输出：** 2D/3D 播放、停止、淡入淡出和安全降级结果；不向玩法返回决定性结果。

**禁止：** include `ReEcho` 玩法类型、决定攻击/命中/流程、让音频完成回调控制玩法、因缺资源或无音频设备导致玩法失败。

## 主模块内部领域意图

这些目录目前属于 `ReEcho` 编译模块，但仍有明确职责边界。

| 架构标识 | 领域 | 设计意图与权威状态 | 对外方式 | 不应承担 |
|---|---|---|---|---|
| `AREA-Core` | `Core` | 稳定公共类型和兼容契约 | 值类型、枚举、快照 | 具体流程和资产加载 |
| `AREA-Data` | `Data` | 校验生产数据并发布不可变运行时快照 | 类型化查询、稳定 ID | 执行表中自由文本或保存第二份平衡常量 |
| `AREA-AbilityCombat` | `AbilitySystem` / `Combat` | GAS 属性、Effect、能力、生命与元素结算 | 能力请求、结果和事件 | UI 布局、音频/动画资源策略 |
| `AREA-Weapons` | `Weapons` | 当前武器定义、攻击步骤和载体执行 | 稳定 WeaponId、攻击请求 | 本局存档、商店或 UI 生命周期 |
| `AREA-Encounter` | `Encounter` | 确定性固定步遭遇时钟和完成信号 | 时间/完成事件 | 构筑和存档所有权 |
| `AREA-Run` | `Run` | 本局阶段、构筑、背包、Echo 存储/回放和安全存档 | 窄命令、只读摘要 | 具体 Widget 布局和世界表现 |
| `AREA-Recording` | `Recording` | 20 Hz 位置与成功主动技能事件、历史插值 | 录制数据、Playback | 自动攻击序列化和当前世界命中结算 |
| `AREA-Player` | `Player` | 输入、移动、相机和玩家侧装配 | 输入命令、只读状态 | 重复持有 Run/Combat 权威状态 |
| `AREA-Presentation` | `Graybox` / `Presentation` | Actor 装配、2D/3D 可见表现、命中反馈 | 消费逻辑结果和快照 | 用动画、特效或资源加载决定玩法结果 |
| `AREA-UI` | `UI` | WBP 表现、用户命令入口和只读信息展示 | 类型化命令、快照、事件 | 直接写血量、构筑、攻击计时和流程内部字段 |
| `AREA-Tests` | `Tests` | 证明确定性契约与跨领域集成 | 自动化证据 | 替代用户的视觉、手感和可用性验收 |

## 主流程

```text
启动 / Continue
  → 角色与初始武器选择
  → 创建竞技场、玩家、EncounterDirector、敌人和可用 Echo
  → 60 Hz 暂停感知固定步推进遭遇
  → 玩家位置与成功主动技能按 20 Hz 录制
  → 敌人清空或超时后完成本场录制
  → 构筑选择 → Time Shard 商店 → Echo 存储/回放选择
  → 下一场遭遇；最终 Boss 结算整局
```

- 选中的稳定 `WeaponId` 在整局内锁定；旧 `InputSlot` 仅作兼容元数据。
- Echo 回放历史位置和成功主动技能，目标选择与命中始终依据当前世界。
- 自动攻击不进入 Recording。
- 局中保存退出会捕获计时、玩家、当前录制和存活敌人；保存失败不得退出。

## UI 框架意图

UI 仍位于 `ReEcho`，但通过框架层隔离：

- `EReEchoUIScreen` 提供稳定屏幕身份。
- `UReEchoUIManagerSubsystem` 拥有 WBP 类注册、活跃实例和 Viewport 层。
- `UReEchoUIFlowCoordinatorSubsystem` 拥有创建/关闭、焦点、输入模式、暂停和暂停安全切换。
- `AReEchoGameMode` 提供玩法快照并处理稳定 ID 命令端点，不拥有资产路径或直接管理 Widget Viewport 生命周期。

## 数据流与设计意图

```text
Design/Data/ReEchoData.xlsx
  → scripts/data/sync_xlsx_to_csv.py
  → Content/Data 中经过校验的 UTF-8 生产 CSV
  → 类型化 CSV Reader 与 FReEchoCsvDataRegistry
  → 本局固定的不可变运行时快照
```

- XLSX 是已迁移领域的权威策划编辑源；生成 CSV 是可 diff、可打包的运行时源。
- Unreal 运行时不读取 XLSX，也不依赖 Excel/Office。
- Behavior、Formula、Effect 和 AttackPattern 使用稳定 ID 映射到注册的 C++ 实现；表格描述文本不会作为逻辑执行。
- 已迁移领域的旧 JSON 仅用于迁移；其余领域必须通过独立 Plan 原子迁移，不能形成双事实来源。

## 跨领域不变量

- 依赖指向权威逻辑和服务，不能从底层模块反向依赖主流程或表现。
- 每种运行时状态只能有一个权威拥有者；其他层读取快照、订阅事件或发送受控命令。
- 逻辑产生结果，Animation、VFX、Audio 和 UI 只消费结果；表现完成回调不控制玩法。
- 稳定 ID、存档格式、CSV Schema、公共 API 和生成器属于共享契约，破坏性修改必须显式协调。
- 序列化 `.uasset`/`.umap` 负责宿主地图和资产，不承载可由 C++/权威数据明确表达的第二份玩法规则。

## 文档维护规则

Plan 中形成的决策分两类：

1. **任务级决策**留在 Plan：局部实现选择、已否决方案、迁移步骤和验证证据。
2. **当前全局架构**晋升到本文件：模块存在原因、职责/状态所有权、公共契约、依赖方向和跨领域不变量。

本文件与 `CODEBASE_MAP.md` 使用同一组 `MOD-*` / `AREA-*` 标识。新增、拆分、合并或重命名模块/领域时，两份文档必须在同一次 Plan 关闭中同步；仅移动实现路径而不改变设计意图时，只更新 `CODEBASE_MAP.md` 中对应标识的代码落点。

Planner 关闭每个 Plan 前必须完成一次架构审阅，并在 Plan 的“架构影响与设计决策”中记录以下二者之一：

- 已更新 `shared/ARCHITECTURE.md`，列出晋升的设计意图或架构变化；
- 已审阅，无需修改，并说明该 Plan 为什么没有改变全局模块、状态所有权、公共契约或依赖方向。
