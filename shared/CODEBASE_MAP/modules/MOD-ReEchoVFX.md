# `MOD-ReEchoVFX`：战斗 Niagara 表现入口

## 模块状态

GroundTriple与开场共用Skill03地裂：消费bGroundedSlam或bPhaseOpening时以Boss自身地面锚点为中心，无升空/下降延时及提前地裂；仅ImpactResolved生成每击地裂。bGroundedSlam不是免伤标记，伤害由宿主的开场状态区分。非原地Skill03保持既有路径。

羊一→二、二→三阶段共用BeginBossTransformationEffects，蓄力统一改GoatSkill04Charging（NS_Goat_Skill04_Charging）；地面预警、爆发、挂点/缩放/时序保持不变。周围献祭怪物仍使用GoatSkill03Charging。

落地改为等待怪物变身动画实际结束；等待期间GoatSkill03Charging继续随身体播放，实际落地才清理，不按原3.9秒硬关闭。羊爆发时间不顺延。

怪物献祭蓄力独立于BeginSacrificeVisual，改为演出时钟1.0秒一次启动，仍在3.9秒落地清理；升空与怪物变身动画保持0.8秒。取消后重置启动状态。

后续同步调整：怪物蓄力VFX/升空/变身动画均从0.8秒开始，去掉单独0.5秒蓄力等待；VFX随身体保持至3.9秒落地，羊的爆发在3.5秒悬停结束，清理逻辑不变。

用户澄清：2.8秒触发的是怪物自身变身动画，不是羊；羊仍按原流程在4.0秒爆发，怪物蓄力跟随至4.4秒落地结束。下段“Boss2.8秒”说明已被本条覆盖。

献祭怪物改用与Boss变身一致的GoatSkill03Charging（不再GoatSkill02Impact），在聚焦结束时启动，按Flipbook体型缩放，实时跟随视觉中心；系统提前完成则重新激活，落地/取消结束，不再落地重播受击特效。Boss阶段提交及变身爆发移到空中挣扎开始时（默认2.8秒），怪物悬停到4.0秒才下落。

Boss一→二阶段献祭使用独立 `SacrificeEffect` 生命周期：只读表现请求 `GoatSkill02Impact`（NS_Goat_Skill02_BeAttacked），起点与逐帧位置来自目标Flipbook世界Bounds中心；不会发送伤害事件。升空消失时停止，原位显现开始时重播一次，演出结束/取消及StopAllEffects时清理。该效果不复用Boss技能待处理计时器，也不改Niagara资产。

- 当前形态：`MOD-ReEcho` 内的文档型逻辑入口，**不是**独立 Runtime Module。
- 功能检索标识：`AREA-Presentation`。
- 代码根：`Source/ReEcho/{Public,Private}/Presentation/VFX/`。
- 宿主装配：`AReEchoPlayerPawn`、`AReEchoEchoActor`、`AReEchoEnemyActor`。
- 资产根：`Content/VFX/`、`Content/Mat/`、`Content/00_Textures/`、`Content/01_Textures/`。
- 导入清单：`Design/Art/VFX/combat_vfx_import_manifest.csv`。

## 存在原因

Boss 变身使用独立的 Charge/Ground/Burst Niagara 实例，复用羊 Skill03 资源但不调用技能伤害。由 GameMode 显式 Begin/Burst/End，身体蓄力取当前 Flipbook 世界包围盒顶部，法阵取 Boss 地面高度且固定朝上；二、三阶段倍率来自程序 DA。实例与技能特效字段分离，阶段事件清技能特效时不误清演出；死亡/清场同步清理。资源内部可见性、亮度及画面节奏仍须 PIE 验收。

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

弓和枪的普通投射物命中复用 Gun Profile `AttackCommitted` 中的 Bullet Spark；只有最终编译投射物的
`ExplosionRadiusCm > 0` 时才改用 Bow Profile 中现有的 Boom 作为共享爆炸命中特效。选择依据是本次投射物已快照化的
最终玩法事实，而不是硬编码爆炸箭头或爆炸枪口的配件 ID；每颗投射物仍由表现 Actor 去重，只生成一次命中特效。

**不负责：**

- 攻击频率、前摇/恢复计时、伤害、碰撞、阵营、元素、死亡和投射物轨迹；
- 读取 XLSX/CSV 决定玩法，或把 Niagara User Parameter 当作玩法输入；
- 用粒子碰撞、Notify、播放完成回调或自动销毁控制 HitIntent；
- 扫描整包美术资源、按短名猜资产、静默覆盖已有 `.uasset`；
- 保存瞬时粒子实例。继续游戏时由权威逻辑快照重新发布在途载体事实。

