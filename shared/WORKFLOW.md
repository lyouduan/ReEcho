# ReEcho 多 AI 工作流

本文档解释协作模型，不是第二本规则书；冲突时以 `AGENTS.md` 路由到的权威规则为准。

## 权威映射

| 事项 | 权威文件 |
|---|---|
| 启动和读取顺序 | `AGENTS.md` |
| 项目约束、本地/远端边界和验证 | `shared/PROJECT_RULES.md` |
| 程序/策划/美术边界 | 对应专业规则 |
| 策划 Bug/需求登记、待合并修改、效果确认和经验 | `shared/DESIGNER_RULES.md`、`issues/TEMPLATE.md`、`shared/DESIGNER_EXPERIENCE/<策划身份>.md` |
| 程序接管 Issue、修复、交付策划测试和最终集成 | `shared/PROGRAMMER_RULES.md` |
| 项目秘书职责 | `shared/SECRETARY_RULES.md` |
| Plan、评审和发布 | `shared/PLANNER_RULES.md` |
| 具体实现 | `shared/EXECUTOR_RULES.md` |
| 提交身份和程序最终构建 | `shared/GIT_RULES.md` |
| Git LFS 检出、大文件边界和对象发布 | `shared/PROJECT_RULES.md`、`shared/GIT_RULES.md` |
| 单项任务规格和证据 | `plans/<id>-*.md` |

## 核心模型

本地工作自由，远端边界严格。

- 每个成员/AI 在自己的克隆中自行决定分支、worktree、stash、WIP 提交、merge/rebase 和并行方式；共享规则不追踪本地所有权。
- 程序路线分别确认 Planner-Executor 分工和本地工作区模式这两个独立答案，但在同一次首次确认中取得它们；两者仍互不推导。选择“一任务一 worktree”时，即使同一 AI 完成全流程，每个任务也使用独立文件夹。
- Plan 不是写锁，而是大任务的公开规格、设计决策和验收历史。所有大任务在实现开始前发布编号 Plan 到 `main`；小型修复、只读审计或明确豁免可直接处理。
- 远端 `main` 是唯一权威发布分支；程序路线和项目秘书发布时按 `GIT_RULES.md` 使用唯一临时 `main-publish-lock` 串行集成。策划按 `DESIGNER_RULES.md` 用 `issue/<策划身份>/<简述>` 登记 Bug、用 `request/<策划身份>/<简述>` 登记需求、用 `merge/<策划身份>/<简述>` 提交已验收修改；美术使用 `artist/<task>`。程序可按 `PROGRAMMER_RULES.md` 推荐流程接管已经存在的 Issue 分支：fetch Issue 和最新 main，把 `origin/main` merge 到 Issue 后修复并普通推送同一分支供策划测试，验收通过后再由程序侧按发布锁集成 main。这些协作分支不构成发布状态，也不存在 Exchange。
- 编号 Plan 在实现开始前发布到 `main`。远端 main 拥有编号；冲突时尚未发布的本地 Plan 以后到先得方式整体后移。
- `Writes`、`Stable Reads` 和 `Isolated | ReadOnly | SharedContract | Exclusive` 只描述影响面与集成风险，不授予或阻止本地写入。

## 跨机器集成

Fetch 是安全边界的第一步。有他人新提交时，在集成或发布前报告并审计：

1. 物理/Git 冲突；
2. 无文本冲突也可能存在的逻辑冲突；
3. 受影响 Plan、公共契约、测试/构建证据和集成顺序的耦合。

清晰、范围不变且符合已批准契约的集成可继续；AI 不因快进或干净 merge 忽略语义审计，也不替人决定真实逻辑冲突、产品取舍、不可逆覆盖或范围扩张。

## 推进节奏

默认能合并就合并、能推送就推送、能删除就删除，并鼓励小批量、较频繁地推送 main。“能”表示范围、验证、人工验收、远端审计、发布和清理门禁均满足；主观验收、产品取舍、外部提交语义选择和可能丢失工作的清理仍提醒并等待人决定。

程序路线每次推送 main 都先抢占 `main-publish-lock`，合入最新 main，并在最终候选上执行 FullRebuild、发布匹配精选预构建包；远端核验成功后才释放锁。本地 WIP 身份自由；进入远端的正式提交必须含已确认的人类账号 + AI 身份及且仅一个专业标签。

## 难合并资源

- `.gitattributes` 声明的 LFS 文件在任何角色打开 UE 或验证前都必须由 `scripts/setup_lfs.py` 还原；Git 指针进入远端不等于大文件对象已经完成发布。
- 蓝图数据总目录与受影响的领域 DataAsset 是一个发布单元。多个本地尝试可以并行，进入 main 前按当前远端权威资产审计并组合，禁止静默整块覆盖。
- Unreal Editor 锁只串行化同一 Git common directory 下的 Editor/命令，不是远端所有权。
- `.uasset`、`.umap` 和其他二进制资产可本地独立尝试；程序集成时必须明确比较路径和语义，必要时由用户选择保留哪一份。

该结构用更少的共享控制面换取本地速度，把真正不可自动决定的冲突集中到 pull/push 边界。
