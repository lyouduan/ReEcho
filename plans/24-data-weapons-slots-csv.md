# Plan 24 - data - weapons and slots CSV migration

## Locked goal

将武器体系、攻击阶段、插槽类型、配件和配件效果迁移到 CSV 注册表，使策划可以组合武器基础数值、攻击模组、核心元素和数值/行为型配件；玩家与回响按相同不可变定义执行武器，同时保持 GAS 冷却/伤害、确定性回放和当前可玩武器行为。

## Dependency status

Plan 21、22 已于 2026-08-10 人验、审查并本地合并。Plan 22 已落地通用 `ReEchoCsvDataReader.*`、独立角色构筑领域读取器和原子应用/硬失败契约；本 Plan 仍必须等待 Plan 23 人验并合并后启动，以消费最终 ElementId/Reaction API。

启动前规划者还要依据 Plan 22 的领域读取扩展点与 Plan 23 的最终 `ElementId`/Reaction API 做一次窄校准。Plan 24 执行期间独占上述公共数据文件，默认只消费 Plan 23 元素接口，不再改写它。

## Source baseline

- `武器体系W`：匕首、长剑、镰刀、鞭、弓、枪、法杖的攻击阶段和初始属性。
- `武器插槽C`：通用核心、各武器专属槽位和近 80 行配件效果。
- 当前表有大量空配件名称与空词条，且许多效果是事件逻辑而非数值；这些行必须保留设计来源但不能自动 enabled。
- 当前运行时只有 `PhysicalOrb/Sword/ElementalOrb` 三槽、DeveloperSettings 武器数组和 Actor switch，与目标七类武器/配件组合尚不等价。

## Locked acceptance

### 🤖 Automated

- [ ] 武器主表分离 ID、类型、展示/资源键、基础属性、AttackPatternId、允许槽位集合与 enabled；不再从“攻击时长：0.8s”文本解析数值。
- [ ] 多段攻击使用有序攻击阶段子表，能表达时长、物攻/元攻系数、范围/角度覆盖、移动/击退/无敌窗口和阶段条件；次序稳定且可校验。
- [ ] 插槽类型/武器允许槽位与具体配件分离；核心决定物理/元素伤害通道时引用 Plan 23 的稳定 ElementId。
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

1. 仅在 Plan 23 已人验并合并后，从更新后的 main 创建 `plan/24-weapons-slots-csv`；读取 Plan 21–23 最终 Execution notes、`ReEchoCsvDataReader.*`、两个既有领域读取器、共享快照、内建行为注册和最终 ElementId/Reaction API。缺 Plan 23 则停止。
2. 在协调板独占生产 manifest/schema、共享快照、注册表编排、内建注册、`ReEcho.Build.cs` 与静态校验入口；默认只消费 Plan 23 元素 API，如需改其公共契约先交回规划者。
3. 武器/配件的 `BehaviorId`、`EffectKind` 和 AttackPattern handler 必须进入显式内建注册函数，并在模块启动加载前注册；不得依赖静态初始化顺序。
4. 列出当前三个武器 ID/槽位与工作簿七种武器/角色初始武器的映射，评估 BuildSnapshot/录制兼容；任何重编号或语义替换先由人拍板。
5. 将所有配件行分类为 `Numeric`、`ParameterizedBehavior`、`AttackPatternReplacement`、`UniqueBehavior`、`Incomplete/Disabled`，并补稳定 ID 提案。缺名称/词条的源行不能用行号直接成为长期运行时 ID，需人确认命名/ID 或保持禁用。
6. 采集现有三个武器攻击间隔、伤害、射程、元素循环、Sword/Projectile 行为与录制/回响测试基线。
7. 先用一个武器、一个多段 pattern、一个核心和一个数值配件打通全链；若通用模型不能表达，记录最小 schema 扩展，不立刻为全部 80 行写专属类。
8. 不修改武器/角色 `.uasset`、贴图或动画资产；若视觉资源缺失，用现有可玩表现验证逻辑，资产补齐另开 Plan 并认领。
9. 沿用 Plan 23 建立的“生产基线＋局部覆盖”负例组包方式，禁止把六张武器领域表复制进所有旧夹具目录。

## Target table responsibilities

生产表文件固定为 `weapons.csv`、`attack_steps.csv`、`slot_types.csv`、`slot_profiles.csv`、`parts.csv`、`part_effects.csv`；具体列名服从 Plan 22/23 已落地的领域读取约定，语义至少包括：

- Weapon：ID、type、display/resource keys、attack pattern ID、基础 interval/range/projectile/explosion/concentration 等、allowed slot profile、enabled。
- AttackStep：weapon/pattern ID、ordered index、duration、damage coefficients、geometry/range/angle、movement/status/condition behavior references。
- SlotType/Profile：武器类型允许的槽位和数量限制，核心例外规则。
- Part：ID、weapon type/slot type、display text、rarity/tags、enabled/implementation status。
- PartEffect：part ID、order、trigger、effect kind、target/op/value 或 behavior/attack pattern ID、typed parameters、duration/cooldown/stack rule。
- 运行时由类型化武器领域读取器校验阶段顺序、槽位组合、外键、范围和 handler 白名单；`csv_schema.csv` 只描述同一契约，不解释攻击公式或行为。
- 武器表加载后，注册表必须做跨领域校验：Plan 22 所有启用角色的 `DefaultWeaponId` 必须引用启用武器；未知武器不能等到 `StartRun()` 或攻击时才失败。

