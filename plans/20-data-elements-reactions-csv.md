# Plan 20 - data - elements, statuses and reactions CSV migration

## Locked goal

将元素体系、必要状态定义和元素反应迁移到 Plan 18 的 CSV 注册表，使策划可以配置元素身份、触发/附着角色、反应配对、数值参数、状态引用与已注册反应行为，同时保持确定性伤害结算、GAS 权威属性和玩家/回响一致语义。

## Dependency status

Plan 18 已于 2026-08-10 人验、审查并合并，但其实现仍以中央注册表、manifest/schema、打包依赖和单体校验器为扩展中心；因此本 Plan 不再与 Plan 19 并行。必须等待 Plan 19 完成、人验并合并，基于它拆出的“通用 CSV 内部层 + 领域读取器 + 唯一注册表编排/原子发布”接口再启动。

Plan 20 在执行期独占生产 manifest/schema、`FReEchoCsvDataSnapshot`、注册表编排、内建行为注册、`ReEcho.Build.cs` 和静态校验入口。Plan 21 只能在本 Plan 合并后开始运行时接线。

## Source baseline

- `元素体系Y`：火/雷触发元素，草/水附着元素，以及灼烧、汽化、生长、导电、两种有顺序的强化。
- `状态Z`：元素免疫、灼烧、嘲讽、眩晕、减速、无敌、隐身、流血等反应/构筑会引用的状态。
- 当前 C++ 只把 Water/Flame/Grass 视为战斗元素，以少量双倍伤害配对模拟反应；与工作簿中的 Lightning、状态和六种反应并不等价。

## Locked acceptance

### 🤖 Automated

- [ ] 元素表将 ID、角色（Trigger/Attachment 等）、显示文本键、颜色/视觉键和 enabled 分列；Reaction/Status 通过稳定 ID 引用，不靠中文名称匹配。
- [ ] 状态表提供反应所需的持续时长、叠加/刷新/互斥策略和已注册状态行为；纯展示描述不作为运行时判定。
- [ ] 反应表分离 ordered trigger/attachment、ReactionBehaviorId、FormulaId/有限公式类型、数值系数、范围、状态引用、暴击/回响修正规则。
- [ ] 不执行 Excel/CSV 中的任意公式字符串。伤害/范围计算使用白名单 FormulaId 与类型化系数；独特行为通过注册 handler。
- [ ] 至少实现并自动化覆盖工作簿六个反应：灼烧、汽化、生长、导电、草→水强化、水→草强化；顺序敏感配对不能被无序集合合并。
- [ ] Lightning 成为可识别的战斗元素；元素免疫、附着清除/阻断、强化不叠加等状态机行为与表一致且确定。
- [ ] 玩家与回响使用同一注册快照和反应结算；回响触发是否受 EchoEfficiency 影响、反应能否暴击等由表字段/已注册规则明确，不能散落在调用方。
- [ ] 元素伤害仍经 GAS/现有权威伤害路径结算；Actor 只持目标状态与表现反馈，不重新成为数值权威。
- [ ] 当前可玩元素武器与既有 Plan 14 自动化保持或按人批准的工作簿语义更新；任何有意行为变化必须在执行经验和 UI/遥测中可见。
- [ ] 旧 `elements.json`、`reactions.json`、`statuses.json` 与 C++ 重复常量退出该领域运行时权威。
- [ ] Editor build、元素/状态/反应自动化、全量 `ReEcho.*`、静态校验和 `git diff --check` 通过。

### 🎮 Human PIE

- [ ] 人逐一触发六种反应，确认颜色/状态提示、范围、持续时间、顺序差异和伤害可读。
- [ ] 人分别用玩家与回响触发反应，确认两者遵守同一表定义且回响修正规则符合设计。
- [ ] 人修改一项反应系数或持续时长、重启 Run 后确认无需重新编译即可生效。

## Step 0 gates

1. 只在 Plan 19 已人验并合并后，从更新后的 main 创建 `plan/20-elements-reactions-csv`；读取 Plan 18、19 最终 Execution notes、当前 `FReEchoCsvDataSnapshot`/领域读取接口、内建行为注册入口和数据测试。若 Plan 19 未合并则停止。
2. 在协调板独占本 Plan 的公共数据文件；不得复制或绕过 Plan 19 留下的通用读取/原子发布路径。
3. 将本领域的 `BehaviorId`、`EffectKind`、`FormulaId`/状态行为通过显式内建注册函数注册，并保证发生在 `LoadAndPublishDefault()` 之前；不得依赖静态初始化顺序。
4. 采集当前 `ReEchoElementReaction`、敌人附着状态、投射物元素传递、GAS damage source 和 Plan 14 自动化基线。
5. 对照工作簿与现有 JSON/C++ 列出有意差异，特别是 Lightning 缺失、当前双倍伤害简化和工作簿 Formula 描述。行为改变必须由人确认，不以“表里写了”自动覆盖已验收原型。
6. 先确定 FormulaId 白名单与单位：元素攻击、ReactionEfficiency、DamageIncrease、范围、秒、UE 厘米如何组合。若汽化平方公式等存在量纲/数值歧义，设为阻断 gate 请人拍板，禁止自行改成看似合理的数值。
7. 先确定顺序敏感状态机和抗递归规则，确保导电/范围附着不会因新反应无限递归或因遍历顺序产生不确定结果。
8. 本任务不修改武器槽表；只提供 Plan 21 可消费的稳定 ElementId/Reaction API，并在最终 Execution notes 写清。

