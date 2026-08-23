# Plan 81 - 程序 - 设置页 UI 完成（控制键位重绑定 + 图形/音频占位补全）

## 协调

- Planner 负责人：Gavyn-side AI（本项目 Planner + 秘书）
- Executor 负责人：Gavyn-side AI（同 AI 在用户 程序 指导下执行）
- Plan 编写方（AI 侧）：`Gavyn-side AI | ReEcho teammate-side AI`
- 实现编写方（AI 侧）：`Gavyn-side AI`
- 任务状态：`Proposed`
- 人工验收：`PendingBeforeClose`（键位重绑定手感、布局可读性、三类别视觉对齐需人工判断；持久化重启保留需人工 PIE 验收）
- 本地规划 / 实现基线：`origin/main @ 63b3305`
- 本地实现方式（可选，仅作交接说明）：一任务一 worktree（`plan/81-settings-ui-complete`），主工作树仅快进合并；WBP 视觉与控件由用户在编辑器内经 MCP 引导置入/调整，程序侧负责 C++ 契约绑定与键位持久化逻辑。
- 依赖 / 阻塞：WBP 视觉置入与控件命名由用户经 MCP 引导逐步落地；键位重绑定的输入映射来源与持久化落点需在 Milestone 3 首步审计后定稿（见决策记录）。
- Writes:
  - `Source/ReEcho/Public/UI/ReEchoSettingsWidget.h`（控制类别键位重绑定所需字段/方法、`ControlsPanel` 内控件绑定）
  - `Source/ReEcho/Private/UI/ReEchoSettingsWidget.cpp`（控制类别列表构建、键捕获、写入、持久化、重置默认；图形/音频绑定核对）
  - `Source/ReEcho/Private/Tests/ReEchoSettingsInteractionTests.cpp`（扩充对控制类别交互层与键位重绑定契约的断言）
  - `Content/ReEcho/UI/WBP_ReEchoSettings.uasset`（由用户经 MCP/编辑器置入视觉与控件；程序按资产命名更新 `BindWidget`）
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`（同步设置页三类别契约与键位持久化落点）
- Stable Reads:
  - `Source/ReEcho/Public/UI/ReEchoSettingsWidget.h` 与 `.cpp`（设置页 Widget 骨架，稳定契约）
  - `Source/ReEcho/Public/UI/Framework/ReEchoUIScreenTypes.h`（`EReEchoUIScreen::Settings` 枚举，稳定）
  - `Source/ReEcho/Private/UI/Framework/ReEchoUIManagerSubsystem.cpp`（`ScreenClasses` 屏幕→Widget 映射，不修改）
  - 引擎 `UInputSettings`（`GetInputSettings` / `GetActionMappings` / `GetAxisMappings` / `SaveConfig`），或 MCP 引导确认的实际输入映射来源
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`（各 `OpenScreen` 调用点，稳定接口；不修改）
- 影响模式：`SharedContract`（新增键位持久化契约 + 控制类别 UI 契约；图形/音频为既有契约核对）
- 兼容承诺 / 下游操作：不新增 / 删除 `EReEchoUIScreen` 枚举项；不改动音频五滑块既有行为；不改动图形既有控件语义；键位持久化向后兼容——无存档/配置时使用默认映射，不破坏现有键位；若引入新输入映射落点，须显式声明且不改变玩法输入读取路径。
- 明确排除：不把图形/音频派生数值硬编码进 UI；不重构输入系统为 Enhanced Input（除非 MCP 引导明确确认并另立子决策）；不改动战斗 / 玩法 / 存档逻辑；不新建 Runtime Module；不处理敌人血条等无关 UI。

## 锁定目标

将当前设置页从「图形 + 音频可用、控制占位」升级为**三类别全部可用**：

