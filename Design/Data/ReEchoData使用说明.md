# ReEcho 策划配表使用说明

本说明面向使用 Excel 或 WPS 配置角色、构筑、元素、武器和配件数据的策划。

首次从远端拉仓库并执行正式验收时，先按 [`ReEchoData策划验收清单.md`](ReEchoData策划验收清单.md) 准备隔离环境、分层测试并记录结果。

## 1. 唯一编辑源

- 只编辑仓库内的 `Design/Data/ReEchoData.xlsx`。
- 不再编辑仓库外的旧版《回响肉鸽数值与构筑体系》工作簿。
- 不要直接修改 `Content/Data/*.csv`。CSV 是由工作簿生成、供 Unreal 读取和打包的运行时数据。
- 不要在 JSON、C++、Actor、Widget 或 DeveloperSettings 中再维护一份相同数值。
- `ReEchoData.xlsx` 是不适合并行合并的二进制文件。开始修改前先确认没有其他人正在编辑它，并先同步最新分支。

## 2. 第一次使用

完整配表验收需要本地仓库；单独拿到 XLSX 只能检查布局、编辑和下拉，不能验证 CSV 生成或游戏生效。首次拉取、指定分支/提交、Python/Unreal 环境和反馈模板见 [`ReEchoData策划验收清单.md`](ReEchoData策划验收清单.md)。

在仓库根目录打开 PowerShell，安装一次固定版本的 XLSX 依赖：

```powershell
python -m pip install -r scripts\data\requirements.txt
```

确认当前工作簿和已提交 CSV 一致：

```powershell
python scripts\data\sync_xlsx_to_csv.py --check
```

出现 `[PASS] XLSX export package validates and production CSV bytes match.` 后再开始配表。

## 3. 应该编辑哪里

每个生产 Sheet 从左侧开始直接放置生成 CSV 的 Excel Table，不再并排保留一套旧说明表。旧表中的程序说明、规则文字和备注已按稳定 ID 合并到主实体 Table 的 `Description` 列；生产 Table 是唯一编辑与导出真源。

| Sheet | 可编辑生产 Table | 生成文件 |
|---|---|---|
| `角色体系J` | `tblCharacters`、`tblCharacterAliases` | `characters.csv`、`character_aliases.csv` |
| `角色能力A` | `tblCharacterAbilities` | `character_abilities.csv` |
| `构筑体系G` | `tblCards`、`tblCardEffects` | `cards.csv`、`card_effects.csv` |
| `元素体系Y` | `tblElements`、`tblReactions` | `elements.csv`、`reactions.csv` |
| `状态Z` | `tblStatuses` | `statuses.csv` |
| `武器体系W` | `tblWeaponTypes`、`tblWeapons`、`tblAttackSteps` | `weapon_types.csv`、`weapons.csv`、`attack_steps.csv` |
| `武器插槽C` | `tblSlotTypes`、`tblSlotProfiles`、`tblParts`、`tblPartEffects` | `slot_types.csv`、`slot_profiles.csv`、`parts.csv`、`part_effects.csv` |
| `经济系统` | `tblShopPriceRanges`、`tblShopDropLevels`、`tblEnemyShardDrops` | `shop_price_ranges.csv`、`shop_drop_levels.csv`、`enemy_shard_drops.csv` |

这些 Table 的数据行可以直接编辑，也可以在 Table 内新增或删除整行。表头、Sheet 顶部说明、系统契约和 Table 外区域被锁定是正常现象。

以下区域不生成当前生产 CSV：

- `属性S`：属性字典和说明。
- `怪物体系M`：仅作锁定参考，不生成当前生产 CSV；怪物生产数据位于独立 `ReEchoEnemyData.xlsx`。
- `_WorkbookMeta`、`_ExportMap`、`_SystemData`：系统 Sheet，策划不要修改。
- 历史 `武器体系（废案）` Sheet 已从 canonical 工作簿移除；运行时和 CSV 生成从不读取它。

