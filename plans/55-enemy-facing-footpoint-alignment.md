# Plan 55 - 程序 - 怪物逻辑朝向与 Flipbook 脚点对齐

## 协调

- Planner 负责人：Codex（lyouduan / Gavyn-side AI）。
- Executor 负责人：Codex（lyouduan / Gavyn-side AI）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@5e66e6b`。
- 本地实现方式：当前本地 `main` 直接实施；不创建 worktree。
- 依赖 / 阻塞：Plan52 场景改造仍为本地 WIP；本 Plan 不修改地图资产，但共用最终 Editor 构建与 PIE 验收窗口。
- Writes：本 Plan；`AReEchoEnemyActor`、`UReEchoEnemyPresentationComponent`、必要的 2D Presentation Profile/Controller 公共接口；相关聚焦测试；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与匹配表现模块文档；最终精选 Win64 Editor 预构建包。
- Stable Reads：EnemyLogic 的 `FacingDirection`、Combat/VFX 显式 Direction、现有 Animation2D Flipbook/Profile 契约、保存结构字段。
- 影响模式：`SharedContract`（EnemyHost 逻辑朝向、存档兼容、表现组件层级和脚点坐标契约）。
- 兼容承诺 / 下游操作：保留 EnemyLogic、伤害、方向盾、攻击/投射物/VFX 显式方向、现有保存字段和角色 Blueprint；旧存档 Transform Rotation 仅可用于一次性推导缺失逻辑朝向，不再恢复到 Actor。
- 明确排除：玩家/Echo Actor 朝向重构；修改战斗数值、敌人行为选择、Flipbook 原图/Pivot；逐帧 Bounds 驱动；让表现层成为玩法朝向权威。

## 锁定目标

怪物 Actor 永远保持 Identity Rotation；EnemyLogic `FacingDirection` 是唯一玩法朝向，角色表现只使用相机面片和左右镜像。`FootRoot` 定义稳定脚点原点，`GroundRoot` 直接挂在 `FootRoot`；`PresentationMotionRoot` 根据当前 Flipbook 的联合 RenderBounds、Renderer/Root Transform、世界高度缩放与相机面片角度计算基础偏移，使渲染内容底边中心对齐 `FootRoot` 原点，并在其上独立叠加受击/攻击临时位移。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`AREA-Presentation`；EnemyLogic/Combat/VFX 只消费稳定显式方向契约。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；审阅匹配 Presentation 模块说明并按实际结果更新。
- 设计意图：彻底分离玩法朝向、相机面片朝向和脚点坐标，消除 Actor Yaw 导致的阴影旋转/公转及不同尺寸、Pivot、相机角度下的角色跳脚。
- 权威状态与依赖：EnemyLogic Snapshot 拥有 `FacingDirection`；Actor Transform 不再编码朝向；Profile 可提供自动脚点开关与美术微调，但自动结果来自完整 Flipbook RenderBounds。
- 决策记录：使用完整 `FTransform` 把 Flipbook 局部底边中心转换到 FootRoot 空间，不手写仅适用于固定俯角的三角公式；使用整个 Flipbook 的联合 Bounds，禁止逐帧 Bounds 引入抖动。
- 相关文档同步范围：关闭前审阅 `ARCHITECTURE.md`、`README.md` 和匹配模块文档；拓扑不变时记录无需修改理由。

## 锁定验收

- [ ] 怪物转向、移动、攻击、受击和保存恢复期间 Actor Rotation 始终为 Identity；代码中不再以 Actor Forward/Rotation 表达怪物逻辑朝向。
- [ ] 方向盾、Boss 传送回退、攻击/投射物/VFX 和受击回退继续使用显式 `FacingDirection`，行为无回归。
- [ ] FlipbookRoot 始终朝向相机，左右朝向只改变 FacingSign。
- [ ] `GroundRoot` 直接挂在 `FootRoot`，阴影不继承 PresentationMotion、相机面片或 Actor 朝向旋转。
- [ ] Idle/Move/Attack/Hit 切换、左右镜像、不同角色尺寸/Pivot和至少两种相机俯角下，Flipbook 底边中心均对齐 FootRoot 原点且不逐帧跳动。
- [ ] 自动对齐与美术 FootpointOffset 分离；临时表现位移不会覆盖基础对齐结果。
- [ ] 旧存档兼容、聚焦自动化、完整 Editor 构建、`validate_project.py` 与 `git diff --check` 通过。
- [ ] 用户 PIE 验证怪物转向时阴影稳定、脚底不漂移。

## Step 0 门禁

- 基线分支/提交：`origin/main@cf9a21a`；远端最大编号 54，本 Plan 使用 55。
- 引擎/构建可用性：同步后 UE 5.8 Editor Development FullRebuild 已通过。
- 现有聚焦测试结果：现有层级测试反而要求 GroundRoot 位于 PresentationMotionRoot 且非绝对旋转，需替换为新契约；缺少 Actor Identity Rotation 与变换后底点对齐覆盖。
- 共享契约 / 难合并资源风险：`BP_EnemyGameplay_*` 为二进制且当前存在用户 WIP；优先用原生默认子对象层级迁移和 C++ 测试，不主动重保存无关 Blueprint。
- 基线损坏时的停止条件：无法在不改变 EnemyLogic/Combat 权威方向或破坏旧存档字段的前提下移除 Actor Rotation，或 RenderBounds 无法稳定取得时停止并报告。

## 实现提纲

1. 删除 Enemy Actor 朝向旋转；将 Actor Forward 消费者迁移到规范化逻辑 FacingDirection，并对保存/恢复 Rotation 做兼容清理。
2. 将 GroundRoot 改挂 FootRoot，更新运行时层级修复和组件树测试。
3. 新增稳定脚点对齐计算：当前 Flipbook 联合 Bounds 的底边中心经 Renderer、FlipbookRoot 相机旋转和缩放转换到 FootRoot 空间，取相反位移作为基础 Motion Offset。
4. 将 authored、calculated alignment、transient motion 三类偏移分离；在 Flipbook/Profile/相机角度/缩放变化时刷新，禁止逐帧帧包围盒抖动。
5. 补充方向盾、存档、Actor Identity、层级、不同 Bounds/角度/镜像和动画切换测试，完成构建、自动化与 PIE 验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 格式/静态 | `.clang-format`、`python scripts/validate_project.py`、`git diff --check` | 源码与项目不变量通过 |
| C++ | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新准确预构建包 |
| 自动化 | 聚焦 `ReEcho.Enemies`、`ReEcho.Presentation.Animation2D`、保存/方向盾相关测试 | 逻辑方向、Identity Rotation、脚点和阴影契约通过 |
| 人工 | PIE 连续绕怪物移动并触发 Idle/Move/Attack/Hit | 仅左右翻转；阴影不旋转/公转；脚底无漂移 |

## 执行记录

### 变化

### 证据

### 剩余风险

### 人工验收结果/请求

- `PendingBeforeClose`。

### 架构文档审阅结果
