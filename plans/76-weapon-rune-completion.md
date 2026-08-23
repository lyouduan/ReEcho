# Plan 76 - 程序 - 非隐藏武器符文完整实现

## 协调

- Planner 负责人：当前程序用户 + Codex（规划与执行合并）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划基线：`origin/main@73dd2b9738139d06997ab157a7014a3a5f329a4e`（包含 Plan81 的角色相对武器表现约束）。
- 本地实现基线：保留 Plan67 CodeBuddy 候选 `0f7c15f` 已形成的商店、数据表和图标工作，并在本 Plan 发布后合入最新 `origin/main`；不把 CodeBuddy 的 Plan76 半成品直接合入。最终组合提交在实现分支建立时回填。
- 本地实现方式：一任务一 worktree；Plan76 使用独立分支/worktree，Plan67 恢复分支只作组合基线和恢复点。
- 依赖 / 阻塞：Plan75 的三槽符文装备、`part_effects.csv` 解释器和武器选择契约；Plan67 的 XLSX/CSV/图标候选。正式实现提交因包含 Plan67 未发布候选，发布前必须同时完成 Plan67 集成审计。
- Writes:
  - `Design/Data/ReEchoData.xlsx`
  - `Content/Data/parts.csv`、`part_effects.csv`、`statuses.csv`、`csv_schema.csv`、manifest/hash 同步产物
  - `scripts/data/sync_xlsx_to_csv.py` 及聚焦数据校验（仅在契约需要时）
  - `Source/ReEcho/{Public,Private}/Data/`
  - `Source/ReEcho/{Public,Private}/Weapons/`
  - `Source/ReEcho/{Public,Private}/Graybox/`
  - `Source/ReEcho/{Public,Private}/Run/`
  - `Source/ReEcho/Private/Tests/`
  - `Source/ReEchoCombat/{Public,Private}/Combat/`
  - `Source/ReEchoCombat/{Public,Private}/AbilitySystem/`
  - `Source/ReEchoCombat/Private/Tests/`
  - `Source/ReEchoWeapons/{Public,Private}/Weapons/`
  - `Source/ReEchoWeapons/Private/Tests/`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`
  - 本 Plan。
- Stable Reads:
  - `策划数据源/【开普勒】回响数值与构筑体系.xlsx` 的 `武器符文C`、`状态Z`；按 Excel 隐藏行/列过滤。
  - `plans/67-shop-drop-table-overhaul.md`、Plan75 已发布契约。
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`MOD-ReEchoEnemies.md`。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：稳定 `WeaponId`、现有正式 `PartId`、三槽装备/存档和现有 15 个已实现符文保持兼容；占位 `P_AUDIT_C_*` 只对本 Plan 范围内可见行迁移为正式稳定 ID。Plan67 商店继续只消费 `Enabled && ShopEnabled && Implemented` 的正式符文。
- 明确排除：源表隐藏行 8-15、57-77；法杖与鞭符文；枪械源行 57；符文专用美术/VFX；Plan67 商店概率、刷新与槽位 UI 优化。

## 锁定目标

以程序说明为行为权威，完整实现策划源 `武器符文C` 中所有非隐藏符文：源行 2-56，排除隐藏行 8-15，共 47 个。保留已经实现的 15 个，并补齐 32 个，使每个符文均满足：可由 XLSX 配置、同步为 CSV、通过数据校验、可装备、运行时生效，并可由 Plan67 商店候选池获得。

补完范围固定为：弓 8 个、镰刀 9 个、长剑 7 个、枪 8 个。源行 57 虽曾被生产表误列为普通占位，但策划源实际隐藏，必须按退役数据处理，不得启用；法杖/鞭同理。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` / `AREA-Data` / `AREA-Run` / `AREA-Weapons`、`MOD-ReEchoCombat`、`MOD-ReEchoWeapons`。
- 对应模块文档：维护 `MOD-ReEcho.md`、`MOD-ReEchoCombat.md`、`MOD-ReEchoWeapons.md`，均已加入 Writes；审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEchoEnemies.md`。
- 设计意图：延续 Plan75 的 XLSX → CSV → 类型化编译链。静态装备修改继续由 `ApplyPartEffect` 编译；命中、暴击、击杀、攻击组、临时叠层、投射物和主动行为编译为资源无关效果规格，并在 Combat 最终结算结果之后由武器宿主适配层执行。禁止解析 Description、按具体 PartId 写行为分支或直接修改生成 CSV。
- 权威状态与依赖：
  - Combat 继续独占生命、状态、无敌和最终伤害；新增窄状态命令与临时属性修正接口。
  - ReEchoWeapons 继续独占攻击节拍、攻击身份和逻辑投射物；新增穿透所需的命中去重/继续飞行契约，不读取符文表。
  - 主模块 WeaponRuntime 负责把已校验 `part_effects` 编译为符文效果规格；WeaponActor 仅维护按 `AttackIdentity` 快照的瞬态执行上下文。
  - RunSubsystem 继续独占时间碎片；掉落实体只能通过窄授予命令提交，Actor 不直接写 `TimeShards`。
- 数据决策：
  - `parts` 中 32 个可见占位 ID 迁移为按武器/效果命名的正式稳定 PartId；对应图标、Effect 外键和测试一并迁移。
  - `part_effects` 的逻辑字段使用注册枚举/BehaviorId，并在 XLSX 中保持下拉选择；数值参数留作数值单元格。
  - `Z_Vertigo` 注册为 `Status.Stun`，默认 1 秒；`Z_Bleeding` 注册为 `Status.Bleeding`，默认 3 秒。流血每层每秒损失最大生命值 0.5%，每层拥有独立 3 秒期限。
- 确定性决策：20%/30% 概率由 `AttackIdentity.Sequence + PartId + 目标稳定索引 + 本次命中序号` 生成确定性抽样，不使用全局随机种子。
- 行为解释锁定：
  - “掉落时间碎片”生成可拾取的逻辑掉落实体；仅玩家来源可产出，Echo 不复制经济收益。
  - “攻击范围”修改 `RangeCm`，不修改攻击角度；“爆炸范围”修改 `ExplosionRadiusCm`。
  - “外圈”定义为本次有效攻击半径的外 50%，额外伤害倍率 +40%。
  - “攻击速度-500%”按攻击间隔增加 500%执行，即有效攻速乘 `1/6`；伤害倍率乘 `2.8`，避免负攻速或隐式 0.1 下限成为第二事实来源。
  - 同攻击命中阈值在首次达到时触发一次：4 人成长/无敌在第 4 个有效伤害结果触发，超过 6 人陨星在第 7 个触发。陨星不会递归触发自身阈值效果。
  - 临时叠层按各层独立 5 秒期限；“连击双速弓弦”最多 5 层，停止攻击 1 秒后每秒流失 1 层，并受 5 秒硬期限约束。
  - 分裂箭最多选择命中点附近 3 个其他有效目标，子箭伤害乘 0.5；子箭继承同一攻击的其他投射物/命中效果但不能再次分裂。
  - 暴击穿透在当前直击为暴击时继续飞行，同一投射物对每个目标最多结算一次；与爆炸组合时每次有效接触都可爆炸。
  - 镰刀投掷/召回走既有主动攻击输入：首次按键前飞最多 5m并停留；停留期间以 `1 / AttackSpeed` 秒为间隔造成 20%攻击力范围伤害；首次接触造成 60%攻击力伤害；再次按键召回。范围继承当前镰刀有效范围。
- 相关文档同步范围：实现后更新上述三个模块文档；仅当模块依赖/拓扑或索引入口变化时更新 `ARCHITECTURE.md` / `README.md`。
- 关闭前逐项填写审阅结果。

## 锁定补完清单

| 武器 | 源行 | 数量 | 补完行为 |
|---|---:|---:|---|
| Bow | 16-21、24-25 | 8 | 分裂、1m爆炸、暴击穿透、暴击流血、三连射、击杀碎片、击杀移速、连击双速 |
| Scythe | 26-27、29-35 | 9 | 群攻范围成长、命中吸血、20%晕眩、每目标3次命中流血、外圈增伤、命中移速、群攻无敌、命中攻速、投掷召回 |
| LongSword | 38、40-42、44-46 | 7 | 群攻范围成长、累计命中碎片、暴击流血、群攻陨星、命中移速、命中攻速、20%晕眩 |
| Gun | 49-56 | 8 | 极限蓄能、连发、1m爆炸、暴击穿透、30%命中流血、命中吸血、累计命中碎片、连射攻速 |

## 锁定验收

- [ ] 可见源行集合精确为 47；隐藏行 8-15、57-77 不会进入启用/商店/运行时集合。
- [ ] 47 个符文均拥有正式 PartId、有效 Effect 行、`Enabled=true`、`ImplementationStatus=Implemented`、`ShopEnabled=true`；生成 CSV 与 XLSX 字节级同步检查通过。
- [ ] 每个静态数值符文由数据驱动编译，装备/卸下/换武器重建不累乘、不残留。
- [ ] 每个动态符文存在聚焦自动化，覆盖单独行为及关键组合：分裂+爆炸、穿透+爆炸、同槽双符文、换装后的延迟投射物、Echo 经济隔离。
- [ ] 流血独立叠层/到期、晕眩行动禁止、临时属性叠层/到期、群攻阈值、陨星非递归、碎片拾取事务均有可观察证据。
- [ ] 弓/枪投射物路径和镰刀投掷召回使用当前世界目标与 Combat Resolver；不绕过最终伤害、阵营和击杀裁决。
- [ ] Plan67 商店候选统计能看到新增 32 个符文，但本 Plan 不修改其概率/UI规则。
- [ ] `.clang-format`、Development Editor 构建、项目校验、数据同步 `--check`、聚焦自动化和 `git diff --check` 通过。
- [ ] 用户在 PIE 手测四把武器的 32 个补完符文；未通过前保持 `PendingBeforeClose`。
- [ ] 未提交精选 `GIT_RULES.md` 允许列表之外的 UE 生成产物或机器本地文件。

## Step 0 门禁

- 基线分支/提交：Plan 文档从 `origin/main@73dd2b9` 发布；实现从“Plan67 CodeBuddy 保留成果 + 包含 Plan81 与本 Plan 的最新主线”组合恢复点建立独立分支。
- 引擎/构建可用性：待实现前确认 Editor 已关闭；先执行静态校验，最终执行 Development 构建与聚焦自动化。
- 现有聚焦测试结果：组合基线尚未生成可复用的最终构建证据；CodeBuddy Plan76 半成品验证失败，不作为证据。
- 共享契约 / 难合并资源风险：XLSX/CSV 必须成组集成；`WeaponActor`、`ProjectileLogic`、Combatant/状态契约与主分支 Plan77-80 表现改动耦合，表现路径必须保持只读。
- 基线损坏时的停止条件：组合基线若无法通过未修改语义的静态/现有聚焦测试，先隔离基线问题，不以削弱验收或复制旧逻辑绕过。

## 实现提纲

1. 建立可见行审计测试与正式 PartId/Effect/Status 数据，扩同步校验和逻辑字段下拉选项。
2. 扩 Combat 状态命令、独立流血栈、晕眩查询、临时 GAS 属性修正和运行时无敌窄接口。
3. 扩 WeaponRuntime 效果编译与 WeaponActor 按攻击身份的效果上下文，完成命中/暴击/击杀/攻击组/叠层/碎片/陨星/外圈机制。
4. 扩逻辑投射物穿透与主模块分裂子弹，完成弓/枪行为及组合测试。
5. 实现镰刀投掷/停留/召回逻辑 Actor，并接既有主动攻击输入。
6. 补齐数据、Runtime、Combat、Weapons、Run 与跨模块自动化；更新模块文档和执行记录。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| XLSX/CSV | `python scripts/data/sync_xlsx_to_csv.py --check` + 聚焦数据测试 | 可见行过滤、47/32计数、正式 ID、Effect/Status 枚举与生成文件一致 |
| 静态 | `python scripts/validate_project.py` | 项目、Schema、行为注册、源码/预构建不变量通过 |
| C++ 格式 | 对修改 `.h/.cpp` 运行仓库 `.clang-format` | 仅目标文件格式化，diff 无意外语义改写 |
| C++ 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development`；发布候选 `-FullRebuild` | UHT/UBT 成功，精选预构建包匹配最终源码 |
| Combat | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Combat` | 状态、流血、晕眩、临时属性、无敌通过 |
| Weapons | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Weapons` | 效果编译、投射物、32符文与组合通过 |
| 跨域 | 运行 Plan76 新增聚焦过滤并回归 Run/Save/AttackMode | 掉落事务、保存重建、输入/回放边界通过 |
| 文本 | `git diff --check` | 无空白/编码问题 |
| 人工 | PIE：弓、镰刀、长剑、枪逐个装备补完符文 | 用户记录通过或明确延期 |

