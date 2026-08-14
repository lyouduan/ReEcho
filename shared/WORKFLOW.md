# ReEcho 多 AI 工作流

本文档解释协作模型，不是第二本规则书；冲突时以 `AGENTS.md` 路由到的权威规则为准。

## 权威映射

| 事项 | 权威文件 |
|---|---|
| 启动和读取顺序 | `AGENTS.md` |
| 项目约束、本地/远端边界和验证 | `shared/PROJECT_RULES.md` |
| 程序/策划/美术边界 | 对应专业规则 |
| 项目秘书职责 | `shared/SECRETARY_RULES.md` |
| Plan、评审和发布 | `shared/PLANNER_RULES.md` |
| 具体实现 | `shared/EXECUTOR_RULES.md` |
| 提交身份和程序最终构建 | `shared/GIT_RULES.md` |
| 单项任务规格和证据 | `plans/<id>-*.md` |

## 核心模型

本地工作自由，远端边界严格。

- 每个成员/AI 在自己的克隆中自行决定分支、worktree、stash、WIP 提交、merge/rebase 和并行方式；共享规则不追踪本地所有权。
- Plan 不是写锁，而是大任务的公开规格、设计决策和验收历史。所有大任务在实现开始前发布编号 Plan 到 `main`；小型修复、只读审计或明确豁免可直接处理。
- 远端仓库只有一个分支：`main`。不存在远端规划、任务、交接、评审或广播分支，也不存在 Exchange。
- 编号 Plan 在实现开始前发布到 `main`。远端 main 拥有编号；冲突时尚未发布的本地 Plan 以后到先得方式整体后移。
- `Writes`、`Stable Reads` 和 `Isolated | ReadOnly | SharedContract | Exclusive` 只描述影响面与集成风险，不授予或阻止本地写入。

## 跨机器集成

Fetch 是安全边界的第一步。没有他人新提交时，完成适用验证即可继续；有他人新提交时，在 pull、merge、rebase、cherry-pick 或 push 前报告：

1. 物理/Git 冲突；
2. 无文本冲突也可能存在的逻辑冲突；
3. 受影响 Plan、公共契约、测试/构建证据和集成顺序的耦合。

用户选择采用远端、采用本地、组合适配或延期/拆分。AI 不因快进或干净 merge 自动替人决定语义。

## 推进节奏

默认能合并就合并、能推送就推送、能删除就删除，并鼓励小批量、较频繁地推送 main。“能”表示范围、验证、人工验收、远端审计、发布和清理门禁均满足；主观验收、产品取舍、外部提交语义选择和可能丢失工作的清理仍提醒并等待人决定。

程序路线每次推送 main 都在最新最终候选上执行 FullRebuild 并发布匹配精选预构建包。本地 WIP 身份自由；进入远端的正式提交必须含已确认的人类账号 + AI 身份及且仅一个专业标签。

## 难合并资源

- 权威 `Design/Data/ReEchoData.xlsx` 与生成 CSV 是一个发布单元。多个本地尝试可以并行，进入 main 前按当前远端权威表审计并组合，禁止静默整块覆盖。
- Unreal Editor 锁只串行化同一 Git common directory 下的 Editor/命令，不是远端所有权。
- `.uasset`、`.umap` 和其他二进制资产可本地独立尝试；程序集成时必须明确比较路径和语义，必要时由用户选择保留哪一份。

该结构用更少的共享控制面换取本地速度，把真正不可自动决定的冲突集中到 pull/push 边界。
