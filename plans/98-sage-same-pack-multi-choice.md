# Plan 98 - 程序 - 智者同组 3 选 2 修复

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（选卡页多选视觉、交互和实际获得两张卡需用户 PIE 验收）。
- 规划 / 实现基线：`origin/main@797228875fad1106d3858074faded456d34184cb`。
- 本地实现方式：一任务一 worktree；Plan 发布后从最新 `origin/main` 创建 `ReEcho-plan98-sage-multi-choice`。
- 依赖 / 阻塞：依赖 Plan87 的类型化角色能力、Plan88 的逐关免费卡组、Plan97 复用 Trait Choice 页的付费商店卡组模式；商店卡组继续严格单选。
- Writes：
  - `plans/98-sage-same-pack-multi-choice.md`
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Source/ReEcho/Public/Run/CharacterAbilities/ReEchoCharacterAbilityRuntime.h`
  - `Source/ReEcho/Private/Run/CharacterAbilities/ReEchoCharacterAbilityRuntime.cpp`
  - `Source/ReEcho/Public/UI/ReEchoTraitCardChoiceWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoTraitCardChoiceWidget.cpp`
  - `Source/ReEcho/Public/ReEchoGameMode.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoCharacterPromotionTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoTraitTests.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `shared/LESSONS.md`（修正旧的“额外页面”经验为同组多选事务）
  - `Binaries/Win64/` 下 `shared/GIT_RULES.md` 允许的最终精选预构建包
- Stable Reads：
  - `策划数据源/【开普勒】回响数值与构筑体系.xlsx` 的 `角色体系J!D2` 与 `构筑体系G!E2:F2`
  - `Content/Data/character_abilities.csv`
  - `plans/87-forge-cleanup-and-character-abilities.md`
  - `plans/88-card-drop-system.md`
  - `plans/97-tiered-shop-card-pack-choice.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`
- 影响模式：`SharedContract`。Run 的免费选卡事务、Trait Choice UI Delegate 和 GameMode 适配同时变化；不修改 Cards 资格/授予算法、商店卡包事务、策划数值或 WBP 资产。
- 兼容承诺：Plan97 商店卡包仍是最多三张候选中购买一张；普通角色仍为 3 选 1；智者能力参数继续来自 `character_abilities.csv` 的 `Value=1 / Interval=4`；旧存档的 `NormalTraitSelections` 继续可读。
- 明确排除：不改卡牌投放 Tier、候选随机种子、商店价格、角色晋升规则、卡面资源和选卡页面布局；不新增第二套卡牌目录或在 Widget 直接授予卡牌。

## 已确认根因

1. 策划源定义“每第 4 次获得卡牌组时，可以额外选择 1 张”，产品语义已由用户确认是同组三张从选一张变为选两张。
2. 现有 Run 在第一张卡提交后才增加 `NormalTraitSelections`，随后设置 `BonusTraitChoicesRemaining` 并重新进入 `CardChoice`，因此生成的是第二组新的三选一。
3. Trait Choice Widget 只有单个 `SelectedOfferIndex`，Trait Delegate 只传一个 `CardId`，无法表达同组多选。
4. `ReEcho.Characters.SageBonusCadence` 当前通过，但它明确断言“打开第二次 bonus choice”，测试保护了错误语义。

## 锁定目标与设计决策

1. 每个实际投放的免费卡组只生成三张候选一次。普通情况要求精确选择 1 张；智者每第 4 个免费卡组要求从同三张中精确选择 2 张。
2. cadence 按“已完成免费卡组数”计算，而不是按获得卡牌数计算。同组获得两张只让计数增加一次；无 `FreeTier`、商店购买和智者额外获得的第二张均不增加组数。
3. Run 在生成免费候选时计算并持有本组 `RequiredSelectionCount`。Widget 只展示该只读配额并返回稳定 CardId 集合；GameMode 只转发批量命令。
4. 新增批量免费选卡事务：先验证阶段、精确数量、ID 唯一且全部来自本组候选，再在构筑副本上顺序授予；任一授予失败则卡牌、属性、货币、组计数和阶段全部不变。成功后一次提交、一次保存并进入 Planning。
5. 批量授予顺序按原三张候选顺序规范化，不依赖玩家点击顺序，保持确定性。现有单卡入口保留为 3 选 1 兼容 facade，但不能绕过智者本组必须选两张的配额。
6. Widget 用选中索引集合支持点击切换；达到配额前确认按钮不可用，超过配额的第三张不可再加入。标题显示“选择 1/2 张构筑卡牌”。没有确认按钮的原生 fallback 在达到精确配额时提交。
7. Plan97 商店模式固定 `RequiredSelectionCount=1`，继续广播单个 Shop ItemId；不得因 Trait 多选而改变价格检查、取消、拒绝恢复或商店层级。
8. 旧存档若携带 `BonusTraitChoicesRemaining`，仅作为迁移期已欠奖励处理：完成该遗留 bonus 页后清除，不计为新卡组；新流程不再写入该标记。`NormalTraitSelections` 的存储键保留，但代码和文档明确其历史值等价于已完成普通卡组数，无需 SaveVersion 升级。
9. 不修改工作簿或生产 CSV：当前 `Interval=4 / Value=1` 数值映射正确，本次修正的是 Run/UI 对“额外选择”的解释。

