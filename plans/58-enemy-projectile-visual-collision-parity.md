# Plan 58 - 程序 - 敌方子弹视觉与碰撞同源

## 协调

- Planner 负责人：Gavyn（JosephLE910）/ Codex。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（由用户在 PIE 验证可见子弹与受伤时机）。
- 本地规划 / 实现基线：规划发布提交 `914307c`；实现采用随后发布的 `origin/main@a6e08c3`，包含用户已决定的兔子单球伤害 `1`。
- 本地实现方式：规划先发布到 `origin/main`；随后从已发布基线创建独立 `plan/58-enemy-projectile-visual-collision-parity` worktree 实现。
- 依赖 / 阻塞：依赖 Plan53 的三条权威兔子逻辑投射物与 Plan49 的 Niagara 资产接入。现有本地 `plan/57-remove-enemy-healthbar` 不写本 Plan 契约；Plan55 已发布实现会写 EnemyHost，但当前 main 已包含其代码。
- Writes：本 Plan；`Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyEventsComponent.h`；`Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*`；`Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxComponent.*`；必要时修改 `ReEchoCombatVfxCatalog.*` 与兔子投射物项目适配资产；`Source/ReEcho/Private/Tests/ReEchoEnemyHostTests.cpp`、`ReEchoCombatVfxTests.cpp` 或新增聚焦测试；`shared/CODEBASE_MAP/{ARCHITECTURE.md,README.md,modules/MOD-ReEcho.md,modules/MOD-ReEchoEnemies.md,modules/MOD-ReEchoVFX.md}`；最终精选 Win64 Editor 预构建包。
- Stable Reads：`Source/ReEchoCombat/**` 命中结算契约；`Core/ReEchoRabbitProjectilePattern.h`；`Design/Data/ReEchoEnemyData.xlsx` 与 `Content/Data/enemy_abilities.csv`；`/Game/VFX/Monster/Rabbit/Particle/NS_Rabbit_Attack_02` 及其静态依赖；玩家碰撞与 `IReEchoCombatTarget::IntersectsCombatPath` 契约；Plan49/53/55/57 的非本任务功能。
- 影响模式：`SharedContract`（EnemyHost 向 Presentation 发布的投射物事件增加逐球身份；不改变 Combat 伤害权威或数据表数值）。
- 兼容承诺 / 下游操作：维持三球、中心球指向锁定目标、两侧 `±32.5°`、单球半径 `50 cm`、每球一次伤害 `1`、Echo 嘲讽目标、保存恢复和前景排序；旧存档缺失逐球表现身份时从既有 `Attack.Sequence + VolleyBallIndex` 确定性重建，不升级保存版本。
- 明确排除：不调整伤害、速度、散射角、半径、敌人 AI 或玩家碰撞体；不让 Niagara 粒子/碰撞成为玩法权威；不以扩大碰撞半径掩盖错位；不顺带实现 Plan57 敌人血条或修改其他攻击特效。

## 锁定目标

修复敌方兔子三球“可见球碰到玩家却不扣血、没有可见球碰到玩家却突然扣血”的错位。三颗可见子弹必须分别对应三条权威逻辑投射物，使用同一份逐球位置、方向和生命周期；任何伤害都能追溯到同一身份的可见球扫掠，且可见球不得继续使用与玩法轨迹独立推进的内部模拟位置。

