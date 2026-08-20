# Plan 59 - 程序 - Shipping 包可运行化（编译修复 + 运行时 CSV 入包）

## 协调

- Planner 负责人：Gavyn-side AI（按 shared/PLANNER_RULES.md 履行）
- Executor 负责人：Gavyn-side AI（待分配；本 Plan 实现可与规划同一 AI）
- Plan 编写方（AI 侧）：`Gavyn-side AI | ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Unassigned | Gavyn-side AI | ReEcho teammate-side AI`。
- 任务状态：`Closed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）；实现已发布至 `origin/main`（`85dc972`），FullRebuild 门禁通过，`validate_project.py` 全 PASS；本机 Shipping 包启动验证通过（exe 不触发 `LowLevelFatalError`，包内 28 张 csv 齐全）。由秘书流程依用户指令关闭。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：`origin/main` 在分配编号时的最大值之后下一空闲编号（本 Plan = 59）；实现基线待发布后从 `origin/main` 建立。
- 本地实现方式（可选，仅作交接说明）：建议一任务一 worktree（如 `ReEcho-plan59`），不在主工作区直接实现。
- 依赖 / 阻塞：无外部阻塞；受 `shared/GIT_RULES.md` 发布门禁约束（推 main 前须 `-FullRebuild` + 精选预构建包 + `validate_project.py`）。
- Writes：
  - `Source/ReEcho/Private/Presentation/Scene/ReEcho2DEditorPreviewActor.cpp`（Shipping 编译修复，`#if WITH_EDITOR` 包裹 `SetIsVisualizationComponent`）
  - `scripts/ue/package_windows.py`（新增 `stage_runtime_csvs()`，archive 后显式拷贝 `Content/Data/*.csv` 进最终包；方案 A 命中即停，未改 `DefaultGame.ini`、未新增占位资产）
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`（新增「运行时 CSV 松散文件与 Shipping 打包契约」一节）
- Stable Reads：
  - `Content/Data/reecho_data_manifest.csv`（运行时表清单，决定必须存在的 CSV 集合）
  - `Content/Data/*.csv`（28 张运行时表，松散文件经 `FFileHelper` 读取）
  - `Source/ReEcho/Private/ReEcho.cpp`（启动期 `LowLevelFatalError` 校验，行 19 附近）
  - `Source/ReEcho/Private/Data/ReEchoCsvDataRegistry.cpp`（CSV 注册/加载）
- 影响模式：`Isolated`（`Isolated | ReadOnly | SharedContract | Exclusive`，仅说明集成影响，不是写锁）。
- 兼容承诺 / 下游操作：不改动 CSV schema、表名、稳定 ID 与 `reecho_data_manifest.csv` 语义；仅在打包层保证这些文件到达 Shipping 包，不影响 Development/PIE 既有路径。
- 明确排除：不改动 `ReEcho.cpp` 的启动校验逻辑本身；不新增运行时数据表；不重排 CSV 列；不触及 Plan52/55 的演示/脚点对齐内容。

## 锁定目标

让 `scripts/ue/package_windows.py` 产出的 Win64 Shipping 包**可直接双击运行、不再因 CSV/编译问题闪退**，达成：

1. Shipping 配置能完整编译通过（消除 `UChildActorComponent::SetIsVisualizationComponent` 在 `WITH_EDITOR` 未定义时的 C2039）。
2. 包内 `ReEcho/Content/Data/` 包含 `reecho_data_manifest.csv` 所列全部运行时 CSV（当前缺失的至少 6 张：`stages.csv`、`attributes.csv`、`encounters.csv`、`encounter_waves.csv`、`spawn_policy.csv`、`spawn_profiles.csv` 必须就位）。
3. 启动后不再出现 `ReEcho.cpp` 的 `LowLevelFatalError: ... stages.csv: File could not be read`。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`（核心运行时 CSV 注册与启动校验，见 `Source/ReEcho/Private/Data/ReEchoCsvDataRegistry.cpp`、`Source/ReEcho/Private/ReEcho.cpp`）。Plan52/55（`MOD-ReEchoPresentation` 等）与本 Plan 无耦合，仅因二进制构建产物在 Git 层同文件存在冲突，需在发布前先解决基线（见协调/审计）。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 需补充“运行时 CSV 松散文件及其 Shipping 打包契约”一节（或确认已有说明覆盖）。已列入 `Writes`。
- 设计意图：运行时 CSV 是“设计唯一真源 → xlsx → csv → 类型化读取”链路的末端，目前经 `FFileHelper` 读松散文件。Shipping 包因 cook 只枚举被资产引用的目录/文件，纯 csv 目录 `Content/Data` 不被打包，导致启动校验失败。本 Plan 目标是在**不引入新运行时契约**的前提下，把这批已存在文件可靠送达包内。
- 权威状态与依赖：不改变数据权威或状态所有者；仅补打包投递环节。依赖 `scripts/ue/package_windows.py` 的 cook/stage 流程与 `Config/DefaultGame.ini` 的打包设置。
- 决策记录：
  - 候选方案 A（推荐先验证）：在 `package_windows.py` 的 cook 之后、stage/archive 之前，显式把 `Content/Data/*.csv` 拷贝进暂存目录 `ReEcho/Content/Data/`。优点：绕开 cook 对非资产目录的枚举盲区，最稳；不依赖 ini 键是否被采纳。代价：打包逻辑多一处脚本维护。
  - 候选方案 B：在 `/Game/Data` 放一个被 cook 引用的占位/Primary Asset（如 `UDataAsset` 或 `AssetManager` 主资产，其引用足以让 cook 枚举并 stage 该目录下的 csv）。优点：纯配置/资产层解决，符合 UE 资产范式。代价：需确认 csv 经 `FFileHelper` 仍按原物理路径可读（cook 后路径/挂载可能变化）。
  - 候选方案 C：依赖 `Config/DefaultGame.ini` 的 `DirectoriesToAlwaysStageAsNonUFS` / `FilesToAlwaysStageAsNonUFS`。已实证在本地三次打包中均未生效（NonUFS 计数恒为 43，6 张 csv 始终未进包），原因疑似纯 csv 目录不被 cook 枚举；除非能证明确为增量 cook 缓存所致、clean cook 可生效，否则不作为主方案。
  - 选择：实现者按 A→B→C 顺序验证，以“包内出现全部 28 张 csv 且 exe 启动不崩”为通过判据，命中其一即停。
- 相关文档同步范围：
  - `ARCHITECTURE.md`：若数据链末端打包契约发生变化则更新；否则审阅说明。
  - `README.md`：路由无变化，审阅说明。
  - `modules/MOD-ReEcho.md`：补充/确认运行时 CSV 打包契约（列入 Writes）。
- 关闭前逐项填写审阅结果：
  - `<文档>` 已更新：列出与验收实现一致的具体变化；
  - `<文档>` 已审阅、无需修改：说明该实现为何不改变此文档中的事实。

## 锁定验收

- [ ] Shipping 包可双击运行，启动后无 `LowLevelFatalError` 崩溃（包内 `Content/Data` 含 `stages.csv` 等全部运行时表）。
- [ ] `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` 通过（发布门禁，推 main 前必跑）。
- [ ] `python scripts/validate_project.py` 通过（静态不变量 / CSV schema / UTF-8）。
- [ ] 未提交 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。
- [ ] 人工验收 `PendingBeforeClose`：在目标机器实际启动打包 exe 确认不闪退，记录结果或明确延期跟进。

## Step 0 门禁

- 基线分支/提交：发布后从 `origin/main` 最新提交建立实现分支（当前 `origin/main` 领先本地 4 个提交，须先解决基线冲突再发布/实现）。
- 引擎/构建可用性：UE 5.8 位于 `C:\Program Files\Epic Games\UE_5.8`；`package_windows.py` 可定位引擎并跑 `RunUAT BuildCookRun`。
- 现有聚焦测试结果：闪退根因已定位（见执行记录）；尚未有自动化覆盖“包内 CSV 完整性”。
- 共享契约 / 难合并资源风险：远端 4 个提交改动 `Binaries/Win64/UnrealEditor-*.dll` 等构建产物，与本地未提交二进制改动同文件；发布 Plan 前须先 `git fetch` + 基线对齐（见审计），避免覆盖/冲突。
- 基线损坏时的停止条件：若对齐后发现 Plan52/55 内容与本 Plan 修复点（ReEcho2DEditorPreviewActor.cpp、打包脚本/配置）产生逻辑冲突，停止并报告。

## 实现提纲

1. 先解决基线：发布 Plan 前 `git fetch`，确认本地 `main` 与 `origin/main` 对齐（fast-forward 或按规定处理冲突），恢复/提交本地 `ReEcho2DEditorPreviewActor.cpp` 修复与一致二进制。
2. 实现 Shipping 编译修复：`ReEcho2DEditorPreviewActor.cpp:15` 用 `#if WITH_EDITOR` 包裹 `SetIsVisualizationComponent(true);`，保证 Shipping 下不调用仅编辑器 API。
3. 实现运行时 CSV 入包：按决策记录 A→B→C 验证，确保 `Content/Data` 全部 28 张 csv 进入 Shipping 包。
4. 每次风险变更后立即验证（本地增量构建 + 一次 Shipping 打包 + 启动 exe）。
5. 更新执行记录；跨越变化边界前在本地记录范围/契约变化。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | 项目/源码不变量、CSV schema、UTF-8 通过 |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0；`ReEcho2DEditorPreviewActor.cpp` 在 Development 与 Shipping 均编过 |
| 发布门禁 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` + 刷新精选预构建包 | 推 main 前通过 |
| 打包 | `python scripts/ue/package_windows.py --output <dir> --no-smoke` | `BUILD SUCCESSFUL`，包内 `ReEcho/Content/Data/` 含全部 28 张 csv |
| 运行时行为 | 启动打包 exe（无 `-nullrhi` 真实窗口） | 不再出现 `ReEcho.cpp` 的 `LowLevelFatalError`；无退出码 3 崩溃 |

## 执行记录

### 变化

- 已定位：Shipping 编译失败 `ReEcho2DEditorPreviewActor.cpp(15,20): error C2039`，`SetIsVisualizationComponent` 非 `UChildActorComponent` 成员（仅 `WITH_EDITOR` 存在）。
- 已定位：Shipping 包启动即崩，`ReEcho.cpp:19` `LowLevelFatalError` 报告 `Content/Data/stages.csv:0:File: File could not be read`；包内 `Content/Data` 仅 22 张 csv，缺 6 张（`stages`/`attributes`/`encounters`/`encounter_waves`/`spawn_policy`/`spawn_profiles`）。
- 已实证：`Config/DefaultGame.ini` 的 `DirectoriesToAlwaysCook` / `DirectoriesToAlwaysStageAsNonUFS` / `FilesToAlwaysStageAsNonUFS`（逐文件 28 项）三次打包均未使缺失 csv 进包（NonUFS 计数恒 43）。
- 已有本地未提交修复：`ReEcho2DEditorPreviewActor.cpp` 加 `#if WITH_EDITOR` 包裹（Shipping 编译已在本机验证通过）。
- 已实现（方案 A）：`ReEcho2DEditorPreviewActor.cpp` 的 `#if WITH_EDITOR` 包裹已纳入 `plan/59-shipping-build-viability` 实现分支，Development 增量构建通过（含该 cpp 重编）。
- 已实现（方案 A）：`scripts/ue/package_windows.py` 增加 `stage_runtime_csvs()`，在 `BuildCookRun` archive 之后把 `Content/Data/*.csv` 显式拷贝进最终包 `ReEcho/Content/Data/`（排除 `Engine` 目录下的 Content），覆盖 manifest 全部 28 张；未走方案 B/C（方案 A 一次通过即停）。

### 证据

- 崩溃报告：`%LOCALAPPDATA%\ReEcho\Saved\Crashes\UECC-Windows-*\CrashContext.runtime-xml` 记录 `ReEcho.cpp` 行 19 `LowLevelFatalError` 与缺失文件。
- 包内文件比对：源 `Content/Data` 28 张 csv vs 包内 22 张，差集为上述 6 张。
- Shipping 编译：加 `#if WITH_EDITOR` 后 `package_windows.py` 首次 `BUILD SUCCESSFUL`（ExitCode=0）。
- 打包实测（plan/59 分支，`scripts/ue/package_windows.py --output c:/tmp/ReEchoPlan59Pkg`）：`BUILD SUCCESSFUL` 无 C2039；日志 `Staged 28 runtime CSV(s) into 1 Content/Data dir(s)`；包内 `Windows/ReEcho/Content/Data` 含全部 28 张 csv（含原缺 6 张），`Engine/Content/Data` 无 stray。
- 启动验证：干净包 `ReEcho.exe` 在 Windows 直接运行，用户手动关闭（ExitCode=0，非崩溃退出码 3）；`ReEcho.cpp:19` 启动校验 `LowLevelFatalError` 未触发。

### 剩余风险

- `Config` ini 键为何对纯 csv 目录失效：未彻底定性，但**已实现走方案 A 显式拷贝，不再依赖 ini 机制**，该不确定性已绕开，不阻塞发布。
- 本地二进制（DLL/target/modules）冲突：已在发布前于主工作树基线对齐（`merge --ff-only origin/main`），实现在 `plan/59` 工作树进行，主工作树保持 main 干净。

### 人工验收结果/请求

- 已在本机实际启动 Shipping 包（`c:/tmp/ReEchoPlan59Pkg/Windows/ReEcho.exe`）：窗口正常显示，用户手动关闭，ExitCode=0，**无 `LowLevelFatalError` 闪退**；包内 `Content/Data` 28 张 csv 齐全。验收项已通过（PendingBeforeClose 关闭前由本机启动验证替代）。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：已补充「运行时 CSV 松散文件与 Shipping 打包契约」一节（数据链末端打包投递从隐式 cook 暂存转为显式 `package_windows.py::stage_runtime_csvs` 拷贝）。
- `shared/CODEBASE_MAP/ARCHITECTURE.md`：路由无变化，未修改（仅补充 MOD-ReEcho 一节内容）。
- `shared/CODEBASE_MAP/README.md`：路由无变化，未修改。
