# Plan 145 - UI / 存档 - 正式存档回溯与三槽存档

## 协调

- Planner 负责人：Gavyn。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Complete`。
- 人工验收：`Passed`。
- 本地规划 / 实现基线：`origin/main@a334a04b`。
- 本地实现方式（可选，仅作交接说明）：Plan 发布后从最新 `origin/main` 创建独立 Plan 145 worktree；规划与执行由同一负责人完成。
- 依赖 / 阻塞：正式成图 `C:\Users\gavynqiu\Downloads\存档回溯.svg`、Figma CSS、`正式-UI视觉\存档回溯、设置、关于` 素材可读；UE 编辑器在资产写入时必须关闭并持有仓库 UE 锁。
- Writes:
  - `plans/145-formal-save-rollback-page.md`
  - `Source/ReEcho/Public/Run/ReEchoRunSaveGame.h`
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Source/ReEcho/Public/UI/ReEchoStartMenuWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoStartMenuWidget.cpp`
  - `Source/ReEcho/Public/ReEchoGameMode.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Tests/`
  - `Content/ReEcho/UI/WBP_ReEchoStartMenu.uasset`
  - `Content/ReEcho/Textures/UI/SaveRollback/Plan145/`
  - `Content/SourceArt/UI/SaveRollback/Plan145/`
  - `scripts/ue/` 中本任务新增或更新的导入、创作与审计脚本
  - `Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
- Stable Reads:
  - `C:\Users\gavynqiu\Downloads\存档回溯.svg`
  - `C:\Users\gavynqiu\Documents\miniGame\正式-UI视觉\正式-UI视觉\存档回溯、设置、关于\`
  - `C:\Users\gavynqiu\.codex\attachments\4ea20379-f47f-4534-b1ad-0a1afaaedb6f\pasted-text.txt`
  - 现有开始菜单、运行存档、设置页及按钮交互实现
  - `shared/GIT_RULES.md`、`shared/PLANNER_RULES.md`、`shared/EXECUTOR_RULES.md`
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：保留既有固定槽 `ReEchoRun` 的读档兼容，将其呈现/迁移为第 1 槽；设置、关于、退出、新游戏和既有运行内自动存档流程不得回归。保存截图缺失或损坏时必须使用正式占位图降级，不影响读档。
- 明确排除：本 Plan 不新增覆盖、删除、重命名、云同步、存档导入导出；三槽全满时仅展示三个已有存档，不显示“新建存档”，不擅自引入覆盖确认。

## 锁定目标

用正式视觉资产替换开始菜单中旧的单一“存档回溯=继续游戏”表现，同时将底层单槽运行存档扩展为三个相互独立的稳定槽位：

- 点击主菜单“存档回溯”进入与 `存档回溯.svg` / `1-存档回溯.png` 一致的正式面板，关闭后返回主菜单。
- 三个槽位各自独立保存运行状态、实际保存时间和真实游戏画面截图；占用槽展示“存档 1/2/3”、到达关卡、卡牌数量、时间戳和截图。
- 点击占用槽立即读取该槽并进入对应游戏进度。
- 空槽显示“新建存档”；点击第一个空槽后将其设为活动槽并沿用现有新游戏/构筑选择流程进入游戏，此后自动保存只写回该活动槽。
- 旧版固定槽 `ReEchoRun` 必须继续可见、可读取，并稳定归入第 1 槽；槽位之间不得串档、误删或互相覆盖。
- 正式 UI 的位置、尺寸、示例文字和图层在 `WBP_ReEchoStartMenu` 设计器中所见即所得；运行时代码只更新内容、可见性、启用状态、截图和交互，不在 C++ 中重建布局。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`（运行存档权威、活动槽、旧存档兼容、截图元数据）、`MOD-ReEchoUI`（开始菜单公共交互契约及正式存档回溯面板）。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`，两者均已加入 `Writes`。
- 设计意图：运行子系统继续作为存档状态权威，UI 只消费不可变槽摘要并发出“读取/新建”意图；截图文件与 SaveGame 中的相对引用绑定，避免把大体积像素数据塞入 SaveGame 对象。
- 权威状态与依赖：增加当前活动槽与三槽摘要公共契约，但不把权威状态转移到 Widget；GameMode 继续编排页面和进入游戏流程，Widget 不直接构造运行状态。
- 决策记录：采用三个稳定物理槽名和一个旧槽兼容入口；截图保存为每槽独立 PNG 并在 SaveGame 中保存相对标识与时间戳。缺图采用 `预览图占位.png`。三槽全满不隐式覆盖，因为用户未授权破坏性覆盖/删除行为。
- 相关文档同步范围：更新两个模块文档与 `Design/UI/ReEcho_UI修改指导.md`；审阅 `ARCHITECTURE.md` 和 `README.md`，若模块拓扑、依赖方向与入门流程未变化则记录“已审阅、无需修改”。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已更新：三槽权威、活动槽、旧槽兼容和真实截图契约已记录。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 已更新：正式面板、只读摘要和运行时不接管几何的交互契约已记录。
  - `Design/UI/ReEcho_UI修改指导.md` 已更新：可编辑节点、素材目录和所见即所得边界已记录。
  - `ARCHITECTURE.md` / `README.md`：已审阅；本次没有改变 Runtime Module 拓扑或项目入口，无需修改。

## 锁定验收

- [x] 主菜单“存档回溯”打开正式面板；布局、图层、正式底板、标题、关闭按钮和三行槽位在 1920x1080 下与成图一致，且在 WBP 设计器中可见、可调整。
- [x] 三个槽能独立保存与读取；依次新建多个槽后，关卡、构筑、截图、时间戳均不串槽。
- [x] 占用槽展示真实游戏截图、实际保存时间、正确关卡和卡牌数量；截图缺失/损坏时只降级为占位图，仍可读档。
- [x] 点击占用槽立即读档；点击“新建存档”使用第一个空槽并进入既有新游戏流程；三槽全满时不显示新建行且不覆盖已有存档。
- [x] 旧固定槽 `ReEchoRun` 能作为第 1 槽出现并成功读取，迁移失败不得删除旧文件。
- [x] 运行内后续自动保存只写回当前活动槽；删除/重置一个新游戏槽不得影响其他槽。
- [x] 设置、关于、退出、新游戏、键鼠/手柄焦点、悬停缩放与 UI 音效无回归。
- [x] 覆盖槽摘要、旧槽兼容、槽隔离与失败降级的自动化测试通过；WBP 编译/审计通过。
- [x] 功能结果有可观察证据。
- [x] 必需自动化/构建检查通过。
- [x] 视觉质量、可读性与真实截图由人工 PIE 验收通过后再关闭。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@a334a04b`；规划时本地 `main` 与远端一致且工作区干净。
- 引擎/构建可用性：实现 worktree 创建后执行 `python scripts/setup_lfs.py --check`；涉及 `.uasset` 前确认 UE 已关闭并获取 common-dir UE 锁；随后确认 Editor 构建、命令行自动化与资产审计可用。
- 现有聚焦测试结果：待实现 worktree 建立后先运行与 RunSubsystem、StartMenu、GameMode 相关的最小测试基线；失败时记录是否为基线问题。
- 共享契约 / 难合并资源风险：`WBP_ReEchoStartMenu.uasset` 为二进制高冲突资产；必须从最新主线开始，通过可重复脚本修改并在发布前重新审计远端变化。
- 基线损坏时的停止条件：LFS 指针异常、WBP 无法加载/编译、现有存档测试在未改代码前失败，或用户仍占用 UE 导致无法安全保存资产。

