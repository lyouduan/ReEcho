# Plan 93 - 程序 - 遭遇 HUD WBP 视觉改造

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner 与 Executor）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`InProgress`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@86aca0ce5257728033179af9af844c10fec77bbf`。
- 本地实现方式：一任务一 worktree，`C:/Users/gavynqiu/Documents/miniGame/ReEcho-plan93-encounter-hud-ui`，分支 `plan/93-encounter-hud-ui`；规划者与执行者合一。
- 依赖 / 阻塞：用户已指定 `正式-UI视觉/正式-UI视觉/战斗场景/1-战斗场景.png` 为 1920×1080 视觉权威，并提供同目录 9 张切图。Plan92 正在修改 `ReEchoGameMode` 和 `TimeShards` 发放路径；本 Plan 可本地实现真实余额展示，但最终集成必须在 Plan92 发布后重新审计并组合适配，禁止覆盖其经济语义。
- Writes:
  - `plans/93-encounter-hud-ui.md`
  - `Content/ReEcho/UI/WBP_ReEchoEncounterHud.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoPlayerHud.uasset`
  - `Content/SourceArt/UI/CombatHud/Plan93/**`
  - `Content/ReEcho/Textures/UI/CombatHud/**`
  - `scripts/ue/import_plan93_combat_hud.py`、`scripts/ue/author_plan93_combat_hud.py`、`scripts/ue/audit_plan93_combat_hud.py`
  - `Source/ReEcho/Public/UI/ReEchoEncounterHudWidget.h`、`Source/ReEcho/Private/UI/ReEchoEncounterHudWidget.cpp`
  - `Source/ReEcho/Public/UI/ReEchoPlayerHudWidget.h`、`Source/ReEcho/Private/UI/ReEchoPlayerHudWidget.cpp`
  - `Source/ReEcho/Public/ReEchoGameMode.h`、`Source/ReEcho/Private/ReEchoGameMode.cpp`（仅接入只读时间碎片余额展示）
  - `Source/ReEcho/Private/Tests/ReEchoUIManagerSubsystemTests.cpp` 与本页新增/维护的聚焦 UI 测试
  - `Design/UI/ReEcho_UI修改指导.md`
  - `docs/ART_ASSET_ORGANIZATION.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `Binaries/Win64/` 下 `GIT_RULES.md` 允许的最终精选预构建包
- Stable Reads:
  - `Source/ReEcho/Public/UI/Framework/ReEchoUIScreenTypes.h`
  - `Source/ReEcho/Private/UI/ReEchoUIManagerSubsystem.cpp`
  - `Source/ReEcho/Private/UI/Framework/ReEchoUIFlowCoordinatorSubsystem.cpp`
  - `Source/ReEcho/{Public,Private}/UI/ReEchoMinimapCanvasWidget.*`
  - `Source/ReEcho/{Public,Private}/ReEchoGameMode.*` 的 Player/Encounter HUD 创建、显隐和数据推送路径
  - `plans/45-ui-interaction-placeholder-assets.md`、`plans/51-fix-restart-hud-screen-reset.md`、`plans/60-ui-full-reskin.md`、`plans/92-enemy-time-shard-drops.md`
- 影响模式：`SharedContract`（新增只读时间碎片 UI 绑定；两个目标 WBP 仍是不可文本合并的 Exclusive 二进制资产，必须串行编辑）。
- 兼容承诺 / 下游操作：保留 `EReEchoUIScreen::PlayerHud/EncounterHud`、既有 Viewport 层级、生命事件绑定、受伤屏幕反馈、最后 5 秒警示、小地图视图转发、关卡 travel 重建和不接管输入的语义；时间碎片只读取 `UReEchoRunSubsystem::TimeShards`，不在 Widget 中修改余额或复制掉落规则。
- 明确排除：不修改遭遇真实时长、关卡流程、暂停、战斗、存档、Plan92 掉落数值/发放语义、数据 Schema、其他页面或 Runtime Module 拓扑；不实现尚无产品契约的技能按钮/冷却；不在 Editor 外手改 `.uasset`；参考效果图不作为运行时整屏纹理。

## 锁定目标

以用户指定的 `1-战斗场景.png` 为 1920×1080 最终构图权威，使用同目录独立 PNG 切图重做战斗常驻 HUD；`WBP_ReEchoEncounterHud` 为主要修改对象，`WBP_ReEchoPlayerHud` 配套完成左上玩家状态。实际游戏世界背景、角色和敌人不属于 UI 纹理。

1. 左上玩家状态改为两行：第一行 `爱心.png + 血条底板.png/血条.png + 当前生命/最大生命`，第二行 `时间碎片.png + 真实 TimeShards 数字`；旧圆形角色头像不再出现在正式构图中，但生命事件与受伤反馈保持。
2. 顶部中央使用 `时间底板.png`，动态关卡标题显示为 `第 {EncounterIndex} 关`，倒计时改为零补齐 `MM:SS`；`时间指针.png` 与 `时间显示.png` 按参考图居中叠放，最后 5 秒仍由 C++ 变为警示色。
3. 右上使用 `回响显示框.png` 包住现有 `UReEchoMinimapCanvasWidget`；矢量回响路径、玩家点和竞技场映射继续使用真实运行时视图，不用参考图中的绿色手绘轨迹替代。
4. 底部使用 `技能栏.png` 作为命中测试不可见的装饰宿主；当前没有正式技能按钮/冷却契约，因此只交付参考图中的空深色栏，不伪造技能状态。
5. 所有元素以 1920×1080 为作者设计面，保留边缘安全区；1280×720、2560×1440 与 21:9 下按既有 DPI/锚点策略保持可见、不过度拉伸、不拦截战斗输入。
6. 目标目录中的 `1-战斗场景.png` 只归档为 Reference；9 张独立切图归档为 Elements 并以稳定 ASCII 名导入 `/Game/ReEcho/Textures/UI/CombatHud`。原始中文交付名、像素尺寸和 SHA-256 进入可复现清单。

## 架构影响与设计决策

- 受影响架构标识：`AREA-UI`、文档型入口 `MOD-ReEchoUI`；当前实现仍属于 `MOD-ReEcho`，不新增 Runtime Module。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已加入 `Writes`；关闭前核对两页 WBP/C++ 分工、TimeShards 只读投影、绑定和验证路线。
- 设计意图：把最终视觉、布局、锚点和尺寸保留在 WBP，把生命、时间碎片、关卡索引、剩余时间、最后 5 秒警示和小地图数据继续留在 C++ 只读展示路径。
- 权威状态与依赖：Run 继续唯一拥有 TimeShards，Combatant 继续拥有生命，GameMode 只把 Run 余额投影给 Player HUD；不改变屏幕枚举、Viewport 层级、模块拓扑或状态写入方向。
- 决策记录：先发布 `Proposed` 骨架获取正式编号；用户随后提供完整参考和切图，本次扩张到 Player HUD 配套与真实 TimeShards 展示。两个 WBP 串行 authoring；效果图只对照，不整屏导入。Plan92 仍拥有掉落发放，Plan93 只读余额并在发布边界组合适配。
- 相关文档同步范围：`shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `shared/CODEBASE_MAP/README.md` 关闭前审阅；预期因拓扑和路由不变而无需修改。`MOD-ReEchoUI.md` 和 `Design/UI/ReEcho_UI修改指导.md` 按最终真实契约维护。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：待关闭前审阅。
  - `shared/CODEBASE_MAP/README.md`：待关闭前审阅。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：待实现后维护或记录无需修改原因。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：待实现后维护或记录无需修改原因。
  - `Design/UI/ReEcho_UI修改指导.md`：待实现后维护或记录无需修改原因。

## 锁定验收

- [x] 用户目标图、9 张切图、1920×1080 构图、素材映射与保留行为已在实现前补入本 Plan。
- [ ] `WBP_ReEchoEncounterHud` 保持正确原生父类，显示顶部时间底板、`第 N 关`、`MM:SS`、指针、右上回响框/真实小地图和底部空技能栏。
- [ ] `WBP_ReEchoPlayerHud` 保持正确原生父类，显示爱心、动态生命条/数值与真实时间碎片余额，不显示旧圆形角色头像。
- [ ] HUD 不接管输入；生命事件、受伤反馈、时间碎片权威、关卡索引、倒计时、最后 5 秒警示、页面显隐与 travel 后重建语义无回归。
- [ ] 两个目标 WBP Compile/Save 成功，`CompileAllBlueprints` 无错误、警告或加载失败。
- [ ] 按最终变化面执行聚焦自动化、Editor 构建、项目校验和 `git diff --check`。
- [ ] 用户在 1920×1080 PIE 对照目标图验收布局、可读性、生命/碎片、小地图、倒计时和技能栏；并检查 1280×720、2560×1440、21:9 无关键裁切。
- [ ] 未提交精选 `GIT_RULES.md` 允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@86aca0ce5257728033179af9af844c10fec77bbf`。
- 引擎/构建可用性：基线包含 UE 5.8 精选 Editor 包；首次内容修改前需确认 Editor/命令锁和现有会话，最终候选按变化面重新构建。
- 现有聚焦测试结果：基线包含 UI Manager 的 Player/Encounter HUD 创建与 travel reset 覆盖；实现前补充本页绑定、格式和只读余额检查。
- 共享契约 / 难合并资源风险：两个目标 WBP 均为 Exclusive 二进制资产，当前所有 worktree 均未修改它们且没有 Unreal Editor/同克隆锁。Plan92 正在修改 `ReEchoGameMode.cpp` 和 TimeShards 事务，最终发布前必须审计其传入实现。
- 基线损坏时的停止条件：`origin/main` 出现批准基线之外的新提交；任一目标 WBP 正被其他任务编辑；目标要求改变遭遇真实计时、输入、屏幕层级、TimeShards 写入或 Plan92 掉落语义；需要独占命令时 Editor 会话尚未保存关闭。

## 实现提纲

1. 归档 Reference/Elements、生成哈希清单并导入 9 张稳定 ASCII UI Texture2D；核对 sRGB、透明通道、无 mipmap、UI 压缩和过滤。
2. 审计两个 WBP 的当前控件树；用幂等 authoring 脚本按 1920×1080 构图重排 Player/Encounter HUD，保留绑定与小地图控件。
3. 增加 `TimeShardText` 只读绑定、真实余额投影、`第 N 关` 与 `MM:SS` 格式；增加聚焦自动化，不改变 Run 写入或掉落规则。
4. 逐资产 Compile/Save、`CompileAllBlueprints`、聚焦自动化和 Development 构建；获取运行截图并与目标图对照返工。
5. Plan92 发布后执行外部提交审计与组合适配，完成最终 FullRebuild、静态检查、模块文档审阅和人工验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 源图/导入 | 清单 SHA-256、Texture2D 配置与资产加载审计 | 1 Reference + 9 Elements 可追溯，运行时只导入切图 |
| WBP | 两个目标资产 Compile/Save；`CompileAllBlueprints` | Player/Encounter HUD 0 errors、0 warnings、0 failed loads |
| UI 聚焦 | HUD 创建/travel reset、本页绑定/格式/余额检查 | 创建、生命/碎片/计时/小地图刷新与重建契约通过 |
| C++ | 修改源码时执行 `.clang-format` 与 `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 成功并刷新匹配精选包 |
| 静态 | `python scripts/validate_project.py`；`git diff --check` | 项目与差异通过 |
| 发布 | 非纯文档最终候选执行 `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终候选完整构建并刷新允许列表产物 |
| 人工 | 1920×1080 对照图；1280×720、2560×1440、21:9；普通/最后 5 秒、生命/碎片、小地图和页面显隐 | 用户报告 `Passed` 或具名返工项 |

## 执行记录

### 变化

- 创建 Plan93 `Proposed` 骨架和独立 worktree；尚未修改目标 WBP 或运行时源码。
- 用户提供 1920×1080 最终战斗场景参考图和 9 张独立切图；Plan93 已锁定为 Encounter HUD 主改、Player HUD 配套，并进入 `InProgress`。

### 证据

- 编号前已 fetch；当时 `origin/main@513ec51b6d101979b6d2cf548dfc3403762b7966` 与主工作区一致，远端最大 Plan 编号为 92，新任务使用 93。
- 首次发布前 fetch 发现 Plan91 两个新提交推进远端至 `99bbfe53e5bd5321f408f5c6ba9987ec7a0845c3`；只读审计确认不修改 Encounter HUD、无物理冲突或逻辑冲突。经用户确认组合适配后，Plan93 干净变基到该提交并更新本地规划 / 实现基线。
- 锁定视觉规格发布前，远端新增 `86aca0ce5257728033179af9af844c10fec77bbf` 的 Plan91 文档扩展；只读审计确认仅修改 `plans/91-tiered-shop-card-slots.md`，与 Plan93 无物理、逻辑或编号冲突。经用户再次确认组合适配后，本规格更新干净变基到该提交。
- 已读取 `MOD-ReEchoUI`、UI 修改指导、Encounter HUD 公共头与配对实现，并确认现有 WBP/C++ 边界。
- 已核对交付切图尺寸：时间底板 1159×216、时间显示 161×45、时间指针 44×150、回响显示框 360×322、技能栏 1801×265、爱心 68×68、时间碎片 68×70、血条/底板各 232×29；新素材与 Plan45 占位素材哈希/尺寸不同。
- 已检查全部本地 worktree、Git-common-dir Unreal 锁和 UnrealEditor 进程：两个目标 WBP 无其他未提交修改，当前没有锁或 Editor 进程。

### 剩余风险

- 两个 WBP 是二进制资源，后续必须在专属 worktree 串行编辑并逐批验证。
- Plan92 的本地候选正在修改 `ReEchoGameMode.cpp` 与 TimeShards 事务；Plan93 不读取其未发布 worktree，最终发布前需等待或组合适配远端正式结果。

### 人工验收结果/请求

- `PendingBeforeClose`：实现后由用户在 PIE 对照 `1-战斗场景.png` 验收，并具名反馈偏差。

### 架构文档审阅结果

- 待实现完成后逐项填写。
