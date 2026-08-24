# Plan 92 - 程序 - 怪物逐关时间碎片掉落配置

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：`origin/main@87f675b71be5f6027a9828e064555ce00357a950`。
- 本地实现方式（可选，仅作交接说明）：独立 worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan92-enemy-shard-drops`，分支 `plan/92-enemy-shard-drops`。
- 依赖 / 阻塞：依赖现有 `TimeShards` 余额与 `GrantTimeShards` 事务、Plan88 已实现的逐关投放流程、Cards 的 `NoEnemyShardDrops` 与 `BonusShardDropEncounterIndex` 规则、现有敌人稳定 `EnemyId + SpawnIndex + Archetype`。策划源表没有 Boss 掉落配置，首版 Boss 不产生本表奖励。
- Writes:
  - `plans/92-enemy-time-shard-drops.md`
  - `Design/Data/ReEchoData.xlsx`
  - `Design/Data/ReEchoData使用说明.md`
  - `Design/Data/ReEchoData策划验收清单.md`
  - `Design/Data/ReEchoEnemyData.xlsx`
  - `Content/Data/enemy_shard_drops.csv`
  - `Content/Data/enemies.csv`
  - `Content/Data/csv_schema.csv`
  - `Content/Data/reecho_data_manifest.csv`
  - `Content/Data/README.md`
  - `scripts/data/sync_xlsx_to_csv.py`
  - `scripts/data/test_sync_xlsx_to_csv.py`
  - `scripts/validate_project.py`
  - `Source/ReEcho/Public/Data/ReEchoCsvDataRegistry.h`
  - `Source/ReEcho/Private/Data/ReEchoCsvDataRegistry.cpp`
  - `Source/ReEcho/Private/Data/ReEchoCsvDataReader.cpp`
  - `Source/ReEcho/Private/Data/ReEchoEnemyCsvReader.cpp`
  - `Source/ReEcho/Private/Data/ReEchoEnemyDefinitionCompiler.cpp`
  - `Source/ReEcho/Public/Run/ReEchoRunSaveGame.h`
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Source/ReEcho/Public/ReEchoGameMode.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Public/Graybox/ReEchoEnemyActor.h`
  - `Source/ReEcho/Private/Tests/ReEchoEnemyShardDropTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoSaveGameTests.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`
  - `Binaries/Win64/` 下 `GIT_RULES.md` 允许的最终精选预构建包
- Stable Reads:
  - `策划数据源/【开普勒】回响数值与构筑体系.xlsx` 的 Excel 可见区域 `投放系统!A38:D47`
  - `Content/Data/encounters.csv`
  - `Content/Data/encounter_waves.csv`
  - `Source/ReEcho/Public/Graybox/ReEchoEnemyActor.h`
  - `Source/ReEcho/Private/Graybox/ReEchoEnemyActor.cpp`
  - `Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`
  - `Source/ReEchoCards/{Public,Private}/Cards/ReEchoCardRuntime.*`
  - `plans/88-card-drop-system.md`
- 影响模式：`SharedContract`（`Isolated | ReadOnly | SharedContract | Exclusive`，仅说明数据、Run 与敌人死亡接线的集成影响）。
- 兼容承诺 / 下游操作：保留 `TimeShards` 余额、商店消费、卡牌禁掉落/下一关 1.5 倍语义及武器符文拾取物入口；旧存档可读取，新增掉落随机种子必须可确定迁移。删除未消费的 `Enemies.Reward` 和每场固定 `15` 奖励，避免三份奖励权威并存。
- 明确排除：不调整商店价格、卡牌文本/数值、武器符文奖励、Boss 奖励、时间碎片视觉资产或拾取表现；不把掉落区间硬编码到 C++/JSON/Widget；不修改原始 `策划数据源` 工作簿；不让 Echo/攻击载体自行发放基础怪物奖励。

## 锁定目标

