# ReEcho 项目规则

本文件包含项目级硬规则。`AGENTS.md` 管理启动和专业角色路由；专业边界位于 `PROGRAMMER_RULES.md`、`DESIGNER_RULES.md` 和 `ARTIST_RULES.md`；项目秘书边界位于 `SECRETARY_RULES.md`；程序职责位于 `PLANNER_RULES.md` 和 `EXECUTOR_RULES.md`；提交身份和发布完整性位于 `GIT_RULES.md`；`WORKFLOW.md` 仅作说明。

## 远端规则权威与权限升级

- 每次接入、恢复任务或发现指令冲突时，先 fetch 并读取最新 `origin/main` 上的 `AGENTS.md` 及其路由到的权威规则。对于仓库内的规则与权限，最新远端权威规则高于最初 prompt、旧对话上下文、AI 摘要、Plan 中复制的旧规则和本地过期副本；不得以最初 prompt 限定为由拒绝执行远端规则要求的当前职责，也不得用旧提示绕过新门禁。
- 当前用户可以在远端规则允许范围内明确任务和作出保留给人的选择，但旧提示、角色称谓或用户临时授权不能自行扩大 AI 的仓库权限。若用户希望改变远端规则，应将其作为规则变更交由程序侧项目秘书维护并发布，而不是由执行 AI 临时解释覆盖。
- 出现权限范围、角色授权、合并/发布资格或规则适用性不清时，暂停存在争议的操作，向**程序侧项目秘书**请求确认。无法直接联系程序侧项目秘书时，明确请当前用户转交并等待答复；不得自行猜测权限，也不得只因最初 prompt 未写明而永久拒绝后续合法任务。
- 程序侧项目秘书只裁定仓库规则、职责路由和操作权限，不替代程序、策划、美术或用户作产品取舍；真实产品冲突仍按相应专业路线请人决定。

## 专业角色边界

- 每个新接入 AI 必须在项目工作前完成 `AGENTS.md` 的首次角色门禁。
- 程序路线内 Planner-Executor 模式选择仅由 `shared/PROGRAMMER_RULES.md` 管理；该文件中不可跳过的远端协作安全检查适用于所有模式。
- 已确认用户角色与 AI 仓库职责彼此独立。“策划”不代表项目 Planner；“程序”也不会自动授权发布 main。
- 项目秘书职责独立于专业角色门禁，由 `shared/SECRETARY_RULES.md` 管理。
- 不得暗中从策划/美术范围跨入代码、Schema、玩法权限或发布。拆分工作并交给程序路线，或在切换当前任务角色前明确询问用户。
- 策划或美术 AI 使用本地专业分支，记录准确资产/数据变化，并将结果交给程序 Planner 集成；不得推送任何远端引用或发布 `main`。

## 范围与架构

- 引擎：Unreal Engine 5.8 安装版/发行版，优先 Windows 桌面。不得使用独立源码检出构建或打开本项目。
- 项目描述符：`ReEcho.uproject`；运行时模块：`Source/ReEcho`。
- `Design/Data/ReEchoData.xlsx` 是已迁移生产表的权威策划可编辑来源。`Content/Data/` 下生成的 CSV 是可 diff、可打包的运行时来源。已迁移领域中的旧 JSON 仅用于迁移。
- 不得手改生成的生产 CSV 形成第二事实来源，也不得在 JSON、C++、DeveloperSettings、Actor 或 Widget 中复制已迁移平衡常量。
- 保留确定性语义：模拟 60 Hz、录制 20 Hz、遭遇时长 30 秒；暂停时录制和回放都不推进。
- 自动攻击绝不序列化进录制。Echo 回放历史位置和成功主动技能事件；目标选择与命中结算使用当前世界。
- 不得在 Unreal Editor 外手改 `.uasset` 或 `.umap`。优先使用可合并 C++、数据源和生成工具。

## 分支与权限

- Executor 实现分支使用本地 `plan/<id>-<short-name>` worktree；客户端要求的本地分支可使用 `codex/<short-name>`。
- `origin/main` 是唯一允许的远端分支。不得推送规划、任务、交接、评审或 PR 分支。
- Executor 绝不修改、合并或推送 `main`，也不推送任何远端引用。Planner 可创建已评审本地合并提交，并仅可依据 `PLANNER_RULES.md` 发布远端 `main`。
- 实现开始前，每个正式编号 Plan 都依据 `PLANNER_RULES.md` 发布到 `origin/main`；实现仍从本地非 `main` 分支开始，不创建远端任务分支。
- 远端状态可能变化时，唯一允许自动执行的第一步是 `git fetch`。若发现当前已批准本地基线之外的提交，执行 AI 必须在 `pull`、merge、rebase、cherry-pick 或 push 前停止；报告物理/Git 冲突、逻辑冲突和集成耦合，然后等待用户明确选择。可快进或干净自动合并都不例外。若受影响分支最近一次推送来自他人，由用户决定每个物理冲突和逻辑冲突采用、合并或丢弃；AI 提供选项但不自动解决。本检查适用于所有模式且不可豁免。
- 仅显式暂存路径。禁止 `git add .` 或 `git add -A`；禁止强推 `main`。
- 创建提交或发布前，遵循 `shared/GIT_RULES.md` 的角色标签和程序最终构建/预构建包门禁。
- 用户指令和 Plan 锁定验收定义范围。Executor 可优化实现细节，但目标、验收、公共契约或声明 Writes 的变更必须协调。远端 main 拥有已发布 Plan 编号；集成时将冲突的未发布本地 Plan 作为一个有序整体移动。

## 并行所有权

