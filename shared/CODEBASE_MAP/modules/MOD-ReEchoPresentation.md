# `MOD-ReEchoPresentation`：`ReEchoPresentation`

## 模块状态

- 当前状态：Plan74 实现候选。
- Runtime Module：`ReEchoPresentation`。
- Build 文件：`Source/ReEchoPresentation/ReEchoPresentation.Build.cs`。
- 主要目录：`Source/ReEchoPresentation/{Public,Private}/Presentation/Animation2D/`。
- 相关 Plan：Plan40、Plan50、Plan74、Plan102、Plan116。

## 存在原因

角色与怪物共享同一套 2D Profile、语义状态机、Flipbook 渲染和逐帧碰撞查询。独立模块阻止 Paper2D 资产类型、动画帧和表现状态渗入 GameMode、敌人逻辑、战斗与武器模块。

## 职责与排除项

### 负责

- `PresentationId -> Profile` 的 Cook 可追踪表现目录。
- 语义动画状态、Clip/Profile、状态机和播放控制。
- Flipbook 尺寸、朝向、锚点及安全隐藏。
- 角色稳定 `WorldHeight` 及归一化手持武器挂点；只描述外观空间，不引用具体武器。
- Character/Echo Profile 拥有 Cook 可见的小地图头像绑定；只描述对应外观，不处理小地图坐标或玩法身份。
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
- Move、Born、Attack.Charge、Attack.Basic、Hit、Transform.Phase2、Death 等类型化表现命令。除 Move 基础循环外均为可选能力；Profile 缺少语义时表现层保持当前有效画面并返回未播放，不借用其他语义。Born 是可抢占的非循环瞬时表现，完成后回到 Move；Death 仍为最高优先级终结独占。动画集转换缺少 `Transform.Phase2` 时，Controller 保留旧形态直到玩法完成事件，再原子切换目标动画集的基础循环，不能提前显示目标形态或把表现失败反馈成玩法失败。
- 朝向和武器视觉集合 ID。

### 输出

- 当前 Flipbook 可见表现。
- 当前 Player/Echo Profile 对应的只读小地图头像。
- 不参与玩法裁决的逐帧碰撞快照和调试图形。

### 稳定公共类型/API

- `UReEcho2DPresentationCatalog`
- `UReEcho2DCharacterPresentationProfile`（`MinimapIcon` 是该精确 Player/Echo 外观的 Cook 可见 UI 图标；可 Cook 的 `bUseAuthoredDeathPivot` 显式选择逐帧死亡脚点，`DeathGroundSink` 配置仅影响表现的下沉距离）
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

`UReEcho2DCharacterPresentationProfile::WorldHeight` 提供共享武器布局归一化所需的稳定角色高度；人物 Profile 不拥有左右手挂点。主模块从唯一 Weapon Presentation Catalog 读取通用左右挂点，本模块不解析 Weapon Profile，也不从当前 Flipbook Bounds 或人物宽度推算挂点。

`UReEcho2DCharacterPresentationProfile::MinimapIcon` 由 Character/Echo 分域 Profile 分别绑定。主模块只读取活动 Profile 并投影给 HUD；Presentation 模块不依赖 Widget、GameMode、Arena 坐标或 Recording。硬引用保证配置图标进入 cook，缺失图标由 UI 表现层安全降级。

Plan116 的元素反应字仍属于 `ReEcho` 主模块 Enemy Presentation/UI 适配：它只读 Combat 的反应完成事件并生成可丢弃世界表现，不进入本 Runtime Module 的 Profile/FSM，也不改变 `ReEchoPresentation` 的依赖方向。

## 内部组成

