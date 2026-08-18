# Plan 49 - 程序 - Niagara 战斗特效与兔子飞行投射物接入

## 协调

- Planner 负责人：Gavyn 侧程序 Planner。
- Executor 负责人：Gavyn 侧程序 AI。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@b029f05`。
- 本地实现方式（可选，仅作交接说明）：发布本 Plan 后，从已核验的 `origin/main` 创建 `plan/49-combat-vfx` 分支及 `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan49-vfx` 独立 worktree。
- 依赖 / 阻塞：美术资源位于 `C:\Users\gavynqiu\Documents\miniGame\Content (2)\Content`，制作版本已确认为 UE 5.8；美术播放约定来自 `C:\Users\gavynqiu\Documents\miniGame\Content (2)\回复.txt`；用户已确认采用真实飞行的兔子投射物，并确认这些资产可用于本项目交付。最终视觉质量由用户 PIE 验收。
- Writes:
  - `plans/49-combat-niagara-vfx-integration.md`
  - `ReEcho.uproject`
  - `Source/ReEcho/ReEcho.Build.cs`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/**`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*`
  - `Source/ReEcho/{Public,Private}/Player/ReEchoPlayerPawn.*`（仅限表现组件装配与只读事件接线）
  - `Source/ReEcho/{Private}/Presentation/Enemy/ReEchoEnemyPresentationComponent.cpp`
  - `Source/ReEcho/Private/Tests/*Vfx*Tests.cpp`
  - `Source/ReEchoEnemies/{Public,Private}/Enemies/ReEchoEnemyEventsComponent.*`
  - `Source/ReEchoEnemies/{Public,Private}/Enemies/ReEchoEnemyProjectileLogic.*`
  - `Source/ReEchoEnemies/Private/Tests/**`（仅限投射物和表现事件契约测试）
  - `Content/VFX/**`、`Content/Mat/**`、`Content/01_Textures/**` 中导入清单明确列出的资产
  - `Design/Art/VFX/combat_vfx_import_manifest.csv`
  - `scripts/art/import_combat_vfx.py` 及其聚焦测试
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`
  - `shared/CODEBASE_MAP/README.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
  - `shared/LESSONS.md`（仅在形成有证据的可复用经验时）
  - `Binaries/Win64/` 下 `GIT_RULES.md` 允许的最终预构建包文件
- Stable Reads:
  - `Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`
  - `Source/ReEchoWeapons/**`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`
  - `Content/Data/enemies.csv` 与 `Design/Data/ReEchoData.xlsx`（只读；兔子敌方远程伤害继续保持当前 0 值）
- 影响模式：`SharedContract`（增加敌人特殊动作的类型化表现事件，并让主模块消费；不改变 Combat/Weapons 的伤害权威）。
- 兼容承诺 / 下游操作：缺失、加载失败或被裁剪的特效只降低视觉反馈，不能阻止攻击、命中、移动、存档或回放；保留现有灰盒命中反馈作为安全回退。导入资产保留原始 `/Game/VFX`、`/Game/Mat`、`/Game/01_Textures` 引用关系。兔子投射物在本 Plan 内仍为 0 伤害，待玩家看清并验收飞行表现后才允许由策划数据恢复伤害。
- 明确排除：不整包复制 1438 个 `.uasset`；不迁移 `Map/`、`Developers/`、`SourceArt/`、备用特效或 `People/Bullet` 重复资产；不让 Niagara 回调、粒子碰撞或播放时长决定玩法伤害、攻击窗口、命中、冷却、销毁或存档；不修改武器/敌人平衡数值；不把 VFX 拆成新的 Runtime Module；不由 AI 代替用户完成视觉验收。

## 锁定目标

1. 将美术确认的 8 个正式 Niagara System 及其完整静态依赖以可审计、可复现的最小集合接入项目：
   - `Monster/Rabbit/Particle/NS_Rabbit_Charging_01`：兔子蓄力。
   - `Monster/Rabbit/Particle/NS_Rabbit_Attack_02`：兔子飞行子弹。
   - `Monster/Rabbit/Particle/NS_Rabbit_BeAttacked_01`：玩家受击。
   - `Monster/Fox/Particle/NS_Fox_Rush_02`：狐狸蓄力。
   - `Monster/Fox/Particle/NS_Fox_Rush_01`：狐狸方向箭头。
   - `Monster/Fox/Particle/NS_Fox_Rush_04`：狐狸冲刺。
   - `People/Sword/Particle/NS_People_Sword_Attack_01`：玩家近战刀光。
   - `People/Sword/Particle/NS_Rabbit_BeAttacked_01`：怪物受击。
2. 建立只读战斗 VFX 表现适配层：订阅 Combat/Enemy 的类型化事件并播放语义特效；逻辑模块不读取 Niagara 资产，特效不反向驱动玩法。
3. 兔子攻击由当前锁点即时范围命中表现改为真实飞行投射物：逻辑拥有轨迹、碰撞、边界、存活期和销毁，Niagara 只跟随视觉载体；本 Plan 继续保持其伤害为 0。
4. 狐狸按类型化特殊动作阶段播放“蓄力 + 方向箭头 + 冲刺”，玩家近战提交时播放刀光，玩家/怪物实际受伤时播放各自权威受击特效。
5. 所有特效具备安全回退、确定性去重和生命周期清理，不因暂停、死亡、取消、关卡结束或对象销毁留下循环/悬空组件。

## 架构影响与设计决策

- 受影响架构标识：
  - `MOD-ReEcho` / `AREA-Presentation`：新增 Niagara 目录、语义目录与只读播放适配器，并在玩家/敌人 Host 装配。
  - `MOD-ReEchoEnemies` / `AREA-Enemies`：提供特殊动作阶段事件和通用敌方投射物逻辑状态；仍不依赖 Niagara 或主模块表现类型。
  - `MOD-ReEchoCombat` / `AREA-AbilityCombat`：仅消费现有 `FReEchoAttackCommittedEvent`、`FReEchoDamageEvent`，不修改状态所有权或模块代码。
  - `MOD-ReEchoWeapons` / `AREA-Weapons`：仅消费其经 Combat 发布的攻击提交事实，不修改武器节拍、步骤和载体权威。
- 对应模块文档：创建 `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`（明确标记为 `MOD-ReEcho` 内的文档型逻辑入口、非 Runtime Module）；维护 `MOD-ReEcho.md` 与 `MOD-ReEchoEnemies.md`；审阅 `MOD-ReEchoCombat.md`、`MOD-ReEchoWeapons.md`。上述实际需修改的文档均已加入 `Writes`。
- 设计意图：玩法逻辑只发布稳定语义和空间上下文，表现适配器根据语义选择资产并管理 Niagara 生命周期。美术可独立替换同语义资源，程序可独立调整攻击逻辑而不把资产路径散落到 Combat、Weapons 或 EnemyLogic。
- 权威状态与依赖：
  - `ReEchoCombat` 保持攻击提交和命中结算权威；`ReEchoWeapons` 保持玩家攻击节拍与武器载体权威；`ReEchoEnemies` 保持敌人行为阶段与敌方投射物运动权威。
  - `ReEcho` 主模块的 VFX 表现层只读订阅以上事件，单向依赖 Niagara。
  - 新增的敌人阶段事件携带稳定动作/阶段、攻击身份、位置和锁定方向，不携带资源对象或表现参数。
- 决策记录：
  1. 采用“逻辑载体 + Niagara 跟随”的真实兔子飞行子弹，而非用 Niagara 粒子碰撞决定命中；原因是能复用 `FReEchoEnemyProjectileLogic` 的确定性轨迹、碰撞和存档语义。
  2. 不新增 `ReEchoVFX` Runtime Module；当前接线依赖主模块玩家/敌人 Host，先作为 `MOD-ReEcho` 内的独立表现领域，避免为少量适配器制造循环依赖。文档以 `MOD-ReEchoVFX` 稳定入口描述边界，并明确其非 Runtime Module 状态。
  3. 资产采用精确根清单 + 递归静态依赖 + SHA-256 manifest，不整包迁移；这兼顾可复现性、许可审计与仓库体积。
  4. 美术标注的播放时长和前后景仅作为表现配置；玩法时序继续以逻辑事件和表数据为准。循环或跟随特效必须由逻辑生命周期显式停止。
  5. `NS_Rabbit_BeAttacked_01` 同名但不同目录的两个资产按美术答复分别映射玩家受击与怪物受击，禁止只按短名查找。
- 相关文档同步范围：
  - `ARCHITECTURE.md`：补充逻辑事件到 VFX 表现的单向数据流和“Niagara 不拥有玩法”的跨模块不变量。
  - `README.md`：增加 `MOD-ReEchoVFX` 文档型入口和 `AREA-Presentation` 路由。
  - `MOD-ReEcho.md`：记录 VFX 装配、目录、公共入口、回退和测试位置。
  - `MOD-ReEchoEnemies.md`：记录特殊动作阶段事件和敌方投射物逻辑/表现边界。
  - `MOD-ReEchoVFX.md`：详细记录存在原因、语义目录、输入事件、生命周期、排序/朝向/缩放约定、扩展方式、资产 manifest、代码与测试位置。
  - `MOD-ReEchoCombat.md`、`MOD-ReEchoWeapons.md`：关闭前审阅现有事件是否足够；若正文事实未变化，在本 Plan 记录无需修改原因。
- 关闭前逐项填写审阅结果：见“架构文档审阅结果”。

## 锁定验收

- [ ] 精确导入清单只包含 8 个正式 Niagara System 及其实际递归静态依赖；脚本可在给定源根时校验 SHA-256、缺失依赖、目标冲突并重复执行，仓库不含整包无关资产。
- [ ] UE 5.8 能加载并编译清单内 Niagara/材质/纹理资产；无丢失引用、重定向器、机器绝对路径或未确认插件依赖；必要的 Fixed Bounds 已检查。
- [ ] 兔子在 Windup 播放蓄力，Commit 后从兔子位置沿锁定方向生成可见飞行子弹；逻辑投射物负责移动、玩家/边界碰撞和销毁，视觉载体同步销毁，伤害保持 0。
- [ ] 狐狸在 Windup 播放蓄力与方向箭头，在 Commit/Active 播放跟随冲刺特效；取消、打断、死亡和遭遇结束会清理陈旧特效。
- [ ] 玩家近战攻击提交播放朝向正确的刀光；玩家与怪物仅在 `AppliedDamage > 0` 时分别播放正确目录的受击特效；Echo 的玩家武器攻击也沿同一语义接线播放刀光但不会被误判为敌方伤害。
- [ ] 缺失/加载失败特效不会改变攻击、伤害、移动、暂停、回放、存档或关卡流程，并留下明确但限频的诊断信息。
- [ ] 类型化事件和运行时自动化覆盖：语义映射、阶段顺序、重复提交去重、兔子投射物轨迹/碰撞/到期、取消/死亡清理、缺失资产回退与“VFX 不影响玩法”。
- [ ] 修改的 C++ 已格式化；Editor Development 构建、聚焦自动化、`python scripts/validate_project.py`、`git diff --check` 通过；最终发布候选完成 `-FullRebuild` 并刷新允许的预构建包。
- [ ] 用户在 PIE 验收兔子/狐狸组合效果、刀光、玩家/怪物受击、方向、尺寸、前后景、裁剪与可读性。
- [ ] 未提交清单外资源、精选预构建允许列表外 UE 生成产物、日志或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@b029f05`；Plan 发布后再次 fetch，核验远端准确包含本文件且无新外部提交。
- 引擎/构建可用性：UE 5.8 安装版；执行任何 Editor/构建命令前使用 Git common-dir Unreal 锁，并先确认交互式 Editor 已关闭。
- 现有聚焦测试结果：基线构建/自动化证据不冒充本候选证据；实现后重新运行受影响套件。
- 共享契约 / 难合并资源风险：`.uasset` 为二进制，冲突不能自动选 ours/theirs；必须只从干净远端基线导入精确路径，并在推送前重新 fetch 审计同路径资产、EnemyActor、EnemyEvents 和预构建包变化。
- 基线损坏时的停止条件：正式根资产或递归依赖缺失；资产不是 UE 5.8 可加载格式；需要未声明第三方插件/Shader；远端在实现前或发布前改动同路径二进制、敌人事件、EnemyActor 或投射物契约；此时停止相关写入并报告物理冲突、逻辑冲突和组合方案。

## 实现提纲

1. 生成 8 个根资产的递归静态依赖闭包与 SHA-256 manifest；先做 dry-run，确认目标路径、重复短名和未解析引用，再复制最小集合。
2. 启用 Niagara 插件和主模块依赖，在 `Presentation/VFX` 建立集中语义目录、播放参数、事件去重和生命周期管理；资产路径只在一个目录中解析。
3. 为敌人特殊动作增加表现中立的 Windup/Committed/Cancelled/Interrupted 类型化阶段事件；Host 只负责装配和把逻辑事实转交表现。
4. 泛化并复用敌方投射物逻辑承载兔子子弹；主模块创建只读视觉载体，按逻辑位置更新/销毁，保持当前敌方远程伤害 0。
5. 接入狐狸三段特效、玩家近战刀光、玩家/怪物受击特效及灰盒回退；确保暂停、死亡、取消和 EndPlay 清理。
6. 加入资产、事件与投射物聚焦测试；在 UE 中加载/编译/保存导入资产并验证 Fixed Bounds、引用与 cook 可达性。
7. 更新 Plan 执行记录和全部相关 `CODEBASE_MAP` 文档，完成客观门禁后交给用户做 PIE 视觉验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 导入清单 | `python scripts/art/import_combat_vfx.py --source-root "C:\Users\gavynqiu\Documents\miniGame\Content (2)\Content" --check` | 8 个根资产、递归依赖、SHA-256 与目标路径全部一致，无未声明文件 |
| Python | 聚焦 `scripts/art` 测试 | dry-run、缺失、哈希不符、冲突与重复执行行为通过 |
| 静态 | `python scripts/validate_project.py` | 项目、模块依赖、资源清单和源码不变量通过 |
| 格式 | 对修改的 `.h/.cpp` 执行仓库 `.clang-format`，再检查 diff | 仅目标文件格式变化且无玩法漂移 |
| C++ | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0并刷新开发候选 |
| 自动化 | `scripts/ue/Run-Automation.cmd` 聚焦 VFX、Enemies、Weapons/Combat 受影响套件 | 语义事件、生命周期、投射物和回归检查通过 |
| 资产 | UE Commandlet/Editor 资产加载、Niagara/材质编译、引用与必要 cook 检查 | 8 个正式系统及依赖无加载错误、缺失引用或裁剪问题 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` 后 `python scripts/validate_project.py`、`git diff --check` | 最终候选与预构建包指纹一致，工作区只含允许产物 |
| 人工 | 用户 PIE 检查兔子、狐狸、刀光、双方受击及前后景/裁剪 | 记录 `Passed` 或具体返工项 |

## 执行记录

### 变化

- 待实现。

### 证据

- Plan 编写前只读确认：远端最大 Plan 编号为 48。首次规划基线为 `f874923`；发布门禁构建期间远端前进到 `b029f05`，用户确认以远端 Plan47/Card Runtime 为权威进行组合适配，本 Plan 基线随之更新。
- 美术包只读审计确认 8 个正式 Niagara 根资产可形成约 80 个 `.uasset` 的最小静态依赖闭包；实际 manifest 以实现阶段脚本生成并复核的精确结果为准。

### 剩余风险

- Niagara 资产的实际尺寸、朝向、透明排序、Fixed Bounds 与镜头裁剪仍需 UE 加载和用户 PIE 判断。
- 兔子从锁点范围行为改为飞行投射物会改变攻击到达时序，但不恢复伤害；恢复伤害必须另行由策划/用户确认数据语义。

### 人工验收结果/请求

- `PendingBeforeClose`：用户负责 PIE 视觉验收；不要求 Executor 消耗视觉探索 token 代替人判断。

### 架构文档审阅结果

- 待实现与关闭评审时逐项填写。