把策划源 `投放系统!A38:D47` 的可见“怪物掉落时间碎片配置”迁入生产 XLSX/CSV，并让每只非 Boss 敌人在死亡时按照死亡发生时的真实 `EncounterIndex` 与敌人类别获得一次随机时间碎片奖励：

| 关次 | 近战 | 远程 | 精英 |
|---|---:|---:|---:|
| 1 | 2–3 | 4–6 | 无 |
| 2 | 2–3 | 4–6 | 无 |
| 3 | 2–3 | 4–6 | 10–13 |
| 4 | 2–3 | 4–6 | 10–13 |
| 5 | 2–3 | 7–9 | 15–20 |
| 6 | 2–3 | 7–9 | 15–20 |
| 7 | 2–3 | 7–9 | 15–20 |
| 8 | 2–3 | 7–9 | 15–20 |

- 类别按运行时 `Archetype` 统一解析：`Ranged` 为远程、`Elite` 为精英、`Boss` 不参与，其余非 Boss Archetype 为近战。该映射集中在单一解析函数，不由各敌人 Host 分别判断。
- 表中 `/` 迁移为缺省区间；缺区间时奖励为 0。配置行缺失、区间只有一端、最小值大于最大值、负值或重复关次必须在同步/校验阶段报错，运行时不得猜默认值。
- 奖励在敌人最终死亡事件上只结算一次并直接进入 Run 的 `TimeShards` 余额；首版不生成新的世界拾取物或 VFX，避免 20 秒拾取物过期改变表中经济预期。表现需求另立窄 Plan。
- 同 Stage 留存的旧敌人在后续小关死亡时，按死亡时的当前关次结算，不按出生关次结算。
- 同一 Run、关次、稳定 `SpawnIndex` 与类别应得到相同区间结果；保存/继续或相同死亡事件的重复通知不得重摇或重复入账。不同新 Run 允许得到不同结果。
- `NoEnemyShardDrops` 生效时本次基础掉落为 0；`BonusShardDropEncounterIndex` 命中当前关时，对每只敌人的基础整数掉落乘 `1.5` 并沿用当前 `FMath::RoundToInt` 取整语义。关末只清除一次性标记，不再固定发放 15。
- 敌人由玩家或 Echo 击杀都属于同一只敌人的基础死亡掉落，只结算一次；武器符文的额外掉落继续只按其既有攻击来源规则执行，不与基础掉落合并成第二判断。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoCards`、`MOD-ReEchoEnemies`；`AREA-Data`、`AREA-Run`、`AREA-Encounter`、`AREA-Enemies`、`AREA-Tests`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，并具名审阅/必要时维护 `MOD-ReEchoCards.md` 与 `MOD-ReEchoEnemies.md`；三者均已加入 `Writes`。
- 设计意图：以逐关生产表作为基础怪物经济奖励的唯一权威，让 Enemy 继续只拥有逻辑/死亡状态，Run 继续拥有货币和卡牌经济规则，GameMode 只负责把类型化敌人死亡事实路由给 Run。
- 权威状态与依赖：
  - `Design/Data/ReEchoData.xlsx/经济系统/tblEnemyShardDrops` → `enemy_shard_drops.csv` → `FReEchoCsvDataSnapshot` 是区间权威。
  - `UReEchoRunSubsystem` 是余额、一次性卡牌经济规则、每 Run 随机种子和原子发放权威。
  - `ReEchoEnemies` 继续拥有敌人 Archetype、存活状态和死亡事件，不读取 Run 或 XLSX，也不直接修改货币。
  - `AReEchoGameMode` 只从已注册敌人的类型化 `OnDeath` 接线提交 `EnemyId/SpawnIndex`；接线必须集中，出生与读档恢复不得漏绑或重复绑定。
- 决策记录：
  1. 生产表采用每关一行的宽表字段 `EncounterIndex, MeleeMin, MeleeMax, RangedMin, RangedMax, EliteMin, EliteMax`，最接近策划原表，减少策划编辑和审阅成本；运行时再类型化解析类别。
  2. 表放入主生产工作簿 `经济系统`，而不是复制到怪物定义表；原因是它随关次变化且控制 Run 经济，不是单个 EnemyDefinition 的固有属性。
  3. 删除 `Enemies.Reward` 字段及读取结构；该字段当前虽存在于敌人 XLSX/CSV，但运行时没有消费且无法表达逐关区间，保留会制造第二事实来源。
  4. 删除 `CompleteEncounter` 的固定 `15`，卡牌禁掉落与 1.5 倍改为死亡事务中的修正；关末仍负责清除下一关一次性标记。
  5. 增加独立持久化的 `EnemyShardDropSeed`，新 Run 生成非零种子；旧存档从已有稳定 Run 数据加固定 salt 确定迁移并提升 SaveVersion。不得借用 UI/商店刷新序列或全局时间随机。
  6. 首版直接入账而非复用 `AReEchoTimeShardPickupActor`；后者是武器符文专用、会过期且没有生产掉落表现，直接复用会暗中改变经济结果。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：关闭前审阅；预期不改变 Runtime Module 拓扑或依赖方向。
  - `shared/CODEBASE_MAP/README.md`：关闭前审阅；预期不新增稳定架构标识或阅读路由。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：更新 `AREA-Data` 与 `AREA-Run` 的逐关怪物掉落权威、随机性和死亡路由。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`：审阅 `NoEnemyShardDrops` 与 `BonusShardDropEncounterIndex` 的消费边界；事实变化时同步。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`：审阅 Enemy 只发布死亡事实、不拥有货币的边界；公共死亡契约变化时同步。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅。
  - `README.md`：待审阅。
  - `MOD-ReEcho.md`：待维护。
  - `MOD-ReEchoCards.md`：待审阅/维护。
  - `MOD-ReEchoEnemies.md`：待审阅/维护。

## 锁定验收

- [x] 策划源可见 `投放系统!A38:D47` 精确迁移到生产 `tblEnemyShardDrops` 与 `enemy_shard_drops.csv`；隐藏行/列不参与迁移。
- [x] 自动化逐行覆盖第 1–8 关的近战、远程、精英区间，证明结果始终落在闭区间内；第 1–2 关精英与 Boss 始终为 0。
- [x] 同一 Run/关次/SpawnIndex 结果确定，换新 Run 允许变化；保存恢复不重摇，重复死亡通知不重复发放。
- [x] 同 Stage 残留敌人在下一小关死亡时使用新的当前关次配置。
- [x] 玩家与 Echo 击杀均触发同一基础掉落且每敌人最多一次；失败遭遇已获得余额遵循现有 Run 存档/回滚语义，不建立私有补偿账本。
- [x] `NoEnemyShardDrops` 令基础掉落为 0；下一关 1.5 倍逐只生效并按既有取整规则计算，关末清除一次性标记。
- [x] 旧 `Enemies.Reward` 与每场固定 `15` 已删除，运行时、Schema、测试与说明中不存在仍可生效的第二基础奖励路径。
- [x] 旧 SaveVersion 可确定迁移，新版本往返保持掉落种子与余额；坏表/坏区间在同步或启动校验时报出定位信息。
- [ ] `scripts\data\sync_xlsx_to_csv.py --check`、聚焦 Python 测试、`ReEcho.Data`/`ReEcho.Run`/新掉落自动化、最终 FullRebuild、项目校验、预构建检查与 `git diff --check` 通过。（除主干已有的 3 个陈旧 `ReEcho.Data` 断言外均通过，详见执行记录。）
- [ ] 用户在 PIE 分别击杀近战、远程、精英并核对余额增量；确认未再收到每关固定 15。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`plan/92-enemy-shard-drops`，批准基线 `origin/main@87f675b71be5f6027a9828e064555ce00357a950`。
- 引擎/构建可用性：UE 5.8 安装版；Plan-only 发布走纯文档例外，执行候选在最终集成基线上必须 `-FullRebuild`。
- 现有聚焦测试结果：只读审计确认现有 `CompleteEncounter` 固定发 15、`Enemies.Reward` 只解析不消费、`GrantTimeShards` 与 Cards 经济规则可复用；执行者需在修改前记录聚焦基线。
- 共享契约 / 难合并资源风险：`Design/Data/ReEchoData.xlsx`、`Design/Data/ReEchoEnemyData.xlsx` 是二进制权威表，必须与生成 CSV 成组发布；`ReEchoRunSubsystem.cpp`、`ReEchoGameMode.cpp` 与 Plan88/商店和阶段流程高度耦合。推送前远端若前进，必须重新审计表格语义、死亡接线和卡牌经济规则。
- 基线损坏时的停止条件：策划源可见区域与本 Plan 矩阵不一致；远端新增修改同一生产 XLSX/CSV、Run 奖励、Enemy death 或 Card economy；实现需要改变 Boss 奖励、拾取表现或失败遭遇回滚语义。命中时停止越界部分并请用户决策。

## 实现提纲

1. 在生产 `ReEchoData.xlsx/经济系统` 新增 `tblEnemyShardDrops`，同步导出、Schema、manifest、说明和校验；从敌人生产表删除旧 `Reward` 列及 C++ 解析字段。
2. 在 Data Registry 增加逐关掉落行和严格解析/查询接口，集中实现 Archetype → 掉落类别映射与区间选择，禁止缺表默认。
3. 在 Run 增加持久化 `EnemyShardDropSeed` 与“提交一次敌人死亡奖励”的窄事务；先应用禁掉落/1.5 倍规则，再原子更新余额，重复键无副作用。
4. 在 GameMode 的统一敌人装配路径绑定死亡事件，把当前 Encounter、EnemyId 和 SpawnIndex 交给 Run；出生与读档恢复复用同一绑定，Enemy/Combat 不访问货币。
5. 删除关末固定 15，仅保留下一关加成标记清理；确保同 Stage 残留敌人按死亡关次处理。
6. 增加数据、区间、确定性、存档迁移、幂等、类别、跨关、卡牌修正和固定奖励删除回归测试。
7. 更新相关模块文档并完成关闭前具名审阅；最终组合候选执行 FullRebuild 后交由用户 PIE 验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 策划真源 | 可见行审计 + `scripts\data\sync_xlsx_to_csv.py --check` | 原表 8 行与生产 XLSX/CSV 字节一致，隐藏内容未导出 |
| 数据工具 | `python -m unittest scripts.data.test_sync_xlsx_to_csv` | 新表、缺列、半区间、逆区间、重复关次与旧 Reward 删除契约通过 |
| 格式 | 对修改的 `.h/.cpp` 运行仓库 `.clang-format` | diff 无无关格式噪声 |
| 聚焦自动化 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Data`、`-Filter ReEcho.Run`、`-Filter ReEcho.Economy.EnemyShardDrops` | 数据解析、逐关矩阵、死亡幂等、跨关、卡牌修正与存档迁移通过 |
| 最终构建 | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新准确精选预构建包 |
| 静态/预构建 | `python scripts\validate_project.py`、`python scripts\ue\prebuilt_editor.py check`、`git diff --check` | 项目、生产数据与预构建不变量通过 |
| 人工 PIE | 在具名关次击杀近战/远程/精英并观察余额 | 增量符合区间、无固定 15、禁掉落/1.5 倍行为符合预期 |

