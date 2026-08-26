# Plan 119 - 程序 - 伤害跳字显示理论伤害

## 协调

- Planner 负责人：Codex（Gavyn-side AI）。
- Executor 负责人：Codex（Gavyn-side AI）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@568381e6389979fadd66f557e79d7737ff0e3bc5`。
- 本地实现方式：`feat/expected-damage-number`；`C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan119-expected-damage-number`。
- 依赖 / 阻塞：复用现有 `FReEchoDamageEvent::RawDamage/AppliedDamage`，不依赖未发布的 Plan118 实现。
- Writes:
  - `plans/119-expected-damage-number.md`
  - `Source/ReEcho/{Public,Private}/Combat/ReEchoElementReaction.*`
  - `Source/ReEcho/Private/Presentation/Enemy/ReEchoEnemyPresentationComponent.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoCombatHudTests.cpp`
  - `Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - 精选 Win64 Editor 预构建包及 manifest（最终发布门禁刷新）。
- Stable Reads:
  - `Source/ReEchoCombat/{Public,Private}/Combat/ReEchoCombat{Contracts,Types,antComponent}.*`
  - `Source/ReEchoCombat/Private/Combat/ReEchoHitResolver.cpp`
  - `Source/ReEchoCombat/Private/Combat/ReEchoElementHitResolver.cpp`
  - `Source/ReEcho/{Public,Private}/UI/ReEchoDamageNumberActor.*`
- 影响模式：`Isolated`。仅改变敌人受击跳字读取字段，不改变伤害结算、生命扣除、死亡、事件 Schema、音频或 VFX 强度。
- 兼容承诺 / 下游操作：`AppliedDamage` 继续表示实际生命扣除；`RawDamage` 继续表示目标规则修正后、生命上限钳制前的本次伤害。跳字显示值改为 `RawDamage`，颜色、取整、动画和生成条件不变。
- 明确排除：不修改玩家受击反馈、怪物/玩家生命、过量伤害结算、伤害公式、元素反应、Boss 对玩家的跳字或保存格式。

## 锁定目标

敌人受击时，世界空间伤害跳字显示本次原本应造成的最终伤害，而不是被目标剩余生命钳制后的实际扣血量。例如怪物剩余 `7` 点生命、本次经过攻击方与目标方规则后的伤害为 `20`，生命仍只扣 `7` 并死亡，但跳字显示 `20`。

实现必须使用现有伤害事件中的语义字段，不从当前/历史生命反推理论伤害，也不改变 `AppliedDamage` 的公共含义。普通伤害、元素伤害、反应伤害与持续伤害只要走现有敌人 `OnHurt` 表现入口，都使用同一显示选择函数。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI`、`AREA-UI`、`AREA-Tests`。
- 设计意图：Combat 继续同时提供结算前最终伤害 `RawDamage` 与实际扣血 `AppliedDamage`；Enemy Presentation 明确选择前者用于跳字，其他消费者继续按各自语义选择字段。
- 不新增 `DisplayDamage` 字段，避免复制数据和扩大 Combat Schema；用一个可测试的纯函数集中跳字字段选择，防止后续回退到 `AppliedDamage`。
- `ARCHITECTURE.md`、CODEBASE_MAP `README.md` 与 Runtime Module 拓扑预计不变；更新 UI 模块与 UI 指导中的跳字数值契约。

## 锁定验收

- [ ] `RawDamage=20`、`AppliedDamage=7` 的有效敌人受击事件生成跳字 `20`，生命结算仍只扣 `7`。
- [ ] 非过量伤害 `RawDamage=AppliedDamage` 时显示不变；跳字仍仅在 `AppliedDamage>0` 的有效受击事件生成。
- [ ] 元素/反应/Burn 等通过统一敌人 Hurt 入口的伤害使用同一数值语义；颜色、字体、缩放动画与生命周期不回归。
- [ ] 不修改 `FReEchoDamageEvent` Schema、Combat Resolver、`AppliedDamage` 消费者、生命或死亡规则。
- [ ] UE 5.8 Editor 构建、`ReEcho.UI.CombatHud`、静态校验及最终 `-FullRebuild` 发布门禁通过。
- [ ] 用户在 `Level00` 以低血怪物验证过量伤害跳字，并确认通过。

## Step 0 门禁

- 基线：当前任务工作树准确基于 `origin/main@568381e6`；Plan118 只有 Plan 在主线，本任务不读取其本地实现。
- 现状证据：`UReEchoEnemyPresentationComponent::HandleCombatHurt` 在 `AppliedDamage>0` 时把 `Event.AppliedDamage` 传给 `SpawnDamageNumber`；Resolver 同一事件已同时写入目标修正后的 `RawDamage` 和实际扣血 `AppliedDamage`。
- 共享契约风险：禁止改名、重解释或新增伤害事件字段；只改变 Enemy Presentation 的显示选择。
- 停止条件：若发现某条敌人伤害路径的 `RawDamage` 不是生命钳制前最终伤害，或需要修改 Combat 结算才能获得正确值，则停止扩张并重新评审。

## 实现提纲

1. 增加纯表现辅助函数，从有效伤害事件选择 `RawDamage` 作为跳字值。
2. Enemy Hurt 表现使用该函数；保留 `AppliedDamage>0` 生成门禁和现有颜色/动画。
3. 增加普通与过量伤害自动化，更新 UI 文档，完成构建、聚焦测试、静态校验和人工验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 单元语义 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.CombatHud` | 20 理论/7 实扣显示 20；普通伤害保持一致 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码 0 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目与格式检查通过 |
| 最终发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终组合候选与精选预构建包匹配 |
| 人工 | `Level00` 低血怪物承受高额伤害 | 跳字为理论伤害，生命/死亡表现正常 |

## 执行记录

### 变化

- 待实现。

### 证据

- 现有 `FReEchoDamageEvent` 已提供所需的 `RawDamage` 与 `AppliedDamage`，无需修改 Combat 契约。

### 剩余风险

- 需要在真实低血击杀场景确认玩家看到的取整结果符合预期。

### 人工验收结果/请求

- `PendingBeforeClose`。

### 架构文档审阅结果

- 待实现后填写。