## 实现提纲

1. 建立三槽命名、活动槽、槽摘要、旧固定槽兼容和截图元数据的运行存档契约，保持既有自动存档调用方可用。
2. 由 GameMode 编排占用槽读取与空槽新建；在安全的游戏画面时机捕获并写入每槽 PNG，保存失败不改变当前活动槽或误进游戏。
3. 导入正式标题、浅/深存档底板和预览占位资产；在 `WBP_ReEchoStartMenu` 中作者化三行槽位、关闭按钮和样例内容，保证可自由编辑且运行时不重建几何。
4. 接通槽摘要、截图加载、点击/焦点/悬停反馈；清理被替代的历史 Continue 视觉，但保留必要的兼容绑定。
5. 增加 C++ 自动化、资产结构审计、文档和模块地图；执行构建、自动化、静态检查、WBP 编译与 PIE 人工验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目/源码不变量与补丁格式通过 |
| LFS / 资产前置 | `python scripts/setup_lfs.py --check` | 正式源图和项目二进制均非 LFS 指针占位 |
| C++ 构建 | `scripts/ue/Build-Editor.cmd`；发布前按规则完整重建 | UHT/UBT 退出码为 0 |
| 聚焦自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho` 或更窄的新增测试过滤器 | 三槽、兼容迁移、摘要和失败降级报告通过 |
| 资产审计 | Plan 145 WBP 创作/审计脚本 + UE 命令行加载编译 | 正式控件存在、命名/锚点/层级/示例内容符合契约且无运行时几何接管 |
| 人工 PIE | 新建 3 槽、跨槽读档、旧槽读取、真实截图、关闭返回、设置/关于回归 | 用户确认视觉和交互可接受 |

## 执行记录

### 变化

- 2026-08-28：用户确认三槽独立存档、旧存档保留、槽点击立即读取、空槽进入新游戏，并要求保存真实游戏画面截图。
- 2026-08-28：未获授权的全槽覆盖/删除明确排除；三槽全满时仅展示已有槽。
- 2026-08-28：SaveGame 升级为 v23，增加逻辑槽号、UTC 保存时间和预览文件名；Run 建立三个物理槽及活动槽，保留旧 `ReEchoRun` 第 1 槽兼容入口。
- 2026-08-28：GameMode 接通占用槽立即读取、空槽选择第一个空槽、新游戏全槽保护，以及稳定游戏画面的下一帧真实视口 PNG 捕获。
- 2026-08-28：正式存档回溯素材已导入，`WBP_ReEchoStartMenu` 已作者化 `StartMenuPanel` / `SaveRollbackPanel` 和三行可编辑槽位；运行时只更新摘要、状态与截图。
- 2026-08-28：修复跨局商店报价重复：SaveGame 升级为 v24，新增一次生成并持久化的统一 `RunSeed`；商店武器/符文、商店卡组、战后选卡和敌人碎片掉落均从本局种子派生，旧档从既有卡牌种子确定性迁移。
- 2026-08-28：用户完成存档回溯蓝图最终微调并确认可以推送、合入远端主分支，Plan 145 人工验收通过。

### 证据

- 规划基线：`origin/main@a334a04b`。
- 已检查现有固定槽 `ReEchoRun`、`UReEchoRunSaveGame`、`UReEchoRunSubsystem`、开始菜单 Widget 与 GameMode 进入流程。
- 已检查正式 SVG、Figma CSS 和 `1-存档回溯.png`，确认目标为两条占用示例加一条新建示例的三行结构。
- `scripts/ue/Build-Editor.ps1`：Development Editor 构建通过。
- `scripts/ue/Run-Automation.ps1 -Filter ReEcho.Run.SaveSnapshot`：找到 1 项并通过，覆盖槽选择、v23 元数据、截图路径和序列化往返。
- `scripts/ue/Run-Automation.ps1 -Filter ReEcho.Shop.FreshRunsUseDistinctOfferSeeds`：找到 1 项并通过，连续 16 个相同角色/武器的新局产生多个不同根种子和多个不同的首屏武器/符文商店组合。
- `scripts/ue/Run-Automation.ps1 -Filter ReEcho.Run.SaveSnapshot`：RunSeed v24 持久化、读档恢复及 v23 确定性迁移回归通过。
- `python scripts/validate_project.py` 与 `git diff --check`：通过。
- `scripts/ue/audit_plan145_start_menu_tree.py`：`[Plan145Audit] PASS`，正式节点名称、类型、父级、默认可见性和所见即所得层级通过。

### 剩余风险

- 真实截图已经延迟到 Encounter 激活后的下一帧捕获；仍需 PIE 人工确认不同分辨率下画面内容和落盘时机。
- 自动化覆盖元数据和序列化；旧槽文件的真实磁盘兼容、三槽连续新建与跨槽画面仍需 PIE 人工确认。

### 人工验收结果/请求

- `Passed`：用户完成蓝图细节微调并确认手测结果可合入远端主分支。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：已同步 SaveVersion 23、三槽物理命名、活动槽、旧槽兼容和截图契约。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：已同步正式 WBP 节点、只读槽摘要和 C++ 不接管几何的边界。
- `Design/UI/ReEcho_UI修改指导.md`：已同步正式素材路径、可编辑控件与运行时更新范围。
- `ARCHITECTURE.md` / `README.md`：已审阅；模块拓扑和项目入口不变，无需修改。
