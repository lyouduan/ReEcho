# Plan 148 - 程序 - 近战刀光范围缩放适配

## 协调

- Planner 负责人：Codex（程序路线）。
- Executor 负责人：Codex（程序路线）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@409910aa08e544722910639e557ff15c6ef5af22`。
- 本地实现方式（可选，仅作交接说明）：规划 worktree `C:\tmp\ReEcho-plan148-weapon-vfx-range-scale-plan`；发布后从准确 `origin/main` 创建独立执行 worktree。
- 依赖 / 阻塞：沿用 Plan126 的 `WeaponAttackVfxRoot` 最终武器挂点、Plan100 的长剑/镰刀动作与左右朝向、当前 Weapon Presentation DA 和专用 Niagara；实施前须确认本 Plan 已进入 `origin/main`。
- Writes: `plans/148-weapon-vfx-range-scale.md`；`Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`；`Source/ReEchoWeapons/Public/Weapons/ReEchoWeaponGeometry.h`、`Source/ReEchoWeapons/Private/Weapons/ReEchoWeaponGeometry.cpp` 及其聚焦测试；`Source/ReEcho/Public/Weapons/ReEchoWeaponActor.h`；`Source/ReEcho/Private/Weapons/ReEchoWeaponActor.cpp`；`Source/ReEcho/Public/Graybox/ReEchoEnemyActor.h`、`Source/ReEcho/Private/Graybox/ReEchoEnemyActor.cpp`；`Source/ReEcho/Public/Presentation/Weapon/ReEchoWeaponPresentationProfile.h`；`Source/ReEcho/Public/Presentation/VFX/ReEchoCombatVfxComponent.h`；`Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxComponent.cpp`；`Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`；必要的武器运行聚焦测试；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`；FullRebuild 刷新的 `Binaries/Win64/ReEchoEditor.prebuilt.json` 及其准确允许列表产物；若安全 Editor 自动化可用，长剑与镰刀 Weapon Presentation DA 及镰刀专用 Niagara。
- Stable Reads: `Content/Data/attack_steps.csv`；`Content/Data/part_effects.csv`；`Source/ReEchoWeapons/Public/Weapons/ReEchoWeaponTypes.h`；`Source/ReEchoWeapons/Private/Weapons/ReEchoWeaponLogic.cpp`；Plan126 已关闭的挂点契约；长剑与镰刀当前 Niagara 资源局部轴和生产 DA。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：不改变伤害、玩法基础攻击范围、攻击角度、攻击节拍、斩弹、手动/自动互斥、最终挂点、左右播放方向、枪弓与命中特效；无范围修正时长剑和镰刀本体/刀光保持 DA 基础尺寸。镰刀 FullSpin 围绕其可见组件自身中心旋转，长剑仍以手部挂点为圆心三挥。
- 明确排除：不修改 AttackStep 的玩法基础范围；镰刀 `400cm` 只是 Weapon Presentation DA 的本体基础长度。不重做 Niagara，不用刀光/武器 Bounds 参与伤害判定，不把玩法基础范围复制进 Weapon Presentation DA，不修改 Boss 武器表现。

## 锁定目标

长剑和镰刀的武器本体与提交刀光必须共用“当前最终范围 / AttackStep 基础范围”倍率：无范围符文时保持 DA 基础尺寸；静态范围符文或临时群攻成长改变范围后，长剑本体/刀光只消费长度局部轴，镰刀本体/刀光消费相机平面 XY 轴。刀光仍以 Plan126 最终武器挂点为原点，武器偏移、镜像与挂点不参与倍率重算。镰刀本体的 DA 基础长度为 `400cm`。

