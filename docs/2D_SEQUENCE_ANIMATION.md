# ReEcho 2D 序列动画修改与扩展指南

本文说明 ReEcho 当前 Paper2D 序列动画架构、生产 Profile、七态语义状态机，以及后续增加角色或动画时的修改入口。旧的 `Idle / Walk / Attack` 说明仅适用于玩家基础路径；生产 FSM 以本节契约为准。

## 当前生产状态契约（Plan74）

默认 FSM 为 `Idle(0) < Move(10) < Attack.Charge(30) < Attack.Basic(40) < Hit(60) < Transform.Phase2(80) < Death(100)`。Charge 循环且拥有动作占用，移动变化不能覆盖它；玩法提交后进入 Basic，动作结束/取消可显式返回当前 Idle/Move。Transform 是不可被普通攻击或 Hit 覆盖的一次性阶段过渡，完成后切换 Phase2 AnimationSet；Death 为终结状态。任何播放完成都只改变表现，不产生伤害、移动或 AI 结果。

狐狸 Base/Phase2 均显式配置 Charge 与 Basic。TimeGuard 的 Base 和 Phase2 均配置完整基础状态；阶段开始播放 Transform，阶段完成切换暗形态集合。仓库当前没有独立 Boss 变身帧，因此 Transform 显式复用暗形态素材作为具名回退，后续只需替换 Profile Clip，无需修改玩法代码。

生产资产覆盖可用 `scripts/ue/audit_plan74_animation_contracts.py` 只读检查；它覆盖 8 个玩家/Echo Profile、8 个怪物 Profile、七态 FSM、必需 Clip 和循环策略。运行时缺失语义会记录警告并保留安全表现回退，不静默改变玩法。

## 当前接入范围

### 智者玩家 `J_SPADE`

| 状态 | 判断条件 | 表现资源 | 播放方式 |
|---|---|---|---|
| `Idle` | 未移动且未处于攻击表现 | `/Game/ReEcho/Art/Animation2D/Players/Spade/Walk/Textures/Idel_01` | 静态 Billboard，资源原始比例 |
| `Walk` | 有有效移动速度且未攻击 | `/Game/ReEcho/Art/Animation2D/Players/Spade/Flipbooks/Walk` | 循环 Flipbook，资源原始比例 |
| `Attack` | Moon Staff `W_J_02` 成功攻击 | `/Game/ReEcho/Art/Animation2D/Players/Spade/Flipbooks/Attack` | 单次播放；每次成功攻击从第 0 帧重播 |

状态优先级为：

```text
Attack > Walk > Idle
```

因此移动中发动攻击时进入 `Attack`，攻击播放窗口结束后根据当时速度返回 `Walk` 或 `Idle`。

通用 2D 动画代码属于独立 `ReEchoPresentation` Runtime Module。四个生产角色 `J_SPADE/J_DIAMOND/J_CLOVER/J_HEART` 都通过 `DA_PresentationCatalog` 解析 Profile；当前非 Spade Profile 可继续使用单帧 Flipbook，但不再由 Pawn 按角色硬编码贴图路径。`J_CAT` 已退出生产角色集合，旧存档身份迁移到 `J_SPADE`。

`DA_PresentationCatalog` 只保存 `PresentationId -> Profile`。敌人 Gameplay Blueprint Class 由主模块的 `DA_EnemyGameplayClassRegistry` 单独维护，避免表现模块反向依赖玩法 Actor。

### 敌人

Grunt、Shield、Bomber、Slime、Rabbit、Fox、TimeGuard 都使用 Enemy Definition 的稳定 `PresentationId`，经同一 Catalog 解析 Profile 与 Gameplay Blueprint。动画外观不再按 `EnemyKind`、Archetype 或生成顺序选择；TimeGuard 没有专属序列时由 `DA_Enemy_TimeGuard` 显式复用 Goat 动画。

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
- `AReEchoEnemyActor`：只把 Definition 的 `PresentationId` 交给表现组件；不读取 Flipbook、Profile 或具体敌人外观。

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

# 逐帧碰撞标注与预览

当前实际参与序列播放的玩家 `walk / attack` 与怪物 `Grount / Rabbit / Goat / Fox_Walk / Fox_Attack` 使用 Paper2D `EachFrameCollision`。Idle 仍使用静态 `Idel_01` 贴图。`UReEcho2DAnimationComponent` 会实际启用每帧 Sprite BodySetup，但固定使用 `QueryOnly`、对象类型 `WorldDynamic`，对 `Pawn` 保留查询响应且关闭自动 Overlap 事件；它不会阻挡角色移动，也不会在逐帧切换时产生无人消费的重叠回调。Actor Root Capsule 仍是移动与阻挡权威。动画停用或切换到非逐帧碰撞 Flipbook 时，Paper2D 碰撞同步关闭。

