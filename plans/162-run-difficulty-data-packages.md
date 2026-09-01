# Plan 162 - 程序 - 本局锁定的三档难度数据包

## 协调

- Planner 负责人：JosephLE910 + Codex。
- Executor 负责人：JosephLE910 + Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@f10ee5255d1502475ecece3bec89d77c456e893a`。
- 本地实现方式（可选，仅作交接说明）：独立 worktree `ReEcho-plan162-difficulty`，本地分支 `plan/162-difficulty-profiles`。
- 依赖 / 阻塞：三档初始数据均复制当前生产怪物、Boss、Encounter 与刷怪配置；策划后续分别调数值，不属于本 Plan 的平衡验收。
- Writes: `plans/162-run-difficulty-data-packages.md`；`Design/Data/ReEchoDifficultyParty.xlsx`、`Design/Data/ReEchoDifficultyStandard.xlsx`、`Design/Data/ReEchoDifficultyNightmare.xlsx` 及被替代的怪物/Encounter 策划工作簿；`scripts/data/sync_xlsx_to_csv.py`、配套测试与说明；`scripts/validate_project.py`；`Content/Data/` 的难度生产 CSV、manifest 与说明；`Source/ReEcho/{Public,Private}/Core/`、`Data/`、`Run/`、`UI/ReEchoSettingsWidget.*`、`ReEchoGameMode.*`、聚焦自动化；`shared/CODEBASE_MAP/ARCHITECTURE.md`、`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`。
- Stable Reads: 当前 `ReEchoEnemyData.xlsx` / `ReEchoEncounterData.xlsx` 可见生产表；现有 `FReEchoCsvDataSnapshot`、`UReEchoRunSubsystem`、`UReEchoRunSaveGame`、`UReEchoPlayerProgressSaveGame`、设置页 C++/WBP 可选绑定与 StartRun/Continue 流程。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：稳定难度 ID 为 `Party | Standard | Nightmare`，显示名为“派对 | 常规 | 噩梦”；现有生产数据无损迁入 `Standard`，旧存档和没有难度字段的玩家偏好确定性迁移为 `Standard`；继续游戏只使用存档难度；新游戏默认使用上次成功应用的难度。
- 明确排除：本局中途切换；难度专属奖励、成就或解锁；本 Plan 代替策划调整派对/噩梦平衡；按倍率推导难度；运行时读取 XLSX；把难度状态交给 Widget 或 Actor；修改未提交的主工作区 UI 蓝图。

## 锁定目标

建立三套结构相同、初始内容相同、可由策划分别编辑的完整难度数据包：`Party（派对/简单）`、`Standard（常规/正常）`、`Nightmare（噩梦/困难）`。每套均覆盖相同的怪物基础、怪物技能、Boss 阶段、分关怪物数值、Stage、Encounter、波次与出生配置，不通过统一倍率限制策划。

玩家只能在尚未开始本局时通过设置页选择难度。新开一局时，Run 根据已应用的最后选择按需加载并锁定对应不可变数据快照；本局全部小关、后续波次、后来出生的怪物和 Boss 阶段都持续使用该快照，暂停设置页不能更改本局难度。继续游戏以存档难度重新加载同一数据包，不受全局偏好覆盖。

难度选择同时进入玩家偏好存档和本局存档。旧偏好与旧本局存档迁移为 `Standard`；缺失、未知、禁用或解析失败的数据包不得静默回退到另一档难度，必须给出可诊断错误并阻止错误新局/恢复。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`，以及其内部 `AREA-Core`、`AREA-Data`、`AREA-Run`、`AREA-Encounter`、`AREA-UI`；UI 文档入口 `MOD-ReEchoUI`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`，均已加入 `Writes`。
- 设计意图：把“策划编辑哪个难度的数据”“本局正在使用哪个难度”“设置页显示哪个偏好”分成清晰的三层。Data 只负责校验、按需装载并返回不可变快照；Run 独占本局 DifficultyId 和快照；UI 只编辑尚未开局时的玩家偏好，不参与怪物参数选择。
- 权威状态与依赖：
  - 三份 difficulty XLSX 是对应难度怪物/Encounter 配置的独立策划权威，统一生成各自生产 CSV；CSV 仍是运行时唯一磁盘输入。
  - `UReEchoRunSubsystem` 独占已锁定的本局 DifficultyId 与 `RunDataSnapshot`；所有 Encounter/Enemy 编译继续只消费 `GetRunDataSnapshot()`。
  - `UReEchoPlayerProgressSaveGame` 保存下一局默认难度，`UReEchoRunSaveGame` 保存当前局难度；Continue 先按存档 ID加载对应快照，再做既有构筑/Encounter 迁移与恢复。
  - 设置页通过 RunSubsystem 的窄接口读写偏好；运行中入口只读展示并禁用选择。
- 决策记录：
  - 采用三份同构完整数据包，不采用 `Easy/Hard` 统一倍率；代价是策划数据量和生成校验增加，但可逐怪、逐关、逐技能与逐波次独立调节。
  - 采用“新局/继续游戏时按需加载并缓存选定包”，不在每关或每次刷怪读磁盘；既满足选择后加载，也避免关中 IO 和半局漂移。
  - 本局锁定而非“下一小关生效”；这是用户最终决策，暂停设置不能改变当前局或待生效值。
  - `Standard` 作为兼容迁移值，现有生产行为必须逐行一致；Party/Nightmare 初始复制 Standard，难度差异由策划后续提交产生。
- 相关文档同步范围：更新 `ARCHITECTURE.md` 的 XLSX→CSV→本局快照链；更新 `MOD-ReEcho.md` 的 Data/Run/Encounter 权威和代码位置；更新 `MOD-ReEchoUI.md` 的设置页编辑门禁。`CODEBASE_MAP/README.md` 仅在阅读路线发生变化时更新。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：待实现后填写。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：待实现后填写。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：待实现后填写。
  - `shared/CODEBASE_MAP/README.md`：待实现后审阅。

## 锁定验收

- [ ] 三份 difficulty XLSX 的可见生产表结构和初始内容一致，统一同步能生成并校验对应 CSV；`Standard` 与 Plan 前生产怪物/Encounter CSV 语义逐行一致。
- [ ] 新游戏设置页提供“派对 / 常规 / 噩梦”，默认显示上次成功应用值；尚未开始本局时可以应用，恢复默认选择“常规”。
- [ ] 本局开始后难度不可更换；当前局全部 Encounter 和出生敌人共享开局锁定快照，设置页只读显示本局难度。
- [ ] 新局仅按需装载所选难度数据包；重复使用同一难度允许缓存，但不得每关/每个怪物重新读 CSV。
- [ ] 本局存档保存 DifficultyId；继续游戏先恢复并装载存档难度，不受全局偏好影响；旧存档确定性迁移为 `Standard`。
- [ ] 未知 DifficultyId、缺表、Schema/外键错误或所选数据包加载失败会显式拒绝新局/恢复，不静默换档。
- [ ] 聚焦自动化覆盖三档解析、按需缓存/隔离、StartRun 锁定、设置门禁、偏好与 Run Save 往返、旧版本迁移和继续游戏快照一致性。
- [ ] `python scripts/validate_project.py`、XLSX 同步 `--check`、受影响自动化、Editor Development 构建通过；最终发布候选完成 `-FullRebuild` 并刷新精选预构建包。
- [ ] UI 布局、中文可读性、新局可选/局中禁用和三档实际开局结果由用户 PIE 验收。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@f10ee5255d1502475ecece3bec89d77c456e893a`。
- 引擎/构建可用性：Git LFS `scripts/setup_lfs.py --check` 已通过；实现后按矩阵使用仓库 UE 5.8 入口。
- 现有聚焦测试结果：Plan 发布前只完成只读代码/数据审计；实现基线测试待执行记录补充。
- 共享契约 / 难合并资源风险：Schema、生成器、三份 XLSX、Run/Save 和 Settings/GameMode 为共享契约；远端最近 47 个提交涉及商店/卡牌/符文与 UI，当前无分支级真实冲突，但最终发布需重新审计同路径变化。主工作区两份未提交 UMG 与远端同路径更新有二进制覆盖风险，本任务完全不读取或修改该脏工作区。
- 基线损坏时的停止条件：当前生产 XLSX/CSV 不一致；所选表无法无损复制；旧存档无法确定性迁移；最新 main 引入难度/设置/Save 的矛盾契约；需要修改用户未提交 UMG 才能满足基础功能。