镰刀武器本体的 FullSpin 必须围绕其当前可见组件中心旋转，不再因手部挂点到镰刀中心的 `HeldOffsetRatio` 形成环绕角色的轨道；旋转期间自身中心和中心型刀光挂点保持稳定。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoCombat` / `AREA-AbilityCombat`、`MOD-ReEcho` / `AREA-Weapons`、`MOD-ReEchoPresentation` / `AREA-Presentation`、文档入口 `MOD-ReEchoVFX`；`MOD-ReEchoWeapons` 仅稳定读取并审阅，不修改其玩法范围权威。
- 对应模块文档：维护 `MOD-ReEcho.md`、`MOD-ReEchoCombat.md`、`MOD-ReEchoPresentation.md`、`MOD-ReEchoVFX.md`，均已加入 `Writes`；关闭前审阅 `MOD-ReEchoWeapons.md`、`ARCHITECTURE.md` 与 `README.md`。
- 设计意图：一次攻击的玩法执行层提供不可变的最终范围事实，表现层只决定该倍率如何映射到资源局部尺寸；避免表现重算符文或 DA 复制基础范围形成第二权威。
- 权威状态与依赖：`AReEchoWeaponActor` 继续在 Weapon Logic 完成静态符文编译后叠加临时范围层，并让执行、命中与 `AttackCommitted` 共用同一有效 Commit。`ReEchoCombat` 的资源无关事件增加最终范围、基础范围、范围倍率和有效角度值字段；VFX 只读消费。依赖方向不变，Combat 不依赖 Weapon 或 Presentation。
- 决策记录：拒绝把“武器当前缩放”再次乘入特效，避免父级继承与 `bPreserveWorldSize` 下重复缩放；拒绝在 DA 保存 `400/250cm` 基础范围。DA 只保存缩放模式、局部轴遮罩和表现钳制。长剑角度从 180 度收窄到 120 度无法由普通 Transform 精确形变，本 Plan 只锁定最远范围匹配，动态弧度参数化留作明确后续。
- 相关文档同步范围：维护上述四个模块文档；审阅 `ARCHITECTURE.md`、`README.md` 和 `MOD-ReEchoWeapons.md`，仅在拓扑、路由或 Weapons 契约事实变化时修改正文。
- 关闭前逐项填写审阅结果：待实现后补充。

## 锁定验收

- [x] 无范围修正时长剑、镰刀刀光保持 DA 当前作者尺寸与 Plan126 最终挂点，不改变位置、厚度、左右朝向或播放方向。
- [x] 长剑静态 `AttackRange ×1.5` 时，实际伤害与刀光最远端共享 `1.5` 范围倍率；只缩放 DA 声明的径向轴。
- [x] 镰刀/长剑群攻成长的临时范围层在下一次攻击提交时被锁定；镰刀延迟播放仍使用提交时倍率，不读取延迟触发时的可变状态。
- [x] Niagara 缩放只作用于本次生成的 Component；持有武器本体从同一倍率独立计算 DA 声明的局部轴。两者都不移动 `WeaponAttackVfxRoot`、Weapon Actor Location 或 Slot Offset。
- [x] 长剑/镰刀静态符文、群攻成长获得及过期都刷新持有武器尺寸；当前生产卡牌无 `AttackRange` 效果，不伪造卡牌数据。
- [x] 枪、弓、DamageApplied、Travel、Boss VFX、近战斩弹和手动/自动事件出口保持回归通过。
- [x] 修改源码完成格式化；聚焦自动化、Development `-FullRebuild`、项目校验、预构建检查和 `git diff --check` 通过。
- [ ] 用户在 PIE 中确认无符文、长剑广域符文及镰刀群攻成长情况下，刀光外缘与实际攻击范围观感匹配且中心不漂移。
- [ ] 用户在 PIE 中确认镰刀 FullSpin 围绕镰刀自身中心旋转，角色左右朝向均不绕人物画圆；长剑手部枢轴三挥无回归。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@409910aa08e544722910639e557ff15c6ef5af22`。
- 引擎/构建可用性：UE 5.8 Windows 仓库入口可用；最终程序候选必须关闭当前工作区对应 Editor 后执行 Development `-FullRebuild`。
- 现有聚焦测试结果：规划阶段仅完成静态链路审计，不复用 Plan126 的历史构建或 PIE 证据。
- 共享契约 / 难合并资源风险：Combat Event、WeaponActor、CombatVfx 和 Profile 头是共享文本契约；DA 为二进制且只允许通过 Editor/安全作者化工具修改。发布前必须 fetch 并逐路径审计。
- 基线损坏时的停止条件：最新 main 的同一事件/武器/VFX 路径存在真实语义冲突；无法从权威 AttackStep 取得稳定基础范围；Niagara 局部径向轴需要改变资源内容或美术取舍；当前工作区对应 Editor 未关闭而验证需要独占。

## 实现提纲

