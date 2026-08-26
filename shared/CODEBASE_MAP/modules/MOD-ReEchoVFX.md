# `MOD-ReEchoVFX`：战斗 Niagara 表现入口

## 模块状态

- 当前形态：`MOD-ReEcho` 内的文档型逻辑入口，**不是**独立 Runtime Module。
- 功能检索标识：`AREA-Presentation`。
- 代码根：`Source/ReEcho/{Public,Private}/Presentation/VFX/`。
- 宿主装配：`AReEchoPlayerPawn`、`AReEchoEchoActor`、`AReEchoEnemyActor`。
- 资产根：`Content/VFX/`、`Content/Mat/`、`Content/00_Textures/`、`Content/01_Textures/`。
- 导入清单：`Design/Art/VFX/combat_vfx_import_manifest.csv`。

## 存在原因

攻击逻辑只应表达“兔子开始前摇”“某次投射物移动”“某次近战已提交”“某个目标实际受伤”等事实，不应知道 Niagara 路径、朝向轴、透明层级或销毁方式。反过来，粒子是否成功加载、播放多久或何时结束，也不能决定攻击提交、命中或伤害。

本入口把两类变化隔离开：Combat/Enemies/Weapons 发布资源中立的稳定语义；`UReEchoCombatVfxComponent` 集中选择资产并管理表现实例。程序可修改玩法而不散落资源路径，美术可替换同语义资产而不进入逻辑模块。

`FReEchoCombatVfxCatalog::GatherPreloadAssetPaths` 枚举全部战斗语义根、Echo 卡牌水草 Aura 和兔子代理纹理/材质。GameInstance 预加载器在菜单阶段异步持有这些 UObject；VFX Component 原同步加载仍是安全回退，加载失败只缺视觉并释放开始门控。

## 职责与排除项

**负责：**

- 集中维护战斗 VFX 语义到完整 `/Game/...Asset.Asset` 路径的唯一映射；
- 订阅 Host 上已经显式装配的 CombatEvents 与 EnemyEvents；
- 按事件位置、方向和生命周期创建、移动、停止 Niagara；
- 区分同短名、不同目录的“玩家受击”和“怪物受击”资产；
- 在资源缺失时限频报警并安全降级；
- 通过精确根清单、递归静态依赖和 SHA-256 manifest 复现美术资产导入。

**不负责：**

- 攻击频率、前摇/恢复计时、伤害、碰撞、阵营、元素、死亡和投射物轨迹；
- 读取 XLSX/CSV 决定玩法，或把 Niagara User Parameter 当作玩法输入；
- 用粒子碰撞、Notify、播放完成回调或自动销毁控制 HitIntent；
- 扫描整包美术资源、按短名猜资产、静默覆盖已有 `.uasset`；
- 保存瞬时粒子实例。继续游戏时由权威逻辑快照重新发布在途载体事实。

## 输入、输出与权威

| 输入 | 来源权威 | VFX 行为 |
|---|---|---|
| `FReEchoAttackCommittedEvent` | `MOD-ReEchoCombat` / Weapons 提交链 | 近战 Pattern 播放一次刀光 |
| `FReEchoDamageEvent::OnHurt` | `MOD-ReEchoCombat` | 仅 `AppliedDamage > 0` 时，在 Target 位置播放对应受击 |
| `FReEchoPresentationActionEvent` | EnemyHost 的 CombatPresentationCoordinator | Rabbit/Fox 的同一动作键与有序 Windup、Committed、Recovery、Ended/Cancelled 驱动阶段表现 |
| `FReEchoEnemyProjectileEvent` | EnemyHost 的逐球逻辑投射物 | 按 `(AttackIdentity, VolleyBallIndex)` 创建、移动和销毁唯一兔子子弹代理；位置直接采用事件快照 |
| `FReEchoCardEncounterTickResult::EchoAuraPulseCount` + 水草规则 | `MOD-ReEchoCards` / Run | 每次权威 2 秒脉冲在存活 Echo 的角色背景层播放一次 Water/Grass Aura；不另建计时器、不参与 4m 元素结算 |
| Actor Death / EndPlay | Combat/UE 生命周期 | 清理所有跟随和非自动销毁实例 |