## 输入、输出与权威

| 输入 | 来源权威 | VFX 行为 |
|---|---|---|
| `FReEchoAttackCommittedEvent` | `MOD-ReEchoCombat` / Weapons 提交链 | 长剑/镰刀 Pattern 播放一次刀光；枪 Pattern 在最终武器枪口播放一次 Muzzle |
| `FReEchoDamageEvent::OnHurt` | `MOD-ReEchoCombat` | 仅 `AppliedDamage > 0` 时，在 Target 位置播放对应受击；怪物致命命中改用世界实例以越过死亡清理 |
| `FReEchoPresentationActionEvent` | EnemyHost 的 CombatPresentationCoordinator | Rabbit/Fox 的同一动作键与有序 Windup、Committed、Recovery、Ended/Cancelled 驱动阶段表现 |
| `FReEchoEnemyProjectileEvent` | EnemyHost 的逐球逻辑投射物 | 按 `(AttackIdentity, VolleyBallIndex)` 创建、移动和销毁唯一兔子子弹代理；位置直接采用事件快照 |
| `FReEchoCardEncounterTickResult::EchoAuraPulseCount` + 水草规则 | `MOD-ReEchoCards` / Run | 每次权威 2 秒脉冲在存活 Echo 的角色背景层播放一次 Water/Grass Aura；不另建计时器、不参与 4m 元素结算 |
| Actor Death / EndPlay | Combat/UE 生命周期 | 清理所有跟随和非自动销毁实例 |

VFX 唯一拥有的是 Niagara/Material Billboard 组件实例及其表现生命周期。逻辑投射物的 `AttackIdentity`、位置、方向、行进距离、碰撞和有效性仍由 Enemies/Host 拥有；VFX 中的 `(AttackIdentity, VolleyBallIndex) → MaterialBillboardComponent` 只是可丢弃的视觉索引。

Player、Enemy 与 Echo Host 的组件树统一提供 `EffectsRoot → AttackVfxRoot / HurtVfxRoot`。Echo 额外提供 `EchoAuraVfxRoot`：它依据当前 Flipbook 的稳定渲染 Bounds 中心定位，并让卡牌 Aura 使用角色当前透明排序的下一背景层。攻击提交、前摇、方向提示和冲刺读取 `AttackVfxRoot`，最终受伤读取 `HurtVfxRoot`；Aura 不复用这两个前景挂点。美术可在 Gameplay Blueprint 中独立调整挂点，但不能把 Aura 中心或层级改成玩法输入。兔子子弹从 Spawned 起就直接使用逻辑投射物世界位置，离开发射者后继续由逻辑事件覆盖位置，绝不附着人物、叠加挂点偏移或反向修改命中。