1. 在 WeaponActor 将静态与临时修正统一成一次有效 Commit，并让执行与事件发布消费同一值；从原始 AttackStep 权威取得本次步骤基础范围，形成稳定倍率快照。
2. 扩展资源无关 `FReEchoAttackCommittedEvent`，传递最终范围、基础范围、倍率和有效角度；补充无效基础范围的 `1.0` 安全回退。
3. 在 `FReEchoWeaponVfxSlot` 增加局部轴遮罩与倍率钳制；Catalog 保留作者 Scale，CombatVfx 在 `ResolveAttachedScale` 前仅对目标刀光叠加范围倍率。
4. 让立即长剑和延迟镰刀都锁定提交时倍率；延迟回调不得重新读取当前武器或符文状态。
5. 补充纯函数、事件和路径回归测试；若生产 DA 需要二进制配置，使用安全 Editor 自动化并精确审计资产差异。
6. 维护模块文档与执行记录，完成格式化、聚焦自动化、FullRebuild、项目/预构建/diff 检查，交由用户 PIE 验收。
7. Weapon Presentation DA 选择是否让持有武器消费同一范围倍率、局部轴与表现钳制；长剑按尺寸主轴、镰刀按 XY，群攻成长获得/过期时即时刷新。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 纯函数/契约 | 聚焦 `ReEcho.Presentation.VFX` 与武器范围自动化 | `1.0/1.5/临时层` 倍率、轴遮罩、钳制和延迟快照断言通过 |
| 武器回归 | `ReEcho.Weapons.Runtime` 与近战斩弹聚焦测试 | 最终 Commit、长剑180度/镰刀360度、斩弹及符文行为不回归 |
| 表现回归 | `ReEcho.Presentation.Combat.Capabilities` | 最终武器挂点、左右朝向、枪弓及现有 VFX 能力通过 |
| C++ | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UE 5.8 UHT/UBT 成功并刷新准确精选包 |
| 静态 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 项目、预构建包、源码与范围不变量通过 |
| 人工 PIE | 长剑/镰刀分别验证无符文、静态范围符文和临时范围层，左右方向各一次 | 刀光外缘匹配实际范围、挂点不漂移、资源轴和厚度正确 |

## 执行记录

### 变化

- 2026-08-28：从 `origin/main@409910aa` 规划；确认 Plan126 已关闭，本任务因扩展 Combat 公共事件及 Weapon/VFX 跨模块契约新建 Plan148，不回写旧计划。
- 2026-08-28：按用户验收方向扩展同一近战表现任务：镰刀 FullSpin 使用位置反补偿锁定自身中心，长剑仍绕手部挂点三挥；不改变玩法范围或攻击时序。
- 2026-08-28：PIE 反馈镰刀本体未显示旋转；确认 `UBillboardComponent` 在渲染阶段保持朝向相机，位置反补偿消除公转后贴图角度仍不可见。将镰刀迁移为与其他可旋转武器一致的透明 Plane；投掷仅切换绝对位置、旋转继续继承 FullSpin，保留当前挂点、尺寸、镜像、召回和刀光附着契约。
- 2026-08-28：按策划尺寸及符文适配方案，镰刀 Weapon Presentation DA 基础长度改为 `400cm`；长剑与镰刀本体通过 DA 轴遮罩消费最终/基础范围倍率，并在群攻成长获得或过期时刷新。生产数据审计确认当前无 `AttackRange` 卡牌；范围来源为镰刀 `×0.8`、长剑 `×1.5` 静态符文与两者群攻成长。
- 2026-08-28：PIE 确认镰刀 `AttackCommitted.Offset.Scale` 生效，但刀光原使用摄像机法线建立平面，会竖直穿入地面。保留 `AS_SCYTHE_1` 已有 `360°` 玩法命中，仅将 `PlayerScytheSlash` 改为世界 `+Z` 法线的地面平行旋转；DA Offset 仍控制尺寸、离地高度与资源局部校正。
- 2026-08-28：继续 PIE 确认组件旋转仍被 Niagara Sprite Renderer 的 FaceCamera 覆盖。将镰刀专用 `NS_People_Sickle_Attack_01` 改为 `CustomFacingVector`，绑定 `User.GroundNormal`；运行时在激活前显式写入局部 `+Z`。作者脚本审计所有 Weapon Presentation Profile，确认没有其他武器共用该 System 后才保存；`01_2` 仅是同目录 Niagara 内部引用。
- 2026-08-28：按用户“球形360°”确认将玩法几何从 `Dist2D + ArcDegrees=360` 的无限高圆柱改为以 WeaponOwner 为球心、`RangeCm` 为半径的真三维球。仅 `Pattern.ScytheSweep` 使用球形敌人和兔子弹丸查询；长剑仍是地面前向 180° 扇形。
- 2026-08-28：PIE 反馈球形范围与刀光边界错位；确认半径均消费同一倍率，但伤害球心还在 WeaponOwner，刀光中心则是 Plan126 最终 `WeaponAttackVfxRoot`。镰刀球心改为攻击提交瞬间的最终武器中心，敌人、外环距离与斩兔子弹共用该 Origin；挂点不可用时才回退 WeaponOwner。
- 2026-08-28：地面法线后仍出现刀光下半部被地表深度遮挡；确认 Renderer Facing 只控制单张 Sprite 朝向，Emitter 世界空间粒子位置不会随地面对齐的 Component 旋转。镰刀专用 Niagara 的所有启用 Emitter 同步改为 Local Space，与 `CustomFacingVector/User.GroundNormal` 共同构成地面平行契约；不关闭深度测试，不让刀光透过人物或场景。
- 2026-08-28：按用户验收将镰刀刀光参考长剑完整消费同一最终攻击范围倍率：修正 DA 中残留的 `(0.6,0.6,0)` 范围权重为 `(1,1,0)`，并把 `AttackCommitted.Offset.Scale` 的地面 XY 基础尺寸从 `(0.5,0.5,1)` 增加 10% 为 `(0.55,0.55,1)`。因此普通与符文放大状态都匹配镰刀本体并保留 10% 视觉余量；不改变 Z 厚度、镰刀本体 400cm 长度或伤害球。
- 2026-08-28：长剑刀光改为与镰刀一致消费 `0811_01` 的真实本地 YZ 平面：本地 X 法线固定映射世界向上，本地 Y 映射 Commit 攻击方向，忽略会掀离地面的 DA Pitch/Yaw，仅保留绕法线的 Roll；默认与四元素 Niagara 统一 Local Space、Mesh Default 及 Sprite GroundNormal/GroundTangent。长剑仍为前方 180°、Commit 同帧出伤与播放，并保留左右 `User.PlayDirection`。