## 4. 修改和增删行

修改现有数据时，直接编辑生产 Table 的数据单元格。

新增配置时：

1. 在对应 Table 的最后一行内使用“插入表格行”，或在 Table 紧邻的下一行输入，使 Excel/WPS 将新行纳入同一个 Table。
2. 填写完整且唯一的稳定 ID。
3. 填写所有必填字段和引用字段。
4. 如果是子表，父 ID 必须与父表中的 ID 完全一致。

删除配置时，删除 Table 内的整行，不要只清空部分必填字段。

不要执行以下操作：

- 修改 Sheet 名、Table 名或表头字段名。
- 把新数据写在 Table 范围之外。
- 合并生产 Table 内的单元格。
- 手动解除工作表保护或修改 `_ExportMap`。
- 在生产 Table 中使用 Excel 公式；生产字段只接受字面值。

## 5. 字段填写规则

- ID：只能使用英文字母、数字、`_`、`-`、`.`，不能重复、留空或带首尾空格。
- 布尔值：使用 `true` 或 `false`。
- 百分比：使用小数，例如 `0.20` 表示 20%。
- 距离：带 `Cm` 的字段使用 Unreal 厘米；不要填米。
- 时间：带 `Seconds` 的字段使用秒；不要填毫秒。
- 禁用行：将 `Enabled` 设为 `false`，并填写 `DisabledReason`。
- 外键：角色默认武器、子效果父 ID、元素和状态引用必须引用已存在且允许使用的稳定 ID。
- 顺序：`Order`、`StepIndex`、`LoadoutOrder` 等字段决定运行时顺序，不要依赖 Excel 当前显示排序。
- 敌人碎片区间：`Min/Max` 是包含两端的整数区间；`EliteMin/EliteMax` 必须同时填写或同时留空，留空表示该场精英不生成基础碎片拾取物。表中数值是单只敌人死亡时生成的拾取物携带量，玩家拾取后才进入余额。
- 数值操作：只使用允许的 `Add`、`Multiply`、`Override`。
- 逻辑字段：`BehaviorId`、`EffectKind`、`FormulaId`、`AttackPatternId` 等只能选择项目已经注册的值。表格不能新增任意脚本、表达式或新逻辑。

角色基础属性只在 `角色体系J` 修改；角色能力在 `角色能力A` 逐行配置。一个角色可以有多行能力效果，但每行必须明确生命周期 `Trigger`、类型化 `Target`、数值 `Value`、触发间隔 `Interval` 和已注册 `BehaviorId`。当前勇者能力按“当前缺失生命比例”实时计算：`Interval=0.1` 表示每缺失 10% 最大生命增加一层，治疗跨回阈值会减少层数，满血为零；它不累计历史受伤量。旧 Forge、诗人随机元素弹和勇者每第二击加成不是当前可配置能力。

有限枚举、布尔、外键和已注册逻辑字段带有单元格下拉。表头上的箭头只是筛选，不代表该列是单选；点击数据单元格后出现的下拉才是字段选项。Excel/WPS 仍可能允许键盘输入，但不在列表中的值会以“停止”错误立即拒绝。若确实需要新增逻辑 ID，应先由程序完成注册并更新表格契约，不能绕过数据验证硬填。

如果不知道某个技术字段应该填什么，先复制同类型已生效行的结构并询问程序，不要自行创造新的逻辑 ID。

## 6. 保存并生成 CSV

完成修改后先保存并关闭 Excel/WPS，然后在仓库根目录运行：

```powershell
python scripts\data\sync_xlsx_to_csv.py
```

该命令会：

1. 读取完整工作簿。
2. 校验表头、类型、必填字段、ID、引用、范围和逻辑白名单。
3. 在临时目录生成完整的跨工作簿 CSV 包。
4. 全部校验通过后才一次性发布到 `Content/Data`。

任何一步失败时，生产 CSV 不会留下部分更新。修正报错后重新运行同一命令即可。

生成成功后再运行：