VFX 唯一拥有的是 Niagara/Material Billboard 组件实例及其表现生命周期。逻辑投射物的 `AttackIdentity`、位置、方向、行进距离、碰撞和有效性仍由 Enemies/Host 拥有；VFX 中的 `(AttackIdentity, VolleyBallIndex) → MaterialBillboardComponent` 只是可丢弃的视觉索引。

Player、Enemy 与 Echo Host 的组件树统一提供 `EffectsRoot → AttackVfxRoot / HurtVfxRoot`。Echo 额外提供 `EchoAuraVfxRoot`：它依据当前 Flipbook 的稳定渲染 Bounds 中心定位，并让卡牌 Aura 使用角色当前透明排序的下一背景层。攻击提交、前摇、方向提示和冲刺读取 `AttackVfxRoot`，最终受伤读取 `HurtVfxRoot`；Aura 不复用这两个前景挂点。美术可在 Gameplay Blueprint 中独立调整挂点，但不能把 Aura 中心或层级改成玩法输入。兔子子弹从 Spawned 起就直接使用逻辑投射物世界位置，离开发射者后继续由逻辑事件覆盖位置，绝不附着人物、叠加挂点偏移或反向修改命中。

```text
Weapons / EnemyLogic
  → CombatEvents / EnemyEvents（稳定语义和值上下文）
  → CombatPresentationCoordinator（特殊动作阶段排序、去重和取消）
  → Enemy/Player/Echo Host 上的 UReEchoCombatVfxComponent
  → FReEchoCombatVfxCatalog（唯一资产映射）
  → Niagara Component（只读表现）

Niagara ─/─→ Commit / HitIntent / Combat / EnemyLogic / SaveGame
```

## 当前语义目录

