# Plan 65 - 程序 - 怪物攻击蓄力与二阶段变身

## 协调

- Planner 负责人：JosephLE910（程序）
- Executor 负责人：JosephLE910（程序）
- Plan / 实现编写方（AI 侧）：`JosephLE910-side AI | ReEcho teammate-side AI`。
- 任务状态：`InProgress`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main @ 4250740`（ReEcho 主工作树，直接在本地主线开发）。
- 依赖 / 阻塞：通用系统可立即实现；正式启用的怪物名单、触发距离、攻击次数、变身时长、变身期间无敌、生命处理和二阶段倍率仍需产品配置确认，未确认前仅使用测试夹具和临时动画资产。
- Writes：
  - `Design/Data/ReEchoData.xlsx`、数据 schema / 生成器 / 编译器及测试（新增可选敌人阶段配置；CSV 只由同步脚本生成）
  - `Source/ReEchoEnemies/Public|Private/**`（阶段状态、二选一触发、事件、快照及自动化）
  - `Source/ReEchoCombat/Public/**`（仅当现有伤害事件不足以区分/去重玩家与回响攻击时扩展公共契约）
  - `Source/ReEchoPresentation/Public|Private/**`（Charge / Transform 标签、阶段动画集选择及自动化）
  - `Source/ReEcho/Public|Private/**`（EnemyActor 感知输入、伤害转发、表现接线、存档恢复及测试）
  - 受影响的敌人 Animation Profile / DataAsset 与迁移脚本（`.uasset` 为 Exclusive）
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`modules/MOD-ReEchoEnemies.md`、`MOD-ReEchoCombat.md`、`MOD-ReEchoPresentation.md`
- Stable Reads：
  - `Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`（攻击身份与伤害结果）
  - 当前敌人仇恨目标选择、Sense 输入、EnemyLogic snapshot 与 GameMode 存档聚合
  - 当前 Special/Boss windup 事件与 `UReEcho2DPresentationController`
  - 当前敌人 Animation Profile、AnimationSets、FootRoot / 阴影 / `Profile.WorldHeight` 归一化流程
- 影响模式：`SharedContract + Exclusive`。敌人状态、存档、表现事件及数据 schema 是共享契约；DataAsset 是不可文本合并资源。
- 明确排除：最终美术/VFX/音频制作、重写仇恨选择、修改伤害数值公式、由 Flipbook 播放结束决定玩法时序、未经确认批量启用生产怪物、清理当前工作区无关美术改动。

## 锁定目标

1. 攻击表现增加 `Attack.Charge -> Attack.Basic`：玩法逻辑仍决定蓄力结束和攻击提交，动画不得成为命中或状态切换权威。
2. 支持可选且只触发一次的二阶段变身，触发条件为 OR：
   - 当前有效仇恨目标进入配置距离；或
   - 怪物承受的有效攻击次数达到配置阈值。
3. 玩家和回响造成的有效攻击都计入次数；同一 `AttackIdentity` 的多段/多命中只计一次。
4. 不吸引仇恨的回响不能通过距离触发；只有当回响实际成为当前有效仇恨目标时，才允许距离条件触发。
5. 变身期间暂停移动和新攻击，并取消尚未提交的蓄力/特殊行动；完成后切换二阶段 AnimationSet 和配置能力/倍率。
6. 所有阶段动画继续使用 DataAsset 的逐动画朝向设置，并统一走 `Profile.WorldHeight`、FootRoot 与阴影归一化；不在 CharacterPrefab 蓝图保存“默认朝右”。

## 架构影响与设计决策

- `MOD-ReEchoEnemies / AREA-Enemies` 是阶段规则唯一权威：保存当前阶段、去重后的受击次数、触发原因、变身剩余时间及一次性标志；每帧只消费宿主提供的当前目标距离与仇恨资格，不扫描世界。
- `MOD-ReEchoCombat / AREA-AbilityCombat` 仍是有效命中权威。仅 `AppliedDamage > 0` 且来源为 Player/Echo 的伤害可计数；优先复用现有 `FReEchoAttackIdentity`，不足时才做最小向后兼容扩展。
- `MOD-ReEchoPresentation / AREA-Presentation` 只消费 `ChargeStarted / AttackCommitted / TransformStarted / TransformCompleted` 事件。新增标签 `Animation.Attack.Charge`、`Animation.Transform.Phase2`；Charge 可循环到玩法提交，Basic/Transform 为一次性视觉。
- 阶段配置由 XLSX 唯一真源生成，建议字段：`EnemyId, PhaseIndex, TriggerRangeCm, RequiredAttackCount, AttackSourcePolicy, RangeTargetPolicy, TransformSeconds, AnimationSetId, AbilitySetId, HealthPolicy, InvulnerableDuringTransform, StatMultipliers, Enabled`。非正阈值关闭对应条件；两个条件均关闭时校验失败或明确保持阶段禁用。
- 同帧两个条件同时满足时采用稳定优先级 `AttackCountReached` 后 `RangeEntered`，只发一次变身事件。
- 表现资产字段逐步收敛为通用 `AnimationSetId`；若现有 `WeaponVisualSetId` 被资源引用，则保留兼容字段/迁移，不直接破坏已保存 DataAsset。
- 存档必须保存阶段、计数、触发状态与变身剩余时间；恢复时逻辑不重复触发，表现根据阶段/过渡状态恢复正确动画集。
- 临时资产允许回退到现有攻击/待机片段，缺失资产不得阻塞玩法或崩溃；关闭前列出仍需美术补齐的资源。

## 锁定验收

- [ ] 当前仇恨玩家进入距离可触发；非仇恨回响进入距离不触发；成为当前仇恨目标的回响进入距离可触发。
- [ ] 玩家/回响有效攻击均计数，同一攻击身份多次命中只计一次；任一 OR 条件满足后仅变身一次。
- [ ] 变身期间停止移动/新攻击并取消未提交行动，完成后切换二阶段逻辑与 AnimationSet。
- [ ] 普通、特殊和 Boss 攻击按事件进入 Charge，再在提交点进入 Basic；动画结束不改变玩法时序。
- [ ] 保存/读取中阶段、计数、过渡状态不丢失、不重复触发。
- [ ] 一二阶段所有状态保持相同 `Profile.WorldHeight` 归一化、FootRoot 原点、阴影宽度与 Flipbook 一致且不旋转。
- [ ] 缺失临时/正式动画安全回退；AnimationSets 无重复项。
- [ ] 构建、聚焦自动化、`validate_project.py` 与 `git diff --check` 通过；PIE 人工验收通过后关闭。

## Step 0 门禁

- 基线：`origin/main @ 4250740`；执行前和发布前均 fetch 并比较远端。
- Unreal Editor 必须关闭后才能构建、导入或修改二进制资产。
- 当前工作区存在大量用户美术资源修改；全部视为基线外，不清理、不暂存、不提交。
- 先确认伤害事件已有的来源与攻击身份是否足够；共享契约若需扩展，先更新生产者/消费者与快照测试。
- Plan 发布和最终实现发布均执行 `Build-Editor.cmd -Configuration Development -FullRebuild`，刷新精选 prebuilt，再跑 `python scripts/validate_project.py`。
- 任一基线测试、数据同步、构建或 prebuilt 指纹失败即停止发布并报告。

## 实现提纲

1. 数据层新增可选阶段配置 schema、XLSX 工作表/同步与验证；先建立测试数据，不擅自填写生产数值。
2. EnemyLogic 增加阶段运行时状态、OR 判定、攻击身份去重、变身计时、动作取消及 snapshot round-trip。
3. EnemyActor 将当前已选仇恨目标的距离与 `CanAttractAggro` 输入逻辑，并把 Combat 的有效伤害结果转交计数接口。
4. 新增强类型阶段事件（Started/Completed，包含阶段、AnimationSet、时长、触发原因）；重复/恢复路径不得重复广播。
5. 将 Special/Boss 的 windup/commit 事件映射为 Charge/Basic，并增加 Phase2 Transform 与阶段 AnimationSet 切换。
6. 迁移 DataAsset：去除重复 AnimationSets，保留逐动画镜像；一二阶段所有状态共用同一 WorldHeight/FootRoot/阴影流程。无正式片段时配置临时回退。
7. 补齐逻辑、表现、数据、存档自动化和文档；增量构建迭代，最终候选执行完整发布门禁。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 数据 | XLSX 同步/数据 schema 聚焦测试 | CSV 可重复生成；非法/双禁用配置有明确诊断 |
| 逻辑 | `ReEcho.Enemies.Logic` | OR、仇恨例外、攻击去重、一次性与 snapshot 全绿 |
| 契约 | `ReEcho.Combat`（若修改） | Player/Echo 来源和 AttackIdentity 兼容 |
| 表现 | `ReEcho.Presentation.Animation2D` | Charge/Basic/Transform/Phase2 状态与回退全绿 |
| 集成 | 敌人宿主与存档自动化 | 当前目标输入、变身暂停、保存恢复正确 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目不变量及格式全绿 |
| 构建 | 增量构建；发布前 `-FullRebuild` | UE 5.8 Editor 编译与 prebuilt 匹配 |
| 人工 PIE | 具名测试怪物分别以距离/攻击次数触发 | 回响例外、动画尺寸/脚点/阴影、存读档与战斗手感正确 |

## 执行记录

### 变化

- 已新增可选 Phase2 定义、`Transforming` 逻辑状态、攻击次数去重快照与当前仇恨目标距离输入。
- 已实现 OR 触发、攻击次数优先的稳定同帧决策、变身期间动作取消、逻辑计时完成及存档恢复。
- 已新增 `Animation.Attack.Charge` / `Animation.Transform.Phase2`，特殊怪和 Boss 按 windup/commit 事件切换 Charge/Basic；阶段事件驱动 Transform 与 Phase2 AnimationSet。
- Enemy Host 仅在玩家或已由 EchoTaunt 规则选中的回响为当前目标时设置距离触发资格。

### 证据

- UE 5.8 Development Editor 增量构建通过（UHT/UBT，2026-08-20）。
- `ReEcho.Enemies.Logic` 自动化通过；新增 `Phase2.RangeAggroPolicy` 与 `Phase2.AttackCountAndSnapshot` 均为 Success。

### 剩余风险

- 生产怪物与数值未确认前只能证明通用系统和测试夹具，不能宣称正式关卡配置完成。
- 变身次数以 Combat 最终 `AppliedDamage > 0` 的实际扣血事件为准；同一攻击多段扣血逐段计数。
- DataAsset 为二进制 Exclusive；迁移时必须关闭编辑器并只提交具名资产。

### 人工验收结果/请求

### 架构文档审阅结果