伤害和碰撞仍完全由 EnemyHost/Combat 逻辑裁决，VFX 只投影逻辑快照。资源加载失败可以看不到特效，但不得改变命中；反过来，Niagara 粒子位置、粒子碰撞或播放完成回调不得产生伤害。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoEnemies` 的 Host→Presentation 类型化事件；`MOD-ReEchoVFX / AREA-Presentation` 的逐球视觉实例；`MOD-ReEchoCombat` 仅审阅，不改变命中/伤害权威。
- 对应模块文档：必须维护 `MOD-ReEchoEnemies.md` 和 `MOD-ReEchoVFX.md`；关闭前审阅 `MOD-ReEcho.md`、`ARCHITECTURE.md` 与 `README.md`。
- 设计意图：消除“玩法三球”和“表现三球”两套独立运动模拟。EnemyHost 拥有轨迹、碰撞和逐球生命周期，VFX 只按事件设置表现 Transform，从结构上阻止肉眼位置与命中位置再次漂移。
- 权威状态与依赖：
  - `FReEchoEnemyProjectileRuntimeState` 继续是位置、方向、碰撞半径、命中消费和有效性的唯一运行时状态；
  - `FReEchoEnemyProjectileEvent` 增加稳定 `VolleyBallIndex`，逐球键为 `AttackIdentity + VolleyBallIndex`，不能只用共享的 `Attack.Sequence`；
  - EnemyHost 为三球分别发布 Spawned/Moved/Ended；Presentation 不读取 Host 私有数组，也不回写玩法；
  - Combat 只接收 EnemyHost 已判定相交的 HitIntent，不依赖 VFX 是否存在。
- 决策记录：
  - 已确认根因是三条逻辑弹道只由中心球发布事件，而一套 Niagara System 自行模拟三颗粒子；两者没有逐球位置对应关系。
  - 不采用“读取 Niagara CPU 粒子位置来伤害玩家”，因为这会把帧率、渲染裁剪和资源改动变成玩法输入。
  - 不采用“继续一套三球 Niagara，只调旋转/半径”，因为内部粒子仍独立移动，无法证明逐球同源。
  - 表现适配必须做到每个逻辑 Ball 恰有一个可见代理。资产审计确认交付 System 只有独立内部粒子模拟、没有可安全复用的逐球位置参数；因此采用其正式依赖纹理 `0814_04` 创建三个 World Billboard。它保留交付红球外观并完全移除第二套运动模拟；未来增强尾迹必须制作单球、逻辑位置驱动的适配资产。
  - 发射挂点偏移只允许在 Spawn 时作为整组一致的视觉原点校准；逐球后续位置必须由对应逻辑位置推进，不能在 VFX 内再次积分速度。
  - 原有临时粒子读回/屏幕坐标诊断在完成验证后删除，长期保留的诊断只记录逐球身份、逻辑位置、命中结果和必要的限频错误。
- 相关文档同步范围：`MOD-ReEchoEnemies.md` 更新逐球事件契约；`MOD-ReEchoVFX.md` 更新逐球视觉所有权、资产适配与不变量；`MOD-ReEcho.md`、`ARCHITECTURE.md`、`README.md` 关闭前审阅，若模块拓扑与导航不变则记录无需修改。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEchoEnemies.md`：待实现后填写；
  - `MOD-ReEchoVFX.md`：待实现后填写；
  - `MOD-ReEcho.md`、`ARCHITECTURE.md`、`README.md`：待关闭前审阅。

## 锁定验收

- [x] 每次兔子齐射产生三个稳定且互不冲突的逐球身份；三球均收到 Spawned/Moved/Ended，表现恰好三球，不是一个共享实例，也不是九球。
- [x] 三颗可见球分别使用其对应逻辑球的位置和方向；自动化或运行时诊断证明表现锚点与逻辑位置误差不超过具名的资产中心偏移容差，且不会随飞行时间累计漂移。
- [x] 玩家静止时：可见球扫过玩家碰撞体会由同一球精确扣 `1`；没有任何可见球扫过时不扣血；每球至多命中一次。
- [ ] 玩家移动时使用连续线段扫掠，不因低帧率穿透；三条侧向轨迹都可独立命中，碰撞结果与画面轨迹一致。
- [x] 命中后逻辑球若按既有规则继续飞行，其可见代理保持同轨；射程结束、Encounter 清理、敌人销毁、保存恢复后没有残留或重复视觉。
- [ ] Echo 嘲讽目标、中心球锁定方向、两侧散射、伤害/半径/速度表值和战斗前景排序无回归。
- [x] 新增聚焦自动化覆盖逐球事件身份、三球生命周期、路径内/外、一次命中和 Presentation 键冲突；`ReEcho.Enemies.Host.RabbitProjectilePipeline`、`ReEcho.Presentation.VFX`、保存相关回归通过。
- [x] Editor Development 构建、`python scripts/validate_project.py`、`git diff --check` 通过；最终发布前在最新组合候选执行 `-FullRebuild` 并刷新精选预构建包。
- [ ] 用户在主项目 PIE 复测静止与移动两个场景，确认“碰到不伤 / 隔空受伤”均消失。
- [x] 未提交 `shared/GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：规划前 `main == origin/main == 8248732`；规划发布后远端依次加入 Plan57 和兔子伤害 `1` 提交，当前实现基线为 `origin/main@a6e08c3`，与本 Plan 源码无文本冲突，伤害语义按用户既有决定采用远端。
- 引擎/构建可用性：UE 5.8；实现构建前关闭交互式 Editor，并使用仓库构建脚本取得共享 Unreal 锁。
- 现有聚焦测试结果：Plan53 自动化已证明三条逻辑轨迹各自可命中，但测试只检查逻辑状态；远端随后把表值改为每球 `1`。Plan49 VFX 测试只证明共享三球 Niagara 中轴旋转，没有验证可见粒子与三条逻辑球位置一致。
- 共享契约 / 难合并资源风险：`ReEchoEnemyActor.*` 与 Plan55 历史热点同文件但当前实现已在基线；本地 Plan57 不应触碰本 Plan。若必须生成新的 Niagara `.uasset`，只能在本 Plan worktree 用 UE 创建并验证依赖，不得整包复制或选择二进制 ours/theirs。
- 基线损坏时的停止条件：生产兔子定义无法加载、现有三球逻辑测试失败、Niagara 资产无法安全拆成一球且没有明确参数驱动方案，或远端在发布/合并前修改相同公共事件契约时，停止扩大修改并报告真实/逻辑冲突。

## 实现提纲

1. 为投射物表现事件加入 `VolleyBallIndex`，定义稳定逐球键并增加纯值测试，拒绝仅以 `Attack.Sequence` 覆盖同一次齐射的三个实例。
2. EnemyHost 去掉“只发布中心球”的特殊分支，为每个逻辑球发布完整生命周期；保存恢复、清场和销毁路径同样逐球结束/重建。
3. 审计 `NS_Rabbit_Attack_02` 的发射器和参数，制作最小项目适配：每条逻辑轨迹只显示一个球，或让一套 System 的三个可见球被三个显式逻辑位置驱动；不复制无关依赖。
4. VFX 组件改为逐球实例/句柄映射，每次 Moved 直接覆盖对应可见球 Transform；保留攻击挂点原点校准、全局前景排序和资源缺失降级。
5. 扩展 EnemyHost 与 VFX 自动化，验证三个键、三条轨迹、命中/未命中、低帧率连续扫掠、清理和恢复；加入能检测共享 Sequence 覆盖与九球回归的断言。
6. 删除临时粒子读回诊断，更新 Plan 与 `CODEBASE_MAP`，执行格式化、静态校验、聚焦自动化和 Editor Development 构建后交用户 PIE。
7. 人工通过后，在最新 `origin/main` 审计远端差异；无耦合或按用户决定完成组合后执行 FullRebuild、提交精选包、推送 main 并清理 worktree。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py`；`git diff --check` | Plan、源码、路径和生成物规则通过 |
| 逐球契约 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Enemies.Host.RabbitProjectilePipeline` | 三球事件身份、方向、位置、路径内外与一次伤害通过 |
| VFX 适配 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.VFX` | 三个唯一视觉代理、资产加载、Transform 同源与清理通过 |
| 保存/生命周期 | 聚焦运行 `ReEcho.Run.SaveSnapshot`、Encounter/Enemy Host 清理测试 | 恢复重建和边界清理无重复/残留 |
| C++ 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 成功并刷新跟踪包 |
| 人工 PIE | 用户静止/移动穿过中心和侧球路径 | 可见相交与扣血一一对应，无隔空伤害 |
| 发布 | 最新候选 `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`；最终 fetch 审计 | 源码与精选 Editor 包一致，远端耦合已处理 |