## Implementation outline

1. 沿用 Plan 22 的通用读取层和 Plan 23 的领域模式新增独立武器领域读取器，扩展共享快照与注册表编排；生产 manifest 缺任一必需表、角色默认武器外键无效或任一领域失败时整包不发布。
2. 用一个垂直切片打通 CSV → build/equipment → Weapon execution → GAS damage/cooldown → recording/echo。
3. 将当前三武器迁移并保持自动化基线，再扩到工作簿七类基础 pattern。
4. 实现有限的通用 modifier、事件行为和 AttackPattern handler；未实现源行保持 disabled。
5. 接 UI/商店/背包所需只读名称与槽位合法性，不重做界面版式和经济系统。
6. 移除旧武器运行时重复源，补组合、原子失败、确定性和回响回归测试。
7. 同步 README、manifest/schema/RuntimeDependencies、静态领域校验、CODEBASE_MAP、PROJECT_STATE、LESSONS 与 Execution notes，列出禁用配件和后续实现批次。

## Expected ownership

- 六张生产武器领域 CSV 及 manifest/schema/README/打包依赖。
- 独立武器领域读取器、共享快照扩展、注册表编排与显式内建行为/AttackPattern 注册。
- `Weapons/ReEchoWeaponActor.*` 及新增的 weapon definition/pattern/effect handlers。
- 必要的 Player/Echo/Recording/Projectile/Run 装备接线。
- `Core/ReEchoBalanceSettings.*` 中旧武器定义的退役或兼容迁移。
- 武器/配件/录制回响自动化和静态验证。
- 与 Plan 23 公共元素文件重叠时先协调，默认只消费其接口。

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| CSV static | `python scripts/validate_project.py` | 武器/阶段/槽位/配件 ID、顺序、外键、行为和组合合法；禁用原因完整 |
| Build | format + `scripts/ue/Build-Editor.cmd` | UE 5.8 Editor build 成功 |
| Automation | pattern、modifier、slot legality、GAS cooldown/damage、recording/echo + 全量 `ReEcho.*` | CSV 驱动且确定，无双权威/旧测试回归 |
| Diff | 初版↔终版、`git diff --check` | Plan 21/23 适配和未实现配件批次记录完整 |
| Human | 多段近战/远程/元素/配件/下一局回响 PIE | 节奏、范围、反馈、槽位和回响一致性通过 |

## Risks and exclusions

- 近 80 个配件包含大量独特逻辑；本 Plan 的完成标准是结构化、可验证和完成批准的可玩批次，不是把缺字段的设计描述全部硬编码上线。
- 攻击 pattern、配件 effect、元素 reaction 是三个不同概念；不把它们塞进一个万能字符串解释器。
- 多段攻击/叠层/随机行为必须使用稳定顺序和项目随机流；不要用帧率或容器遍历顺序决定结果。
- UI 重做、武器美术、音效、经济掉落和平衡调参不在本 Plan；只做必要的数据展示/装备接线。
- 不为并行开发复制 Plan 23 的元素类型、公共快照或 manifest/schema；前置未合并就停止。

## Recommended executor model

最强模型。该任务数据量最大，并横跨武器 pattern、GAS、元素、装备组合、录制/回响与大量未完成设计行。

## Execution notes

### Changed

### Evidence

### Plan 21 and Plan 23 contract adaptation

### Enabled content batch

### Source rows left disabled and why

### Remaining risks

### Human validation requested

## 执行者启动 prompt

```text
你是 ReEcho 项目的执行者。先读 AGENTS.md，再按其最小读取顺序读 plans/24-data-weapons-slots-csv.md、已完成的 Plan 21 与 Plan 23 最终 Execution notes、Plan 14 相关经验，以及 shared/LESSONS.md §GAME 中武器/回响/确定性匹配条目；只有诊断失败时读 §DEBUG。

前置：main 已依次包含经人验的 Plan 21、22、23，且规划者已按 Plan 22 的领域读取接口与 Plan 23 的最终元素契约窄校准本 Plan。否则停止并告诉人。然后在独立 worktree 的 `plan/24-weapons-slots-csv` 分支工作；参考工作簿为仓库上一级 `../回响肉鸽数值与构筑体系.xlsx`。

任务：沿用 Plan 22 的通用读取层和 Plan 23 的领域读取/负例组包模式新增独立武器领域读取器，扩展唯一快照/注册表编排，并使用 Plan 23 的 ElementId/Reaction API。迁移武器、攻击阶段、插槽和配件，同时校验所有启用角色的 DefaultWeaponId 外键。所有 handler 必须显式注册且先于启动加载。验收照 Plan 24 的 🔒 清单；先做垂直切片，禁止复制数据/元素系统、禁止按 PartId 扩张巨型 switch、禁止把缺名称/ID/handler 的行启用，禁止修改 `.uasset`/`.umap`。把 Plan 21–23 适配、启用批次、禁用源行和所有偏差写入 Execution notes。

完成后显式提交到本地分支并告诉人；未经人明确确认不得 push，禁止修改或合并 main。
```
