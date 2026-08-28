# Plan 151 - 程序 - Stage01To02 CG 已观看跳过

## 协调

- Planner 负责人：Codex（程序路线）。
- Executor 负责人：Codex（程序路线）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@aef4811f`。
- 本地实现方式：`C:\tmp\ReEcho-plan151-stage01to02-cg-skip`，分支 `codex/plan151-stage01to02-cg-skip`。
- 依赖 / 阻塞：沿用 Plan133 已发布的 `Stage01To02` 媒体、音频和 CG 后镜头流程。
- Writes:
  - `Source/ReEcho/Public/Run/ReEchoPlayerProgressSaveGame.h`
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Source/ReEcho/Public/UI/ReEchoEncounterTransitionWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoEncounterTransitionWidget.cpp`
  - `Source/ReEcho/Public/ReEchoGameMode.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoEncounterTransitionTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoSaveGameTests.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `plans/151-stage01to02-cg-skip.md`
  - `Binaries/Win64/ReEchoEditor.prebuilt.json` 及其准确 manifest 允许列表产物
- Stable Reads:
  - `plans/133-stage01-to02-cg-transition.md`
  - `Content/Movies/EncounterTransition/Stage01To02.mov`
  - `Content/ReEcho/UI/EncounterTransition/FMS_Stage01To02.uasset`
  - `Content/ReEcho/UI/EncounterTransition/S_Stage01To02.uasset`
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：旧安装没有账号进度槽时按“尚未观看”处理；删除任一 Run 槽不删除账号观看记录；媒体失败不授予观看资格。
- 明确排除：不修改 CG 视频/音乐资产，不改变第一关结束与第二关开始的镜头流程，不增加键盘快捷键，不实现按“通关一次”解锁。

## 锁定目标

只有当前账号曾经自然完整播放过一次 `Stage01To02` CG 时，后续播放该 CG 才在右上角显示“跳过”按钮。首次播放必须完整观看；点击跳过只停止本次视频和 CG 音乐，并复用自然完成后的回响近景、回到主角和解除暂停流程。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`AREA-Run`、`AREA-UI`、`AREA-Tests`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`，均已加入 Writes。
- 设计意图：把“是否已完整看过 CG”建模为账号级持久进度，而不是临时 Run 状态或 Widget 状态；Widget 只呈现按钮并广播请求，GameMode 负责流程编排，RunSubsystem 负责持久状态。
- 权威状态与依赖：新增独立玩家进度 SaveGame 槽；`UReEchoRunSubsystem` 缓存和持久化 `bHasViewedStage01To02Cg`，GameMode 读取资格并在自然成功完成后写入，Widget 不直接访问存档。
- 决策记录：自然完成才置位；失败、中途退出不置位；已具备资格的主动跳过不重复授予；跳过与媒体结束共用现有状态门保证幂等。
- 相关文档同步范围：更新 `MOD-ReEcho.md` 的账号级观看进度和 Stage01To02 编排；更新 `MOD-ReEchoUI.md` 的跳过按钮显示/委托边界。全局模块拓扑和索引路由不变，因此 `ARCHITECTURE.md`、`README.md` 只审阅不改。
- 关闭前逐项填写审阅结果：待实现后补充。

## 锁定验收

- [ ] 新账号或无玩家进度槽时，首次 `Stage01To02` CG 不显示跳过按钮。
- [ ] 第一次自然完整播放成功后持久保存观看标记；重启游戏或删除当前 Run 后资格仍保留。
- [ ] 后续播放时右上角显示“跳过”，点击后立即禁用、停止媒体与 CG 音乐并进入原 CG 后镜头流程。
- [ ] 播放失败、中途退出不写观看标记。
- [ ] 媒体结束与点击同帧只完成一次；跳过不走失败日志/失败授权路径。
- [ ] 自动化覆盖默认值、持久化契约、按钮资格和幂等策略；FullRebuild、项目校验、LFS、预构建包和 `git diff --check` 通过。
- [ ] PIE 人工验证首次、再次播放和点击跳过的画面、音频、焦点与镜头衔接。

