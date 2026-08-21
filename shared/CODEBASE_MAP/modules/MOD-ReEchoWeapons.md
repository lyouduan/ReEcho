# `MOD-ReEchoWeapons` 详细设计

## 模块状态

- Runtime Module：`ReEchoWeapons`。
- 代码根：`Source/ReEchoWeapons/`。
- 架构标识：`MOD-ReEchoWeapons`；功能检索标识：`AREA-Weapons`。
- 当前状态：Plan41 `Review` 候选；等待用户 PIE 后随 Plan41 晋升到 `main`。

## 存在原因

武器规则原先同时存在于策划表、主模块 WeaponRuntime、WeaponActor、投射物/法杖 Actor 与 GAS 定时器中。`AttackIntervalSeconds` 和步骤 `DurationSeconds` 曾形成两道频率门，临时 busy 会结束 held Ability，导致“只攻击第一下”。表现 Actor 同时持有逻辑飞行和命中职责，也容易把资源生命周期变成玩法规则。

本模块集中资源无关的武器 Definition、唯一普通攻击节拍、步骤提交事务、近战几何与逻辑 Projectile/Wave。主模块仍拥有数据读取和可见 Actor，但只负责把输入编译/装配到逻辑对象以及消费结果。

## 职责与排除项

**负责：**

- `FReEchoWeaponDefinition` / Step Definition 等不可变运行时输入；
- `FReEchoWeaponLogic` 的 readiness、步骤游标、攻击序号、暴击/元素游标和提交/回滚；
- 唯一普通攻击频率 `AttackIntervalSeconds / AttackSpeed`；
- 近战范围/弧形与投射方向等资源无关几何；
- `UReEchoProjectileLogicComponent` 的位置、速度、半径、穿透/爆炸、寿命、已命中集合和 Snapshot；
- 完整 `FReEchoAttackIdentity` 与 `FReEchoHitIntent` 的传递。

**不负责：**

- XLSX/CSV 读取、Schema 校验、构筑/商店/存档；
- 最终伤害、格挡、元素、生命、击杀或死亡；
- Sprite/Mesh、动画、VFX、材质、音频、镜头和 Widget；
- Pawn 输入、自动目标筛选、攻击模式 held 状态、Enemy AI。

## 权威状态

| 状态 | 唯一所有者 | 备注 |
|---|---|---|
| 当前不可变武器与步骤定义 | `FReEchoWeaponLogic::Definition` | 由主模块从已校验数据一次编译 |
| 下一次普通攻击 readiness | `FReEchoWeaponLogic` | 唯一频率门；其他系统只能请求或查询剩余时间 |
| 步骤游标、成功攻击数、暴击/元素游标 | `FReEchoWeaponLogic` | 成功 Commit 原子推进；执行失败完整回滚，CommitId 不复用 |
| Projectile/Wave 逻辑位置、寿命、已命中集合 | `UReEchoProjectileLogicComponent` | 表现 Actor 只跟随 Snapshot |
| 最终生命/元素/死亡 | 不属于本模块，由 `MOD-ReEchoCombat` 持有 | Weapons 只能提交 HitIntent |

## 输入、输出与公共契约

### 输入

- 主模块 `ReEchoWeaponRuntime::CompileLogicDefinition` 把 CSV/构筑结果编译为无资源引用的 `FReEchoWeaponDefinition`。
- 主模块装备适配在编译前消费 Cards 的只读规则快照；`G_3_22` 只扩大非 `Core` 槽容量，并把已经校验的装备结果交给 Weapons。
- Attack host 提供 Source、`FReEchoStatBlock`、目标/世界上下文并调用 `TryCommitBasicAttack` 或主动攻击入口。
- 逻辑投射物初始化接收已快照的 `FReEchoLogicalProjectileSpec`，之后不回读 WeaponActor 的可写字段。

### 输出

- `FReEchoWeaponAttackCommit`：成功通过唯一 readiness 后产生，包含完整 AttackIdentity、Step、伤害参数、元素、方向与载体配置。
- `FReEchoWeaponSnapshot`：当前 Definition、步骤、readiness 与非阻塞行为剩余时间的只读副本。
- `FReEchoProjectileSnapshot`：稳定 ProjectileId、AttackIdentity、逻辑位置/方向、生命周期与有效状态。
- `FReEchoHitIntent`：近战或载体发现候选命中后提交给 Combat；不得提前填最终伤害/死亡。

## 依赖方向

