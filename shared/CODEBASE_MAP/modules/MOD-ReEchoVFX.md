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
| `FReEchoEnemySpecialActionEvent` | `MOD-ReEchoEnemies` + EnemyHost | Rabbit/Fox 的 Windup、Committed、Ended 驱动阶段表现 |
| `FReEchoEnemyProjectileEvent` | EnemyHost 的逐球逻辑投射物 | 按 `(AttackIdentity, VolleyBallIndex)` 创建、移动和销毁唯一兔子子弹代理；位置直接采用事件快照 |
| Actor Death / EndPlay | Combat/UE 生命周期 | 清理所有跟随和非自动销毁实例 |

VFX 唯一拥有的是 Niagara/Material Billboard 组件实例及其表现生命周期。逻辑投射物的 `AttackIdentity`、位置、方向、行进距离、碰撞和有效性仍由 Enemies/Host 拥有；VFX 中的 `(AttackIdentity, VolleyBallIndex) → MaterialBillboardComponent` 只是可丢弃的视觉索引。

Player、Enemy 与 Echo Host 的组件树统一提供 `EffectsRoot → AttackVfxRoot / HurtVfxRoot`。两个子节点都是 Blueprint 可编辑的表现挂点：攻击提交、前摇、方向提示和冲刺读取 `AttackVfxRoot`，最终受伤读取 `HurtVfxRoot`。美术可在具体 Gameplay Blueprint 中独立调整两者的相对位置、旋转和缩放，不必修改 C++。兔子子弹从 Spawned 起就直接使用逻辑投射物世界位置，离开发射者后继续由逻辑事件覆盖位置，绝不附着人物、叠加挂点偏移或反向修改命中。

```text
Weapons / EnemyLogic
  → CombatEvents / EnemyEvents（稳定语义和值上下文）
  → Enemy/Player/Echo Host 上的 UReEchoCombatVfxComponent
  → FReEchoCombatVfxCatalog（唯一资产映射）
  → Niagara Component（只读表现）

Niagara ─/─→ Commit / HitIntent / Combat / EnemyLogic / SaveGame
```

## 当前语义目录

| 语义 | 权威资产 | 播放约定 |
|---|---|---|
| RabbitCharging | `/Game/VFX/Monster/Rabbit/Particle/NS_Rabbit_Charging_01` | 世界位置；排序恒为兔子当前 Flipbook `+1`，Windup 开始，提交/结束清理 |
| RabbitProjectile | `/Game/VFX/Monster/Rabbit/MI/BaseVFX003_Inst12`（纹理 `/Game/VFX/Monster/Rabbit/Tex/0814_04`） | 从交付 System 的正式单球材质创建三个 World Material Billboard；保留材质的 Emissive/Bloom 红色光晕，单球可见直径按事件碰撞直径缩放，位置逐帧覆盖为对应逻辑球位置，Ended/清场销毁 |
| PlayerHurt | `/Game/VFX/Monster/Rabbit/Particle/NS_Rabbit_BeAttacked_01` | 玩家实际受伤时世界位置单次播放 |
| FoxCharging | `/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_02` | 世界位置、前景、Windup 开始 |
| FoxDirection | `/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_01` | 攻击挂点、前景、Windup 开始 |
| FoxDash | `/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_04` | 附着狐狸攻击挂点、前景、提交到动作结束 |
| PlayerMeleeSlash | `/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_01` | 近战提交位置和攻击方向，前景单次播放 |
| EnemyHurt | `/Game/VFX/People/Sword/Particle/NS_Rabbit_BeAttacked_01` | 怪物实际受伤时世界位置单次播放 |

禁止用 `NS_Rabbit_BeAttacked_01` 这个短名查找资产；玩家和怪物受击是两个不同 Package。

## 兔子投射物边界

兔子远程 Commit 后，EnemyHost 使用 `FReEchoEnemyProjectileLogic` 创建三条真实逻辑轨迹，中心锁定目标、两侧扇形展开，每球独立一次命中并分别发布 Spawned/Moved/Ended。`UReEchoCombatVfxComponent` 以 `(Attack.Source, Attack.Sequence, VolleyBallIndex)` 建立三个独立 Material Billboard，直接使用事件位置；同一齐射的三球不会再因只用 `Attack.Sequence` 互相覆盖。视觉使用交付资产的 `BaseVFX003_Inst12` 发光材质及其 `0814_04` 红球纹理，既保留 Emissive/Bloom 光晕，又把可见直径映射到逻辑碰撞直径；材质与 Billboard 都不参与碰撞。

