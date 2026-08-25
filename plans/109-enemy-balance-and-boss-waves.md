# Plan 109 - 程序 - 敌人数值调优与 Boss 三波契约

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`；策划已提供数值清单并报告 Encounter.1 可进入，最终仍需用户/策划在准确集成候选验收 Encounter.8 三波与整体难度。
- 本地规划 / 实现基线：`origin/main@dc46a89a80bc67c738952374ae971cde5b69ddd3`。
- 本地实现方式：`plan/109-balance-tuning-handoff`；`C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan109-balance-tuning`。
- 依赖 / 阻塞：依赖 Plan96 的羊 Boss 行为/VFX 继续保持 `WindupEnd` 锁定、Combat 命中和二阶段倍率权威；本 Plan 经当前程序用户确认，以策划新数值覆盖 Plan96 写死的旧伤害期望，但不改变其表现与命中语义。
- Writes:
  - `plans/109-enemy-balance-and-boss-waves.md`
  - `plans/96-sheep-boss-goat-vfx.md`（仅把旧固定伤害期望适配为权威配表及当前新值，保留最新锁定/VFX 决策）
  - `Design/Data/ReEchoData.xlsx`
  - `Design/Data/ReEchoEncounterData.xlsx`
  - `Design/Data/ReEchoEnemyData.xlsx`
  - `Content/Data/{enemies,enemy_combat_stats,enemy_abilities,encounters,encounter_waves,enemy_shard_drops}.csv`
  - `scripts/validate_project.py`
  - `scripts/data/` 下本 Plan 新增或修改的 Encounter.8 三波聚焦测试
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - 最终 FullRebuild 刷新的 `Binaries/Win64/` 精选预构建包
- Stable Reads:
  - `scripts/data/sync_xlsx_to_csv.py`
  - `Content/Data/{csv_schema,reecho_data_manifest,spawn_profiles,spawn_policy,boss_phases}.csv`
  - `Source/ReEcho/{Public,Private}/Encounter/ReEchoEncounterRuntime.*`
  - `Source/ReEcho/{Public,Private}/Data/ReEchoCsvDataRegistry.*`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `plans/{44-boss-gameplay-and-enemy-xlsx-tables,68-reecho-enemy-align-authoritative,96-sheep-boss-goat-vfx}.md`
  - `shared/CODEBASE_MAP/{ARCHITECTURE,README}.md`
- 影响模式：`SharedContract`。同时修改三个策划真源工作簿、生成 CSV、Encounter.8 结构校验和 Plan96 的配表验收期望；与并行表格、Boss/VFX 和最终预构建包工作存在集成耦合。
- 兼容承诺 / 下游操作：保持稳定 Enemy/Encounter/Ability ID、CSV Schema、四工作簿所有权和生成链不变；保留 `M_SHEEP_PrayerBeam.LockTiming=WindupEnd`、BossOrPlayerDeath、二阶段 `1.5` 伤害倍率、Combat 命中权威与 Plan96 表现映射。`ReEchoData.xlsx` 中 Plan105 已禁用的两枚符文必须原样保留。
- 明确排除：不手改生成 CSV 形成第二真源；不整块覆盖策划旧基线工作簿；不改变 C++ 波次调度、Boss AI/VFX、CSV Schema、稳定 ID、存档或其他卡牌/武器/符文数值；不把“狐狸=兔子×1.3 向上取整”等策划公式复制为运行时代码。

## 锁定目标

1. 在最新三个权威工作簿中落实策划交接清单的敌人基准、成长、技能、关卡单位上限、波次数量和时间碎片掉落数值，并由现有同步器生成六份准确 CSV；保留最新主线中与本任务无关的工作簿变化。
2. `Encounter.8` 正式采用三波：`WaveIndex=1/2/3`、`TriggerSeconds=0/10/20`；只有 Wave.1 的 `BossEnemyId=M_SHEEP`，Wave.2/3 只提供增援且不得重复生成 Boss。
3. `validate_project.py` 不再要求 Boss 关单波，改为验证上述精确三波结构、唯一 Boss ID 与时序；错误的缺波、重复 Boss、错索引或错触发时刻必须被拒绝并给出可定位信息。
4. Plan96 保持最新 `MoonStaff`、Skill01~04 表现、`WindupEnd` 锁定和 Combat 命中契约，但伤害验收改为消费权威 `enemy_abilities.csv`。当前一阶段 Skill02/03/04 分别为 `4/24/16`，二阶段经既有 `1.5` 倍率为 `6/36/24`。
5. 证明现有 `FReEchoEncounterWaveScheduler` 已按任意配置波逐行生成事件，无需为 Boss 三波复制 C++ 特例；若实际验证发现运行时无法生成 Wave.2/3，则停止并按范围扩张门禁回报，不暗中修改 C++。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` 的 `AREA-Data`、`AREA-Encounter`、`AREA-Tests`；Plan96 的 Boss/VFX 消费契约受数值期望影响，但 `MOD-ReEchoEnemies`、`MOD-ReEchoVFX` 与 Combat 公共契约不变。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，已加入 Writes；`MOD-ReEchoEnemies.md`、`MOD-ReEchoVFX.md` 仅在关闭前审阅，因为状态所有者、事件和依赖方向预计不变。
- 设计意图：让策划真源同时拥有平衡值和 Boss 波次配置，生成 CSV 是唯一运行时来源；校验器保护结构不变量但不阻止已批准的三波内容。
- 权威状态与依赖：状态所有者不变。三个 XLSX 继续分别拥有主数值、Encounter 与 Enemy 数据；同步器发布 CSV；Registry 编译快照；Encounter scheduler 消费每一行配置并产生确定性事件；GameMode 只消费事件和 `BossOrPlayerDeath`。
- 决策记录：
  1. 不导入基于 `7972288` 的旧二进制工作簿，而在最新 `dc46a89a` 上逐表重建差异，避免丢失 Plan104/105 与其后的主线编辑。
  2. Encounter.8 与普通关一样使用 `0/10/20` 三个确定性门，但只允许第一波携带 Boss；该结构直接复用通用 scheduler，不增加 Boss 专用 C++ 分支。
  3. Plan96 的表现测试应查询配表并验证最终伤害，不再把平衡数值复制进 VFX 契约；Plan 中记录当前值仅用于本轮可审计验收。
  4. 工作簿修改使用现有格式、表结构、保护和可见视图；隐藏行/列不参与读取或决策，输出仍由仓库同步器完成。