## 执行记录

### 变化

- 2026-08-23：完成 CodeBuddy 审计；确认其中断补丁只开始长剑批次且存在数据真源、计数、确定性、状态和编译问题，不直接合入。
- 2026-08-23：保留 Plan67 CodeBuddy 候选并合入 `origin/main@df356c1`；二进制冲突临时采用主分支版本，等待最终重建。
- 2026-08-23：主线新增 Plan81；确认其只约束武器持握表现，不改变 Plan76 的攻击范围、伤害、投射物和符文语义，并纳入后续组合基线。
- 2026-08-23：按源工作簿实际隐藏行确认正式范围为 47 个，补完数为 32；生产表中的枪源行57误分类纳入本 Plan修正。

### 证据

- 只读工作簿审计：`武器符文C` 隐藏行为 8-15、57-77；非隐藏数据行 2-56 排除 8-15，共 47。
- CodeBuddy Plan76 工作树 `validate_project.py` 失败于未注册 `OnAttackGroupGrowth`，未提供构建/PIE证据。

### 剩余风险

- Plan67 与 Plan76 的实现候选组合发布，最终需同时审查 Plan67 商店语义和 Plan76 符文语义，避免将 WIP 提交直接推入 main。
- 符文手感、流血/叠层节奏和镰刀召回操作仍需用户 PIE。

### 人工验收结果/请求

- `PendingBeforeClose`：等待实现完成后的四武器逐符文 PIE。

### 架构文档审阅结果

- 待实现完成后填写。
