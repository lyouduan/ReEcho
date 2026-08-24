# Plan 93 - 程序 - 遭遇 HUD WBP 视觉改造

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner 与 Executor）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Closed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`Passed`。
- 本地规划基线：`origin/main@86aca0ce5257728033179af9af844c10fec77bbf`；最终集成基线：`origin/main@8c805aeccff45e7a7ffb5a2982382cb4e36a4a5e`。
- 本地实现方式：一任务一 worktree，`C:/Users/gavynqiu/Documents/miniGame/ReEcho-plan93-encounter-hud-ui`，分支 `plan/93-encounter-hud-ui`；规划者与执行者合一。
- 依赖 / 阻塞：用户已指定 `正式-UI视觉/正式-UI视觉/战斗场景/1-战斗场景.png` 为 1920×1080 构图参考，并提供同目录 9 张切图；后续明确判定底部技能栏为废案，因此正式 HUD 仅消费其余 8 张。Plan92 尚未发布；经程序用户获知 `ReEchoGameMode.cpp`、文档和精选二进制存在重叠后，明确确认 Plan93 先发布。Plan93 不改变 Plan92 的经济语义，Plan92 后续发布时须以最新 `main` 重新审计和组合适配。
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
3. 右上使用 `回响显示框.png` 包住现有 `UReEchoMinimapCanvasWidget`；Minimap Canvas 自身底板透明且不绘制浅蓝竞技场边框，只保留回响路径、玩家点和真实运行时映射，不用参考图中的绿色手绘轨迹替代。
4. 用户已将参考图底部 `技能栏.png` 面板判定为废案；从 `WBP_ReEchoEncounterHud` 删除该节点，不导入/保留对应运行时 Texture2D，也不以空白容器替代。
5. 所有元素以 1920×1080 为作者设计面，保留边缘安全区；1280×720、2560×1440 与 21:9 下按既有 DPI/锚点策略保持可见、不过度拉伸、不拦截战斗输入。
6. 目标目录中的 `1-战斗场景.png` 只归档为 Reference；9 张独立切图继续归档，废案技能栏标为 `RejectedElement`，其余 8 张以稳定 ASCII 名导入 `/Game/ReEcho/Textures/UI/CombatHud`。原始中文交付名、像素尺寸和 SHA-256 进入可复现清单。

## 架构影响与设计决策

