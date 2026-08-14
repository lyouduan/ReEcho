# Plan 43 - 程序 - 怪物逻辑与表现解耦

## 协调

- Planner 负责人：Gavyn-side Planner。
- Executor 负责人：Gavyn-side AI（当前对话 AI）负责逻辑与允许范围内的集成；表现实现仍需 Plan40 所有权释放后再执行。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。计划分为 `Logic/Integration` 与 `Presentation` 两条实现 lane，最终由 Gavyn-side Planner 集成。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。代码、文档、构建与聚焦自动化已完成，等待用户 PIE 验收。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。用户负责最终 PIE 中的怪物行为、攻击节奏、受击手感、动画和保存继续验收。
- 本地规划 / 实现基线：`origin/main@4686e60`；实现开始前已重新 fetch，远端无新增提交。
- 实现分支：本地 `plan/43-enemy-logic-presentation-separation`，独立 worktree，不创建远端任务分支。
- 依赖 / 阻挡：
  - Plan41 已关闭，`ReEchoCombat` 与 `ReEchoWeapons` 是本 Plan 的稳定基础。
  - Plan40 当前拥有 `Presentation/Animation2D/**`、怪物表现调用点及相应资产；Presentation lane 启动前必须由 Plan40 Planner 释放或明确切分所有权。
  - 现有 Plan42 已保留 `ReEchoGameMode` 场景接缝与 `MOD-ReEcho.md`；本 Plan 的 Host/主流程集成不得与其同时写这些路径。新 `ReEchoEnemies` 模块的隔离逻辑工作可在契约锁定后并行。
  - 当前策划 `WorkbookWriter` 独占 `Design/Data/ReEchoData.xlsx` 及生成 CSV；本 Plan 不修改工作簿或生产 CSV。
  - Plan34 音频目录/设置保持独立；本 Plan 只保留或扩展语义事件适配，不修改音频目录、总线、设置 UI 或资产表。
- Writes：
  - 公共契约与逻辑 lane：`ReEcho.uproject`、`Source/ReEchoEnemies/**`、必要的 `Config/DefaultEngine.ini` Core Redirects、新模块测试。
  - Host/集成 lane：`Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*`、必要的 `Source/ReEcho/{Public,Private}/Encounter/ReEchoEnemyRosterComponent.*`、`Source/ReEcho/{Public,Private}/ReEchoGameMode.*`、`Source/ReEcho/ReEcho.Build.cs`、敌人保存/恢复和跨模块测试。
  - Presentation lane：新建 `Source/ReEcho/{Public,Private}/Presentation/Enemy/**`；只有在 Plan40 所有权释放并显式交接后，才可修改既有 `Presentation/Animation2D/**` 和怪物 Profile/资产引用。
  - 文档/门禁：本 Plan、`shared/CODEBASE_MAP/ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md`、`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`、新增 `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`、`scripts/validate_project.py`、最终 `GIT_RULES.md` 允许的 Win64 Editor 预构建包。
- Stable Reads：`Source/ReEchoCombat/**`、`Source/ReEchoWeapons/**`、`Source/ReEchoAudio/**`、`Source/ReEcho/{Public,Private}/Player/**`、`Run/**`、`Recording/**`、现有 Animation2D Profile/资产、`Config/DefaultGame.ini`、Plan40/41。
- 影响模式：
  - `Source/ReEchoEnemies/**`、模块描述符、Core Redirect、敌人公共事件/快照为 `SharedContract + Exclusive`。
  - Logic lane 与 Presentation lane 的新目录互为 `ReadOnly`，可以并行。
  - `ReEchoEnemyActor.*`、`ReEchoGameMode.*`、Build.cs、最终预构建包由 Integration lane 单写入。
  - 既有 Animation2D 路径保持 Plan40 的 `Exclusive`，交接前禁止写入。
- 兼容承诺 / 下游操作：不改变当前关卡数量、出生数量/随机种子、出生位置、怪物速度、攻击距离、攻击间隔、伤害、盾怪方向规则、爆破怪引信/爆炸/自毁、Boss 判定、元素反应、玩家/Echo 武器命中、存档继续结果、当前外观轮换及结束流程。旧保存必须确定性迁移或直接兼容加载。
- 明确排除：
  - 不新增怪物种类、Boss 招式、行为树、EQS、NavMesh、多人网络或对象池。
  - 不做怪物数值配表/XLSX 拆表，不调整策划平衡参数。
  - 不改玩家武器架构、攻击频率、伤害公式或 Combat 最终裁决。
  - 不重做怪物动画、美术、UI、血条布局、音频目录或音频资产。
  - 不把 Presentation 抽成新的全项目 Runtime Module；本 Plan 只建立怪物表现组件与文件所有权边界。

## 锁定目标

