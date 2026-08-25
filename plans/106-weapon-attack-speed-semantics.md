# Plan 106 - 程序 - 武器攻击速度语义统一

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`NotRequired`；精确数值、叠层和运行时节奏由自动化覆盖，若最终 PIE 手感需要观察则仅作为非阻塞补充。
- 本地规划 / 实现基线：`origin/main@f98ffeee776fb1317d253d6493eb7a13559cd5d8`。
- 本地实现方式：`plan/106-weapon-attack-speed-semantics`；`C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan106-attack-speed`。
- 依赖 / 阻塞：以已进入主线的 Plan104 四武器与统一伤害倍率实现为稳定基线；Plan105 与本任务逻辑独立，但若先进入主线，发布前仍需重新审计实际远端重叠。
- Writes:
  - `plans/106-weapon-attack-speed-semantics.md`
  - `Design/Data/ReEchoData.xlsx`
  - 由工作簿确定性生成的 `Content/Data/part_effects.csv` 及实际随生成器变化的清单文件
  - `Source/ReEchoWeapons/{Public,Private}/Weapons/` 中攻击时长换算契约
  - `Source/ReEchoWeapons/Private/Tests/ReEchoWeaponLogicTests.cpp`
  - `Source/ReEcho/{Public,Private}/Weapons/` 中符文编译、武器 Actor 与镰刀持续攻击节奏
  - `Source/ReEcho/Private/Tests/ReEchoWeaponRuntimeTests.cpp`
  - `Source/ReEchoCombat/{Public,Private}/AbilitySystem/` 中临时攻速加成的加法语义
  - `Source/ReEchoCombat/{Public,Private}/Combat/` 中临时攻速叠层与回收
  - `Source/ReEchoCombat/Private/Tests/ReEchoCombatRuntimeTests.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - 最终 FullRebuild 刷新的 `Binaries/Win64/` 精选预构建包
- Stable Reads:
  - `plans/76-weapon-rune-completion.md` 与 `plans/104-four-weapon-roster.md`
  - `plans/105-card-outcome-tooltips.md`
  - Plan104 已落地主线的 `DamageCoefficient` 单倍率契约、数据和回归测试
  - `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `shared/CODEBASE_MAP/README.md`
- 影响模式：`SharedContract`。改变武器符文 `AttackSpeed` 数值的解释、Weapons 的攻击时长公共契约，以及 Combat 临时攻速层的合成方式。
- 兼容承诺 / 下游操作：`FReEchoStatBlock::AttackSpeed` 的角色基础中性值继续为 `1.0`；装备符文的有符号攻速修正独立编译到武器定义，运行时临时层按百分点加到同一修正率，不把 `-500%` 塞入要求为正的角色属性。存档仍保存稳定 PartId 并在加载时重编译，不新增存档迁移。
- 明确排除：不修改 `DamageCoefficient`、伤害来源属性、伤害倍率数据或相关测试；不调整武器基础攻击时长、符文策划描述、敌人攻击速度、卡牌平衡、移动速度语义或 UI 风格。

## 锁定目标

1. 武器攻速修正统一为有符号比例 `r`：正数表示加速，负数表示减速；最终攻击时长按 `初始攻击时长 × (1 - r)` 计算。例：`0.3s` 搭配 `+60%` 得 `0.12s`，搭配 `-30%` 得 `0.39s`，搭配 `-500%` 得 `1.8s`。
2. 武器符文表中所有 `AttackSpeed` 静态效果改为与描述相同的加法百分点，禁止继续用倒数乘数近似。多个静态攻速符文以及静态符文与临时攻速层按修正率相加后只换算一次攻击时长。
3. 攻击间隔、攻击步骤行为锁定时长、无敌步骤时长与镰刀驻留攻击的命中间隔消费同一个时长倍率，避免“能再次攻击”和“当前攻击仍在执行”采用不同攻速公式。
4. `AttackSpeedPerHit`、`AttackSpeedOnAttack` 等运行时层每层精确增加描述给出的百分点；叠层、到期和取消必须可逆，不受装备静态攻速大小影响，也不得改变移动速度层的现有乘法语义。
5. 公式结果在进入计时器时保留现有 `0.01s` 最小时长安全边界，防止总正修正达到或超过 `+100%` 后产生零或负计时；该边界不改变低于上限的策划例值。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoWeapons`（`AREA-Weapons`）、`MOD-ReEchoCombat`（`AREA-Combat`）、`MOD-ReEcho`（武器数据适配与 `AREA-Tests`）。
- 对应模块文档：维护 `MOD-ReEchoWeapons.md`、`MOD-ReEchoCombat.md`、`MOD-ReEcho.md`，均已加入 Writes。
- 设计意图：让策划表中的“攻击速度 ±N%”保持人类可读的百分点语义，由 Weapons 在最终计时时集中换算为时长；Combat 只负责提供可逆的临时百分点层，不持有武器初始时长。
- 权威状态与依赖：XLSX 继续是符文数值权威，CSV 是生成产物；主模块把装备静态攻速修正编译进不可变武器定义，`ReEchoWeapons` 负责最终时长倍率，`ReEchoCombat` 继续拥有角色基础/临时属性状态。最终命中和伤害裁决边界不变。
- 决策记录：不将 `-500%` 表示为 `1/6` 乘数，因为该表示无法与 `+60%/-30%` 的策划例值及加法叠层同时成立；不允许角色 `AttackSpeed` 变为负值，而是在武器定义中保存装备修正，避免破坏 GAS 属性约束及非武器消费者；临时攻速从乘数改为加法百分点，移动速度继续沿用原乘数。
- 相关文档同步范围：必审 `ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoWeapons.md`、`MOD-ReEchoCombat.md`；预计模块拓扑和索引不变，若实现审阅确认不变则在执行记录中说明无需修改。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅权威链和依赖方向；
  - `README.md`：待审阅 AREA 路由；
  - `MOD-ReEcho.md`：待同步 XLSX/CSV 攻速百分点及适配事实；
  - `MOD-ReEchoWeapons.md`：待同步最终攻击时长公式与统一消费者；
  - `MOD-ReEchoCombat.md`：待同步临时攻速加法层及移动速度兼容边界。

## 锁定验收

- [ ] 以基础攻击时长 `0.3s` 验证：`+60%` 为 `0.12s`、`-30%` 为 `0.39s`、`-500%` 为 `1.8s`；中性值保持 `0.3s`。
- [ ] 所有启用的静态攻速符文逐行对照策划描述，以加法百分点进入有效武器定义；多符文组合不因顺序不同产生不同结果。
- [ ] 攻击间隔、步骤锁定/无敌时长和镰刀驻留命中间隔采用同一倍率，并有加速、减速和极端减速自动化证据。
- [ ] 运行时每击/每次攻击攻速层按百分点线性叠加，叠在任一静态符文上仍准确；单层和多层到期后完整恢复且移动速度行为不回归。
- [ ] 统一伤害倍率提交前后的 `DamageCoefficient` 数据、实现和回归断言无本任务差异。
- [ ] XLSX 同步检查、聚焦 Weapons/Combat 自动化、最终 Development FullRebuild、项目校验、预构建检查和 `git diff --check` 通过。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@f98ffeee776fb1317d253d6493eb7a13559cd5d8`；包含 Plan104 实现、统一伤害倍率提交及其预构建刷新。用户确认“104 已推送到远端”后授权继续。
- 引擎/构建可用性：沿用 UE 5.8 标准脚本；独占构建/自动化前检查交互式 Editor 与 Git common-dir 锁。
- 现有聚焦测试结果：规划期静态审计确认旧实现用 `AttackInterval / AttackSpeed` 和步骤时长除法，枪 `-500%` 以 `×1/6` 近似；这些旧断言是本 Plan 要替换的契约，不作为新行为证据。
- 共享契约 / 难合并资源风险：`ReEchoData.xlsx` 是二进制共享源，最终 `Binaries/Win64` 也是共享发布面；Plan105 若先合入，必须从最新远端组合适配并重新生成/构建，不覆盖其结果。
- 基线损坏时的停止条件：若未修改候选上的单倍率伤害矩阵、装备重编译、Combat 临时层回收或四武器基础攻击失败，先区分基线问题，不通过改伤害倍率或削弱断言绕过。