## 执行记录

### 变化

- `FReEchoEnemyProjectileEvent` 增加 `VolleyBallIndex` 与只读 `CollisionRadiusCm`；EnemyHost 为三个球分别发布 Spawned/Moved/Ended，保存恢复和局间清理同样逐球处理。
- VFX 用 `(AttackIdentity, VolleyBallIndex)` 管理三个独立 World Billboard；每次 Moved 直接采用逻辑事件位置，不再移动会自行模拟三球的 Niagara System，也不保留攻击挂点平移偏差。
- 使用交付资产依赖 `0814_04` 作为红球视觉，按 `2 * CollisionRadius` 映射可见直径；碰撞仍只由 EnemyHost 连续扫掠裁决。
- 删除 `[RabbitAimTrace]` / `[RabbitParticleTrace]` CPU 粒子读回诊断和相应 Niagara 内部依赖。

### 证据

- 用户截图显示两类互相矛盾的现象：可见红球覆盖主角但未扣血；主角附近没有对应可见红球时突然出现 `-10`。
- 只读代码审计确认：玩法保存并推进三条 `BossProjectiles`，但 `ShouldPublishProjectileEvent` 只允许中心球发布事件；VFX 仅以共享 `Attack.Sequence` 创建一套自行模拟三球的 Niagara，因此表现位置与两条侧向逻辑轨迹没有一一映射。
- Editor Development 构建成功；`ReEcho.Enemies.Host.RabbitProjectilePipeline`、`ReEcho.Presentation.VFX.Catalog`、`ReEcho.Run.SaveSnapshot` 自动化通过。

### 剩余风险

- 逻辑、组件数量和 World Transform 已自动验证；Billboard 的最终屏幕尺寸、透明边缘、与人物遮挡关系以及移动玩家实战中的视觉一致性仍需用户 PIE。

### 人工验收结果/请求

- `PendingBeforeClose`：实现和构建后由用户在 PIE 验收。

### 架构文档审阅结果

- `MOD-ReEchoEnemies.md` 已更新：逐球事件、稳定索引、半径上下文和当前单球伤害 `1`。
- `MOD-ReEchoVFX.md` 已更新：逐球 Billboard、纹理路径、身份键、生命周期和禁止恢复独立 Niagara 运动链。
- `MOD-ReEcho.md` 已更新：主模块表现适配改为 Niagara/纹理目录，并写明三球逐球投影。
- `ARCHITECTURE.md` 已更新：全局状态流以战斗 VFX 实例概括 Niagara 与 Billboard，依赖方向不变。
- `README.md` 已更新：VFX 文档入口的资产范围同步为 Niagara/纹理；模块和 AREA 导航不变。
