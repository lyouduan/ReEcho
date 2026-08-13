# ReEcho 2D 序列动画修改与扩展指南

本文说明 ReEcho 当前 Paper2D 序列动画架构、已接入资源、`Idle / Walk / Attack` 状态机，以及后续增加角色或动画时的修改入口。

## 当前接入范围

### 智者玩家 `J_SPADE`

| 状态 | 判断条件 | 表现资源 | 播放方式 |
|---|---|---|---|
| `Idle` | 未移动且未处于攻击表现 | `/Game/2DAnim/Player/Idel_01` | 静态 Billboard，资源原始比例 |
| `Walk` | 有有效移动速度且未攻击 | `/Game/2DAnim/Flipbook/walk` | 循环 Flipbook，资源原始比例 |
| `Attack` | Moon Staff `W_J_02` 成功攻击 | `/Game/2DAnim/Flipbook/attack` | 单次播放；每次成功攻击从第 0 帧重播 |

状态优先级为：

```text
Attack > Walk > Idle
```

因此移动中发动攻击时进入 `Attack`，攻击播放窗口结束后根据当时速度返回 `Walk` 或 `Idle`。

其他玩家角色仍使用各自静态 Billboard，不启用该 Flipbook 状态机。

### Grunt 敌人

仅 `EReEchoEnemyKind::Grunt` 使用 `/Game/2DAnim/Flipbook/01_2`，并在移动、攻击、受击和死亡期间持续循环同一 Flipbook。Shield、Bomber、小 Boss 和最终 Boss 保持原静态 Billboard。

## 代码结构

```text
Source/ReEcho/
├─ Public/Presentation/Animation2D/
│  ├─ ReEcho2DAnimationTypes.h
│  ├─ ReEcho2DAnimationProfile.h
│  └─ ReEcho2DAnimationComponent.h
├─ Private/Presentation/Animation2D/
│  └─ ReEcho2DAnimationComponent.cpp
├─ Public/Player/ReEchoPlayerPawn.h
├─ Private/Player/ReEchoPlayerPawn.cpp
├─ Public/Graybox/ReEchoEnemyActor.h
├─ Private/Graybox/ReEchoEnemyActor.cpp
└─ Private/Tests/ReEcho2DAnimationTests.cpp
```

主要职责如下：

- `EReEcho2DAnimationState`：统一状态枚举，当前包含 `Default`、`Idle`、`Walk`、`Attack`、`Hit`、`Death`。
- `ReEchoResolve2DAnimationState`：纯状态决策函数，根据移动和攻击布尔量返回 `Idle / Walk / Attack`。
- `FReEcho2DAnimationProfile`：保存默认 Flipbook、状态到 Flipbook 的映射、显示尺寸、偏移和排序层级。
- `UReEcho2DAnimationComponent`：加载 Profile、切换状态、控制循环或单次播放、重新播放、左右朝向和显示比例。
- `AReEchoPlayerPawn`：读取角色、移动和成功攻击事实，决定玩家当前表现状态。
- `AReEchoEnemyActor`：仅为 Grunt 激活序列动画，其他敌人走静态回退。

## 状态机设置

玩家每帧由 `AReEchoPlayerPawn::UpdateSpadeAnimationState` 更新状态：

```cpp
const bool bMoonStaffAttack =
    SequenceAttackRemaining > 0.0f && Weapon && Weapon->GetEquippedWeaponId() == MoonStaffWeaponId;
const EReEcho2DAnimationState DesiredState =
    ReEchoResolve2DAnimationState(bMoving, bMoonStaffAttack);
TransitionSpadeAnimationState(DesiredState);
```

状态转换关系：

```text
Idle   -> Walk   : 开始移动
Idle   -> Attack : 成功攻击
Walk   -> Idle   : 停止移动
Walk   -> Attack : 移动中成功攻击
Attack -> Walk   : 攻击窗口结束且仍在移动
Attack -> Idle   : 攻击窗口结束且已停止移动
```

`TransitionSpadeAnimationState` 是唯一切换显示实现的入口：

- `Idle`：关闭 `SequenceAnimation`，显示静态 `CharacterSprite`。
- `Walk`：隐藏静态 Billboard，循环播放 Walk Flipbook。
- `Attack`：隐藏静态 Billboard，单次播放 Attack Flipbook。
- Flipbook 缺失或激活失败：安全回退到 `Idle` 静态贴图。

当前状态可通过 Blueprint 只读接口获取：

```cpp
GetCurrent2DAnimationState()
```

该接口适合调试 UI、状态显示和自动化观察，不应被 Blueprint 用来反向修改玩法状态。

## Profile 与资源映射

玩家 Profile 当前在 `TransitionSpadeAnimationState` 中构造：

```cpp
FReEcho2DAnimationProfile Profile;
Profile.DefaultFlipbook = SpadeIdleFlipbook;
Profile.StateFlipbooks.Add(EReEcho2DAnimationState::Walk, SpadeWalkFlipbook);
Profile.StateFlipbooks.Add(EReEcho2DAnimationState::Attack, SpadeAttackFlipbook);
Profile.bUseNativeScale = true;
```

注意：资源名 `Idel` 是现有资产的实际拼写。本轮代码中的逻辑状态统一使用正确的 `Idle` 和 `Walk`，但不直接重命名 `.uasset`。如果以后修正资产名称，必须通过 Unreal Editor 完成重命名和引用修复。

## Transform 分层