- 受影响架构标识：`AREA-UI`、文档型入口 `MOD-ReEchoUI`；当前实现仍属于 `MOD-ReEcho`，不新增 Runtime Module。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已加入 `Writes`；关闭前核对两页 WBP/C++ 分工、TimeShards 只读投影、绑定和验证路线。
- 设计意图：把最终视觉、布局、锚点和尺寸保留在 WBP，把生命、时间碎片、关卡索引、剩余时间、最后 5 秒警示和小地图数据继续留在 C++ 只读展示路径。
- 权威状态与依赖：Run 继续唯一拥有 TimeShards，Combatant 继续拥有生命，GameMode 只把 Run 余额投影给 Player HUD；不改变屏幕枚举、Viewport 层级、模块拓扑或状态写入方向。
- 决策记录：先发布 `Proposed` 骨架获取正式编号；用户随后提供完整参考和切图，本次扩张到 Player HUD 配套与真实 TimeShards 展示。两个 WBP 串行 authoring；效果图只对照，不整屏导入。Plan92 仍拥有掉落发放，Plan93 只读余额并在发布边界组合适配。
- 相关文档同步范围：`shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `shared/CODEBASE_MAP/README.md` 关闭前审阅；预期因拓扑和路由不变而无需修改。`MOD-ReEchoUI.md` 和 `Design/UI/ReEcho_UI修改指导.md` 按最终真实契约维护。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅；模块、权威状态和依赖方向未变，无需修改。
  - `shared/CODEBASE_MAP/README.md`：已审阅；不新增模块或阅读入口，无需修改。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：已维护 Plan93 WBP/C++ 分工、TimeShards 只读投影与阅读路线。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：已维护战斗常驻 HUD 跨域只读投影说明。
  - `Design/UI/ReEcho_UI修改指导.md`：已维护 Player/Encounter HUD 清单、绑定名、正式资产路径与修改边界。

## 锁定验收

- [x] 用户目标图、9 张切图、1920×1080 构图、素材映射与保留行为已在实现前补入本 Plan。
- [x] `WBP_ReEchoEncounterHud` 保持正确原生父类，显示顶部时间底板、`第 N 关`、`MM:SS`、指针和右上回响框/真实小地图；底部废案面板及其运行时纹理均不存在。
- [x] `WBP_ReEchoPlayerHud` 保持正确原生父类，显示爱心、动态生命条/数值与真实时间碎片余额，不显示旧圆形角色头像。
- [x] HUD 不接管输入；生命事件、受伤反馈、时间碎片权威、关卡索引、倒计时、最后 5 秒警示、页面显隐与 travel 后重建语义无回归。
- [x] 两个目标 WBP Compile/Save 成功，`CompileAllBlueprints` 的 Blueprint 汇总为 0 errors、0 warnings、0 failed loads。
- [x] 按最终组合候选执行聚焦自动化、Development Editor FullRebuild、项目校验和 `git diff --check`。
- [x] 用户通过运行截图连续复核 1920×1080 布局，具名要求删除废案栏、透明化小地图及移除浅蓝边框，并手动调整字体/小地图位置；最终明确要求发布。1280×720、2560×1440、21:9 未另行截图，若后续发现裁切则另立跟进任务。
- [x] 未提交精选 `GIT_RULES.md` 允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@86aca0ce5257728033179af9af844c10fec77bbf`。
- 引擎/构建可用性：基线包含 UE 5.8 精选 Editor 包；首次内容修改前需确认 Editor/命令锁和现有会话，最终候选按变化面重新构建。
- 现有聚焦测试结果：基线包含 UI Manager 的 Player/Encounter HUD 创建与 travel reset 覆盖；实现前补充本页绑定、格式和只读余额检查。
- 共享契约 / 难合并资源风险：两个目标 WBP 均为 Exclusive 二进制资产，当前所有 worktree 均未修改它们且没有 Unreal Editor/同克隆锁。Plan92 正在修改 `ReEchoGameMode.cpp` 和 TimeShards 事务，最终发布前必须审计其传入实现。
- 基线损坏时的停止条件：`origin/main` 出现批准基线之外的新提交；任一目标 WBP 正被其他任务编辑；目标要求改变遭遇真实计时、输入、屏幕层级、TimeShards 写入或 Plan92 掉落语义；需要独占命令时 Editor 会话尚未保存关闭。

## 实现提纲

1. 归档 Reference/Elements、生成哈希清单并导入 8 张获准的稳定 ASCII UI Texture2D；核对 sRGB、透明通道、无 mipmap、UI 压缩和过滤。废案技能栏仅归档，不进入运行时。
2. 审计两个 WBP 的当前控件树；用幂等 authoring 脚本按 1920×1080 构图重排 Player/Encounter HUD，保留绑定与小地图控件。
3. 增加 `TimeShardText` 只读绑定、真实余额投影、`第 N 关` 与 `MM:SS` 格式；增加聚焦自动化，不改变 Run 写入或掉落规则。
4. 逐资产 Compile/Save、`CompileAllBlueprints`、聚焦自动化和 Development 构建；获取运行截图并与目标图对照返工。
5. Plan92 发布后执行外部提交审计与组合适配，完成最终 FullRebuild、静态检查、模块文档审阅和人工验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 源图/导入 | 清单 SHA-256、Texture2D 配置与资产加载审计 | 1 Reference + 9 Elements 可追溯，废案技能栏只归档，其余 8 张进入运行时 |
| WBP | 两个目标资产 Compile/Save；`CompileAllBlueprints` | Player/Encounter HUD 0 errors、0 warnings、0 failed loads |
| UI 聚焦 | HUD 创建/travel reset、本页绑定/格式/余额检查 | 创建、生命/碎片/计时/小地图刷新与重建契约通过 |
| C++ | 修改源码时执行 `.clang-format` 与 `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 成功并刷新匹配精选包 |
| 静态 | `python scripts/validate_project.py`；`git diff --check` | 项目与差异通过 |
| 发布 | 非纯文档最终候选执行 `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终候选完整构建并刷新允许列表产物 |
| 人工 | 1920×1080 对照图；1280×720、2560×1440、21:9；普通/最后 5 秒、生命/碎片、小地图和页面显隐 | 用户报告 `Passed` 或具名返工项 |

## 执行记录

### 变化

