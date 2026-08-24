# Plan 87 - 程序 - 清理勇者 Forge 并对齐角色能力

## 协调

- Planner 负责人：Codex（Gavyn 侧）。
- Executor 负责人：待用户审核后分配。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Unassigned`。
- 任务状态：`Proposed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（角色能力数值、勇者受伤阈值手感与旧 Forge 消失需要用户 PIE 确认）。
- 本地规划 / 实现基线：`origin/main@55a15d1505f52fa182bfb13bb5b5e01770100520`。
- 本地实现方式：一任务一 worktree，`C:/Users/gavynqiu/Documents/miniGame/ReEcho-plan87-forge-character-abilities`，分支 `plan/87-forge-character-abilities-audit`；Plan 先发布占号，审核通过后才进入实现。
- 依赖 / 阻塞：
  - 需求输入是仓库内 `策划数据源/【开普勒】回响数值与构筑体系.xlsx` 的可见 `角色体系J!D2:D5`；隐藏行/列按项目规则视为不存在。本次审计时该 Sheet 无隐藏生产行/列。
  - 生产编辑与导出真源仍是 `Design/Data/ReEchoData.xlsx`，运行时只读 `Content/Data/*.csv`；策划数据源工作簿只作需求对照，不直接成为运行时输入。
  - 勇者能力的层数语义尚需用户审核确认：按“当前缺失生命的 10% 阶梯、治疗会回退”还是“累计损失、获得后永久保留”。未确认前不得实现。
  - 需用户确认是否一并删除最新可见能力列未声明的诗人“随机元素弹”和勇者“每第二击 +50%”遗留能力；本 Plan 默认建议删除，以可见策划源为准。
  - Plan88 卡牌掉落系统已先发布 Plan-only 文档；它与本 Plan 的未来实现共享 Cards/Run 热点。两项任一开始实现前都必须以届时最新 `origin/main` 复核 Writes 和契约，不能用规划时基线覆盖另一方的新逻辑。
