# Plan 135 - 程序 - 结算角色立绘按所选角色驱动

## 协调

- Planner 负责人：Codex（程序路线）。
- Executor 负责人：Codex（程序路线）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Proposed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（结算角色立绘为视觉改动，需 PIE 各角色死亡/胜利对照）。
- 本地规划 / 实现基线：`origin/main@847283f32d54d2d76628730f2666af72b28cb756`。
- 本地实现方式（可选，仅作交接说明）：`plan/135-settlement-character-illustration`，`ReEcho-plan135-settlement-character-illustration` 独立 worktree。
- 依赖 / 阻塞：需为四个 `CharacterId`（J_HEART / J_SPADE / J_CLOVER / J_DIAMOND）各提供一张"失败/结算"立绘（及可选的"胜利"立绘）；美术由 Designer 提供，或过渡期复用 `LoadoutSelection` 的 `Selected` 角色图。需确认是否连胜利一起改（本 Plan 默认两者都改，死亡为主）。
- Writes：`plans/135-settlement-character-illustration.md`；`Content/ReEcho/UI/WBP_ReEchoRestart.uasset`；`Content/ReEcho/Textures/UI/Formal/RoundDefeat/**`（新增每张角色失败立绘）；`Content/ReEcho/Textures/UI/Formal/RoundVictory/**`（可选每张角色胜利立绘）；`Content/SourceArt/UI/Formal/RoundDefeat/**`、`Content/SourceArt/UI/Formal/RoundVictory/**`（新增源图）；必要的 `scripts/ue/**` 导入/审计脚本；`Source/ReEcho/Public/UI/ReEchoRestartWidget.h`；`Source/ReEcho/Private/UI/ReEchoRestartWidget.cpp`；`Source/ReEcho/Private/ReEchoGameMode.cpp`；`Source/ReEcho/Private/Tests/ReEchoRestartWidgetTests.cpp`；`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`。
- Stable Reads：`Source/ReEcho/Public/UI/ReEchoRestartWidget.h`；`Source/ReEcho/Private/UI/ReEchoRestartWidget.cpp`；`Source/ReEcho/Private/ReEchoGameMode.cpp` 的结算调用点（`ReEchoGameMode.cpp:3231` 附近）；`Source/ReEcho/Public/Run/ReEchoRunSubsystem.h` 与 `Private/Run/ReEchoRunSubsystem.cpp` 的 `StartRun`/`CurrentBuild.CharacterId`；`Source/ReEcho/Private/UI/ReEchoLoadoutSelectionWidget.cpp` 的 `LoadoutTexturePath`/`CharacterTexturePath` 约定（`ReEchoLoadoutSelectionWidget.cpp:567`）；`Design/UI/ReEcho_UI修改指导.md`；`Content/SourceArt/UI/Formal/RoundVictory/_SourceManifest.csv`、`Content/SourceArt/UI/Formal/RoundDefeat/_SourceManifest.csv`。
- 影响模式：`SharedContract`（`WBP_ReEchoRestart` 六种状态共用，高冲突二进制资产）。
- 兼容承诺 / 下游操作：保护普通暂停、两种退出确认、胜利、重开与返回主菜单流程；保持既有 `BindWidgetOptional` 名称与类型；未知/缺失立绘时回退既有红帽图，不破坏任何状态或伪造数据。
- 明确排除：不新增第二套结算 WBP；不改 Combat / Run / 存档 / 奖励 / 音乐 / 关卡推进逻辑；不为未知角色伪造任何统计数据；不把整图当作不可编辑的全屏点击层；不重做普通暂停或退出确认页面。

## 锁定目标

在保持现有胜利、失败、重开和返回主菜单逻辑一致的前提下，让结算界面（失败与胜利）展示"本轮实际所选角色"的立绘，而不是固定红帽。