TimeGuard Boss 的 `EnemyHurt` 是一次性世界特效，其位置从作者 `HurtVfxRoot` 向当前 Flipbook 渲染 Bounds 中心插值 50%，避免大体型 Boss 的受击表现半埋地下；普通敌人的非致命受击仍附着各自 `HurtVfxRoot`，致命正伤害则在命中世界位置生成一次性实例，避免随目标死亡清理。该修正不改变 Boss 攻击的命中位置。

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
| FoxDirection | `/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_arrow` | 启用发射器必须为 Local Space，两个启用 Sprite renderer 都把 `SpriteRotationBinding` 绑定到唯一 float 参数 `User.DirectionSpriteRotationDegrees`；Niagara renderer 输入单位是度，vertex factory 才转换为弧度。`Kuang`、`Kuang002` 的 `InitializeParticle.Position Offset` 均为 `(0,0,0)`，粒子起点与组件原点一致。两层 `PivotOffsetBinding` 必须无有效 source；由各自 512x512 纹理 alpha>0 bounds 中心和原 pivot `(0,0.370447)` 做半程校正后，`Kuang/0817_04` 的 `PivotInUVSpace=(0,0.4298418953)`，`Kuang002/0817_05` 为 `(0,0.4352235)`，只消除用户 PIE 所见的 180 度对称上下偏移的一半，不改世界位置。FaceCamera + Automatic/Unaligned 不以组件世界旋转驱动图像方向；运行时必须在 `Activate(true)` 前按实际 `PlayerCameraManager` 的 `ViewRight/ViewUp` 计算 `atan2(dot(LockedDirection,-ViewUp), dot(LockedDirection,ViewRight))` 并写入上述参数，零方向回退 `0°`、无相机回退到确定性的世界 `+Y/+X` 视图基。该屏幕右基与旋转符号来自用户 PIE 中“攻击向左、零旋转箭头尖端向右”的权威观察，不再把 billboard 图像解释为组件 authored axis。运行时实例继续使用 `X/Y=[-500,500]、Z=[-650,350]` 的局部固定 Bounds。组件单独零偏移附着狐狸 Owner RootComponent，组件/粒子起点精确等于 Actor/碰撞中心；Charging 与 FoxDash 仍附着地面高度的攻击挂点。Windup 与 Charging 同时开始，提交/结束/取消时清理；不新增玩法状态 |
| FoxDash | `/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_Trail` | 附着狐狸攻击挂点、前景；Committed 进入分帧 Active 时停止 Charging/Direction 并开始，RecoveryStarted（含完成/撞墙）、Ended/Cancelled/Death/EndPlay 时清理 |
| FoxImpact | `/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_BeAttacked` | 狐狸作为攻击来源且最终 `AppliedDamage > 0` 时，附着受击目标的 Hurt 挂点单次播放 |
| GoatSkill01 | `MoonStaff` + `NS_Goat_Skill02_BeAttacked` | 羊 Boss 从 Plan104 的 `DA_WeaponPresentation_MoonStaff` 读取持有贴图、尺寸和偏移；近战攻击窗口驱动法杖挥舞，并在法杖世界位置复用 Skill02 BeAttacked，玩法圆形命中不变 |
| GoatSkill02 | `NS_Goat_Skill02_Charging` / `Bullet` / `BeAttacked` | 两种投射技能共用；Charging 挂在 `BossWeaponTipRoot`，该节点位于包含 MoonStaff DA 最终左右偏移的法杖贴图顶部，Bullet 使用 Local Space。StationaryVolley 在 Recovery 窗口内逐颗发布四次 Spawned，MovingSpread 同帧发布三向 Spawned；每颗投影权威弹道、命中后经 Combat 结算单弹配表伤害并发布 Ended，实际 `AppliedDamage > 0` 时在角色世界命中坐标播放命中 |
| GoatSkill03 | `NS_Goat_Skill03_Charging` / `Alarming` / `BeAttacked` | 普通Skill03维持Charging/锁定角色地面预警与下砸地裂：Phase1/2落地生成，Phase3下降开始生成且落地去重。特殊开场bPhaseOpening不播放蓄力/预警、不跳跃，Boss原地连续Attack；每个攻击窗口由Host立即发ImpactResolved，地裂只生成一次，XY为Boss中心、Z对齐Boss自身GroundRoot/FootRoot，不读玩家高度。玩家伤害圈不因参演怪物震退半径扩大 |
| GoatSkill04 | `NS_Goat_Skill04_Charging` / `NS_Goat_Skill03_Alarming` / `Lighting` | Charging 开始时显示目标快照预警，权威锁定后重启到 LockedTargetLocation；Lighting 从预警中心释放并使用 CameraPlane mesh facing，伤害起点同一锁点；AbilityEnded/Death/清场清理 |
| PlayerMeleeSlash / Element Slash | 默认 `/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_01`；火/雷/草/水分别为 `NS_People_Sword_Attack_Fire` / `Thunder` / `Grass` / `Water` | 长剑提交时按该次攻击的最终元素选择专用斩击，继承默认长剑的挂点、镜头正面修正、左右播放与 Local Space 契约；无元素或资源不可用时回退默认斩击 |
| PlayerScytheSlash / Element Slash | 默认 `/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_01`；火/雷/草/水分别为 `NS_People_Sickle_Attack_Fire` / `Thunder` / `Grass` / `Water` | 镰刀提交时按该次攻击最终元素选择专用斩击，继承默认镰刀的延迟、挂点、CameraPlane 朝向和 Local Space 契约；无元素或资源不可用时回退默认斩击 |
| PlayerLongSwordImpact / PlayerScytheImpact | 对应 Weapon Presentation DA 的 `DamageApplied` Slot | 来源侧最终 `OnHit` 且 `AppliedDamage > 0` 时，在命中世界位置按攻击方向播放；两把武器可独立换资源 |
| PlayerBowFlight / Element Flight / Impact | 默认 `/Game/VFX/People/Bow/Particle/NS_People_Bow_Attack_01`；火/雷/草/水分别为 `NS_People_Bow_Attack_Fire` / `Thunder` / `Grass` / `Water`；Impact 为 `NS_People_Bow_Boom` | 飞行 System 绑定权威投射物 Actor；Bow 按该颗投射物已经解析的最终战斗元素选择专用 Travel，成功生成后隐藏字母元素回退；无元素或资源不可用时保留默认 Travel 与字母提示。全部飞行 System 使用 Local Space，并把 authored local `+Y` 视觉轴在发射时按锁定攻击方向旋转一次；首次权威命中仍播放同一个 Impact |
| PlayerGunFlight / Element Flight / Muzzle | 默认 `/Game/VFX/People/Bullet/Particle/NS_People_Bullet_Fly`；火/雷/草/水分别为 `NS_People_Bullet_Fire_Fly` / `Thunder_Fly` / `Grass_Fly` / `Water_Fly`；Muzzle 为 `NS_People_Bullet_spark` | 飞行 System 绑定权威投射物 Actor；Gun 按该颗投射物已经解析的最终战斗元素选择专用 Travel，成功生成后隐藏字母元素回退；无元素或资源不可用时保留默认 Travel 与字母提示。每次 `Pattern.GunShot` 提交时，Muzzle 附着最终 `WeaponAttackVfxRoot` 立即播放，不在目标命中点重复播放；Muzzle 启用发射器使用 Local Space，所有启用 Sprite renderer 绑定 `User.DirectionSpriteRotationDegrees`。枪模型只区分左右，因此运行时先把瞄准方向量化为相机水平纯左/纯右，再写入 Sprite 旋转，瞄准高度不驱动枪口主体上下浮动 |
| EnemyHurt | `/Game/VFX/People/Sword/Particle/NS_Rabbit_BeAttacked_01` | 怪物实际受伤时世界位置单次播放 |
| EchoWaterAura / EchoGrassAura | `/Game/VFX/Echo/Particle/NS_Echo_Water` / `NS_Echo_Grass` | `G_2_07/G_2_08` 共享 Cards 权威 2 秒脉冲；一次性附着 Echo 专用 Aura 挂点、角色视觉中心、角色 Priority `-1`，双卡同脉冲并发、自动结束 |
| EchoBorn | `/Game/VFX/Echo/Particle/NS_Echo_Born` | 只在第一关转第二关时自动播放：后台准备 Encounter 2 时启用 Echo 过渡玩法门禁；世界解暂停后仍允许回响、武器表现、法阵和相机 Tick，但禁止回放推进、目标搜索、攻击提交，并暂停 WeaponLogic 冷却。先关闭不透明 CG 覆盖层，再显示 Echo/武器并完整刷新脚点、`GroundRoot`、`GroundShadow`，随后从阴影中心激活法阵；只有镜头全部结束、`Director->StartEncounter()` 后才解除 Echo 与武器门禁并正常攻击。普通关卡、读档和跳关生成 Echo 时不播放。运行时使用 Catalog 默认 `1.0` Component Scale；Fountain004 Mesh 与 Sprite Fountain002/003/005 保持资产作者尺寸。法阵实例是独立世界组件，不附着 Echo；资源失败仍保持回响显形并继续；`GMEchoBorn` 复用同一路径 |

