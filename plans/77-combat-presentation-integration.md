# Plan 77 - 程序 - 角色动画、武器与战斗特效统一表现生命周期

## 协调

- Planner / Executor：当前对话同一程序 AI；用户已确认不采用规划者-执行者拆分。
- 工作区：一任务一 worktree，`codex/combat-presentation-integration`。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`。
- 规划基线：`origin/main@b7c4212f033e1fadd044ddda9167dd55f4b6201b`。
- 前置结果：Plan74 已提供 Attack.Charge / Attack.Basic / Transform.Phase2 语义动画；Plan76 已完成长剑、镰刀、弓、枪的一武器一特效路由。本 Plan 只整合触发与生命周期，不复制 Niagara 资产映射。
- Writes:
  - `plans/77-combat-presentation-integration.md`
  - `Source/ReEcho/{Public,Private}/Presentation/Combat/ReEchoCombatPresentationCoordinator.*`
  - `Source/ReEcho/{Public,Private}/Presentation/Combat/ReEchoCombatPresentationTypes.*`
  - `Source/ReEcho/{Public,Private}/Presentation/Enemy/ReEchoEnemyPresentationComponent.*`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxComponent.*`
  - `Source/ReEcho/{Public,Private}/Weapons/ReEchoWeaponVisualCatalog.*`
  - `Source/ReEcho/{Public,Private}/Weapons/ReEchoWeaponActor.*`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*`
  - `Source/ReEcho/Private/Tests/ReEchoCombatPresentationTests.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`
  - `Binaries/Win64/` 下 `shared/GIT_RULES.md` 允许的最终预构建包文件
- Stable Reads:
  - `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyEventsComponent.h`
  - `Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`
  - `Source/ReEchoPresentation/{Public,Private}/Presentation/Animation2D/**`
  - Plan74 生产 Profile 与 Plan76 武器 Niagara 资产/目录
- 影响模式：`SharedContract`。新增主模块内部的资源中立表现动作契约；不改变 Combat、Enemies、Weapons 的玩法权威或依赖方向。
- 明确排除：不修改攻击节拍、伤害、碰撞、AI、Niagara 内部参数；不新增角色×武器×技能组合表；不为普通怪物生成武器；不在缺少正式配置时猜测 Boss 武器；不在本轮制作逐帧手部挂点数据、音频或镜头震动。

## 锁定目标

1. 将一次攻击表现归一为由 `AttackIdentity / AbilityId` 标识的动作实例，阶段固定为 Windup、Committed、Travel、Recovery、Ended、Cancelled；阶段事件是动画与特效的共同主时钟。
2. 敌人特殊动作只经过一个 Coordinator 去重和排序，再广播给动画轨与 VFX 轨，消除两个组件各自解释原始事件造成的错拍。
3. 武器表现使用一对一 `WeaponVisualKey -> WeaponPresentationProfile`：Profile 只描述该武器的可见本体、动作方式和 Plan76 已有特效语义能力；角色动画集仍独立选择，不产生笛卡尔积配置。
4. Player / Echo 可以拥有 Weapon Track；普通怪物强制 `None`；Boss 只有显式非空 `WeaponPresentationId` 才能启用 Weapon Track。
5. 结束、取消、死亡和 EndPlay 都能幂等清理循环/跟随特效并释放动作占用；重复阶段事件不重复播放。

## 架构影响与原则

- 玩法事件仍是唯一事实源；动画帧、特效完成回调和武器摆动时间不得反向决定 Commit、命中或伤害。
- Coordinator 只拥有可丢弃的表现动作实例、阶段顺序和去重，不拥有玩法冷却或投射物位置。
- 三条表现轨职责固定：Character Track 播语义 Flipbook；Weapon Track 播武器本体位移/旋转；VFX Track 播该技能或该武器唯一绑定的特效。
- 武器 Profile 不保存角色动画资产；角色 Profile 只按稳定 WeaponVisualSetId 选择已有动画集合。武器换装只替换 Weapon Track/Profile，不复制角色状态机。
- 普通怪物的狐狸蓄力、兔子投射等属于 Ability VFX Track；Boss 武器是显式可选能力，不得从怪物类型、贴图名或攻击模式推断。
- 相关文档同步：更新四份模块文档；关闭前审阅 `ARCHITECTURE.md` 与 `CODEBASE_MAP/README.md`，仅在拓扑/路由变化时修改。

## 实现步骤

1. 新增资源中立的动作键、阶段、轨道能力与阶段事件，以及主模块 `UReEchoCombatPresentationCoordinator`；锁定合法阶段推进、重复事件去重、取消/结束幂等。
2. 在 Enemy Host 显式装配 Coordinator；Enemy Presentation 与 Combat VFX 改为消费归一事件。狐狸/兔子从同一次 Windup/Commit/End 广播同步切换动画和特效。
3. 扩展 Weapon Visual Catalog 为一武器一 Profile，集中声明六种武器的本体资源、动作模式和专属攻击表现能力；WeaponActor 只按 Profile 执行动作，不再散落 VisualKey 分支。
4. 增加能力策略与自动化：普通怪物无 Weapon Track；Boss 未配置时无武器；Player/Echo 可绑定；动作键、阶段去重、取消清理和武器 Profile 唯一映射均有测试。
5. 更新模块文档，格式化并执行聚焦自动化、全量构建、项目校验和人工 PIE 交接。

## 锁定验收

- [ ] 同一特殊动作 Windup/Commit/End 只生成一个动作实例，动画与 VFX 获得相同动作键和阶段顺序。
- [ ] 重复或过期阶段不重播；Cancelled、Ended、Death、EndPlay 均幂等清理循环特效和动画动作占用。
- [ ] 狐狸 Windup 同时进入 `Attack.Charge` 与蓄力/方向特效，Commit 同时进入 `Attack.Basic` 与 Dash 特效，End 同时收束。
- [ ] 兔子 Windup 同时进入 `Attack.Charge` 与蓄力特效，Commit 收束蓄力且投射物继续以权威位置驱动，End 不残留。
- [ ] 六种武器每个 VisualKey 只解析到一个 Profile；长剑武器摆动与 Plan76 长剑特效由同一 Commit 触发，其他武器不误播长剑动作。
- [ ] 普通怪物不创建/启用 Weapon Track；Boss 无显式 WeaponPresentationId 时也不创建，显式配置才允许。
- [ ] 缺动画、武器资源或特效时只降级对应视觉，玩法继续。
- [ ] 通过格式化、Development `-FullRebuild`、聚焦自动化、`validate_project.py`、`git diff --check` 和预构建包一致性检查。
- [ ] 用户在 PIE 验收狐狸、兔子、至少一把近战和一把远程武器的同步、朝向、层级与取消/死亡清理。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 纯契约 | `ReEcho.Presentation.Combat` | 动作键、阶段顺序、去重、取消与能力策略通过 |
| 动画/VFX | `ReEcho.Presentation.Animation2D`、`ReEcho.Presentation.VFX` | 同阶段路由、现有 Profile 和 Plan76 Niagara 映射不回归 |
| 武器 | `ReEcho.Weapons` | 六武器 Profile 唯一；节拍、载体和命中不变 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 通过并刷新精选预构建包 |
| 静态 | `python scripts/validate_project.py`、`git diff --check`、`prebuilt_editor.py check` | 项目、文本及源码指纹一致 |
| 人工 | PIE：狐狸、兔子、长剑、弓；中途受击/死亡/切换 | 动画、武器和特效同拍且无残留 |

## Step 0 门禁

- 独占命令前关闭 Unreal Editor，并使用仓库 common-dir 锁。
- 发布 Plan 前重新 fetch；若 `origin/main` 出现外部提交，先展示重叠并由用户选择整合方式。
- Plan76 的 Niagara 资产与语义映射视为稳定前置，不改写二进制 VFX 资产。
- 任一新增公共类型导致模块依赖反转、或必须靠动画完成回调驱动玩法时停止并重规划。

## 执行记录

### 变化

- 新增主模块 `UReEchoCombatPresentationCoordinator` 与资源中立动作键/阶段事件；Enemy Host 显式装配后，普通攻击 Commit 与特殊动作 Windup/Commit/End 统一从 Coordinator 广播。
- Enemy Presentation 与 Combat VFX 不再分别订阅原始 SpecialAction；狐狸和兔子的动画、蓄力/方向/Dash 特效消费同一个动作实例。重复阶段被忽略，Death、EndPlay 和新动作抢占会发布幂等 Cancelled。
- 六种武器收敛到 `WeaponVisualKey -> FReEchoWeaponPresentationProfile` 一对一映射，集中描述手持资源、武器动作模式和专属攻击 VFX 能力；长剑 FullSpin 不再由散落的 VisualKey 分支决定。
- 能力策略明确锁定：Player/Echo 只有非空武器表现 ID 才启用 Weapon Track；普通怪物始终无武器；Boss 也必须显式配置。

### 证据

- Development 增量构建与最终 `-FullRebuild`（99 actions）通过；精选预构建包校验为 7 个模块、`build_id=55116800`、`source=495e7214b155`。`ReEcho.Presentation.Combat` 两项新增测试、`ReEcho.Presentation.VFX` 和 9 项 `ReEcho.Weapons` 回归通过。
- `ReEcho.Presentation.Animation2D.FootpointAlignment` 通过；`AssetProfiles` 在未被本 Plan 修改的 `DA_Enemy_TimeGuard` Phase2 Clip 完整性断言失败。当前 diff 不含 Content 资产，记录为远端基线资产缺口，不冒充本 Plan 回归或顺手修改二进制资产。
- `validate_project.py`、`git diff --check` 和 `prebuilt_editor.py check` 通过。`ARCHITECTURE.md` 与 `CODEBASE_MAP/README.md` 已审阅：模块拓扑及 AREA 路由未改变，无需修改；四份受影响模块文档已同步。

### 剩余风险与人工验收

- 仍需用户在 PIE 验收狐狸、兔子、长剑和弓的动作/特效同拍、层级、朝向及死亡/取消清理。
- TimeGuard Phase2 生产资产缺口需要独立资产任务补齐或确认基线预期；本 Plan 保留现状。
