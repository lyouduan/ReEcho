# Plan 25 - data - XLSX authoring and deterministic CSV generation

## Coordination

- Planner owner: Gavyn-side Planner.
- Intended executor: the Plan25 AI receiving the final handoff prompt.
- Status: `ReadyForHandoff`.
- Planning/integration ref: `origin/coord/plan25-handoff-20260811`.
- Expected implementation branch: `plan/25-xlsx-authoring`, created from the advertised handoff ref containing accepted Plans 21–24 and this final Plan25 contract.
- Depends on: accepted Plans 21–24; all required runtime CSV schemas are now frozen for this implementation batch.
- Blocks: unified data-generation verification and the deferred combined Human PIE pass.
- Writes: canonical XLSX authoring artifact/location, deterministic generator and check mode, generated production CSVs, manifest/schema generation validation, data documentation, fixtures and packaging verification.
- Reads: all accepted typed CSV schemas/readers and `回响肉鸽数值与构筑体系.xlsx` as the current design source/reference.
- Shared-contract impact: every migrated production data table becomes generated output. Runtime Unreal code continues to consume validated CSV and must not gain an XLSX or Excel dependency.
- Downstream guidance: other Planners may continue UI/gameplay work through typed read-only APIs. Do not independently establish another workbook schema, hand-edit generated CSV as a competing truth, or change shared stable IDs without coordination.
- Explicit exclusions: live Excel automation, runtime `.xlsx` parsing, balance redesign, new gameplay behavior, UI redesign and unrelated asset changes.

## Locked goal

让策划只维护一个版本化的 XLSX 工作簿；工作簿保留与当前《回响肉鸽数值与构筑体系》几乎一致的中文 Sheet 和熟悉布局，并在各 Sheet 内补充机器可读的类型化区域。策划运行一个固定 Python 命令后，工具把有影响的区域确定性、事务化地同步到当前 Unreal 使用的严格 CSV 包；生成或发布失败不污染生产数据，并保持 Plans 21–24 已验收的稳定 ID、行为和存档兼容语义。

## Locked source-of-truth model

```text
Designer-edited XLSX
        ↓ deterministic generator
Generated UTF-8 CSV package
        ↓ existing strict readers/registry
Immutable Unreal runtime snapshot
```

- XLSX 是策划编辑源；CSV 是可提交、可 diff、可打包的生成运行时源。
- Unreal 不直接读取 XLSX，不依赖 Excel、COM 或本机已安装 Office。
- C++ BehaviorId/FormulaId/AttackPattern handler 仍是受控逻辑注册，不允许在单元格中执行脚本或表达式。
- 生成 CSV 不允许与手工维护的第二份 CSV 真源长期并存；紧急手改必须回填 XLSX 并由生成器复现，否则校验失败。
- 首次建表时的权威优先级锁定为：已验收生产 CSV 决定稳定 ID、enabled 批次、字段值与运行时语义；原始工作簿只决定熟悉的 Sheet 名、版式、来源描述和审计顺序。两者不一致时不得用旧工作簿静默覆盖已验收 CSV，差异必须写入迁移报告与 Execution notes。

## Locked workbook shape

- canonical 文件固定为仓库内 `Design/Data/ReEchoData.xlsx`；现有仓库外 `../回响肉鸽数值与构筑体系.xlsx` 只作为首次迁移的版式/内容来源，迁移后不再是第二编辑源。
- 保留当前策划熟悉的可见 Sheet 名和左侧展示结构：`属性S`、`元素体系Y`、`构筑体系G`、`角色体系J`、`武器体系W`、`武器插槽C`、`状态Z`。策划描述、评审、备注和示例可以继续保留。
- 机器导出不解析中文效果长句来猜数值或逻辑。每个生产实体/子实体使用同 Sheet 内的命名 Excel Table 和显式类型列；可把技术列放在现有展示列右侧并分组/隐藏，但它们仍是唯一导出权威。原工作簿左侧旧内容若不直接成为生产 Table，必须清楚标为受保护的 `ReferenceOnly`；与生产字段重复的可见内容应引用生产 Table 或与其共用同一单元格，不能形成第二套人工输入。
- 一个 Sheet 可以包含多个命名 Table 并生成多个 CSV，不再建立一组要求策划重复维护的“标准化 Sheet”。展示性说明与机器字段冲突时，导出器报错，不静默从说明文字反推。
- 增加受保护的 `_WorkbookMeta`、`_ExportMap` 和必要的 `_SystemData` 系统 Sheet。`_ExportMap` 明确记录 `SheetName + TableName + AdapterVersion + OutputCsv + SortKey`；策划普通改表不应编辑这些系统契约。
- 生产映射锁定如下；最终列名以已接受 CSV schema 为准：