`PlayerMeleeSlash` 与 `PlayerScytheSlash` 分别绑定长剑、镰刀 AttackPattern，不是通用 Melee 标签。长剑、镰刀、弓和枪的默认 Niagara 引用从 Plan78 起由对应 Weapon Presentation DA 配置：长剑/镰刀的 AttackCommitted Slot 播放斩击轨迹，DamageApplied Slot 消费来源侧最终 `OnHit` 并在 `AppliedDamage > 0` 时播放命中；弓的 Travel Slot 附着逻辑载体且 DamageApplied 在首次正伤害结果播放；枪的默认 Travel Slot 附着逻辑载体，`NS_People_Bullet_spark` 由 AttackCommitted Slot 在最终武器枪口播放，不再作为目标命中反馈。长剑/镰刀四元素 Slash 与 Bow/Gun 四元素 Travel 都由 Combat VFX Catalog 消费攻击已经解析的 `EReEchoElement`，不建立第二份玩法元素；Development `GMElement` 在表现事件或投射物初始化前覆盖同一最终元素，使 VFX 与后续 `HitIntent` 一致，棱镜逐弹解析的最终元素也直接驱动对应 Travel。近战命中特效包含致死正伤害，不依赖目标是否还能播放 Hurt。所有需要服从组件方向/位移的武器 System（包括四个元素长剑/镰刀 Slash、Bow/Gun Travel 与 Gun Muzzle）必须保证全部启用发射器使用 Local Space，并由自动化锁定；长剑与镰刀斩击网格均按资源真实的本地 YZ 平面定向，本地 X 法线映射世界向上、本地 Y 映射攻击方向，Sprite 同步绑定 `User.GroundNormal` 与 `User.GroundTangent`，不得再由相机法线重新竖起。武器战斗 Niagara 使用 `1000` 前景排序下限压过角色与怪物表现。鞭和正式法杖已退出生产清单；表现缺失不得阻塞攻击。

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

