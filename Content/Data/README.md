# ReEcho 数据源

`Design/Data/ReEchoData.xlsx` 是策划唯一编辑源，本目录中的 CSV 是由工作簿生成、供 Unreal 读取和打包的目标运行时数据。本目录里的旧 JSON 文件只作为迁移期参考材料保留；对应领域迁移到 CSV 后，不要再在 CSV、JSON、C++、DeveloperSettings、Actor 或 Widget 里手工维护第二份可编辑真源。

完整策划操作步骤见 [`Design/Data/ReEchoData使用说明.md`](../../Design/Data/ReEchoData使用说明.md)。
首次仓库准备、分层验收、Unreal 启动顺序和反馈模板见 [`Design/Data/ReEchoData策划验收清单.md`](../../Design/Data/ReEchoData策划验收清单.md)。

## Plan25 XLSX authoring

- Canonical workbook: `Design/Data/ReEchoData.xlsx`.
- Fixed sync/check command: `python scripts/data/sync_xlsx_to_csv.py --check`.
- Install locked XLSX dependency: `python -m pip install -r scripts/data/requirements.txt`.
- Runtime code still reads only UTF-8 CSV in `Content/Data`; XLSX, Excel, COM and Office are never runtime dependencies.
- Do not hand-edit generated production CSV as a second truth. Change the workbook machine Tables, run the generator, and commit the resulting CSV diff.

## CSV 契约 v1

- 编码：UTF-8，不带 BOM。
- 分隔符：英文逗号。单元格包含逗号、引号或换行时，用 `"` 包裹；单元格里的字面引号写成 `""`。
- 空值：只允许出现在 `csv_schema.csv` 标记为可选的字段中。禁用的角色/卡牌行仍必须填写 `DisabledReason`。
- 布尔值：只使用小写 `true` 或 `false`。
- 百分比：使用小数表示，例如 `0.20` 表示 20%。
- 单位：距离列名必须带 `Cm`，时间列名必须带 `Seconds`。不要混用米和 Unreal 厘米，也不要混用毫秒和秒。
- ID：稳定 ID 只能包含英文字母、数字、`_`、`-` 和 `.`；不能留空，也不能带首尾空格。
- 引用：子表必须精确引用父表 ID。`runtime_smoke_effects.csv.RuntimeRowId` 引用 `runtime_smoke.csv.Id`；`character_aliases.csv.CanonicalCharacterId` 引用 `characters.csv.Id`；`card_effects.csv.CardId` 引用 `cards.csv.Id`。
- 数值操作：只允许 `Add`、`Multiply` 和 `Override`。
- 逻辑钩子：CSV 可以填写已注册的 C++ `BehaviorId` 和 `EffectKind`，但不会执行表达式、脚本或公式字符串。

## 文件说明

- `reecho_data_manifest.csv`：生产 CSV 的发现表，记录 schema 版本、表 ID、文件名和主键。
- `csv_schema.csv`：给人阅读并供静态校验使用的列契约。
- `runtime_smoke.csv`：最小运行时 smoke 表，用来证明 CSV 加载、打包和改值生效。
- `runtime_smoke_effects.csv`：运行时 smoke 的一对多子表，用于验证类型化数值参数。
- `characters.csv`：canonical 可玩/运行时角色 ID、展示名、基础属性、默认武器、外观 ID 和已注册被动行为 ID。
- `character_aliases.csv`：显式旧 ID / 工作簿 ID 映射，例如 `J_01` 映射到 `J_SPADE`。
- `cards.csv`：canonical 卡牌和锻炼选项行，包括来源、标签、晋升角色桶、抽取组、评审状态和禁用原因。
- `card_effects.csv`：启用卡牌和锻炼选项的有序效果子表，使用类型化目标、`EffectKind`、`ValueOp` 和已注册 `BehaviorId`。
- `elements.csv`：canonical 元素 ID、触发/附着角色、展示键、颜色和视觉键。
- `statuses.csv`：元素反应需要的状态 ID、持续时间、叠加/刷新/互斥策略和已注册状态行为。
- `reactions.csv`：六个 ordered 元素反应，使用触发/附着元素 ID、已注册反应行为、白名单 `FormulaId`、半径、状态引用和回响/暴击规则。
- `weapon_types.csv`：七个稳定武器类型/基础 pattern。
- `weapons.csv`：具体运行时 `WeaponId`、 legacy `InputSlot`、初始选择顺序、启用状态和 data revision。
- `attack_steps.csv`：有序攻击 pattern 阶段。
- `slot_types.csv` / `slot_profiles.csv`：配件槽类型和各武器类型允许槽位。
- `parts.csv` / `part_effects.csv`：78 条武器插槽源行审计和启用配件的一对多 typed effects。
- `TestFixtures/CsvRuntime/`：自动化用的正向和负向 fixtures，不是生产数据。

## 表目录

生产数据目录：`Content/Data/`