### 证据

- 规划静态审计确认：静态符文在 AttackStep 编译期修改 `RangeCm`，群攻成长在 WeaponActor 执行前叠加临时倍率；当前 `AttackCommitted` 不携带范围，且镰刀延迟刀光只锁定方向，因此表现无法匹配最终范围。
- Development 增量构建通过。Editor Python 完成两份 DA 保存：长剑 `AttackCommitted` 使用局部 Y 遮罩，镰刀使用局部 XY 遮罩，均配置 `0.5..2.0` 表现钳制；SDK `MainVersion` 警告后 Win64 Editor 仍继续执行，证据来自完整 session 日志中的两条 `PLAN148_MELEE_RANGE_SCALE`。
- `ReEcho.Presentation.VFX.Catalog`、`ReEcho.Presentation.Combat.Capabilities`、`ReEcho.Weapons.Runtime.MeleeProjectileCutEligibility` 各发现 1 项且 `Result={Success}`；分别覆盖范围轴/钳制、现有武器挂点与枪弓能力、长剑镰刀斩弹回归。
- 最终 Development `-FullRebuild` 100/100 成功，精选包 Build ID `55116800`、source fingerprint `e0ef7b5141d7`；`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过。
- 镰刀自中心补偿后 `ReEcho.Presentation.Combat.Capabilities` 发现 1 项且 `Result={Success}`；最终 Development `-FullRebuild` 97/97 成功，精选包 Build ID `55116800`、source fingerprint `291ab894576e`，项目/预构建/diff 检查再次通过。
- 镰刀透明 Plane 修复后 Development `-FullRebuild` 96/96 成功，精选包 Build ID `55116800`、source fingerprint `adc77aecf4cd`；`ReEcho.Presentation.Combat.Capabilities` 发现 1 项且 `Result={Success}`，项目校验、预构建检查和 `git diff --check` 通过。Plane 的实际整圈视觉旋转仍保留给 PIE 人工验收。
- 持有武器范围同步后，Editor Python 成功保存两份生产 DA，session 日志记录 `PLAN148_HELD_RANGE_SCALE sword_mask=Y scythe_length_cm=400 scythe_mask=X,Y limits=0.5..2.0`。最终 Development `-FullRebuild` 98/98 成功，精选包 Build ID `55116800`、source fingerprint `43a79d288440`；LFS 水合、`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过。
- 本轮聚焦自动化未进入测试队列：UE 5.8 在加载项目前执行 `ValidatePlatforms -AllPlatforms`，因 LinuxArm64/VisionOS SDK 缺少 `MainVersion` 提前退出；Win64 同一源码的 FullRebuild 已通过，但不将构建结果代替记为自动化通过。
- 镰刀刀光地面平行修正后 Development `-FullRebuild` 100/100 成功，精选包 Build ID `55116800`、source fingerprint `3c8e23c779d3`；`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过。纯函数测试已加入地面 XY 攻击方向和世界 `+Z` 法线断言，但聚焦自动化仍受上述全平台 SDK 预检阻塞，保留给 PIE 人工验收。
- Niagara Renderer 地面法线修正后 Development `-FullRebuild` 100/100 成功，精选包 Build ID `55116800`、source fingerprint `680f495d0d3f`；作者化日志记录 `PLAN148_SCYTHE_GROUND_FACING system=/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_01 facing=CustomFacingVector binding=User.GroundNormal`，项目校验、预构建一致性与 `git diff --check` 通过。
- 镰刀真三维球形几何后 Development `-FullRebuild` 101/101 成功，精选包 Build ID `55116800`、source fingerprint `d307bf541fbb`；纯函数测试编译覆盖球顶内点、精确球面和 XYZ 组合越界点。`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过；自动化执行仍受全平台 SDK `MainVersion` 预检阻塞，不记为通过。
- 镰刀球心与最终武器中心对齐后 Development `-FullRebuild` 101/101 成功，精选包 Build ID `55116800`、source fingerprint `778560e1e792`。只读 DA/Niagara 审计记录当前镰刀 `Offset.Scale=(0.5,0.5,1)`、Fixed Bounds `[-100,100]`；由于该 Bounds 不能代表实际可见粒子外缘，不用它反推伤害半径。
- 镰刀下半圈受地面深度遮挡的修正已落在粒子模拟层：生产 Niagara 的全部启用发射器切换为 Local Space，并继续由 Sprite Renderer 的 `CustomFacingVector=User.GroundNormal` 配合组件世界 `+Z` 法线控制整圈平行地面；未关闭深度测试。作者化日志记录 `PLAN148_SCYTHE_GROUND_FACING ... local_space=true facing=CustomFacingVector binding=User.GroundNormal`。最终 Development `-FullRebuild` 101/101 成功，精选包 Build ID `55116800`、source fingerprint `f22d223ed4ef`；`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过。正面相机下的完整下半圈仍需 PIE 人工验收。
- 镰刀刀光尺寸同步后，作者化日志记录 `PLAN148_SCYTHE_VFX_SIZE ... base_scale=(0.55,0.55,1) range_mask=X,Y size_bonus=1.10`；Development `-FullRebuild` 101/101 成功，精选包 Build ID `55116800`、source fingerprint `b840b0abb7f4`，项目校验、预构建一致性与 `git diff --check` 通过。刀光外沿相对镰刀本体的 10% 观感仍需 PIE 人工验收。

### 剩余风险

- 长剑/镰刀 Niagara 的真实局部径向轴必须通过 Editor/PIE 验证；不能只根据组件 X/Y/Z 名称认定。
- 长剑 `120°/180°` 弧度的精确视觉匹配不属于普通缩放能力，本 Plan 不声称解决动态弧度形变。
- 需在 LinuxArm64/VisionOS SDK `MainVersion` 预检恢复或可正常启动当前项目 Editor 后，补跑 `ReEcho.Presentation.Combat.Capabilities`、`ReEcho.Presentation.VFX.Catalog`、长剑/镰刀斩弹与群攻符文聚焦自动化。

### 人工验收结果/请求

- `PendingBeforeClose`：实现和客观门禁完成后，请用户在 PIE 确认刀光与实际范围观感。

### 架构文档审阅结果

- `MOD-ReEcho.md` 已更新：记录最终 Commit 范围快照与只缩放刀光组件的主模块适配。
- `MOD-ReEchoCombat.md` 已更新：记录资源无关 AttackCommitted 几何快照契约。
- `MOD-ReEchoPresentation.md`、`MOD-ReEchoVFX.md` 已更新：记录 DA 轴遮罩、延迟快照和迁移默认值。
- `MOD-ReEchoWeapons.md` 已审阅、无需修改：基础/有效范围和 AttackStep 编译权威未改变，改动位于主模块宿主与 Combat 事件。
- `ARCHITECTURE.md` 已审阅、无需修改：模块拓扑和依赖方向不变。
- `README.md` 已审阅、无需修改：稳定模块及 AREA 路由不变。
