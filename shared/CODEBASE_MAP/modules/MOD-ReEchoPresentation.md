# `MOD-ReEchoPresentation`：`ReEchoPresentation`

## 模块状态

- 当前状态：Plan74 实现候选。
- Runtime Module：`ReEchoPresentation`。
- Build 文件：`Source/ReEchoPresentation/ReEchoPresentation.Build.cs`。
- 主要目录：`Source/ReEchoPresentation/{Public,Private}/Presentation/Animation2D/`。
- 相关 Plan：Plan40、Plan50、Plan74。

## 存在原因

角色与怪物共享同一套 2D Profile、语义状态机、Flipbook 渲染和逐帧碰撞查询。独立模块阻止 Paper2D 资产类型、动画帧和表现状态渗入 GameMode、敌人逻辑、战斗与武器模块。

## 职责与排除项

### 负责

- `PresentationId -> Profile` 的 Cook 可追踪表现目录。
- 语义动画状态、Clip/Profile、状态机和播放控制。
- Flipbook 尺寸、朝向、锚点及安全隐藏。
- 逐帧碰撞快照与调试显示。

### 不负责

- 敌人 Gameplay Blueprint Class、生成、AI、攻击、伤害、死亡或存档。
- 玩家输入、武器规则、GameMode 生命周期和关卡流程。
- 使用动画完成回调决定玩法结果。

## 权威状态

模块只拥有当前表现 Profile、语义状态、播放状态与只读碰撞快照。玩法位置、生命、攻击提交和死亡仍由对应玩法模块负责。

## 输入、输出与公共契约

### 输入

- 稳定 `FName PresentationId`。
- Idle、Move、Attack.Charge、Attack.Basic、Hit、Transform.Phase2、Death 等类型化表现命令。
- 朝向和武器视觉集合 ID。

### 输出

- 当前 Flipbook 可见表现。
- 不参与玩法裁决的逐帧碰撞快照和调试图形。

### 稳定公共类型/API

- `UReEcho2DPresentationCatalog`
- `UReEcho2DCharacterPresentationProfile`
- `UReEcho2DPresentationController`
- `UReEcho2DAnimationComponent`
- `UReEcho2DFrameCollisionDriver`

## 依赖方向

允许依赖 Engine、Paper2D 与 GameplayTags。`ReEcho` 单向依赖本模块。本模块禁止依赖 `ReEcho`、`ReEchoEnemies`、`ReEchoCombat`、`ReEchoWeapons` 或任何项目 Gameplay Actor。

## 运行时流程

```text
玩法事件 / 移动状态
  -> ReEcho 主模块 Host Adapter
  -> CombatPresentationCoordinator（动作身份、阶段排序与去重）
  -> PresentationId / 语义命令
  -> PresentationCatalog / Profile / FSM
  -> PresentationController
  -> AnimationComponent + FrameCollisionDriver
```

敌人 Gameplay Blueprint Class 由主模块 `UReEchoEnemyGameplayClassRegistry` 单独解析，不存入表现 Catalog。

## 内部组成

- Catalog：稳定 ID 到 Profile。
- Profile/FSM：美术可配置语义动画集合和转换策略。
- Controller：执行表现状态切换，持有显式动作占用；循环 Charge 只能由提交、结束或取消事件收束，不回写玩法。
- 主模块 Coordinator：不属于本 Runtime Module；把同一玩法动作阶段同时交给 Animation 与 VFX 轨，避免两个消费者建立彼此漂移的本地时钟。
- AnimationComponent：PaperFlipbook 渲染、比例、朝向和回退。
- FrameCollisionDriver：生成 Query/Debug 快照。

## 代码位置与阅读路线

| 目的 | Public 首读 | Private 实现 | 相关数据/资产 |
|---|---|---|---|
| 目录解析 | `ReEcho2DPresentationCatalog.h` | `ReEcho2DPresentationCatalog.cpp` | Character/Enemy/Echo 分域 Catalog |
| 状态播放 | `ReEcho2DPresentationController.h` | `ReEcho2DPresentationController.cpp` | Character/Enemy Profile、StateMachine |
| Flipbook 渲染 | `ReEcho2DAnimationComponent.h` | `ReEcho2DAnimationComponent.cpp` | Flipbook/Sprite |
| 逐帧查询 | `ReEcho2DFrameCollisionDriver.h` | `ReEcho2DFrameCollisionDriver.cpp` | Collision Track |

## 扩展方式

新增外观时在 `DataAsset/Character/Profiles` 或 `DataAsset/Enemy/Profiles` 创建 Profile 并注册到对应域 Catalog；共享 FSM 位于 `DataAsset/Common/Animation2D`。新增语义状态时扩展 GameplayTag、FSM 和 Profile Clip。敌人 Gameplay Blueprint 映射只在 `DataAsset/Enemy/Catalogs` 的 Registry 中扩展。

## 验证与测试

`ReEcho.Presentation.Animation2D` 覆盖生产 Profile、状态抢占、循环 Charge 取消、Transform 锁定、完成归宿与缺失资源回退；`ReEcho.Presentation.Combat` 覆盖动作阶段去重、收束和武器轨能力策略；`scripts/ue/audit_plan74_animation_contracts.py` 只读审计全部生产玩家、Echo 和怪物 Profile。脚点、比例、朝向与首帧闪烁仍由人工在 PIE 验收。

## 不变量与常见错误

- Catalog 不得引用主模块 Actor 或 Gameplay Blueprint Class。
- Animation/Profile 不得决定命中、伤害、移动或死亡。
- 不允许 SpawnIndex、EnemyKind 或生成顺序替代稳定 PresentationId。
- 移动类路径必须保留精确 Core Redirect，旧资产通过 Editor 保存后完成升级。