| 语义 | 权威资产 | 播放约定 |
|---|---|---|
| RabbitCharging | `/Game/VFX/Monster/Rabbit/Particle/NS_Rabbit_Charging_01` | 世界位置；排序恒为兔子当前 Flipbook `+1`，Windup 开始，提交/结束清理 |
| RabbitProjectile | 核心球 `/Game/VFX/Monster/Rabbit/MI/BaseVFX003_Inst12`；柔光适配材质 `/Game/ReEcho/Materials/VFX/M_RabbitProjectileGlow` | 为每个逐球事件创建唯一 World Material Billboard；适配材质显式提供红色 Additive/Unlit/Emissive，不依赖 Niagara 粒子参数；核心直径精确等于事件碰撞直径，光晕直径为核心的 `1.5` 倍但不参与碰撞；位置逐帧覆盖为对应逻辑球位置，Ended/清场销毁 |
| PlayerHurt | `/Game/VFX/Monster/Rabbit/Particle/NS_Rabbit_BeAttacked_01` | 玩家实际受伤时世界位置单次播放 |
| FoxCharging | `/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_Charging` | 附着狐狸攻击挂点、前景，Windup 开始 |
| FoxDirection | `/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_arrow` | 启用发射器必须为 Local Space 且至少有一个启用 Renderer；`Kuang`、`Kuang002` 的 `InitializeParticle.Position Offset` 均为 `(0,0,0)`，粒子起点与组件原点一致。两套零 SpriteRotation 的 FaceCamera 箭头纹理尖端均朝图像右侧，故 authored visual forward axis 为局部 `+Y`；运行时组件世界旋转必须将该视觉轴映射到 `Event.LockedDirection`，不能用组件 `+X` 代替视觉方向。运行时实例继续使用 `X/Y=[-500,500]、Z=[-650,350]` 的局部固定 Bounds，覆盖最大 `800x600` 相机朝向 Sprite 及其 `500 cm` 半对角线。组件单独附着狐狸 Owner RootComponent，零 placement 偏移且组件/粒子起点精确等于 Actor/碰撞中心；Charging 与 FoxDash 仍附着地面高度的攻击挂点。Direction 保持非退化尺寸和战斗前景排序，Windup 与 Charging 同时开始并按锁定冲撞方向旋转，提交/结束/取消时清理；不新增玩法状态 |
| FoxDash | `/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_Trail` | 附着狐狸攻击挂点、前景；Committed 进入分帧 Active 时停止 Charging/Direction 并开始，RecoveryStarted（含完成/撞墙）、Ended/Cancelled/Death/EndPlay 时清理 |
| FoxImpact | `/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_BeAttacked` | 狐狸作为攻击来源且最终 `AppliedDamage > 0` 时，附着受击目标的 Hurt 挂点单次播放 |
| GoatSkill01 | `MoonStaff` + `NS_Goat_Skill02_BeAttacked` | 羊 Boss 从 Plan104 的 `DA_WeaponPresentation_MoonStaff` 读取持有贴图、尺寸和偏移；近战攻击窗口驱动法杖挥舞，并在法杖世界位置复用 Skill02 BeAttacked，玩法圆形命中不变 |
| GoatSkill02 | `NS_Goat_Skill02_Charging` / `Bullet` / `BeAttacked` | 两种投射技能共用；Charging 挂在 MoonStaff 根，Bullet 使用 Local Space。StationaryVolley 在 Recovery 窗口内逐颗发布四次 Spawned，MovingSpread 同帧发布三向 Spawned；每颗投影权威弹道、命中后经 Combat 结算单弹配表伤害并发布 Ended，实际 `AppliedDamage > 0` 时在角色世界命中坐标播放命中 |
| GoatSkill03 | `NS_Goat_Skill03_Charging` / `Alarming` / `BeAttacked` | Charging 附着 Boss；预警固定在锁定角色落点并使用显式世界向上法线，Boss 落在预警 XY 中心；BeAttacked 作为同尺寸地裂贴地播放，实际伤害仍由 Combat 裁决 |
| GoatSkill04 | `NS_Goat_Skill04_Charging` / `NS_Goat_Skill03_Alarming` / `Lighting` | Charging 开始时显示目标快照预警，权威锁定后重启到 LockedTargetLocation；Lighting 从预警中心释放并使用 CameraPlane mesh facing，伤害起点同一锁点；AbilityEnded/Death/清场清理 |
| PlayerMeleeSlash | `/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_01` | 近战提交位置和攻击方向，前景单次播放 |
| PlayerScytheSlash | `/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_01` | 镰刀提交位置和攻击方向，前景单次播放 |
| PlayerLongSwordImpact / PlayerScytheImpact | 对应 Weapon Presentation DA 的 `DamageApplied` Slot | 来源侧最终 `OnHit` 且 `AppliedDamage > 0` 时，在命中世界位置按攻击方向播放；两把武器可独立换资源 |
| PlayerBowFlight / Impact | `/Game/VFX/People/Bow/Particle/NS_People_Bow_Attack_01` / `NS_People_Bow_Boom` | 飞行 System 绑定权威投射物 Actor；保持资源内部 Renderer 与粒子模块不变，把交付 Niagara 的 authored local `+Y` 视觉轴在发射时按锁定攻击方向旋转一次；首次权威命中播放一次 Impact |
| PlayerGunFlight / Impact | `/Game/VFX/People/Bullet/Particle/NS_People_Bullet_Fly` / `NS_People_Bullet_spark` | 飞行 System 绑定权威投射物 Actor；首次权威命中播放一次 Impact |
| EnemyHurt | `/Game/VFX/People/Sword/Particle/NS_Rabbit_BeAttacked_01` | 怪物实际受伤时世界位置单次播放 |
| EchoWaterAura / EchoGrassAura | `/Game/VFX/Echo/Particle/NS_Echo_Water` / `NS_Echo_Grass` | `G_2_07/G_2_08` 共享 Cards 权威 2 秒脉冲；一次性附着 Echo 专用 Aura 挂点、角色视觉中心、角色 Priority `-1`，双卡同脉冲并发、自动结束 |

`PlayerMeleeSlash` 与 `PlayerScytheSlash` 分别绑定长剑、镰刀 AttackPattern，不是通用 Melee 标签。长剑、镰刀、弓和枪的 Niagara 引用从 Plan78 起由对应 Weapon Presentation DA 配置：长剑/镰刀的 AttackCommitted Slot 播放斩击轨迹，DamageApplied Slot 消费来源侧最终 `OnHit` 并在 `AppliedDamage > 0` 时播放命中；弓/枪的 Travel Slot 附着逻辑载体，DamageApplied Slot 只在首次正伤害权威结果播放。近战命中特效包含致死正伤害，不依赖目标是否还能播放 Hurt。四个需要服从组件方向/位移的 System 必须保证全部启用发射器使用 Local Space，并由自动化锁定；武器战斗 Niagara 使用 `1000` 前景排序下限压过角色与怪物表现。鞭和正式法杖已退出生产清单；表现缺失不得阻塞攻击。