## 架构影响与文档同步

- `MOD-ReEcho / AREA-Run`：从“单卡提交后追加页面”改为“生成时解析配额 + 同组原子批量提交”；Run 保持构筑和阶段权威。
- `MOD-ReEcho / AREA-UI`、`MOD-ReEchoUI`：Trait 模式允许多选集合；商店模式仍单选。Widget 不持有玩法权威。
- `MOD-ReEchoCards`：Cards 仍逐张提供纯授予结果，Run 在同一候选事务中组合；具名审阅，预计正文无需修改。
- `ARCHITECTURE.md`：关闭前审阅；预计模块拓扑、依赖方向和权威边界不变。
- `CODEBASE_MAP/README.md`：关闭前审阅；预计 AREA 路由不变。
- `MOD-ReEcho.md`、`MOD-ReEchoUI.md`：更新智者同组多选、批量事务、UI 配额和 Plan97 单选隔离。
- `LESSONS.md`：现有 GAME 经验把智者描述成独立奖励选择，需修正为“卡组计数与组内卡数分离，批量原子提交”。

## 锁定验收

- [ ] 普通角色及智者第 1～3 个有效免费卡组仍为同组三张选一张。
- [ ] 智者第 4 个有效免费卡组显示同组三张并要求选择两张；确认后两张都进入构筑，不出现第二组选卡页。
- [ ] 同组两张只增加一次卡组计数；第 5～7 组恢复 3 选 1，第 8 组再次 3 选 2；无免费投放关和商店购买不计数。
- [ ] 少选、多选、重复 ID、非候选 ID或第二张授予失败时整组事务无部分状态；成功时保存和后续商店衔接只发生一次。
- [ ] Trait 多选 UI 支持选择/取消、精确配额确认、标题和选中表现；Plan97 商店卡包仍只能购买一张并可正常取消/余额拒绝恢复。
- [ ] 旧 `BonusTraitChoicesRemaining` 存档可完成欠下的一张奖励后清除，不递归、不重复计组。
- [ ] 角色、Trait、Shop、Save 聚焦自动化通过；修改的 C++ 完成格式化、Development FullRebuild、项目校验、预构建检查和 `git diff --check`。
- [ ] 用户在 PIE 以智者验证第 4 组同屏 3 选 2、获得两张、随后进入商店；人工验收后才设为 `Passed/Closed`。
- [ ] 未提交精选允许列表外 UE 生成物、机器本地路径或无关 Plan97 改动。

## 验证矩阵

| 层级 | 检查 | 预期证据 |
|---|---|---|
| 领域事务 | `ReEcho.Characters.SageBonusCadence`、Trait 新增批量用例 | 1/1/1/2 cadence、同组两张、组计数一次、错误输入原子拒绝 |
| UI 契约 | Trait Widget 聚焦自动化或确定性辅助测试 | 多选集合、精确配额、商店单选隔离 |
| 回归 | `ReEcho.Run.Traits`、`ReEcho.Run.Shop`、`ReEcho.Run.Save` 的实际可用过滤器 | 免费投放、Plan97 商店卡包和存档恢复不回归 |
| 构建 | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | 组合源码成功并刷新精选预构建包 |
| 静态 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 项目、包和文本门禁通过 |
| 人工 PIE | 智者第 4 个有效免费卡组选择两张并进入商店 | 用户确认交互、视觉和实际构筑正确 |

## 执行记录

### 变化

- 2026-08-25：策划反馈智者未正确生效；用户确认正确语义为同一个免费卡组从 3 选 1 变为 3 选 2，并授权开始修复。
- 2026-08-25：只读审计策划源、生产 CSV、Run、GameMode、Trait Widget 和现有测试，确认数据参数正确，错误位于 Run 的第二页面状态机和 UI 单选契约。
- 2026-08-25：实现前 fetch 确认 `origin/main@79722887`；Plan97 修改相同热点但语义限定于商店卡包单选，本 Plan 将在其上组合适配。

### 证据

- 策划源 `角色体系J!D2` 与 `构筑体系G!E2:F2` 均为“每第4次获得卡牌组时，可以额外选择1张”。
- `character_abilities.csv` 已正确映射 `SAGE_BONUS_CHOICE / Value=1 / Interval=4`。
- 当前 `ReEcho.Characters.SageBonusCadence` 自动化通过，但断言的是“第四次普通单卡提交后再打开一次 bonus choice”，属于错误测试预期。

### 剩余风险

- WBP 目前按单选表现制作；C++ 可支持多选，但两张同时高亮、标题可读性和确认手感必须由用户 PIE 验收。
- Plan97 刚修改同一 Trait Widget 以复用商店付费卡包；实现和发布前必须专项回归商店模式，不能只验证智者。

### 人工验收结果/请求

- `PendingBeforeClose`：待实现构建完成后，由用户验证第 4 个有效免费卡组同屏选择两张、构筑增加两张且只进入一次后续商店。

### 架构文档审阅结果

- `ARCHITECTURE.md`：待关闭前审阅。
- `CODEBASE_MAP/README.md`：待关闭前审阅。
- `MOD-ReEcho.md`：待更新。
- `MOD-ReEchoUI.md`：待更新。
- `MOD-ReEchoCards.md`：待具名审阅。
