# Plan 60 - 程序 - UI 整包视觉换皮与局部重构

## 协调

- Planner 负责人：Gavyn-side AI（本项目 Planner + 秘书）
- Executor 负责人：Gavyn-side AI（同 AI 在用户 程序 指导下执行）
- Plan 编写方（AI 侧）：`Gavyn-side AI | ReEcho teammate-side AI`
- 实现编写方（AI 侧）：`Gavyn-side AI`
- 任务状态：`Proposed`
- 人工验收：`PendingBeforeClose`（视觉质量、布局可读性需人工判断）
- 本地规划 / 实现基线：`origin/main @ b227c00`
- 本地实现方式（可选，仅作交接说明）：一任务一 worktree（plan/60-ui-reskin），主工作树仅快进合并；用户逐步置入 WBP 切图素材，程序按资产命名重绑 / 按需重构对应 Widget。
- 依赖 / 阻塞：新 UI 切图素材由用户逐步置入 `Content/ReEcho/UI/`；缺素材的屏幕暂不接，待素材到位再落地；不阻塞其他已就绪屏幕。
- Writes:
  - `Source/ReEcho/Public/UI/**` 与 `Source/ReEcho/Private/UI/**`（各屏幕 Widget 类，按屏幕重构或重绑定 `BindWidget`）
  - `Content/ReEcho/UI/**`（新 WBP 资产，由用户置入；程序按资产命名更新 `BindWidget` 与 `ReEchoUIManagerSubsystem` 的 `ScreenClasses` 映射引用）
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`（同步重构后的 Widget/WBP 分工与命名约定）
  - 视情况 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`（仅当 UI 目录 / 所有权文字变化）
