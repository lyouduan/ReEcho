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
- 依赖 / 阻塞：无额外美术依赖。直接复用选角阶段已有的四张角色图（LoadoutSelection 的 `Selected` 变体：`T_UI_Loadout_Character_J_<SUIT>_Selected`，J_HEART / J_SPADE / J_CLOVER / J_DIAMOND），死亡与胜利均按所选 `CharacterId` 切换该图；不新增源图或纹理。
- Writes：`plans/135-settlement-character-illustration.md`；`Content/ReEcho/UI/WBP_ReEchoRestart.uasset`（在 `DefeatCanvas` / `VictoryCanvas` 内新增可选角色 `UImage` 绑定，运行时按 `CharacterId` 设置 Brush）；`Source/ReEcho/Public/UI/ReEchoRestartWidget.h`；`Source/ReEcho/Private/UI/ReEchoRestartWidget.cpp`；`Source/ReEcho/Private/ReEchoGameMode.cpp`（调用点传入 `CharacterId`）；`Source/ReEcho/Private/Tests/ReEchoRestartWidgetTests.cpp`（按角色立绘断言）；`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`。复用（只读）既有资产：`/Game/ReEcho/Textures/UI/LoadoutSelection/T_UI_Loadout_Character_J_<SUIT>_Selected`。
- Stable Reads：`Source/ReEcho/Public/UI/ReEchoRestartWidget.h`；`Source/ReEcho/Private/UI/ReEchoRestartWidget.cpp`；`Source/ReEcho/Private/ReEchoGameMode.cpp` 的结算调用点（`ReEchoGameMode.cpp:3231` 附近）；`Source/ReEcho/Public/Run/ReEchoRunSubsystem.h` 与 `Private/Run/ReEchoRunSubsystem.cpp` 的 `StartRun` / `CurrentBuild.CharacterId`；`Source/ReEcho/Private/UI/ReEchoLoadoutSelectionWidget.cpp` 的 `LoadoutTexturePath` / `CharacterTexturePath` 约定（`ReEchoLoadoutSelectionWidget.cpp:567`）；`Design/UI/ReEcho_UI修改指导.md`；`Content/SourceArt/UI/LoadoutSelection/`（四张选角角色图与命名约定）。
- 影响模式：`SharedContract`（`WBP_ReEchoRestart` 六种状态共用，高冲突二进制资产）。
- 兼容承诺 / 下游操作：保护普通暂停、两种退出确认、胜利、重开与返回主菜单流程；保持既有 `BindWidgetOptional` 名称与类型；未知/缺失立绘时回退既有红帽图，不破坏任何状态或伪造数据。
- 明确排除：不新增第二套结算 WBP；不改 Combat / Run / 存档 / 奖励 / 音乐 / 关卡推进逻辑；不为未知角色伪造任何统计数据；不把整图当作不可编辑的全屏点击层；**不新增结算专用角色立绘**（直接复用选角四图）。

## 锁定目标

在保持现有胜利、失败、重开和返回主菜单逻辑一致的前提下，让结算界面（失败与胜利，两者都改）展示"本轮实际所选角色"的立绘，而不是固定红帽。

- 死亡与胜利均按 `RunSubsystem->CurrentBuild.CharacterId` 解析出对应角色，替换 `DefeatCanvas` / `VictoryCanvas` 内的角色图。
- 使用的图就是选角界面已有的四张角色 `Selected` 变体（`T_UI_Loadout_Character_J_<SUIT>_Selected`），即"开始选角色的那四个"，不做任何新美术。
- 红帽角色（J_HEART 或当前默认主角）行为与现状完全一致：结算红帽 == 开头选中的红帽。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoUI`、`AREA-UI`。不修改公开 Runtime API；仅为既有 `WBP_ReEchoRestart` 增加按 `CharacterId` 选择角色立绘的可选绑定和运行时投影。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`，已加入 `Writes`。
- 设计意图：结算角色图由"写死红帽"变为"数据驱动所选角色"，并与选角阶段（Plan 132 `LoadoutSelection` 已按 `CharacterId` 出 `Selected` / `Unselected` 图）复用同一套角色立绘，避免重复美术。
- 权威状态与依赖：`GameMode` 已持有 `RunSubsystem`；`CurrentBuild.CharacterId` 在 `StartRun` 经 `ResolveStartingBuild` 写入（`ReEchoRunSubsystem.cpp:1304` `Result.Build.CharacterId = Character->Id`，随后进入 `CurrentBuild`），死亡/胜利时可直接读取，**无需新增任何存储字段**。
- 决策记录：
  1. 复用 `WBP_ReEchoRestart`，不新增独立结算页面，以免分叉暂停/重开流程。
  2. 数据传递：在 `ReEchoGameMode.cpp` 调用 `SetDeathScreen` / `SetVictoryScreen` 处，由 `RunSubsystem->CurrentBuild.CharacterId` 取出 `CharacterId` 传入（倾向由 `GameMode` 传入，保持 `RestartWidget` 不直接依赖 `GameInstance` 单例）。`SetDeathScreen` 新增 `FName InCharacterId = NAME_None` 形参，`SetVictoryScreen` 同理。
  3. 资源：直接复用选角四图——按 `CharacterId` 经既有 `LoadoutTexturePath("Character", CharacterId, "Selected")` 解析到 `/Game/ReEcho/Textures/UI/LoadoutSelection/T_UI_Loadout_Character_J_<SUIT>_Selected`，**不新增任何源图或纹理**，胜利与失败共用同一张所选角色（`Selected` 变体）。红帽（J_HEART）解析结果即既有红帽立绘，行为不变。
  4. 绑定：`DefeatCanvas` 内新增可选 `UImage`（如 `DefeatCharacterImage`，命名前缀 `ArtDefeat*`）；`VictoryCanvas` 内 `VictoryCharacterImage` 同理（前缀 `ArtVictory*`）；运行时按 `InCharacterId` 设置 Brush；缺失时回退既有写死红帽图。
  5. 解析：优先复用既有 `LoadoutTexturePath`（与 `LoadoutSelection` 一致）；如需隔离可包一层 `SettlementCharacterTexturePath(CharacterId)` 转发到同一纹理，避免两套路径漂移。
  6. 失败页与胜利页都只投影真实到达关卡、构筑数量与时间碎片等既有的真实数据；不伪造任何缺失来源的数据。
