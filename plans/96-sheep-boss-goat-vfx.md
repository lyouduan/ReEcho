# Plan 96 - 程序 - 羊 Boss 技能 Niagara 装配

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Codex planner-side AI`。
- 实现编写方（AI 侧）：`Codex executor-side AI`。
- 任务状态：`InProgress`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划基线：本地 `main@7249045e`，由 `origin/main@76366797` 与本地狐狸 VFX 候选合并而成；本 Plan 发布后从最新 `origin/main` 创建专属 worktree。
- 本地实现方式：一任务一 worktree；不得在当前含大量美术导入内容的主工作区直接实现。
- 依赖 / 阻塞：依赖 `M_SHEEP` 的 `FReEchoBossIntent`（TelegraphStarted / AttackWindowStarted / AbilityEnded）、敌方逻辑投射物事件、Combat 最终 Hurt、现有 `UReEchoCombatVfxComponent` 和 Boss `AttackVfxRoot/HurtVfxRoot`；依赖用户已修改的 `DA_Enemy_TimeGuard`。
- Writes:
  - `plans/96-sheep-boss-goat-vfx.md`
  - `Content/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_TimeGuard.uasset`（保留并带入当前主工作区用户修改，不得以旧版本或自动生成结果覆盖）
  - `Content/VFX/Monster/Goat/Particle/NS_Goat_Skill02_Charging.uasset`
  - `Content/VFX/Monster/Goat/Particle/NS_Goat_Skill02_Bullet.uasset`
  - `Content/VFX/Monster/Goat/Particle/NS_Goat_Skill02_BeAttacked.uasset`
  - `Content/VFX/Monster/Goat/Particle/NS_Goat_Skill03_Charging.uasset`
  - `Content/VFX/Monster/Goat/Particle/NS_Goat_Skill03_Alarming.uasset`
  - `Content/VFX/Monster/Goat/Particle/NS_Goat_Skill03_BeAttacked.uasset`
  - `Content/VFX/Monster/Goat/Particle/NS_Goat_Skill04_Charging.uasset`
  - `Content/VFX/Monster/Goat/Particle/NS_Goat_Skill04_Lighting.uasset`
  - 上述 Niagara 根实际引用的 `Content/VFX/Monster/Goat/{MI,Mesh,Tex}/**`、公共 `Content/{00_Textures,01_Textures,Mat,VFX}/**` 中经 Unreal Asset Registry 审计确认的准确递归依赖；禁止目录级整体纳入
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxCatalog.*`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxComponent.*`
  - `Source/ReEcho/{Public,Private}/Presentation/Enemy/ReEchoEnemyPresentationComponent.*`（Boss `MoonStaff` 持有与技能阶段挥舞的最窄表现接线）
  - `Source/ReEcho/{Public,Private}/Presentation/Weapon/ReEchoWeaponPresentationCatalog.*`（优先 Stable Read；仅当现有 `MoonStaff` 公共表现契约不足时修改）
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*`（仅当现有公开敌方投射物事件不足以定位 Boss 投射物表现时）
  - `Source/ReEcho/{Public,Private}/ReEchoGameMode.*`（仅增加 Development GM 指令入口，不进入 Shipping 玩法）
  - `Source/ReEchoEnemies/{Public,Private}/Enemies/ReEchoEnemyLogicComponent.*`（仅增加排队指定 Boss Ability 的调试命令；仍由正常固定步状态机提交）
  - `Source/ReEchoEnemies/Private/Tests/ReEchoEnemyLogicTests.cpp`
  - `Source/ReEchoCombat/{Public,Private}/Combat/ReEchoCombatantComponent.*`（仅调整非 Shipping 的 GMGod 调试伤害回报；正式无敌不变）
  - `Source/ReEchoCombat/Private/Tests/ReEchoCombatRuntimeTests.cpp`
  - `docs/GM_COMMANDS.md`
  - `Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoRuntimeAssetPreloadTests.cpp`
  - `Source/ReEcho/Private/Tests/**` 中本 Plan 新增的 Boss VFX 生命周期聚焦测试
  - `scripts/ue/` 下本 Plan 必需的可复现 Niagara 检查/修复脚本（仅通过 Unreal Editor API 修改资产）
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`（审阅；仅当事件消费事实变化时修改正文）
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`（审阅；仅当公共表现契约变化时修改正文）
  - `Binaries/Win64/` 下 `GIT_RULES.md` 允许的最终精选预构建包
- Stable Reads:
  - `Design/Data/ReEchoEnemyData.xlsx`、`Content/Data/{enemies,enemy_abilities,boss_phases}.csv`
  - `Content/ReEcho/DataAsset/Weapon/Catalogs/DA_WeaponPresentationCatalog.uasset`
  - `Content/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_MoonStaff.uasset`（禁止复制为 Boss 专用 Profile）
  - `Content/ReEcho/Textures/Effects/MoonStaff.uasset`
  - `plans/44-boss-gameplay-and-enemy-xlsx-tables.md`
  - `plans/68-reecho-enemy-align-authoritative.md`
  - `Source/ReEchoEnemies/{Public,Private}/Enemies/ReEchoEnemy{Types,LogicComponent,EventsComponent}.*`
  - `Source/ReEcho/{Public,Private}/Presentation/Enemy/ReEchoEnemyPresentationComponent.*`
  - `Source/ReEcho/{Public,Private}/Presentation/Combat/ReEchoCombatPresentation*`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
- 影响模式：`SharedContract`（新增 Boss 行为事件到 Niagara 的只读投影、投射物视觉实例和最终命中特效；不改变 Boss AI、Combat 命中或阶段权威。平衡数值由权威配表拥有，并可由后续已批准数值 Plan 独立调整）。
- 兼容承诺 / 下游操作：`M_SHEEP`、`Boss.TimeGuard`、`Enemy.TimeGuard`、技能顺序、锁点/锁向、二阶段 `1.5` 倍率和保存语义保持不变；伤害、前后摇、范围与冷却继续消费 `enemy_abilities.csv`，Plan109 已在不改变本 Plan 表现语义的前提下更新当前配表；Boss 复用 Plan104 当前 `MoonStaff` Profile/贴图契约，不恢复已删除的 `Staff` Profile；VFX 加载失败只能缺视觉，不得阻塞或延迟技能提交与伤害。
- 明确排除：Plan96 不独立拥有或复制 Boss 平衡常量；不启用当前未配置的 `Boss.ElementCleanse`；不新增 `NS_Goat_Skill04_Alarming`，Skill04 的两次预警均复用现有 `NS_Goat_Skill03_Alarming`；不让 Niagara 粒子、碰撞、完成回调或固定延迟决定命中、投射物位置或技能结束；不整体提交 Goat/公共素材目录；不在 Unreal Editor 外修改 `.uasset`。

## 锁定目标

1. Boss 始终使用 Plan104 当前 `MoonStaff` 契约显示法杖贴图；`M_SHEEP_MeleeSweep` 按既有 Skill01 近战机制挥舞法杖，并在挥舞阶段复用 `NS_Goat_Skill02_BeAttacked` 作为挥击表现。该复用不表示 Boss 自身受击，也不改变 Skill01 圆形命中与伤害。
2. `M_SHEEP_StationaryVolley` 与 `M_SHEEP_MovingSpread` 共用 Skill02 表现：`NS_Goat_Skill02_Charging` 挂在法杖上蓄力；提交后分别按权威能力配置生成站定四连弹或移动三向散射，每个逻辑弹丸生成独立 `NS_Goat_Skill02_Bullet` 并逐帧跟随 `(AttackIdentity, VolleyBallIndex)` 的权威位置；只有实际造成伤害时在受击角色挂一次 `NS_Goat_Skill02_BeAttacked`。
3. `M_SHEEP_BlinkSlam` 使用 Skill03：Boss 播放 `NS_Goat_Skill03_Charging`，同时在已锁定角色当时的世界落点播放 `NS_Goat_Skill03_Alarming`；提交/瞬移时在该落点播放 `NS_Goat_Skill03_BeAttacked`，并清理蓄力和预警。伤害仍只来自既有锁定落点圆形命中。
4. `M_SHEEP_PrayerBeam` 使用 Skill04，并保留配表 `LockTiming=WindupEnd`：Boss 开始播放 `NS_Goat_Skill04_Charging` 时，第一道 `NS_Goat_Skill03_Alarming` 取角色当时位置的只读视觉快照；蓄力结束后玩法才正式锁定位置/方向，第二道 Alarming 重启到权威 `LockedTargetLocation`，同时以权威 `LockedDirection` 播放 `NS_Goat_Skill04_Lighting`。两次预警均不追踪角色、不新增落点伤害，第一道快照不参与玩法锁定；光束长度、起点和生命周期继续来自 Boss Intent；AbilityEnded、Death、EndPlay、遭遇清理时停止。
5. 保留当前主工作区用户修改后的 `DA_Enemy_TimeGuard` 精确内容并纳入 Plan 96 候选；继续由 `Enemy.TimeGuard -> DA_Enemy_TimeGuard` 解析，不覆盖其中已有动画、尺寸、锚点或二阶段配置。
6. 八个正式 Niagara 根及准确递归依赖进入 Git、预加载和 Shipping cook；实例层级、Local/World Space、朝向轴、Bounds 与 AutoDestroy 由 Editor 审计后按资源真实语义配置。
7. Boss 技能真实命中角色时必须继续经 `FReEchoHitIntent -> ReEchoCombat` 造成权威 `enemy_abilities.csv` 配表伤害，不能出现“只播放 BeAttacked/Lighting、角色不掉血”。Plan109 当前一阶段 Skill02 每弹 4、Skill03 24、Skill04 16；二阶段由现有 `1.5` 倍率统一变为 6/36/24。后续平衡调整只改权威配表与对应数值 Plan，不改本 Plan 的 VFX/命中语义。Niagara 碰撞和粒子范围不参与伤害裁决。
8. 新增 `GMBossSkill <Skill01|Skill02|Skill02Moving|Skill03|Skill04>`：寻找当前存活 `M_SHEEP`，将对应正式 Ability 排到下一次可开始的 Boss 行动；`Skill02` 固定指向站定四连弹，`Skill02Moving` 指向移动三向散射。指令不取消当前已提交技能、不直接生成伤害/VFX，并继续走 Telegraph、锁点、AttackWindow、Recovery 与 AbilityEnded。
9. `GMGod` 开启时继续返回已计算伤害并驱动 Hurt/伤害数字，但不修改玩家生命、不触发死亡；正式限时无敌仍返回零伤害，不改变生产规则。

## 架构影响与设计决策

- 受影响架构标识：文档型 `MOD-ReEchoVFX / AREA-Presentation` 为主要实现边界；`MOD-ReEchoEnemies / AREA-Enemies` 只提供既有行为与投射物事件；`MOD-ReEchoPresentation` 继续提供角色动画和渲染根；`MOD-ReEchoCombat` 继续拥有最终 Hurt。
- 对应模块文档：维护 `MOD-ReEchoVFX.md`；审阅 `MOD-ReEchoEnemies.md`、`MOD-ReEchoPresentation.md`、`MOD-ReEcho.md` 和 `MOD-ReEchoCombat.md`，仅在事实变化时更新正文。
- 设计意图：Boss Logic 发布资源中立事件，集中 VFX Catalog 选择 Goat Niagara，VFX Component 以 AttackIdentity 管理前摇、锁点、持续光束、逻辑弹丸和命中实例。动画 DA 与 Niagara 是同一事件的并列消费者，互不作为彼此的时钟。
- 权威状态与依赖：EnemyLogic 拥有技能阶段、锁点和锁向；EnemyHost/ProjectileLogic 拥有投射物位置；Combat Hurt 拥有是否实际命中和最终伤害；VFX 仅拥有可丢弃组件实例。不得在 VFX 中积分弹道或推测命中。
- 决策记录：
  1. Skill02 两个技能按 `AbilityId`/`BehaviorId` 映射同一组资源，弹丸数量严格来自逻辑事件，不从 Niagara 发射数量反推。
  2. Skill03 `Alarming` 使用 Telegraph 已锁定的世界落点，不附着目标继续追踪；提交时即清理，确保预警与可躲逻辑一致。
  3. Skill04 `Lighting` 使用锁定方向，并以 AbilityEnded 为正常清理权威；资产播放时长不延长伤害窗口。
  4. `BeAttacked` 只消费 `AppliedDamage > 0` 的 Combat 最终事实。VFX Component 以 AttackIdentity 关联此前 Boss Ability，避免按“当前正在播放技能”产生并发/延迟误判。
  5. `DA_Enemy_TimeGuard` 是用户已编辑二进制资产：复制前后记录 SHA-256；目标存在不同内容时停止，不做二进制合并，不运行会重建或覆盖该 DA 的 authoring 脚本。
  6. 八个 Niagara 当前位于主工作区未跟踪 Goat 素材中。Executor 先用 Unreal Asset Registry 取得根与递归依赖闭包，再按精确清单复制；`01.uasset` 默认排除，除非用户后续明确指定其语义。
- 相关文档同步范围：关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoEnemies.md`、`MOD-ReEchoPresentation.md`、`MOD-ReEchoCombat.md`；维护 `MOD-ReEchoVFX.md`。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅。
  - `README.md`：待审阅。
  - `MOD-ReEchoVFX.md`：待维护。
  - `MOD-ReEchoEnemies.md`：待审阅/按事实维护。
  - `MOD-ReEchoPresentation.md`：待审阅/按事实维护。
  - `MOD-ReEcho.md`：待审阅。
  - `MOD-ReEchoCombat.md`：待审阅。

## 锁定验收

- [ ] Boss 在一/二阶段均显示 `MoonStaff`，Skill01 挥舞法杖并复用一次 Skill02 BeAttacked 挥击表现；法杖挂点、朝向、缩放和翻面跟随 Boss，且不改变近战判定。
- [ ] Skill02 两种投射技能均在法杖上按真实前摇播放 Charging；4 连弹/3 向散射分别生成 4/3 个 Bullet，位置与逻辑弹道一致，命中或结束后无残留；只有真实伤害在受击角色生成 BeAttacked。
- [ ] Skill03 Charging 与锁定落点 Alarming 同时出现，预警不追踪玩家；提交时清理，真实命中生成一次 BeAttacked。
- [ ] Skill04 Charging 开始时在角色当时位置的只读视觉快照播放第一次 Skill03 Alarming；`WindupEnd` 权威锁定后，在 `LockedTargetLocation` 重启第二次 Alarming并从 Boss 朝 `LockedDirection` 播放 Lighting。第一道快照不改变锁定/伤害，两次预警均不追踪角色、不产生额外伤害，Lighting 在 AbilityEnded/Death/清场时可靠停止。
- [ ] Skill02 每个实际命中的逻辑弹丸、Skill03 下砸范围命中、Skill04 光束范围命中均通过 Combat 对角色扣除准确配表生命；未命中、已躲开或无敌时不扣血。命中特效与最终 `AppliedDamage > 0` 一致，不存在有特效无伤害或无命中却扣血。
- [ ] Skill01 只按已确认契约复用 Skill02 BeAttacked 作为挥击表现，不触发 Skill02 Charging/Bullet/命中语义；二阶段继续复用相同技能映射并由现有数值倍率驱动，不重复或漏播阶段事件。
- [ ] `DA_Enemy_TimeGuard` 与规划时用户修改版本字节一致，并继续被 `Enemy.TimeGuard` Catalog 正确解析；现有动画/锚点/缩放/阶段内容不丢失。
- [ ] 八个 Niagara 通过 Editor 加载与编译检查，Simulation Space、朝向轴、Bounds、Renderer 排序、AutoDestroy/持续生命周期符合对应挂载方式；缺失资产不影响玩法。
- [ ] 八个根及准确依赖被 Git 跟踪、进入预加载和最新 Shipping IoStore；未夹带 `01.uasset`、zip、源图或无关 Goat/公共资产。
- [ ] 修改的 C++ 完成 `.clang-format`；聚焦自动化、Development `-FullRebuild`、`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过。
- [ ] 用户在 PIE 验收一阶段/二阶段、两种 Skill02、Skill03、Skill04、命中/未命中、死亡/重开后的方向、中心、大小、层级和生命周期后，人工验收才可设为 `Passed`。

## Step 0 门禁

- 基线分支/提交：Plan-only 发布基线为合并后的本地 `main@7249045e`；实现必须从包含 Plan 96 的最新 `origin/main` 创建专属 `codex/plan96-sheep-boss-goat-vfx` worktree。
- 引擎/构建可用性：UE 5.8 安装版；执行 Asset Registry、Editor 修改、自动化或最终构建前请用户保存并关闭 Editor，并取得 Git common-dir Unreal 锁。
- 用户资产保护：主工作区 `DA_Enemy_TimeGuard.uasset` 已修改，Goat 资源大多未跟踪。复制前记录源状态与哈希，只复制具名根和审计依赖；不清理、不覆盖、不移动主工作区原件。
- 共享契约 / 难合并资源风险：集中 VFX Catalog/Component 与精选预构建包刚包含狐狸 VFX 本地提交；Plan95 可能并行修改 `ReEchoGameMode` 和预构建包，但不应修改 Boss/VFX 事件。实现发布前必须重新 fetch 并审计实际重叠。
- 基线损坏时的停止条件：Niagara 根无法加载/编译、资源命名与真实视觉语义不符、逻辑事件缺少稳定 AttackIdentity/VolleyBallIndex、DA 与源副本发生额外变化、依赖闭包无法隔离，或实现需要修改 Boss 技能数值/Combat 命中契约时停止扩张并回报。

## 实现提纲

1. 发布 Plan 后创建独立 worktree；记录用户 DA 和八个 Niagara 根的哈希，使用 Unreal Asset Registry 审计依赖、编译、Simulation Space、Bounds、Renderer、朝向轴和循环/AutoDestroy。
2. 精确复制 `DA_Enemy_TimeGuard`、八个根及批准依赖到任务 worktree；验证 DA Catalog 解析与资产完整加载，不运行会覆盖用户 DA 的生成脚本。
3. 扩展集中 VFX 语义与预加载集合；按 Boss Ability/Intent 映射 Charging、Alarming、Lighting，并用 AttackIdentity 管理实例创建、旋转、替换和清理。
4. 复用敌方投射物 Spawned/Moved/Ended，为 Skill02 每球创建和更新 Bullet；仅在现有事件缺字段时对 Host 做最窄补充，不修改 ProjectileLogic 裁决。
5. 回归并补齐 Boss Host 的既有伤害接线：Skill02 每球连续碰撞、Skill03 锁定落点范围、Skill04 锁定方向矩形均只生成 `FReEchoHitIntent` 并由 Combat 结算；用一/二阶段自动化锁定准确扣血。不得以 VFX 可见范围补偿或替代逻辑范围。
6. 在 Combat 最终 Hurt 上按 AttackIdentity 解析 Skill02/03 命中语义，只有 `AppliedDamage > 0` 播放 BeAttacked；未命中和无敌不播放。
7. 增加 Catalog、预加载、Boss 状态顺序、投射物数量/移动、锁点/锁向、伤害与命中门控、Death/EndPlay 清理和缺资产降级测试；维护模块文档与 Plan 执行记录。
8. 完成格式化、聚焦自动化、FullRebuild、静态/预构建检查和 Shipping 收录审计；Planner 评审准确资产清单，用户 PIE 验收后再关闭和发布实现。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 资产审计 | Unreal Editor/Asset Registry 检查八个根与递归依赖 | 可加载、无 Niagara 编译错误；空间、方向、Bounds、Renderer 和生命周期明确 |
| DA 保护 | 源/候选 SHA-256、Catalog 加载自动化 | 用户修改字节被原样带入，`Enemy.TimeGuard` 仍解析该 DA |
| VFX 聚焦 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Presentation.VFX` | 三组技能阶段、投射物、命中与清理映射通过 |
| Boss/投射物伤害回归 | `ReEcho.Enemies.Boss.*`、Enemy Host/Combat projectile and area-hit focused tests | Skill02/03/04 对命中角色造成准确一/二阶段伤害；未命中/无敌不扣血；行为、锁点、数量和弹道不被表现改变 |
| 预加载 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Presentation.RuntimeAssetPreload` | 八个正式根进入完整去重的预加载集合 |
| 构建 | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新精选预构建包 |
| 静态 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 项目、预构建与文本门禁通过 |
| Shipping | 检查最新 Windows Shipping IoStore | 八个根与准确依赖实际收录 |
| 人工 PIE | 一/二阶段分别验证 Skill02/03/04、命中/未命中、死亡/重开 | 用户确认方向、中心、大小、层级、节拍和生命周期符合预期 |

## 执行记录

### 变化

- 2026-08-24：用户要求按当前羊 Boss 设定装配 `/Game/VFX/Monster/Goat/Particle` 特效，并明确当前已修改的 `DA_Enemy_TimeGuard` 必须纳入 Plan 96。
- 2026-08-24：规划盘点识别八个具名正式根；锁定 Skill02=投射技能、Skill03=闪身下砸、Skill04=祷告光束，Skill01 因无具名资源保持不变。
- 2026-08-24：用户明确 Boss 技能击中角色时必须造成相应伤害；Plan 将配表扣血与最终命中特效一致性列为硬验收，同时保持 Combat 而非 Niagara 为唯一伤害权威。
- 2026-08-24：实现 Goat Skill02/03/04 的集中语义、Boss Intent 生命周期、逐球逻辑投射物 Niagara、锁点预警、锁向光束和按 AttackIdentity 对齐的最终命中特效；Boss 投射物事件补充稳定的 `M_SHEEP_Projectile` 表现标识。
- 2026-08-24：代码审计发现二阶段 `PhysicalAttackMultiplier/AttackSpeedMultiplier` 仅被编译但未用于 Boss 技能提交；补为 Phase2 提交伤害乘物理倍率、冷却除攻速倍率。一阶段继续直接采用能力表值，空间判定和最终扣血仍由 Host/Combat 权威链执行。
- 2026-08-25：Planner 评审退回 `InProgress`：Skill02/03 命中特效错误附着通用 Hurt 根、Skill04 未消费世界长度、附着特效缺少资源级空间契约。本轮先把命中改为 Combat 最终世界坐标，并让 Skill04 在激活前写入世界起止点、长度和宽度；通用语义挂点迁移继续作为关闭前架构修正项。
- 2026-08-25：二阶段运行时审计确认致命伤拦截先进入 `Transforming`，但同一命中的后续 Hurt 又通过 `NotifyHurt` 将 Phase 覆盖成 `HitReaction`，导致转换计时永不完成且 `bPhase2Triggered` 阻止重试。修复锁定 Transforming、转换期间统一免伤，并为缺少 `Transform.Phase2` Clip 的 DA 延迟到完成事件再切换 Phase2 基础动画。
- 2026-08-25：用户补充最终表现映射：Boss 使用 Plan104 新 `MoonStaff` 契约；Skill01 挥杖复用 Skill02 BeAttacked；Skill02 Charging 改挂法杖；Skill03 保持蓄力、锁点预警和落点下砸；Skill04 蓄力与锁定点爆发两次均复用 Skill03 Alarming，锁定点取角色被锁定时的 `LockedTargetLocation`，不追踪且不新增伤害，随后播放 Lighting。
- 2026-08-25：静态审计发现 Boss Skill02 创建逻辑投射物后未发布 `Spawned`，而 VFX 只在该事件创建 Bullet；同时命中清理把所有带 `VolleyBallIndex` 的弹误作 Rabbit 持续弹，羊弹可能命中后继续到最大射程。两项均纳入本轮修复与回归。
- 2026-08-25：用户确认 Skill04 按配表处理，不把 `WindupEnd` 改为 `WindupStart`。第一道 Alarming 因此只是蓄力开始时目标当前位置的视觉快照；第二道才消费蓄力结束后的权威锁点/锁向并与 Lighting 同步。
- 2026-08-25：用户要求增加 GM 指令让 Boss 使用对应技能。契约锁定 `GMBossSkill` 只排队真实 Ability，不绕过正式 Boss 状态机；Skill02 的两个配表 Ability 用 `Skill02`（StationaryVolley）和 `Skill02Moving`（MovingSpread）显式区分。
- 2026-08-25：用户要求修改 `GMGod` 为显示伤害但不扣血。契约锁定只改变非 Shipping 的 debug gate：返回已计算伤害供 Hurt/伤害数字消费，跳过生命与死亡写入；正式限时无敌保持零伤害。
- 2026-08-25：Plan109 经当前程序用户批准更新 Boss 数值；本 Plan 的伤害验收改为读取权威 `enemy_abilities.csv`，当前 Skill02/03/04 为 `4/24/16`、二阶段为 `6/36/24`。`WindupEnd`、`GMBossSkill`、Combat 命中和全部 VFX 映射保持不变。

### 证据

- 当前生产数据为 `M_SHEEP`，但沿用 `BehaviorProfileId=Boss.TimeGuard`、`PresentationId=Enemy.TimeGuard`；自动化直接加载 `/Game/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_TimeGuard`。
- 既有事件已提供 Boss Telegraph/AttackWindow/AbilityEnded、锁点/锁向/AttackIdentity，以及敌方投射物 Spawned/Moved/Ended；Combat Hurt 提供最终 AppliedDamage，具备表现只读接入基础。
- 主工作区确认 `DA_Enemy_TimeGuard.uasset` 已修改，Goat 八个具名 Niagara 根和大部分依赖尚未跟踪；因此资产复制与精确依赖审计是 Step 0 硬门禁。
- 用户 DA 与八个根复制后 SHA-256 均与主工作区来源一致；包内 `/Game/` 引用递归审计只复制缺失或哈希不同的 50 个依赖文件，未纳入 `01.uasset`。
- 第一次 FullRebuild 在 98 个动作的 `ReEchoCombatVfxComponent.cpp` 编译处发现 C4456 局部变量遮蔽；只重命名第二个局部变量后，第二次 FullRebuild 98/98 成功，Build ID `55116800`，当时源码指纹 `df2b0548348a`。随后补充 Phase2-only 防护，最终候选需再次 FullRebuild。
- 最终候选 FullRebuild 98/98 成功并刷新七模块精选预构建包，Build ID `55116800`、source fingerprint `7459ba735e47`；`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过。
- `Run-Automation.cmd -Filter ReEcho.Enemies.Logic` 在测试发现前被 LinuxArm64/VisionOS SDK `MainVersion` 校验阻断；命令虽返回 0，但没有任何测试执行证据，因此明确记为未运行，不声称通过。VFX/资产 Editor 审计受同一环境门阻塞。
- 2026-08-25 组合适配 `origin/main@07b7b3ed` 后，最终源码完成 FullRebuild 96/96，Build ID `55116800`、source fingerprint `91558a68beac`；Plan95 与 Plan96 重叠的精选预构建包由该最终组合源码统一刷新。
- `Run-Automation.cmd -Filter ReEcho.Presentation.VFX` 再次在测试发现前被 LinuxArm64/VisionOS SDK `MainVersion` 阻断并返回 1；新增纯函数测试已通过 UHT/UBT 编译，但没有运行时自动化通过证据。
- 二阶段状态修正后的最终 FullRebuild 96/96 通过，Build ID `55116800`、source fingerprint `d60209bd33bc`；`ReEcho.Enemies.Logic.Phase2` 聚焦自动化仍在测试发现前被同一 LinuxArm64/VisionOS `MainVersion` 校验阻断，命令返回 0 但没有执行测试，不作为通过证据。

### 剩余风险

- 资源名称给出预期语义，但实际 Niagara 朝向、空间、循环、中心、大小和 Renderer 层级必须由 Editor/PIE 验证，不能仅凭文件名确认。
- Plan95 与本 Plan 可能并行刷新相同预构建包；实现进入 main 前必须以最新远端候选重建，不能语义合并二进制。
- UnrealEditor-Cmd 和 GUI NoCompile 均在执行 Python 前被本机 LinuxArm64/VisionOS SDK `MainVersion` 校验阻断，Niagara Editor 加载/编译、Simulation Space、Bounds 和 Renderer 尚未验证；当前依赖证据来自保守包内引用闭包，不冒充 Editor 审计通过。
- `NS_Goat_Skill04_Lighting` 的二进制字符串审计没有发现可确认的 `User.StartPosition/EndPosition` 契约。运行时已经在激活前写入世界起止点、长度和宽度，但必须在 Editor 中确认或补接同名 Niagara User 参数，视觉长度才可验收。

### 人工验收结果/请求

- `PendingBeforeClose`：实现完成后由用户在 PIE 验收三组技能及一/二阶段整体表现。

### 架构文档审阅结果

- 待最终候选逐项填写。