生产敌人表现由 `PresentationId -> Catalog Entry -> Profile + Gameplay Blueprint` 唯一解析。Grunt、Shield、Bomber、Slime、Rabbit、Fox、TimeGuard 共用同一 Host/Controller/FSM 契约，各 Blueprint 只调整碰撞、比例、阴影、脚点和挂点。Fox 的 `Animation.Idle` 使用循环 Walk，只有玩法攻击门提交攻击时才播放一次 `Animation.Attack.Basic`，结束后自动返回基础状态；伤害仍由原攻击流程触发，不使用动画通知。TimeGuard 使用通用 Profile/FSM，不再加载 `Boss2D` 静态贴图。

角色新增的 `walk` 内容仍由 `DA_Character_J_SPADE` 的 `Animation.Move` Clip 引用；替换同路径资产后无需增加 Pawn 分支，停止移动仍回到静态 `Idel_01`，攻击仍由 MoonStaff 组合集的一次性 `attack` Clip 覆盖。

Collision Source 必须保存进资产，不能只停留在未保存的 Editor 会话。关闭交互 Editor 后可运行 `scripts/ue/configure_plan40_flipbook_collision.py`；该脚本只把上述三个 Flipbook 设置为 `EachFrameCollision` 并保存，不会生成或猜测各 PaperSprite 的碰撞轮廓。

Paper2D 内建每帧碰撞表示整帧 Sprite 的通用查询轮廓；它不携带 Body/Weapon 语义，也不直接触发伤害。`UReEcho2DFrameCollisionTrack` 继续负责经过审核的 Body Hurtbox、Weapon AttackHitbox 和攻击窗口。如果没有 Track，伤害逻辑仍回退到既有 Capsule/武器范围查询。

Plan40 的碰撞不会从透明像素或整张角色加武器合成图在运行时自动生成。每个 Flipbook 必须有经过审核的 JSON 标注，之后由 `scripts/ue/build_plan40_collision_tracks.py` 确定性生成 `UReEcho2DFrameCollisionTrack`。

标注放在 `Content/ReEcho/Art/Animation2D/CollisionAnnotations/*.json`，格式如下：

```json
{
  "asset_name": "DA_Collision_Spade_Attack",
  "flipbook": "/Game/ReEcho/Art/Animation2D/Players/Spade/Flipbooks/Attack.Attack",
  "source_revision": "reviewed-source-revision",
  "pixels_per_unreal_unit": 1.0,
  "pivot_pixels": [512, 512],
  "frames": [
    {
      "body_hurtboxes": [[[420, 240], [580, 240], [580, 760], [420, 760]]],
      "weapon_attack_hitboxes": [],
      "attack_active": false
    }
  ]
}
```

- `frames` 数量必须与 Flipbook 关键帧数完全一致。
- 每个多边形必须为 3–16 个顶点、有限数值且面积非零。
- `body_hurtboxes` 只标身体；不能把武器、特效或整张 alpha 外接框算入身体。
- `weapon_attack_hitboxes` 只标武器有效区域，并且只有 `attack_active=true` 且存在已提交攻击实例时才可查询。
- `source_revision` 必须对应本次人工审核的源图/标注版本；资源变化后必须重新审核。
- 缺少或不匹配 Track 时继续使用 Capsule/现有武器范围查询，不会产生隐式碰撞。

PIE 中使用以下控制台变量查看叠加：

```text
reecho.Animation2D.DrawFrameCollision 1   // 绿色 Body Hurtbox
reecho.Animation2D.DrawFrameCollision 2   // 绿色 Body + 红色活跃 Attack Hitbox
```

统一查看当前物体所有碰撞体积，可在 Editor Console 或启动 CMD 中使用：

```text
ReEcho.DebugCollision 2
UnrealEditor.exe ReEcho.uproject -ExecCmds="ReEcho.DebugCollision 2"
```

调试等级：`0` 关闭；`1` 显示青/橙色 Actor Root Capsule；`2` 额外显示黄色 Paper2D 当前帧真实 Box、Sphere、Capsule、Convex 线框及帧/形状数量；`3` 再显示绿色语义 Body Hurtbox 与红色当前活跃 Attack Hitbox。黄色形状只表示 Paper2D 烘焙查询几何，不代表语义武器伤害区。独立命令 `reecho.Animation2D.DrawFrameCollision 1/2` 仍可只查看语义轨道。

叠加线由当前 Flipbook 帧、Profile 缩放、Pivot 和朝向共同驱动。视觉与碰撞不一致时应修正 JSON/Track，不能用移动 Actor Root 或修改移动 Capsule 来补偿。
