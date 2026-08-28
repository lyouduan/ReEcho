# ReEcho 程序用户路线

本文件在用户确认“程序”后适用。它定义专业边界；`PLANNER_RULES.md` 和 `EXECUTOR_RULES.md` 定义可选职责分工，`PROJECT_RULES.md` 定义所有模式都必须遵守的本地/远端边界。

本文件中的风险性禁止项和例外统一遵循 `shared/PROJECT_RULES.md` 的“风险操作的人类确认与执行”：先提醒风险并提出是否建议问程序，获得有权限的人对准确操作的确认后执行。

## Planner-Executor 模式

本节是程序路线选择 Planner-Executor 模式的唯一权威。

- 仅当当前用户在当前对话中确认采用时，才把规划和执行交给不同职责；不得从账号、Git 历史或其他对话推断。未采用时，同一个 AI 可以同时规划、实现、评审和集成。
- 两种模式使用同一套 Plan 和发布门禁。Planner-Executor 只是分工方式，不是权限系统，也不限制各自如何组织本地分支、worktree、提交或合并。
- Planner 负责规划、评审、集成和发布决策；Executor 负责按已发布 Plan 实现并回报证据。由同一个 AI 工作时，仍须在大任务实现前完成同样的正式 Plan 发布。
- Planner-Executor 模式不决定本地文件夹组织；本地隔离方式是独立选择，但两项应按 `AGENTS.md` 在同一次首次确认中询问。

## 本地工作区模式

本节是程序路线选择“一任务一 worktree”的唯一权威。该选择与 Planner-Executor 分工彼此独立。

- `AGENTS.md` 的一次性首次确认必须同时取得 Planner-Executor 模式和本地工作区模式。若用户只回答了其中一项，一次性询问所有仍缺失的程序选项；任何适用选项未得到答案前停止程序工作。当前对话已经明确的选项不重复询问，也不得根据另一选项、当前目录、分支或其他对话推断。
- 选择“是”后，每个新任务在首次写入前都必须从批准基线创建或复用该任务专属的本地 worktree/独立文件夹。不得直接在主工作区实现，不得让两个无关任务共用一个 worktree；同一任务的后续轮次可继续复用其原 worktree。
- 选择“否”后，可以在当前工作区直接工作，也可以按需要使用分支、worktree、stash 或 WIP 提交；首次写入前仍须检查 `git status`，保护已有未提交内容和其他任务文件。
- 选择默认持续当前对话；用户可为具名任务改选。改选只影响尚未开始写入的新任务，不自动搬移、覆盖或删除现有工作区。
- worktree 选择只决定本地隔离方式，不改变 Plan 门禁、专业权限、远端发布权限、风险确认或同克隆 Unreal 锁。

## 大任务 Plan 门禁

- 所有大程序任务都必须在实现前创建正式编号 Plan，并按 `PLANNER_RULES.md` 发布到 `origin/main`；是否采用 Planner-Executor 模式不影响该要求。
- 大任务通常包括新系统/模块、跨文件行为、公共 API/Schema/存档/生成器契约、架构调整、数据迁移或需要成组验收的功能。
- 小型、边界清晰的修复、只读审计或用户明确豁免的任务可直接处理，但仍要说明范围、执行适用验证，并维护受影响的模块文档。

## 远端协作安全检查（所有模式均适用）

- 远端状态可能变化时先 `git fetch`。
- 若 fetch 发现当前批准基线之外的他人提交，先报告并审计物理/Git 冲突、逻辑冲突和耦合。清晰、范围不变且符合已批准契约的快进、merge 和物理冲突处理可继续；真实逻辑冲突、产品取舍、不可逆覆盖或范围扩张才等待用户决定采用远端、采用本地、组合适配或延期/拆分。
- 发布 `origin/main` 前必须按 `GIT_RULES.md` 原子取得 `main-publish-lock`；获锁后重新合入最新 main，并重跑因基线变化失效的适用门禁。
- 完成候选后遵循“能合并就合并、能推送就推送、能删除就删除”；主观验收、产品取舍、外部语义选择和可能丢失工作的清理仍等待人决定。

## 程序接管 Issue 分支修复

本节是程序在策划已发布的 `issue/<策划身份>/<简述>` 上定位、修复、交付测试候选并最终集成 main 的唯一权威。这是推荐但可选的 Bug 修复方式；程序也可以按普通 Plan/本地任务流程修复。选择本流程不免除大任务 Plan、工作区模式、构建、测试或 main 发布锁。

### 接管与同步

