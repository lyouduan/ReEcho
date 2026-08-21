# Plan 70 - 程序 - 玩家受伤与低血量全屏红光反馈

## 协调

- Planner 负责人：当前对话程序 Planner。
- Executor 负责人：当前对话独立程序 Executor（`plan70_executor`）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`ReadyToPublish`。
- 人工验收：`PendingBeforeClose`。
- 本地规划基线：`origin/main@2027652`；最终发布基线：`origin/main@8ba2ec4`（已审计 Plan67/69、Settings/StartMenu 与预构建包更新，并在隔离发布副本中三方整合 Plan70 后重新 FullRebuild）。
- 本地实现方式：用户已确认不采用一任务一 worktree，直接在当前本地 `main` 工作区执行；必须保护现有未跟踪的 BadRabbit/Rabbit Death 美术目录。
- 依赖 / 阻塞：受伤事实复用 `UReEchoCombatEventsComponent::OnHurt`；生命变化复用 `UReEchoCombatantComponent::OnHealthChanged`。最终红光颜色、范围、曲线与呼吸观感需要用户在 PIE 验收。
- Writes:
  - `plans/70-player-hurt-screen-feedback.md`
  - `Source/ReEcho/Public/UI/ReEchoPlayerHudWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoPlayerHudWidget.cpp`
  - `Source/ReEcho/Public/UI/ReEchoPlayerScreenFeedbackWidget.h`（新增）
  - `Source/ReEcho/Private/UI/ReEchoPlayerScreenFeedbackWidget.cpp`（新增）
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`（仅显式传入玩家 Combat Events）
  - `Source/ReEcho/Private/Tests/ReEchoPlayerScreenFeedbackWidgetTests.cpp`（新增聚焦测试）
  - `Content/ReEcho/UI/WBP_ReEchoPlayerHud.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoPlayerScreenFeedback.uasset`（新增）
  - `Content/ReEcho/Materials/UI/M_UI_PlayerHurtVignette.uasset`（新增；准确目录可由 Executor 在既有 UI 资产规范内调整并同步本 Plan）
  - `scripts/ue/author_plan70_hurt_vignette.py`（新增幂等 Editor 作者ing/验证脚本）
  - `scripts/ue/author_plan70_screen_feedback_widget.py`（新增幂等 WBP 作者ing/编译/验证脚本）
  - `Content/Level00.umap`（用户补充要求：场景切换为 SC02，保留既有镜头、碰撞和装饰布局）
  - `scripts/ue/switch_level00_to_sc02.py`（新增 Editor API 切换脚本）
  - `scripts/ue/verify_level00_sc02.py`（新增 SC02 专项只读引用链校验）
  - `scripts/ue/verify_plan52_scene_assets.py`（Level00 预期场景同步为 SC02）
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `Binaries/Win64/` 下 `GIT_RULES.md` 允许的最终预构建包文件
- Stable Reads:
  - `Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`
  - `Source/ReEchoCombat/Public/Combat/ReEchoCombatantComponent.h`
  - `Source/ReEcho/Public/Player/ReEchoPlayerPawn.h`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxComponent.*`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`
  - `plans/49-combat-niagara-vfx-integration.md`
  - `plans/51-fix-restart-hud-screen-reset.md`
- 影响模式：`SharedContract`（扩展 Player HUD 初始化参数并新增 UI 内部表现契约；只读消费 Combat 事件，不修改伤害结算、生命权威或 Combat Module）。
- 兼容承诺 / 下游操作：红光反馈缺失、资产加载失败或 Widget 未绑定时只降低视觉反馈，不得影响伤害、生命、暂停、Restart、存档或战斗流程；原生 Widget fallback 必须可安全运行。
- 明确排除：不使用 `SceneViewExtension`、自定义 RDG Pass 或 Camera Post Process；不修改伤害数值、格挡语义、死亡流程、Combat 公共事件结构或生产数据表；不把视觉参数写入 Combat；不修改或提交现有未跟踪 BadRabbit/Rabbit Death 资产。

## 锁定目标

1. 玩家受到 `AppliedDamage > 0` 且非格挡的最终伤害时，屏幕左右立即出现一次短促红光，并平滑衰减；连续受击重触发单一效果，不生成或排队多个 Widget。
2. 玩家生命降低时允许出现持续的低血量红光；血量越低可配置为越明显，但启用阈值、强度、颜色、边缘宽度、柔和度和呼吸速度均为表现层可调参数，不固化当前讨论数值为玩法常量。
3. 瞬时受击脉冲与持续低血量底色使用独立输入，在统一上限内合成；回血应平滑减弱或关闭低血量效果，死亡时停止循环并让死亡流程接管。
4. 效果只属于当前玩家的 Player HUD；敌人/Echo 受伤、零伤害、格挡、初始化、存档恢复不会误触发受击脉冲。
5. 全屏表现适配窗口模式、16:9 与 21:9；中心战斗区域保持可读，暂停/结算等更高 UI 层不被遮挡。

## 架构影响与设计决策

- 受影响架构标识：
  - `MOD-ReEcho` / `AREA-UI`：新增 Player HUD 内部的全屏反馈子控件与显式事件接线。
  - `MOD-ReEchoUI`：维护 Player HUD/WBP 分工、全屏反馈入口和人工验收边界。
  - `MOD-ReEchoCombat` / `AREA-AbilityCombat`：仅稳定读取既有 `OnHurt` 与生命变化事件；不修改模块代码或权威契约。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `MOD-ReEchoUI.md`，二者均已加入 `Writes`；关闭前审阅 `MOD-ReEchoCombat.md`，若事件事实未变化则在执行记录中写明无需修改。
- 设计意图：Combat 只发布结算事实，Player HUD 只绑定并路由当前玩家事件，独立 Screen Feedback Widget 拥有效果状态，WBP/UI Material 拥有视觉形状与可调曲线。任何表现失败均不得反向改变玩法。
- 权威状态与依赖：玩家生命仍由 `UReEchoCombatantComponent`/Combat 权威拥有；新 Widget 只保存瞬时显示状态。`ReEcho` 主模块继续单向依赖 `ReEchoCombat`，不新增 Runtime Module 或反向依赖。
- 决策记录：
  1. 采用 `UMG + User Interface Material`，而非 `SceneViewExtension`：本需求不需要 SceneColor/GBuffer/RDG，UMG 更符合现有 UI 生命周期、分辨率适配和美术调参边界。
  2. 新建 `UReEchoPlayerScreenFeedbackWidget`，不把动画状态堆入血条控件；未来治疗、护盾破裂等屏幕反馈可复用该表现入口，但本 Plan 不预建未使用的通用事件总线。
  3. HUD 初始化显式接收 `UReEchoCombatEventsComponent`，不从 Combatant Owner 隐式查找组件；依赖可见且便于测试。GameMode 只负责装配，不保存反馈状态。
  4. `OnHurt` 只驱动瞬时脉冲，`OnHealthChanged` 只驱动持续低血量输入，避免回血/初始化误触发受击。
  5. 低血量映射采用 WBP 可编辑曲线或等价可调配置，C++ 仅归一化生命比例、过滤事件并钳制安全范围；最终默认参数由人工 PIE 调整。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：关闭前审阅；预计无 Runtime 拓扑或跨模块不变量变化，无需修改正文。
  - `shared/CODEBASE_MAP/README.md`：关闭前审阅；预计无新稳定模块/AREA 路由，无需修改正文。
  - `MOD-ReEcho.md`：维护 Player HUD 对 Combat 最终事件的只读表现装配与代码位置。
  - `MOD-ReEchoUI.md`：维护全屏反馈控件、WBP/材质职责、可调参数和验证路径。
  - `MOD-ReEchoCombat.md`：仅审阅既有事件是否足够；不因只读消费者制造文档 diff。
- 关闭前逐项填写审阅结果：在执行记录记录每份相关文档“已更新”或“已审阅、无需修改”及理由。

## 锁定验收

- [ ] 玩家实际受伤出现左右红光并在配置时长内衰减；零伤害、格挡、敌人/Echo 受伤不会触发。
- [ ] 低血量红光随配置曲线变化，回血平滑减弱；瞬时脉冲与持续底色合成后不超过配置上限。
- [ ] 连续受击只重触发单实例反馈，不叠加 Widget、不形成长期不透明红屏。
- [ ] 初始化、重新绑定、销毁、Restart/Travel 后 Delegate 数量正确，无重复触发或悬空引用。
- [ ] 原生 fallback 与 `WBP_ReEchoPlayerHud` 均可安全运行；反馈资产缺失不影响玩法。
- [ ] 聚焦自动化覆盖过滤、强度钳制、低血量映射输入和重绑定清理；修改的 C++ 完成格式化。
- [ ] Editor Development 构建、聚焦自动化、`python scripts/validate_project.py`、`git diff --check` 通过；发布候选完成 `-FullRebuild` 并刷新允许的预构建包。
- [ ] 用户在 PIE 验收 16:9/21:9、连续受击、回血、严重低血量、暂停与死亡切换时的视觉质量。
- [ ] 未提交现有未跟踪美术目录或精选预构建允许列表之外的 UE 生成产物。

## Step 0 门禁

- 基线分支/提交：`origin/main@8ba2ec4`；发布前已审计该基线相对本地候选的全部外部提交，并在隔离副本完成三方整合与 FullRebuild。
- 引擎/构建可用性：UE 5.8 安装版；构建/Editor 命令前使用 Git common-dir Unreal 锁，并要求用户保存和关闭交互式 Editor。
- 现有聚焦测试结果：不复用旧 HUD/VFX 构建证据；本候选重新验证。
- 共享契约 / 难合并资源风险：`WBP_ReEchoPlayerHud.uasset` 为 Exclusive 二进制资产，执行前必须确认同路径无外部变化；当前未跟踪 BadRabbit/Rabbit Death 目录不在 Writes，必须保持原样。
- 基线损坏时的停止条件：fetch 发现新外部提交、WBP 同路径变化、基线构建失败或无法安全创建/编辑 UE 资产时停止并报告，不用文本方式修改 `.uasset`。

## 实现提纲

1. 新增独立 Screen Feedback 原生 Widget，提供低血量输入、受击重触发和受控合成；保留无资产 fallback。
2. Player HUD 显式绑定 Combat Events 与 Combatant，在重新初始化和销毁时对称解绑；GameMode 装配当前玩家的 `Combatant`、`CombatEvents` 和头像。
3. 通过 Unreal Editor 创建 UI Material、反馈 WBP，并把反馈子控件接入 `WBP_ReEchoPlayerHud`；所有全屏节点 `HitTestInvisible`，保持更高屏幕层覆盖关系。
4. 新增聚焦自动化，验证纯计算/过滤/重绑定契约；维护相关 `CODEBASE_MAP` 文档。
5. 完成构建、静态检查与自动化后交给用户 PIE 调参验收；视觉参数返修不改变 Combat 契约。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| C++ 格式 | 仓库 `.clang-format` 处理修改的 `.h/.cpp` | diff 仅含预期语义与格式 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0 |
| 聚焦自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.PlayerScreenFeedback` | 过滤、映射、重触发/重绑定相关报告通过 |
| Blueprint/资产 | Unreal Editor Compile/Save + `CompileAllBlueprints` 或等价资产加载检查 | WBP/材质可加载且绑定完整 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目与文本不变量通过 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终候选及精选预构建包匹配 |
| 人工 | PIE：16:9/21:9、连续受击、回血、低血量、暂停、死亡/Restart | 用户记录 Passed 或明确延期 |