## 执行记录

### 变化

- 2026-08-24：Plan92 已发布到 `origin/main@513ec51b`；用户指示开始实现，复用独立 worktree 并将状态改为 `InProgress`。
- 2026-08-24：只读审计策划源 Excel 可见区域 `投放系统!A38:D47`，8 行数值与用户提供矩阵一致，无隐藏行/列。
- 2026-08-24：确认生产 `ReEchoData.xlsx` 尚无对应表/CSV；`enemies.csv/Reward` 仅被解析、未被 Definition 或运行时消费；当前实际基础奖励为 `CompleteEncounter` 固定 `15`。
- 2026-08-24：确认现有 `TimeShards`、`GrantTimeShards`、卡牌禁掉落/下一关 1.5 倍与稳定敌人 `SpawnIndex` 可复用；Plan92 选择逐敌死亡直接入账并以持久 Run seed 保证确定性。
- 2026-08-24：新增 `经济系统/tblEnemyShardDrops` 与生成 CSV，删除旧敌人 `Reward` 列；Python 同步器、Schema、manifest 与中英文使用说明同步维护。
- 2026-08-24：Run 新增 v13 `EnemyShardDropSeed` 与已处理死亡键；GameMode 的出生/恢复共用装配函数订阅 Combat 最终死亡，按当前 Encounter 和 Archetype 逐只原子入账；`CompleteEncounter` 不再固定发 15。
- 2026-08-24：新增数据矩阵、区间、类别、禁掉落、1.5 倍、幂等和存档往返自动化，并更新商店货币接缝测试。