- 创建 Plan93 `Proposed` 骨架和独立 worktree，随后按锁定规格进入本地实现。
- 用户提供 1920×1080 最终战斗场景参考图和 9 张独立切图；Plan93 已锁定为 Encounter HUD 主改、Player HUD 配套，并进入 `InProgress`。
- 已归档 1 张 Reference 与 9 张 Elements，生成尺寸/SHA-256/运行时名清单；废案技能栏标为 `RejectedElement`，其余 8 张通过 Editor 导入 `/Game/ReEcho/Textures/UI/CombatHud/`，Reference 未进入运行时。
- 已通过幂等 UMG authoring 修改两个目标 WBP：Player HUD 使用爱心、图片生命填充和真实碎片文本并折叠旧头像；Encounter HUD 使用交付时钟/回响切图、保留真实小地图，并移除废案技能栏。
- C++ 已增加图片生命比例、TimeShards 只读投影、`第 N 关` 与 `MM:SS` 格式，并新增 WBP 实例/绑定/真实小地图聚焦自动化。同步维护 UI 指导、美术目录说明和两个模块阅读入口。
- 用户在运行截图中明确将底部技能栏面板判定为废案；本地候选改为从 Encounter WBP 删除 `ArtSkillBar`，清理其无引用运行时纹理，并在源清单保留 `RejectedElement` 追溯记录。
- 用户随后要求小地图底板透明；已移除 `SReEchoMinimapCanvas::OnPaint` 的深蓝半透明背景绘制，保留外层回响框、竞技场边线、轨迹和玩家点。
- 用户运行复核后指出残留浅蓝细框；已继续移除 Canvas 绘制的竞技场边线，最终只保留外层交付回响框、轨迹和实时点。
- 用户在最终视觉复核中手动将 `EncounterText` 的 Y 位置由 `27` 调整为 `0`，将生命/碎片文字字号调整为 `25`，并把真实小地图内缩到 `(40,32)`、尺寸约 `273.316×254.796`；这些 WBP 改动已保留并同步回幂等 authoring/审计脚本，避免后续重跑回退。

### 证据