`FReEchoElementReactionVfxCatalog` 是六类元素反应的唯一语义资产映射，敌人体型只使用生产 Definition 的稳定 `PresentationId`。任何元素附着状态都不创建标记 Niagara；`UReEchoCombatVfxComponent` 只在 Combat 发布权威反应结果后，按其目标及 Conduct 发现边播放反应特效，正式根进入首场预加载。旧 `ElementAuraRing + ElementAttachmentLabel + ElementAuraLight` 三件套已经整体删除，不再保留文字、环形或点光源表现双轨。

Development `GMReaction` 只调用 `UReEchoCombatVfxComponent` 的直接预览入口：六种语义均在最近存活敌人的 HurtVfxRoot 播放并按调试预览时限停止，不准备元素附着、不调用伤害/反应解析器，也不发布权威 Combat 事件。该入口不得被正式玩法调用。

附着 Niagara 不再隐含固定的零偏移/单位缩放契约：`FReEchoCombatVfxCatalog::ResolvePlacement` 按语义提供资源局部修正和缩放策略。武器语义直接消费对应 Weapon Presentation Slot 的 `Offset` 完整 Transform 与 `bPreserveWorldSize`；例如长剑 `AttackCommitted` 的离地高度、倾角和大小只在武器 DA 配置，不得在语义分支硬编码。运行时先按 Commit 锁定方向对齐特效，再以四元数组合 DA 的局部旋转修正，禁止直接相加欧拉角导致换向后倾角留在旧世界轴。武器方向与弓箭一致，生成后将 Niagara Component 旋转切为绝对世界旋转并显式写入 Commit 方向，父级 AttackVfxRoot 只继续拥有位置和生命周期。世界尺寸型身体/武器特效可抵消角色/DA 的累计缩放，同时保留 DA Scale 作为艺术倍率；地面预警、逻辑投射物、精确命中和定向光束仍使用玩法事件给出的世界空间事实，不经角色附着根换算。

Plan148 在上述作者 Scale 之后增加可选攻击范围倍率：Combat 事件提供本次 Commit 的最终/基础范围比，Weapon Slot 只声明局部轴遮罩与表现钳制。长剑默认只扩展资源局部斩击方向，镰刀默认扩展相机平面两个径向轴，法线/厚度轴保持不变；旧二进制 Profile 尚未重存时由同语义迁移默认值保证行为，显式 DA 配置优先。延迟生成必须捕获提交倍率，禁止在 timer 回调中重新读取当前符文。
FullSpin 近战的伤害仍在 Combat Commit 当帧结算；刀光只在 Weapon Profile 的 `MotionDurationSeconds` 结束后按该次 Commit 已锁定方向播放，使武器先完成一周环绕再释放刀光。新 Commit 会替换尚未释放的旧表现计时，不改变攻击冷却或命中权威。
Burn Fire 和 Water 反应都按 `PresentationId` 区分 Slime/Rabbit/Fox/TimeGuard(Goat) 体型资源；TimeGuard 的元素反应与受击特效共用 `HurtVfxRoot` 到当前 Flipbook 渲染中心的半高世界位置，再转换成跟随挂点的局部偏移。Burn 由反应产生的 `bBurnActive` 定时状态驱动并绑定目标；Growth、Vaporize、Conduct 和两种 Enhance 只由反应结果事件创建。Burn、Growth、Vaporize 与两种 Enhance 在生成瞬间读取目标 Flipbook 稳定 Render Bounds 经组件最终 Transform 得到的世界直径，再按各 Niagara Fixed Bounds 与语义 `TargetCoverageRatio` 等比归一；随后反向补偿 `HurtVfxRoot` 累计缩放，保证不同怪物和 Boss 的最终特效范围随其最终 Flipbook 大小一致且动画帧切换不引起缩放抖动。`NS_Element_Grass` 的全部启用发射器使用 Local Space，保证运行时归一同时约束粒子位置、速度与大小。Growth、Vaporize 与两种 Enhance 即使复用可循环元素素材，也由适配器在 2 秒硬上限时销毁；不得进入元素附着的状态生命周期。Conduct 是双目标连线，不参与单体 Flipbook 尺寸归一；其每条电链必须同时取得来源与目标显式配置的 `HurtVfxRoot` 世界坐标，任一挂点缺失时不生成该电链且不回退 Actor 根节点。多目标特效按目标自身动画排序，缺失元素 Niagara 只告警且不得影响玩法。