1. 把怪物的 AI、攻击节奏、爆破引信、受击位移状态、行为快照从 `AReEchoEnemyActor` 迁入独立 `ReEchoEnemies` Runtime Module。
2. 保留 `AReEchoEnemyActor` 作为轻量 UE 世界宿主/组合根，只负责组件装配、Transform/Collision 的实际应用、世界对象生命周期及模块间适配。
3. 建立独立 `UReEchoEnemyPresentationComponent`，只读消费怪物行为事件、Combat 结果与快照；表现失败、资源缺失或动画结束不能影响怪物逻辑、伤害或主流程。
4. 让逻辑和表现开发者在不同目录工作：逻辑开发不需要接触 Sprite/Flipbook/VFX/UI，表现开发不需要接触 AI、攻击冷却、生命、伤害或存档算法。
5. 怪物对玩家造成伤害、玩家/Echo 对怪物造成伤害都继续通过 `ReEchoCombat` 的 `FReEchoHitIntent -> ResolveHit -> CombatEvents` 单一结算路径。
6. 保存/继续、敌人全灭结束遭遇、自动目标选择和表现回归均保持现有玩家可见语义。
7. 完成后，修改怪物逻辑原则上只需修改 `ReEchoEnemies` 及其测试；修改怪物表现原则上只需修改 `Presentation/Enemy`、Profile/资产及表现测试。只有新增/改变公共事件或快照字段时才需要跨 lane 协调。

## 当前问题与迁移基线

当前 `AReEchoEnemyActor` 同时拥有以下职责：

- `EReEchoEnemyKind`、HP、移动速度、接触伤害和攻击间隔配置；
- 追踪玩家、移动、转向、接触攻击和爆破怪引信；
- GAS、Combatant、元素状态、伤害入口与 CombatTarget 接口；
- 受击击退、抖动、攻击脉冲和死亡缩放；
- Sprite/Flipbook/Profile、元素光环、血条、伤害数字和命中特效；
- Combat→Audio 适配；
- 保存/恢复运行态。

因此逻辑开发和表现/资源接入都会修改 `ReEchoEnemyActor.*`。当前主流程还通过 `TActorIterator<AReEchoEnemyActor>` 每帧扫描存活怪物，并在保存时再次扫描具体 Actor，形成 GameMode 对具体怪物类的耦合。

当前行为基线必须先由测试固定：

- 类型声明为 `Grunt | Shield | Bomber | Boss`，当前正常出生只包含 `Grunt | Bomber | Boss`；本 Plan 不擅自启用 Shield 出生。
- 普通非 Boss 外观按 `SpawnIndex` 在 Grunt/Rabbit/Goat/Fox Profile 中确定性轮换；外观不是新的玩法怪物类型。
- 普通接触攻击在距离 `<= 85 cm` 且 cooldown ready 时提交；玩家处于武器无敌窗口时仍消耗本次攻击 cooldown。
- Bomber 进入 TriggerRadius 后引信不可取消；到时即提交爆炸，在 DamageRadius 外不造成伤害，但仍自毁。
- Shield 正面伤害为 0、背面为原始伤害的 2 倍；最终扣血仍由 Combat Resolver 完成。
- 受击期间击退会实际改变 Actor 位置，因此属于逻辑/世界状态；抖动、缩放和色彩属于表现状态。
- Enemy 死亡后关闭碰撞并延迟 `0.45s` 销毁；GameMode 当前检测无存活敌人后结束遭遇。

## 架构影响与设计决策

- 受影响架构标识：新增 `MOD-ReEchoEnemies`、新增 `AREA-Enemies`；修改 `MOD-ReEcho`、`MOD-ReEchoCombat`，关联 `AREA-Encounter`、`AREA-Presentation`、`AREA-Tests`。`MOD-ReEchoWeapons` 只作为稳定攻击生产者读取，若公共契约不变则不修改。
- 对应模块文档：
  - 新增 `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`，并加入 Writes；
  - 维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，记录 EnemyHost、Roster 和 Presentation adapter 的真实位置；
  - 维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`，记录 Enemy 模块作为 CombatTarget/CombatEvents 消费者的依赖边界；
  - `MOD-ReEchoWeapons.md` 关闭前必须具名审阅；只有 Target/HitIntent 契约或代码位置变化时才进入实现 Writes。
- 设计意图：用“宿主组合 + 逻辑权威 + 战斗裁决 + 只读表现”的单向结构，消除逻辑和表现对同一个 Actor 文件的持续争用。
- 权威状态与依赖：新增 EnemyLogic 权威；Combat 继续独占生命、元素、伤害、击杀和死亡裁决；Presentation 只拥有表现实例与生命周期；GameMode 只编排遭遇，不复制怪物内部状态。
- 相关文档同步范围：`ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoCombat.md`、新增 `MOD-ReEchoEnemies.md`，以及经审阅后可能无需修改的 `MOD-ReEchoWeapons.md`、`MOD-ReEchoAudio.md`。

### 目标 Runtime Module 拓扑

```text
ReEcho
  ├──> ReEchoEnemies
  ├──> ReEchoWeapons
  ├──> ReEchoCombat
  └──> ReEchoAudio

