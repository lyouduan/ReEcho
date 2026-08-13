# Plan XX - <专业> - <简短名称>

## 协调

- Planner 负责人：
- Executor 负责人：
- Plan 编写方（AI 侧）：`Gavyn-side AI | ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Unassigned | Gavyn-side AI | ReEcho teammate-side AI`。
- 任务状态：`Proposed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`NotRequired`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：
- 实现分支：
- 依赖 / 阻塞：
- Writes:
- Stable Reads:
- 影响模式：`Isolated`（`Isolated | ReadOnly | SharedContract | Exclusive`）。
- 兼容承诺 / 下游操作：
- 明确排除：

## 锁定目标

描述玩家可见结果或流水线结果。修改本节需 Planner/用户同意。

## 架构影响与设计决策

- 受影响架构标识：列出所有直接修改和受公共契约影响的 `MOD-*` / `AREA-*`；没有时写 `None` 并说明理由。
- 对应模块文档：逐项列出需要创建/维护的 `shared/CODEBASE_MAP/modules/MOD-*.md`，并确认已加入 `Writes`。
- 设计意图：为什么要做，以及希望建立或保护什么职责边界。
- 权威状态与依赖：本 Plan 是否改变状态所有者、公共契约、模块/目录职责或依赖方向。
- 决策记录：关键选择、考虑过的替代方案、选择原因和已知代价。
- 相关文档同步范围：逐项列出 `ARCHITECTURE.md`、`README.md` 和所有相关 `modules/MOD-*.md`；只有确实不相关的文件才可省略并说明原因。
- 关闭前逐项填写审阅结果：
  - `<文档>` 已更新：列出与验收实现一致的具体变化；
  - `<文档>` 已审阅、无需修改：说明该实现为何不改变此文档中的事实。

## 锁定验收

- [ ] 功能结果有可观察证据。
- [ ] 必需自动化/构建检查通过。
- [ ] 仅在手感、可读性、视觉质量或可用性需要判断时请求人工验收。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：
- 引擎/构建可用性：
- 现有聚焦测试结果：
- 活跃独占所有权或共享契约批准：
- 基线损坏时的停止条件：

## 实现提纲

在不修改锁定目标、验收或已发布契约的前提下，可以优化实现细节。

1. 检查最小相关代码/数据表面。
2. 每次完成一个连贯变更。
3. 每次风险变更后立即验证。
4. 更新执行记录；跨越变化边界前，在本地记录所有权/范围/契约变化。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | 项目/源码不变量通过 |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd` | UHT/UBT 退出码为 0 |
| 运行时行为变化时自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | 受影响报告通过 |
| 需要人工时 | 具名 PIE/可用性任务 | 记录人工结果或明确延期跟进 |

## 执行记录

### 变化

### 证据

### 剩余风险

### 人工验收结果/请求

### 架构文档审阅结果