- 相关文档同步范围：必审 `ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoEnemies.md`、`MOD-ReEchoVFX.md`；预计只需在 `MOD-ReEcho.md` 补充 Encounter.8 三波结构，拓扑、索引和 Enemy/VFX 状态边界不变时仅在执行记录说明。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅拓扑与依赖方向；
  - `README.md`：待审阅 AREA 路由；
  - `MOD-ReEcho.md`：待同步 Encounter.8 三波结构与数据校验事实；
  - `MOD-ReEchoEnemies.md`：待审阅 Enemy/Boss 数值消费边界；
  - `MOD-ReEchoVFX.md`：待审阅 Plan96 配表伤害与表现解耦边界。

## 锁定验收

- [ ] 三份 XLSX 保持原格式、保护、表结构和最新非本任务内容；目标表可见值与策划交接逐项一致，六份生成 CSV 与工作簿字节一致。
- [ ] 敌人基准/成长/技能、ActiveUnitLimit、全部波次数量、Encounter.8 三波和碎片掉落区间与交接清单一致；狐狸基准与成长满足策划给出的向上取整结果。
- [ ] Encounter.8 校验接受唯一合法三波并拒绝缺波、错时刻、错索引、非首波 Boss 或重复 Boss；错误包含文件/行/字段上下文。
- [ ] 运行时调度静态审计和聚焦证据证明 Wave.2/3 会进入通用警告/提交事件，Boss 仅生成一次；不引入 C++ Boss 特例。
- [ ] Plan96 保留最新 `WindupEnd` 与 VFX 语义，伤害期望对齐新配表及二阶段倍率，不再继续锁死旧值。
- [ ] `sync_xlsx_to_csv.py --check`、聚焦 Python 测试、`validate_project.py`、最终 `-FullRebuild`、预构建检查和 `git diff --check` 通过。
- [ ] 用户/策划在准确最终候选中验收 Encounter.8 三波、Boss 不重复及整体数值体验；未提交允许列表外 UE 生成物或机器本地文件。

## Step 0 门禁