- 死亡时按 `RunSubsystem->CurrentBuild.CharacterId` 解析出对应角色立绘，替换 `DefeatCanvas` 内的角色图。
- 胜利时同样按所选角色出图（本 Plan 默认胜利也一并数据驱动；若仅先做死亡，胜利保持现状亦可，但应留同一扩展点）。
- 红帽角色（J_HEART 或当前默认主角）行为与现状完全一致：结算红帽 == 开头选中的红帽。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoUI`、`AREA-UI`。不修改公开 Runtime API；仅为既有 `WBP_ReEchoRestart` 增加按 `CharacterId` 选择角色立绘的可选绑定和运行时投影。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`，已加入 `Writes`。
- 设计意图：结算角色图由"写死红帽"变为"数据驱动所选角色"，与选角阶段（Plan 132 `LoadoutSelection` 已按 `CharacterId` 出 `Selected`/`Unselected` 图）保持一致；为后续手工微调保留所见即所得的 1920×1080 设计面。
- 权威状态与依赖：`GameMode` 已持有 `RunSubsystem`；`CurrentBuild.CharacterId` 在 `StartRun` 经 `ResolveStartingBuild` 写入（`ReEchoRunSubsystem.cpp:1304` `Result.Build.CharacterId = Character->Id`，随后进入 `CurrentBuild`），死亡/胜利时可直接读取，**无需新增任何存储字段**。
- 决策记录：
  1. 复用 `WBP_ReEchoRestart`，不新增独立结算页面，以免分叉暂停/重开流程。
  2. 数据传递：在 `ReEchoGameMode.cpp` 调用 `SetDeathScreen` / `SetVictoryScreen` 处，由 `RunSubsystem->CurrentBuild.CharacterId` 取出 `CharacterId` 传入（倾向由 `GameMode` 传入，保持 `RestartWidget` 不直接依赖 `GameInstance` 单例）。`SetDeathScreen` 新增 `FName InCharacterId = NAME_None` 形参，`SetVictoryScreen` 同理。
  3. 资源：为四个 `CharacterId` 各新增一张失败立绘，命名 `T_UI_Settlement_Defeat_Character_J_<SUIT>.png`，置于 `Content/SourceArt/UI/Formal/RoundDefeat/` 并导入 `/Game/ReEcho/Textures/UI/Formal/RoundDefeat/`。胜利立绘可选：`T_UI_Settlement_Victory_Character_J_<SUIT>.png`。过渡方案：若 Designer 暂只出红帽正式图，其余角色可先复用 `LoadoutSelection` 的 `Selected` 角色图（`LoadoutTexturePath("Character", CharacterId, "Selected")`），红帽仍用既有 `VictoryCharacter`/`DefeatCharacter`；最终由各角色独立正式立绘替换。
  4. 绑定：`DefeatCanvas` 内新增可选 `UImage`（如 `DefeatCharacterImage`，命名前缀 `ArtDefeat*`），运行时按 `CharacterId` 设置 Brush；`VictoryCanvas` 内 `VictoryCharacterImage` 同理。缺失时回退既有写死红帽图。
  5. 解析：新增 `SettlementCharacterTexturePath(CharacterId, bVictory)` 映射到上述纹理，与 `LoadoutSelection` 的 `LoadoutTexturePath` 约定一致（`/Game/ReEcho/Textures/UI/...`）。
  6. 失败页依旧只投影真实到达关卡、构筑数量与时间碎片；不伪造任何缺失来源的数据。
- 相关文档同步范围：审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `README.md`；维护 `MOD-ReEchoUI.md` 的结算角色立绘事实；若拓扑与稳定路由不变则只在执行记录中写明无需修改。
- 关闭前逐项填写审阅结果：待实现后补充。

## 锁定验收

