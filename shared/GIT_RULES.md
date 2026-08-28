# ReEcho Git 规则

本文件是提交身份和发布完整性规则的唯一权威。角色和职责文件只路由至此，不重复定义标签或程序发布构建门禁。

本文件中的风险性禁止项和例外统一遵循 `shared/PROJECT_RULES.md` 的"风险操作的人类确认与执行"：先提醒风险并提出是否建议问程序，获得有权限的人对准确操作的确认后执行；提交身份真实性不属于可豁免风险项。

## AI 正式提交身份

推送到任何远端引用或准备进入远端 main 的每个 AI 正式提交，其标题必须以且仅以一个标签开头；标签需匹配创建该正式提交所使用的专业路线或明确指派的仓库职责：

| 提交创建者 | 标题必需前缀 |
|---|---|
| 程序路线 | `[PROGRAMMER]` |
| 策划路线 | `[DESIGNER]` |
| 美术路线 | `[ARTIST]` |
| 明确指派的项目秘书职责 | `[SECRETARY]` |

- 任何角色不得使用其他角色的标签。项目秘书仅对 `SECRETARY_RULES.md` 范围内的协调/控制面提交使用其标签，不得用它标记专业实现。
- 程序 Planner 的集成或合并提交使用 `[PROGRAMMER]`，因为 Planner/Executor 是程序路线内的仓库职责，不是独立专业身份。
- 混合范围不是组合标签的理由；应先按专业边界拆分提交。
- AI 创建准备推送到远端或进入 main 的正式提交前，仓库级 `user.name` 必须同时包含当前已确认的 GitHub 人类账号和执行该提交的 AI 身份，例如 `JosephLE910 + Codex`；`user.email` 必须使用该账号已验证或 GitHub noreply 地址。人类账号未知或未经用户确认时，必须在正式提交前停止并询问，不能只写 AI 名称。
- Author 配置不替代标题职业标签；两者必须同时满足。提交说明可附加人类账号，但不能用说明代替正确的 Author 字段。
- 本地 WIP 提交的名称、粒度和临时身份可由每个成员自由组织；推送远端协作分支或发布 main 前必须通过 squash、reword、重新提交或精选 cherry-pick，形成满足本节的人类账号 + AI 身份和单一专业标签的正式候选。不得仅为给已经发布的历史提交补标签而重写远端历史。

## 提交文本编码

AI 创建的正式提交，其标题与正文必须是合法 UTF-8，且不得包含乱码（mojibake，如 GBK/其他编码字节被当 UTF-8 显示产生的乱字符）：

- 从 `.xlsx` / GBK / GB2312 等来源摘取的中文文本（如策划表名、字段名、决策摘要）写入提交标题或正文前，必须先转码为 UTF-8 并人工核对无乱码，严禁把编码错误的字节串直接塞入标题。
- 标题是硬约束重点：标题中不得出现任何无法在终端正确显示的中文字符；合并提交、rebase、cherry-pick 生成的描述同样适用。
- 提交前应通过 `git log --oneline -1` 复核标题可在终端正确显示；若发现乱码，必须在推送前 reword 修正，不得带乱码标题进入 `origin/main`。

## Git LFS 提交与发布门禁

- 提交 LFS 路径前运行 `python scripts/setup_lfs.py --check`、`git lfs status` 和 `git lfs fsck`，确认工作树是已还原的真实文件、索引按 `.gitattributes` 生成 LFS 指针，并且没有把指针文本当普通文件提交。
- 包含 LFS 对象的分支使用普通 `git push`；Git LFS pre-push 必须先成功上传对象，Git 引用才能视为推送成功。不得用跳过 hook、`--no-verify` 或仅推 Git 指针的方式绕过对象上传。
- 推送后在准确最终候选上运行 `git lfs push --dry-run origin HEAD`；仍列出待推对象时发布未完成，必须补齐对象并重新核验。Git 远端引用仍按本文件其他发布步骤单独核对提交号；随后运行 `git lfs fsck`，并在发布报告中记录 LFS 文件、对象检查和远端引用。
- 获取最新主线、合入外部提交或切换候选后，若 `.gitattributes` 或 LFS 路径变化，必须运行 `python scripts/setup_lfs.py` 还原对象，并在最终候选上重跑本节门禁。旧候选的 LFS 证据随基线变化失效。
- `git lfs push --all`、`git lfs migrate import`、改写已发布指针/对象历史或扩大 LFS 追踪模式不属于普通发布步骤，按 `PROJECT_RULES.md` 的风险确认机制执行。