## Target table responsibilities

生产表文件固定为 `elements.csv`、`statuses.csv`、`reactions.csv`；具体列名服从 Plan 19 已落地的领域读取约定，语义至少包括：

- Element：ID、role、display/color/presentation keys、enabled。
- Status：ID、behavior ID、duration/stack/refresh policy、tags、enabled。
- Reaction：ID、ordered incoming/attached element IDs、behavior ID、formula ID、coefficients/radii、status references、crit/echo flags、enabled。
- 可变数值为独立字段或类型化参数；描述文本只服务 UI/策划，不参与计算。
- 运行时范围、枚举、顺序配对、外键和 FormulaId 白名单由类型化 C++ 元素领域读取器校验；`csv_schema.csv` 同步服务静态校验/文档，但不是运行时公式解释器。

## Implementation outline

1. 新增独立元素/状态/反应领域读取器，扩展共享快照与注册表编排；不把解析细节搬回 Plan 18 的中央 cpp。
2. 导入/验证三个生产表与引用图，生产 manifest 缺任一必需表或任一领域失败时整包不发布。
3. 将纯判定核心改为读取不可变反应定义，保留一个确定的结算入口。
4. 以有限 handler 实现六种反应和所需状态，不按 ReactionId 在 Actor 中散布 switch。
5. 接回敌人状态、GAS 伤害、投射物/武器元素、表现反馈和回响调用方。
6. 添加顺序、递归、范围目标排序、状态刷新/阻断、原子失败、回响修正和表变值自动化。
7. 移除该领域旧运行时重复值，更新数据 README、manifest/schema/RuntimeDependencies、静态领域校验、CODEBASE_MAP、PROJECT_STATE、LESSONS 和 Execution notes。

## Expected ownership

- `Content/Data/elements.csv`、`statuses.csv`、`reactions.csv` 及生产 manifest/schema/README/打包依赖。
- 独立元素领域读取器、共享快照扩展、注册表编排与显式内建行为/公式注册。
- `Combat/ReEchoElementReaction.*` 及新增的领域 registry/handler。
- 必要的 `Core/ReEchoTypes.*`、GAS effect/source、Enemy/Projectile 状态接线。
- 元素反应自动化和静态验证。
- 不修改武器攻击模组/插槽内容；与 Plan 21 重叠的公共文件需提前在协调板声明。

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| CSV static | `python scripts/validate_project.py` | 元素/状态/反应 ID、配对、公式、引用与范围通过；负例精确失败 |
| Build | format + `scripts/ue/Build-Editor.cmd` | UE 5.8 Editor build 成功 |
| Automation | 六反应、顺序、状态、递归/目标排序、玩家/回响 + 全量 `ReEcho.*` | 结算确定，表参数变值可见，无旧测试回归 |
| Diff | 初版↔终版、`git diff --check` | Plan 18 适配和批准的设计差异记录完整 |
| Human | 六反应 PIE | 伤害、范围、持续、提示和回响语义通过人验 |

## Risks and exclusions

- 公式量纲和平方成长可能导致极端数值；没有人拍板不擅自“平衡修正”。
- 连锁和范围附着必须使用稳定目标排序/已访问集合，不把 TSet/TMap 非稳定遍历暴露到结算结果。
- 不在本 Plan 重做武器架构或所有状态系统；只实现这些表和 Plan 19/21 已确认依赖的公共状态能力。
- 表驱动不等于无代码：新的独特反应行为仍需 C++ handler 和自动化后才能 enabled。
- 本 Plan 与 Plan 19/21 串行；不得为了并行而复制快照、manifest、行为注册或静态校验系统。

## Recommended executor model

强模型。该任务涉及确定性状态机、GAS 伤害、范围/连锁目标选择和工作簿公式翻译。

## Execution notes

### Changed

### Evidence

### Plan 18 contract adaptation

### Approved semantic differences from current prototype

### Public element/reaction contract for Plan 21

### Remaining risks

### Human validation requested

## 执行者启动 prompt

```text
你是 ReEcho 项目的执行者。先读 AGENTS.md，再按其最小读取顺序读 plans/20-data-elements-reactions-csv.md、已完成的 Plan 18 与 Plan 19 最终 Execution notes、Plan 14 执行经验，以及 shared/LESSONS.md §GAME 中元素/状态相关匹配条目；只有诊断失败时才读 §DEBUG。

前置：main 已合并 Plan 18 和经人验的 Plan 19，Plan 20 是当前唯一 CSV 领域迁移任务；Plan 21 尚未开工。否则停止并告诉人。然后在独立 worktree 的 `plan/20-elements-reactions-csv` 分支工作；参考工作簿为仓库上一级 `../回响肉鸽数值与构筑体系.xlsx`。

任务：沿用 Plan 19 拆出的通用读取层，新增独立元素领域读取器并扩展唯一快照/注册表编排，把元素、必要状态和六个元素反应迁入 CSV。显式注册所有内建行为/公式并保证先注册后启动加载。验收照 Plan 20 的 🔒 清单；不得执行任意公式、不得在 Actor 中按 ReactionId 散布特判、不得自行修正有歧义的平方公式/量纲，也不得修改 `.uasset`/`.umap`。把 Plan 18/19 适配和语义差异写入 Execution notes。

完成后显式提交到本地分支并告诉人；未经人明确确认不得 push，禁止修改或合并 main。
```