禁止用 `NS_Rabbit_BeAttacked_01` 这个短名查找资产；玩家和怪物受击是两个不同 Package。

## 兔子投射物边界

兔子远程 Commit 后，EnemyHost 使用 `FReEchoEnemyProjectileLogic` 创建真实逻辑轨迹：移动散射同帧创建三条扇形轨迹，站定连发按 `ActiveSeconds` 依次发布四条同向轨迹；每球独立一次命中并分别发布 Spawned/Moved/Ended。`UReEchoCombatVfxComponent` 以 `(Attack.Source, Attack.Sequence, VolleyBallIndex)` 为每条已发布轨迹建立唯一 Material Billboard，直接使用事件位置，不会因只用 `Attack.Sequence` 互相覆盖。视觉使用交付资产的 `BaseVFX003_Inst12` 与 `0814_04` 绘制核心球；原 Niagara 的材质审计确认 `Inst1/Glo_c002` 是柔和渐变光晕，`Inst2/Glo_C178` 是圆环，`Inst3/Glo_c130` 是尖刺。由于原 Niagara 材质依赖 `Particles.Color` 等粒子输入，普通 Billboard 不直接复用这些实例；`M_RabbitProjectileGlow` 只复用 `Glo_c002`，显式定义红色 Additive/Unlit/Emissive 输出。核心可见直径等于逻辑碰撞直径，两种技能的单发尺寸固定一致；光晕只向外扩展表现，不参与碰撞。

交付的 `/Game/VFX/Monster/Rabbit/Particle/NS_Rabbit_Attack_02` 仍保留为原始美术资产和依赖清单根，但不再承担运行时三球位移。它内部自行模拟三颗粒子，历史实现同时移动 Niagara Component 与本地粒子，造成“画面覆盖却不命中 / 看不到球却受伤”；禁止恢复这条独立运动链。若未来要恢复尾迹或更复杂表现，必须制作读取逐球逻辑位置的单球适配资产，不能让粒子位置反向驱动玩法。

当前两条兔子能力的 `ProjectileSpeedCmPerSecond == 0`，兼容路径分别按各自 `MaxRangeCm / CooldownSeconds` 推导弹速，使旧表能够生成可见飞行载体；一旦策划填写正数，显式表值立即成为权威。当前 `ReEchoEnemyData.xlsx → enemy_abilities.csv` 的兔子能力伤害为 `1`；VFX 仍不拥有伤害、碰撞或禁伤开关。

旧 `AReEchoHitImpactActor / ReEchoAttackEffects / HitStarburst` 已删除；玩家和敌人受击只能走本模块的 `PlayerHurt / EnemyHurt` Niagara 语义，禁止再生成独立火焰星爆 Actor。

保存结构为兼容既有版本仍使用 `BossProjectiles` 字段名，但其数组现已承载通用敌方逻辑投射物；恢复后 Host 重发 Spawned，使视觉可重建。字段重命名需要独立存档迁移，不在表现任务中顺手修改。

## 资产导入与扩展方式

1. 先为新语义确定逻辑事件、空间上下文和停止条件；事件不能携带 Niagara 对象。
2. 把正式根资产加入 `scripts/art/import_combat_vfx.py`，生成/审阅新的递归依赖闭包。
3. `--copy` 只复制清单文件；目标存在但哈希不同时必须停止，不能覆盖。必须经 UE 修改的项目适配资产同时校验原包哈希与脚本中具名的项目哈希，未列入适配白名单的差异仍一律拒绝。
4. 在 `FReEchoCombatVfxCatalog` 增加完整路径，并在组件内消费已有类型化事件。
5. 增加资产加载和语义映射测试，构建后由用户在 PIE 验收尺寸、朝向、排序和裁剪。

