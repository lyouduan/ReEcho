# ReEcho 数据源

`Design/Data/ReEchoData.xlsx` 是主策划数据编辑源；怪物体系和关卡/刷怪体系分别由独立 `ReEchoEnemyData.xlsx`、`ReEchoEncounterData.xlsx` 唯一负责。统一入口联合生成本目录 CSV，但工作簿不共享二进制文件或 ExportMap 所有权。

主数据操作步骤见 [`Design/Data/ReEchoData使用说明.md`](../../Design/Data/ReEchoData使用说明.md)；怪物数据见 [`Design/Data/ReEchoEnemyData使用说明.md`](../../Design/Data/ReEchoEnemyData使用说明.md)。
验收分别见 [`Design/Data/ReEchoData策划验收清单.md`](../../Design/Data/ReEchoData策划验收清单.md) 和 [`Design/Data/ReEchoEnemyData策划验收清单.md`](../../Design/Data/ReEchoEnemyData策划验收清单.md)。

## Plan25 XLSX authoring

- Canonical workbooks: `Design/Data/ReEchoData.xlsx` and independent `Design/Data/ReEchoEnemyData.xlsx`.
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
- `characters.csv`：canonical 可玩/运行时角色 ID、展示名、基础属性、默认武器和外观 ID；不再混入角色能力逻辑参数。
- `character_aliases.csv`：显式旧 ID / 工作簿 ID 映射，例如 `J_01` 映射到 `J_SPADE`。
- `character_abilities.csv`：角色能力的一对多类型化配置，明确角色、触发点、目标、数值、间隔与已注册行为；描述文字不参与运行。
- `cards.csv`：canonical 卡牌行，包括来源、标签、晋升角色桶、抽取组、评审状态和禁用原因；旧勇者 Forge 选项已删除。
- `card_effects.csv`：启用卡牌的有序效果子表，使用类型化目标、`EffectKind`、`ValueOp` 和已注册 `BehaviorId`。
- `elements.csv`：canonical 元素 ID、触发/附着角色、展示键、颜色和视觉键。
- `statuses.csv`：元素反应需要的状态 ID、持续时间、叠加/刷新/互斥策略和已注册状态行为。
- `reactions.csv`：六个 ordered 元素反应，使用触发/附着元素 ID、已注册反应行为、白名单 `FormulaId`、半径、状态引用和回响/暴击规则。
- `weapon_types.csv`：七个稳定武器类型/基础 pattern。
- `weapons.csv`：具体运行时 `WeaponId`、 legacy `InputSlot`、初始选择顺序、启用状态和 data revision。
- `attack_steps.csv`：有序攻击 pattern 阶段。
- `slot_types.csv` / `slot_profiles.csv`：配件槽类型和各武器类型允许槽位。
- `parts.csv` / `part_effects.csv`：78 条武器插槽源行审计和启用配件的一对多 typed effects。
- `enemies.csv`：兼容敌人及开普勒史莱姆、兔子、狐狸、Boss 的稳定定义。
- `enemy_shard_drops.csv`：第 1–8 场近战、远程、精英怪死亡时的时间碎片闭区间；空精英区间表示该场不产出。
- `enemy_abilities.csv`：普通怪类型化技能、Boss 四个主动技能与元素清洗被动。
- `boss_phases.csv`：Boss 确定性阶段与临时战斗倍率。
- `stages.csv` / `encounters.csv` / `encounter_waves.csv`：四个逻辑阶段、八场遭遇与逐场显式波次。
- `spawn_profiles.csv` / `spawn_policy.csv`：角色→稳定 EnemyId、距离环、预警、双锚和边界约束。
- `TestFixtures/CsvRuntime/`：自动化用的正向和负向 fixtures，不是生产数据。

## 表目录

生产数据目录：`Content/Data/`

| 表 ID | 文件路径 | 用途 |
|---|---|---|
| `RuntimeSmoke` | `Content/Data/runtime_smoke.csv` | CSV 加载、打包和改值 smoke 表 |
| `RuntimeSmokeEffects` | `Content/Data/runtime_smoke_effects.csv` | `RuntimeSmoke` 的效果子表 |
| `Characters` | `Content/Data/characters.csv` | 角色基础属性、默认武器和外观 |
| `CharacterAliases` | `Content/Data/character_aliases.csv` | 旧 ID / 工作簿 ID 到 canonical 角色 ID 的显式映射 |
| `CharacterAbilities` | `Content/Data/character_abilities.csv` | 角色能力触发、目标、数值和已注册行为 |
| `Cards` | `Content/Data/cards.csv` | 卡牌抽取组、晋升角色桶和启用状态 |
| `CardEffects` | `Content/Data/card_effects.csv` | 卡牌的有序数值/行为效果 |
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
| `Enemies` | `Content/Data/enemies.csv` | 稳定 EnemyId、基础属性、行为 Profile 与表现键 |
| `EnemyAbilities` | `Content/Data/enemy_abilities.csv` | Boss 主动/被动技能、确定性顺序与空间参数 |
| `BossPhases` | `Content/Data/boss_phases.csv` | Boss 时间阶段、Echo 策略与临时强化倍率 |
| `EnemyShardDrops` | `Content/Data/enemy_shard_drops.csv` | 逐场敌人类别时间碎片掉落区间 |
| `Stages` | `Content/Data/stages.csv` | 逻辑阶段、遭遇范围和跨场清理策略 |
| `Encounters` | `Content/Data/encounters.csv` | 八场结束条件、双锚比例、并发和单位上限 |
| `EncounterWaves` | `Content/Data/encounter_waves.csv` | 0/10/20 秒波次及 Boss 生成 |
| `SpawnProfiles` | `Content/Data/spawn_profiles.csv` | 类型敌人 ID、距离环、间距和预警 |
| `SpawnPolicy` | `Content/Data/spawn_policy.csv` | 玩家/回响安全距离、预测和边界策略 |

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
