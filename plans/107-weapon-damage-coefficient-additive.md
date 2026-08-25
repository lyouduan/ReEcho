# Plan 107 - 程序 - 武器伤害倍率加法语义

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`NotRequired`；倍率、属性来源和真实攻击伤害均可由自动化精确验证。
- 本地规划基线：`origin/main@061083d59373f420da6637499ea4f0a215ed1854`；最终实现已变基到 `origin/main@dc46a89a`。
- 本地实现方式：`plan/107-weapon-damage-coefficient-additive`；`C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan107-damage-coefficient-additive`。
- 依赖 / 阻塞：依赖已进入主线的 Plan104 单一 `DamageCoefficient` 契约。Plan106 攻击速度候选会并行修改同一工作簿、`part_effects.csv`、武器测试、模块文档和预构建包；发布实现前必须以实际最新 `origin/main` 重新审计和组合适配。
- Writes:
  - `plans/107-weapon-damage-coefficient-additive.md`
  - `Design/Data/ReEchoData.xlsx`
  - 由工作簿生成的 `Content/Data/part_effects.csv`
  - `scripts/validate_project.py`
  - `scripts/data/test_sync_xlsx_to_csv.py`
  - `Source/ReEcho/Private/Tests/ReEchoWeaponRuntimeTests.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`
  - 最终 FullRebuild 刷新的 `Binaries/Win64/` 精选预构建包
- Stable Reads:
  - `Content/Data/{weapons,attack_steps,parts}.csv`
  - `Source/ReEcho/{Public,Private}/Weapons/ReEchoWeaponRuntime.*`
  - `Source/ReEchoWeapons/{Public,Private}/Weapons/`
  - `plans/104-four-weapon-roster.md`
  - `plans/106-weapon-attack-speed-semantics.md`（并行候选契约审计）
  - `shared/CODEBASE_MAP/{ARCHITECTURE,README}.md`
- 影响模式：`SharedContract`；收紧 `PartEffects/Target=DamageCoefficient` 的数据契约，并改变所有武器符文伤害倍率的解释方式。
- 兼容承诺 / 下游操作：保留 Plan104 的单倍率和宝石选属性来源契约；`DamageCoefficient` 仍是小数比例，符文 Value 改为有符号百分点增量。发布时重新生成 CSV、重建 Editor 预构建包。
- 明确排除：不修改攻击速度、攻击时长、物攻/元攻属性来源、核心宝石通道、武器基础倍率、攻击段基础倍率、其他武器参数或符文中文描述。

## 锁定目标

1. 武器符文的伤害倍率按“武器/攻击段初始倍率 + 所有符文有符号倍率增量”计算，不再将符文倍率当作乘数。
2. 生产枪初始 `20%` 搭配“极限蓄能枪口”`+180%` 后，武器 Definition、实际攻击段和真实伤害提交均使用 `200%`，即 `0.2 + 1.8 = 2.0`。
3. 所有生产 `DamageCoefficient` 符文使用 `ValueOp=Add`：`-40%` 写 `-0.4`，`+50%` 写 `0.5`，`+180%` 写 `1.8`。数据验证拒绝该 Target 使用 Multiply/Override。
4. Plan104 的物理/元素单倍率矩阵保持有效：宝石只选择 `PhysicalAttack` 或 `ElementalAttack`，不参与倍率加法。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`（`AREA-Data`、`AREA-Tests`）、`MOD-ReEchoWeapons`（`AREA-Weapons`）。
- 对应模块文档：维护 `MOD-ReEcho.md` 与 `MOD-ReEchoWeapons.md`，均已加入 Writes。
- 设计意图：让策划表中的“伤害倍率 ±N%”直接保存为百分点增量；初始倍率继续由 Weapons/AttackSteps 权威持有，装备编译按现有 ValueOp 管线加到两个投影，避免基础倍率不是 100% 时出现隐蔽乘法错误。
- 权威状态与依赖：状态所有者和模块依赖方向不变。XLSX 是符文数值权威，生成 CSV 是运行时输入，Weapons 继续负责有效 Definition 和 AttackStep，宝石继续只决定属性来源。
- 决策记录：保留通用 `ApplyValueOperation`，由数据 Target 契约强制 `Add`；相比在 C++ 忽略 ValueOp，这能让 XLSX/CSV 自描述并让错误在发布门禁阶段带定位失败。运行时现有 RuleFlag/AttackStep 应用链已支持顺序线性加法，无需新增第二倍率字段。
- 相关文档同步范围：必审 `ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoWeapons.md`；模块拓扑预计不变，审阅后在执行记录注明。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅模块拓扑与状态所有权；
  - `README.md`：待审阅 AREA 路由；
  - `MOD-ReEcho.md`：待同步 XLSX/CSV Target 专用加法门禁和装备投影；
  - `MOD-ReEchoWeapons.md`：待同步基础倍率与符文增量相加公式。

## 锁定验收

- [x] 枪 `20% + 180% = 200%` 在 Weapon Definition、AttackStep 与真实物理/元素攻击提交中均有精确证据。
- [x] 弓 `100% - 40% = 60%`、弓/剑 `100% + 50% = 150%`，全部启用伤害倍率符文都按有符号加法编译。
- [x] 工作簿与生成 CSV 无漂移；所有 `DamageCoefficient` 符文使用 Add，错误 ValueOp 会产生带工作表/表/行/列定位的校验失败。
- [x] Plan104 四武器乘三类核心宝石的单倍率矩阵继续通过，攻击速度数据和实现无本任务差异。
- [x] 聚焦 Weapons 自动化、XLSX 同步测试、项目校验、最终 Development FullRebuild、预构建检查和 `git diff --check` 通过。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@061083d5`；已包含 Plan104 单一伤害倍率实现和后续角色重开变形修复。
- 引擎/构建可用性：UE 5.8 / Win64 有效，无运行中的 UnrealEditor；基线精选预构建包可直接执行自动化。
- 现有聚焦测试结果：`ReEcho.Weapons` 17/17 通过；其中极限蓄能枪口断言仍把 `+180%` 错误解释为乘 `2.8`，明确锁定错误结果 `0.56`。XLSX 同步检查与项目校验通过。
- 共享契约 / 难合并资源风险：Plan106 与本任务在二进制工作簿、生成 CSV、武器测试、模块文档和最终 DLL 物理重叠；逻辑可独立开发，但不得在发布前跳过最新远端组合审计。
- 基线损坏时的停止条件：若除上述错误语义外的武器矩阵、属性来源或同步门禁出现非本任务失败，先区分基线问题，不以放宽断言或恢复双倍率字段掩盖。

