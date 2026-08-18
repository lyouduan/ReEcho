# ReEchoEnemyData 使用说明

`Design/Data/ReEchoEnemyData.xlsx` 是怪物体系唯一策划真源，由怪物策划独立维护。主工作簿 `ReEchoData.xlsx` 不承载、镜像或回写怪物生产表。

## 生产表

| Sheet | Excel Table | 生成文件 | 用途 |
|---|---|---|---|
| `Enemies` | `tblEnemies` | `Content/Data/enemies.csv` | 兼容敌人与开普勒史莱姆、兔子、狐狸、Boss 的稳定 `EnemyId`、基础属性、行为 Profile 与表现键 |
| `EnemyAbilities` | `tblEnemyAbilities` | `Content/Data/enemy_abilities.csv` | 普通怪类型化技能、Boss 主动技能和元素清洗；以 `OwnerEnemyId` 关联敌人 |
| `BossPhases` | `tblBossPhases` | `Content/Data/boss_phases.csv` | Boss 确定性阶段；以 `BossEnemyId` 关联敌人 |

工作簿必须有独立 `_ExportMap` Sheet 和 `tblExportMap`，只声明上述三张表。不要把怪物表加入主工作簿的 ExportMap。

## 编辑规则

1. 同一时刻只允许一名怪物策划编辑此二进制工作簿；开始前先与当前所有者交接。
2. 只在 Excel Table 数据行中编辑；不要改表名、列名、列顺序、Sheet 名、保护或 `_ExportMap`。
3. `Id`、`BehaviorId` 和引用字段是稳定程序契约，不是展示文本。已发布 ID 不得复用或静默改名。
4. 数字单位写在列名中：距离为 cm，时间为 seconds，倍率直接写十进制倍率。
5. 非适用的数字字段必须填 `0`，不能留空。`Notes` 可留空。
6. `EnemyAbilities.OwnerEnemyId`、`BossPhases.BossEnemyId` 必须从 `tblEnemies[Id]` 下拉选择；普通怪技能只能匹配其注册 Archetype。
7. `Boss.ElementCleanse` 使用 `SequenceOrder=0`，伤害/空间/普通计时字段为 `0`；只配置清洗间隔与免疫时间。
8. 主动技能使用大于零且同 Boss 内唯一的 `SequenceOrder`。运行时按此顺序确定性轮转。

## 同步命令

在仓库根目录运行：

```powershell
python scripts\data\sync_xlsx_to_csv.py --check
python scripts\data\sync_xlsx_to_csv.py
python scripts\data\sync_xlsx_to_csv.py --sheet "Enemies"
```

统一入口会同时读取 `ReEchoData.xlsx` 和 `ReEchoEnemyData.xlsx`，先生成临时完整包并校验 manifest、schema、跨表外键、注册行为和领域条件，再原子发布选中 CSV。Unreal 运行时只读取 `Content/Data/*.csv`，不读取 XLSX。

## 常见失败

- `Workbook does not exist`：独立怪物工作簿缺失，不能用 CSV/JSON/C++ 默认值绕过。
- `Unknown registered C++ boss behavior id`：`BehaviorId` 没有对应注册实现或拼写错误。
- `Unknown Enemies.Id reference`：技能/阶段引用了不存在的敌人。
- `Non-Bomber fields must use explicit zero`：非 Bomber 行误填了引爆字段。
- `Enabled boss ... has no enabled active ability`：Boss 没有至少一个可执行主动技能。
- `Generated CSV drift detected`：工作簿生成字节与仓库生产 CSV 不一致；确认工作簿所有权后运行正式同步。