ReEchoEnemies ──> ReEchoCombat
ReEchoWeapons ──> ReEchoCombat

ReEchoEnemies ─/─> ReEcho
ReEchoEnemies ─/─> ReEchoWeapons
ReEchoEnemies ─/─> ReEchoAudio
ReEchoEnemies ─/─> UI / Paper2D / Presentation assets
```

`ReEchoEnemies` 可依赖 UE Core/Engine 和 `ReEchoCombat` 的公共值类型/接口，但不得 include `AReEchoGameMode`、`AReEchoEnemyActor`、Widget、Animation2D、音频服务或主模块私有类型。

### 目标实例组成

```text
AReEchoEnemyActor                         // ReEcho：轻量组合根
  ├── UCapsuleComponent                   // 世界 Transform/阻挡/移动 Sweep 权威
  ├── UAbilitySystemComponent             // GAS 宿主
  ├── UReEchoCombatantComponent           // Combat：生命/属性/元素权威
  ├── UReEchoCombatEventsComponent        // Combat：类型化结果总线
  ├── UReEchoEnemyLogicComponent          // Enemies：AI/攻击/行为状态权威
  ├── UReEchoEnemyEventsComponent         // Enemies：行为事件总线
  ├── UReEchoEnemyPresentationComponent   // ReEcho：只读表现
  └── UReEchoCombatAudioAdapterComponent  // ReEcho：Combat→Audio 翻译