当前清单是 8 个正式根、84 个静态依赖资产。`Map/`、`Developers/`、`SourceArt/`、备用特效和 `People/Bullet` 重复资产不属于本次导入。

## 代码与验证位置

| 目的 | 位置 |
|---|---|
| 语义与资产目录 | `Presentation/VFX/ReEchoCombatVfxCatalog.*` |
| 阶段协调 | `Presentation/Combat/ReEchoCombatPresentationCoordinator.*` |
| 事件订阅、生成和清理 | `Presentation/VFX/ReEchoCombatVfxComponent.*` |
| 人物攻击/受击/Aura 挂点 | `Player/ReEchoPlayerPawn.*`、`Graybox/ReEchoEnemyActor.*`、`Graybox/ReEchoEchoActor.*` 中的 `AttackVfxRoot` / `HurtVfxRoot`，以及 Echo 专用 `EchoAuraVfxRoot` |
| 敌人阶段/投射物事件契约 | `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyEventsComponent.h` |
| 敌人投射物装配 | `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*` |
| 玩家/Echo/敌人 Host 装配 | 对应 `PlayerPawn` / `EchoActor` / `EnemyActor` 构造函数 |
| 映射、资产加载与狐狸 Direction 运行时粒子/Renderer 自动化 | `Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp` |
| 首场资源清单与驻留 | `Presentation/VFX/ReEchoCombatVfxCatalog.*` → `Presentation/Loading/ReEchoRuntimeAssetPreloader.*`；测试为 `ReEchoRuntimeAssetPreloadTests.cpp` |
| 玩家武器攻击表现路由 | `Presentation/VFX/ReEchoCombatVfxCatalog.*`、`Graybox/ReEchoProjectileActor.*`、`Weapons/ReEchoWeaponActor.*`；正式清单仅含长剑、镰刀、弓和枪，MoonStaff Wave 是非生产动画辅助 |
| 导入器与聚焦测试 | `scripts/art/import_combat_vfx.py`、`scripts/art/test_import_combat_vfx.py`；狐狸冲刺只读审计为 `scripts/ue/audit_fox_dash_vfx.py`，方向箭头 Local Space 修复脚本为 `scripts/ue/fix_fox_direction_local_space.py` |
| Goat Boss 资产审计 | `scripts/ue/audit_goat_boss_vfx.py`；若 Editor 被跨平台 SDK 校验阻断，以包内 `/Game/` 引用递归闭包作为保守投递证据，并明确保留 Editor/PIE 验收 |
| 测试专用预览 Harness | `Presentation/VFX/ReEchoVfxPreviewActor.*`、`ReEchoVfxPreviewTests.cpp`；测试地图 author/verify 位于 `scripts/ue/author_vfx_test_scene.py` 与 `verify_vfx_test_scene.py` |

## 测试场景边界

`AReEchoVfxPreviewActor` 是 `/Game/ReEcho/Testing/VFX` 专用的 Editor-only 可丢弃宿主。Production 模式直接读取 Combat/Element Catalog 或 Weapon Presentation Profile，缺失槽显示 `Missing`；Sandbox Transform 只修改预览 Niagara Component。多目标 Conduct 的 Production 路径仅在 PIE 建立真实 Enemy/Combatant，通过 `ReEchoElementReaction::ApplyHitToWorld` 进入正式 resolver，并捕获正式 `FReEchoElementReactionResolvedEvent`；敌人既有 `UReEchoCombatVfxComponent` 消费权威 `ReactionLinks`。黄色 Authored Links 只属于 `NOT APPLIED` Visual Calibration，绝不进入 Production。青色范围圆/方向箭头读取正式 Event 半径和 Link，并随目标移动重绘。测试地图仍不能替代 `Level00` 的真实时序验收。

PIE 测试默认只初始化为 Ready；测试专用 Slate Overlay 与 Space 主动 Release，每次重建瞬时目标后重新运行 resolver，避免旧 VFX/状态无界叠加。Overlay 同时提供 Restart、Reset Targets、Clear、Reset Camera。测试相机复制正式倾斜正交默认参数到瞬时 `AReEchoArenaCameraActor`，WASD/QE/滚轮只修改测试 Rig 的 Camera Pan/Rotation/OrthoWidth，不写生产配置。