1. **图形类别**：分辨率、显示模式、画质、VSync、亮度、音频输出等选项全部真实可交互并写入 `UGameUserSettings`；与恢复的 WBP（332KB）视觉对齐；占位文案仅在控件缺失时作为兜底保留。
2. **音频类别**：已在 Plan34 完整落地（主/音乐/环境/战斗/UI 五滑块 + 静音 + 诊断音），本 Plan 仅核对控件与 WBP 视觉对齐、命中正常。
3. **控制类别（核心缺口）**：完整按键重绑定——
   - 列出当前各操作的按键映射；
   - 点击某项进入捕获态，按下新键写入该操作；
   - 写入并持久化到配置（落点见决策记录），重启后保留；
   - 支持「重置默认」恢复默认映射；
   - 冲突键给出提示/覆盖策略。

**实现策略：循序渐进、由你引导**。本 Plan 不要求一次性把三类别全做完，而是先交出一个「能打开、能切换、可见」的设置页，再按你的引导逐类别补全功能：

- **第一步（Milestone 0，最高优先）：先把图形素材（WBP 视觉切图）放入** `WBP_ReEchoSettings`，使设置页在编辑器可打开、三类别（图形/音频/控制）可切换、无占位死文本、命中正常。这是后续一切功能的前提，**先做这一步，不做完不进后续**。
- 后续 Milestone（图形 / 音频 / 控制类别功能）在你经 MCP 引导下一步步推进，**每完成一类即验证一类**，不跨类批量实现、不预先把三类别代码一次写完。

## 架构影响与设计决策

- 受影响架构标识：`AREA-UI`（`Source/ReEcho/{Public,Private}/UI/`）；文档入口 `MOD-ReEchoUI`（`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`）。实现仍属 `MOD-ReEcho`，不新增 Runtime Module，故无新 `MOD-*` 标识。键位重绑定若需新增本地子系统/落点，须在 `MOD-ReEchoUI` 中记录契约。
- 对应模块文档：`MOD-ReEchoUI.md` 必须进入 Writes 并在关闭前更新（设置页三类别契约、控制类别键位重绑定数据归属）；`MOD-ReEcho.md` 若目录/所有权文字变化则更新，否则关闭审阅说明。
- 设计意图：设置页从「两类别可用 + 控制占位」升级为「三类别全可用」；键位重绑定为新增能力，需确定数据归属与冲突策略，且不得破坏玩法输入读取。
- 权威状态与依赖：不改变屏幕枚举语义、`ScreenClasses` 映射键、`UIFlow` 开关节点；仅替换/扩充 WBP 资产与对应 `BindWidget`，并新增键位持久化写入路径。依赖用户在编辑器内经 MCP 引导置入视觉与控件命名。
- 决策记录：
  - **输入映射来源（M3 首步审计）**：当前 `Source` 中搜不到 `EnhancedInput`/`UInputSettings`/`SaveConfig` 用法，说明项目尚无现成键位重绑定基础设施。默认提案：读取 `UInputSettings::GetInputSettings()` 的 Action/ Axis Mapping 作为当前键位来源，重绑定后写回并 `SaveConfig()`（落点 `Config/DefaultInput.ini` 或项目约定文件）。若 MCP 引导确认项目实际采用其他输入定义（如自定义键位表 / `PlayerInput` 按键映射），则以实际来源为准。代价：legacy 映射不支持 Enhanced Input 的触发/组合键语义，但本作 2.5D 动作需求以单键映射为主，足够覆盖。
  - **持久化落点**：优先跟随输入来源——`UInputSettings::SaveConfig()` 落到项目 ini；若需跨设备/账号一致，再评估 `USaveGame`。保持「无存档用默认」向后兼容。
  - **WBP 契约**：控制类别列表行（操作名 + 当前键 + 重绑按钮）由用户在编辑器经 MCP 置入；程序侧以 `BindWidgetOptional` 容器 + 运行时 `AddChild` 填充行，避免硬编码行数。
  - 不一次性重做整套 UI 框架，仅补齐设置页三类别内容与键位持久化。
- 相关文档同步范围：
  - `ARCHITECTURE.md`：无拓扑变化 → 关闭时审阅说明「无需修改」（除非键位持久化引入新子系统）。
  - `README.md`（CODEBASE_MAP 索引）：无路由变化 → 关闭时审阅说明。
  - `modules/MOD-ReEchoUI.md`：更新（Writes）。
  - `modules/MOD-ReEcho.md`：审阅，按需更新 UI 目录描述。
