# Plan 21 - data - weapons and slots CSV migration

## Locked goal

将武器体系、攻击阶段、插槽类型、配件和配件效果迁移到 CSV 注册表，使策划可以组合武器基础数值、攻击模组、核心元素和数值/行为型配件；玩家与回响按相同不可变定义执行武器，同时保持 GAS 冷却/伤害、确定性回放和当前可玩武器行为。

## Dependency status

本 Plan 必须在 Plan 18 合并后由规划者按最终 CSV 契约修订。运行时集成还依赖 Plan 20 最终 ElementId/Reaction API；在 Plan 20 未完成时，执行者可以做只读内容归一化设计，但不得复制元素枚举/反应系统或自行发明临时接口。

## Source baseline

- `武器体系W`：匕首、长剑、镰刀、鞭、弓、枪、法杖的攻击阶段和初始属性。
- `武器插槽C`：通用核心、各武器专属槽位和近 80 行配件效果。
- 当前表有大量空配件名称与空词条，且许多效果是事件逻辑而非数值；这些行必须保留设计来源但不能自动 enabled。
- 当前运行时只有 `PhysicalOrb/Sword/ElementalOrb` 三槽、DeveloperSettings 武器数组和 Actor switch，与目标七类武器/配件组合尚不等价。

## Locked acceptance

### 🤖 Automated

- [ ] 武器主表分离 ID、类型、展示/资源键、基础属性、AttackPatternId、允许槽位集合与 enabled；不再从“攻击时长：0.8s”文本解析数值。
- [ ] 多段攻击使用有序攻击阶段子表，能表达时长、物攻/元攻系数、范围/角度覆盖、移动/击退/无敌窗口和阶段条件；次序稳定且可校验。
- [ ] 插槽类型/武器允许槽位与具体配件分离；核心决定物理/元素伤害通道时引用 Plan 20 的稳定 ElementId。
- [ ] 配件效果使用一对多 effect rows：纯数值走通用 modifier，通用触发逻辑走 BehaviorId/typed parameters，独特攻击模组走已注册 AttackPattern/Behavior handler。
- [ ] 不按每个 PartId 新建 switch 或 Component。仅需要持续状态/Tick/空间实体的配件可挂组件；一次命中、暴击、击杀、叠层和掉落等优先走事件/Ability/handler。
- [ ] 所有工作簿武器和插槽行有源行追踪与 implementation status；缺名称、缺 ID、缺参数或没有 handler 的配件必须 disabled 并给出原因。
- [ ] 当前三个可玩武器/槽位行为保持，或按人批准的映射迁入七类架构；GAS cooldown/伤害仍权威，Weapon Actor 不恢复第二套计时/伤害权威。
- [ ] 玩家切换武器、BuildSnapshot、Recorder 武器变化和 Echo Playback 仍只记录/引用稳定 WeaponId；同一 Run 内使用同一注册快照，表变化不会让历史回响漂移。
- [ ] 至少有代表性自动化覆盖：单段远程、多段近战、核心元素、Add/Multiply/Override 配件、暴击/命中触发配件、禁用未实现配件、非法槽位组合和表数值变更。
- [ ] 旧 `weapons.json`、DeveloperSettings `Weapons` 及武器 Actor 重复常量退出该领域运行时权威；不存在双写。
- [ ] Editor build、武器/录制/回响/元素回归、全量 `ReEcho.*`、静态校验和 `git diff --check` 通过。

### 🎮 Human PIE

- [ ] 人至少体验一件多段近战、一件单发远程、一件爆炸/元素武器，确认范围、节奏、碰撞、动画/反馈与表一致。
- [ ] 人安装代表性的核心、纯数值配件和行为配件，确认槽位限制、叠加规则、说明与实际效果一致。
- [ ] 人切换武器并进入下一局，确认玩家与回响武器、攻击节奏和元素行为一致。
- [ ] 人修改一项武器基础数值和一项配件数值、重启 Run 后确认无需重新编译即可生效。

## Step 0 gates

1. 基于已合并 Plan 18 创建 `plan/21-weapons-slots-csv`；规划者须先按 Plan 18 终版修订 schema/API 引用。
2. 运行时接线前必须读取已完成的 Plan 20 Execution notes 和公共 ElementId/Reaction API；Plan 20 未合并则停止接线，不复制临时元素系统。
3. 列出当前三个武器 ID/槽位与工作簿七种武器/角色初始武器的映射，评估 BuildSnapshot/录制兼容；任何重编号或语义替换先由人拍板。
4. 将所有配件行分类为 `Numeric`、`ParameterizedBehavior`、`AttackPatternReplacement`、`UniqueBehavior`、`Incomplete/Disabled`，并补稳定 ID 提案。缺名称/词条的源行不能用行号直接成为长期运行时 ID，需人确认命名/ID 或保持禁用。
5. 采集现有三个武器攻击间隔、伤害、射程、元素循环、Sword/Projectile 行为与录制/回响测试基线。
6. 先用一个武器、一个多段 pattern、一个核心和一个数值配件打通全链；若通用模型不能表达，记录最小 schema 扩展，不立刻为全部 80 行写专属类。
7. 不修改武器/角色 `.uasset`、贴图或动画资产；若视觉资源缺失，用现有可玩表现验证逻辑，资产补齐另开 Plan 并认领。