## 实现提纲

1. 将当前怪物与 Encounter 策划源无损整理为三份同构 difficulty 工作簿，扩展同步器、Schema、manifest、说明和回归测试。
2. 增加稳定难度枚举/ID 与 Data 按需装载缓存；以 Standard 基线快照为模板，原子替换所选难度拥有的完整领域数据。
3. RunSubsystem 增加偏好接口与本局锁定状态；StartRun 在解析构筑前取得所选快照，失败时不产生半初始化本局。
4. 扩展玩家偏好存档和 Run Save 版本/迁移；Continue 用存档难度建立快照后再恢复其他状态。
5. 设置页增加难度控件和新局/局中编辑门禁；GameMode 传递入口上下文，不让 Widget 决定本局状态。
6. 补齐聚焦自动化、日志和 `CODEBASE_MAP`，执行静态、同步、构建与运行时验证，交由用户 PIE 验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| XLSX/CSV | `python scripts/data/sync_xlsx_to_csv.py --check`；`python -m unittest scripts.data.test_sync_xlsx_to_csv` | 三份同构源表与全部生成 CSV 无漂移，负例校验有效 |
| 静态 | `python scripts/validate_project.py`；`git diff --check` | Schema、manifest、文档、生成产物与源码不变量通过 |
| C++ | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0 |
| 聚焦自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Difficulty`；受影响 `ReEcho.Run.SaveGame` / Settings 测试 | 难度装载、锁定、迁移、恢复和 UI 门禁通过 |
| 最终发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最新组合候选与精选 Win64 Editor 预构建包匹配 |
| 人工 | 新游戏分别选择三档、局中打开设置、保存退出并继续 | 中文选项正确；局中不可改；继续游戏维持存档难度；策划改表后的对应档可观察生效 |

## 执行记录

### 变化

- 待实现。

### 证据

- Plan 前 `python scripts/setup_lfs.py --check`：通过，3 个 LFS 路径已还原。
- 远端审计：本地旧 main 无独有提交，`origin/main` 前进 47 个提交至 `f10ee525`；远端范围为商店、卡牌、符文背包和 UI 发布，本任务从该远端 tip 创建隔离 worktree。

### 剩余风险

- 三档初始内容相同，因此功能验收需要临时 fixture/自动化证明路由隔离；正式平衡差异仍由策划后续编辑。
- Settings WBP 是美术可编辑资产；本 Plan 优先使用 C++ 可选绑定/安全回退，不覆盖主工作区未提交 UMG。

### 人工验收结果/请求

- `PendingBeforeClose`：实现候选构建后，请用户在 PIE 验收开局选择、局中锁定和 Continue。

### 架构文档审阅结果

- 待实现后填写。