- Stable Reads:
  - `Source/ReEcho/Public/UI/Framework/ReEchoUIScreenTypes.h`（`EReEchoUIScreen` 枚举，稳定契约）
  - `Source/ReEcho/Private/UI/Framework/ReEchoUIManagerSubsystem.cpp`（`ScreenClasses` 屏幕→Widget 映射）
  - `Source/ReEcho/Private/UI/Framework/ReEchoUIFlowCoordinatorSubsystem.cpp`（Open/Close/Focus 屏幕流契约）
  - `Source/ReEcho/Private/Data/ReEchoCsvDataRegistry.*`（属性 / 构筑数值快照，仅读取用于显示绑定）
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`（各 `OpenScreen` 调用点，稳定接口；不修改）
- 影响模式：`SharedContract`（重构涉及 `BindWidget` 命名与 `ScreenClasses`/WBP 映射契约；纯资产替换部分为 `Isolated`）
- 兼容承诺 / 下游操作：所有屏幕的功能行为（按钮、输入切换、数据绑定、开关节点）保持不变；不把 CSV 派生数值硬编码进 UI；命中测试 / 可见性维持可用；不新增 / 删除 `EReEchoUIScreen` 枚举项（新增独立界面另立 Plan）；不改动 `UIFlow` 开关节点键。
- 明确排除：不修改战斗 / 玩法 / 存档逻辑；不新建 Runtime Module；不产生 `MOD-ReEchoUI` 之外的架构标识；不处理敌人血条（Plan57 已移除，本 Plan 不含 `WBP_ReEchoEnemyHealthBar`）；不改变游戏数值来源。

## 锁定目标

将当前所有 UI 屏幕（StartMenu / Loadout / Settings / Restart / TraitChoice / InventoryShop / Stats / Weather / EncounterHud / PlayerHud 及其子 Widget：LoadoutEntry、TraitCardEntry、StoredEchoEntry、EchoManagement、AttackMode、HealthBar[已停用]、Weather）整体换成新视觉风格与设计稿布局，**功能与数据绑定不变**。按用户置入素材的节奏逐屏落地；必要时对当前 Widget 结构做局部重构，以干净地承接新 WBP 资产。本 Plan 是 Plan45（UI 交互占位资源）之后的正式视觉落地，最终替换占位美术。

## 架构影响与设计决策

- 受影响架构标识：`AREA-UI`（`Source/ReEcho/{Public,Private}/UI/`）；文档入口 `MOD-ReEchoUI`（`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`）。实现仍属 `MOD-ReEcho`，不新增 Runtime Module，故无新 `MOD-*` 标识。
- 对应模块文档：`MOD-ReEchoUI.md` 必须进入 Writes 并在关闭前更新（重构后的 Widget/WBP 分工、命名约定、资产引用规则）；`MOD-ReEcho.md` 若 UI 目录 / 所有权文字变化则更新，否则在关闭审阅中说明"无需修改"。
- 设计意图：视觉与布局与功能逻辑解耦——Widget 只承担"契约绑定 + 行为"，美术完全由 WBP 资产承载；统一命名约定，使用户可逐步替换而程序改动局部可控、可验证。
- 权威状态与依赖：不改变屏幕枚举语义、`ScreenClasses` 映射键、`UIFlow` 开关节点；仅替换资产与对应 `BindWidget` 名。依赖用户素材供给节奏（见依赖 / 阻塞）。
- 决策记录：
  - 策略 **A 优先、B 按需**：用户置入新 WBP 后，若资产内命名控件与现有 `BindWidget` 完全一致 → 仅换资产引用（最省事、风险最低，影响模式 `Isolated`）；若设计稿结构不同（控件树 / 命名变化）→ 对该屏幕做 **B 局部重构**（改 C++ Widget 以匹配新结构，影响模式 `SharedContract`）。代价：B 阶段须重测该屏功能与命中测试。
  - 不一次性重做整套 UI 框架（`UIManager`/`UIFlow`），仅按屏幕替换内容；避免引入架构风险与回归。
  - 数据绑定（属性 / 构筑 / 抽卡 / 数值）继续走 `ReEchoCsvDataRegistry` 快照与既有 `GetAttributeRawValue` 等接口，绝不内联数值。
- 相关文档同步范围：
  - `ARCHITECTURE.md`：无拓扑变化 → 关闭时审阅说明"无需修改"。
  - `README.md`（CODEBASE_MAP 索引）：无路由变化 → 关闭时审阅说明。
  - `modules/MOD-ReEchoUI.md`：更新（Writes）。
  - `modules/MOD-ReEcho.md`：审阅，按需更新 UI 目录描述。
- 关闭前逐项填写审阅结果（见末尾"架构文档审阅结果"）。

## 锁定验收

- [ ] 每屏换皮后功能可观察：按钮 / 输入切换 / 数据正确显示，无输入死锁。
- [ ] `scripts/ue/Build-Editor.cmd` 通过（UHT/UBT 退出码 0）。
- [ ] `python scripts/validate_project.py` 通过（CSV schema / UTF-8 / 二进制清单）。
- [ ] 仅在视觉质量、可读性、布局需要时请求人工验收（PendingBeforeClose）。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。
- [ ] 未新增 / 删除 `EReEchoUIScreen` 项；未改动 `UIFlow` 开关节点。

## Step 0 门禁

- 基线分支/提交：`origin/main @ b227c00`
- 引擎/构建可用性：Editor 可启动，Development 构建可用（Plan57 已验证门禁链路）。
- 现有聚焦测试结果：`ReEchoUIManagerSubsystemTests`（PlayerHud / EncounterHud 创建与 travel reset）须保持通过。
- 共享契约 / 难合并资源风险：`Content/ReEcho/UI/*.uasset` 为二进制，用户置入新资产时避免与并行 Plan（如 Plan58 敌人弹道视觉）的 UI 改动在同一文件冲突；按屏幕隔离提交。
- 基线损坏时的停止条件：若 `main` 在发布 / 实现期间前进（他人提交），立即 fetch 重做外部提交集成审计，不静默合并。

## 实现提纲

不修改锁定目标、验收或已发布契约；可优化实现细节。

逐屏单元（建议顺序，可按用户素材节奏调整）：
1. StartMenu（`WBP_ReEchoStartMenu` + `ReEchoStartMenuWidget`）
2. Loadout（`WBP_ReEchoLoadoutSelection` / `WBP_ReEchoLoadoutEntry` + 对应 Widget）
3. Settings（`WBP_ReEchoSettings` + `ReEchoSettingsWidget`）
4. Restart（`WBP_ReEchoRestart` + `ReEchoRestartWidget`）
5. TraitChoice（`WBP_ReEchoTraitCardChoice` / `WBP_ReEchoTraitCardEntry` + 对应 Widget）
6. InventoryShop（`WBP_ReEchoInventoryShopScreen` + `ReEchoInventoryShopWidget`，含 Plan56 时钟属性面板）
7. Stats（`WBP_ReEchoStatsScreen` + `ReEchoStatsWidget`）
8. PlayerHud / EncounterHud / Weather（HUD 组，常驻、不抢焦点）

每屏标准动作：
1. 检查最小相关代码 / 数据表面（该屏 Widget 的 `BindWidget` 清单 + `ScreenClasses` 映射）。
2. 比对用户置入的新 WBP 命名：一致 → 仅换资产引用（A）；不一致 → 局部重构 Widget 并重绑（B）。
3. 每次完成一个连贯变更后立即 Build 验证。
4. PIE 验证该屏功能与命中测试；必要时记录人工视觉验收。
5. 更新执行记录；跨 B 重构边界时在本地记录范围 / 契约变化。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | 项目/源码不变量通过 |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd` | UHT/UBT 退出码为 0 |
| 运行时行为变化时自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | `ReEchoUIManagerSubsystemTests` 等受影响报告通过 |
| 需要人工时 | 具名 PIE/可用性任务 | 记录人工结果或明确延期跟进 |

## 执行记录

### 变化

### 证据

### 剩余风险

### 人工验收结果/请求

### 架构文档审阅结果