## 执行记录

### 变化

- 新增 `UReEchoPlayerScreenFeedbackWidget`：受击脉冲与低血量底色独立输入、统一上限合成、连续受击单实例重触发、死亡清理，并提供无资产原生左右边缘 fallback。
- 通过 `scripts/ue/author_plan70_hurt_vignette.py` 在 Unreal Editor API 内幂等创建 `/Game/ReEcho/Materials/UI/M_UI_PlayerHurtVignette`；UI Domain 的 Translucent 材质按屏幕 UV 生成左右对称软渐变，公开 `TintColor`、`EdgeWidth`、`Softness`。原生反馈控件优先用全屏 `UImage` 动态材质实例消费这些参数，加载失败才回退纯色 Border。
- Player HUD 显式接收并对称绑定当前玩家 `Combatant` / `CombatEvents`；`OnHurt` 过滤零伤害与格挡，生命变化只更新持续反馈。GameMode 三处装配调用已传入 `Player->CombatEvents`。
- 新增 `ReEcho.UI.PlayerScreenFeedback.*` 聚焦自动化；维护 `MOD-ReEcho` 与 `MOD-ReEchoUI`。
- 未在 Unreal Editor 外修改 `.uasset`。材质与 `WBP_ReEchoPlayerScreenFeedback` 均由 Editor API 创建、编译并保存；WBP 提供项目级可调默认值表面，原生 `RebuildWidget` 在其没有自定义根时仍构建全屏 Image/fallback。Player HUD 通过 `FClassFinder` 硬引用并实际构造该 WBP Class；反馈 Widget 通过 `FObjectFinder` 硬引用材质，二者均进入 cook 依赖收集，加载失败仍安全回退原生类/Border。
- 按用户补充要求将 `Level00` 的唯一 Arena Actor 切换为 `BP_ArenaScene_SC02` / `DA_ArenaScene_SC02`，保留原位置、镜头边界、碰撞配置、组件相对变换和既有装饰布局；该地图切换与受伤反馈没有运行时耦合。