## 实现提纲

1. 用表格工具在权威工作簿把攻速静态效果改为有符号加法比例，并由仓库脚本确定性回生成 CSV；逐行核对描述、ValueOp 与 Value。
2. 在装备编译阶段累计武器静态攻速修正，并把类型化值传给资源无关 `FReEchoWeaponDefinition`；不污染角色基础 `AttackSpeed`。
3. 在 `ReEchoWeapons` 集中实现时长倍率，供攻击间隔、步骤锁定/无敌与主模块镰刀驻留攻击共同消费。
4. 将 Combat 临时攻速 GameplayEffect/fallback 改为加法百分点和精确可逆层；移动速度参数与现有乘法行为保持不变。
5. 补充数据、Weapons、主模块与 Combat 回归，审阅模块文档，完成聚焦自动化、最终 FullRebuild、静态门禁和远端集成审计。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| XLSX/CSV | `python scripts/data/test_sync_xlsx_to_csv.py`；`python scripts/data/sync_xlsx_to_csv.py --check` | 权威工作簿与生成 CSV 无漂移，所有攻速行使用有符号加法百分点 |
| Weapons 纯逻辑 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Weapons.Logic` | `0.3s` 四组例值、步骤时长和最小时长边界通过 |
| 装备/世界执行 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Weapons` | 静态符文组合、枪 `-500%`、镰刀驻留间隔和伤害倍率无回归 |
| Combat | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Combat` | 临时层与静态修正线性合成、到期恢复、移动速度兼容通过 |
| C++ | `.clang-format`；`scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 退出码为 0并刷新精选开发预构建包 |
| 静态/发布 | `python scripts/validate_project.py`；`python scripts/ue/prebuilt_editor.py check`；`git diff --check` | 项目、源码、文档、预构建与文本门禁通过 |
| 远端审计 | `git fetch origin` 后比较候选基线与 `origin/main` | 无遗漏的 Plan105/其他并行重叠，伤害倍率契约保持主线最新状态 |

## 执行记录

### 变化

- 2026-08-25：用户确认本次仅处理攻击速度，伤害倍率等待其他 AI 的提交；随后确认 Plan104 已推送远端并授权继续。
- 2026-08-25：从含统一伤害倍率提交的 `origin/main@f98ffeee` 建立独立工作树。静态审计确认旧实现按速度乘数做除法，无法同时满足 `+60% → ×0.4`、`-30% → ×1.3` 与修正率加法叠层。

### 证据

- 规划期源码与生产 CSV 审计完成；尚未产生实现验证证据。

### 剩余风险

- 总正攻速达到 `+100%` 时数学公式给出零时长，运行时必须明确保留现有 `0.01s` 安全下限；当前生产数值低于该边界，但叠层可触达。
- 静态装备修正与 Combat 临时层位于不同模块，必须通过组合测试证明只相加一次，并覆盖 GAS 与无 GAS fallback 两条路径。

### 人工验收结果/请求

- `NotRequired`；如用户希望比较极限蓄能枪口的实际节奏，可在自动化通过后补做 PIE 观察，但不替代数值断言。

### 架构文档审阅结果

- 待实现后填写。
