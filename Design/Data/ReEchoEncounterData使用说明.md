# ReEchoEncounterData 使用说明

`Design/Data/ReEchoEncounterData.xlsx` 是关卡阶段、遭遇、波次和刷怪空间规则的唯一策划真源。策划只改 XLSX；`Content/Data/*.csv` 由工具生成，不手改。

## Sheet 与职责

| Sheet / Table | 作用 | 主要可调项 |
|---|---|---|
| `Stages / tblStages` | 逻辑关卡阶段 | 遭遇范围、同阶段是否保留怪物、跨阶段是否清理、未来场景 ID |
| `Encounters / tblEncounters` | 单场规则 | 30 秒或 Boss 结束条件、双锚比例、远程/精英并发、活动单位上限 |
| `EncounterWaves / tblEncounterWaves` | 分波数量 | 0/10/20 秒时各类怪数量；Boss 行直接引用稳定 Boss ID |
| `SpawnProfiles / tblSpawnProfiles` | 类型出生参数 | 角色对应 EnemyId、距离环、间距、预警时间 |
| `SpawnPolicy / tblSpawnPolicy` | 全局出生约束 | 玩家/回响安全距离、预测提前量、边界策略和最大候选次数 |

## 编辑规则

1. 只编辑 Excel Table 的数据行；不要改 Sheet、Table、列名、列序、保护或 `_ExportMap`。
2. 时间统一为秒，距离统一为 Unreal 厘米，比例写 0～1 小数，计数写整数。
3. `Id` 和引用字段是稳定程序契约，不是展示文案。已发布 ID 不复用、不静默改名。
4. `EchoAnchorRatio + PlayerAnchorRatio` 必须等于 1；无可用回响时运行时确定性回退玩家锚。
5. 波次在同一 Encounter 内按 `WaveIndex` 唯一；触发时间不可倒序或超出普通战时长。
6. `SpawnProfiles.EnemyId` 必须从下拉中选择已注册怪物。要加新怪，先在 `ReEchoEnemyData.xlsx` 建稳定定义。
7. 不要在 `Notes`、单位字符串或公式中编码逻辑；新玩法先由程序注册类型化枚举/Behavior，再开放下拉。

## 同步与检查

在仓库根目录运行：

```powershell
python scripts\data\sync_xlsx_to_csv.py
python scripts\data\sync_xlsx_to_csv.py --check
python scripts\data\test_sync_xlsx_to_csv.py
```

统一入口会联合检查主数据、怪物、Encounter 和音频工作簿，先在临时目录生成完整包，通过 schema、外键和领域校验后才事务发布 CSV。Unreal 只读取 CSV，不读取 XLSX。

## 注意事项

以下列已作为稳定契约存在（同步脚本与 C++ 读取器都会按 `HasExactColumns` 精确校验），但当前首版运行时**不消费其值**：策划在 XLSX 中修改这些列的**数值不会改变任何玩法行为**，属于预留字段。它们被解析后存入运行时结构体，但没有任何行为分支读取：

| Table / Sheet | 预留列 | 当前固定行为 |
|---|---|---|
| `Encounters / tblEncounters` | `MeleeTargetingPolicy`、`ReplayPolicy` | 近战索敌、回响重放均走固定默认行为 |
| `SpawnProfiles / tblSpawnProfiles` | `DistributionPolicy`、`SpacingPolicy` | 出生固定为环形取样 + 最小间距夹取 |
| `SpawnPolicy / tblSpawnPolicy` | `BoundaryPolicy`、`CandidatePolicy`、`PlayerPredictionPolicy`、`MultiEchoPolicy` | 边界固定夹取、候选固定取首回声锚点、按速度预测 |

- **不要依赖这些列做玩法调整**；要让其生效，需先由程序注册并接上对应行为分支，再开放编辑。
- **不要删除这些列**：读取器要求 CSV 列与契约 1:1，删列会让 `validate_project.py` 与运行时精确列校验失败。
- 这 8 个列与 Boss（`boss_phases.csv` 的 `RefillHealthPolicy=RefillToMaximum`）同理，均为首版最小实现之外的预留位。

## 常见报错

- `Anchor ratios must sum to one`：双锚比例和不为 1。
- `Unknown ... reference`：引用的 Stage、Encounter 或 EnemyId 不存在/未启用。
- `Wave trigger sequence must be 0/10/20`：当前首版普通战波次时刻被改坏。
- `Finite or reference field must use an in-cell list validation`：下拉或表格保护被破坏。
- `Generated CSV drift detected`：XLSX 与仓库 CSV 不一致；确认改动后运行正式同步。