## 实现提纲

1. 将权威工作簿中四条生产 `DamageCoefficient` 效果改为 `Add` 与有符号增量，并确定性生成 `part_effects.csv`。
2. 在项目数据校验中强制 `DamageCoefficient` 只能使用 `ValueOp=Add`，补充工作簿错误用例并验证定位信息。
3. 扩充生产武器回归：枚举全部伤害倍率符文；验证枪极限蓄能的 Weapon/Step/物理与元素真实伤害均为 `2.0` 倍；保留 Plan104 宝石矩阵。
4. 同步模块文档与执行记录，完成格式化、聚焦自动化、全量构建、预构建和发布前远端审计。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| XLSX/CSV | `python scripts/data/test_sync_xlsx_to_csv.py`；`python scripts/data/sync_xlsx_to_csv.py --check` | 权威工作簿与 CSV 无漂移，错误 DamageCoefficient ValueOp 被定位拒绝 |
| Weapons | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Weapons` | 全部生产倍率符文、枪 200% 实际攻击和 Plan104 单倍率矩阵通过 |
| C++ | `.clang-format`；`scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 退出码为 0并刷新精选开发预构建包 |
| 静态/发布 | `python scripts/validate_project.py`；`python scripts/ue/prebuilt_editor.py check`；`git diff --check` | 项目、文档、预构建与文本门禁通过 |
| 远端审计 | `git fetch origin` 后比较候选基线与 `origin/main` | Plan106/其他并行重叠已组合或明确留待集成，不覆盖最新契约 |

## 执行记录

### 变化

- 2026-08-25：用户要求继续实现“伤害倍率与武器初始倍率相加”，具名验收为枪 `20% + 180% = 200%`。
- 2026-08-25：只读审计确认 Plan104 的单倍率和宝石属性来源已经进入主线；当前四条伤害倍率效果仍使用 Multiply。弓/剑基础为 `1.0` 因而结果偶然正确，枪基础为 `0.2`，现有生产测试锁定了错误结果 `0.56`。
- 2026-08-25：基线 `ReEcho.Weapons` 17/17、XLSX 同步检查和项目校验通过；已用工作簿工具渲染审阅全部 14 个工作表及目标效果区，现有格式完整。
- 2026-08-25：四条权威效果改为 `Add/-0.4/+0.5/+0.5/+1.8`；装备编译改为独立汇总 `Weapon.DamageCoefficientModifier`，最后分别加到武器与 AttackStep 初始倍率并统一钳制非负。发布门禁和工作簿错误用例拒绝该 Target 使用非 Add 运算。
- 2026-08-25：候选变基到最新 `origin/main@dc46a89a`，保留 Plan105 对工作簿和两条符文禁用的变化并重新生成 CSV。主线禁用镰刀投掷回收与长剑陨星后未同步的旧测试目录已做最小适配：只更新启用数量并停止执行禁用符文，不恢复其数据或行为。

### 证据

- `python scripts/data/test_sync_xlsx_to_csv.py`：18/18 通过；`sync_xlsx_to_csv.py --check` 通过。错误 DamageCoefficient ValueOp 可映射回 `武器插槽C/tblPartEffects` 的具体行列；生产工作簿保护、生成字节和全部 14 个工作表最终渲染复核通过。
- `ReEcho.Weapons`：最新主线组合候选 17/17 通过；`StaticEffectsReachAttackSteps` 覆盖四条生产倍率增量，枪 Definition/Step 为 `2.0`，物理与火焰核心真实提交分别为 `PhysicalAttack × 2.0`、`ElementalAttack × 2.0`；`DamageCoefficientMatrix` 继续覆盖四武器乘三核心。
- Development Editor 初次组合构建 95 个动作、运行时重构增量 6 个动作、主线禁用符文测试适配增量 5 个动作均通过；最终 FullRebuild 完成 95 个动作并刷新精选预构建包。
- `validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过。

### 剩余风险

- Plan106 尚未进入当前主线，本候选不复制其攻击速度变化；两者后续集成必须从各自 XLSX/CSV 单元格变化组合后重新生成并测试。两项契约逻辑独立，但共改同一二进制工作簿、生成 CSV、测试和预构建包。

### 人工验收结果/请求

- `NotRequired`；真实攻击提交与生产数据均可自动化精确验证。

### 架构文档审阅结果

- `ARCHITECTURE.md`：已审阅；Runtime Module 拓扑、依赖方向与状态所有者未变化，无需修改。
- `README.md`：已审阅；AREA 路由和模块索引未变化，无需修改。
- `MOD-ReEcho.md`：已同步 XLSX/CSV 的 DamageCoefficient 专用 Add 门禁、装备增量 RuleFlag 和 Definition/Step 投影边界。
- `MOD-ReEchoWeapons.md`：已同步 `Max(0, 初始倍率 + Σ符文增量)`、末端钳制和宝石不参与倍率加法的契约。
