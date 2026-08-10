# Plan 19 - data - character and build CSV migration

## Locked goal

将角色体系与构筑体系迁移到 Plan 18 建立的 CSV 运行时注册表：策划可通过 CSV 配置角色基础数值、初始武器、展示信息、角色被动、卡牌池、流派标签、通用数值效果和已实现的特殊行为，同时保持当前确定性抽卡、角色晋升、GAS 属性初始化和回响 BuildSnapshot 契约。

## Dependency status

本 Plan 是完整下游任务书，但实现层必须在 Plan 18 完成、经人验和规划者合并后再校准。规划者届时根据 Plan 18 的终版 diff、Execution notes 与公共头文件修订本 Plan 的文件名、接口和验证命令；执行者不得在 Plan 18 未合并时自行复制一套加载器。

## Source baseline

- `角色体系J`：4 个角色、基础生命/物攻/元攻/基础武器和各自能力。
- `构筑体系G`：1–3 级卡牌、流派、效果说明、评审、词条和部分价值。
- `属性S`：角色、卡牌和武器使用的属性词条、数值类型与范围。
- 当前工作簿存在 ID/内容缺口：角色使用 `J_01`–`J_04`，而运行时已有 `J_SPADE` 等；部分卡牌缺名称、词条、价值或仍标记“存疑”。这些条目可以被结构化保留，但不得在信息不完整时自动启用。

## Locked acceptance

### 🤖 Automated

- [ ] 角色主表将 ID、名称/文本键、基础 HP、物攻、元攻、初始武器和被动行为拆成独立机器字段，不再解析“生命值：15”这类描述文本。
- [ ] 卡牌主表与效果子表分离；一张卡可以有多个顺序明确的效果，不用逗号列表、换行描述或嵌套 JSON 表示运行时规则。
- [ ] 纯数值效果通过 Plan 18 的通用 `EffectKind/Operation/Target/Value` 路径生效；至少覆盖 Add、Multiply、Override 和立即治疗/恢复等当前已实现需求。
- [ ] 特殊逻辑通过稳定 `BehaviorId + typed parameters` 注册；角色/卡牌 ID 不再出现在持续膨胀的 `ApplyTraitCard`/晋升 switch-if 链中。
- [ ] 需要持续监听的被动使用事件/Ability/必要组件；仅需要一次修改的效果不创建专属 Component。
- [ ] 所有工作簿角色和卡牌行在 CSV 中有可追踪状态；缺 ID、缺名称、存疑或尚无 handler 的行必须 `enabled=false`/等价状态并给出可校验原因，绝不进入运行时抽取池。
- [ ] 当前可玩角色和当前可抽卡牌保持行为等价；CSV 中修改代表性角色基础属性或基础卡数值后，重启 Run 即反映到 GAS/BuildSnapshot，无需改 C++。
- [ ] 抽卡仍按现有 Run 状态确定性生成、同批无重复、优先较少持有、只接受当前 pending choice；禁用/非法卡不参与随机种子候选集合。
- [ ] 角色晋升 Hunter/Poet/Brave/Sage、Poet 关后成长、Brave Forge/Sage 额外选择等已实现能力保持；具体 ID 映射由人批准并记录。
- [ ] 旧 `characters.json`、`cards.json` 及对应 C++/DeveloperSettings 重复值退出该领域运行时权威；不存在双写。
- [ ] 新增 schema/外键/未知行为负例测试、角色加载测试、卡牌效果测试和确定性回归；Editor build、全量 `ReEcho.*`、静态校验、`git diff --check` 通过。

### 🎮 Human PIE

- [ ] 人依次进入四种角色/晋升结果，确认基础属性、默认武器、角色能力、头像/外观映射和生命校正正确。
- [ ] 人完成多轮卡牌选择，确认卡名/说明、抽取节奏、实际数值和回响统计一致。
- [ ] 人修改一项允许的角色数值与一张基础卡数值、重启 Run，确认无需重新编译即可生效。

## Step 0 gates

1. 必须基于已合并的 Plan 18 main 创建 `plan/19-character-build-csv`；先读 Plan 18 最终 Execution notes、最终公共头文件和新增数据测试。
2. 规划者必须先完成“按 Plan 18 实际接口修订本 Plan”的动作；若本 Plan 仍引用不存在的接口/路径，执行者停止并交回规划者。
3. 输出角色与卡牌 ID 对照：工作簿 ID、当前 JSON ID、当前 C++/录制 ID、目标 canonical ID、alias/migration 处理。任何可能破坏历史 BuildSnapshot/录制引用的重编号先请人拍板。
4. 把工作簿所有卡牌分成：`Numeric`、`ParameterizedBehavior`、`UniqueBehavior`、`Incomplete/Disabled`。不要以“描述看起来简单”为由临时写 CardId 特判。
5. 采集当前六张可抽卡、四角色晋升、Poet/Brave/Sage 行为和相关自动化基线；基线失败则先报告，不在迁移中顺手改玩法。
6. 本任务不修改 `.uasset`/`.umap`。角色贴图保持现有硬引用/映射，CSV 只引用稳定资源 ID/软路径契约；如需资产改动必须另行认领并经人确认。