Conduct 蔓延延迟是纯表现配置：`FReEchoAttackIdentity.WeaponId` 在正式武器 Commit 时快照，VFX adapter 经 CSV Weapon VisualKey 解析 `UReEchoWeaponPresentationProfile.ConductLinkPropagationDelaySeconds`。resolver/伤害/完整 ReactionLinks 立即完成；adapter 保留 BFS Link 顺序，以 `index * delay` 调度。新 Event、解绑和 EndPlay 取消旧 timer batch；延迟触发时以 weak actors 重新读取当前 CombatTargetLocation，死亡/失效目标跳过。正式 Electricity 的两个 emitter 都是 World Space；端点的精确 Niagara Position 类型由资产自动化锁定。组件以世界原点和 identity rotation、`autoActivate=false` 生成，先用 LWC-safe `SetVariablePosition` 填当前世界坐标，再激活。长度与方向只由这两个端点决定，禁止叠加组件平移、旋转、Vec3 写入或未声明参数。delay=0 保持原同时播放。

## 不变量与常见错误

- 资源加载失败只能少一个视觉，不得让攻击失败。
- 受击只消费 Combat 的最终 `AppliedDamage`，不能从重叠或预测命中提前播放。
- 投射物表现绝不是位置真相；每颗球的 Moved 都直接覆盖对应 World Material Billboard 的 Transform，禁止在表现侧再次积分速度。补光晕只能更换同一代理的材质或叠加同位置表现层，不得恢复会自行运动的三球 Niagara。
- 攻击与受击必须使用两个独立的 Blueprint 可编辑挂点；不得重新合并到一个通用位置，也不得在 VFX 组件里按角色 ID 写死偏移。
- 所有角色攻击/受击战斗特效进入独立的全局前景排序带：`max(1000, 宿主当前 UReEcho2DAnimationComponent::TranslucencySortPriority + 1)`。Echo 卡牌 Water/Grass Aura 是唯一明确的角色背景状态层，动态使用所属角色 Priority `-1` 并保持视觉中心重合；不得把它误送入战斗前景带。挂点 Transform 和 Niagara 参数都不得改变玩法。
- 循环/跟随效果必须在 Death、Ended 和 EndPlay 都可清理。
- 资产朝向修正集中在适配器，禁止为了迁就特效轴修改玩法攻击方向。
- 旋转 Niagara Component 只能可靠影响 Local Space 发射器；有方向语义的资产必须同时校验启用发射器的 Simulation Space，并验证实际粒子位置/速度确实服从组件坐标系。组件 Transform、逻辑投射物轨迹和屏幕方向都正确时，禁止继续修改玩法方向来补偿 System 内部粒子模块。
- 资产说明中的“前向轴”不是运行时真相。兔子三球交付说明称 `+Y` 为中轴，但 CPU 粒子速度读回证明中间球实际为本地 `32.5°`；该适配只允许集中在 `FReEchoCombatVfxCatalog`，Host、敌人逻辑和投射物逻辑不得复制角度补偿。

## Shipping 打包：VFX 资产如何入包

VFX 资产（`/Game/VFX/...` 下的 `NS_*`/`M_*`/`MI_*`/`T_*`/`BP_*`，以及 `/Game/ReEcho/Materials/VFX` 下的兔子子弹材质/纹理）是**纯资产 + C++ 字符串路径运行时加载**（`ReEchoCombatVfxComponent.cpp` 经 `LoadObject<UNiagaraSystem>(ResolvePath(...))` 按路径挂载，不写进任何数据表、不构造成 GAS 装配）。这带来一项打包约束：

