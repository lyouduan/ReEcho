# `MOD-ReEchoAudio`：`ReEchoAudio`

## 模块状态

- 当前状态：当前 `main` 的独立 Runtime Module。
- 描述符：`ReEcho.uproject`。
- Build 文件：`Source/ReEchoAudio/ReEchoAudio.Build.cs`。
- 注册入口：`Source/ReEchoAudio/Private/ReEchoAudio.cpp` 中的 `FReEchoAudioModule`。
- 主要目录：`Source/ReEchoAudio/Public/`、`Source/ReEchoAudio/Private/`。
- 相关基线：Plan33 音频运行时基础；Plan34 增加策划目录、持久设置、战斗 BGM 接入与真实后端淡入修复；Plan46 为全部 31 个基础目录项接入资源并硬化非阻塞播放、衰减和 Cook；Plan114 增加正式资源、确定性单声道/裁切派生和 Weapon/Element/Stage 变体；Plan123 补齐反应、拾取、卡牌展示、装备与普通关胜利语义，并按 `ECHO音频配置说明.xlsx` 主需求页与交付文件夹的交集，把 15 个无策划授权的基础行收敛为显式静默目录项，删除全部旧 Generated 与表外音频。

## 存在原因

音频资源映射、播放实例、总线、冷却、并发、优先级、音乐和环境状态变化频繁，且依赖音频设备与资源可用性。把这些策略放进独立模块，可以让玩法只发送稳定语义，不需要知道 Sound 资产、混音或播放失败细节，也不会因为音频不可用而阻塞战斗和主流程。

## 职责与排除项

### 负责

- 接收稳定 `EventId` 的一次性音效请求与音乐/环境状态请求。
- 管理 Master/Music/Ambience/SFX/UI 等语义总线的音量和静音状态。
- 将语义 ID 通过目录解析为后端可播放描述。
- 执行冷却、并发、优先级、2D/3D、暂停策略和安全降级。
- 隔离 Unreal 音频后端，允许使用假后端做无设备自动化。
- 对缺目录项、缺资源、无 World、无设备和播放拒绝提供确定性失败，不抛出玩法异常。

### 不负责

- 不 include `ReEcho`、Combat、Weapons、Run、UI 或具体玩法 Actor 类型。
- 不判断一次攻击是否成立、是否命中、是否死亡或是否进入下一流程。
- 不通过播放完成回调驱动玩法。
- 不持有角色生命、战斗模式、商店、录制或 Widget 状态。
- 不在音频模块内硬编码具体玩法资源触发位置；主模块负责把玩法结果翻译成语义请求。

## 权威状态

| 状态 | 权威对象 | 说明 |
|---|---|---|
| 运行时服务生命周期 | `UReEchoAudioService` | GameInstance Subsystem，统一请求入口与 Tick |
| 音频目录 | `FReEchoAudioCatalog` / `IReEchoAudioCatalogProvider` | `(EventId, VariantId)` 到播放描述的唯一映射接口；空变体是基础回退；`StartTimeSeconds` 是策划可调的非负源内起播点 |
| 音频资源与来源 | `Content/ReEcho/Audio/**` / `Design/Audio/**` | 前者只允许权威目录非空路径指向的运行时 SoundWave；后者只保存用户源文件、确定性正式派生与来源清单，不再保存 Generated 占位音；空 `AssetPath` 表示经策划确认的静默语义 |
| 音乐状态 | `UReEchoAudioService` + Policy Engine | 独立状态通道，不由 GameMode 缓存第二份 |
| 环境状态 | `UReEchoAudioService` + Policy Engine | 与音乐分离，可独立停止/切换 |
| Master/总线音量与静音 | `UReEchoAudioService` + `UReEchoAudioUserSettings` | Service 是运行时权威；模块自有 SaveGame 是跨进程持久化权威，不进入 Run Save |
| 冷却/并发/优先级 | `FReEchoAudioPolicyEngine` | 决定请求是否播放或替换，不返回玩法决策 |
| 实际播放实例 | `IReEchoAudioBackend` 实现 | Unreal 后端拥有音频对象生命周期 |

## 输入、输出与公共契约

### 输入

- `FReEchoAudioEventRequest`：稳定 `EventId`、可选 `VariantId`、世界位置、来源类别、强度和请求上下文。
- `SetMusicState` / `SetAmbienceState`、对应 Variant 入口与停止命令。
- Master/Bus 音量、静音和 Tick。

### 输出

- 通过后端执行 2D/3D 播放、停止或状态切换。
- 对无效请求安全拒绝；调用者不依赖播放成功才能继续玩法。

### 稳定公共类型/API

- `UReEchoAudioService`：主入口；`QueueEventForNextWorld` 只保存一个通用 OneShot，待当前游戏 World 被替换后投递，不延迟关卡切换。
- `FReEchoAudioEventRequest`、`FReEchoAudioEventDefinition`。
- `EReEchoAudioBus`、`EReEchoAudioChannel`、`EReEchoAudioEventType`、`EReEchoAudioPausePolicy`、`EReEchoAudioSourceCategory`。
- `IReEchoAudioCatalogProvider`：目录替换边界。
- `IReEchoAudioBackend`：设备/引擎后端替换边界。

