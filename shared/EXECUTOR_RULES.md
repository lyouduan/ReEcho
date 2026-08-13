# ReEcho Executor 规则

本文件只包含 Executor 操作。最小启动顺序遵循 `AGENTS.md`，硬约束遵循 `PROJECT_RULES.md`。Planner-Executor 模式选择由 `PROGRAMMER_RULES.md` 管理；不可跳过的远端协作安全检查适用于所有模式。

## 安全开始

1. 在隔离的 `plan/<id>-<short>` 分支/worktree 中工作，绝不直接在 `main` 中实现。
2. 确认指定 Plan 已发布到 `origin/main`；任务确实无需 Plan 时，则确认提示中嵌入的契约：
   - 声明的 implementation-authored-by AI side 与本次 Executor 交接一致；缺失或错误时，编辑前停止并请 Planner 修正来源；
   - 生命周期为 `Ready`、`InProgress`，或仅在明确指派评审修复时为 `Review`；
   - 已命名实现基线和分支；
   - Writes/Reads、影响模式和排除项与 Exchange 一致；
   - 程序任务已列出受影响 `MOD-*` / `AREA-*`、设计意图和 `CODEBASE_MAP` 同步范围，且每个被修改 Runtime Module 的 `modules/MOD-*.md` 位于 Writes；
   - 已具备所需 `SharedContract`/`Exclusive` 批准。
3. `Reserved` 所有权行只声明意图，不阻塞互不重叠或只读工作。绝不写入其他所有者的 `Active` `Exclusive` 资源。
4. 只读取指定 Plan、匹配的 Exchange 区块、选中的代码路线和相关经验。仅在失败需要诊断后读取 `§DEBUG`。

同机 agent 使用独立 worktree；不同机器的成员使用独立克隆和本地任务分支；`origin/main` 是唯一远端分支。除 `shared/GIT_RULES.md` 管理的精选“拉取即开”Editor 包外，构建产物均保留本地。

## 在契约内实现

- 可自由优化实现细节，但必须保留锁定目标、验收和受保护门禁。
- 在 Plan 的执行记录中写明有意义的偏差和证据。
- 更新 Plan Writes 中与所拥有实现直接相关的模块文档，使设计意图、契约、依赖、运行流程、测试和代码位置与实现一致。不要越权编辑未声明的 `CODEBASE_MAP` 文档、`LESSONS`、工作流规则或已关闭 Exchange 历史。
- 若实现必须扩张 Writes，或修改稳定 ID、Schema、存档格式、生成器契约或公共 API，只暂停跨边界部分并通知 Planner；安全时继续无关的范围内工作。
- 禁止为让检查通过而削弱验收、暗中复制提供者内部数据模型，或将 WIP 分支视为已验收功能。

## 并行所有权

- `Isolated`：只编辑声明的互不重叠文件。
- `ReadOnly`：只使用提供者已发布的稳定表面。
- `SharedContract`：使用已约定接口；通过 Planner 请求扩展，而不是编辑提供者拥有的文件。
- `Exclusive`：难以合并的工件/目标只有一个活跃写入者。

权威工作簿只有活跃 `WorkbookWriter` 可提交 `Design/Data/ReEchoData.xlsx` 及其生成的生产 CSV 包。其他领域通过 Plan/交接提交请求；只读运行时消费者可并行。不会发布的一次性本地 QA 改动不认领仓库所有权。

## Windows UE 工具链

从项目根目录使用仓库入口：

```powershell
python scripts\validate_project.py
scripts\ue\Build-Editor.cmd -Configuration Development
python scripts\ue\package_windows.py
```

Unreal 功能自动化（`scripts\ue\Run-Automation.cmd`）仅在 Plan 或用户明确要求时作为可选证据，不是默认上层规则门禁。只执行 `PROJECT_RULES.md` 和 Plan 要求的检查。运行需要关闭 Editor 的命令前，请用户保存并关闭交互式 Editor。不得声称 AI 完成 PIE 或视觉验收；这些结果属于用户。

### 同克隆 Unreal 锁

锁通过 Git common directory 共享，因此克隆中的所有 worktree 都能看到同一文件：

```powershell
$gitCommon = (git rev-parse --path-format=absolute --git-common-dir).Trim()
$lockDir = Join-Path $gitCommon 'reecho-locks'
$lockPath = Join-Path $lockDir 'unreal-editor.lock'
New-Item -ItemType Directory -Force -Path $lockDir | Out-Null
$lockAcquired = $false

try {
    $lockStream = [System.IO.File]::Open($lockPath, 'CreateNew', 'Write', 'None')
    $lockAcquired = $true
    $writer = [System.IO.StreamWriter]::new($lockStream)
    $writer.WriteLine("pid=$PID branch=$(git branch --show-current)")
    $writer.Dispose()
    # Run the exclusive Editor/project command here.
}
catch [System.IO.IOException] {
    throw "ReEcho Unreal resource is already locked: $lockPath"
}
finally {
    if ($lockAcquired -and (Test-Path -LiteralPath $lockPath)) {
        Remove-Item -LiteralPath $lockPath
    }
}
```

不得在 catch 路径中删除他人的锁。仅在确认没有 UnrealEditor/commandlet 进程使用本项目，并在交接中记录恢复过程后，才能删除过期锁。

## 验证与交接

1. 风险变更后立即执行聚焦检查，交接前执行必需的最终矩阵。
2. 汇总当前提交的证据；不得依赖过期二进制或 rebase 前结果。
3. 将 Plan 生命周期更新为 `Review`，保留独立人工验收值（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`），并只释放不再编辑的资源。
4. 显式暂存文件，并使用 `shared/GIT_RULES.md` 要求的身份前缀、Plan 编号和具体结果在本地提交。不得推送任务分支；在同一克隆中将本地分支/提交交给 Planner。
5. 报告提交、变更表面、通过/失败检查、不可用前置条件和剩余人工验收。

Executor 不得推送任何远端引用、合并/发布 `main`、创建 PR/MR、移除自己的 worktree 或删除远端引用。

## 接管未完成工作

- 从 Git 和执行记录识别最后一个已验证提交。
- 使用 `git status` 和 `git diff --stat` 分离当前未验证变化。
- 每次只修改并验证一个假设；不得叠加推测性修复。
- 回滚前用分支/stash/patch 保留他人工作。没有明确权限时，禁止破坏性历史或文件系统清理。
- 修复源码并重建；不得把 `Intermediate`、`Binaries`、`Saved`、打包输出或其他生成产物当作源码修补。