## Step 0 门禁

- 基线分支/提交：`origin/main@aef4811f`。
- 引擎/构建可用性：沿用当前 UE 5.8 安装版；执行前运行 LFS hydration 检查。
- 现有聚焦测试结果：待记录 `ReEcho.UI.EncounterTransition` 与 `ReEcho.SaveGame` 基线/候选结果。
- 共享契约 / 难合并资源风险：GameMode、RunSubsystem、过渡 Widget 和预构建包与并行 main 变化可能重叠；发布实现前须按发布锁合入最新 main 并重跑门禁。
- 基线损坏时的停止条件：现有 Stage01To02 自然完成路径或 SaveGame 基线测试失败且不能与本任务隔离时停止并报告。

## 实现提纲

1. 新增独立玩家进度 SaveGame 与 RunSubsystem 窄查询/写入接口。
2. 在过渡 Widget 右上角创建只拦截自身区域的“跳过”按钮，按 GameMode 注入资格显隐并广播一次请求。
3. GameMode 将主动跳过和自然结束收敛到同一幂等完成门；只有自然成功完成写入观看资格。
4. 增加自动化，更新模块文档和执行记录。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| LFS | `python scripts/setup_lfs.py --check` | 现有媒体和资产均为已还原文件 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目、文档和文本不变量通过 |
| C++ 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新准确预构建包 |
| 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.EncounterTransition`；`-Filter ReEcho.SaveGame` | 跳过策略与存档迁移/持久化契约通过 |
| 预构建 | `python scripts/ue/prebuilt_editor.py check` | manifest 与源码指纹匹配 |
| 人工 | PIE 首次完整播放、重启后再次播放并点击跳过 | 首次无按钮；再次有按钮；媒体/音乐停止且镜头流程正常 |

## 执行记录

### 变化

- 新增独立 `ReEchoPlayerProgress` SaveGame，由 RunSubsystem 在 GameInstance 初始化时加载，并仅在 Stage01To02 CG 自然完成且保存成功后缓存已观看状态。
- 过渡 Widget 新增右上角“跳过”按钮；媒体层不接收点击，按钮资格由 GameMode 从 RunSubsystem 注入，点击后立即禁用并广播。
- 跳过按钮底图使用 `0.5` 透明度，按钮文字保持完全不透明。
- GameMode 区分自然完成、失败与主动跳过；跳过关闭媒体和 CG 音频后复用原 CG 后镜头流程，自然结束与按钮请求仍由播放状态门保证只处理一次。

### 证据

- `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`：通过；UE 5.8 Development Editor 完整重建并刷新 7 个模块预构建包，源码指纹 `82e644406d2e`。
- `ReEcho.UI.EncounterTransition.Policy`：通过；验证首次/非播放隐藏与已观看播放中显示策略。
- `ReEcho.Run.PlayerProgress.Stage01To02Cg`：通过；验证默认未观看及独立 SaveGame 内存序列化往返。
- `python scripts/validate_project.py`：通过。
- `python scripts/ue/prebuilt_editor.py check`：通过，`build_id=55116800`。
- `git diff --check`：通过。

### 剩余风险

- 自动化不能替代真实媒体播放与 Slate 命中测试；按钮视觉位置、首次无按钮、第二次可跳过、视频/CG音乐立即停止以及 CG 后镜头衔接需要 PIE 人工确认。

### 人工验收结果/请求

- `PendingBeforeClose`：请使用本任务工作树进行首次自然完整播放，再重启并再次进入第一关转场验证跳过按钮。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已更新：记录账号级观看状态权威、独立存档槽和自然完成/跳过流程。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 已更新：记录按钮命中、显隐和委托边界。
- `shared/CODEBASE_MAP/ARCHITECTURE.md` 已审阅、无需修改：没有新增 Runtime Module 或改变模块依赖拓扑。
- `shared/CODEBASE_MAP/README.md` 已审阅、无需修改：现有 `AREA-Run` / `AREA-UI` 路由仍准确。