| 策划 Sheet | 锁定命名 Table | 生成 CSV |
|---|---|---|
| `角色体系J` | `tblCharacters`, `tblCharacterAliases` | `characters.csv`, `character_aliases.csv` |
| `构筑体系G` | `tblCards`, `tblCardEffects` | `cards.csv`, `card_effects.csv` |
| `元素体系Y` + `状态Z` | `tblElements`, `tblStatuses`, `tblReactions` | `elements.csv`, `statuses.csv`, `reactions.csv` |
| `武器体系W` | `tblWeaponTypes`, `tblWeapons`, `tblAttackSteps` | `weapon_types.csv`, `weapons.csv`, `attack_steps.csv` |
| `武器插槽C` | `tblSlotTypes`, `tblSlotProfiles`, `tblParts`, `tblPartEffects` | `slot_types.csv`, `slot_profiles.csv`, `parts.csv`, `part_effects.csv` |
| `_SystemData` | `tblRuntimeSmoke`, `tblRuntimeSmokeEffects` | `runtime_smoke.csv`, `runtime_smoke_effects.csv` |

- `属性S` 先保留为策划字典/说明来源，不在未定义权威关系前强行导出。`武器体系（废案）` 明确排除生产导出；`怪物体系M` 和 `经济系统` 保留原貌但不属于本 Plan 的生成范围，后续领域 Plan 再认领。

## Locked command and publish behavior

```text
python scripts/data/sync_xlsx_to_csv.py
python scripts/data/sync_xlsx_to_csv.py --check
python scripts/data/sync_xlsx_to_csv.py --sheet "武器体系W"
```

- 无参数命令从 canonical 工作簿同步全部生产映射；`--check` 只读检测 XLSX/CSV 漂移；`--sheet` 仅用于开发加速，但仍要对完整临时快照执行跨表/handler/稳定 ID 校验，并以同一事务发布该 Sheet 影响的全部 CSV。
- 测试可使用 `--input <fixture.xlsx>`；非 canonical `--input` 必须同时指定测试输出目录或强制只读，绝不能覆盖 `Content/Data`。生产/文档命令默认路径不能依赖个人绝对路径。
- 所有16张 manifest 生产表先写入同盘临时生成包，经 schema、外键、顺序、稳定 ID、handler 和现有项目 validator 全部通过后才发布。发布阶段必须使用备份＋事务标记＋`os.replace`（或等价同盘替换）；任一替换失败立即回滚全部受影响文件，下次运行先恢复未完成事务。故障注入测试必须证明发布中途失败后生产 CSV 字节不变。
- Python XLSX 依赖必须在仓库内锁定版本并提供明确的一次性安装/自检入口；固定同步命令遇到缺依赖时给出可执行的安装提示，不允许依赖某个执行者机器上碰巧已有的包。
- `_ExportMap.OutputCsv` 只允许 manifest 中的文件名白名单，不接受绝对路径、`..` 或子目录逃逸；未知/重复 Table、未知输出和重复输出所有权都必须在写临时包前失败。
- 机器导出区域默认只接受字面量类型值，不依赖 Excel/COM 重新计算或 XLSX 缓存公式。展示区公式可以存在；导出 Table 中的公式默认报错，除非某列由版本化的确定性生成器明确白名单支持。

## Locked acceptance

### Automated