| 表 ID | 文件路径 | 用途 |
|---|---|---|
| `RuntimeSmoke` | `Content/Data/runtime_smoke.csv` | CSV 加载、打包和改值 smoke 表 |
| `RuntimeSmokeEffects` | `Content/Data/runtime_smoke_effects.csv` | `RuntimeSmoke` 的效果子表 |
| `Characters` | `Content/Data/characters.csv` | 角色基础属性、默认武器、外观和被动行为 |
| `CharacterAliases` | `Content/Data/character_aliases.csv` | 旧 ID / 工作簿 ID 到 canonical 角色 ID 的显式映射 |
| `Cards` | `Content/Data/cards.csv` | 卡牌、锻炼选项、抽取组、晋升角色桶和启用状态 |
| `CardEffects` | `Content/Data/card_effects.csv` | 卡牌和锻炼选项的有序数值/行为效果 |
| `Elements` | `Content/Data/elements.csv` | 元素身份、触发/附着角色、展示键和颜色 |
| `Statuses` | `Content/Data/statuses.csv` | 状态行为、持续时间、叠加/刷新/互斥策略 |
| `Reactions` | `Content/Data/reactions.csv` | ordered 元素反应、白名单公式、参数、状态引用和回响/暴击规则 |
| `WeaponTypes` | `Content/Data/weapon_types.csv` | 稳定武器类型和基础 pattern |
| `Weapons` | `Content/Data/weapons.csv` | 具体 WeaponId、InputSlot、LoadoutOrder、启用状态和 data revision |
| `AttackSteps` | `Content/Data/attack_steps.csv` | 有序攻击 pattern 阶段 |
| `SlotTypes` | `Content/Data/slot_types.csv` | Core/Grip/Blade 等配件槽 ID |
| `SlotProfiles` | `Content/Data/slot_profiles.csv` | 各武器类型允许的配件槽 |
| `Parts` | `Content/Data/parts.csv` | 78 条带 SourceSheet/SourceRow 的武器插槽审计行 |
| `PartEffects` | `Content/Data/part_effects.csv` | 启用配件的一对多 typed effects |

自动化 fixture 目录：`Content/Data/TestFixtures/CsvRuntime/`

| Fixture | 路径 | 用途 |
|---|---|---|
| `ValidAlt` | `Content/Data/TestFixtures/CsvRuntime/ValidAlt/` | 正向 smoke 改值重载样例 |
| `ReactionValueChanged` | `Content/Data/TestFixtures/CsvRuntime/ReactionValueChanged/` | 正向反应系数改值重载样例 |
| `DuplicateId` | `Content/Data/TestFixtures/CsvRuntime/DuplicateId/` | 重复 ID 负例 |
| `MissingRequired` | `Content/Data/TestFixtures/CsvRuntime/MissingRequired/` | 必填字段缺失负例 |
| `UnknownReference` | `Content/Data/TestFixtures/CsvRuntime/UnknownReference/` | 外键引用未知行负例 |
| `UnknownBehavior` | `Content/Data/TestFixtures/CsvRuntime/UnknownBehavior/` | 未注册 `BehaviorId` 负例 |
| `UnknownEffectKind` | `Content/Data/TestFixtures/CsvRuntime/UnknownEffectKind/` | 未注册 `EffectKind` 负例 |
| `UnknownFormulaId` | `Content/Data/TestFixtures/CsvRuntime/UnknownFormulaId/` | 未注册 `FormulaId` 负例 |
| `UnknownWeaponReference` | `Content/Data/TestFixtures/CsvRuntime/UnknownWeaponReference/` | 角色默认武器引用未知/未启用武器负例 |
| `DuplicateReactionPair` | `Content/Data/TestFixtures/CsvRuntime/DuplicateReactionPair/` | 重复 ordered reaction pair 负例 |
| `DuplicateWeaponInputSlot` | `Content/Data/TestFixtures/CsvRuntime/DuplicateWeaponInputSlot/` | 重复/非法热键武器映射负例 |
| `InvalidWeaponPartEffectBehaviorPair` | `Content/Data/TestFixtures/CsvRuntime/InvalidWeaponPartEffectBehaviorPair/` | 武器配件 EffectKind/BehaviorId 不匹配负例 |
| `IllegalRange` | `Content/Data/TestFixtures/CsvRuntime/IllegalRange/` | 数值越界负例 |
| `UnsupportedVersion` | `Content/Data/TestFixtures/CsvRuntime/UnsupportedVersion/` | 不支持 schema 版本负例 |

Fixture 目录只保存局部覆盖表；Python 和 C++ 自动化会先复制生产 CSV 基线，再套用 fixture 中存在的覆盖文件来组装完整临时包。

打开 Unreal 前先运行 `python scripts/validate_project.py`。它会校验 CSV schema、ID、外键、枚举、行为/效果/公式白名单、禁用源行、当前六张卡牌抽取池、六个元素反应、UTF-8 编码和预期负例 fixtures。

## Plan 20 reaction formula notes

- Registered element reaction formulas are `Element.ElementAttackDot`, `Element.ElementAttackSquared`, `Element.AttachInRadius`, `Element.ChainElementAttack` and `Element.EnhanceNextReaction`.
- `Element.ChainElementAttack` uses `(ElementalAttack + 2) * ReactionEfficiency * DamageIncrease`; `DamageMultiplier` is not the `+2` term.
- `Element.AttachInRadius` multiplies `RadiusCm` by `ReactionEfficiency` and still respects attachment blocking and elemental immunity.
- `CanCrit=true` is intentionally rejected at load time until crit-enabled reaction damage is implemented.
- `AffectedByEchoEfficiency` is consumed by reaction damage execution; current production reaction rows set it to `false`.
- `UnsupportedCanCrit` is the negative fixture for unsupported crit configuration.
