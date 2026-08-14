# ReEcho 多 AI 工作流

本文档解释协作模型，不是第二本规则书。

## 权威映射

| 事项 | 权威文件 |
|---|---|
| 启动和读取顺序 | `AGENTS.md` |
| 项目约束、所有权、分支和验证 | `shared/PROJECT_RULES.md` |
| 风险提醒、问程序建议和人工确认后执行 | `shared/PROJECT_RULES.md` |
| 程序/策划/美术边界 | `shared/PROGRAMMER_RULES.md`、`shared/DESIGNER_RULES.md`、`shared/ARTIST_RULES.md` |
| 项目秘书协调职责 | `shared/SECRETARY_RULES.md` |
| Planner 操作 | `shared/PLANNER_RULES.md` |
| Executor 操作 | `shared/EXECUTOR_RULES.md` |
| 当前本地工作、所有权和警告 | `shared/PLANNER_EXCHANGE.md` |
| 单项任务范围和证据 | `plans/<id>-*.md` |

若本文解释与上述权威文件不同，以上述权威文件为准。工作流变更与仓库中其他变更一样需要评审。

## 角色和生命周期

- 首次接入选择用户专业路线：程序、策划或美术；这与 AI 的仓库职责不同。
- 用户决定方向和主观验收，并选择如何集成外部 main 差异。
- 明确指派后，项目秘书维护协作控制面一致性，但不接管产品决策或专业实现。
- Planner 定义范围、评审交付、集成已验收本地工作，并且是唯一可发布 `main` 的职责。
- Executor 默认在隔离的本地分支/worktree 中实现、记录证据并交回本地分支，不推送远端引用；风险例外遵循 `PROJECT_RULES.md` 的人工确认机制。
- 策划和美术 AI 作为本地专业执行者，将数据/资产和证据交给程序 Planner，而不自行发布远端状态。

任务生命周期为 `Proposed -> Ready -> InProgress -> Review -> Closed`；`Blocked` 仅用于例外情况，并需写明解除条件。人工验收单独记录为 `NotRequired`、`PendingBeforeClose`、`PendingFollowUp` 或 `Passed`。

## 本地协调契约

每个 Plan 声明负责人、生命周期、人工验收、本地/实现基线、依赖、准确 Writes/Reads、影响模式、兼容承诺和排除项。所有权状态为 `Reserved`、`Active` 或 `Released`。

- `Isolated`：文件互不重叠，且不修改共享契约。
- `ReadOnly`：使用稳定表面，不编辑提供者拥有的文件。
- `SharedContract`：修改 API、稳定 ID、Schema、存档格式或生成器契约；受影响本地工作在跨越边界前达成一致。
- `Exclusive`：编辑难以合并的工件或外部目标；只能有一个活跃写入者。

预留只声明本地意图，不阻塞无关工作。只有 `Active Exclusive` 行会阻塞另一写入者。范围扩张需在跨越边界前记录到本地。

## 本地并行与跨机器集成

同机 Executor 使用独立 worktree 和本地分支；不同成员使用独立克隆和本地分支。每个克隆内的安全并行由文件所有权和契约边界决定。

远端仓库只有一个分支：`main`。不存在远端规划、任务、交接或评审分支。编号 Plan 在实现开始前发布到 `main`，使每台机器无需侧分支即可看到权威编号和声明范围。

这会在执行前公开编号意图，同时让实现保持本地。成员从已发布 Plan 了解范围，通过 fetch `origin/main` 获取已验收实现。pull 或 push 前，Planner 报告物理/Git 冲突、逻辑冲突、耦合及任何 Plan 编号冲突；用户选择远端、本地、组合适配或延后。

远端 main 拥有 Plan 编号。编号前 Planner fetch 并读取远端最大 Plan 编号；编号后将 Plan 推送到 main 并在执行前核验。若并发发布仍冲突，将冲突 Plan 及其后所有未发布本地 Plan 整体移到首个空闲连续编号区间，并更新实时引用。

提供者/消费者工作在一个克隆内达成本地契约后可并行。跨机器消费者不能依赖未发布的提供者工作；提供者必须先验收进入 main，消费者再 fetch/审计 main 并集成。

共享文档不进入 Executor 热路径：Executor 更新其 Plan 和实现本地文档；Planner 在评审时更新共享状态/路线/经验。实现期间 Exchange 仅记录本地所有权、范围、依赖或契约变化。

## 权威工作簿与独占资源

`Design/Data/ReEchoData.xlsx` 只有一个活跃仓库写入者（`WorkbookWriter`）；生成的生产 CSV 属于同一发布单元。只读消费者和一次性本地 QA 可并行；可发布 QA 改动交给当前写入者。

Unreal Editor 锁是机器本地的，但通过 Git common directory 由一个克隆中的所有 worktree 共享。它只串行化确实需要独占 Editor/项目访问的操作；命令位于 `EXECUTOR_RULES.md`。

## 本地与远端通道

1. **规划已发布，实现留本地**：编号 Plan 文件及必要实时协调先进入 `main`；任务分支、worktree、实现提交和交接默认保留本地，不作为侧引用推送。
2. **本地验收**：Planner 评审 Executor 提交、解决人工验收，并将已验收范围合入本地 main。
3. **远端 main 集成**：fetch、审计外部 main、与用户解决 Plan 编号和行为差异、验证结果候选，然后仅非强制推送 main。

常设的仅 Plan 授权只覆盖编号 Plan 及必要实时协调，默认不覆盖实现。其他 main 发布仍需单候选授权、有明确范围的常设授权，或按 `PROJECT_RULES.md` 对准确风险操作取得人工确认。

## 采用该结构的原因

- 单一远端分支消除过期协调引用和重复分支清理。
- 本地 worktree 保留同机并行能力，不向远端暴露 WIP。
- 外部 main 审计在 pull/push 前暴露语义耦合，而不是假设 Git 可干净合并就足够。
- 远端权威编号解决独立 Planner 冲突，无需重写已发布历史。
- 代价是明确的：不同机器不能通过 Git 远端引用消费彼此未发布的契约或评审 WIP。