- Writes:
  - `plans/87-forge-cleanup-and-character-abilities.md`
  - `Design/Data/ReEchoData.xlsx`
  - `Content/Data/characters.csv`、新增的角色能力规范化 CSV（最终表名在实现前锁定）
  - `Content/Data/cards.csv`、`Content/Data/card_effects.csv`、`Content/Data/csv_schema.csv`、`Content/Data/reecho_data_manifest.csv`
  - `Content/Data/TestFixtures/CsvRuntime/**` 中包含 Forge 生产副本或受新表/schema 影响的精选 fixture
  - `scripts/data/sync_xlsx_to_csv.py`、`scripts/data/test_sync_xlsx_to_csv.py`、`scripts/validate_project.py`
  - `Source/ReEcho/Public/Data/ReEchoCsvDataRegistry.h`
  - `Source/ReEcho/Private/Data/ReEchoCharacterBuildCsvReader.cpp`、`Source/ReEcho/Private/Data/ReEchoCsvDataRegistry.cpp`
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`、`Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Source/ReEcho/Public/Run/CharacterAbilities/**`、`Source/ReEcho/Private/Run/CharacterAbilities/**`（新增；具体文件名可在不改变职责的前提下优化）
  - `Source/ReEcho/Public/Core/ReEchoTypes.h`（仅旧 `ForgeChoice` 阶段移除及必要保存兼容）
  - `Source/ReEcho/Public/Player/ReEchoPlayerPawn.h`、`Source/ReEcho/Private/Player/ReEchoPlayerPawn.cpp`（仅将 Combat 生命结果路由到角色能力命令）
  - `Source/ReEcho/Public/UI/ReEchoTraitCardChoiceWidget.h`、`Source/ReEcho/Private/UI/ReEchoTraitCardChoiceWidget.cpp`
  - `Source/ReEcho/Public/ReEchoGameMode.h`、`Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Run/ReEchoCharacterPromotion.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoCharacterPromotionTests.cpp`、新增角色能力聚焦测试及受影响 Save/Run/UI 测试
  - `Content/Data/README.md`、`Design/Data/ReEchoData使用说明.md`、`Design/Data/ReEchoData策划验收清单.md`
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoCombat.md`、`MOD-ReEchoCards.md`、`MOD-ReEchoUI.md`
- Stable Reads:
  - `策划数据源/【开普勒】回响数值与构筑体系.xlsx`
  - `Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`
  - `Source/ReEchoCombat/Public/Combat/ReEchoCombatantComponent.h`、配对实现
  - `Source/ReEchoWeapons/**`（验证勇者攻击增益被武器伤害快照消费，不在 Weapons 写角色分支）
  - `Source/ReEchoCards/**`（确认删除 Forge 数据后通用卡牌授予和普通抽取不回归）
  - `plans/15-character-promotion.md`、`plans/22-data-character-build-csv.md`、`plans/47-card-build-runtime-module.md`
- 影响模式：`SharedContract`（修改角色数据 schema、Run/Combat 事件接缝及卡牌目录；`ReEchoData.xlsx` 是不可文本合并的二进制编辑真源，实施期间必须独占）。
- 兼容承诺 / 下游操作：保留四个稳定角色 ID、别名、晋升结果、基础生命/物攻/元攻和现有普通卡牌流程；角色能力数值只来自规范化生产表，逻辑只按注册行为执行；旧存档处于 `ForgeChoice` 时必须确定性迁移到普通 `CardChoice`，不能卡死或静默再次发放 Forge 收益。
- 明确排除：不改变角色晋升计分规则、默认武器、角色/武器表现、普通卡牌内容、商店、Echo、敌人、音频或最终伤害裁决；不把角色能力逻辑塞进 Combat/Weapons/Cards；不让运行时读取或解析 XLSX 中文自由文本；不在本 Plan 中修改策划数据源原始工作簿。

## 现状审计与结论

### 勇者 Forge

Forge 不是无引用残留，而是当前可运行的旧勇者能力：

1. `characters.csv` 以 `Character.BraveForge` 标记勇者；注册表和 schema 允许该 ID。
2. `CompleteEncounter` 在非终局胜利后把勇者送入 `EReEchoRunPhase::ForgeChoice`。
3. `GenerateForgeOffers` 从卡牌目录取 `OfferGroup=Forge` 的 `FORGE_LIGHT/MEDIUM/EXTREME`；`ApplyForgeChoice` 通过 Cards 通用授予逻辑应用九条属性效果，再进入普通选卡。
4. GameMode 和 `UReEchoTraitCardChoiceWidget` 识别 Forge 阶段、标题和提交路径。
5. `ReEcho.Characters.BraveForge` 自动化锁定这条行为；数据 fixture、说明和图标脚本也保留三张 Forge 卡。

因此清理必须同时移除流程、数据、UI 特化、测试和存档兼容，不能只删三行 CSV。

### 角色能力映射

| 可见策划源 | 当前 CSV | 当前逻辑 | 结论 |
|---|---|---|---|
| 智者 J_01：每第 4 次获得卡牌组时额外选 1 张 | `Character.SageBonusChoice` + `PassiveValue=4` | 已实现每 4 次普通卡牌选择追加一次选择，但代码按 `RoleId=Sage` 和常量 `4` 判断，未消费 `PassiveBehaviorId/PassiveValue` | 行为可用，配置映射不完整 |
| 猎手 J_02：移速 +20%、暴击率 +20%、暴击伤害 +50% | `MovementSpeed=1.0`、`CriticalRate=0.4`、`CriticalEffect=1.0` | 暴击两项会随角色属性进入武器伤害；移速按 `420 * MovementSpeed`，当前仍为 100% | 暴击已实现，移速漏实现 |
| 诗人 J_03：每一关结束后反应效能永久 +10% | `Character.PoetReactionGrowth` + `PassiveValue=0.05` | `CompleteEncounter` 后永久加 `PassiveValue` | 机制已实现，生产数值仍是旧 +5% |
| 勇者 J_04：每损失 10% 血量，物攻和元攻各 +1 | `Character.BraveForge`；另有 `EverySecondAttackBonus=0.5` | 实际运行 Forge；武器还消费每第二击 +50%；没有血量阈值增长 | 最新能力未映射、未实现，存在两套旧能力 |

此外，诗人当前 `RandomElementProjectiles=true` 会让投射物随机元素，但该行为未出现在本次可见角色能力列。它与勇者第二击加成一并列为待用户确认删除的旧设计。

当前工程并不存在统一的角色能力执行器。`PassiveBehaviorId` 只有校验/查找作用，诗人、勇者和智者分别由 RunSubsystem 的不同硬编码分支执行，猎手则混入最终基础属性。Plan87 将建立单一、类型化、可配数值的角色能力入口，而不是继续增加第四种特殊分支。

## 锁定目标

在用户确认两项待决语义后，以可见 `角色体系J!D2:D5` 为角色能力需求权威，完成以下结果：

1. 删除勇者旧 Forge 能力及其专用流程、三张 Forge 卡、九条 Forge 效果、UI 特化和测试；旧保存若停在 Forge 阶段，安全进入普通卡牌选择。
2. 建立从 `Design/Data/ReEchoData.xlsx` 的规范化角色能力表到 UTF-8 CSV、类型化 Definition/Runtime、Run/Combat 事件接缝和保存状态的单一链路；数值参数由表管理，行为 ID 只选择已注册代码逻辑。
3. 智者能力真正消费配置的间隔与额外选择数；猎手获得 20% 移速、20% 暴击率和 50% 暴击效果；诗人每次合法关卡完成永久增加 10% 反应效能；勇者按确认后的生命损失语义，每个 10% 阈值使物攻、元攻各增加 1。
4. 移除用户确认废弃的未列出角色遗留能力，且普通角色晋升、卡牌、存读档、战斗伤害和 UI 流程不回归。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`（`AREA-Data`、`AREA-Run`、`AREA-Player`、`AREA-UI`、`AREA-Tests`）、`MOD-ReEchoCombat`、`MOD-ReEchoCards`，以及文档型 `MOD-ReEchoUI`。
- 对应模块文档：`MOD-ReEcho.md` 维护角色能力 Definition/Runtime、Run 权威和 Player 事件路由；`MOD-ReEchoCombat.md` 明确只发布最终生命变化并提供通用属性命令，不认识角色 ID；`MOD-ReEchoCards.md` 删除 Forge 作为卡牌目录特殊消费者的事实；`MOD-ReEchoUI.md` 删除 Forge 标题/提交流程。以上均已加入 Writes。
- 设计意图：把“触发条件与效果数值”从中文描述和分散分支中规范化，把“如何执行”集中到角色能力 Runtime。Run 继续拥有整局构筑/永久成长；Combat 继续拥有遭遇内生命和战斗属性；Player 只做事件到窄命令的适配。
- 权威状态与依赖：
  - XLSX/CSV → 不可变角色能力 Definition 是配置权威。
  - `UReEchoRunSubsystem` 继续拥有跨关永久能力状态和 `CurrentBuild`。
  - `UReEchoCombatantComponent`/GAS 继续拥有当前 Actor 的生命与即时战斗属性；勇者只消费其最终 `HealthChanged`，不得从原始碰撞或候选伤害推算。
  - `MOD-ReEchoCombat`、`MOD-ReEchoCards`、`MOD-ReEchoWeapons` 不反向依赖主模块角色类。
- 决策记录：
  - 不直接解析 `角色能力` 中文字符串。规范化生产表新增角色能力/效果参数，导出独立 CSV，并在加载时拒绝未知 Trigger、EffectKind、Target、BehaviorId 或缺失必填参数。
  - 猎手的三项静态能力以明确的“角色获得时属性效果”表达，避免把能力来源继续伪装成无来源基础值；若实现评估认为现有 promotion delta 必须保留最终 StatBlock，则至少要由生成器/聚焦测试证明三个可见数值逐项映射。
  - 诗人和智者不再在 RunSubsystem 直接比较角色名并使用常量；统一按当前角色的类型化能力 Definition 分派。
  - 勇者只消费 Combat 已发生的最终生命变化。格挡、无敌、零伤害、初始化、复活、最大生命调整不得错误生成损血层数。
  - Forge 数据从生产 workbook/CSV 删除而不是仅 `Enabled=false`，因为最新可见角色能力已经替换该机制；历史 Plan 保留审计事实，不回写。
  - 普通卡牌 Widget 可继续复用，但删除 `bForgeChoice`、Forge 标题和 Forge 提交分支；UI 不执行角色能力。
  - 保存兼容采用确定性迁移，不恢复 Forge：旧 `ForgeChoice` → `CardChoice`，丢弃仅与旧 Forge 候选相关的瞬时选择；已应用到 StatBlock 的历史数值不逆向猜测或扣除。
- 待用户审核决策：
  1. **推荐**勇者使用“当前缺失生命阶梯”：`floor((MaxHealth-CurrentHealth)/(MaxHealth*10%))`，治疗会回退层数，复活/满血归零；该模型无重复受伤刷永久属性的问题。备选是“累计最终伤害永久换层”，需新增可保存累计余数，并明确治疗后再次掉血是否可重复获益。
  2. **推荐**删除诗人随机元素弹和勇者每第二击 +50%，因为它们不在最新可见能力列；若要保留，必须由用户明确声明为该列之外的额外能力，并补入规范化表与说明。
- 相关文档同步范围：`ARCHITECTURE.md` 审阅并记录角色能力跨 Run/Combat 的状态流；`README.md` 增加/更新角色能力路由；四份相关模块文档更新权威、入口、数据和测试；数据使用说明与验收清单增加角色能力配表、同步和 PIE 方法。
- 关闭前逐项填写具名架构文档审阅结果。

## 锁定验收

- [ ] `策划数据源/【开普勒】回响数值与构筑体系.xlsx` 的四条可见角色能力均有逐字段的规范化生产表/CSV 映射和自动化证据；运行时不读中文描述。
- [ ] 智者在配置间隔 4 后只额外选择配置数量 1，额外选择本身不递归计数；修改测试 fixture 参数会改变结果，证明没有硬编码 4。
- [ ] 猎手实际移速为同基准角色的 120%，暴击率增加 20 个百分点、暴击效果增加 50 个百分点；晋升/开局两种进入方式不重复应用。
- [ ] 诗人每次成功完成合法关卡后永久增加 10 个百分点反应效能，失败、恢复初始化和重复结束回调不增加；保存恢复保持已增长值。
- [ ] 勇者只对最终生命减少按用户确认的 10% 阈值语义生成层数，每层物攻和元攻各 +1；格挡、无敌、治疗、最大生命变化、死亡/复活、存读档和跨关行为符合锁定语义。
- [ ] 勇者完成关卡后直接进入普通 `CardChoice`；生产目录、候选、UI、schema、测试和脚本中不存在可运行 `FORGE_LIGHT/MEDIUM/EXTREME` 或 `Character.BraveForge`。
- [ ] 从旧存档恢复 `ForgeChoice` 不会卡死、重复获益或崩溃；已获得的历史属性保持原值，后续只执行新能力。
- [ ] 用户确认处理后的诗人随机元素弹、勇者第二击加成不存在幽灵逻辑；四角色描述、属性面板和真实效果一致。
- [ ] XLSX→CSV `--check`、项目校验、Development Editor 构建、角色/Run/Combat/Cards/Save 聚焦自动化通过。
- [ ] 用户以新游戏分别验证四角色，确认 Forge 界面消失和四项能力体感/数值正确。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@55a15d1505f52fa182bfb13bb5b5e01770100520`；首次审计后的 Plan86 设置/暂停 UI 与 Plan88 卡牌掉落均已纳入规划基线。两者与本 Plan 文档无真实冲突；Plan88 与本 Plan 的未来实现共享 Cards/Run 热点，实施前重新审计。
- 引擎/构建可用性：Plan-only 发布只跑文档/项目静态校验；进入程序实现后按 `GIT_RULES.md` 在关闭 Editor 后构建最新 Development Editor，最终候选 FullRebuild 并提交精选预构建包。
- 现有聚焦测试结果：基线存在 `ReEcho.Characters.PromotionRoles`、`SageBonusCadence`、`BraveForge` 及 Run/Card/Save 自动化；实现前先跑角色聚焦基线，旧 `BraveForge` 测试只在新测试覆盖替代语义后删除。
- 共享契约 / 难合并资源风险：`Design/Data/ReEchoData.xlsx` 是 Exclusive 二进制真源；实施前再次 fetch 并确认无其他任务编辑角色/卡牌表、Run phase、Player health route 或 TraitCardChoice Widget。
- 基线损坏时的停止条件：角色或卡牌生产表漂移；远端修改相同 XLSX/Run/Player/UI 热点；用户未确认勇者层数语义和两项遗留能力处理；当前 Editor 未关闭而需构建/改二进制工作簿。

## 实现提纲

1. 用户审核现状反馈并确认勇者层数语义、两项未列出遗留能力；仅随后把 Plan 状态改为 `Ready/InProgress`。
2. 在 canonical `ReEchoData.xlsx` 建立规范化角色能力/效果 Table，更新 ExportMap、schema、manifest、生成器和负例；生成 CSV 并证明与可见策划源逐项一致。
3. 建立纯角色能力 Definition/Runtime；把智者、猎手、诗人行为迁入统一入口并删除按 RoleId/常量的旧分支。
4. 由 Player Host 只读订阅 Combat 最终 `HealthChanged`，调用 Run/角色能力窄命令实现勇者；同步 Run 永久状态与当前 Combat 属性时避免初始化递归和重复应用。
5. 删除 Forge phase/API/GameMode/UI 特化及生产卡/效果；加入旧存档 phase 迁移，清理精选 fixture、脚本和说明。
6. 删除用户确认废弃的诗人随机元素弹/勇者第二击能力，或将明确保留项规范化进同一能力表；补齐四角色、存档和跨模块回归。
7. 同步 CODEBASE_MAP 与数据文档，完成自动化、构建、用户 PIE，验收后再合并/发布实现。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Plan-only | `python scripts/validate_project.py`；`git diff --check` | Plan schema/编号/文本差异通过 |
| 数据 | `python scripts/data/sync_xlsx_to_csv.py --check`；数据单元测试 | workbook/CSV 零漂移，新角色能力 schema 和负例通过 |
| 静态 | `python scripts/validate_project.py` | Forge 精确禁用/删除约束、行为白名单、CSV 引用和源码不变量通过 |
| C++ | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0 |
| 聚焦自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Characters`；并回归 Run、Combat、Cards、Save、UI TraitChoice | 四能力、旧阶段迁移及跨领域行为通过 |
| 最终发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最新源码完整构建并刷新精选预构建包 |
| 人工 | 新游戏逐角色：移动/暴击、战后成长、四选一节拍、勇者受伤阈值；勇者战后不出现 Forge | 用户报告 `Passed` 或具名返工项 |

## 执行记录

### 变化

- 已在最新远端 `main` 建立 Plan87 独立工作树。
- 已只读审计指定策划源、canonical workbook、生产 CSV、CSV reader/registry、Run/Player/Combat/Cards/UI 调用链与角色聚焦测试；尚未清理 Forge，也未实现新能力。

### 证据

- 指定工作簿 `角色体系J` 可见范围为 `A1:D5`，生产能力单元格为 `D2:D5`，无隐藏生产行/列。
- 生产 `characters.csv` 的四行、`PassiveBehaviorId/PassiveValue` 和 StatBlock 与运行时代码逐项比对，得到本 Plan“角色能力映射”表。
- Forge 真实引用覆盖 Run phase/API、GameMode、TraitCardChoice Widget、3 张生产卡、9 条生产效果、fixtures、脚本与自动化，确认不能孤立删除。
- 首次审计期间远端从 `52b1648c` 前进至 `b8836ed1`（Plan86 文档与 Editor 预构建包），发布前又前进至 `55a15d15`（只新增 Plan88 文档）。本工作树已重放到最新提交；文档无真实冲突，未来 Cards/Run 实现热点已显式记录。

### 剩余风险

- 勇者血量阈值的“永久获得/随缺血变化”会决定保存结构、跨关与治疗语义，必须由用户确认。
- 可见角色能力列与生产表还存在两个未声明遗留行为；若不明确去留，最终仍会出现文字与实际玩法不一致。
- 移除 Forge 会改变旧存档阶段和 CardDomainRevision；必须以显式迁移和聚焦测试保护，不能仅依赖 CSV 缺行后的容错。

### 人工验收结果/请求

- `PendingBeforeClose`。实现前等待用户审核本 Plan，并确认两项“待用户审核决策”。

### 架构文档审阅结果

- 规划阶段已确认 `MOD-ReEcho`、`MOD-ReEchoCombat`、`MOD-ReEchoCards`、`MOD-ReEchoUI` 均受契约或事实影响；实现关闭前逐项更新并记录具体结果。
