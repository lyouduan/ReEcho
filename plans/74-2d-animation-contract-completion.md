# Plan 74 - 程序 - 角色与怪物 2D 动画契约补全

## 协调

- Planner 负责人：JosephLE910（程序）
- Executor 负责人：JosephLE910（程序）
- Plan / 实现编写方（AI 侧）：`JosephLE910-side AI | Codex`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main @ 887722b176867d682976e81aafb42860861d921c`。
- 本地实现方式：`C:\Users\binnanliang\Documents\ReEcho-worktrees\2d-animation-redesign`，分支 `codex/2d-animation-redesign`。
- 依赖 / 阻塞：复用 Plan65 已发布的敌人阶段事件、`Animation.Attack.Charge`、`Animation.Transform.Phase2` 与 AnimationSet 接口；生产动画资产修改必须关闭 Unreal Editor 并通过 Editor API 保存。
- Writes：
  - `Source/ReEchoPresentation/{Public,Private}/Presentation/Animation2D/**`
  - `Source/ReEcho/{Public,Private}/Presentation/Enemy/ReEchoEnemyPresentationComponent.*`
  - `Source/ReEcho/Private/Tests/ReEcho2DAnimationTests.cpp`
  - `scripts/ue/` 下本 Plan 新增或维护的动画 Profile/FSM 审计与配置脚本
  - `Content/ReEcho/Animation2D/SM2D_DefaultCharacter.uasset`
  - `Content/ReEcho/Animation2D/DA_Enemy_Fox.uasset`
  - `Content/ReEcho/Animation2D/DA_Enemy_TimeGuard.uasset`
  - 必须为生产覆盖一致性而修改的其他 `Content/ReEcho/Animation2D/DA_Character_*.uasset`、`DA_Echo_*.uasset`、`DA_Enemy_*.uasset`（逐项在执行记录列出，不改源画风格）
  - 已有 `Content/ReEcho/Art/Animation2D/**` Flipbook 只读复用；若必须修复资产循环/帧率等元数据，先记录准确资产并通过 Editor 保存
  - `docs/2D_SEQUENCE_ANIMATION.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `plans/74-2d-animation-contract-completion.md`
  - 程序发布门禁刷新出的 `Binaries/Win64/ReEchoEditor.prebuilt.json` 与 manifest 精选 Editor 产物
- Stable Reads：
  - `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyEventsComponent.h`
  - `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyTypes.h`
  - `Source/ReEchoEnemies/Private/Enemies/ReEchoEnemyLogicComponent.cpp`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*`
  - `Content/ReEcho/Art/Animation2D/**`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`
- 影响模式：`SharedContract + Exclusive`。表现语义/FSM 是共享契约；Profile、FSM 与 Flipbook 是不可文本合并的二进制资产。
- 兼容承诺 / 下游操作：玩法位置、碰撞、伤害、攻击提交、阶段计时、死亡和存档权威不变；缺失动画必须安全回退并报告，不能影响玩法推进。
- 明确排除：不制作或 AI 重绘新美术帧，不修改敌人攻击数值/阶段触发条件，不让动画通知或播放完成反向驱动玩法，不修改根 Box Collision、录制或回放语义，不处理无关 VFX/音频。

## 锁定目标

1. 角色、Echo、普通怪和 Boss 使用同一套可审计的 2D 动画语义契约，生产 Profile 不再以静默缺项表示“暂未支持”。
2. 狐狸在特殊攻击 `WindupStarted` 时播放基础形态对应的蓄力动画，在 `ActionCommitted` 时播放攻击/突进动画，结束后回到当前基础状态。
3. Boss/TimeGuard 在阶段开始时播放不可被普通攻击覆盖的变身动画，阶段完成后稳定进入 Phase2 AnimationSet；不得闪现一次阶段基础帧后才变身。
4. `Attack.Charge`、`Attack.Basic`、`Transform.Phase2`、`Hit`、`Death` 具有明确的循环、抢占、锁定和完成归宿；动画结束只改变表现。
5. 提供生产资产覆盖审计，能列出每个 Profile 在 Base/Phase2 下的 Idle、Move、Charge、Basic、Hit、Transform、Death 状态、Flipbook、循环策略和回退结果。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoPresentation`、`AREA-Presentation`、`MOD-ReEcho`；`MOD-ReEchoEnemies` 为事件生产者 Stable Read，除非发现现有 Started/Committed/Ended/PhaseTransition 契约无法表达取消，否则不修改。
- 对应模块文档：`MOD-ReEchoPresentation.md` 与 `MOD-ReEcho.md` 已加入 Writes；`MOD-ReEchoEnemies.md` 仅审阅事件契约，默认不修改。
- 设计意图：维持 `玩法事件 -> Host Adapter -> 语义命令 -> FSM/Profile -> Flipbook` 单向链，将动画覆盖和转换策略集中在 Presentation，而不是按 Fox/Boss 在玩法代码硬编码资源路径。
- 权威状态与依赖：Enemies/Combat 继续拥有玩法时序与结果；Presentation 只拥有当前语义、AnimationSet、播放状态和可见回退。不新增模块反向依赖。
- 决策记录：
  - 扩充现有 GameplayTag/FSM/Profile，不引入第二套枚举状态机。
  - `Charge` 可以循环直到玩法提交/结束；`Basic` 与 `Transform` 为一次性；`Transform` 的抢占级别高于 Hit/Attack，Death 为终结状态。
  - 狐狸基础形态和 Phase2 都显式绑定 Charge/Basic；Boss Phase2 不以普通 Idle 冒充正式 Transform。若仓库没有独立正式 Transform 帧，保留具名回退并将视觉替换列为人工验收阻塞，不伪造美术。
  - 自动化验证生产 DataAsset，而不是只构造内存 Profile 证明 Controller API。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：审阅模块拓扑；预计无需修改，因为依赖方向不变。
  - `shared/CODEBASE_MAP/README.md`：审阅稳定路由；预计无需修改，因为标识与代码根不变。
  - `MOD-ReEchoPresentation.md`：更新语义状态、回退、覆盖审计和测试契约。
  - `MOD-ReEcho.md`：更新 `AREA-Presentation` 的生产 FSM 与特殊动作/变身链。
  - `MOD-ReEchoEnemies.md`：审阅事件生产契约；无契约变化时记录无需修改。
- 关闭前逐项填写审阅结果。

## 锁定验收

- [ ] 默认 FSM 正式包含 Idle、Move、Attack.Charge、Attack.Basic、Hit、Transform.Phase2、Death，并锁定明确优先级/完成归宿。
- [ ] 生产狐狸 Base 与 Phase2 Profile 均解析到可加载的 Charge/Basic Clip；Charge 循环，Basic 单次且可重播。
- [ ] 生产 Boss/TimeGuard Profile 可解析 Transform 与 Phase2 基础/攻击 Clip；阶段切换期间 Transform 不被普通攻击/Hit 覆盖。
- [ ] 所有生产玩家、Echo、怪物 Profile 通过覆盖审计；缺失项按允许回退表报告，不静默失败。
- [ ] 自动化覆盖生产狐狸 `Charge -> Basic -> Base`、生产 Boss `Base -> Transform -> Phase2`、抢占/取消和缺失回退。
- [ ] `.clang-format`、`Build-Editor.cmd -Configuration Development`、聚焦自动化、`python scripts/validate_project.py` 与 `git diff --check` 通过。
- [ ] 用户在 PIE 验收狐狸蓄力、Boss 变身、脚点/比例/阴影/朝向/首帧闪烁后，人工验收方可设为 `Passed`。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main @ 887722b176867d682976e81aafb42860861d921c`；远端最大 Plan 编号 73，因此本任务使用 74。
- 引擎/构建可用性：UE 5.8 安装版；当前检查无 UnrealEditor/UnrealEditor-Cmd 进程、无共享 Unreal 锁。Plan 和实现每次发布前执行 `-FullRebuild`。
- 现有聚焦测试结果：Plan65 记录 Enemies Logic 通过；当前生产 Profile 只自动验证 Fox Idle/Basic，Phase2 Controller 测试使用内存 Profile，缺少生产 Charge/Transform 证据。
- 共享契约 / 难合并资源风险：`SM2D_DefaultCharacter.uasset` 与生产 Profile 为 Exclusive；进入远端前重新 fetch，若外部提交修改相同资产则停止并审计。
- 基线损坏时的停止条件：基线构建、现有 `ReEcho.Presentation.Animation2D` 或项目校验失败时，先分离基线问题，不以削弱测试或覆盖资产绕过。

## 实现提纲

1. 增加生产 Profile/FSM 覆盖审计与聚焦测试，先得到当前缺口清单。
2. 扩充 FSM 状态定义、Controller 抢占/完成/取消语义，并保持玩法单向依赖。
3. 通过 Unreal Editor API 配置狐狸 Base/Phase2 Charge/Basic 与 Boss Transform/Phase2 Profile；不在 Editor 外修改 `.uasset`。
4. 补生产资产自动化、缺失回退诊断和文档；逐项核对角色、Echo 与全部生产怪物。
5. 完成构建、聚焦自动化、静态验证和发布审计，再请求用户 PIE 人工验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 资产审计 | 本 Plan 的 Editor 审计脚本 | 每个生产 Profile/AnimationSet 输出语义、资产、循环与回退矩阵，退出码 0 |
| 表现自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.Animation2D` | 生产狐狸/Boss、FSM 抢占、完成归宿与回退通过 |
| C++ | `.clang-format`、`scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码 0 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目与源码不变量通过 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`、prebuilt 校验、最终 fetch | 最终候选与精选 Editor 包匹配且无新增外部提交 |
| 人工 | PIE：狐狸蓄力/突进，Boss 变身/Phase2，角色与怪物回归 | 用户确认视觉、脚点、比例、阴影、朝向和无首帧闪烁 |

## 执行记录

### 变化

### 证据

### 剩余风险

- 正式美术帧若不存在，本 Plan 不生成替代画面；对应 Profile 只能具名回退并等待美术资产。

### 人工验收结果/请求

- `PendingBeforeClose`：狐狸蓄力、Boss 变身和 Phase2 连续性需要用户 PIE 验收。

### 架构文档审阅结果