- 编号前已 fetch；当时 `origin/main@513ec51b6d101979b6d2cf548dfc3403762b7966` 与主工作区一致，远端最大 Plan 编号为 92，新任务使用 93。
- 首次发布前 fetch 发现 Plan91 两个新提交推进远端至 `99bbfe53e5bd5321f408f5c6ba9987ec7a0845c3`；只读审计确认不修改 Encounter HUD、无物理冲突或逻辑冲突。经用户确认组合适配后，Plan93 干净变基到该提交并更新本地规划 / 实现基线。
- 锁定视觉规格发布前，远端新增 `86aca0ce5257728033179af9af844c10fec77bbf` 的 Plan91 文档扩展；只读审计确认仅修改 `plans/91-tiered-shop-card-slots.md`，与 Plan93 无物理、逻辑或编号冲突。经用户再次确认组合适配后，本规格更新干净变基到该提交。
- 已读取 `MOD-ReEchoUI`、UI 修改指导、Encounter HUD 公共头与配对实现，并确认现有 WBP/C++ 边界。
- 已核对交付切图尺寸：时间底板 1159×216、时间显示 161×45、时间指针 44×150、回响显示框 360×322、技能栏 1801×265、爱心 68×68、时间碎片 68×70、血条/底板各 232×29；新素材与 Plan45 占位素材哈希/尺寸不同。
- 已检查全部本地 worktree、Git-common-dir Unreal 锁和 UnrealEditor 进程：两个目标 WBP 无其他未提交修改，当前没有锁或 Editor 进程。
- 8 张获准 Texture2D 导入和两个 WBP 首次/原生模块重建后 Compile/Save 均成功；只读 Editor 审计确认 Player/Encounter WBP 分别继承 `UReEchoPlayerHudWidget` / `UReEchoEncounterHudWidget`，新节点均为 `Is Variable`，旧头像折叠，唯一真实 Minimap 保留，废案 `ArtSkillBar` 不存在，所有表现层均不接管输入。
- Texture2D 审计确认 8 张运行时资源尺寸与清单一致，统一为 sRGB、Bilinear、NoMipmaps、EditorIcon 压缩和 UI LOD Group；WBP 引用全部指向 `/Game/ReEcho/Textures/UI/CombatHud/`，废案纹理不在运行时目录。
- 废案清理经 Unreal Editor 完成：Encounter HUD 控件数由 10 降为 9，`ArtSkillBar` 节点移除；确认无资产引用后删除 `/Game/ReEcho/Textures/UI/CombatHud/T_UI_CombatHud_SkillBar`。原始 `技能栏.png` 仍以 `RejectedElement` 保存在 SourceArt，可按需恢复。
- `scripts/ue/Build-Editor.cmd -Configuration Development` 已成功，UHT/UBT 编译新增 `ReEchoCombatHudTests.cpp` 和 HUD/GameMode 变化并刷新精选 Editor 包；随后 `python scripts/validate_project.py` 通过，`git diff --check` 通过。
- 2026-08-24 再次 fetch 发现 `origin/main@8c805aeccff45e7a7ffb5a2982382cb4e36a4a5e` 新增 Plan89 VFX 前置修复。只读审计确认其业务源码/资产不碰 Plan93 HUD，只有最终精选 Editor 二进制需要在组合后重建；已请求用户在变基/发布前确认组合策略，当前未改写本地基线。
- 对 Reference 与原始切图做 alpha/像素对照后，保留元素的 1920×1080 源图落点锁定并写入 WBP：爱心 `(30,24)`、碎片 `(31,85)`、生命/碎片底板 `(111,43)/(111,103)`、时钟 `(394,51)`、指针 `(952,87)`、时间显示 `(894,123)`、回响框 `(1560,23)`；最终只读 Slot 审计与这些坐标一致，底部废案面板不再生成。
- 聚焦自动化 `ReEcho.UI.CombatHud.Formatting` 通过：实际加载两个 WBP，验证新增健康填充/碎片绑定、`第 N 关`、`MM:SS`、头像折叠、真实 Minimap 保留以及废案面板不存在。`CompileAllBlueprints` 完成，Blueprint 汇总为 0 errors、0 warnings、0 failed loads；命令启动期另有 6 条既有环境/Legacy 资产警告。
- 小地图透明化及浅蓝竞技场边线移除后，Development Editor 增量构建通过；`ReEcho.UI.Minimap.Transform` 与 `ReEcho.UI.CombatHud.Formatting` 均为 Success，项目校验和 `git diff --check` 通过。
- Plan92 已产生干净本地提交 `315390488243f2c66822d919fc27ae4bc8f8dbe3` 但尚未发布到远端。只读审计确认它在 GameMode 的改动集中于敌人死亡绑定，未触碰 Plan93 的 Tick HUD 投影；其 Run 事务继续唯一写入 TimeShards，Plan93 的只读展示语义兼容。程序用户随后明确确认改变原顺序，由 Plan93 先发布、Plan92 后续适配。
- 最终 fetch 确认 `origin/main@8c805aeccff45e7a7ffb5a2982382cb4e36a4a5e` 未变化；经用户明确确认 Plan93 先发布后，将该基线的 Plan89 VFX 提交合入候选。业务源码/WBP 无冲突，双方生成的 manifest/DLL 冲突通过最终组合源码 FullRebuild 统一重建解决。
- 最终候选执行 `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` 成功，96 个编译/链接动作完成并刷新 UE 5.8 精选 Editor 包，源码指纹为 `1ae3276b9900`。
- 最终 `ReEcho.UI.Minimap.Transform`、`ReEcho.UI.CombatHud.Formatting`、`ReEcho.UIManagerSubsystem`（2 项）与 `ReEcho.UI.PlayerScreenFeedback`（3 项）全部 Success；Plan93 authoring 可幂等复现用户最终 WBP 调整，增强审计通过。
- `CompileAllBlueprints` 首次运行的 Blueprint 汇总已为 0/0/0，但本机端口 8000 被占用使 ModelContextProtocol 插件返回启动期错误；禁用该非项目运行时插件重跑后命令干净退出 0，Blueprint 汇总仍为 0 errors、0 warnings、0 failed loads。

### 剩余风险

- Plan92 的本地候选仍修改 `ReEchoGameMode.cpp`、文档与精选二进制；按用户确认由 Plan92 后续以发布后的 Plan93 `main` 为基线重新审计和组合适配。
- 1280×720、2560×1440 与 21:9 未独立截图，用户接受 Plan93 先发布；若出现 DPI/裁切问题另立跟进任务。

### 人工验收结果/请求

- `Passed`：用户通过运行截图逐项反馈并验收最终视觉，最后明确要求包含其字体/小地图位置调整提交、合并并发布 `origin/main`。

### 架构文档审阅结果

- `ARCHITECTURE.md` 与 CODEBASE_MAP `README.md` 的拓扑/入口无需修改；`MOD-ReEchoUI.md`、`MOD-ReEcho.md` 与 UI 修改指导已按最终本地候选同步，ART 资产组织说明已补 Plan93 SourceArt/运行时导入边界。