```

Runtime Module 是编译/依赖边界；Actor 实例实际持有的是由这些模块定义的组件或逻辑对象。本 Plan 不使用“Actor 持有模块”这种不准确表述。

### 权威状态表

| 状态 | 唯一权威 | 合法消费者 | 禁止的第二份状态 |
|---|---|---|---|
| Archetype、AI phase、攻击 cooldown、Bomber fuse、攻击序号 | `UReEchoEnemyLogicComponent` | Host、Presentation snapshot、Save adapter | EnemyActor 私有计时器、动画计时器 |
| 目标感知输入 | Host 在当前世界采样出的 `FReEchoEnemySenseSnapshot` | EnemyLogic | EnemyLogic 自行查询 PlayerController/GameMode |
| Actor Transform、Capsule 和阻挡移动 | `AReEchoEnemyActor`/Capsule | EnemyLogic 通过 Intent 请求 | Presentation 直接移动 Actor |
| 生命、属性、元素、最终伤害、死亡 | `UReEchoCombatantComponent` + Combat Resolver | EnemyLogic/Presentation/Audio 只读订阅 | EnemyLogic/Presentation 直接扣血 |
| 当前表现 Clip、渲染组件、VFX、血条、伤害数字 | `UReEchoEnemyPresentationComponent` | 用户可见输出 | Logic 根据动画结束推进状态 |
| 本场存活敌人集合 | `UReEchoEnemyRosterComponent` 或等价单一 roster | GameMode、保存捕获 | GameMode 每帧扫描世界并复制内部状态 |
| 遭遇时间/结束编排 | `AReEchoEncounterDirector` / `AReEchoGameMode` | Enemy sense、UI、Run | EnemyLogic 直接切换 Run phase |

### 公共契约

以下名称可由 Executor按 UE 命名规则微调，但语义、方向和禁止项为锁定设计；改变需回到 Planner/用户。

#### `FReEchoEnemyDefinition`

不可变逻辑定义，至少包含：

- `Archetype`：兼容 `Grunt | Shield | Bomber | Boss`；
- `MaxHealth`、`MoveSpeedCmPerSecond`、`ContactDamage`、`AttackIntervalSeconds`；
- `ContactRangeCm`；
- Bomber 的 `TriggerRadiusCm`、`DamageRadiusCm`、`FuseDurationSeconds`；
- Shield 的方向伤害策略 ID；
- 不含 Texture、Flipbook、Material、Sound、WidgetClass 或 Content path。

第一阶段由现有硬编码和 `UReEchoBalanceSettings` 编译出等价 Definition，不在本 Plan 中改 XLSX/CSV 权威。后续配表必须另立策划/程序 Plan。

#### `FReEchoEnemySenseSnapshot`

Host 每个推进步显式提供的只读世界输入，至少包含：

- self/target 位置、二维方向和距离；
- target 是否存在、是否存活、是否处于武器无敌窗口；
- 当前世界/遭遇时间；
- 必要时使用弱目标引用，但不得让 Logic 自行搜索全世界或访问 GameMode；
- 不含任何表现组件、动画帧或资源状态。

#### `FReEchoEnemyActionIntent`

EnemyLogic 单次推进的确定性输出，至少可以表达：

- 期望朝向；
- 正常移动或击退移动的方向/距离；
- `AttackCommitted`，包含唯一 AttackIdentity/Sequence、目标、RawDamage、DamageSource 和来源位置；
- Bomber 爆炸范围判定输入与提交后自毁意图；
- 当前行为状态用于只读 Snapshot；
- 不携带 Sprite、Animation、VFX 或 Audio 资源引用。

Host 负责把移动 Intent 通过 Capsule sweep 应用到世界，把攻击 Intent 转成 `FReEchoHitIntent` 交给 Combat。Combat 结果不由 EnemyLogic伪造。

#### `FReEchoEnemyLogicSnapshot`

用于表现读取、测试和保存适配，至少包含：

- Archetype、SpawnIndex、行为 phase；
- cooldown/fuse/受击剩余时间；
- Bomber fuse active；
- 当前移动/朝向、击退速度；
- AttackSequence；
- 只读且不能通过返回引用被外部修改。

逻辑快照不重复 Combatant 的生命和元素状态。完整保存由 `EnemyLogicSnapshot + CombatantSnapshot + Transform` 组合而成。

#### `FReEchoEnemyPresentationSnapshot`

由 Host 聚合 Logic 与 Combat 的只读数据，至少包含：

- `Archetype`、`AppearanceId`、`SpawnIndex`；
- `bMoving`、朝向、Fuse 进度、行为 phase；
- Combatant 的生命比例和元素附着；
- 不提供任何可写 Logic/Combat 指针。

`Archetype` 是玩法身份，`AppearanceId` 是表现身份。当前 Grunt/Rabbit/Goat/Fox 轮换通过 Host/表现配置确定，不得把四个外观误建成四个玩法 Archetype。

#### 事件契约

- `EnemyActionCommitted`：攻击门已经提交、cooldown/引信已消耗；不代表造成伤害。玩家无敌时仍可发布，以保持当前攻击动作语义。
- `EnemyFuseStarted` / `EnemyFuseProgress`：只描述逻辑引信状态，表现可以预警但不能取消或延长引信。
- `Combat OnHurt/OnDeath/OnElementStateChanged`：继续由 Combat 发布，是伤害和元素结果唯一事实来源。
- Presentation 和 Audio Adapter 可以独立订阅同一事件；二者互不调用。
- 动画完成、资源加载成功/失败、VFX 完成不得产生 AttackIntent、修改 cooldown、延迟死亡或结束遭遇。

### 行为与 Combat 的时序

```text
Encounter/GameMode 提供当前世界与目标
  -> Host 构造 EnemySenseSnapshot
  -> EnemyLogic::Advance
  -> EnemyActionIntent
      -> Host 应用 Capsule 移动
      -> Host 将攻击转成 Combat HitIntent
      -> Combat ResolveHit
          -> Combatant 改生命/元素
          -> Target OnHurt / OnDeath
          -> Source OnHit / OnKill
  -> EnemyEvents 发布已提交行为
  -> Presentation 消费 EnemyEvents + CombatEvents
  -> Audio Adapter 消费语义事件