- [ ] 选 J_HEART / J_SPADE / J_CLOVER / J_DIAMOND 中任一带到死亡，结算失败页角色立绘 = 该角色，不再是固定红帽。
- [ ] 选红帽角色死亡，结算角色立绘 = 红帽（与现状一致）。
- [ ] 胜利页同理按所选角色出图（若本 Plan 含胜利分支）。
- [ ] 未知 / 缺失立绘时回退红帽，不崩溃、不影响暂停/退出确认/重开/返回主菜单。
- [ ] `WBP_ReEchoRestart` 编译通过；`RootPanel`/`TitleText`/按钮及既有 Victory/Defeat 美术绑定名称与类型保持可加载。
- [ ] `SetDeathScreen` / `SetVictoryScreen` 调用点（`ReEchoGameMode.cpp:3231`）更新，并扩展 `ReEchoRestartWidgetTests.cpp` 增加"按角色立绘"断言。
- [ ] `python scripts/validate_project.py`、聚焦 `ReEcho.UI.Restart` 自动化与 `CompileAllBlueprints` 通过。
- [ ] 使用 Unreal MCP/Editor 截图对照，并由用户完成最终视觉与可用性人工验收。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@847283f32d54d2d76628730f2666af72b28cb756`。
- 引擎/构建可用性：UE 5.8 安装版；Editor/命令必须经 Git common-dir Unreal 锁串行化。
- 现有聚焦测试结果：实施前记录 `WBP_ReEchoRestart` 加载/编译与 `ReEcho.UI.Restart` 自动化基线。
- 共享契约 / 难合并资源风险：`WBP_ReEchoRestart.uasset` 是同一页面六种状态的二进制资产，属高冲突共享资产；发布前必须审计远端同路径变化并重新验证所有状态。
- 基线损坏时的停止条件：WBP 当前无法加载/保存、稳定绑定已在远端改变、Unreal 锁由其他活动进程持有，或立绘资源缺失且无回退时停止对应风险步骤并报告。

## 实现提纲

1. 检查最小相关表面：`ReEchoRestartWidget.h/.cpp` 的 `SetDeathScreen`/`SetVictoryScreen` 与 `DefeatCanvas`/`VictoryCanvas` 绑定；`ReEchoGameMode.cpp:3231` 调用点；`ReEchoRunSubsystem` 的 `CurrentBuild.CharacterId` 取值路径；`ReEchoLoadoutSelectionWidget` 的纹理路径约定。
2. C++ 签名扩展：为 `SetDeathScreen` / `SetVictoryScreen` 增加 `FName InCharacterId = NAME_None`；`GameMode` 调用处从 `RunSubsystem->CurrentBuild.CharacterId` 传入。
3. 新增 `SettlementCharacterTexturePath(CharacterId, bVictory)` 助手，映射到 `/Game/ReEcho/Textures/UI/Formal/{RoundDefeat,RoundVictory}/T_UI_Settlement_{Defeat,Victory}_Character_J_<SUIT>`；`NAME_None`/未知时回退既有红帽 `VictoryCharacter`/`DefeatCharacter`。
4. WBP 绑定：在 `DefeatCanvas` / `VictoryCanvas` 增加可选 `UImage`（`DefeatCharacterImage` / `VictoryCharacterImage`，`BindWidgetOptional`，前缀 `ArtDefeat*` / `ArtVictory*`，不参与命中测试）；`SetDeathScreen` / `SetVictoryScreen` 内按 `InCharacterId` 设置 Brush。
5. 美术资源：为四个 `CharacterId` 产出失败立绘（胜利可选），放 `Content/SourceArt/UI/Formal/RoundDefeat/`（与 `RoundVictory/`）并写 `_SourceManifest.csv`；新增/复用 `scripts/ue/import_*` 幂等导入到 `/Game/ReEcho/Textures/UI/Formal/...`。过渡期其余角色复用 `LoadoutSelection` 的 `Selected` 图。
6. 测试：扩展 `ReEchoRestartWidgetTests.cpp`，对四个 `CharacterId` 断言 `DefeatCanvas` 内角色图 Brush 被正确设置；红帽回退断言；未知 `CharacterId` 回退断言。
7. 每次风险变更后立即验证：先 `validate_project.py`，再 Editor 编译与聚焦自动化；跨越 WBP 边界前在本地记录范围/契约变化。
8. 更新执行记录；关闭前填写架构文档审阅结果与人工验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | 项目/源码不变量通过 |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd` | UHT/UBT 退出码为 0 |
| 运行时行为变化时自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.Restart` | 受影响报告通过（含按角色立绘断言） |
| 资源 | `CompileAllBlueprints` + 纹理存在性检查 | `WBP_ReEchoRestart` 编译通过，四类角色立绘均可解析 |
| 需要人工时 | 具名 PIE/可用性任务 | 各角色死亡/胜利结算对照，记录人工结果或明确延期跟进 |

## 执行记录

### 变化

### 证据

### 剩余风险

### 人工验收结果/请求

### 架构文档审阅结果