`EventId` 是跨模块契约。具体 Sound 资产路径、并发对象和混音资源不是玩法公共 API。

## 依赖方向

```text
MOD-ReEcho ──→ MOD-ReEchoAudio
MOD-ReEchoAudio ─/─→ MOD-ReEcho / Combat / Weapons / UI / Presentation
```

公共头只依赖 Core/CoreUObject/Engine 所需基础类型。主模块通过适配器订阅玩法事件并构造音频请求；音频模块不知道事件最初来自哪种具体 Actor 或 Widget。

## 运行时流程

```text
玩法/流程/UI 已确认的语义结果
  → 主模块适配器选择稳定 EventId / StateId 与可选 VariantId
  → UReEchoAudioService
      → Catalog 查找定义
      → Policy Engine 检查总线、暂停、冷却、并发和优先级
      → IReEchoAudioBackend
          → Unreal 2D/3D 播放或安全失败
```

音乐与环境使用独立状态通道：重复设置相同 `(StateId, VariantId)` 应幂等；切换状态由服务/策略层管理，调用者不直接保存 AudioComponent。Service 同时保存期望 State/Variant，异步预载重试或 World 替换后恢复同一组合。

跨 World OneShot 使用 Service 的单槽队列：调用方在旧 World 发出稳定 EventId，Ticker 仅在解析到不同的新 Game/PIE World 后交给 Policy Engine。队列不持有玩法类型、不等待播放完成，并在 Service 反初始化时清空。

## 内部组成

### `UReEchoAudioService`

- 位置：`Public/ReEchoAudioService.h`、`Private/ReEchoAudioService.cpp`。
- 角色：GameInstance 级门面、总线状态、音乐/环境状态入口、请求分发和 Tick。
- 不应：包含玩法类型、查找敌人/玩家、在回调中改变游戏状态。

### `FReEchoAudioCatalog`

- 位置：`Public/ReEchoAudioCatalog.h`、`Private/ReEchoAudioCatalog.cpp`。
- 角色：提供稳定 `(EventId, VariantId)` 到 `FReEchoAudioEventDefinition` 的类型化查询；精确变体不存在时回退同 EventId 的空变体。
- 数据：`Design/Data/ReEchoAudioEvents.xlsx` 独立拥有 `audio_events.csv`，不耦合 `ReEchoData.xlsx` / `ReEchoEnemyData.xlsx`。
- 加载：运行时对锁定 16 列 CSV 做 quote-aware 严格解析，复合键必须唯一；`StartTimeSeconds` 必须非负并由一次性/循环后端消费；仅在整表成功后原子替换，失败保留上一份有效目录。
- 预载：soft asset 异步预载暴露 `NotStarted/Loading/Ready/Failed` 状态，失败可重试且播放仍安全 no-op。
- 打包：`DefaultGame.ini` 的 `DirectoriesToAlwaysCook=/Game/ReEcho/Audio` 显式包含所有 CSV 文本软引用资产，不依赖地图偶然硬引用。

### `FReEchoAudioPolicyEngine`

- 位置：`Private/ReEchoAudioPolicyEngine.h/.cpp`。
- 角色：纯策略决策，包括冷却、并发、优先级、暂停与总线有效音量。
- 测试：优先通过假目录和假后端验证，不要求真实声卡或资产。

### `IReEchoAudioBackend`

- 位置：`Public/ReEchoAudioBackend.h`、`Private/ReEchoAudioBackend.cpp`。
- 角色：把已批准的播放命令适配到 Unreal 音频对象。
- 边界：后端失败只影响听觉结果；不得让服务抛出影响玩法的失败。
- 加载契约：播放后端只读取已驻留的 soft pointer，绝不在一次性或状态播放路径调用同步加载；失败启动不会覆盖当前状态。Service 保存期望 Music/Ambience 状态，在预载完成后自动重试，并在 Game/PIE World 替换时重建仍需要的 Loop。
- 空间契约：3D 命令在组件播放前写入世界位置和球形线性衰减覆盖；内半径为 `AttenuationMin`，衰减距离为 `AttenuationMax - AttenuationMin`。2D 事件不启用衰减覆盖。
- 增益契约：组件 `VolumeMultiplier` 保存 Policy Engine 算出的有效总线音量；淡入淡出只控制独立 fader。淡入必须用非自动播放组件和 `FadeIn(Duration, 1.0f)`，不得把组件基础音量初始化为零。

### Events 与 Types

- 位置：`Public/ReEchoAudioEvents.h`、`Public/ReEchoAudioTypes.h` 及对应 Private 实现。
- 角色：跨模块稳定值类型和枚举，不携带 Combat、UI、纹理、动画或具体玩法类。