## Target table responsibilities

具体文件名和列名服从 Plan 18 最终 schema，但语义至少包括：

- Character：ID、display text key、基础属性、默认武器 ID、appearance ID、passive behavior ID、enabled。
- Card：ID、tier、display text key、description text key、tags、offerable/enabled、stack/conflict policy、review status。
- CardEffect：owner card ID、order、trigger、effect kind、target/op/value 或 behavior ID、类型化参数、duration/cooldown/stack rule。
- ID alias/migration：只在 Plan 18 最终契约需要且经人批准时存在；不能靠名称猜映射。

## Implementation outline

1. 先按 Plan 18 接口导入/验证角色、卡牌与效果子表，保留所有源行的状态。
2. 将当前角色初始值、卡牌目录/描述与 `ApplyTraitCard` 通用数值分支迁到注册表。
3. 将已有角色被动和特殊卡牌接入有限行为注册；复用 GAS/Run 事件，不为每行造 Component。
4. 让 Run 初始化、抽卡、应用卡牌、角色晋升和 UI 只读同一注册快照。
5. 移除/隔离该领域旧重复常量，补自动化和人验诊断。
6. 更新数据 README、CODEBASE_MAP、PROJECT_STATE、相关经验与本 Plan Execution notes。

## Expected ownership

- Plan 18 最终角色/卡牌 CSV 与 schema 文件。
- `Run/ReEchoRunSubsystem.*`、`Run/ReEchoCharacterPromotion.*`、必要的角色/卡牌数据 adapter/behavior 文件。
- 角色/卡牌 UI 只做数据读取接线，不重做版式。
- 相关 GAS 初始化、Player/Echo appearance 接口仅做必要适配。
- 角色/构筑自动化测试和静态验证规则。

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| CSV static | `python scripts/validate_project.py` | 所有启用行完整，ID/外键/行为有效；禁用源行有明确原因 |
| Build | format + `scripts/ue/Build-Editor.cmd` | UE 5.8 Editor build 成功 |
| Automation | 角色、抽卡、应用效果、晋升、GAS 初始化 + 全量 `ReEcho.*` | CSV 变值可见，确定性和既有角色机制无回归 |
| Diff | 初版 Plan ↔ 终版 Plan；`git diff --check` | Plan 18 适配偏差与未实现源行已记录 |
| Human | 四角色与多轮抽卡 PIE | 属性、能力、说明、回响统计和节奏通过人验 |

## Risks and exclusions

- 本任务结构化全部设计行，但不承诺实现工作簿中每个尚未评审/缺 ID 的独特能力；启用必须有 handler 和测试。
- 不在 CSV 中执行伤害公式字符串，不用自由文本作为运行时规则。
- 不改变回响记录内容：仍只记录历史位置、成功主动技能和既有武器变化/BuildSnapshot 信息。
- 若 Plan 18 的通用 effect 过弱，应在本分支提出最小公共扩展并先与 Plan 20/21 协调，不复制第二套 effect 系统。

## Recommended executor model

强模型。任务涉及 Run、GAS 初始化、确定性抽卡、角色晋升与多种被动事件；需要基于 Plan 18 最终实现调整。

## Execution notes

### Changed

### Evidence

### Plan 18 contract adaptation

### Source rows left disabled and why

### Remaining risks

### Human validation requested

## 执行者启动 prompt

```text
你是 ReEcho 项目的执行者。先读 AGENTS.md，再按其最小读取顺序读 plans/19-data-character-build-csv.md、已完成的 plans/18-data-csv-runtime-foundation.md 最终 Execution notes，以及 shared/LESSONS.md §GAME 中与玩法状态/确定性相关的匹配条目；只有遇到失败诊断才读 §DEBUG。

前置：规划者必须已根据 Plan 18 终版修订 Plan 19，main 已包含 Plan 18。否则停止并告诉人。然后在独立 worktree 的 `plan/19-character-build-csv` 分支工作；参考工作簿为仓库上一级 `../回响肉鸽数值与构筑体系.xlsx`。

任务：把角色和构筑体系迁到 Plan 18 的唯一 CSV 注册表。验收照 Plan 19 的 🔒 清单；实现层可调整，但不要复制加载器、不要按 CardId 扩张巨型 switch、不要把缺 ID/缺 handler 的设计行启用，也不要修改 `.uasset`/`.umap` 或碰用户已有资产改动。所有 Plan 18 接口适配、禁用行和需求偏差写入本 Plan Execution notes。

完成后显式提交/push到本分支并告诉人，禁止修改或合并 main。
```