### 证据

- 实际 Executor 基线：`origin/main@36df622`，开始时 `HEAD` 与远端一致；工作区仅有两组受保护的未跟踪美术目录。
- `Build-Editor.cmd -Configuration Development`：通过，并刷新 ReEcho Editor 预构建包。
- `Run-Automation.cmd -Filter ReEcho.UI.PlayerScreenFeedback`：最终候选发现 3 项，Filter、Lifecycle 与 Math 均通过；覆盖过滤、强度钳制、低血量映射输入、重复初始化与事件源重绑定清理。
- `Build-Editor.cmd -Configuration Development -FullRebuild`：最终 WBP/cook 硬引用候选通过，刷新 7 个允许模块的 Win64 Editor 预构建包，源码指纹 `eaa2e8872a90`。
- `author_plan70_hurt_vignette.py` 使用 `UnrealEditor-Cmd -DDC-ForceMemoryCache -ExecutePythonScript=...` 连续执行两次均退出 0；首次通过 Editor API 创建并保存表达式图，第二次对被 CDO 硬引用的既有资产执行无写入幂等验证，日志确认 `domain=UI`、`blend=Translucent`、标量参数 `EdgeWidth/Softness` 与向量参数 `TintColor`。
- `author_plan70_screen_feedback_widget.py` 使用相同 Editor 命令环境幂等创建、编译、保存并重新加载 `WBP_ReEchoPlayerScreenFeedback`；日志确认继承的 12 个调参项均存在并写入初始默认值，包括低血量阈值、最大强度、曲线指数、呼吸和受击参数。
- `python scripts/validate_project.py`：通过；`git diff --check`：通过（仅有 Git 的 LF→CRLF 工作区提示）。
- 最新远端发布候选基线 `origin/main@8ba2ec4`：Plan70 三方应用无冲突，GameMode 仅三处 HUD 初始化增加 `CombatEvents`，保留 Plan67/69 逻辑；`Build-Editor.cmd -Configuration Development -FullRebuild` 通过，刷新 7 个允许模块的预构建包，源码指纹 `1eeb93d7fd2b`。
- 最新候选 `ReEcho.UI.PlayerScreenFeedback` 聚焦自动化发现 3 项，Filter、Lifecycle、Math 全部 `Success`；`verify_level00_sc02.py` 确认 SC02 Texture → Material → Profile → Blueprint → Level00 引用链通过；项目静态校验与 `git diff --check` 通过。