- [ ] 版本库内有明确、稳定、无本机绝对路径的 canonical XLSX 位置；工作簿版本和每个 sheet 的 schema/version 可识别。
- [ ] 版本库中恰好一个 canonical 策划工作簿；其七个当前生产 Sheet 名、熟悉的左侧可见布局、中文说明和源行顺序得到保留，系统技术列不会迫使策划在另一组标准化 Sheet 重复录入。
- [ ] `_ExportMap` 通过命名 Table 支持一 Sheet 对多 CSV；至少自动化证明 `角色体系J→2 CSV`、`构筑体系G→2 CSV`、`武器体系W→3 CSV`、`武器插槽C→4 CSV` 的映射完整且无重复输出所有权。
- [ ] 精确覆盖 `reecho_data_manifest.csv` 的16张生产表：runtime smoke parent/effects、characters/aliases、cards/effects、elements/statuses/reactions、weapon types/weapons/attack steps、slot types/profiles、parts/effects；纯 manifest/schema 元数据可由工具生成或一致性校验，但不能漏表或形成重复输出所有权。
- [ ] 每个 sheet 使用稳定表名、机器可读字段行和策划说明；稳定 ID、外键、枚举、单位、必填/可选、enabled/disabled/DisabledReason 与既有 CSV 契约一致。
- [ ] 生成器使用仓库内声明的 Python 依赖或项目已配置的可复现运行时；不要求打开 Excel，不使用 COM，不写用户目录。
- [ ] XLSX 依赖版本锁定、安装步骤和缺依赖错误可复现；不能以当前电脑已经安装 Excel、Python 包或 Codex 私有运行时作为通过条件。
- [ ] 三个固定命令可用；`--sheet` 对受影响 CSV 做成组事务更新且仍运行全局关系校验，`--check` 确认只读，fixture 可通过 `--input` 注入且无法误写生产目录。
- [ ] 生成顺序、行顺序、浮点/布尔格式、UTF-8、换行和 CSV quoting 确定；相同 XLSX 连续生成得到字节一致结果。
- [ ] 生成先写经过校验的临时完整包；正常发布、替换中途故障回滚和遗留事务恢复均有自动化。任一 sheet/schema/外键/handler/范围/发布失败时，生产 CSV 零部分更新并清楚报告 sheet、table、row、column。
- [ ] 生产导出字段不依赖 Excel 公式缓存；导出 Table 中未白名单公式、错误值、合并单元格跨字段或仅存在于中文描述中的隐含参数会明确失败。
- [ ] 提供默认同步模式和只读 `--check` 模式；`--check` 能识别 XLSX 与已提交 CSV 漂移、意外手改、缺表和陈旧生成物，且不会修改文件。
- [ ] 初次迁移生成的 CSV 与 Plan 24 最终生产数据在稳定 ID、enabled 批次和运行时数值语义上等价；任何有意格式/顺序变化记录在 Execution notes。
- [ ] 初始 canonical 工作簿的机器 Table 从已验收 CSV 反向播种；原始工作簿与生产 CSV 的角色、卡牌、元素和武器差异输出为可审计迁移报告，不得把原表旧值当成更高优先级。
- [ ] 现有生产基线＋局部覆盖 fixtures 继续工作；fixture 不要求复制整本 XLSX，也不能被生产生成器误覆盖。
- [ ] 工作簿源行追踪可复查，尤其保留武器插槽 78 行审计、62 条无名 disabled 行和批准启用批次。
- [ ] `武器体系（废案）`、`怪物体系M`、`经济系统` 不会被当前生成器误写入生产 CSV；`属性S` 的参考身份明确且无重复运行时权威。
- [ ] 七个生产 Sheet 和三个系统 Sheet 均完成渲染/目视检查：可编辑技术区与只读参考区可区分，表头冻结，枚举/布尔等有限值尽量使用数据验证，说明无明显截断或遮挡，系统契约受保护。
- [ ] 武器领域只有在 Plan 24 最终 schema/hash 契约冻结后接入；生成结果不得改变三个旧热键、三个 StartSelectable、`W_J_04` 或历史 WeaponId 语义。
- [ ] 静态验证将 XLSX→CSV check 纳入 `scripts/validate_project.py` 或等价快速入口；失败证据区分“工作簿错误”“生成漂移”“运行时 schema 错误”。
- [ ] 干净 clone 可执行生成/check、Editor Development build、全量 `ReEcho.*`、Shipping Cook/Pak/Archive CSV staging smoke 和 `git diff --check`。

### Human unified verification

- [ ] 策划在 XLSX 修改一个角色数值、一个卡牌数值、一个反应数值、一个武器数值和一个配件数值，生成 CSV 后无需重新编译即可在新 Run 生效。
- [ ] 统一回归开始/继续、局中保存退出、角色/武器选择、抽卡→商店、武器切换、玩家/回响、元素反应和代表性配件。
- [ ] 策划能从生成错误直接定位到工作表和单元格，不需要阅读 C++ 或 Python 堆栈。

## Step 0 gates