### 证据

- 首次 `git fetch --prune origin` 时本地与 `origin/main` 同为 `f8e8a40b`，远端最大编号为 Plan90；推送门禁随后发现远端已发布 `87f675b7` 的商店槽 Plan91。经用户确认，远端编号优先，本任务整体后移为 Plan92 并变基到 `origin/main@87f675b7`。
- Excel 可见行审计：`投放系统!38:47` 均为可见，列 A:D 无隐藏列；数值为本 Plan 锁定矩阵。
- `rg`/源码审计：`FReEchoCsvEnemyRow::Reward` 只有 CSV Reader 写入，无消费方；`UReEchoRunSubsystem::CompleteEncounter` 固定增加 `RoundToInt(15 * multiplier)`。
- `python scripts/data/test_sync_xlsx_to_csv.py`：18/18 通过；`sync_xlsx_to_csv.py --check` 通过。
- `ReEcho.Run`：16/16 通过；`ReEcho.Run.EnemyShardDrops`：2/2 通过；`ReEcho.Shop.PostDrawCurrencyCanPurchase`：1/1 通过。
- Development FullRebuild 成功；`validate_project.py`、prebuilt check、`git diff --check` 通过。
- `ReEcho.Data` 的 6 项中 3 项失败，均可由未修改的任务基线数据复现：测试仍期待 39 张可抽卡（基线为 37）、Rabbit 二形态关闭（基线为开启）、60 条未命名禁用配件（基线为 0）；本 Plan 不越界改写这些断言。

### 剩余风险

- 原始表没有明确 Boss、世界拾取表现或失败遭遇回滚语义；本 Plan 锁定 Boss=0、直接入账、沿用现有 Run 存档语义。若产品要改为可拾取实体或 Boss 奖励，需用户更新锁定目标。
- 两个生产 XLSX 和 Run/GameMode 是近期共享热点；执行或发布前远端前进会使本次二进制表审计与构建证据失效。

### 人工验收结果/请求

- `PendingBeforeClose`：实现完成后由用户在 PIE 验证实际余额增量与关末不再固定发 15。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅；未改变 Runtime Module 拓扑或依赖方向，无需修改。
- `shared/CODEBASE_MAP/README.md`：已审阅；未增加稳定架构标识或阅读路由，无需修改。
- `MOD-ReEcho.md`：已维护逐关基础敌人奖励权威、Run 随机/幂等与 GameMode 死亡路由。
- `MOD-ReEchoCards.md`：已维护禁掉落/下一关 1.5 倍只声明规则、Run 唯一消费的边界。
- `MOD-ReEchoEnemies.md`：已维护 Enemy 只发布死亡事实、不持有区间/随机种子/余额的边界。