### 剩余风险

- 最终视觉参数必须经 PIE 人工判断；自动化不能证明舒适度和可读性。
- UI Material 已具备真实左右软渐变与运行时接入；WBP Class Defaults 已提供阈值、低血量最大强度、曲线指数、颜色、边宽、柔和度、呼吸和受击参数的 Editor 调参入口。最终观感仍须 PIE 人工判断。
- `WBP_ReEchoPlayerHud.uasset` 是二进制独占写入面，实施期间若远端同路径变化需停止审计。

### 人工验收结果/请求

`PendingBeforeClose`：实现完成后由用户验收红光强度、覆盖范围、呼吸节奏和多宽高比表现。

### 架构文档审阅结果

- `MOD-ReEcho.md`：已更新，记录 Combat 最终事件到 Player HUD 的只读表现装配。
- `MOD-ReEchoUI.md`：已更新，记录反馈控件职责、fallback、代码路线与验证入口。
- `MOD-ReEchoCombat.md`：已审阅、无需修改；既有 `OnHurt` / 生命事件事实和权威归属未变化，仅新增消费者。
- `ARCHITECTURE.md`：已审阅、无需修改；未新增 Runtime Module 或改变依赖拓扑。
- `README.md`：已审阅、无需修改；没有新增稳定模块或 AREA 标识。
