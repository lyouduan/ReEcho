# ReEchoEnemyData 策划验收清单

## 文件与所有权

- [ ] `Design/Data/ReEchoEnemyData.xlsx` 存在，且本次编辑已取得怪物工作簿独占所有权。
- [ ] 主 `Design/Data/ReEchoData.xlsx` 没有 `tblEnemies`、`tblEnemyAbilities`、`tblBossPhases`。
- [ ] 独立工作簿包含 `Enemies/tblEnemies`、`EnemyAbilities/tblEnemyAbilities`、`BossPhases/tblBossPhases`。
- [ ] 独立 `_ExportMap/tblExportMap` 只拥有 `enemies.csv`、`enemy_abilities.csv`、`boss_phases.csv`，无重复输出。
- [ ] 表头和数据验证未被删除；表头锁定、数据行可编辑、系统页受保护。

## 敌人基础表

- [ ] 启用的稳定 ID 为 `M_Grunt`、`M_Shield`、`M_Bomber`、`M_TimeGuard`。
- [ ] `Archetype` 与 `BehaviorProfileId` 配对正确，Boss 标记只用于 Boss。
- [ ] 所有单位为 cm/seconds；生命、碰撞尺寸为正，其他字段在 schema 范围内。
- [ ] Bomber 的触发半径、伤害半径、引信时间为正；其他敌人对应字段明确为 `0`。
- [ ] `M_TimeGuard` 为 650 HP、45 cm/s、奖励 30；主工作簿和 JSON/C++ 中没有重复生产数值。

## Boss 技能与阶段

- [ ] 四个启用主动技能分别使用注册行为 `Boss.MeleeSweep`、`Boss.Projectile`、`Boss.BlinkSlam`、`Boss.PrayerBeam`。
- [ ] 主动技能 `SequenceOrder` 大于零、同 Boss 内唯一，且固定轮转顺序符合策划预期。
- [ ] 投射物速度为正；Blink Slam 有正前摇和正半径；每个技能最小距离不大于最大距离。
- [ ] `Boss.ElementCleanse` 为 `SequenceOrder=0`、间隔 9 秒、免疫 1 秒，其他数值字段为明确中性值。
- [ ] 30 秒阶段只出现一次：销毁本场 Echo，物攻×2、元攻×2、攻速×1.5、移速×1.25，不回血且不改最大生命。

## 自动验收

- [ ] `python scripts\data\sync_xlsx_to_csv.py --check` 通过并确认无 CSV drift。
- [ ] `python scripts\data\test_sync_xlsx_to_csv.py` 通过。
- [ ] `python scripts\validate_project.py` 通过。
- [ ] Unreal `ReEcho.Data.Enemies.*` 与完整 `ReEcho.Data.*` 自动化通过。
- [ ] 打包依赖包含三个怪物 CSV；`Content/Data/enemies.json` 不存在。
- [ ] PIE 确认四种敌人按稳定 ID 加载，未知/禁用 ID 明确失败且不回退。