- 关闭前逐项填写审阅结果（见末尾「架构文档审阅结果」）。

## 锁定验收

- [ ] 设置页三类别均可打开且无占位死文本（控制类别显示真实键位列表而非「暂未接入」）。
- [ ] 控制类别可点击捕获新键、写入并持久化，重启后保留；「重置默认」可恢复默认映射。
- [ ] 图形/音频控件与恢复 WBP（332KB）视觉对齐、命中正常，功能行为不变。
- [ ] `scripts/ue/Build-Editor.cmd` 通过（UHT/UBT 退出码 0）。
- [ ] `python scripts/validate_project.py` 通过（CSV schema / UTF-8 / 二进制清单）。
- [ ] `ReEchoSettingsInteractionTests`（`ReEcho.UI.SettingsInteraction`）通过，并扩充控制类别断言。
- [ ] 仅在视觉质量、可读性、重绑定手感需要判断时请求人工验收（PendingBeforeClose）。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。
- [ ] 未新增 / 删除 `EReEchoUIScreen` 项；未改动 `UIFlow` 开关节点。

## Step 0 门禁

- 基线分支/提交：`origin/main @ 63b3305`
- 引擎/构建可用性：Editor 可启动，Development 构建可用（Plan80 已验证门禁链路）。
- 现有聚焦测试结果：`ReEcho.UI.SettingsInteraction` 须保持通过（WBP 含图形/音频控件、根 `CanvasPanel_0`、音量轨道 `HitTestInvisible`）。
- 共享契约 / 难合并资源风险：`Content/ReEcho/UI/WBP_ReEchoSettings.uasset` 为二进制，用户经 MCP 编辑时避免与并行 Plan 的 UI 改动同文件冲突；按类别隔离提交。
- 基线损坏时的停止条件：若 `main` 在发布 / 实现期间前进（他人提交），立即 fetch 重做外部提交集成审计，不静默合并。

## 实现提纲

不修改锁定目标、验收或已发布契约；可优化实现细节。

- **Milestone 0 — 先把图形素材放上（用户 MCP 引导，最高优先）**：在 `WBP_ReEchoSettings` 置入视觉切图与三类别容器（图形/音频/控制），确保编辑器可打开、三类别可切换、`ControlsPanel` 容器就位、无占位死文本、命中正常；程序侧确认 `BindWidgetOptional` 绑定。此步完成并验证后再进入后续类别。
- **Milestone 1 — 图形类别核对**：确认 WBP 控件（分辨率/显示模式/画质/VSync/亮度/音频输出）真绑定生效并写入 `UGameUserSettings`；与恢复 WBP 视觉对齐；占位文案仅在控件缺失时保留。
- **Milestone 2 — 音频类别核对**：验证 Plan34 已完整，仅做视觉对齐与命中核对，不改变行为。
- **Milestone 3 — 控制类别键位重绑定**：
  1. 审计实际输入映射来源（`UInputSettings` Action/ Axis Mapping 或 MCP 确认来源）。
  2. 构建操作→当前键列表（运行时 `AddChild` 填充行）。
  3. 点击进入捕获态，拦截下一次按键，校验冲突。
  4. 写回输入设置并持久化（`SaveConfig` 或约定落点）。
  5. 「重置默认」恢复默认映射。
  6. 补充 `ReEchoSettingsInteractionTests` 对控制类别交互层与重绑定契约的断言。
- 每步完成一个连贯变更后立即 Build 验证；PIE 验收该类别功能与命中；必要时记录人工视觉验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | 项目/源码不变量通过 |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd` | UHT/UBT 退出码为 0 |
| 运行时行为变化时自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | `ReEcho.UI.SettingsInteraction` 等受影响报告通过 |
| 需要人工时 | 具名 PIE/可用性任务 | 记录重绑定持久化（重启保留）与三类别视觉对齐结果或明确延期跟进 |

## 执行记录

### 变化

### 证据

### 剩余风险

### 人工验收结果/请求

### 架构文档审阅结果
