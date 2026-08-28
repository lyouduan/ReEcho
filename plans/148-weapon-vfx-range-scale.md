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
- Writes: `plans/148-weapon-vfx-range-scale.md`；`Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`；`Source/ReEcho/Public/Weapons/ReEchoWeaponActor.h`；`Source/ReEcho/Private/Weapons/ReEchoWeaponActor.cpp`；`Source/ReEcho/Public/Presentation/Weapon/ReEchoWeaponPresentationProfile.h`；`Source/ReEcho/Public/Presentation/VFX/ReEchoCombatVfxComponent.h`；`Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxComponent.cpp`；`Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`；必要的武器运行聚焦测试；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`；FullRebuild 刷新的 `Binaries/Win64/ReEchoEditor.prebuilt.json` 及其准确允许列表产物；若安全 Editor 自动化可用，长剑与镰刀 Weapon Presentation DA。
- Stable Reads: `Content/Data/attack_steps.csv`；`Content/Data/part_effects.csv`；`Source/ReEchoWeapons/Public/Weapons/ReEchoWeaponTypes.h`；`Source/ReEchoWeapons/Private/Weapons/ReEchoWeaponLogic.cpp`；Plan126 已关闭的挂点契约；长剑与镰刀当前 Niagara 资源局部轴和生产 DA。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：不改变伤害、基础攻击范围、攻击角度、攻击节拍、斩弹、手动/自动互斥、武器本体尺寸、最终挂点、左右播放方向、枪弓与命中特效；无范围修正时保持当前刀光尺寸。
- 明确排除：不在本 Plan 调整镰刀基础 `250cm` 数值，不重做 Niagara，不用刀光 Bounds 参与伤害判定，不把玩法基础范围复制进 Weapon Presentation DA，不修改 Boss 武器表现。

## 锁定目标

长剑和镰刀的提交刀光必须消费本次攻击实际执行的最终范围快照：无范围符文时保持当前武器与刀光比例；静态范围符文或临时群攻成长改变范围后，刀光只在 DA 声明的资源局部径向轴同步缩放，仍以 Plan126 的最终武器挂点为原点且不发生位置漂移。长剑按向外斩击半径适配，镰刀按 360 度刀光平面半径适配。

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
- [x] 缩放只作用于本次生成的 Niagara Component，不缩放或移动 `WeaponAttackVfxRoot`、武器 Actor、武器贴图或 Slot Offset。
- [x] 枪、弓、DamageApplied、Travel、Boss VFX、近战斩弹和手动/自动事件出口保持回归通过。
- [x] 修改源码完成格式化；聚焦自动化、Development `-FullRebuild`、项目校验、预构建检查和 `git diff --check` 通过。
- [ ] 用户在 PIE 中确认无符文、长剑广域符文及镰刀群攻成长情况下，刀光外缘与实际攻击范围观感匹配且中心不漂移。
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

### 证据

- 规划静态审计确认：静态符文在 AttackStep 编译期修改 `RangeCm`，群攻成长在 WeaponActor 执行前叠加临时倍率；当前 `AttackCommitted` 不携带范围，且镰刀延迟刀光只锁定方向，因此表现无法匹配最终范围。
- Development 增量构建通过。Editor Python 完成两份 DA 保存：长剑 `AttackCommitted` 使用局部 Y 遮罩，镰刀使用局部 XY 遮罩，均配置 `0.5..2.0` 表现钳制；SDK `MainVersion` 警告后 Win64 Editor 仍继续执行，证据来自完整 session 日志中的两条 `PLAN148_MELEE_RANGE_SCALE`。
- `ReEcho.Presentation.VFX.Catalog`、`ReEcho.Presentation.Combat.Capabilities`、`ReEcho.Weapons.Runtime.MeleeProjectileCutEligibility` 各发现 1 项且 `Result={Success}`；分别覆盖范围轴/钳制、现有武器挂点与枪弓能力、长剑镰刀斩弹回归。
- 最终 Development `-FullRebuild` 100/100 成功，精选包 Build ID `55116800`、source fingerprint `e0ef7b5141d7`；`validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过。

### 剩余风险

- 长剑/镰刀 Niagara 的真实局部径向轴必须通过 Editor/PIE 验证；不能只根据组件 X/Y/Z 名称认定。
- 长剑 `120°/180°` 弧度的精确视觉匹配不属于普通缩放能力，本 Plan 不声称解决动态弧度形变。

### 人工验收结果/请求

- `PendingBeforeClose`：实现和客观门禁完成后，请用户在 PIE 确认刀光与实际范围观感。

### 架构文档审阅结果

- `MOD-ReEcho.md` 已更新：记录最终 Commit 范围快照与只缩放刀光组件的主模块适配。
- `MOD-ReEchoCombat.md` 已更新：记录资源无关 AttackCommitted 几何快照契约。
- `MOD-ReEchoPresentation.md`、`MOD-ReEchoVFX.md` 已更新：记录 DA 轴遮罩、延迟快照和迁移默认值。
- `MOD-ReEchoWeapons.md` 已审阅、无需修改：基础/有效范围和 AttackStep 编译权威未改变，改动位于主模块宿主与 Combat 事件。
- `ARCHITECTURE.md` 已审阅、无需修改：模块拓扑和依赖方向不变。
- `README.md` 已审阅、无需修改：稳定模块及 AREA 路由不变。