交付的 `/Game/VFX/Monster/Rabbit/Particle/NS_Rabbit_Attack_02` 仍保留为原始美术资产和依赖清单根，但不再承担运行时三球位移。它内部自行模拟三颗粒子，历史实现同时移动 Niagara Component 与本地粒子，造成“画面覆盖却不命中 / 看不到球却受伤”；禁止恢复这条独立运动链。若未来要恢复尾迹或更复杂表现，必须制作读取逐球逻辑位置的单球适配资产，不能让粒子位置反向驱动玩法。

当前表中 `ProjectileSpeedCmPerSecond == 0`，兼容路径暂按 `MaxRangeCm / CooldownSeconds` 推导 500 cm/s，使旧表能够生成可见飞行载体；一旦策划填写正数，显式表值立即成为权威。当前 `ReEchoEnemyData.xlsx → enemy_abilities.csv` 的兔子能力伤害为 `1`；VFX 仍不拥有伤害、碰撞或禁伤开关。

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
| 事件订阅、生成和清理 | `Presentation/VFX/ReEchoCombatVfxComponent.*` |
| 人物攻击/受击挂点 | `Player/ReEchoPlayerPawn.*`、`Graybox/ReEchoEnemyActor.*`、`Graybox/ReEchoEchoActor.*` 中的 `AttackVfxRoot` / `HurtVfxRoot` |
| 敌人阶段/投射物事件契约 | `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyEventsComponent.h` |
| 敌人投射物装配 | `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*` |
| 玩家/Echo/敌人 Host 装配 | 对应 `PlayerPawn` / `EchoActor` / `EnemyActor` 构造函数 |
| 映射与资产加载自动化 | `Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp` |
| 导入器与聚焦测试 | `scripts/art/import_combat_vfx.py`、`scripts/art/test_import_combat_vfx.py` |

## 不变量与常见错误

- 资源加载失败只能少一个视觉，不得让攻击失败。
- 受击只消费 Combat 的最终 `AppliedDamage`，不能从重叠或预测命中提前播放。
- 投射物表现绝不是位置真相；每颗球的 Moved 都直接覆盖对应 World Material Billboard 的 Transform，禁止在表现侧再次积分速度。补光晕只能更换同一代理的材质或叠加同位置表现层，不得恢复会自行运动的三球 Niagara。
- 攻击与受击必须使用两个独立的 Blueprint 可编辑挂点；不得重新合并到一个通用位置，也不得在 VFX 组件里按角色 ID 写死偏移。
- 所有角色战斗特效进入独立的全局前景排序带：`max(100, 宿主当前 UReEcho2DAnimationComponent::TranslucencySortPriority + 1)`。这既保证特效覆盖所属对象，也避免宿主之间动态脚点排序使某个对象的特效被其他角色遮住；不得为单个语义重新设置为背景层，挂点 Transform 也不得改变这一覆盖保证。
- 循环/跟随效果必须在 Death、Ended 和 EndPlay 都可清理。
- 资产朝向修正集中在适配器，禁止为了迁就特效轴修改玩法攻击方向。
- 旋转 Niagara Component 只能可靠影响 Local Space 发射器；有方向语义的资产必须同时校验启用发射器的 Simulation Space，并验证实际粒子位置/速度确实服从组件坐标系。组件 Transform、逻辑投射物轨迹和屏幕方向都正确时，禁止继续修改玩法方向来补偿 System 内部粒子模块。
- 资产说明中的“前向轴”不是运行时真相。兔子三球交付说明称 `+Y` 为中轴，但 CPU 粒子速度读回证明中间球实际为本地 `32.5°`；该适配只允许集中在 `FReEchoCombatVfxCatalog`，Host、敌人逻辑和投射物逻辑不得复制角度补偿。
- 前景/背景排序必须相对宿主当前 Flipbook 动态求 `+1/-1`，不能写固定全局值；排序不得复用为碰撞层或目标选择规则。
- 当前是主模块内领域；只有依赖和团队边界确实稳定、能避免循环时才考虑拆独立 Runtime Module。
