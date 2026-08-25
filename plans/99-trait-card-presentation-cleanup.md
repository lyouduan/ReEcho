# Plan 99 - 程序 - 构筑卡牌展示清理与确认按钮接入

## 协调

- Planner 负责人：当前程序路线 AI。
- Executor 负责人：当前程序路线 AI。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@ec95e402`。
- 本地实现方式（可选，仅作交接说明）：一任务一 worktree；实现 worktree 在 Plan 发布后从准确 `origin/main` 创建。
- 依赖 / 阻塞：修改 WBP 前 Unreal Editor 必须关闭；当前检查未发现正在运行的 Unreal Editor。
- Writes:
  - `plans/99-trait-card-presentation-cleanup.md`
  - `Source/ReEcho/Public/UI/ReEchoTraitCardEntryWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoTraitCardEntryWidget.cpp`
  - `Source/ReEcho/Private/UI/ReEchoTraitCardChoiceWidget.cpp`
  - `Source/ReEcho/Private/Tests/`
  - `Content/ReEcho/UI/WBP_ReEchoTraitCardEntry.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoTraitCardChoice.uasset`
  - `scripts/ue/`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
- Stable Reads:
  - `Content/ReEcho/Textures/UI/InteractionPlaceholder/PauseAndCombat/T_UI_Pause_ButtonLight.uasset`
  - `Content/SourceArt/UI/InteractionPlaceholder/Elements/PauseAndCombat/暂停按钮浅.png`
  - `C:/Users/gavynqiu/Documents/miniGame/正式-UI视觉/正式-UI视觉/游戏暂停&重开/暂停按钮浅.png`
  - `Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
- 影响模式：`Exclusive`；两个 WBP 是不可文本合并的二进制资产，修改期间串行 authoring。
- 兼容承诺 / 下游操作：保留卡牌稳定 ID、抽取/选择事务、标题/说明/插图和按钮索引语义；仅删除标签及纯色占位表现，确认按钮继续使用既有可用性与点击委托。
- 明确排除：不删除生产数据中的 Tags 字段，不改变卡牌效果、抽取资格、价格、等级、揭示动画或商店流程。

## 锁定目标

构筑三选一页面不再展示卡牌标签，也不再显示程序生成或运行时染色的纯色卡底；底部“确定”按钮使用用户指定的浅色暂停按钮美术，并保持选择前禁用、选择后可点击的现有行为。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` 的 `AREA-UI`，以及文档型入口 `MOD-ReEchoUI`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`，已加入 `Writes`；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 只读审阅。
- 设计意图：把纯表现留在 UMG/WBP；C++ 仅提供卡牌正文、图片、选择状态和委托，不再向条目注入仅用于旧占位表现的标签与颜色。
- 权威状态与依赖：不改变 Run/Cards 权威状态或模块依赖；只收窄 Trait Card Entry 的内部显示参数和 WBP 绑定契约。
- 决策记录：完整删除 WBP 标签与纯色框节点及其 C++ 可选绑定，而非折叠；确认按钮复用已导入且与用户源图一致的浅色暂停按钮纹理，避免重复运行时资产。
- 相关文档同步范围：审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 的 Trait Card 展示契约；如事实未变化，在执行记录逐项说明无需修改。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：待实现后记录。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：待审阅。
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：待审阅。
  - `shared/CODEBASE_MAP/README.md`：待审阅。

## 锁定验收

- [ ] `WBP_ReEchoTraitCardEntry` 不含标签控件与纯色/运行时染色卡底控件，C++ 不再写入这些表现。
- [ ] 无 WBP 的 fallback 不再绘制三张纯色按钮底板。
- [ ] `WBP_ReEchoTraitCardChoice` 的“确定”按钮使用指定浅色按钮纹理，真实按钮点击区、文字和禁用状态仍有效。
- [ ] C++/WBP 聚焦回归、Blueprint 编译、项目校验与构建检查通过。
- [ ] 用户在 PIE 中验收卡面干净、按钮外观与参考一致且确认流程可用。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@ec95e402`。
- 引擎/构建可用性：项目 UE 5.8 工具入口存在；最终证据在实现候选上执行。
- 现有聚焦测试结果：待实现前确认 Trait Card 测试清单。
- 共享契约 / 难合并资源风险：两个 WBP 为二进制 Exclusive 写面；使用 Git common-dir Unreal 锁串行 authoring。
- 基线损坏时的停止条件：基线 WBP 无法加载/编译、指定纹理不存在或 Trait Card 选择事务已有失败时停止并报告，不用视觉改动掩盖行为问题。

## 实现提纲

1. 审计两个 Trait Card WBP、配对 C++ 与聚焦测试，确认标签、纯色框和确认按钮的准确节点。
2. 移除条目标签/纯色框绑定与 fallback 颜色，保持图片、正文和选择语义。
3. 在选择页 WBP 以浅色暂停按钮纹理建立可编辑表现层，透明真实按钮承载点击与禁用。
4. 编译/保存 Blueprint，执行聚焦自动化、C++ 构建、项目校验和 diff 检查。
5. 更新执行记录、模块文档审阅结论，并交给用户进行 PIE 视觉/交互验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 资产 | Trait Card authoring/audit 脚本与 `CompileAllBlueprints` | 标签/纯色框节点不存在，确认按钮纹理正确，两个 WBP 可编译保存 |
| C++ | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0 |
| 聚焦行为 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.TraitCard`（按现有实际测试名收窄） | 条目/选择/确认状态回归通过 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目、源码和内容不变量通过 |
| 人工 | PIE 进入普通三选一与商店同级卡组选择 | 无标签/纯色卡底；确定按钮外观、禁用与点击符合参考 |

## 执行记录

### 变化

- 待实现。

### 证据

- 待执行。

### 剩余风险

- 视觉对齐和不同分辨率缩放需要用户在 PIE 中验收。

### 人工验收结果/请求

- `PendingBeforeClose`：待用户检查普通三选一和商店卡组选择两条入口。

### 架构文档审阅结果

- 待实现后补齐。