1. 读取 Plans 21–24 最终 Execution notes、全部生产 manifest/schema、领域读取器和 `scripts/validate_project.py`；记录最终表集合、列契约和稳定 ID，不凭旧工作簿猜测。
2. 从现有 `../回响肉鸽数值与构筑体系.xlsx` 派生仓库内唯一 canonical `Design/Data/ReEchoData.xlsx`，保留七个生产 Sheet 的名称、左侧布局和可见说明；记录原文件约 8 MB 的 Git 二进制更新成本，不把两个文件同时作为编辑源。
3. 在 Exchange 认领 canonical XLSX、生成脚本、16张生产 CSV、manifest/schema、Python 依赖锁和静态验证入口；Plan 24 已验收，武器领域必须在本批次完整接入，不再保留“等待 schema 冻结”的分支路径。
4. 先用 `角色体系J` 打通“同 Sheet 两个命名 Table→临时两张 CSV→既有 reader/validator→成组事务发布/回滚”，再扩展全部领域。
5. 初次全量生成前保存已接受 CSV 的语义快照，逐表比较 ID、外键、enabled 批次和数值；不能用“大量格式变化”掩盖语义漂移。

## Implementation outline

1. 以现有工作簿为版式骨架，定义命名 Table、技术列、`_WorkbookMeta` 和 `_ExportMap`；复用现有 CSV 列契约，不制造另一套同义字段，也不解析策划长句生成逻辑。
2. 实现固定入口 `scripts/data/sync_xlsx_to_csv.py` 的通用 Table reader、类型解析、一对多映射、稳定排序、CSV writer、临时包、事务发布/回滚/恢复和 check/diff 报告。
3. 先迁 accepted Plans 21–23 sheets，再接 Plan 24 最终武器/插槽 sheets。
4. 接入静态验证、依赖声明、文档和干净 clone/打包验证。
5. 完成统一自动化后交给人做一次集中 PIE，而不是每张表重复人工验收。

## Risks and exclusions

- XLSX 是二进制文件，Git 无法友好合并；同一 canonical workbook 是 Active ownership 独占资源，其他 Planner 通过公告协调，不并行编辑同一文件。
- 不把所有逻辑做成万能表解释器；XLSX 只配置已注册行为及其类型化参数。
- 不在本 Plan 顺手重平衡现有数值。发现源表与已验收 CSV 冲突时先列差异，由人决定权威值。
- 不让生成工具依赖个人路径、已登录 Excel、网络服务或不可复现的插件。

## Recommended executor model

强模型。工作跨二进制 authoring、确定性生成、全部数据 schema、事务发布/恢复、迁移对比和打包验证；可把纯 sheet 映射机械工作交给较便宜模型，但公共生成器与验收由强模型负责。

## Execution notes

### Changed

### Evidence

### Workbook and schema decisions

### Intentional CSV differences

### Remaining risks

### Human validation requested

## 执行者启动 prompt

```text
你是 ReEcho Plan25 执行者。只有 Plan25 Coordination 状态为 `ReadyForHandoff` 且人给出最终 base 后才开工。先按 AGENTS.md 最小顺序读取 shared/PLANNER_EXCHANGE.md 的公告/所有权、plans/25-data-xlsx-authoring-csv-generation.md、Plans 21–24 最终 Execution notes 和相关数据读取器/校验入口。

从指定最终 base 创建 `plan/25-xlsx-authoring` 独立分支/worktree，按 Coordination 认领 canonical XLSX、生成器、16张生产 CSV、manifest/schema、Python 依赖锁和校验入口。任务是建立“仓库内一个 `Design/Data/ReEchoData.xlsx`、多个接近原策划表的中文 Sheet、同 Sheet 命名 Table 一对多导出→确定性/事务化生成严格 CSV→既有 Unreal 快照”的完整链路；保持稳定 ID、启用批次、运行时语义和打包方式。固定入口为 `python scripts/data/sync_xlsx_to_csv.py`，并实现 `--check`、`--sheet`、测试用 `--input`。机器列必须类型化，禁止从中文效果长句猜逻辑；导出区域不得依赖 Excel/COM 公式缓存。Plan24 已验收，武器和插槽领域必须完整接入本批次。

验收照 Locked acceptance。禁止运行时读取 XLSX、禁止 COM/Excel 依赖、禁止手改生成 CSV 成为第二真源、禁止修改玩法或 `.uasset/.umap`。完成后更新 Execution notes，提交并推送自己的任务分支；禁止合并 main。
```