## origin/main 单发布者锁

`main-publish-lock` 是程序路线和项目秘书发布 `origin/main` 时唯一允许的远端协调分支，也是 main 发布锁的唯一权威状态。它不是内容发布面、任务分支或长期备份；策划/美术协作分支不使用此锁。除下述“编号 Plan 单独发布例外”外，规则、文档、代码、数据、资产和集成候选进入 main 前都必须遵循本节；禁止强推 main。

### 编号 Plan 单独发布例外

发布编号 Plan 不需要获取、检查或等待 `main-publish-lock`。该例外仅适用于候选只新增或修改一个或多个 `plans/<id>-*.md` 编号 Plan，且不包含 `plans/TEMPLATE.md`、规则、实现、生成物或其他无关 WIP；即使远端锁已存在，合规的 Plan-only 候选也可继续发布。Plan 发布者不得创建、更新或删除锁分支。

Plan-only 发布前必须 fetch 最新 `origin/main`，完成 Plan 编号、外部变化和文件范围审计，并让候选基于准确远端主线；只允许使用普通非强制 push 发布 `origin/main`。推送被拒或远端在推送前后发生变化时，不得强推或覆盖，必须重新 fetch，按 `PLANNER_RULES.md` 处理编号占用与传入变化，重建候选并重跑失效的静态验证。发布后必须核验远端 main 已包含准确 Plan 提交。除本例外外，任何 main 发布仍执行下方完整锁流程。

1. 抢锁前必须形成身份合规、范围明确的本地正式提交并保持工作区干净；先 fetch `origin/main`，再检查远端 `refs/heads/main-publish-lock`。
2. 锁分支不存在时，只能用下列空 expected value 的准确 lease 原子创建，并以当前正式候选作为锁分支初始提交：

   ```powershell
   git push --force-with-lease=refs/heads/main-publish-lock: origin HEAD:refs/heads/main-publish-lock
   ```

   push 失败或远端锁提交号不等于本地 `HEAD` 时均视为未获锁，必须停止发布；不得改用普通 push、强推或删除现有未合入锁来抢占。
   未获锁时只允许 fetch、`ls-remote`、log、diff 和祖先检查等只读审计；不得将 `origin/main`、本地 `main`、其他 main 跟踪分支或其提交副本以 merge、rebase、cherry-pick、reset 等方式接入当前待发布候选，也不得先更新本地 main 后绕行合并或提前执行最终发布构建。普通开发可继续本地工作，但必须等实际获锁后才开始本轮 main 发布集成。
3. 锁分支已存在时，先 fetch 该引用并检查祖先关系。若锁提交尚未进入 `origin/main`，它代表其他发布者的进行中/中断候选，任何其他 AI 不得 merge、更新、删除该锁或推送 main；报告持有提交、作者和与 main 的差异。若锁提交已是 main 的祖先，代表发布完成但清理中断，程序集成职责或秘书可在核验后用准确 lease 删除该已合入锁，再重新抢锁。
4. 获锁后必须重新 fetch `origin/main`，把准确最新 main **merge** 到当前工作分支。不得在持锁期间 rebase 或重写已发布到锁分支的候选历史。清晰、范围不变的物理冲突可按已批准契约处理；遇到真实逻辑冲突、产品取舍、不可逆覆盖或范围扩张时保留锁、先报告并等待人决定。
5. 获锁后的 merge、冲突解决或远端基线变化会使此前受影响的构建、测试和静态证据失效；必须对最终组合候选重新提交并执行适用门禁。
6. 最终候选必须是抢锁时提交的后代，并以普通非强制 push 快进更新锁分支：

   ```powershell
   git push origin HEAD:refs/heads/main-publish-lock
   ```

7. 更新锁分支后再次 fetch `origin/main`，确认 `origin/main` 是本地 `HEAD` 的祖先，并确认远端锁仍准确指向本地 `HEAD`。任一检查失败都不得发布；保留锁，重新合并最新 main、提交并重跑失效门禁。
8. 仅在上述检查通过后，使用普通非强制 push 发布 main：

   ```powershell
   git push origin HEAD:refs/heads/main
   ```

9. 发布后 fetch 并确认远端 main 与候选提交号完全一致。只有 main 已发布成功且远端锁仍指向同一候选时，才能用该准确提交号作为 lease 删除锁；网络超时或结果不明时先查询远端，不得盲目重推或解锁：

   ```powershell
   git push --force-with-lease=refs/heads/main-publish-lock:<CandidateCommit> origin :refs/heads/main-publish-lock
   ```