卡牌 `G_2_30`（连接，连接！）启用时，GameMode 只把卡牌规则和当前存活 Echo 集合投影给玩家的 `UReEchoCombatVfxComponent`；组件以 Echo Actor 为稳定键维护 `/Game/VFX/Echo/Particle/NS_Echo_Chain`，每个存活 Echo 独立一条。固定步只同步规则与集合生命周期，VFX Component 每个渲染 Tick 通过当前 `UReEcho2DAnimationComponent` 的 Flipbook `RenderBounds.Origin` 计算玩家与 Echo 的真实渲染中心世界坐标，不复用受击挂点。该资源沿用 Beam 模板的参数契约：`User.StartPosition` 为绝对世界位置，`User.EndPosition` 为相对起点的位移；Niagara Component 保持世界原点单位变换。Chain Niagara 的每个启用 Emitter 在 Particle Update 栈执行 `Update Beam`，并把 VFX adapter 设为 Tick prerequisite，在同帧端点写入之后更新现有 Ribbon；持续移动时禁止重初始化或重生。组件还会逐帧用两端的 effect-local 坐标和有限安全边距更新 System Fixed Bounds，避免长距离 Ribbon 超出资产静态 Bounds 后被误剔除，而不采用无限 Bounds。连接线正伤害沿统一 Hurt 事件复用 `EnemyHurt`，在命中世界位置生成一次性特效；所有怪物致死正伤害均沿同一世界实例策略保留受击表现。链条生命周期由卡牌玩法状态持有：资源自身播放完成时，只要玩家及对应 Echo 仍存活便重新激活；仅在卡牌关闭、玩家或 Echo 死亡、Echo 清场、Boss 阶段退休 Echo 或组件 EndPlay 时清理。资源或当前 Flipbook 缺失时只缺视觉，不改变连接线穿越伤害。

Echo Born 不复用居中的 `EchoAuraVfxRoot`，普通 `InitializeEcho` 也不登记出生表现；只有第一关转第二关的专用延迟显现链路会先定位并隐藏 Echo，再武装并播放法阵。法阵中心使用 Echo Actor 的稳定世界 XY，并只从 `GroundShadow` 读取地面 Z，避免阴影的镜头相关 2D 补偿污染固定世界落点。Niagara 作为不附着 Echo 的独立世界组件创建，先设置暂停 Tick、排序、`User.GroundNormal=(0,0,1)` 及 `User.GroundTangent=(1,0,0)`，再显式激活；Sprite 的 CustomFacing 与 CustomAlignment 同时锁定法线和面内方向，不会再绕世界 Up 跟随相机旋转。第一关转第二关时镜头先定位隐藏 Echo 并保持静止，在最终 CG 遮罩仍存在时先激活法阵，再关闭过渡界面；法阵启动 0.4 秒后显示 Echo/武器并开始后续镜头拉远，法阵继续播放到自身生命周期结束。PreserveWorldSize 保持生成时的世界尺寸，且不阻塞初始化、回放、攻击或伤害。该 System 从 Goat 地面法阵复制为独立资产，其 Sprite/Ribbon/Mesh Renderer 只引用 Echo 目录下 4 个青蓝材质实例；Boss 原 System 和共享材质保持不变。

Echo Born 的 Editor authoring 脚本不得在 `RequestCompile` 后立即保存退出：所有 Renderer、颜色、Mesh 或 Local Space 修改结束后必须调用 `CompileNiagaraSystemAndWait`，等待包含 GPU Shader 的编译完成并确认无 outstanding request，再保存 System。这样首次 PIE 激活只读取已编译资产，不把长时间 Niagara 编译转移到游戏主线程。