```text
ReEcho ─────────────→ ReEchoWeapons ─────────→ ReEchoCombat

ReEchoWeapons ─/─→ ReEcho / ReEchoAudio / UI / Presentation
ReEchoCombat  ─/─→ ReEchoWeapons
```

模块公共依赖只有 UE Core/Engine 与 `ReEchoCombat` 的窄契约。策划数据、Run、Cards、Recording、资源和具体 Actor 都由主模块适配；`ReEchoWeapons` 不依赖 `ReEchoCards`。

## 运行时流程

### 攻击候选与阵营

Weapons 不拥有阵营规则，但所有候选载体必须消费 Combat 的 `ReEchoCombatRelations`。`FReEchoWeaponLogic` 在 Commit 时把来源 Actor 的阵营快照进 `FReEchoAttackIdentity`；近战弧、投射物连续路径和爆炸半径在形成 HitIntent 前排除同阵营目标。这样回响与玩家共用武器逻辑时不会互伤，投射物也不会因先撞到玩家而提前销毁。最终 Resolver 仍会重复校验，Weapons 的过滤只负责候选正确性，不取代 Combat 裁决。

主模块 `AReEchoWeaponActor` 通过 `IReEchoCombatAffiliation` 解析事件归因：玩家为 `DamageSource::Player`，回响为 `DamageSource::Echo`。不要在具体 Projectile/Wave 中通过 `Cast<AReEchoEchoActor>` 重复判断来源类型。

### Definition 编译与装配

```text
XLSX → CSV → ReEcho Data Reader
  → FReEchoEffectiveWeaponDefinition（主模块）
  → CompileLogicDefinition
  → FReEchoWeaponDefinition（资源无关）
  → FReEchoWeaponLogic::Initialize
  → WeaponActor 仅组合逻辑与表现
```

CSV Row 和资源 key 不能进入逻辑模块，避免数据加载器或表现层成为第二个规则解释器。

### 唯一普通攻击节拍

```text
BasicAttack Ability/Host 请求 TryCommit
  → WeaponLogic 检查 readiness
  ├─ 未就绪：返回剩余时间，held 循环继续等待
  └─ 就绪：原子生成 Commit、推进步骤/游标并设置下一 readiness
       → 世界执行成功：确认 Commit
       → 载体创建/执行失败：回滚 readiness 与游标，不复用 CommitId
```

频率只使用策划武器体系的 `weapons.AttackIntervalSeconds / AttackSpeed`。`attack_steps.DurationSeconds` 只表示行为/表现窗口，不得拒绝下一次 Commit；GAS 也不得再持有基础攻击第二道 cooldown。

### 命中载体

```text
Commit
  → Melee 几何查询，或 Projectile/Wave Logic 初始化
  → 逻辑位置推进与候选去重
  → 生成 HitIntent
  → ReEchoHitResolver::ResolveHit
  → Weapons 仅转发 Snapshot/Impact 结果给主模块表现
```

可见 Projectile/Wave Actor 不是飞行真相源；即使没有美术资源，逻辑载体也必须完成移动、命中和过期。

主模块可从已装备 Definition 的稳定 `VisualKey` 选择不同纹理或程序回退，但不得为此修改 Commit Carrier、Projectile Spec、碰撞半径、速度、范围或爆炸结算。当前 canonical Staff 仍是 `Pattern.StaffProjectile → Projectile`；复用 `StaffLightWave` 只表示视觉资源复用，不得切回旧 `Pattern.MoonStaffWave` 行为。

六武器的手持与首用攻击纹理由主模块 `FReEchoWeaponVisualCatalog` 集中解析和枚举，GameInstance 预加载器在菜单阶段异步预热。该机制不进入 `ReEchoWeapons` 逻辑模块；缺图或异步失败时，世界 Actor 继续使用原同步读取与程序/刀光回退，Commit 和命中不等待表现成功。

### 持有者瞄准适配

`AReEchoWeaponActor` 通过单一 `ResolveOwnerAimDirection` 把宿主状态编译为武器世界方向。玩家宿主读取 `AReEchoPlayerPawn::AttackAimDirection`，Echo 宿主读取 `AReEchoEchoActor::AttackAimDirection`，两者都无需旋转根 Actor；其他宿主才回退到 `Owner` 前向。攻击位移、Commit 事件、近战查询、Projectile、Wave 与 SwordArc 必须消费同一结果，禁止各自重新读取 Actor Rotation/Forward，否则会再次出现逻辑瞄准与碰撞/表现解耦后攻击方向固定的问题。

