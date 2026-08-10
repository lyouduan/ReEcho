# ReEcho 数据源

CSV 是面向策划编辑的目标运行时数据源。本目录里的旧 JSON 文件只作为迁移期参考材料保留；对应领域迁移到 CSV 后，不要再在 JSON、C++、DeveloperSettings、Actor 或 Widget 里维护第二份可编辑真源。

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
| `DuplicateReactionPair` | `Content/Data/TestFixtures/CsvRuntime/DuplicateReactionPair/` | 重复 ordered reaction pair 负例 |
| `IllegalRange` | `Content/Data/TestFixtures/CsvRuntime/IllegalRange/` | 数值越界负例 |
| `UnsupportedVersion` | `Content/Data/TestFixtures/CsvRuntime/UnsupportedVersion/` | 不支持 schema 版本负例 |

Fixture 目录只保存局部覆盖表；Python 和 C++ 自动化会先复制生产 CSV 基线，再套用 fixture 中存在的覆盖文件来组装完整临时包。

打开 Unreal 前先运行 `python scripts/validate_project.py`。它会校验 CSV schema、ID、外键、枚举、行为/效果/公式白名单、禁用源行、当前六张卡牌抽取池、六个元素反应、UTF-8 编码和预期负例 fixtures。