## Target table responsibilities

具体列名服从 Plan 18/20 最终契约，语义至少包括：

- Weapon：ID、type、display/resource keys、attack pattern ID、基础 interval/range/projectile/explosion/concentration 等、allowed slot profile、enabled。
- AttackStep：weapon/pattern ID、ordered index、duration、damage coefficients、geometry/range/angle、movement/status/condition behavior references。
- SlotType/Profile：武器类型允许的槽位和数量限制，核心例外规则。
- Part：ID、weapon type/slot type、display text、rarity/tags、enabled/implementation status。
- PartEffect：part ID、order、trigger、effect kind、target/op/value 或 behavior/attack pattern ID、typed parameters、duration/cooldown/stack rule。

## Implementation outline

1. 在 Plan 18 注册表上建立武器/阶段/槽位/配件关系模型和完整静态校验。
2. 用一个垂直切片打通 CSV → build/equipment → Weapon execution → GAS damage/cooldown → recording/echo。
3. 将当前三武器迁移并保持自动化基线，再扩到工作簿七类基础 pattern。
4. 实现有限的通用 modifier、事件行为和 AttackPattern handler；未实现源行保持 disabled。
5. 接 UI/商店/背包所需只读名称与槽位合法性，不重做界面版式和经济系统。
6. 移除旧武器运行时重复源，补组合、确定性和回响回归测试。
7. 更新 README、CODEBASE_MAP、PROJECT_STATE、LESSONS 与 Execution notes，列出禁用配件和后续实现批次。

## Expected ownership

- Plan 18 最终武器/阶段/槽位/配件 CSV 与 schema。
- `Weapons/ReEchoWeaponActor.*` 及新增的 weapon definition/pattern/effect handlers。
- 必要的 Player/Echo/Recording/Projectile/Run 装备接线。
- `Core/ReEchoBalanceSettings.*` 中旧武器定义的退役或兼容迁移。
- 武器/配件/录制回响自动化和静态验证。
- 与 Plan 20 公共元素文件重叠时先协调，默认只消费其接口。

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| CSV static | `python scripts/validate_project.py` | 武器/阶段/槽位/配件 ID、顺序、外键、行为和组合合法；禁用原因完整 |
| Build | format + `scripts/ue/Build-Editor.cmd` | UE 5.8 Editor build 成功 |
| Automation | pattern、modifier、slot legality、GAS cooldown/damage、recording/echo + 全量 `ReEcho.*` | CSV 驱动且确定，无双权威/旧测试回归 |
| Diff | 初版↔终版、`git diff --check` | Plan 18/20 适配和未实现配件批次记录完整 |
| Human | 多段近战/远程/元素/配件/下一局回响 PIE | 节奏、范围、反馈、槽位和回响一致性通过 |

## Risks and exclusions

- 近 80 个配件包含大量独特逻辑；本 Plan 的完成标准是结构化、可验证和完成批准的可玩批次，不是把缺字段的设计描述全部硬编码上线。
- 攻击 pattern、配件 effect、元素 reaction 是三个不同概念；不把它们塞进一个万能字符串解释器。
- 多段攻击/叠层/随机行为必须使用稳定顺序和项目随机流；不要用帧率或容器遍历顺序决定结果。
- UI 重做、武器美术、音效、经济掉落和平衡调参不在本 Plan；只做必要的数据展示/装备接线。

## Recommended executor model

最强模型。该任务数据量最大，并横跨武器 pattern、GAS、元素、装备组合、录制/回响与大量未完成设计行。

## Execution notes

### Changed

### Evidence

### Plan 18 and Plan 20 contract adaptation

### Enabled content batch

### Source rows left disabled and why

### Remaining risks

### Human validation requested

## 执行者启动 prompt

```text
你是 ReEcho 项目的执行者。先读 AGENTS.md，再按其最小读取顺序读 plans/21-data-weapons-slots-csv.md、已完成的 Plan 18 与 Plan 20 最终 Execution notes、Plan 14 相关经验，以及 shared/LESSONS.md §GAME 中武器/回响/确定性匹配条目；只有诊断失败时读 §DEBUG。

前置：main 已包含 Plan 18，运行时接线阶段还必须包含 Plan 20；规划者已按两者终版接口修订 Plan 21。否则停止并告诉人。然后在独立 worktree 的 `plan/21-weapons-slots-csv` 分支工作；参考工作簿为仓库上一级 `../回响肉鸽数值与构筑体系.xlsx`。

任务：把武器、攻击阶段、插槽和配件迁到唯一 CSV 注册表。验收照 Plan 21 的 🔒 清单；实现层可调整，但先做垂直切片，禁止复制数据/元素系统、禁止按 PartId 扩张巨型 switch、禁止把缺名称/ID/handler 的行启用，禁止修改 `.uasset`/`.umap`。把 Plan 18/20 适配、启用批次、禁用源行和所有偏差写入 Execution notes。

完成后显式提交/push到本分支并告诉人，禁止修改或合并 main。
```