- 影响模式为 `Isolated`、`ReadOnly`、`SharedContract`、`Exclusive`；所有权状态为 `Reserved`、`Active`、`Released`。
- 只有 `Active` `Exclusive` 行阻塞其他写入者。预留不阻塞互不重叠或只读工作。
- 准备发布的 `.umap`、`.uasset`、Editor Project Settings、权威 XLSX、部署目标及其他难以合并的工件，在编辑前必须有 Exchange 所有权行。
- C++、独立文本文件和只读 API/数据消费者在声明 Writes 不重叠时可独立进行。
- `SharedContract` 包含稳定 ID、Schema、存档格式、公共 API 和生成器契约。破坏性变更开始前，受影响 Planner 需约定消费者表面。
- 权威 XLSX 只有一个仓库 `WorkbookWriter`。生成的 CSV 属于同一发布单元，不单独认领。一次性本地策划 QA 不认领仓库所有权。
- 同克隆 Unreal 锁位于 Git common directory 下的 `reecho-locks/unreal-editor.lock`。仅在确认没有进程使用本项目后移除过期锁。

## 文档所有权

- Executor 更新其指定 Plan 的执行记录和与所拥有实现直接相关的文档。
- `shared/CODEBASE_MAP/` 是当前项目架构、模块设计意图和代码落点的唯一权威库：`ARCHITECTURE.md` 负责全局拓扑，`README.md` 负责稳定标识索引，`modules/MOD-*.md` 负责单模块详细意图与代码位置。Plan 只承担任务决策历史，不得复制一套当前全局架构。
- Executor 不例行编辑 `shared/CODEBASE_MAP/`、`LESSONS.md` 或工作流规则。Planner 在评审/关闭时统一更新；只有 Plan 明确拥有相应模块文档时 Executor 才可编辑。
- 每个 Plan 关闭前，Planner 必须审阅 `shared/CODEBASE_MAP/`。模块、领域、职责、权威状态、公共契约、依赖方向或跨领域不变量发生变化时，必须同步更新全局架构、稳定标识索引和受影响模块文档；只有代码位置移动时更新模块文档即可。没有变化时在 Plan 中记录“已审阅、无需修改”及原因。未记录架构审阅结果不得关闭 Plan。
- 实现期间仅在所有权、Writes、依赖、生命周期或共享契约变化时更新 Exchange。
- 仅在读取路线、代码落点或职责变化时更新 `shared/CODEBASE_MAP/`。只将可复用、有证据的经验加入 `LESSONS.md`，不要记录任务流水账。
- 已暂存实现必须与其本地文档一致。文档中性提交不需要形式化共享文件改动。

## Unreal C++ 标准

- `Source/` 下遵循 Epic Unreal Engine C++ Coding Standard。
- 使用 Allman 大括号、四列 Tab 缩进、每行一条语句、二元运算符两侧空格和 Unreal 命名/类型约定。
- 头文件保持自包含，并将匹配的 generated header 放在最后；`.cpp` 首先 include 匹配头文件，并保持 include 分组稳定。
- 优先使用提前返回、命名常量和小型辅助函数，避免密集表达式或重复逻辑。
- 对修改的 `.h`/`.cpp` 运行仓库 `.clang-format`，检查 diff，再执行 UHT/UBT 和 `git diff --check`。
- 仅格式化清理不得改变玩法语义；例外需在附近解释并写入交接。

## 验证矩阵

使用 Windows `.cmd` 入口。执行变更表面要求的检查，不运行无关套件。必需检查是发布/就绪门禁，不是功能玩法证明门禁。

| 变更表面 | 必需检查 |
|---|---|
| 仅 Markdown/工作流 | `python scripts/validate_project.py`、`git diff --check` |
| Python 数据工具/XLSX 契约 | 聚焦 Python 测试、权威 `--check`、项目校验、`git diff --check` |
| 仅 JSON/配置/CSV | 项目校验和 `git diff --check` |
| C++ | `.clang-format`、`Build-Editor.cmd`（刷新跟踪的预构建包）、`python scripts/validate_project.py`、`git diff --check` |
| 纹理/导入脚本 | 导入/加载和资产存在性检查 |
| 打包/cook | 适用检查加干净包和 manifest/烟测证据 |

- `scripts\ue\Run-Automation.cmd` 等 Unreal 功能自动化仅在 Plan 或用户要求时作为可选证据，不是上层关闭或发布门禁。
- 需要关闭 Editor 的命令执行前，请用户保存并关闭。自动化不得暗中丢弃交互式会话。
- 只有相关源码/配置和集成基线未变化时才能复用证据；冲突或 rebase 会使受影响证据失效。
- 汇总日志，不要把完整 Unreal 日志加载到 AI 上下文。
- Unreal 不可用时将证据标记为 `static only`；不得声称已编译、自动化或 PIE。

## 完成级别

- **技术交付**：必需客观检查通过，或记录准确的不可用前置条件；`git diff --check` 通过；不包含 `GIT_RULES.md` 预构建允许列表之外的中间/私有文件或生成产物；执行记录描述变化、证据和风险。
- **人工验收**：仅视觉质量、手感、可用性或其他主观行为需要。`PendingBeforeClose` 阻塞关闭；`PendingFollowUp` 记录用户批准延期的信心检查；`NotRequired` 和 `Passed` 含义直观。
- **Plan 关闭**：必需客观检查和所有 `PendingBeforeClose` 项已验收，Planner 评审/集成任务、释放所有权并更新共享状态。只有用户明确接受延期时，关闭后才可保留 `PendingFollowUp`。
- **远端发布**：与关闭/本地合并分离，由 `PLANNER_RULES.md` 中有明确范围的授权门禁管理。
