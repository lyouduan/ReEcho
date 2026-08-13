# ReEcho 当前全局架构

本文只描述当前 `main` 已存在的全局拓扑、依赖方向和跨模块不变量。各模块的详细设计意图与代码位置从 [`README.md`](README.md) 进入。

## 项目形态

ReEcho 是 Unreal Engine 5.8 的 2.5D 时间回响肉鸽原型。角色、敌人与 Echo 使用 2D 表现，移动、碰撞、攻击和世界流程运行于 3D 场景；`/Game/Level00` 承载运行时生成的有界竞技场与六场遭遇。

## 当前 Runtime Module

| 架构标识 | Runtime Module | 存在原因 | 详细文档 |
|---|---|---|---|
| `MOD-ReEcho` | `ReEcho` | 作为 UE 玩法装配根，组合世界生命周期、主流程、尚未独立的玩法领域、表现和 UI | [`modules/MOD-ReEcho.md`](modules/MOD-ReEcho.md) |
| `MOD-ReEchoAudio` | `ReEchoAudio` | 隔离音频目录、策略、总线和播放后端，使缺资源或无设备不影响玩法 | [`modules/MOD-ReEchoAudio.md`](modules/MOD-ReEchoAudio.md) |

```text
MOD-ReEcho ─────────────→ MOD-ReEchoAudio
    │                         │
    │ 语义事件/状态请求       └─ 播放、停止、淡入淡出与安全降级
    └─ 不等待音频结果

MOD-ReEchoAudio ─/─→ MOD-ReEcho
```

依赖必须保持单向。音频是结果消费者，不决定攻击、命中、流程、存档或 UI 命令是否成功。

## 主运行流程

```text
启动 / Continue
  → 角色与初始武器选择
  → AReEchoGameMode 创建竞技场、玩家、EncounterDirector、敌人与可用 Echo
  → 60 Hz 暂停感知固定步推进遭遇
  → 玩家位置与成功主动技能按 20 Hz 录制
  → 敌人清空或超时后完成本场录制
  → 特质构筑 → Time Shard 商店 → Echo 存储/回放选择
  → 下一场遭遇；最终 Boss 结束整局
```

`AReEchoGameMode` 只负责编排跨领域步骤；每个领域的状态仍由其权威组件或 Subsystem 持有，GameMode 不保存第二份真相。

## 跨模块状态流

| 状态/结果 | 权威拥有者 | 其他系统如何使用 |
|---|---|---|
| 本局阶段、构筑、背包、Echo 存储与保存 | `UReEchoRunSubsystem` | 发送窄命令、读取摘要，不直接改字段 |
| 当前生命、属性、格挡、元素状态 | 当前 Combatant/GAS 路径 | UI/表现读取快照或订阅结果 |
| 武器选择、攻击步骤与攻击载体 | 当前 Weapon/Combat 路径 | Player/Echo 发请求，表现消费结果 |
| 玩家历史与 Echo Playback | Recording/Playback | 世界流程启动/停止，当前世界重新选目标与结算 |
| 屏幕实例、焦点、输入模式与暂停策略 | UI Manager/Flow Coordinator | GameMode 发送屏幕命令，不直接管理 Viewport |
| 音频目录、总线、音乐/环境状态与播放实例 | `MOD-ReEchoAudio` | 主模块发送语义请求，不读取播放内部状态决定玩法 |

## 数据权威流

```text
Design/Data/ReEchoData.xlsx
  → scripts/data/sync_xlsx_to_csv.py
  → Content/Data/*.csv
  → 类型化 CSV Reader / FReEchoCsvDataRegistry
  → 本局固定的不可变运行时快照
```

- XLSX 是已迁移领域的策划编辑源；CSV 是可 diff、可打包的运行时源。
- Unreal 运行时不读取 XLSX，也不执行表格自由文本。
- Behavior、Formula、Effect 与 AttackPattern 通过稳定 ID 映射到注册实现。
- 已迁移领域的旧 JSON 仅用于迁移，不能成为第二事实来源。

## 跨模块不变量

- 依赖指向权威逻辑或底层服务，底层模块不得反向依赖主流程或表现。
- 每种运行时状态只有一个权威拥有者；其他模块只读快照、订阅事件或发送受控命令。
- 逻辑产生结果；Animation、VFX、Audio 和 UI 只消费结果，表现完成回调不控制玩法。
- 稳定 ID、存档格式、CSV Schema、公共 API 和生成器属于共享契约，破坏性修改必须显式协调。
- `.uasset`/`.umap` 承载宿主地图和资产引用，不保存可以由 C++ 或权威数据明确表达的第二份玩法规则。
- 模块缺失可选资源或外部设备时必须安全降级，不得导致确定性玩法失败。

## 候选架构边界

Plan41 正在本地 `Review` 将战斗与武器逻辑拆为 `ReEchoCombat`、`ReEchoWeapons`。在用户 PIE 通过并合入 `main` 前，它们不是当前全局模块；其详细设计只存在于 Plan41 本地候选文档中，不能在这里写成既成事实。

## 维护规则

Plan 中形成的局部方案、替代方案和验证历史留在 Plan。通过验收并合入的模块存在原因、职责、状态所有权、公共契约、依赖方向、代码落点和跨领域不变量必须晋升到本目录。