1. 只接管已存在于远端、严格一分支一 Bug、包含策划确认报告、日志分析和完整原始复现日志的准确 Issue 分支；程序不得借本流程自行创建新 `issue/...`、改名复用其他事项或把 `request/...` 当修复分支。仅当报告按 `DESIGNER_RULES.md` 证明客观无法生成日志并记录策划确认时，才允许没有原始日志。缺日志、未分析日志或打包多个 Bug 的分支必须退回策划拆分补证，不能进入程序修复。先 fetch 最新 `origin/main` 和目标 Issue 分支，运行 `python scripts/validate_project.py`，记录两者准确 tip，并审计 Issue 独有提交、源分支、日志、未决产品取舍和相对 main 的影响面。
2. 推荐为该 Issue 创建独立 worktree；至少必须遵守本对话已确认的本地工作区模式，不得占用有未提交内容或其他任务进行中的主文件夹。以准确远端 Issue tip 建立本地跟踪分支，不从本地 `main` 或缓存副本重建基线。
3. Issue 分支已经发布，接管后先把刚 fetch 的 `origin/main` **merge** 到本地 Issue 分支，再基于这个组合状态定位和修复；不得 rebase、强推或重写已发布历史。开始修复前把报告状态改为“处理中”，记录程序接管者、main tip、Issue tip、合并提交和发现的耦合。
4. 清晰且不改变已确认语义的物理冲突可解决后继续；真实逻辑冲突、Issue 含有与该 Bug 无关且尚未验收的独有实现、产品取舍或无法判断的跨团队耦合，先报告并等待有权限的人决定，不能把无关历史顺带发布 main。

### 修复与策划验收

1. 程序只实现报告中已确认的 Bug 范围，补充诊断、自动化测试和受影响模块文档；提交身份按 `shared/GIT_RULES.md` 路由为实际程序角色。同一 Issue 的后续修正继续复用该分支，不另建平行程序远端分支。
2. 将候选交给策划测试前，再次 fetch `origin/main` 和远端 Issue 分支。远端 Issue 有新提交时先 merge 到本地，主线前进时再次 merge 最新 `origin/main`，并重跑失效验证；所有推送都是对同名 Issue 分支的普通非强制 push，不使用 `main-publish-lock`。
3. 每个交给策划的程序测试候选都必须执行 `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`，提交匹配源码的精选 Win64 Editor 预构建包，并通过 `python scripts/validate_project.py`、适用自动化、`git diff --check` 与 LFS 校验。这样策划可拉取该 Issue 分支后直接打开项目测试；没有通过或经准确人工确认跳过的项目必须如实写入报告。
4. 程序把准确候选提交号、构建/测试证据、测试入口和已知限制写入 Issue 报告后推送。策划在同一远端 Issue 分支上测试，不修改程序代码；策划 AI 将针对准确候选的通过/失败现象、日志和人工结论写回报告与经验文件，按 `shared/GIT_RULES.md` 使用实际策划身份提交并普通推送同一分支。
5. 策划验收失败时，程序 fetch 并 merge 远端 Issue 的反馈提交，再按同一流程修复和发布下一候选。策划未明确确认准确候选通过前，不得把修复发布到 main。

### 程序侧集成

1. 策划确认准确候选通过后，程序 fetch 最新 Issue 与 `origin/main`，确认验收提交覆盖当前修复、报告和日志完整，并把远端 Issue tip 作为正式 main 候选；本流程不再创建 `merge/...` 分支。
2. 按 `shared/GIT_RULES.md` 取得 `main-publish-lock`，获锁后把最新 `origin/main` merge 到候选，重新完成最终 FullRebuild、自动化、项目校验、差异审计与 LFS 门禁，再普通推送锁分支和 main。测试分支上的旧构建不能替代最终组合候选构建。
3. 获锁合入 main 并形成可引用的最终集成提交后，先在同一发布候选的报告中记录该集成提交、最终门禁与预期结果，再提交报告闭环并完成发布；不能等发布后另起一次 main 发布补记录。发布后核验结果，确认远端 Issue 的全部提交均已进入 main、报告与日志已留档且远端 Issue tip 未变化后，按 `DESIGNER_RULES.md` 的正常生命周期准确删除该 Issue 分支。

## Windows 打包权威流程

本节是 Windows 测试包和正式包的唯一规则入口。打包必须以准确来源、磁盘已保存状态和可复核证据为准，不能用“构建成功”代替资源一致性或日志可用性证明。

标准一键入口为 `python scripts\ue\package_windows.py`：无参数时从当前工作树生成桌面 Development 日志测试包；明确要求远端最新主线时使用 `--remote-main`；明确要求正式包时使用 `--formal`。参数含义与输出位置只在 `scripts/README.md` 维护，本节只维护门禁原则。