```powershell
python scripts\data\sync_xlsx_to_csv.py --check
python scripts\validate_project.py
```

需要由程序开发只同步单个 Sheet 时可以使用：

```powershell
python scripts\data\sync_xlsx_to_csv.py --sheet "武器体系W"
```

策划日常操作优先使用无参数的全量同步，避免漏掉跨表关系。

## 7. 查看游戏效果

- XLSX 不会被游戏直接读取，必须先生成 CSV。
- 只改表不需要重新编译 C++。
- CSV 在 ReEcho 模块启动时加载一次；如果同步 CSV 时 Unreal Editor 已经打开，必须重启 Editor，仅停止 PIE 再点 Play 不会重载。
- 数据在新的 Run 中读取；已有 Run 或“继续游戏”可能持有开始时的数据快照，不适合验证新数值。
- 正确顺序是“保存并关闭表格→生成和校验 CSV→重启/启动 Unreal Editor→Play→新游戏”，再检查所改角色、卡牌、反应、武器或配件是否生效。
- 最新 `origin/main` 跟踪与源码匹配的 Win64 Editor 预构建包。策划安装项目规定的 UE 5.8 后，先运行 `python scripts\ue\prebuilt_editor.py check`；通过即可双击 `ReEcho.uproject`，无需安装 C++ 编译环境或自行构建。若检查失败或 Unreal 提示模块缺失/版本不同，停止并将完整错误交给程序刷新预构建包，不要由策划自行运行 `Build-Editor.cmd`。详见策划验收清单。

## 8. 常见报错

| 报错关键词 | 常见原因 | 处理方式 |
|---|---|---|
| `Missing XLSX dependency` | 未安装固定依赖 | 运行第 2 节的 `pip install` 命令 |
| `Columns do not match` | 修改了表头、漏列或多列 | 恢复原表头，不要自行增删字段 |
| `Duplicate` | ID、输出所有权或反应组合重复 | 找到报错行并改成唯一值 |
| `Unknown reference` | 子表引用了不存在的父 ID | 修正父 ID，或先补齐父表行 |
| `Unknown ...Id` / `behavior id` | 使用了未注册逻辑 ID | 改用已有注册值，或交给程序新增逻辑实现 |
| `Formula cells are not allowed` | 在生产 Table 中填写了 Excel 公式 | 改成最终字面值 |
| `must be unlocked for authoring` | 生产区保护状态被意外修改 | 不要手动改保护；从 Git 恢复工作簿或联系程序 |
| `must remain locked` | 系统或参考区被意外解锁 | 从 Git 恢复工作簿或联系程序 |
| `drift` | XLSX 与已提交 CSV 不一致 | 若刚改完表，运行全量同步；否则先确认是否有人手改 CSV |

生成错误会尽量使用 `Sheet:Table:row:column` 定位。例如：

```text
角色体系J:tblCharacters:row 4:column DefaultWeaponId: ...
```

按提示打开对应 Sheet、Table、行和列修正，不需要阅读后续 Python 堆栈。

## 9. 提交前检查

- 工作簿修改和生成的 CSV 必须放在同一次变更中。
- 不要只提交 XLSX，也不要只提交手改 CSV。
- 检查 `git diff -- Content/Data`，确认只有预期配置发生变化。
- 确认 `--check`、`validate_project.py` 均通过。
- 告知协作者自己修改了哪些 Sheet 和稳定 ID。
- 提交完成后释放 `ReEchoData.xlsx` 的独占编辑权，避免多人同时修改二进制工作簿。

## 10. 最短操作清单

```text
同步最新分支并确认无人同时编辑
→ sync_xlsx_to_csv.py --check
→ 编辑 ReEchoData.xlsx 中对应生产 Table
→ 保存并关闭 Excel/WPS
→ sync_xlsx_to_csv.py
→ sync_xlsx_to_csv.py --check
→ validate_project.py
→ 新 Run 验证
→ 同时提交 XLSX 和生成 CSV
```
