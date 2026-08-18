# Plan 48 - 程序 - 开普勒关卡、怪物配表与八场分波运行时

## 协调

- Planner 负责人：Codex。
- Executor 负责人：Codex（同一 AI 规划与执行）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`，由用户在 PIE 中验收八场推进、三波刷新、跨战斗留存、刷怪落点、普通怪行为和 Boss 衔接。
- 本地规划 / 实现基线：初始为 `origin/main@86bf9a482a284d362f9e94bdcd5ba449bc44d795`；当前组合基线为 `origin/main@39136cdb30a607e4beca949cdc6ac8f5bb91001a`。
- 本地实现方式：一任务一 worktree；Plan 发布后创建 `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan48`，本地分支 `plan/48-kepler-encounter-enemy-data-runtime`，不在主工作区实现。
- 依赖 / 阻塞：依赖已关闭的 Plan25、29-31、41、43、44 所建立的 XLSX→CSV、Echo、Combat、Enemies、Boss 与保存契约。Plan47 已发布且正在重构 Cards，并声明可能修改 `ReEchoData.xlsx`、数据生成器、Data/Run/Encounter/Enemies 适配、存档和模块文档；本 Plan 允许本地并行，但最终集成必须以届时 `origin/main` 为基线组合适配，不得整块覆盖 Plan47 的 Schema、存档或模块契约。Plan42 的正式 Editor 场景资产不作为本 Plan 前置条件。
- Writes：本 Plan；新增 `Design/Data/ReEchoEncounterData.xlsx`、使用说明与策划验收清单；维护 `Design/Data/ReEchoEnemyData.xlsx`、对应使用说明与验收清单；新增/生成 `Content/Data/stages.csv`、`encounters.csv`、`encounter_waves.csv`、`spawn_profiles.csv`、`spawn_policy.csv`，以及成组维护的 `enemies.csv`、`enemy_abilities.csv`、`csv_schema.csv`、`reecho_data_manifest.csv`、`Content/Data/README.md`；`scripts/data/sync_xlsx_to_csv.py` 与聚焦 Python 测试；`Source/ReEcho/{Public,Private}/Data/**`、`Encounter/**`、`Run/**`、`Recording/**`、`Graybox/ReEchoEnemyActor.*`、`Graybox/ReEchoEchoActor.*`、`ReEchoGameMode.*`、远端场景适配导致的 `Player/ReEchoPlayerPawn.*`、`Weapons/ReEchoWeaponActor.*`、投射物/光波适配与对应测试；`Source/ReEchoCombat/**`、`Source/ReEchoWeapons/**`、`Source/ReEchoEnemies/**` 与对应测试；必要的 `Config/DefaultGame.ini`、`ReEcho.uproject` / Build.cs 依赖；`shared/CODEBASE_MAP/{ARCHITECTURE.md,README.md,modules/MOD-ReEcho.md,modules/MOD-ReEchoCombat.md,modules/MOD-ReEchoEnemies.md,modules/MOD-ReEchoWeapons.md}`；最终 `GIT_RULES.md` 允许的 Win64 Editor 预构建包。
- Stable Reads：外部策划源 `C:\Users\gavynqiu\Documents\miniGame\【开普勒】回响配置表.xlsx`（确认时 SHA-256 `A2499C983574B1CFCC8A4EF4223E6F166E791A0662D08C99E516CF7F941C7B80`）；外部说明书 `C:\Users\gavynqiu\Documents\miniGame\时间回响_Demo关卡与怪物设计说明书_v1.0(1) (1).docx`（确认时 SHA-256 `4C47014779D8E11FE32DF7939F30152CD5E109F38DDD2E7F78BD62BE630C4769`）；`MOD-ReEchoCombat` 的命中/伤害公共契约；`MOD-ReEchoWeapons`；`MOD-ReEchoAudio`；Plan47 的 Cards 公共结果；Plan42 的场景提案与当前 Level00 空间边界。
- 影响模式：`SharedContract`（CSV Schema/manifest、Data Registry、Encounter/Run/Save、Enemy Definition 与公共快照）；`Exclusive`（新增权威 `ReEchoEncounterData.xlsx`、维护后的 `ReEchoEnemyData.xlsx`、同批生成 CSV、最终预构建包）。这是远端集成影响说明，不是跨机器写锁。
- 兼容承诺 / 下游操作：XLSX 是唯一策划可编辑真源，CSV 是确定生成并供运行时打包的真源；不在 C++、JSON、Config 或 Widget 复制已经迁移的关卡/刷怪数值。保留现有稳定 Boss ID `M_TimeGuard` 和旧敌人 ID 以支持旧保存恢复；策划源 `M_SHEEP` 作为来源映射/显示语义，不直接让旧存档失去定义。现有 v8/v9 保存按显式迁移处理，不能把原六场已完成存档静默解释成未完成八场新 Run。若 Plan47 先提升 SaveVersion，以其远端版本为基线追加迁移，不覆盖卡牌域修订。
- 明确排除：不制作或接入正式场景、WBP、贴图、动画、VFX 或音频资产；不执行 Plan42 的 Editor authored map；不实现文档中的商店阶级价格、存活/无伤奖励和完整卡牌构筑；不在本 Plan 新增玩家 `0.1s` 全局受伤无敌、同帧多投射物去重或胜利优先仲裁；不以自由文本执行玩法逻辑；不把需求总表导出运行时数据；不替代用户做刷怪手感、可读性或难度平衡验收。

## 锁定目标

将外部《开普勒》回响配置表和关卡/怪物说明书中已经明确的结构化内容迁移为项目正式的策划配表与运行时数据契约：怪物基础属性/技能继续由独立 `ReEchoEnemyData.xlsx` 负责，关卡阶段、八场遭遇、三波刷新、出生空间参数和双锚策略由新增 `ReEchoEncounterData.xlsx` 负责。统一同步命令一次验证并事务式生成全部生产 CSV；非法类型、单位、ID、外键、比例、波次或行为 ID 必须指出工作簿/Sheet/Table/行/列并失败，不能静默采用默认值。

运行时以不可变 Encounter Catalog 驱动八场流程。战斗1-7固定30秒，不因提前杀光敌人提前结束；每场在0/10/20秒按表刷新并提前发布预警。战斗1-2、3-5、6-7分别属于三个逻辑阶段：同阶段相邻战斗保留存活怪物，跨阶段清理存活怪物；Boss为独立阶段。阶段只定义玩法生命周期和未来场景 ID，不在本 Plan 创建场景资产。

刷怪位置由单一确定性 Spawn Resolver 负责：按 Encounter 配置选择当前玩家预测锚或最近场次回响 E1 的未来路径锚，使用类型化的距离环、最小间距、预警和单位上限约束生成候选点，并在越界时镜像/回退到场内。Echo锚可从完整录制按 `waveTime + 2.2s` 采样；玩家未来位置采用当前速度在2.2秒内的确定性线性外推并钳制到 Arena 边界，禁止读取尚未发生的未来输入。多回响只使用最近场次 E1 作为出生锚，不改变玩家已选择的战斗回放集合。

为策划源四类敌人提供可运行的类型化定义：近战史莱姆、远程兔子、精英狐狸和现有羊形 Boss 语义。史莱姆使用接触攻击及表驱动间隔；兔子使用锁定位置、带预警的远程爆点/投射攻击；狐狸使用正面防御与带预警的直线突进；Boss继续复用Plan44已验证的四主动技能、元素清洗和30秒阶段。行为通过注册 `BehaviorProfileId` / `BehaviorId` 选择，数值直接来自表；不在 `SpecialMechanism` 文本中编码逻辑。旧 `M_Grunt/M_Shield/M_Bomber` 定义保留为兼容行但新八场不再以它们替代史莱姆/兔子/狐狸。

策划源之间出现差异时锁定以下优先级：本 Plan 中的人工锁定决策 > 当前外部XLSX结构化单元格 > DOCX对应表格 > DOCX叙述性正文 > 旧运行时占位实现。具体采用XLSX的波次 `5/5/0 → 3/3/2 → 5/5/2` 和 Boss“玩家或Boss血量清空”结束条件；比例按0～1小数解释。所有运行时距离统一输出厘米、时间统一秒，不保留 `30s`、`1.0m`、`Boss+24`、`无`、`—`、`≥2(每技能)` 等混合字符串。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` 的 `AREA-Data`、`AREA-Encounter`、`AREA-Run`、`AREA-Recording`、`AREA-Enemies`、`AREA-Presentation`、`AREA-Tests`；`MOD-ReEchoCombat`、`MOD-ReEchoWeapons`、`MOD-ReEchoEnemies`。Plan48 返修将阵营关系收敛为 Combat 公共契约，Weapons 的近战/投射物候选筛选和最终 Resolver 共同消费同一规则。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoCombat.md`、`MOD-ReEchoEnemies.md` 与 `MOD-ReEchoWeapons.md`，均已加入Writes；审阅 `ARCHITECTURE.md` 和 `README.md`，如拓扑/稳定路由确有变化则同候选更新。
- 设计意图：将“怪物是什么”和“这一场何时、在哪里生成什么”分开。Enemies模块只拥有单个敌人的确定性行为状态；Encounter Catalog/Spawn Resolver拥有关卡、波次和出生决策；GameMode只编排并生成Host，不再持有刷怪平衡常量或逐类型分支。
- 权威状态与依赖：
  - `ReEchoEnemyData.xlsx → enemies/enemy_abilities/boss_phases.csv → Enemy Catalog` 是敌人定义唯一链路。
  - `ReEchoEncounterData.xlsx → stages/encounters/encounter_waves/spawn_profiles/spawn_policy.csv → Encounter Catalog` 是关卡/刷怪唯一链路。
  - `AReEchoEncounterDirector` 继续独占固定步遭遇时钟；新增波次调度读取该时钟，不自行使用World漂移计时器。
  - Spawn Resolver是纯数据/确定性算法，接收稳定随机种子、Arena边界、只读玩家运动状态和只读录制路径，返回Spawn Intent；它不生成Actor、不操作UI。
  - `UReEchoEnemyLogicComponent` 继续独占敌人AI phase、冷却、攻击序号和行为状态；GameMode/EnemyHost只应用Intent并转发到Combat。
  - `UReEchoEnemyRosterComponent` 继续独占当前活动敌人集合；同阶段切场不清空，跨阶段由显式Stage策略清空。
- 决策记录：
  - 回响与玩家属于同一阵营，敌人属于敌对阵营。阵营由 Combat 公共值契约和 Actor 显式接口提供，并快照进攻击身份；不得在 Echo、Projectile、WeaponActor 或具体 Enemy 中散落按类型判断。Weapons 在候选阶段过滤友方以避免投射物被友方提前消费，HitResolver 再做最终裁决防线；显式自毁等例外必须通过命名 Intent 标志进入同一规则。
  - 新建独立 `ReEchoEncounterData.xlsx`，不把关卡/波次混入敌人工作簿，也不直接把外部原表作为运行时输入。原因是两类表拥有不同生命周期、外键和策划编辑边界。
  - 工作簿保留多个策划可编辑Sheet，但每个生产区域必须是命名Excel Table并由独立 `_ExportMap` 声明；说明性Sheet不导出。
  - `EncounterWaves` 使用逐Encounter显式行，不在运行时隐藏计算“每关+1”；当前数值以外部XLSX波次数量为准。后续策划要增加数量时直接改表。
  - 保留 `M_TimeGuard` 作为Boss稳定运行时ID，避免破坏保存/引用；通过展示名、PresentationId、SourceSheet/SourceRow/Notes记录羊Boss语义，不引入双重可写定义。
  - 旧普通敌人ID保留作保存兼容但不进入新Encounter引用；新行为使用 `M_SLIME/M_RABBIT/M_FOX` 稳定ID。
  - 逻辑阶段与具体地图资产解耦；Stage表可以持有稳定 `SceneId`，当前未接入的场景以已注册兼容值指向Level00，禁止软路径自由拼接。
  - 普通战斗按时间结束；清场不提前完成。Boss按Boss死亡/玩家死亡结束，30秒只触发现有回响销毁与玩家强化阶段。
  - 波次预警是确定性Spawn Intent状态，不由VFX完成回调触发；缺少表现时仍在计划时间生成，用户只负责判断提示可读性。
- 相关文档同步范围：`shared/CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`modules/MOD-ReEcho.md`、`modules/MOD-ReEchoEnemies.md`；`Content/Data/README.md`、`scripts/data/README.md`、新增两份Encounter配表使用/验收文档及现有Enemy配表文档。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅并更新。
  - `shared/CODEBASE_MAP/README.md`：已审阅并更新。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：已更新并审阅。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`：已更新并审阅。

## 锁定验收

- [x] 仓库内新增策划可编辑的 `ReEchoEncounterData.xlsx`，包含规范化Stage、Encounter、Wave、SpawnProfile、SpawnPolicy生产Tables及独立ExportMap；`ReEchoEnemyData.xlsx`包含史莱姆、兔子、狐狸和兼容Boss/旧ID的规范化定义。
- [x] 一次运行 `python scripts/data/sync_xlsx_to_csv.py` 可以联合验证全部权威工作簿并事务式生成对应CSV；`--check`证明仓库CSV与工作簿逐字节一致。
- [x] 表格数值是纯类型：比例0～1，距离厘米，时间秒，计数整数；不存在运行时解析单位字符串、范围字符串、中文“无/—”或自由文本逻辑。
- [x] Schema/manifest覆盖全部新增表、主键、外键、枚举、Behavior白名单和数值范围；重复ID、未知敌人/阶段/行为、非法波次顺序、比例和为非1、越界计数、负时间、缺表/改ExportMap均确定性失败并定位单元格。
- [x] 新Run有8场：1-7为30秒定时战，Boss为第8场；普通战提前清场不结束，玩家死亡仍立即失败；Boss死亡结束，30秒阶段不结束Boss战。
- [x] Encounter 1-7均在0/10/20秒执行三波且预警提前量来自表；暂停时预警、波次、录制和Echo不推进，恢复后顺序不漂移、不重复刷新。
- [x] 战斗1-2、3-5、6-7分别按Stage保留存活怪物；2→3、5→6、7→Boss显式清理；保存/继续能恢复当前Encounter、Stage、已触发波次、预警状态与存活敌人，不重复奖励或重复生成。
- [x] Anchor比例逐场按表生效；无Echo时确定性回退玩家锚，多Echo只使用最近场次E1路径锚；同一Run种子、相同录制与输入得到相同Spawn Intent序列。
- [x] Spawn Resolver遵守类型距离环、最小间距、玩家≥4m、基础回响≥2.5m、Arena边界镜像/回退和活跃单位上限；无法满足约束时给出确定性降级/拒绝结果与聚合诊断，不无限重试。
- [x] 史莱姆、兔子、狐狸分别通过其注册Logic行为产生接触、远程爆点/投射和防御突进；攻击冷却/前摇/后摇/范围/伤害来自表，表现缺失不改变提交时刻或伤害结果。
- [x] 远程1.2秒爆点上限、精英技能并发和总活跃单位上限由单一Encounter/Spawn协调器执行；Enemy组件不各自复制全局令牌状态。
- [x] Boss保留Plan44四技能、9秒净化/1秒免疫、30秒回响销毁与强化、保存恢复；Boss Encounter读取第8场配置并使用战斗7最近E1作为默认Boss回响来源，不破坏玩家其他回响存储。
- [x] 旧六场保存得到显式兼容结果：已完成旧Run仍为完成；未完成旧Run迁移或拒绝的策略有测试和清晰诊断；不得静默改写为另一条八场历史。若Plan47已升级SaveVersion，迁移链保持连续。
- [x] 现有武器、Combat、Recording、Echo选择、商店、Cards和音频语义无回归；Plan47集成后重新验证共享Data/Run/Save/Enemies接缝。
- [x] 使用说明明确告诉策划编辑哪个工作簿/Sheet、哪些列可编辑、单位/枚举/ID规则、同步命令和常见错误；策划无需手改CSV。
- [x] Python数据测试、项目校验、聚焦Unreal自动化和 Editor Development 构建已通过；最终组合候选已执行 `-FullRebuild` 并确认只包含 `GIT_RULES.md` 允许的预构建产物。
- [ ] 用户在PIE确认八场推进、三波节奏、刷怪方向/距离、跨战斗残留、三类普通怪手感及Boss衔接后，人工验收才能设为`Passed`。

## Step 0 门禁

- 基线分支/提交：Plan发布并核验后，从 freshly fetched `origin/main` 创建 `plan/48-kepler-encounter-enemy-data-runtime` worktree；开始实质实现前把Plan状态改为`InProgress`。
- 引擎/构建可用性：发布Plan前在 `origin/main@86bf9a4 + Plan48` 最终候选执行程序发布所需 `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`；实现阶段使用增量构建，最终发布再次在合并候选FullRebuild。
- 现有聚焦测试结果：Plan44关闭时Enemy、Boss、Save与数据链通过；Plan48开始时先运行 `sync_xlsx_to_csv.py --check`、聚焦Python测试和现有 `ReEcho.Enemies.*` / Encounter基线，记录实际结果。
- 共享契约 / 难合并资源风险：Plan47与本Plan均可能写Data生成器、Schema、Data/Run/Save、Enemies和预构建包。两者可本地并行，但任何一方进入远端后，另一方必须执行外部提交审计和组合适配；二进制XLSX不得整文件静默取一方，需按命名Table语义合并并重生CSV。
- 基线损坏时的停止条件：权威工作簿无法通过现有`--check`、Plan47已发布不兼容Schema/Save契约且未完成组合决策、现有Enemy/Boss基线构建失败、外部XLSX哈希变化且语义差异未审阅，或实现需要修改本Plan排除的Combat通用规则/正式场景资产时停止相关越界部分并报告。

## 实现提纲

1. 将外部XLSX/DOCX逐字段映射到canonical模型，建立来源追踪、稳定ID、单位转换和冲突决策清单。
2. 创建 `ReEchoEncounterData.xlsx` 的五张生产Table、ExportMap、保护/下拉/说明与验收文档；维护Enemy工作簿的规范化新敌人/技能行和兼容行。
3. 扩展Schema、manifest和同步器，生成五张新增CSV及更新Enemy CSV；新增工作簿结构、数据范围、外键和确定性字节测试。
4. 在 `AREA-Data` 编译不可变Encounter Catalog，在纯算法层实现Stage/Wave调度与Spawn Resolver，先以无World测试证明时间、锚点、边界和上限。
5. 改造Encounter/GameMode编排：八场、普通战定时结束、阶段内Roster留存、跨阶段清理、预警/波次Intent和Boss第8场语义。
6. 扩展Enemies类型化行为，实现史莱姆、兔子、狐狸Logic；EnemyHost只提供Sense/应用Intent/Combat转发，表现继续只读消费。
7. 扩展保存快照和迁移，覆盖当前Stage/Encounter、波次门、预警、全局令牌、Roster和Boss状态；与Plan47最终SaveVersion组合。
8. 运行聚焦Python、Data、Encounter、Enemies、Save、Recording/Echo及共享回归；更新Plan执行记录和所有相关CODEBASE_MAP文档。
9. 构建可供用户直接打开的Editor候选，请用户PIE验收；根据手测只修范围内确定性/接线问题，不由AI代做高token视觉遍历。
10. 人工通过后整理正式提交，在最新远端main上组合适配Plan47/其他提交，重跑最终FullRebuild/校验，合并推送并安全清理worktree。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| XLSX/CSV | `python scripts/data/test_sync_xlsx_to_csv.py`；`python scripts/data/sync_xlsx_to_csv.py --check` | 多工作簿ExportMap、Schema、外键、单位、错误定位、确定性输出通过 |
| 静态 | `python scripts/validate_project.py`；`git diff --check` | 数据、文档、源码和项目不变量通过 |
| C++格式/构建 | 修改文件运行仓库`.clang-format`；`scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT及模块链接成功 |
| Data自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Data` | 新Catalog、非法数据、稳定ID和运行时读取通过 |
| Encounter/Spawn | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Encounter` | 八场、三波、暂停、Stage留存、锚点、边界、上限、确定性通过 |
| Enemies | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Enemies` | 三类普通怪与现有Boss行为、快照/恢复通过 |
| Save/Echo共享回归 | 运行Plan内具名Save、Recording、Echo、Run聚焦集合 | 旧保存策略、波次恢复、E1锚和Plan47组合契约通过 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`；预构建check；最终fetch审计 | 最新准确集成候选及精选Editor包一致 |
| 人工PIE | 新Run完整跑至Boss，观察每场三波、阶段留存、刷怪位置、三类怪与Boss衔接 | 用户确认玩法可用性、节奏与提示可读性 |

## 执行记录

### 远端 Plan42 组合适配（2026-08-18）

- 已按用户决定获取并采用 `origin/main@39136cdb30a607e4beca949cdc6ac8f5bb91001a` 作为新的表现与场景基线；主工作树已 fast-forward，同步前本地 `main` 工作区干净。
- 物理冲突集中在 `ReEchoEnemyActor`、`ReEchoEnemyPresentationComponent`、`ReEchoGameMode`、`MOD-ReEcho.md` 和 Editor 预构建包。组合候选保留远端 ArenaScene/ArenaCamera、方盒碰撞、角色 Prefab、表现组件层级及场景资产，同时保留本 Plan 的八场/三波、稳定 EnemyId、三类新敌人、全局特殊技能许可与 v9 保存语义。
- 新适配使用 `EnemyDefinition.Archetype` 为 `Slime/Ranged/Elite/Boss` 选择远端 Grunt/Rabbit/Fox/Goat Gameplay Prefab；旧 Archetype 才保留兼容外观变体路由。EnemyHost 按配表碰撞半径/半高设置远端 `UBoxComponent`，再通过 ArenaScene 的玩法平面落位。
- 预编译包冲突未选择旧 Plan48 DLL；冲突阶段先采用远端包占位，待组合源码编译后由 `Build-Editor.cmd` 统一刷新。
- 数据工具测试 15/15 通过，`sync_xlsx_to_csv.py --check` 通过。首次 `validate_project.py` 仅因组合源码尚未重建、prebuilt source fingerprint 过期而失败；用户关闭 UE 后已完成组合源码构建并刷新预构建包。

### 玩家攻击方向组合缺陷修复（2026-08-18）

- 人工 PIE 发现自动与手动攻击都固定朝屏幕上方，而 Echo 自动索敌正常。根因是 Plan42 为避免旋转玩家根碰撞，将瞄准改为只写 `AReEchoPlayerPawn::AttackAimDirection`，但 `AReEchoWeaponActor` 的世界执行仍从 `Owner->GetActorForwardVector()` 读取方向；玩家根 Actor 不再旋转后，该值始终是默认方向。Echo 会主动旋转自身 Actor，因此没有暴露同一缺陷。
- 设计决策是保持逻辑/表现解耦，不恢复“瞄准时旋转整个玩家 Actor”。`AReEchoWeaponActor::ResolveOwnerAimDirection` 成为主模块世界适配的唯一方向入口：玩家读取显式逻辑瞄准，其他持有者回退自身前向；攻击位移、光波、投射物、近战几何、剑弧表现和 `AttackCommitted` 事件统一消费该入口。
- 新增 `ReEcho.Weapons.PlayerLogicalAimDrivesProjectile` 接缝回归：在玩家 Actor 朝向不变时把逻辑目标放到侧方，断言生成投射物跟随 `AttackAimDirection` 而不是 Actor Forward。
- `Build-Editor.cmd -Configuration Development` 成功；`validate_project.py`、15 项数据工具测试与 XLSX/CSV `--check` 通过；`ReEcho.Weapons` 10/10、`ReEcho.AttackMode` 7/7、`ReEcho.Encounter` 3/3、`ReEcho.Enemies` 16/16 全部成功且无失败。等待用户 PIE 复验实际自动/手动攻击方向。

### 敌方远程伤害临时安全降级（2026-08-18）

- 用户复验确认玩家攻击方向已修复，但敌方远程攻击的投射物/预警表现当前仍不可见。为避免玩家在缺少可读反馈时承受不可规避伤害，暂时把权威工作簿 `ReEchoEnemyData.xlsx / EnemyAbilities` 中四项敌方远程能力伤害设为 `0`：`M_TimeGuard_Projectile`、`M_TimeGuard_BlinkSlam`、`M_TimeGuard_PrayerBeam`、`M_RABBIT_RangedBurst`。
- 本次只修改数据，不加入隐藏 C++ 特判；能力的前摇、冷却、锁点/锁向、投射物与事件仍照常运行，便于后续美术资产接入和链路验收。Boss 近战挥砍、狐狸突进、史莱姆/通用接触伤害均保持原值。
- 待远程攻击表现可见且用户 PIE 验证可读性后，策划只需在同一工作簿恢复这四项 `Damage` 并运行统一同步脚本，不需要改代码。
- 与 `origin/main@b27310e` 的 Plan45 UI 更新完成组合：文本改动为可加性合并，旧 DLL/target/prebuilt 未覆盖最终结果，全部由组合源码 FullRebuild 重新生成。发布候选验证结果为：数据同步测试 15/15、`ReEcho.Enemies` 16/16、`ReEcho.Encounter` 3/3、`ReEcho.Weapons` 10/10、`ReEcho.UI.SettingsInteraction` 1/1，均无失败；`sync_xlsx_to_csv.py --check`、`validate_project.py` 与发布级 FullRebuild 均通过。

### 变化

- 新增 `ReEchoEncounterData.xlsx` 五张生产表及 ExportMap，并扩展 `ReEchoEnemyData.xlsx` 的史莱姆、兔子、狐狸和两项类型化能力；统一同步器事务式生成五张 Encounter CSV 与更新后的 Enemy CSV。
- Data Registry 新增 Encounter Reader/Catalog；Encounter Runtime 新增纯 WaveScheduler 与 SpawnResolver。GameMode 改为八场表驱动流程、普通战定时完成、Stage 内保留/跨 Stage 清理，并在预警时锁定出生位置、提交时复用。
- `ReEchoEnemies` 新增 Slime/Ranged/Elite Archetype；通用 `Abilities` 取代误导性的 Boss-only 数组。兔子锁点爆发、狐狸锁向突进/正面防御、史莱姆接触攻击均使用配表 Definition。
- Encounter coordinator 独占远程爆发窗口、精英技能并发与总活跃单位上限；EnemyLogic 只消费 `bSpecialActionPermitted`，不复制全局令牌。
- SaveVersion 提升为 v9，保存稳定 EnemyId、Wave 游标、预警批次/出生解析序号、预留位置和全局令牌剩余时间；已推进的旧六场保存明确拒绝，避免静默解释为八场新历史。
- 保留 Plan32“解锁但未选择时仍默认上一场回响”的较新语义，并修正遗留 Plan31 冲突测试。

### 证据

- `python scripts/data/test_sync_xlsx_to_csv.py`：15/15 通过；`sync_xlsx_to_csv.py --check` 与 `validate_project.py` 通过。
- `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`：成功，五个 Runtime Module 预构建包已刷新。
- `ReEcho.Data`、`ReEcho.Encounter`、`ReEcho.Enemies`、`ReEcho.Run` 聚焦自动化全部成功；最终全量 `ReEcho` 自动化 93 成功、1 个既有 `ReEcho.Presentation.Animation2D.AssetProfiles` 失败。该失败在未修改的 `origin/main@95808e2` 单独复跑同样失败，确认为基线资源断言，不是 Plan48 回归。
- `git fetch --prune origin` 后 `origin/main` 仍为 `95808e2`，实现期间无外部提交、真实冲突或逻辑覆盖风险。

### 剩余风险

- 用户尚未在 PIE 完整跑完八场；刷怪密度、橙色预警可读性、Stage 残留和三类怪手感仍需人工确认。
- 外部XLSX没有完整表达所有普通怪技能细节；本Plan只实现DOCX和XLSX共同明确、可类型化的行为，不从空白“特殊机制”猜测额外技能。若策划要求兔子的第二个独立技能或狐狸额外机制，需先补表并由用户锁定。
- 当前没有四个正式场景资产；Stage语义可以验收流程和留存，但场景切换视觉仍归Plan42/后续美术接入。
- `ReEcho.Presentation.Animation2D.AssetProfiles` 在 main 与本候选都因 Static Idle 的 Paper2D collision 断言失败，属于既有资源侧基线问题，后续由对应 Presentation/资源任务处理。

### 人工验收结果/请求

当前组合候选已通过增量 Editor 构建并自带匹配预构建包，等待用户在 `ReEcho-plan48` PIE 验收；进入 `origin/main` 前仍需在最终候选执行发布级 FullRebuild。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅并更新八场流程、Encounter 数据/协调所有权与依赖方向。
- `shared/CODEBASE_MAP/README.md`：已审阅并更新 `AREA-Encounter` 路由。
- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：已更新 Data、Encounter、Run/Save、EnemyHost 与 v9 组合状态。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`：已更新三类普通怪、通用 Ability、全局许可输入和代码阅读路线。

### 回响友伤返修（2026-08-18）

- 用户 PIE 发现回响会伤害主角。根因不是回响索敌：Echo 仍只锁定 Enemy；共享近战、投射物、爆炸与 HitResolver 只排除攻击源/检查存活，没有阵营契约，因此玩家作为 `IReEchoCombatTarget` 会被回响载体命中，且回响武器事件错误沿用默认 `DamageSource::Player`。
- 新增 Combat 权威 `EReEchoCombatFaction`、`IReEchoCombatAffiliation` 与 `ReEchoCombatRelations`。Player/Echo=`PlayerSide`，Enemy=`EnemySide`；攻击 Commit 快照来源阵营。Weapons 在近战弧、投射物路径及爆炸候选阶段过滤友方，Combat Resolver 对物理、元素和连锁反应做最终防线；Bomber 自毁仅以具名 Intent 标志放行同阵营自身伤害。
- 新增 `ReEcho.Weapons.EchoAttacksIgnorePlayerSide`：把玩家放在回响与敌人之间，分别验证回响近战不伤玩家且仍伤敌、投射物穿过玩家后仍命中敌人；新增 `ReEcho.Combat.FactionRelations` 锁定敌我、同阵营禁止、显式例外与旧调用兼容规则。增量 Editor 构建成功；新专项 2/2、`ReEcho.Weapons` 11/11、`ReEcho.Combat` 9/9、`ReEcho.Enemies` 16/16 全部成功。当前机器未发现 `clang-format`，已按仓库风格检查 diff，正式发布前仍执行项目校验、diff check 与最终 FullRebuild。
- 架构文档审阅：`MOD-ReEchoCombat.md` 已补阵营权威和双层过滤；`MOD-ReEchoWeapons.md` 已补候选职责和 Echo 归因；`MOD-ReEcho.md` 已补三个 World Host 的阵营适配边界；`MOD-ReEchoEnemies.md` 已补 EnemySide 与自毁例外。`ARCHITECTURE.md` 已审阅，无模块拓扑或依赖方向变化；`README.md` 已审阅，无稳定标识或路由变化。
