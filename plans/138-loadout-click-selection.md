# Plan 138 - 程序 - 开场角色与武器改为点击选中

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner 与 Executor）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（需 PIE 验证鼠标悬停与点击的实际交互手感）。
- 本地规划 / 实现基线：`origin/main@2b1fc6c5`。
- 本地实现方式：独立 worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan138-loadout-click-selection`，分支 `plan/138-loadout-click-selection`。
- 依赖 / 阻塞：以 Plan 132 的两阶段 Loadout UI 为现有契约；不依赖新美术。执行 Unreal 构建/自动化前需确认 Editor 已关闭，并遵守同克隆 Unreal 锁。
- Writes:
  - `plans/138-loadout-click-selection.md`
  - `Source/ReEcho/Public/UI/ReEchoLoadoutSelectionWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoLoadoutSelectionWidget.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoLoadoutSelectionTests.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `Design/UI/ReEcho_UI修改指导.md`
  - 精选 Win64 Editor 预构建包及 manifest（仅最终发布门禁刷新）。
- Stable Reads:
  - `Source/ReEcho/Public/UI/ReEchoLoadoutEntryWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoLoadoutEntryWidget.cpp`
  - `Content/ReEcho/UI/WBP_ReEchoLoadoutSelection.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoLoadoutEntry.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoLoadoutTooltip.uasset`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp` 的最终 `OnLoadoutConfirmed(CharacterId, WeaponId)` 接入点。
  - `plans/132-start-loadout-two-stage-ui.md`
- 影响模式：`SharedContract`（修正开场 Loadout 页的鼠标输入语义，但不改变最终委托签名、数据来源或 WBP 布局）。
- 兼容承诺 / 下游操作：鼠标 Hover 继续保留原生 Tooltip、全局悬停缩放和其他纯表现反馈；键盘/手柄 Focus 继续沿用现有候选预览/选中行为；只有鼠标点击才改变鼠标路径下的当前角色/武器、箭头、Selected/Unselected 图和确认对象。
- 明确排除：不修改选角/武器页面构图、用户已微调的 WBP、纹理、文字、箭头位置或 Entry 尺寸；不修改角色/武器资格、CSV、Run、存档、首关启动或最终确认委托；不把本轮扩展为键盘/手柄导航重构。

## 锁定目标

修正开场两阶段选择页把“最后一次鼠标悬停项”误当作“已选中项”的交互：

1. 角色阶段和武器阶段的鼠标 Hover 只负责 Tooltip、悬停缩放等瞬时表现，不修改当前选择。
2. 鼠标只有实际点击条目时才更新当前 `CharacterId` / `WeaponId`、Selected/Unselected 图、条目内箭头和确认按钮所指向的对象。
3. 未点击任何条目时，无论鼠标经过多少项，都保持初始全亮、无箭头、无可用确认对象。
4. 已点击 A 后再悬停 B，A 仍保持选中、箭头仍在 A、确认仍提交 A；点击 B 后才切换到 B。
5. 键盘/手柄 Focus 路径保持当前行为，避免破坏现有导航和无鼠标操作。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoUI`、`AREA-UI`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`，已加入 `Writes`。
- 设计意图：把瞬时指针状态（Hover）与持久候选状态（Selected ID）重新分离，防止鼠标经过即改变最终提交对象。
- 权威状态与依赖：`UReEchoLoadoutSelectionWidget::SelectedCharacterId` / `SelectedWeaponId` 仍是页面内未提交候选的唯一权威；本 Plan 只收窄鼠标可写入口为点击，不新增第二套状态。
- 决策记录：保留 Entry 的 `OnEntryHovered` 表现事件以避免破坏潜在蓝图/下游监听，但父 Selection Widget 不再把该事件路由到 `SelectCharacter` / `ChooseWeapon`。点击事件继续调用选择函数；Focus Preview 继续保留现状。相比在 Hover 后回滚状态，此方案没有瞬时箭头/确认对象抖动，也不需要复制状态。
- 相关文档同步范围：更新 `MOD-ReEchoUI.md` 和 `Design/UI/ReEcho_UI修改指导.md` 中 Plan 132 的“Hover/Focus/点击统一 Selected”旧说明；审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `README.md`，若模块拓扑和索引未变则记录无需修改。
- 关闭前逐项填写审阅结果：待实现后补充。

## 锁定验收

- [x] 角色阶段初始时，连续 Hover 四个角色不会设置 `SelectedCharacterId`，不会出现选择箭头或可用确认按钮。
- [x] 点击角色 A 后 Hover 角色 B，A 的 Selected 图/箭头与最终确认对象不变；点击 B 后才切换到 B。
- [x] 武器阶段满足同样的“Hover 不选中、点击才选中”规则。
- [ ] 鼠标 Hover 的原生 Tooltip、跟随定位和全局悬停缩放仍可用，且不出现锚定说明重复框。
- [x] 键盘/手柄 Focus 仍能沿用现有候选预览/选择与锚定说明回退，不影响确认流程。
- [x] 第一次确认仍只进入武器阶段；第二次确认仍只广播一次准确的 `(CharacterId, WeaponId)`。
- [x] `ReEcho.UI.LoadoutSelection` 自动化覆盖未选择 Hover、已选择后 Hover 其他项、点击切换和两阶段确认边界并通过。
- [x] Development Editor 构建、`python scripts/validate_project.py`、预构建一致性和 `git diff --check` 通过。
- [ ] 用户完成 PIE 鼠标交互人工验收后再关闭和发布实现。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@2b1fc6c5`；创建时远端最新 Plan 编号为 137。
- 引擎/构建可用性：UE 5.8 安装版；Editor/命令必须经 Git common-dir Unreal 锁串行化。
- 现有聚焦测试结果：Plan 132 已提供 `ReEcho.UI.LoadoutSelection.{Flow,Assets}`；实施前代码审计确认 Flow 当前明确断言“Mouse hover previews the matching character”，即旧行为与本轮目标相反。
- 共享契约 / 难合并资源风险：本轮不写二进制 WBP；C++ 热点限定为 Loadout Selection Widget 与其聚焦测试。发布 Plan 与实现前均需重新审计最新 main 对相同路径的传入变化。
- 基线损坏时的停止条件：现有 WBP 无法加载、点击事件未能唯一映射稳定索引、最新主线已重构同一选择状态机，或必须修改 GameMode/Run 权威契约才能实现时停止并报告。