- Catalog：稳定 ID 到 Profile。
- Profile/FSM：美术可配置语义动画集合和转换策略。
- Controller：执行表现状态切换，持有显式动作占用；循环 Charge 只能由提交、结束或取消事件收束，不回写玩法。Enemy Host 会先按只读眩晕事实冻结 AnimationComponent，再取消玩法动作；暂停期间收到的结束或取消事件立即清理其他表现轨，但动画状态切换延迟到解除眩晕的同一帧，保证当前帧不跳回 Move、一次性动作不被误判完成且旧攻击不会在醒来后续播。死亡仍解除暂停并独占播放。Host 判定死亡后可调用 `BeginTerminalDeath` 独占播放一次非循环 Death；进入后拒绝 Move、Attack、Hit、Transform 与动画集切换，完成回调只通知 Host 销毁表现宿主，不返回 Move，也不裁决玩法死亡。死亡会清理瞬时 VFX，但保留独立 GroundShadow；死亡阶段以当前 Sprite 帧的底边中心持续锁定同一 Profile 脚点，并据此更新阴影中心和宽度，而不是使用整个 Flipbook 的合并边界，使画面在固定世界位置向下塌落且直到销毁前保持地面接触感。
- 主模块 Coordinator：不属于本 Runtime Module；把同一玩法动作阶段同时交给 Animation 与 VFX 轨，避免两个消费者建立彼此漂移的本地时钟。
- AnimationComponent：PaperFlipbook 渲染、比例、朝向和回退；Enemy Presentation 只读暴露 Born 是否仍活跃，Born 期间以当前 Sprite Bounds 逐帧计算脚底和 GroundShadow，供 Host 只延迟 Phase2 启动。死亡 Sprite 可用自定义 Pivot 提供逐帧主体脚点，Profile 以会进入 Cook 的 `bUseAuthoredDeathPivot` 显式声明该策略，主模块敌人表现据此固定 GroundShadow，并按 `DeathGroundSink` 让主体继续向下贴入阴影；未配置时继续使用当前帧 Bounds 底边中心。Death 仍可抢占 Born，且 authored pivot 仍是 Death 专用路径。运行时不得读取 PaperSprite 的 `PivotMode` 或 `CustomPivotPoint`，因为二者属于编辑器专用数据。
- FrameCollisionDriver：生成 Query/Debug 快照。

## 代码位置与阅读路线

| 目的 | Public 首读 | Private 实现 | 相关数据/资产 |
|---|---|---|---|
| 目录解析 | `ReEcho2DPresentationCatalog.h` | `ReEcho2DPresentationCatalog.cpp` | Character/Enemy/Echo 分域 Catalog |
| 状态播放 | `ReEcho2DPresentationController.h` | `ReEcho2DPresentationController.cpp` | Character/Enemy Profile、StateMachine |
| Flipbook 渲染 | `ReEcho2DAnimationComponent.h` | `ReEcho2DAnimationComponent.cpp` | Flipbook/Sprite |
| 逐帧查询 | `ReEcho2DFrameCollisionDriver.h` | `ReEcho2DFrameCollisionDriver.cpp` | Collision Track |

## 扩展方式

新增 Player/Echo 外观时在 `DataAsset/Character/Profiles` 创建并分别注册到对应域 Catalog，同时设置与该域外观一致的 `MinimapIcon`；敌人 Profile 仍位于 `DataAsset/Enemy/Profiles`。共享 FSM 位于 `DataAsset/Common/Animation2D`。新增语义状态时扩展 GameplayTag、FSM 和 Profile Clip。当前生产敌人的 Death 映射保持角色族一致：Grunt/Shield/Bomber/Slime 共用 Slime Death，Rabbit/Fox 使用各自 Death，GoatPriest/TimeGuard 共用 Goat Death；均为非循环、可重启动作。敌人 Gameplay Blueprint 映射只在 `DataAsset/Enemy/Catalogs` 的 Registry 中扩展。

## 验证与测试

`ReEcho.Presentation.Animation2D` 覆盖生产 Profile、Born 播放/完成归宿/缺失 no-op、状态抢占、循环 Charge 取消、Transform 锁定、Move 完成归宿、Death 终结独占/一次完成与缺失资源 no-op；`ReEcho.Presentation.Combat` 覆盖动作阶段去重、收束和武器轨能力策略；`scripts/ue/audit_plan82_animation_assets.py` 只读审计全部生产玩家、Echo、怪物 Profile、共享 Born 状态与源贴图导入链，`scripts/ue/audit_plan102_combat_hud.py` 审计四个 Player/Echo Profile 的八张小地图头像绑定。脚点、比例、朝向、Born/Death 实际播放完成、首帧闪烁和小地图图标可读性仍由人工在 PIE 验收。

## 不变量与常见错误

- Catalog 不得引用主模块 Actor 或 Gameplay Blueprint Class。
- Animation/Profile 不得决定命中、伤害、移动或死亡。
- 武器挂点只使用 Profile 的稳定参考高度；不得随 Move/Attack 的单帧 Bounds 重算。
- 不允许 SpawnIndex、EnemyKind 或生成顺序替代稳定 PresentationId。
- 移动类路径必须保留精确 Core Redirect，旧资产通过 Editor 保存后完成升级。