```

表现订阅不位于上述逻辑链的返回路径中。

### 攻击提交语义

- 普通接触攻击：进入距离且 cooldown ready 即提交；提交后设置 cooldown。目标无敌、格挡或最终伤害为 0 都不回滚攻击动作。
- Bomber：进入 TriggerRadius 后引信成为逻辑状态；引信到时提交一次爆炸并自毁。目标是否在 DamageRadius 只决定是否生成有效目标 HitIntent，不决定 Bomber 是否自毁。
- Shield：方向修正继续由 CombatTarget 的 `ModifyIncomingRawDamage` 或等价 Combat 防御策略完成；EnemyLogic 不直接扣血。
- Enemy 攻击频率归 EnemyLogic，不强行复用玩家 `FReEchoWeaponLogic`。未来某类怪物装备数据驱动武器时另建适配，不让本 Plan 引入 `ReEchoEnemies -> ReEchoWeapons` 依赖。

### 受击和死亡拆分

- Combat OnHurt 到达后，EnemyLogic 接受明确 Hurt command 或直接订阅公共 CombatEvents，启动游戏性的击退时间/速度；Host 执行 swept movement。
- Presentation 同时独立订阅 OnHurt，播放抖动、缩放、命中特效和伤害数字。
- Combat OnDeath 后，Logic 停止产生移动/攻击；Host 立即关闭玩法碰撞并通知 Roster；Presentation 播放 `0.45s` 死亡表现，Actor lifespan 仅负责视觉残留，不阻塞遭遇逻辑结束。
- 当前“死亡表现尚在播放但遭遇已经判定清空”的玩家可见时序必须通过基线测试和 PIE 对比确认；未经用户决定不得改变。

### 主流程与 Roster

- `AReEchoGameMode` 仍决定何时开始/恢复遭遇以及生成哪些 Host，不把 Spawn 规则放进 EnemyLogic。
- 新增 `UReEchoEnemyRosterComponent` 或等价单一 roster，GameMode 生成成功后注册 EnemyHost；死亡/销毁时确定性注销。
- Roster 提供 `LivingCount`、`OnAllEnemiesDefeated`、按 SpawnIndex 排序的保存快照，不保存第二份生命/AI 状态。
- GameMode 改为消费 roster，不再每帧使用 `TActorIterator<AReEchoEnemyActor>` 判断全灭，也不在保存时重新扫描具体 Actor。
- `AReEchoEncounterDirector` 继续只拥有遭遇时钟、暂停和超时结束，不获得怪物 AI 或保存职责。

### 保存与兼容

- 当前 `FReEchoEnemyRuntimeState` 的所有可恢复字段必须有明确迁移目标：Kind、SpawnIndex、Transform、CurrentHealth、ElementState、AttackCooldown、FuseRemaining、FuseActive、HitReactionRemaining、KnockbackVelocity、ShakeDirection。
- 逻辑字段进入 EnemyLogic snapshot；生命/元素进入 Combatant snapshot；Transform 属于 Host；纯表现字段尽量不成为新逻辑权威。
- 若把反射类型从 `/Script/ReEcho` 迁到 `/Script/ReEchoEnemies`，必须添加精确 Core Redirect，并用旧路径资产/保存加载测试证明。
- 旧 SaveGame 版本必须继续加载；若布局变化则递增版本并提供确定性迁移，禁止静默丢怪、重置生命、重置 Bomber fuse 或交换 SpawnIndex。
- 保存时仅保存存活敌人；恢复后外观选择、目标 tie-break 和攻击序号必须确定性。

## 并行开发协议

### Lane A：Logic / Contract（Gavyn-side）

独占：

- `Source/ReEchoEnemies/**`；
- Enemy Definition/Sense/Intent/Snapshot/Event 公共契约；
- 纯逻辑测试；
- `MOD-ReEchoEnemies.md` 初稿。

不得修改：

- Animation2D、Profile、Content 资产、Widget、血条布局；
- Plan40/Presentation lane 文件；
- 未经 Integration lane 协调的 EnemyActor/GameMode。

### Lane B：Presentation（ReEcho teammate-side）

独占：

- `Source/ReEcho/{Public,Private}/Presentation/Enemy/**`；
- 经 Plan40 Planner 明确释放后的怪物表现 Profile/资产和表现测试。

只读消费：

- `FReEchoEnemyPresentationSnapshot`；
- `UReEchoEnemyEventsComponent`；
- `UReEchoCombatEventsComponent`。

不得修改：

- EnemyLogic、攻击 cooldown、伤害、Combatant、HitResolver、SaveGame、GameMode；
- 为了播放动画而新增第二份行为状态。

### Lane C：Host / Integration（单写入）

由 Gavyn-side Planner 指定一个 Executor 独占：

- `ReEchoEnemyActor.*`、`ReEchoGameMode.*`、Roster、Build.cs、uproject、Core Redirect、保存组合和跨模块集成测试；
- 最终冲突解决、FullRebuild、预构建包。

Lane A/B 不同时编辑 Host 文件。需要公共契约变化时先暂停受影响 lane，由 Planner 更新 Plan/Writes 后再继续。

### 可并行与必须串行的边界

- 可并行：Logic 的纯状态机/测试与 Presentation 新组件/表现测试，前提是本 Plan 的 Snapshot/Event 语义已锁定且 Plan40 所有权已交接。
- 必须串行：uproject/Build.cs 加模块、EnemyActor 组合、GameMode/Roster、Core Redirect、保存迁移、最终预构建包。
- Plan42 占用 GameMode 期间，Lane A 可在新模块中推进；Lane C 必须等待或由两边 Planner 对准确文件切片。
- 远端只有 `main`；不通过远端任务分支交换代码。跨机器 lane 的共享契约必须来自已评审、人工批准的 main 基线或由各 Planner 认可的准确提交，不允许复制未发布头文件形成两份真相。

## 锁定验收

### 架构与依赖

- [x] `ReEcho.uproject` 注册 `ReEchoEnemies` Runtime Module，模块可独立加载。
- [x] 依赖满足 `ReEcho -> ReEchoEnemies -> ReEchoCombat`，且 `ReEchoEnemies` 不依赖 `ReEcho`、Weapons、Audio、UI、Paper2D 或 Content 资产。
- [x] `AReEchoEnemyActor` 不再保存 AI cooldown、Bomber fuse、AttackSequence 或表现动画计时的第二份权威状态。
- [x] EnemyLogic 代码中不存在 Sprite、Flipbook、Material、Widget、AudioService、GameMode 或 PlayerController include/资源路径。
- [x] Presentation 代码不调用伤害、元素、cooldown、AI phase 修改接口；缺资源时逻辑继续运行。
- [x] 所有新反射类型、API 宏、Build.cs 正确；保留原 `AReEchoEnemyActor` 反射路径，无需新增 Core Redirect。

### 行为等价

- [x] Grunt、Bomber、Boss 的当前出生数量、种子、位置边界和 SpawnIndex 顺序不变；Shield 仍不被正常出生流程擅自启用。
- [x] 追踪、转向、移动速度、接触距离、攻击间隔和玩家无敌时 cooldown 消耗与基线一致。
- [x] Bomber TriggerRadius、FuseDuration、DamageRadius、伤害和自毁时序与基线一致。
- [x] Shield 正面 0、背面 2 倍规则保持；最终伤害只由 Combat Resolver 应用。
- [x] 玩家、Echo 的近战/投射物/光波仍可通过通用 CombatTarget 命中怪物；目标 tie-break 仍使用稳定 SpawnIndex。
- [x] 元素附着、反应、Burn tick、免疫、击杀和死亡事件无回归。
- [x] 怪物全灭和超时仍由原遭遇编排控制；Roster 只替换具体 Actor 扫描，死亡事实即时读取 EnemyLogic。

### 表现与并行边界

- [x] `UReEchoEnemyPresentationComponent` 仅凭 Snapshot/Event 驱动 Idle/Move/Attack/Hurt/Death、Fuse 状态与元素表现。
- [x] 当前 Grunt/Rabbit/Goat/Fox Profile 轮换与 Boss 回退保持，未把 Appearance 当作 Archetype。
- [x] Animation/VFX 缺失、播放失败、提前结束或更换资源不会改变攻击、移动、伤害、死亡或遭遇结束。
- [x] Logic 与 Presentation 实现位于不同目录；Host 集成由单一 lane 完成。
- [ ] 用户完成怪物行为和表现 PIE 验收。

### 保存与工程门禁

- [x] 局中保存/继续恢复所有存活怪物的 Transform、类型、SpawnIndex、生命、元素、cooldown、Bomber fuse、受击位移、攻击序号与自爆提交状态。
- [x] 保存版本升级为 v7；v4/v5/v6 保持在支持范围，新增字段对旧保存按零值确定性补齐，原 Actor 反射路径不变。
- [x] 新模块纯逻辑、Host/Combat/Weapons/Save/AttackMode 聚焦测试通过。
- [ ] UE 5.8 Development Build 与最终 `-FullRebuild` 通过，四模块基线扩展为五模块精选预构建包并通过指纹检查。
- [x] `python scripts/validate_project.py`、`git diff --check` 通过。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：实现前 fetch；当前实现基线 `origin/main@4686e60`。若远端前进，先报告物理冲突、逻辑冲突和集成耦合，由用户决定后再集成。
- 引擎/构建可用性：使用项目标准 UE 5.8；开始迁移前运行 Development Editor build，确认五模块改造前的四模块基线可构建。
- 现有聚焦测试结果：至少记录 `ReEcho.Combat.*`、`ReEcho.Weapons.*`、`ReEcho.Bomber*`、元素、保存/继续、AttackMode、Presentation Animation2D 的基线；缺少直接 Enemy 测试时先补 characterization tests，不以当前实现细节替代玩家可见语义。
- 活跃独占所有权或共享契约批准：确认 Plan40 的 Enemy/Animation2D 写入已释放或切片；确认 Plan42 的 GameMode 写入已释放或仅启动 Lane A；确认 WorkbookWriter 和 Plan34 不被越界写入。
- 基线损坏时的停止条件：出现与本 Plan 无关的玩家攻击、元素、保存、Animation2D 或 Arena 失败时，记录为基线缺陷并停止依赖该证据的迁移；不得把无关行为修复混入本 Plan。

## 实现提纲

1. **补基线测试**：把当前 Grunt/Shield/Bomber/Boss 配置、追踪/攻击、玩家无敌 cooldown、Shield 方向伤害、Bomber fuse/范围/自毁、外观轮换、死亡/全灭、保存继续固定为 characterization tests。
2. **建立模块与纯契约**：创建 `ReEchoEnemies`、API 宏、Definition/Sense/Intent/Snapshot/Event，不接入主流程；验证模块不反向依赖。
3. **提取 EnemyLogic**：把 AI phase、cooldown、Fuse、AttackSequence、击退状态迁入 `UReEchoEnemyLogicComponent`；使用显式 `Advance(Sense, Delta)`，禁用组件隐式 World Tick/全局查询。
4. **保持 Combat 单一裁决**：Host 将 Logic 的 AttackIntent 翻译成 HitIntent；EnemyHost 继续实现 `IReEchoCombatTarget` 并转发 Combatant/SpawnIndex/防御策略；移除 EnemyActor 直接扣血或重复元素逻辑。
5. **建立 Roster**：生成时注册、死亡/销毁时注销；替换 GameMode 每帧和保存时的具体 Actor 扫描；证明全灭、超时和清场只发生一次。
6. **迁移保存**：组合 Host Transform、EnemyLogic snapshot、Combatant snapshot；添加版本迁移/Core Redirect；验证旧保存与新往返。
7. **建立 Presentation 组件**：迁移 Sprite/Profile、动画、元素光环、命中特效、伤害数字、血条和死亡表现；只订阅 Snapshot/Event，不向 Logic/Combat 回写。
8. **主 Actor 瘦身**：EnemyActor 只保留组件创建、Sense 构造、Intent 应用、接口转发和生命周期；删除旧 AI/表现计时器及兼容 facade，除非旧资产反射加载明确需要。
9. **并行结果集成**：Integration lane 按锁定契约组合 Logic 与 Presentation，解决 Plan40/42 后续 main 差异；任何公共契约变化先更新 Plan 再让两 lane 适配。
10. **架构与验证**：更新 CODEBASE_MAP、模块边界校验器、聚焦/完整回归；用户 PIE 后在最终 main 做 FullRebuild、发布、释放 worktree/分支。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态边界 | `python scripts/validate_project.py`；include/Build.cs 扫描 | 五模块描述符一致；Enemies 无主模块/表现/音频/武器反向依赖 |
| 纯逻辑 | `ReEcho.Enemies.Logic.*` | 移动、攻击 cadence、Sense→Intent、Bomber、Shield、受击、Snapshot 确定性通过 |
| Combat 集成 | `ReEcho.Combat.*` + Enemy focused tests | 玩家↔怪物均只经 HitResolver；Hit/Hurt/Kill/Death/Element 事件正确 |
| Weapons/Echo | `ReEcho.Weapons.*`、AttackMode、Echo combat focused tests | 通用 CombatTarget、范围/路径、SpawnIndex tie-break 无回归 |
| Encounter | Enemy roster/clear/timeout tests | 注册/注销、全灭、超时、清场恰好一次；无每帧具体 Actor 扫描 |
| Save | Enemy old-save migration + new round-trip | 存活怪物完整恢复，cooldown/fuse/元素时间正确重基准 |
| Presentation | Enemy presentation contract/asset tests | 现有 Profile、Fallback、Attack/Hurt/Death/Fuse/元素表现可达，缺资源不影响逻辑 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UE 5.8 UHT/UBT 五个 Runtime Module 成功 |
| 完整回归 | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | 发现的完整套件通过；环境预检不可用时准确记录，不冒充通过 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`；`prebuilt_editor.py check` | 最终 main 五模块预构建包、Build ID、源码指纹一致 |
| 人工 | 用户 PIE | 普通怪/Bomber/Boss、自动/手动攻击、Echo、受击/死亡、全灭、保存继续和视觉手感通过 |

## 审核时需要用户确认的设计点

本 Plan 暂按以下方案锁定；用户审核时可纠正：

1. 新 Runtime Module 命名为 `ReEchoEnemies`，只含怪物逻辑与公共契约；怪物表现组件继续留在 `ReEcho` 主模块，暂不新建全项目 Presentation Runtime Module。
2. `AReEchoEnemyActor` 保留为世界宿主，逻辑和表现分别为组件；不把 EnemyHost 整体迁入逻辑模块。
3. 怪物攻击不复用玩家 `FReEchoWeaponLogic`，而是由 EnemyLogic 产生 AttackIntent，再进入 Combat；未来装备型怪物另行设计。
4. GameMode 使用单一 Roster 取代每帧 `TActorIterator<AReEchoEnemyActor>` 全灭检查和保存扫描。
5. 本 Plan 只做架构等价迁移，不同时把怪物数值搬进 XLSX，也不增加新怪物行为。

## 执行记录

### 变化

- 2026-08-14 用户审核并授权当前 Gavyn-side AI 开始执行；先实施不与 Plan40/42 重叠的 `ReEchoEnemies` 逻辑/契约 lane，Host/Presentation 到达前重新核对所有权。
- 2026-08-14 用户回复“继续”，确认采用 Planner 提议的精确切片：Plan43 接管 `ReEchoEnemyActor.*`、新 `Presentation/Enemy/**`，以及 GameMode 中仅与敌人生成、Roster、保存、恢复和全灭判断有关的代码；Plan40 保留通用 Animation2D/资产，Plan42 保留竞技场、地图和相机逻辑。
- 2026-08-14 创建第五个 Runtime Module `ReEchoEnemies`，实现资源无关 Definition/Sense/Intent/Snapshot、显式事件注入、EnemyLogic 与 EnemyEvents；模块只依赖 Core/Engine/Combat。
- 2026-08-14 把现有 Grunt/Shield/Bomber/Boss 数值编译为等价 Definition；实现接触攻击 cadence、目标无敌仍消费动作、不可取消 Fuse、一次性自爆、受击击退、死亡门控与快照恢复。
- 2026-08-14 新增模块内 `UReEchoEnemyRosterComponent`：只保存 Host/Logic 弱引用，按 SpawnIndex 稳定排序，存活状态即时读取 LogicSnapshot，不复制第二份 alive 真相。
- 2026-08-14 聚焦测试发现“攻击提交返回新空 Intent，丢失同帧移动”偏差，已在 `CommitAttack` 的单一入口改为补全现有 Intent；同时补充 `bSelfDestructCommitted` 防止 Host 销毁前重复爆炸。
- 2026-08-14 完成 Host/Presentation/主流程切片：`AReEchoEnemyActor` 只组合组件、采样 Sense、应用 Transform/Collision 和把 ActionIntent 翻译到 Combat；资源映射、动画、血条、元素与命中特效迁入 `Presentation/Enemy`。
- 2026-08-14 GameMode 的敌人全灭、清场、保存与恢复改用单一 Roster；删除旧 Bomber 规则副本，炸弹怪引信与自毁只由 EnemyLogic 决定。
- 2026-08-14 保存版本升至 v7，新增 `AttackSequence` 与 `bSelfDestructCommitted`；v4-v6 仍可加载，缺失字段按默认值归一化。

### 证据

- 实现基于 `origin/main@4686e60`；实施前与复核时 fetch 均确认远端未前进。
- `scripts/ue/Build-Editor.cmd -Configuration Development` 成功，UHT/UBT 生成并链接 `UnrealEditor-ReEchoEnemies.dll`，预构建清单扩展为五模块候选。
- `scripts/ue/Run-Automation.cmd -Filter ReEcho.Enemies.Logic` 发现 6 项并全部 `Result={Success}`：LegacyDefinitions、ContactCadence、InvulnerableTargetConsumesAttack、BomberFuse、HurtAndSnapshot、Roster。
- `scripts/ue/Run-Automation.cmd -Filter ReEcho.Enemies` 发现 8 项并全部成功；新增 Host 组合/保存与 Host→Logic→Combat 两次攻击管线验证。
- `ReEcho.Weapons` 9/9、`ReEcho.Combat` 7/7、`ReEcho.AttackMode` 7/7、`ReEcho.Run.SaveSnapshot` 1/1 聚焦回归通过。
- `python scripts/validate_project.py` 通过；`git diff --check` 通过。
- 校验器已新增 `ReEchoEnemies` 依赖/include/禁止 token 门禁，防止后续重新引入主模块、Weapons/Audio/表现依赖、全世界扫描、`FindComponentByClass`、直接伤害或 Content 资源路径。
- 完整 `ReEcho` 套件发现 80 项，其中 78 项成功；两个可独立复现且不在本 Plan 写集内的既有失败为 `Presentation.Animation2D.AssetProfiles`（Plan40 资产碰撞断言）和 `Run.EchoReplayResolver.EmptyStale`（回放空选择语义），未越界混入修复。

### 剩余风险

- 用户 PIE 尚未完成，怪物实际外观、动画、受击手感、Bomber 引信/爆炸、死亡残留、全灭和局中保存继续仍需人工确认。
- 最终 `-FullRebuild`、合入 main、远端差异审计、推送与 worktree/分支清理按照项目门禁在人工验收后执行。
- 全套自动化的两个独立既有失败分别属于 Plan40 表现资产和 Echo 回放语义，不阻塞本 Plan 聚焦证据，但仍应由其所有者另行处理。

### 人工验收结果/请求

- `PendingBeforeClose`；请用户在 PIE 验证普通怪/Bomber/Boss 的移动与攻击、Bomber 引信/爆炸/自毁、玩家与 Echo 命中、受击/元素/死亡表现、全灭结算，以及局中保存退出后继续。

### 架构文档审阅结果

- 已新增并维护 `MOD-ReEchoEnemies.md`，同步全局拓扑、索引、Combat 消费关系、Host/Presentation/Roster/保存的真实代码位置和 `AREA-Enemies` 校验。
- 已同步 `ARCHITECTURE.md`、CODEBASE_MAP 索引、`MOD-ReEcho.md` 与 `MOD-ReEchoCombat.md`；审阅确认 `MOD-ReEchoWeapons.md` 的公共契约和代码位置未变化，因此无需制造无信息修改。