```text
Actor Root / Collision
└─ VisualEffectRoot
   ├─ CharacterSprite
   └─ SequenceAnimation
```

- Actor Root 和 Collision 是玩法权威，不受视觉动画改变。
- `VisualEffectRoot` 承接浮动、攻击前冲、受击抖动、拉伸和死亡缩小。
- `CharacterSprite` 与 `SequenceAnimation` 互斥显示。
- Flipbook 组件只负责序列帧、朝向、尺寸和排序。

不要同时在 Actor、`VisualEffectRoot` 和 Flipbook 上重复施加同一位移或缩放，否则会出现表现叠加、尺寸跳变或碰撞错位。

## 播放规则

### Walk

- 使用循环播放。
- 静止时必须退出到 `Idle`，不能继续停留在 Walk 动画。
- 当前移动阈值为 `GetVelocity().SizeSquared2D() > 25.0f`。

### Attack

- 使用单次播放，不设置永久循环。
- 每次成功攻击调用 `StartAttackVisual`，刷新 `SequenceAttackRemaining`。
- 如果已经处于 `Attack`，再次成功攻击会使用 `bRestart=true` 从第 0 帧重新播放。
- 连续攻击因此按真实攻击成功节奏重复播放，而不是脱离攻击逻辑自行循环。

### Idle

- 智者使用静态 `Idel_01`。
- 使用资源原始宽高比和 `1.0` 缩放。
- 原有程序化轻微浮动仍由 `VisualEffectRoot` 保留。

## 增加新动画

### 为现有状态更换 Flipbook

1. 在 Unreal Editor 中导入纹理、创建 PaperSprite 和 PaperFlipbook。
2. 确认 Flipbook 有有效 Render Bounds、正确 Pivot、透明材质和帧率。
3. 通过硬引用或可 Cook 追踪的 Profile 资产引用加载资源。
4. 将资源加入对应 `StateFlipbooks`。
5. 明确该状态是循环还是单次播放。
6. 增加资源加载、映射、循环策略和缺失回退测试。

### 增加 `Hit` 或 `Death` 独立动画

1. 明确状态优先级，例如 `Death > Hit > Attack > Walk > Idle`。
2. 只让 Pawn/Enemy 提供事实，不把伤害、死亡或 GAS 逻辑放进动画组件。
3. 在 Profile 中添加 `Hit`、`Death` 映射。
4. 明确不可打断、可打断和播放结束后的目标状态。
5. 保留缺失资源回退，不能让 Actor 隐形。
6. 不使用动画通知驱动实际伤害或死亡，除非另有正式玩法方案。

### 为其他角色启用状态机

不要直接复用 `J_SPADE` 的硬编码判断。建议下一步把角色资源映射迁移到独立的 Profile DataAsset，然后按 CharacterId 选择 Profile。每个 Profile 至少应配置：

- 静态回退纹理；
- Idle、Walk、Attack Flipbook；
- 循环策略；
- 原始比例或目标世界高度；
- Pivot/本地偏移；
- Translucent Sort Priority。

## 调试与验证

### 自动化

```powershell
cmd.exe /c scripts\ue\Build-Editor.cmd
cmd.exe /c scripts\ue\Run-Automation.cmd -Filter ReEcho.Presentation.Animation2D.AssetProfiles
python scripts/validate_project.py
git diff --check
```

专项测试覆盖：

- Flipbook 可加载且 Render Bounds 非空；
- 静止解析为 `Idle`；
- 移动解析为 `Walk`；
- 攻击覆盖移动并解析为 `Attack`；
- Walk 循环播放；
- Attack 单次播放；
- 连续攻击从头重播；
- 原始比例保持；
- 缺失资源安全失败。

### PIE 人工检查

- 智者静止时显示 `Idel_01`，不播放 Walk。
- 智者移动时播放 Walk，停止后立即返回 Idle。
- 智者攻击时播放 Attack；移动攻击时 Attack 优先。
- 连续攻击时每次成功攻击均能重新播放动作。
- Idle、Walk、Attack 切换时比例、锚点和位置不跳变。
- 左右朝向正确，透明边缘和场景排序正常。
- 同一 Actor 不会同时显示 Billboard 与 Flipbook。
- Grunt 保持循环，其他敌人外观不变。
- 移动、碰撞、伤害、死亡、GAS、录制和存档语义不变。

## 常见问题

### Flipbook 可加载但画面不可见

检查 PaperSprite 是否有非空烘焙几何和 Render Bounds。仓库提供 `scripts/ue/repair_2d_animation_flipbooks.py` 作为开发期修复入口，资产修改必须通过 Unreal Editor API 保存。

### 动画停在第一帧

检查组件 Tick 是否启用、`PrimaryComponentTick.bCanEverTick` 是否为 true，以及激活时是否调用 `SetComponentTickEnabled(true)` 和 `PlayFromStart()`。

### 连续攻击只播放一次

状态持续为 `Attack` 时，普通状态去重会阻止重播。每次成功攻击必须调用：

```cpp
SetAnimationState(EReEcho2DAnimationState::Attack, false, true);
```

### Idle 与 Walk 同时显示

所有显示切换必须经过 `TransitionSpadeAnimationState`。进入 Walk/Attack 时隐藏 `CharacterSprite`；进入 Idle 或资源失败时停用 `SequenceAnimation` 并恢复静态贴图。

### 人物比例跳变

智者当前静态贴图和 Flipbook 都使用资源原始比例。不要再次使用固定世界高度归一化；Profile 应保持 `bUseNativeScale = true`。