锁分支存在即阻止其他 AI 开始 main 发布。锁持有者中断时不得仅按超时自动破锁：未合入候选由程序集成职责或秘书审计后续传、保留或在准确风险确认后解除；已合入候选按第 3 项机械清理。锁分支的准确 lease 创建/清理是远端引用限制的具名默认例外，但不授权任何其他强推、远端分支或历史覆盖。

策划的 `issue/...`、`request/...`、`merge/...` 协作分支不使用 `main-publish-lock`；其命名、按事项复用、旧分支迁移、同步、人工确认、经验记录和合并后删除由 `shared/DESIGNER_RULES.md` 唯一管理。程序按 `shared/PROGRAMMER_RULES.md` 接管现有 Issue 分支时也不使用发布锁，只有最终进入 main 才取得锁。协作分支上的每个正式提交按实际创建角色使用单一身份：策划登记或验收使用 `[DESIGNER]`，程序修复、构建与程序合并使用 `[PROGRAMMER]`；所有提交仍须满足本文件的 UTF-8 和人类账号 + AI 身份要求。

## 程序发布构建门禁

本门禁适用于程序路线的每一次 `origin/main` 推送，包括 Plan、Markdown、规则、源码、内容或混合候选。Planner-Executor 模式选择本身不能豁免；跳过构建或远端协作检查的具名步骤必须按 `shared/PROJECT_RULES.md` 提醒风险、建议问程序并取得准确人工确认。

**纯文档性质例外（豁免 `-FullRebuild`）**：若本次候选仅新增或修改文档性质内容（仅 `.md` 规则、Plan、工作流、说明/架构文档，且不改动任何 C++、二进制、预构建包、DataAsset 反射契约或 UE 内容），则**不需要** `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`，可直接提交并推送。此类候选仍须满足：按 `shared/PROJECT_RULES.md` 验证矩阵执行 `python scripts/validate_project.py` 与 `git diff --check`；推送前完成远端基线 `git fetch` 与冲突/重叠审计；以及本文件「提交文本编码」对标题 UTF-8 的约束。一旦候选混入任何代码/二进制/内容变更，即回到完整 `-FullRebuild` 门禁。

程序路线每次推送 `origin/main` 前（纯文档例外不适用时），必须使用项目标准 UE 5.8 安装版/发行版完整构建最新、准确的最终集成候选：

```powershell
scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild
```

命令成功后会刷新精选 Win64 Editor 预构建包及其源码指纹。同一候选必须包含全部已批准源码/内容/配置变更，以及 `Binaries/Win64/ReEchoEditor.prebuilt.json` 声明的匹配跟踪文件。

- 发布门禁要求 `-FullRebuild`；实现期间可使用报告目标已是最新的增量构建，但它不能满足最终推送门禁。
- 禁止发布仅含源码的程序候选，也不得复用最终集成/rebase/冲突解决之前的构建证据。
- 构建失败、缺少二进制、二进制哈希不匹配、源码指纹过期、Engine Build ID 错误或预构建刷新后工作区不干净，都会阻止推送。
- 构建后运行 `python scripts/validate_project.py`，并在最终 fetch/push 门禁前提交刷新后的允许列表产物。
- 可发布的生成 Editor 产物仅限 manifest、`ReEchoEditor.target`、`UnrealEditor.modules` 及 manifest 指定的准确模块 DLL。其他 `.target`、`Intermediate`、PDB/导入库、Live Coding patch、生成的解决方案、缓存、日志和机器本地路径均保留在本地。
- 该包只保证使用 manifest 记录的准确 UE 5.8 安装版/发行版 Build ID 在 Win64 上拉取即开。其他引擎构建必须本地编译，或建立各自经过评审的交付契约。

上述构建/预构建要求是默认发布门禁。若有权限的人在获知"拉取可能无法直接打开、源码与二进制可能不匹配、下游需自行编译"的准确风险后，仍明确确认对具名候选跳过其中某项检查或产物刷新，AI 可以发布，但必须记录"经人工确认跳过/未验证"的准确项目，绝不能声称门禁通过。风险提醒必须明确建议先问程序；候选或远端基线变化后需重新确认。

策划、美术和秘书提交不重建或修改程序拥有的预构建包。其本地提交保留各自角色标签并由程序 Planner 集成；之后的程序发布构建覆盖最终组合候选。
