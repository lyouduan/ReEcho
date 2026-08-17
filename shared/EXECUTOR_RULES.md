# ReEcho Executor 规则

本文件定义具体实现职责。它既适用于独立 Executor，也适用于一个 AI 同时承担规划和执行。硬约束遵循 `PROJECT_RULES.md`。

本文件中的风险性禁止项和例外统一遵循 `shared/PROJECT_RULES.md` 的“风险操作的人类确认与执行”：先提醒风险并提出是否建议问程序，获得有权限的人对准确操作的确认后执行。

## 安全开始

1. 大任务先确认指定 Plan 已发布到 `origin/main`，状态允许实施，目标、验收、基线、依赖、Writes/Reads、影响模式、排除项和实现 AI 侧清晰。小型修复、只读审计或用户明确豁免的任务可按准确本地任务记录开始。
2. 程序大任务须列出受影响 `MOD-*` / `AREA-*`、设计意图和 `CODEBASE_MAP` 同步范围，且每个被修改 Runtime Module 的 `modules/MOD-*.md` 位于 Writes。
3. 按 `PROGRAMMER_RULES.md` 已确认的本地工作区模式执行。选择“一任务一 worktree”时，首次写入前创建或复用本任务专属独立文件夹，禁止直接在主工作区实现；选择“否”时可直接工作，也可使用任意本地分支、worktree、stash、WIP 提交、merge 或 rebase。隔离 worktree 不是共享权限门禁，但明确选择后是当前任务的本地执行约束。
4. 只读取指定 Plan、选中的代码路线和相关经验；失败需要诊断后再扩展到匹配的 `LESSONS.md` 调试章节。
5. `origin/main` 是唯一远端分支。Executor 默认不自行推送远端；远端发布交给当前用户授权的程序集成职责。

## 在契约内实现

- 可自由优化实现细节，但必须保留锁定目标、验收和受保护门禁。
- 在 Plan 执行记录中写明有意义的偏差、决策和证据。
- 更新 Plan Writes 中直接相关的模块文档，使设计意图、契约、依赖、运行流程、测试和代码位置与实现一致。不要把无关共享文档带入候选。
- 若实现必须扩张目标、验收、稳定 ID、Schema、存档格式、生成器契约或公共 API，只暂停越界部分并通知负责规划/用户；安全的范围内工作可继续。
- Plan 中的 `Isolated | ReadOnly | SharedContract | Exclusive` 只说明集成风险，不是本地写锁。与其他本地工作重叠时由当前成员自行协调；进入远端前必须审查并适配实际 main。
- 权威 XLSX 与生成的生产 CSV 是一个发布单元。允许本地试验，但禁止进入 main 时静默覆盖他人表格修改或只提交其中一半。
- 禁止为让检查通过而削弱验收、暗中复制提供者内部数据模型，或将本地 WIP 视为已验收功能。

## Windows UE 工具链

从项目根目录使用仓库入口：

```powershell
python scripts\validate_project.py
scripts\ue\Build-Editor.cmd -Configuration Development
python scripts\ue\package_windows.py
```

Unreal 功能自动化仅在 Plan 或用户明确要求时作为可选证据。运行需要关闭 Editor 的命令前，请用户保存并关闭；不得声称 AI 完成 PIE、视觉、手感或音频主观验收。

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

不得在 catch 路径中删除他人的锁。仅在确认没有 UnrealEditor/commandlet 进程使用本项目，并记录恢复过程后，才能删除过期锁。

## 验证与交接

1. 风险变更后执行聚焦检查，交接前执行 `PROJECT_RULES.md` 和 Plan 的最终矩阵。
2. 汇总当前候选证据；不得依赖过期二进制、冲突解决前或 rebase 前的结果。
3. 将 Plan 更新为 `Review`，保留人工验收值并补齐执行记录及模块文档审阅结果。
4. 本地提交方式自由；交接可以是提交、分支、worktree 或干净/明确的工作树。报告准确路径、基线、diff、验证、风险和剩余人工验收。
5. 若创建准备进入远端的正式提交，必须按 `shared/GIT_RULES.md` 使用人类账号 + AI 身份和单一专业标签；本地 WIP 可在发布前整理。
6. 不自行推送任务分支、创建 PR/MR 或发布 main。若用户明确授权当前 AI 承担集成/发布，则切换到 `PLANNER_RULES.md` 和 `GIT_RULES.md` 的远端边界门禁。

## 接管未完成工作

- 从 Git、Plan 和执行记录识别最后一个已验证候选。
- 使用 `git status` 和 `git diff --stat` 分离已验证与未验证变化。
- 每次只修改并验证一个假设；不得叠加推测性修复。
- 回滚前用分支、stash 或 patch 保留他人工作。没有准确人工确认时，不做破坏性历史或文件系统清理。
- 修复源码并重建；不得把 `Intermediate`、未精选 `Binaries`、`Saved`、打包输出或日志当作源码修补。
