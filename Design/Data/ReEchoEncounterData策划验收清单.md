# ReEchoEncounterData 策划验收清单

## 工作簿

- [ ] 五张生产 Sheet/Table 和独立 `_ExportMap` 均存在，表头锁定、数据行可编辑、下拉有效。
- [ ] 一次同步同时生成 `stages.csv`、`encounters.csv`、`encounter_waves.csv`、`spawn_profiles.csv`、`spawn_policy.csv`。
- [ ] 所有时间为秒、距离为厘米、比例为 0～1、计数为整数，无 `30s`、`1m`、`—` 等混合文本。

## 当前 Demo 契约

- [ ] 共有 4 个逻辑 Stage、8 个 Encounter；1-7 为 30 秒，8 为 Boss/玩家死亡结束。
- [ ] Encounter 1-7 各有 0/10/20 秒三波；战斗 1-2 的精英数量均为 0。
- [ ] 波次数量为 `5/5/0 → 3/3/2 → 5/5/2`，Boss 在第 8 场 0 秒生成。
- [ ] 双锚比例、远程 1.2 秒窗口上限、精英并发和活动单位上限与策划原表一致。
- [ ] 近战、远程、精英分别引用 `M_SLIME`、`M_RABBIT`、`M_FOX`；Boss 保留 `M_TimeGuard`。

## 自动与人工验收

- [ ] `python scripts\data\sync_xlsx_to_csv.py --check` 通过。
- [ ] `python scripts\data\test_sync_xlsx_to_csv.py` 和 `python scripts\validate_project.py` 通过。
- [ ] `ReEcho.Encounter.*`、`ReEcho.Enemies.*`、保存/继续相关自动化通过。
- [ ] PIE 由用户确认三波节奏、阶段内残留/跨阶段清理、出生方向与距离、三类怪行为及 Boss 衔接。
