# Plan 102 - 程序 - 战斗 HUD 计时指针与小地图角色图标

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner 与 Executor）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Closed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`Passed`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：`origin/main@55c4b4e427a16819c41bf562c0e3d853a6695b92`。
- 本地实现方式：一任务一 worktree，`C:/Users/gavynqiu/Documents/miniGame/ReEcho-plan102-combat-hud-timer-minimap-icons`，分支 `plan/102-combat-hud-timer-minimap-icons`。
- 依赖 / 阻塞：继承 Plan93 已验收的 `WBP_ReEchoEncounterHud` 构图、真实小地图投影与透明底板；用户提供 `C:/Users/gavynqiu/Documents/miniGame/拆分_无出血线_512x512/拆分_无出血线_512x512` 下 8 张透明头像作为四个正式角色的 Player/Echo 小地图图标。Plan63 本地未提交候选正在同一 Presentation Profile 头文件追加死亡脚点字段，Plan101 本地候选正在修改 `ReEchoGameMode` 商店段；本 Plan 使用独立 worktree，发布前按实际 `main` 组合适配，不覆盖其语义。
- Writes:
  - `plans/102-combat-hud-timer-minimap-icons.md`
  - `Content/ReEcho/UI/WBP_ReEchoEncounterHud.uasset`
  - `Content/SourceArt/UI/CombatHud/Plan102/**`
  - `Content/SourceArt/UI/CombatHud/Plan93/{README.md,_SourceManifest.csv}`（同步废弃倒计时黑底的可追溯状态）
  - `Content/ReEcho/Textures/UI/CombatHud/Minimap/**`
  - `Content/ReEcho/Textures/UI/CombatHud/T_UI_CombatHud_TimeReadout.uasset`（删除废弃运行时纹理）
  - `Content/ReEcho/DataAsset/Character/Profiles/DA_Character_J_{HEART,SPADE,CLOVER,DIAMOND}.uasset`
  - `Content/ReEcho/DataAsset/Character/Profiles/DA_Echo_J_{HEART,SPADE,CLOVER,DIAMOND}.uasset`
  - `Source/ReEchoPresentation/Public/Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h`
  - `Source/ReEcho/{Public,Private}/Encounter/ReEchoEncounterDirector.*`
  - `Source/ReEcho/{Public,Private}/UI/ReEchoEncounterHudWidget.*`
  - `Source/ReEcho/{Public,Private}/UI/ReEchoMinimapCanvasWidget.*`
  - `Source/ReEcho/{Public,Private}/Player/ReEchoPlayerPawn.*`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoEchoActor.*`
  - `Source/ReEcho/{Public,Private}/ReEchoGameMode.*`
  - `Source/ReEcho/Private/Tests/ReEchoCombatHudTests.cpp` 及实际新增/维护的聚焦测试
  - `scripts/ue/import_plan93_combat_hud.py`、`scripts/ue/author_plan93_combat_hud.py`、`scripts/ue/audit_plan93_combat_hud.py`（同步当前正式 HUD，不再恢复废弃底块）
  - `scripts/ue/import_plan102_minimap_icons.py`、`scripts/ue/audit_plan102_combat_hud.py`
  - `Design/UI/ReEcho_UI修改指导.md`
  - `docs/ART_ASSET_ORGANIZATION.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`
  - `Binaries/Win64/` 下 `shared/GIT_RULES.md` 允许的最终精选预构建包
- Stable Reads:
  - `plans/93-encounter-hud-ui.md` 与 `Content/SourceArt/UI/CombatHud/Plan93/**`
  - `Source/ReEcho/{Public,Private}/UI/ReEchoPlayerHudWidget.*`
  - `Source/ReEcho/{Public,Private}/Recording/**` 的 Echo 录制角色身份
  - `Content/Data/characters.csv` 的稳定 `CharacterId` 与现有 Character/Echo Presentation Catalog
  - `Source/ReEcho/Public/Core/ReEchoBalanceSettings.h`
  - `Source/ReEcho/Private/Tests/ReEchoEncounterDirectorTests.cpp`
- 影响模式：`SharedContract`。修改 `FReEchoMinimapView` 的只读表现数据、Encounter HUD 状态投影与角色 Presentation Profile 的 UI 图标字段；`WBP_ReEchoEncounterHud` 是不可文本合并的二进制资产，须串行 authoring。
- 兼容承诺 / 下游操作：Encounter Director 继续唯一拥有表驱动遭遇时钟；Widget 只读取剩余/总时长并计算表现角度。小地图继续保留同一坐标变换和 Echo 轨迹折线；图标缺失时保留旧点状 fallback，不影响玩法或流程。稳定 `CharacterId`、录制、存档、战斗和 Presentation Catalog 选择不变。
- 明确排除：不修改遭遇真实时长、结束条件、暂停/固定步语义、玩家或 Echo 世界表现、轨迹采样、竞技场投影、生命/时间碎片、商店、输入、存档 Schema、其他页面布局；不重新设计顶部时钟底板或主观调整未具名 HUD 元素。

## 锁定目标

1. 从 `WBP_ReEchoEncounterHud` 移除倒计时文字后的黑色半透明 `ArtTimeReadout` 底块；对应运行时 Texture2D 不再保留，Plan93 原始 SourceArt 继续用于历史追溯。
2. 顶部中央 `ArtClockNeedle` 在每场遭遇开始时指向右侧，随权威倒计时连续沿下半圆转向左侧；表现进度为 `0..180°`，现有竖直源图因初始朝下而使用 `-90..+90°` Render Transform 映射，半程竖直向下，剩余时间为 0 时到达左侧。
3. 右上角小地图不再用纯色方点表示玩家与 Echo。四个角色按稳定 `CharacterId` 使用用户提供的对应透明头像；Player 使用正常肤色版本，Echo 使用暗色金眼版本。Echo 轨迹线与真实位置投影继续保留。
4. 图标资产以 8 张原始 `512×512` PNG 归档、哈希和语义映射为可追溯来源，运行时导入到稳定 ASCII 路径并通过 Character/Echo Presentation Profile 硬引用，保证 cook 可见。
5. HUD 不接管输入、不写入权威时钟或角色状态；缺少图标或总时长非法时安全降级，不造成崩溃或流程变化。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`AREA-UI` / 文档型 `MOD-ReEchoUI`、`MOD-ReEchoPresentation`。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoUI.md`、`MOD-ReEchoPresentation.md` 均已加入 `Writes`。
- 设计意图：Encounter Director 继续提供权威时钟事实，GameMode 只把总时长/剩余时长与角色表现图标投影给 HUD；WBP 拥有指针枢轴和布局，C++ 拥有归一化进度与只读刷新；角色 Profile 拥有 Player/Echo 对应图标，Minimap 不重复维护 `CharacterId -> Texture` 映射。
- 权威状态与依赖：新增 Presentation Profile 的硬引用 `MinimapIcon` 表现字段，不改变角色/录制身份；`ReEcho` 已依赖 `ReEchoPresentation`，依赖方向不变。`FReEchoMinimapView` 增加非权威纹理指针，生命周期由加载中的 Profile/Actor 硬引用保证。
- 决策记录：未采用“Minimap 内硬编码 8 个路径”，避免 UI 建立第二份角色身份真源；未采用固定 30 秒计算，避免覆盖表驱动 Encounter Duration。指针源图默认向下，因此用 `-90..+90°` 表示用户所述从右到左 `0..180°`，并把 Render Pivot 设在源图顶部圆轴心。
- 相关文档同步范围：`MOD-ReEcho.md` 维护遭遇时钟/小地图只读投影；`MOD-ReEchoUI.md` 与 UI 修改指导维护指针和图标契约；`MOD-ReEchoPresentation.md` 维护 Profile UI 图标职责。关闭前审阅 `CODEBASE_MAP/ARCHITECTURE.md` 和 `README.md`，预期因模块拓扑和路由不变而无需正文修改。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅；Runtime Module 拓扑、依赖方向和权威状态流未变，无需修改。
  - `shared/CODEBASE_MAP/README.md`：已审阅；现有 `AREA-UI` / `AREA-Presentation` 路由仍正确，无需修改。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：已维护 Encounter 总时长与 Profile 小地图图标的只读 HUD 投影。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：已维护黑底删除、指针旋转、透明小地图与 Profile 图标契约。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`：已维护 `MinimapIcon` 职责、Cook 引用与缺图降级边界。
  - `Design/UI/ReEcho_UI修改指导.md`：已维护 Plan93/102 战斗 HUD 当前构图与 SourceArt 路由。

## 锁定验收

- [x] 当前 WBP 中 `ArtTimeReadout` 不存在，倒计时文字后无黑色半透明底块，废弃运行时纹理不在 Content 中。
- [x] 指针按 `Remaining / Duration` 连续计算：满时位于右侧、半时竖直向下、0 时位于左侧；暂停时权威时间不推进，因此指针不推进。
- [x] 四个 `CharacterId` 的 Player/Echo Profile 分别绑定正确的 8 张头像；小地图玩家和每个 Echo 使用各自 Profile 图标替代方点，轨迹线继续显示。
- [x] 图标缺失、无效 Arena 或非法 Duration 安全降级；不改变 Encounter、Recording、Combat、Run 或输入语义。
- [x] WBP Compile/Save、`CompileAllBlueprints`、Texture/Profile/Widget 资产审计与聚焦自动化通过。
- [x] 修改源码按 `.clang-format` 格式化，最终组合候选通过 Development `-FullRebuild`、项目校验和 `git diff --check`。
- [x] 用户在 1920×1080 运行画面确认底块、指针方向/进度、Player/Echo 图标大小和可读性。
- [x] 未提交精选 `GIT_RULES.md` 允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@55c4b4e427a16819c41bf562c0e3d853a6695b92`；编号前远端最大 Plan 为 101，本任务使用 102。
- 引擎/构建可用性：UE 5.8 安装版可用；基线 Plan93 资产审计退出 0，当前主工作区与任务 worktree 均干净。执行独占 Editor 命令前使用 Git-common-dir 锁并确认无交互式 Editor 进程。
- 现有聚焦测试结果：基线 `ReEcho.UI.CombatHud.Formatting` 与 Minimap Transform 已覆盖格式、绑定和坐标变换；本 Plan 补充指针角度、Profile 图标与 Slate 图标 fallback 覆盖。
- 共享契约 / 难合并资源风险：`WBP_ReEchoEncounterHud` 当前没有其他 worktree 未提交修改。Plan63 本地候选与本 Plan同改 Character Presentation Profile 头文件但字段位于不同职责段，最终组合时必须保留双方字段并重跑构建/资产审计；Plan101 与旧 shop 修复候选修改 GameMode 的商店段，本 Plan修改 HUD/Minimap 段，发布前仍须审计文本与精选二进制组合。
- 基线损坏时的停止条件：远端 `main` 出现批准基线外提交；目标 WBP 被其他任务修改；8 张图标语义无法与四个稳定 CharacterId 一一确认；实现需要改变权威遭遇时长、录制身份、存档或模块依赖方向；独占 Editor 会话未保存关闭。

## 实现提纲

1. 归档 8 张 PNG，生成尺寸/SHA-256/`CharacterId + Player|Echo` 映射清单；通过 Unreal Editor 导入 UI Texture2D，并写入对应 Character/Echo Presentation Profile。
2. 扩展 Presentation Profile 的 `MinimapIcon` 表现字段，由 Player/Echo Actor 提供只读 getter，GameMode 将纹理随当前位置投影进 `FReEchoMinimapView`。
3. 让 Slate Minimap 使用纹理 Brush 绘制 Player/Echo 图标，保留轨迹线与缺图 fallback 点；增加纯函数/聚焦自动化。
4. 扩展 Encounter HUD 状态投影以携带总时长，绑定 `ArtClockNeedle`，计算 `-90..+90°` 角度；在 WBP 设置顶部轴心 Pivot，移除 `ArtTimeReadout` 并同步 Plan93 幂等 authoring/audit 脚本。
5. 执行导入/作者ing幂等复跑、资产审计、WBP 编译、聚焦自动化、C++ 构建与静态检查；交用户进行 1920×1080 视觉验收后再关闭/发布实现。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 源图/导入 | Plan102 manifest SHA-256；Editor 导入与 Profile 审计 | 8 张 512×512 透明 PNG 映射正确，Texture2D 为 UI 配置且 Profile 硬引用 |
| 纯逻辑 | `ReEcho.UI.CombatHud.Formatting`、`ReEcho.UI.Minimap.Transform` 及新增图标/指针断言 | 满/半/零时角度正确，坐标变换和缺图 fallback 无回归 |
| WBP/资产 | Plan93/Plan102 authoring 与 audit 幂等复跑；`CompileAllBlueprints` | WBP 0 errors、0 warnings、0 failed loads；废弃底块不存在，Needle Pivot/资源/图标绑定正确 |
| C++ | 对修改文件运行 `.clang-format`；实现阶段 `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 成功并刷新匹配精选包 |
| 静态 | `python scripts/validate_project.py`；`git diff --check` | 项目和差异通过 |
| 发布 | 最终组合候选执行 `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UE 5.8 完整重建并刷新允许列表预构建产物 |
| 人工 | 1920×1080 普通战斗：满时/半时/最后 5 秒/归零，0/1/多 Echo | 用户确认无黑底、指针从右经下方转到左、各角色 Player/Echo 图标可辨且不遮挡轨迹 |

## 执行记录

### 变化

- 将 8 张透明 Player/Echo 头像归档到 Plan102 SourceArt，生成稳定语义映射、字节数与 SHA-256 清单，导入 8 个 UI Texture2D 并绑定四组 Character/Echo Presentation Profile。
- Presentation Profile 新增 Cook 可见 `MinimapIcon`；Player/Echo Actor 只读提供活动 Profile 图标，GameMode 将其与玩家/Echo 位置投影到透明 Slate 小地图。图标以 34/30 px 居中绘制，缺图时保留旧色点，Echo 轨迹折线不变。
- Encounter Director 暴露当前表驱动总时长，Encounter HUD 按剩余/总时长计算 `-90..+90°` Render Angle；WBP 把指针 Pivot 设在顶部轴心，移除 `ArtTimeReadout` 及其废弃运行时纹理。
- 同步 Plan93 导入/authoring/审计脚本与 SourceArt 清单，确保重建当前 HUD 时不会把黑底或底部废案面板恢复。
- 更新 UI、主模块、Presentation 与美术资产文档；全局模块拓扑和索引路由未变。

### 证据

- 用户确认采用远端新基线后，本地 `main` 已从 `ea214053` 安全快进到 `origin/main@55c4b4e4`，无本地独有提交或未提交文件。
- 编号前 fetch 并从 `origin/main` 枚举 Plan，最大编号为 101。
- 已直接审计当前 `WBP_ReEchoEncounterHud`：`CountdownText`、`ArtClockNeedle` 与 `ArtTimeReadout` 均存在；`ArtTimeReadout` 是倒计时后的独立半透明底图，Needle 当前无运行时旋转绑定。
- 已读取 Encounter HUD、Minimap、Encounter Director、GameMode HUD 投影、Player/Echo Actor、Presentation Profile、聚焦测试与三个相关模块文档；确认总时长来自表驱动 Director，现有小地图只绘制方点。
- 已视觉核对 8 张外部 PNG：`01/02/03/04` 分别为 Clover/Spade/Heart/Diamond 的 Echo 暗色金眼头像；`05/06/07/08` 分别为 Diamond/Spade/Clover/Heart 的 Player 正常头像。
- `import_plan102_minimap_icons.py` 导入并绑定 8 个 Profile；`audit_plan102_combat_hud.py` 最终退出 0，核对 8 张源图哈希/尺寸、Texture UI 设置、精确 Profile 绑定、`has_readout=False` 和 Needle Pivot `(0.5, 0.12)`。
- Plan93 authoring 复跑后结构审计退出 0：Player HUD 18 个、Encounter HUD 9 个 Widget，`ArtSkillBar` / `ArtTimeReadout` 与两个废弃运行时纹理均不存在。authoring 每次 Compile/Save 可重写 WBP 编译元数据，幂等性以结构审计而非二进制字节哈希证明。
- `CompileAllBlueprints` 完成，0 errors、0 warnings、0 failed loads；存在 6 条基线启动/旧资产 warning，无 Blueprint Compile warning。
- `ReEcho.UI.CombatHud.Formatting` 与 `ReEcho.UI.Minimap.Transform` 聚焦自动化均找到 1 个测试并以 `Result={Success}` 完成；前者覆盖满/半/零/越界/非法时长角度、WBP 绑定与 8 个 Profile 图标非空。
- Visual Studio LLVM `clang-format 19.1.5` 已格式化 12 个改动 C++ 文件；最终 `Build-Editor.cmd -Configuration Development -FullRebuild` 完成 101 个 action，`Result: Succeeded`，刷新 7 个允许的 Editor 预构建模块，source fingerprint `2eef0e06b428`。
- `python scripts/validate_project.py` 和 `git diff --check` 最终通过；独占 Unreal 锁已释放，无遗留 Editor 进程。最终 fetch 确认 `origin/main@889e26cb` 仍是 Plan102 空计划提交，无新基线需组合。

### 剩余风险

- 指针 Pivot 和图标显示尺寸仍需在 1920×1080 PIE 中由用户主观验收；自动化只能证明角度、绑定和资产契约。
- Plan63/Plan101 等本地候选尚未进入远端；用户视觉验收后、实现发布前若 `main` 前进，必须重新审计源码、DataAsset 与精选二进制组合，并重跑最终验证。

### 人工验收结果/请求

- `Passed`：用户在运行画面验收后确认“看起来没什么问题”，并授权将候选推送/合并到远端 `main` 后清理本地任务 worktree 与工作分支。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅，本候选未新增模块、反转依赖或改变状态权威，不修改。
- `shared/CODEBASE_MAP/README.md`：已审阅，修改仍由现有 `AREA-UI`、`AREA-Encounter`、`AREA-Player`与 `AREA-Presentation` 索引正确路由，不修改。
- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoUI.md`、`MOD-ReEchoPresentation.md` 与 `Design/UI/ReEcho_UI修改指导.md`：均已同步当前候选的只读时钟/小地图图标契约。