## 实现提纲

1. 保留 Entry Hover 广播与 Tooltip/悬停视觉能力；Selection Widget 的 Hover 处理只隐藏键盘 Focus 回退说明并刷新表现，不再调用选择函数。
2. 保持 Click→`SelectCharacter` / `ChooseWeapon` 与 Focus Preview 路径不变，确认按钮继续只读 Selected ID。
3. 扩展 `ReEchoLoadoutSelectionTests.cpp`：角色和武器分别验证 Hover 不写 Selected ID；已点击 A 后 Hover B 不改变 A；点击 B 才切换；原两阶段单次最终广播继续通过。
4. 同步 UI 模块文档和蓝图微调指导中的输入语义，不修改 WBP 或用户布局。
5. 执行 Development 构建、聚焦自动化、静态校验与预构建检查；记录执行证据并交付用户 PIE 手测。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目、源码、Plan 与生成物边界通过 |
| C++ | `scripts\ue\Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0，精选 Editor 包与源码匹配 |
| 聚焦自动化 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.UI.LoadoutSelection` | Hover 不改变选择、点击改变选择、两阶段确认与资产契约通过 |
| 预构建 | `python scripts/ue/prebuilt_editor.py check` | manifest 与当前源码指纹一致 |
| 人工 | PIE 新游戏，角色页与武器页分别执行“Hover 多项→点击 A→Hover B→确认” | 未点击不误选；Hover B 不偷换 A；点击与最终提交一致 |

## 执行记录

### 变化

- `UReEchoLoadoutSelectionWidget` 保留角色/武器 Entry 的 Hover 事件绑定，但 Hover handler 不再调用 `SelectCharacter` / `ChooseWeapon`。有效 Hover 只关闭键盘 Focus 的锚定说明回退并刷新表现，Selected ID、箭头和确认对象保持不变。
- 点击与键盘/手柄 Focus 路径保持既有行为：点击更新鼠标候选，Focus 继续更新无鼠标候选；两次确认及最终委托签名未变。
- `ReEchoLoadoutSelectionTests.cpp` 新增角色/武器“未点击 Hover 不选中”“点击 A 后 Hover B 保持 A”“再次点击才切换”断言，并保留 Focus 与两阶段单次广播回归。
- `MOD-ReEchoUI.md` 与 UI 修改指导已把 Plan132 的 Hover/Focus/点击统一候选旧说明更新为 Plan138 的点击选中语义；没有修改任何 WBP、纹理或用户布局。

### 证据

- `scripts\ue\Build-Editor.cmd -Configuration Development`：成功，UHT/UBT 97 个 action 完成；精选 Editor 包刷新为 `build_id=55116800 source=733aa136d938`。
- `scripts\ue\Run-Automation.cmd -Filter ReEcho.UI.LoadoutSelection`：找到 2 项，`Assets` 与 `Flow` 均为 `Result={Success}`；Flow 包含本轮角色/武器 Hover 与点击断言。
- `python scripts/validate_project.py`：通过；仅输出既有 Dagger 移除后的 AttackPatternReplacement NOTE。
- `python scripts/ue/prebuilt_editor.py check`：通过，`modules=7 build_id=55116800 source=733aa136d938`。
- `python scripts/setup_lfs.py --check`：通过，LFS checkout 已 hydration；`git diff --check`：通过。

### 剩余风险

- 自动化覆盖状态写入和两阶段事务边界，但不能替代真实 Slate 指针路径的主观验收；仍需 PIE 确认 Hover Tooltip/缩放保留、箭头不跟鼠标偷换，以及实际点击目标与最终进入首关组合一致。
- 本轮不修改 WBP，用户此前对 Selection/Entry/Tooltip 的微调保持原样。

### 人工验收结果/请求

- `PendingBeforeClose`：请在本 Plan worktree 的 `ReEcho.uproject` 中执行角色页和武器页各一次“先 Hover 多项 → 点击 A → Hover B → 确认”，确认未点击不误选、Hover B 不改变 A。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：已更新 Hover 与 Selected ID 的职责边界、点击/Focus 写入口和 Tooltip 表现契约。
- `Design/UI/ReEcho_UI修改指导.md`：已更新 Loadout Designer 的交互说明，明确 Hover 不移动箭头或确认对象。
- `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅；模块拓扑、依赖方向和 UI→GameMode 最终委托边界未变，无需修改。
- `shared/CODEBASE_MAP/README.md`：已审阅；现有 `AREA-UI`→`MOD-ReEchoUI.md` 路由不变，无需修改。