- 相关文档同步范围：审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `README.md`；维护 `MOD-ReEchoUI.md` 的结算角色立绘事实；若拓扑与稳定路由不变则只在执行记录中写明无需修改。
- 关闭前逐项填写审阅结果：待实现后补充。

## 锁定验收

- [ ] 选 J_HEART / J_SPADE / J_CLOVER / J_DIAMOND 中任一带到死亡，结算失败页角色立绘 = 该角色在选角界面所用的 `Selected` 立绘，不再是固定红帽。
- [ ] 同上角色带至胜利，结算胜利页角色立绘 = 该角色选角 `Selected` 立绘。
- [ ] 选红帽角色死亡/胜利，结算角色立绘 = 红帽（与现状一致）。
- [ ] 未知 / 缺失立绘时回退红帽，不崩溃、不影响暂停/退出确认/重开/返回主菜单。
- [ ] `WBP_ReEchoRestart` 编译通过；`RootPanel` / `TitleText` / 按钮及既有 Victory/Defeat 美术绑定名称与类型保持可加载。
- [ ] `SetDeathScreen` / `SetVictoryScreen` 调用点（`ReEchoGameMode.cpp:3231`）更新，并扩展 `ReEchoRestartWidgetTests.cpp` 增加"按角色立绘"断言（覆盖四个 `CharacterId` 与回退）。
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

1. 检查最小相关表面：`ReEchoRestartWidget.h/.cpp` 的 `SetDeathScreen` / `SetVictoryScreen` 与 `DefeatCanvas` / `VictoryCanvas` 绑定；`ReEchoGameMode.cpp:3231` 调用点；`ReEchoRunSubsystem` 的 `CurrentBuild.CharacterId` 取值路径；`ReEchoLoadoutSelectionWidget` 的 `LoadoutTexturePath` 约定（确认 `Selected` 变体纹理已存在于 `/Game/ReEcho/Textures/UI/LoadoutSelection/`）。
2. C++ 签名扩展：为 `SetDeathScreen` / `SetVictoryScreen` 增加 `FName InCharacterId = NAME_None`；`GameMode` 调用处从 `RunSubsystem->CurrentBuild.CharacterId` 传入。
3. 纹理解析：复用 `LoadoutTexturePath("Character", CharacterId, "Selected")` 得到选角所用的四张图之一；`NAME_None` / 未知时回退既有红帽 `VictoryCharacter` / `DefeatCharacter`。可包一层 `SettlementCharacterTexturePath` 转发，避免路径漂移。
4. WBP 绑定：在 `DefeatCanvas` / `VictoryCanvas` 增加可选 `UImage`（`DefeatCharacterImage` / `VictoryCharacterImage`，`BindWidgetOptional`，前缀 `ArtDefeat*` / `ArtVictory*`，不参与命中测试）；`SetDeathScreen` / `SetVictoryScreen` 内按 `InCharacterId` 设置 Brush。
5. 资源：无新增美术资源，直接复用选角四图（确认 `T_UI_Loadout_Character_J_<SUIT>_Selected` 已导入且命名匹配 `CharacterPresentationOrder` 的 J_HEART/SPADE/CLOVER/DIAMOND）。
6. 测试：扩展 `ReEchoRestartWidgetTests.cpp`，对四个 `CharacterId` 断言 `DefeatCanvas` / `VictoryCanvas` 内角色图 Brush 被正确设置；红帽回退断言；未知 `CharacterId` 回退断言。
7. 每次风险变更后立即验证：先 `validate_project.py`，再 Editor 编译与聚焦自动化；跨越 WBP 边界前在本地记录范围/契约变化。
8. 更新执行记录；关闭前填写架构文档审阅结果与人工验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | 项目/源码不变量通过 |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd` | UHT/UBT 退出码为 0 |
| 运行时行为变化时自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.Restart` | 受影响报告通过（含按角色立绘断言） |
| 资源 | `CompileAllBlueprints` + 纹理存在性检查 | `WBP_ReEchoRestart` 编译通过，四类角色选角立绘均可解析 |
| 需要人工时 | 具名 PIE/可用性任务 | 各角色死亡/胜利结算对照，记录人工结果或明确延期跟进 |

## 执行记录

### 变化

### 证据

### 剩余风险

### 人工验收结果/请求

### 架构文档审阅结果