1. **先确定准确物理工程。** 用户没有明确说“远端最新主分支”时，必须从用户确认、正在运行的进程或最新 Unreal Editor 日志确定其实际打开的 `ReEcho.uproject`，并从该物理工作树打包；不得擅自改用本地 `main`、`origin/main`、另一 worktree 或旧打包目录。只有用户明确要求远端最新时，才先 fetch 并从准确 `origin/main` 建立干净打包工作树。
2. **所见即所得只覆盖已保存到磁盘的状态。** 打包前要求用户在准确工程内保存全部资产并正常关闭 Editor；脚本检测到 `UnrealEditor.exe` 或 `UnrealEditor-Cmd.exe` 仍运行时必须停止并提醒，禁止强制关闭或声称未保存的内存状态已进入包。打包前记录 `Content/Config` 工作区状态；使用镜像 worktree 时，必须证明其 `Content/Config` 与用户实际工程的已保存快照无差异。
3. **先通过 LFS 门禁。** 打包前运行 `python scripts/setup_lfs.py --check`；fetch、checkout、merge 或切换打包来源后如涉及 `.gitattributes` 或 LFS 路径，重新还原并检查对象。LFS 指针、缺失对象或未还原的大资源会直接阻止打包。
4. **默认生成带完整日志的 Development 测试包。** 用户只说“打包”而未明确说“正式包”“发布包”或“无日志包”时，使用 Win64 Development Game 配置；只有用户明确要求正式发布或无日志时才使用 Win64 Shipping。UE 5.8 安装版的 Shipping 引擎预构建关闭普通日志，不得用项目侧宏强开后冒充可用的 Shipping 完整日志包。Development Game 会编译非 Editor 目标；引用 Editor-only API 的自动化测试必须同时受 `WITH_EDITOR` 保护。
5. **来源或模式变化必须干净构建。** 工作树、提交、`Content/Config`、Development/Shipping 模式或关键打包配置变化后，必须执行 Clean Build/Cook/Stage/Archive，不得用 `--no-clean`、旧 Cook、旧二进制或另一模式的输出拼装本次包。Cook 对软引用、Loose/NonUFS 数据和影片的收集必须按项目契约验证；运行时从磁盘读取但未被 Cook 引用的文件必须显式 Stage。
6. **每个包附带来源清单。** 输出根目录必须包含 `ReEchoPackageSource.txt`，至少记录源 `.uproject` 绝对路径、准确 Git 提交、打包模式、`Content/Config` 已保存工作区差异数量及明细；无法证明来源时不得把包描述为 UE 所见即所得。
7. **日志路径按实际模式验证。** Development 包的完整会话日志通常位于包内 `Windows\\ReEcho\\Saved\\Logs\\ReEcho-session-*.log`；环境发生重定向时再检查 `%LOCALAPPDATA%\\ReEcho\\Saved\\Logs`。AI 必须实际启动目标 EXE 并确认本次新生成、非空的会话日志，不能只检查 `ShopPurchaseAudit.log` 或 `UIInteractionAudit.log` 就声称完整日志已开启。
8. **交付前完成闭环检查。** 至少确认 Build、Cook、Stage、Archive 成功，目标 EXE 存在且能持续运行冒烟时长，来源清单准确，预期日志实际生成，并关闭所有冒烟测试进程。交付给策划前应清理包内由打包者冒烟产生的旧 `Saved` 测试状态，或明确隔离并标注证据日志，避免策划误把旧日志当作本次复现原始日志。

## 程序范围

- 可以修改 C++、Python 工具、Schema、配置、公共契约和技术文档，但不得越过用户目标或专业角色边界。
- 修改任何 Runtime Module 前，大任务须在 Plan 中写明设计意图和受影响 `MOD-*` / `AREA-*`，并将对应 `shared/CODEBASE_MAP/modules/MOD-*.md` 纳入同一候选；新增模块必须同时创建其详细文档。轻量任务在本地记录等价信息。
- 即使客观检查通过，玩法手感、视觉质量、文案可用性和音频平衡仍由用户验收。
- 通过已发布契约消费策划编写的 XLSX 和美术资产；不得用便利常量或隐藏生成文件替代事实来源。
- 请求主要属于平衡/内容编写或视觉/音频制作时，指出策划/美术边界，并在切换路线前询问用户。

## 跨角色交接

- 向策划提供稳定字段、单位、允许 ID 和验证错误；不要要求其在文本单元格中编码新逻辑。
- 向美术提供准确资产路径、尺寸/格式、绑定名称和运行时约束；不要要求其在 Blueprint 中修改玩法状态。
- 评审专业变更的运行时契约和集成安全，但主观验收留给对应专业用户。

提交或发布前，遵循 `shared/GIT_RULES.md` 中适用的身份和发布规则。