- cooker 只 cook「从根地图 `/Game/Level00` 出发、沿资产引用图可达」的资产。VFX 资产只在 `.cpp` 字符串路径里出现，没有任何 cooked 资产（地图/蓝图/DataAsset）硬/软引用它们，因此默认被 cook **漏掉**，Shipping 包运行时 `LoadObject` 返回 null，组件走降级分支只打 `Warning: [VFX] Missing semantic asset ... gameplay continues without it`——不崩、玩法照常、仅缺特效。
- **确定化投递**：`Config/DefaultGame.ini` 的 `+DirectoriesToAlwaysCook` 已显式列出 `/Game/VFX` 与 `/Game/ReEcho/Materials/VFX`，强制 cook 枚举并 stage 整个 VFX 树（递归覆盖 People/Monster/Weapon/Common 全部子目录及兔子子弹材质/纹理）。新增 VFX 子目录只要落在 `/Game/VFX` 之内即自动入包；若落在之外须同步补 ini 项。
- **验证手段（重要，避免误判）**：UE 5.8 默认启用 IoStore，游戏资产打包进 `ReEcho-Windows.utoc` + `ReEcho-Windows.ucas`，而 `ReEcho-Windows.pak` 仅含 ini 等零散文件。**切勿用 `UnrealPak -List ReEcho-Windows.pak` 验证 VFX 是否入包**（该 pak 不含游戏资产，会得到误导性的 0）；正确做法是对 IoStore 容器：
  `UnrealPak.exe ReEcho-Windows.utoc -List | Select-String "ReEcho/Content/VFX"`（IoStore 容器内路径前缀为 `../../../ReEcho/Content/VFX/...`，非 `/Game/VFX`）。
- 此契约仅影响打包投递，不改变 VFX 语义、资产路径映射、资产命名或运行时 `LoadObject` 契约；不影响 Development/PIE 既有路径。
- 前景/背景排序必须相对宿主当前 Flipbook 动态求 `+1/-1`，不能写固定全局值；排序不得复用为碰撞层或目标选择规则。
- 当前是主模块内领域；只有依赖和团队边界确实稳定、能避免循环时才考虑拆独立 Runtime Module。
# 元素反应 Niagara

`FReEchoElementReactionVfxCatalog` 是 Grass/Water 附着及六类反应的唯一语义资产映射，敌人体型只使用生产 Definition 的稳定 `PresentationId`。`UReEchoCombatVfxComponent` 管理可丢弃的持续附着组件，并按 Combat 提供的权威目标及 Conduct 发现边播放瞬时反应；正式根进入首场预加载。旧 `ElementAuraRing + ElementAttachmentLabel + ElementAuraLight` 三件套已经整体删除，不再保留文字、环形或点光源表现双轨。

Development `GMReaction` 只调用 `UReEchoCombatVfxComponent` 的直接预览入口：六种语义均在最近存活敌人的 HurtVfxRoot 播放并按调试预览时限停止，不准备元素附着、不调用伤害/反应解析器，也不发布权威 Combat 事件。该入口不得被正式玩法调用。

附着 Niagara 不再隐含固定的零偏移/单位缩放契约：`FReEchoCombatVfxCatalog::ResolvePlacement` 按语义提供资源局部修正和缩放策略。武器语义直接消费对应 Weapon Presentation Slot 的 `Offset` 完整 Transform 与 `bPreserveWorldSize`；例如长剑 `AttackCommitted` 的离地高度、倾角和大小只在武器 DA 配置，不得在语义分支硬编码。运行时先按 Commit 锁定方向对齐特效，再以四元数组合 DA 的局部旋转修正，禁止直接相加欧拉角导致换向后倾角留在旧世界轴。武器方向与弓箭一致，生成后将 Niagara Component 旋转切为绝对世界旋转并显式写入 Commit 方向，父级 AttackVfxRoot 只继续拥有位置和生命周期。世界尺寸型身体/武器特效可抵消角色/DA 的累计缩放，同时保留 DA Scale 作为艺术倍率；地面预警、逻辑投射物、精确命中和定向光束仍使用玩法事件给出的世界空间事实，不经角色附着根换算。
FullSpin 近战的伤害仍在 Combat Commit 当帧结算；刀光只在 Weapon Profile 的 `MotionDurationSeconds` 结束后按该次 Commit 已锁定方向播放，使武器先完成一周环绕再释放刀光。新 Commit 会替换尚未释放的旧表现计时，不改变攻击冷却或命中权威。
Burn Fire 由 `bBurnActive` 状态驱动并绑定目标，Growth 由各目标最终 Grass 附着驱动；Vaporize、Conduct 和两种 Enhance 以短生命周期 Niagara 绑定各自存活目标。多目标特效按目标自身动画排序，缺失元素 Niagara 只告警且不得影响玩法。