- 基线分支/提交：`origin/main@dc46a89a80bc67c738952374ae971cde5b69ddd3`；已吸收 Plan96 最新光束锁定时机和 Plan105 工作簿符文调整。
- 引擎/构建可用性：UE 5.8 可用；最终程序发布仍需用户关闭 Editor、取得 Git common-dir Unreal 锁并执行 `Development -FullRebuild`。
- 现有聚焦测试结果：策划报告旧候选 CSV 可加载且 Encounter.1 可进入，但该证据来自 `7972288` 派生基线，不作为本 Plan 最终证据；当前主线校验会按预期拒绝 Boss 三波。
- 共享契约 / 难合并资源风险：`ReEchoData.xlsx`、Plan96、`MOD-ReEcho.md` 和精选预构建包均刚被并行任务修改；本 Plan 已选择组合适配，发布前再次 fetch，任何新增同行编辑都重新审计。
- 基线损坏时的停止条件：最新工作簿无法被 artifact-tool 无损导入/导出、保护/表/隐藏视图丢失、同步器生成非目标 CSV 差异、三波运行时需要 C++ 新语义、Plan96 又改变数值或锁定契约，或最终构建/校验出现非本任务回归时停止并报告。

## 实现提纲

1. 用 artifact-tool 导入并渲染三份最新工作簿，检查目标表的可见行列、表范围、格式、公式和保护；建立交接值到精确行列的审计映射。
2. 逐格写入 `tblEnemyShardDrops`、`tblEnemies`、`tblEnemyCombatStats`、`tblEnemyAbilities`、`tblEncounters` 和 `tblEncounterWaves`；按现有表样式插入 Encounter.8 Wave.2/3，保留所有非目标值和格式。
3. 由 `sync_xlsx_to_csv.py` 生成六份生产 CSV，确认没有额外 CSV 漂移；逐项比较生成值与交接清单。
4. 把 Encounter.8 校验改为精确三波契约并增加合法/非法聚焦测试；把 Plan96 的旧固定伤害适配为配表驱动，维护 `MOD-ReEcho.md`。
5. 执行工作簿关键范围/公式错误扫描与全 sheet 渲染、同步器测试、项目校验和 diff 审计；在最终集成基线上完成 FullRebuild/预构建检查，并请求人工验收后关闭与发布。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 工作簿 | artifact-tool 检查目标范围、公式/错误并渲染所有受影响 sheet | 值、样式、保护、表结构、隐藏视图与原工作簿一致；无公式错误或内容裁切 |
| 数据同步 | `python scripts/data/sync_xlsx_to_csv.py --check` | 四工作簿与全部生产 CSV 一致，只有六份目标 CSV发生语义变化 |
| Python 聚焦 | `python -m unittest` 运行本 Plan Encounter 三波与既有同步器聚焦测试 | 合法三波通过，非法结构被拒绝；同步器保护/生成契约不回归 |
| 静态 | `python scripts/validate_project.py`；`git diff --check` | Schema、ID、引用、平衡域、Encounter 和项目不变量通过 |
| 运行时审计 | `FReEchoEncounterWaveScheduler` 代码与现有/新增聚焦证据 | 三波逐行调度、0/10/20 时序、Boss 仅 Wave.1 一次 |
| 发布构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`；`python scripts/ue/prebuilt_editor.py check` | 最新组合候选 UHT/UBT 成功，精选预构建包与源码/内容匹配 |
| 人工 | Encounter.8 进入后观察 0/10/20 秒波次、Boss 唯一性与整体难度 | 用户/策划确认三波、数值与体验可接受 |

## 执行记录

### 变化

- 2026-08-25：收到策划《ReEcho 数值调优交接清单》；确认其旧分支未在当前克隆出现，旧基线落后主线 26 个提交，因此决定在最新真源上重建差异而非覆盖二进制工作簿。
- 2026-08-25：程序用户确认采用组合适配：保留 Plan96 最新光束锁定与 Plan105 符文改动，接受策划 Boss 新伤害，并把 Encounter.8 三波固化为正式校验契约。
- 2026-08-25：静态审计确认 `FReEchoEncounterWaveScheduler::Configure` 遍历 Encounter 的所有 Wave，按每行生成普通角色事件并仅在 `BossEnemyId` 非空时生成 Boss；预期无需 C++ 特例。

### 证据

- 待实现。

### 剩余风险

- `PendingBeforeClose`：最终组合候选尚未进行 Encounter.8 人工验收。

### 人工验收结果/请求

- 待最终候选。

### 架构文档审阅结果

- 待实现关闭前填写。