## 代码位置与阅读路线

| 目的 | 先读代码 | 说明 |
|---|---|---|
| Definition/Commit/Snapshot | `Public/Weapons/ReEchoWeaponTypes.h` | 模块公共值契约 |
| 唯一节拍与步骤事务 | `Public/Weapons/ReEchoWeaponLogic.h` → `Private/Weapons/ReEchoWeaponLogic.cpp` | readiness、游标、提交/回滚中心 |
| 近战/投射几何 | `ReEchoWeaponGeometry.*` | 无 Actor 资源依赖的空间算法 |
| 逻辑 Projectile/Wave | `ReEchoProjectileLogicComponent.*` | 飞行、范围、生命周期、去重和 HitIntent |
| 模块测试 | `Private/Tests/ReEchoWeaponLogicTests.cpp` | cadence、步骤、投射物、来源生命周期 |
| 数据编译适配 | `Source/ReEcho/Public/Weapons/ReEchoWeaponRuntime.h` → Private 实现 | CSV/Build → Logic Definition |
| 世界/表现宿主 | `Source/ReEcho/Public/Weapons/ReEchoWeaponActor.h` → Private 实现 | 组合 Logic、Actor、Sprite/Mesh/VFX；不拥有规则 |
| 表现资源目录/预热 | `Source/ReEcho/Public/Weapons/ReEchoWeaponVisualCatalog.h` → Private 实现 | 主模块资源适配；`ReEchoWeapons` 不依赖它 |
| 跨域回归 | `Source/ReEcho/Private/Tests/ReEchoWeaponRuntimeTests.cpp`、`ReEchoAttackModeTests.cpp` | 数据、构筑、Actor、GAS 与世界接缝 |

## 扩展方式

- 新武器：优先新增数据和现有 AttackPattern Definition；只在行为确实不同且可独立测试时新增执行器。
- 新投射物/光波形状：扩资源无关 Spec/Geometry/Logic；表现使用 Snapshot，不把碰撞回调变成权威命中。
- 新 OnHit/OnKill 效果：Weapons 在 Intent/Commit 中携带稳定 effect/behavior ID，Combat 在最终结果后执行合法规则；不要在 Actor 回调直接改目标。
- 新武器表现：只改主模块 WeaponActor/Presentation 适配，不修改 readiness 或逻辑位置。
- 新攻速来源：汇总进 `FReEchoStatBlock::AttackSpeed`，仍由同一 WeaponLogic 计算间隔。

## 验证与测试

- 纯规则：Definition 校验、单一 cadence、步骤顺序、AttackSpeed 缩放、Commit/rollback。
- 几何/载体：近战弧、弹数/散射、穿透/爆炸、命中去重、过期和 Snapshot。
- 接缝：held Ability 遇到临时未就绪不会退出；自动/手动均能连续至少两次成功 Commit。
- 生命周期：发射后 Source 销毁，延迟命中仍安全结算且不访问失效来源。
- 命令：`scripts/ue/Run-Automation.cmd -Filter ReEcho.Weapons`，并回归 AttackMode、Combat、数据/Run Snapshot。
- Plan47 接缝：`G_3_22` 可装备两个同类非 Core 配件，Core 仍保持原容量；装备重建、换武器和保存走同一有效槽上限。
- 商店接缝：Weapons 只公开有效槽容量和装备重建；配件出售资格、价格、所有权、草稿与存档仍由主模块 Data/Run/UI 负责，购买不得自动替换已提交装备。
- 用户 PIE：不同武器的连续攻击、攻速变化、无目标、模式切换和实际手感。

## 不变量与常见错误

- 普通攻击只有 WeaponLogic readiness 一道频率门；步骤 Duration、GAS cooldown 和动画结束都不能再阻塞 Commit。
- Weapons 不直接改 Combatant 生命/元素，也不决定 Kill/Death。
- Definition、Commit、Projectile Spec 必须无 Widget、Sound、Animation、Texture、Material 等资源引用。
- 逻辑 Projectile/Wave 拥有位置和命中集合；视觉 Actor 不能保存第二份飞行真相。
- Commit 失败必须回滚可见武器状态，但 CommitId 保持单调且不复用。
- Weapons 只能依赖 Combat，不能反向 include 主模块或表现层。
- Cards 槽位规则必须在主模块数据/装备适配层求值；不得把 CardState 或卡牌 ID 下沉进 Weapons 逻辑模块。