## 代码位置与阅读路线

| 目的 | Public 首读 | Private 实现 | 相关数据/资产 |
|---|---|---|---|
| 发送一次性音效 | `ReEchoAudioService.h`、`ReEchoAudioEvents.h` | `ReEchoAudioService.cpp` | `ReEchoAudioEvents.xlsx` -> `audio_events.csv` |
| 维护目录音频资源 | `ReEchoAudioCatalog.h` | `ReEchoAudioCatalog.cpp`、`scripts/audio/*.py` | `Design/Audio/**` -> `Content/ReEcho/Audio/**` |
| 增加语义总线 | `ReEchoAudioTypes.h`、`ReEchoAudioService.h` | Service/Policy Engine | `UReEchoAudioUserSettings` + 设置页 Apply/Cancel |
| 修改冷却/并发/优先级 | `ReEchoAudioCatalog.h`、Events/Types | `ReEchoAudioPolicyEngine.*` | 目录定义 |
| 替换播放后端 | `ReEchoAudioBackend.h` | `ReEchoAudioBackend.cpp` | Unreal Sound/AudioComponent |
| 主模块接入战斗/UI | 音频公共 API | `Source/ReEcho/Private/Combat/` 或相应主模块适配器 | 稳定语义 ID，不由 Audio include 调用方 |

## 扩展方式

- 新音效：先定义稳定语义 ID 和来源事件，再增加目录项；不要让调用点直接加载 Sound 路径。
- 新来源类别或总线：扩展公共枚举、策略处理、设置入口和自动化，明确旧值兼容。
- 新后端：实现 `IReEchoAudioBackend`，保持 Service/Policy 测试可使用假后端。
- 新音乐/环境行为：通过独立状态通道扩展，避免把持续状态伪装成重复的一次性 SFX。
- 持久设置：`ReEchoAudio` 自有 `USaveGame` 是 authority；控件变化只实时预览，Apply 才保存，取消/销毁恢复上次保存值；诊断音不持久化。

## 验证与测试

- 自动化：`Source/ReEchoAudio/Private/Tests/ReEchoAudioFoundationTests.cpp`。
- 重点覆盖：目录解析、无效 ID、安全降级、总线音量/静音、状态幂等、冷却、并发、优先级、暂停策略和假后端调用。
- 语义路由审计：`python scripts/audio/validate_audio_event_routes.py` 比较 37 个稳定 EventId、常量和主模块生产引用，拒绝只有测试引用的孤立事件；变体集合另由项目数据域校验限定。
- 资产审计：在取得同克隆 Unreal 锁后运行 `scripts/audio/validate_audio_catalog_assets.py`，验证当前 52 行目录中的 37 个绑定行、15 个精确静默行、31 个唯一 SoundWave、类型、循环标记与空间音效单声道约束，并拒绝 `/Game/ReEcho/Audio` 下任何目录白名单外资产。`UI.CardSelect` 复用 `UI.Purchase`，`UI.Error/Revive` 复用 `UI.Cancel`，共享波形不复制资产。
- 构建：`scripts/ue/Build-Editor.cmd -Configuration Development` 必须同时产出 `UnrealEditor-ReEchoAudio.dll`。
- 静态：模块边界不得出现 `#include` 主模块玩法路径。
- 人工验收：真实资源可听性、响度平衡、空间定位和混音由用户在 PIE/设备上判断；链路异常优先用 Audio Insights 的 Events/Sounds/总线表区分“请求已发出”“组件仍存活”和“设备有最终信号”。

## 不变量与常见错误

- 音频永远是玩法结果消费者，不是流程门。
- 不得因为目录缺项、资源缺失或无设备导致攻击、UI 或保存失败。
- 一次性 SFX、音乐状态和环境状态不能共用一套含糊生命周期。
- 冷却/并发/优先级只有 Policy Engine 一份权威状态。
- 不得用零 `VolumeMultiplier` 模拟淡入；UE 的 fader 与组件基础音量相乘，不会替换它。
- 不得在后端用 `LoadSynchronous()` 补救未完成的预载；状态启动失败必须保留现有状态并允许重试。
- 玩法不应为异步预载而重复发状态；Service 持有期望状态并负责重试，World 替换后不能用旧句柄误判为仍在播放。
- CSV 中的 SoundWave 软路径必须由显式 Cook 目录覆盖，不能依赖当前地图是否引用资源。
- 变体只由稳定 WeaponId、ElementId 或 StageId 选择；资产路径仍只存在于目录，未知/空变体回退基础事件时允许得到显式静音，不能重新引入通用占位音。
- 玩法调用点只认识稳定语义 ID，不认识资产路径。
- next-world 队列只跨现有 World 生命周期投递声音，不得创建复活规则、延迟 `OpenLevel` 或用播放结果确认重开成功。
- 模块公共头不得泄漏 `ReEcho`、Combat、Weapons、UI 或 Presentation 类型。
