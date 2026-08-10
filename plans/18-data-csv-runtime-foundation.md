# Plan 18 - data - CSV runtime foundation

## Locked goal

建立 ReEcho 唯一、可验证、可打包的 CSV 运行时数据基础，使后续角色、构筑、元素、武器和插槽数据可以由策划修改 CSV 后生效，而不再把同一数值重复维护在 JSON、C++、DeveloperSettings 或 Actor/Widget 中。

本 Plan 只建设公共数据契约、加载/注册/验证/打包能力和最小夹具，不迁移 Plan 19–21 的完整领域内容。

## Source baseline

- 策划源参考：仓库上一级的 `回响肉鸽数值与构筑体系.xlsx`。
- 相关 Sheet：`属性S`、`角色体系J`、`构筑体系G`、`元素体系Y`、`状态Z`、`武器体系W`、`武器插槽C`。
- 当前项目：`Content/Data/*.json` 只做静态审阅；运行时仍读取 C++/DeveloperSettings，且 `ReEcho.Build.cs` 只显式打包 `cards.json`。
- 当前状态不能把 JSON 与 CSV 同时定义成可编辑真源。Plan 18 完成后，CSV 是目标策划源；尚未由 Plan 19–21 迁移的领域保留旧运行时路径，但旧 JSON 进入只读迁移状态，不再新增第二份平行配置。

## Locked acceptance

### 🤖 Automated

- [ ] `Content/Data/` 下存在有版本号的 CSV 数据契约和人可读说明；编码、分隔、转义、空值、布尔、百分比、距离/时间单位、稳定 ID 和引用规则明确且可被自动校验。
- [ ] 运行时通过一个公共、只读的数据注册入口加载 CSV；解析全部成功后一次性发布不可变快照，禁止逐表半成功和静默混用旧常量。
- [ ] 开发/测试环境遇到重复 ID、缺必填值、未知引用、未知 `EffectKind`/`BehaviorId`、非法数值范围或不支持的 schema version 时明确失败，并指出文件、行和字段。
- [ ] CSV 不执行任意表达式或脚本。数值规则使用有限的类型/操作；逻辑规则只引用 C++ 已注册的行为 ID 和类型化参数。
- [ ] 支持一对多子表，避免把数组、攻击阶段、多个效果或可变参数继续塞进一个自然语言单元格。
- [ ] 至少一个生产级最小 CSV 和一个测试夹具证明：修改 CSV 值、重新启动加载后，注册表返回新值且无需重新编译 C++。
- [ ] Windows Editor 与 Shipping 都能找到相同 CSV；打包清单/烟雾证据证明所有生产 CSV 被带入包中。
- [ ] `scripts/validate_project.py` 覆盖 CSV schema、唯一性、外键、枚举/行为白名单、UTF-8 和禁止空白 ID；旧 JSON 校验在迁移期保留但标明非运行时真源。
- [ ] 现有 60 Hz、20 Hz、30 秒、暂停语义和回响记录契约不因加载层引入非确定性或热更新中途换表。
- [ ] Editor build、相关数据自动化、全量 `ReEcho.*`、静态校验、Shipping 数据加载烟雾和 `git diff --check` 通过。
- [ ] 不生成或手改 DataTable `.uasset`；不提交工作簿副本、本机绝对路径、构建产物或临时导出物。

### 🎮 Human

- [ ] 人在一份代表性 CSV 中修改允许的测试数值、重新启动项目后看到对应只读诊断/测试值变化；无需打开 UE 资产编辑器。
- [ ] 人确认 CSV 列名、错误信息和 README 足以让策划独立定位填表错误。

## Step 0 gates

1. 在 `plan/18-csv-foundation` 独立 worktree 开工，记录 main 基线 commit、UE 5.8 Editor build 与现有 `ReEcho.*` 测试结果。
2. 不认领或修改当前工作树中的 `.uasset`/`.umap`；本任务应完全由 C++、CSV、Python/脚本、配置和 Markdown 完成。
3. 用 `rg` 确认所有 `Content/Data/*.json`、`UReEchoBalanceSettings`、硬编码卡牌/角色/元素/武器目录和 `RuntimeDependencies` 调用点，形成迁移矩阵；不要凭 README 推断运行时。
4. 先提出并在 Plan 执行经验中记录 ID 映射方案：工作簿的 `J_01`–`J_04` 与当前 `J_SPADE` 等 ID 不一致；已有录制/构筑使用的运行时 ID不得静默重编号。若无法保持兼容，停止并请人拍板 alias/migration 方案。
5. 先验证一个最小夹具可在 Editor 与 Shipping 读取；若项目路径/打包机制无法稳定包含松散 CSV，停止扩展并记录证据，不先迁移领域数据。
6. 明确加载失败策略：无效数据不能发布部分注册表，也不能悄悄回退到另一份可编辑配置。具体 UI/启动阻断形式由执行者实现并在执行经验说明。

## Locked data boundary

- CSV 保存策划可调的标量、枚举、引用、标签、显示文本键、有限数值操作和已注册行为 ID。
- 纯数值至少支持 `Add`、`Multiply`、`Override` 三类明确操作；百分比统一保存为小数，例如 `0.20` 表示 20%。
- 距离、时间、角度等字段必须在 schema/列名中固定单位；不得在同一列混用米与 UE 厘米、秒与毫秒。
- 逻辑行为使用 `BehaviorId + typed parameters`。允许对少量稳定 `EffectKind` 做分发，不允许按每个 Card/Weapon/Part ID 扩张一个巨型 switch。
- Component 不是默认实现：只有需要生命周期、持续状态、Tick 或场上空间实体的行为才考虑 Component；一次性数值/事件优先走通用 modifier、GAS effect/ability 或无状态 handler。
- 工作簿是需求来源，CSV 是迁移后的项目真源。不得维护“工作簿、CSV、JSON、C++ 四份都可编辑”的状态。

## Implementation outline

执行者可根据 UE 5.8 与当前代码选择具体类名、解析库和注册表形态，但必须保持上述契约。

1. 定义 schema version、manifest/文件发现、CSV 基础类型和错误模型。
2. 建公共加载/注册边界；先加载到临时结构，完成全局验证后原子发布只读快照。
3. 建行为/效果类型注册与参数校验接口，只提供最小通用操作和测试行为，不提前实现 Plan 19–21 的领域大全。
4. 扩展 `validate_project.py`，让 CI/执行者在启动 UE 前就能发现绝大多数填表错误。
5. 建最小生产表/夹具、确定 RuntimeDependencies 或同等可复现打包方案，并验证 Editor/Shipping 路径。
6. 更新 `Content/Data/README.md`、`shared/PROJECT_RULES.md`、`shared/CODEBASE_MAP.md` 和 Plan 执行经验，写清 CSV/旧 JSON 的迁移期权威边界。

## Expected ownership

- 公共数据加载/注册的新 `Source/ReEcho/{Public,Private}/Data/*`（具体名称由执行者定）。
- `Content/Data/README.md`、CSV manifest/schema/最小夹具。
- `scripts/validate_project.py`、`Source/ReEcho/ReEcho.Build.cs` 及必要的只读配置。
- 数据加载自动化测试。
- 不修改 Plan 19–21 的核心领域实现文件，除非最小编译接线确有必要并在执行经验中说明。

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| CSV static | `python scripts/validate_project.py` | schema、ID、引用、行为和编码全部通过；负例夹具被精确拒绝 |
| Format | 仓库 `.clang-format` + `git diff --check` | 所有新 C++ 合规，无空白错误 |
| Build | `scripts/ue/Build-Editor.cmd` | UE 5.8 Editor UHT/UBT 成功 |
| Automation | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | 既有测试与新增加载/失败原子性测试通过 |
| Packaging | clean Windows Shipping package + manifest/load smoke | 包内生产 CSV 齐全，启动时注册表加载成功 |
| Human | 修改代表性测试数值并重启 | 策划确认无需改 C++/资产即可生效，错误提示可读 |

## Risks and rollback

- 最大风险是迁移期出现多真源。每个领域只有在 Plan 19–21 完成后才切换运行时权威；切换必须整表原子完成。
- CSV 天然缺少嵌套结构；必须用子表和外键，不能用不可校验的 JSON 字符串或自然语言补洞。
- Shipping 松散文件路径与本地 Editor 路径不同；未取得包内加载证据前不得宣布基础设施完成。
- 数据快照只在确定的启动/Run 边界装载；不在遭遇中途热换，避免玩家与回响使用不同定义。

## Recommended executor model

强模型。该任务同时涉及 UE 打包、公共 API、确定性、校验器和多后续任务接口，Plan 19–21 都依赖其最终形态。

## Execution notes

### Changed

- Created isolated worktree `C:/Users/gavynqiu/Documents/miniGame/ReEcho-plan18` on branch `plan/18-csv-foundation` from main baseline `ff4409fc2c9448e35b67b5d02042a38cedfe8d59` (`ff4409f Merge plans 14-16: reactions, roles, and bomber ranges`). The planner/main working tree already had unrelated `.uasset` and planning-file changes; this branch does not claim or edit those assets.
- Added CSV contract v1 under `Content/Data/`: `reecho_data_manifest.csv`, `csv_schema.csv`, `runtime_smoke.csv` and `runtime_smoke_effects.csv`.
- Added automation fixtures under `Content/Data/TestFixtures/CsvRuntime/` for one valid alternate value and negative cases: duplicate ID, missing required value, unknown reference, unknown `BehaviorId`, unknown `EffectKind`, illegal numeric range and unsupported schema version.
- Added public runtime API `FReEchoCsvDataRegistry` in `Source/ReEcho/{Public,Private}/Data/ReEchoCsvDataRegistry.*`.
  - `LoadSnapshotFromDirectory` parses and validates into a temporary snapshot only.
  - `LoadAndPublishFromDirectory` and `LoadAndPublishDefault` replace the global snapshot only after all tables pass.
  - `GetSnapshot` returns the published `TSharedPtr<const FReEchoCsvDataSnapshot>`.
  - `RegisterBehaviorId` and `RegisterEffectKind` define the C++ allowlists. Plan18 registers only `RuntimeSmoke.LogValue` and `ScalarModifier`.
- Replaced the default game module class with `FReEchoModule` so module startup loads the default CSV package. Invalid production CSV logs all file/line/field issues and blocks startup with `UE_LOG(..., Fatal, ...)`; there is no partial registry and no fallback to JSON/DeveloperSettings.
- Added `ReEcho.Data.*` automation tests for default loading, alternate fixture value reload, invalid-load atomicity and all negative fixture classes.
- Extended `scripts/validate_project.py` to validate CSV schema/manifest/tables/fixtures, UTF-8 without BOM, stable IDs, foreign keys, value operation enum, behavior/effect allowlists and production CSV staging dependencies while keeping legacy JSON validation marked as migration-only.
- Staged production CSV files in `ReEcho.Build.cs` via `RuntimeDependencies` as NonUFS loose files. Test fixtures are not production-staged.
- Updated `scripts/ue/Find-UnrealEngine.ps1` to avoid `Get-ChildItem -Directory` / `-Attributes` because this Windows PowerShell environment rejected both parameters; it now filters containers with `Where-Object { $_.PSIsContainer }`.
- Updated `Content/Data/README.md`, `shared/PROJECT_RULES.md`, `shared/CODEBASE_MAP.md` and `shared/PROJECT_STATE.md` to record CSV as the target runtime source and legacy JSON as migration-only.

### Evidence

- Step 0 source scan found:
  - `Content/Data/*.json`: `cards`, `characters`, `elements`, `encounters`, `enemies`, `global_balance`, `reactions`, `statuses`, `weapons`.
  - Runtime data still comes from `UReEchoBalanceSettings`, `Config/DefaultGame.ini` and hardcoded C++ catalogs in current gameplay domains.
  - Prior packaging only staged `Content/Data/cards.json`; Plan18 now stages the production CSV files.
- Workbook read-only spot check found `角色体系J` IDs:
  - `J_01` 智者/Sage -> current runtime `J_SPADE`.
  - `J_02` 猎手/Hunter -> current runtime `J_DIAMOND`.
  - `J_03` 诗人/Poet -> current runtime `J_CLOVER`.
  - `J_04` 勇者/Brave -> current runtime `J_HEART`.
  - Current runtime also has local default/prototype `J_CAT`, which has no workbook alias in this spot check and must not be dropped silently.
- `python scripts/validate_project.py` passes:
  - legacy migration-only JSON files=9, effective cards=27, encounters=6.
  - CSV schema, production tables, fixtures, IDs, references, behavior/effect allowlists, UTF-8 and staging deps pass.
- `.clang-format` was run with Visual Studio LLVM clang-format on changed C++ files.
- `scripts/ue/Build-Editor.cmd -Configuration Development` passes with UE 5.8 installed build.
- `scripts/ue/Run-Automation.cmd -Filter ReEcho` passes. Log evidence: 19 automation tests found and completed successfully, including 4 new `ReEcho.Data.*` tests.
- `python scripts/ue/package_windows.py --smoke-seconds 5` passes clean Win64 Shipping BuildCookRun and packaged process stayed alive for 5 seconds.
- Packaged NonUFS manifest contains:
  - `ReEcho/Content/Data/csv_schema.csv`
  - `ReEcho/Content/Data/reecho_data_manifest.csv`
  - `ReEcho/Content/Data/runtime_smoke.csv`
  - `ReEcho/Content/Data/runtime_smoke_effects.csv`
- `git diff --check` passes.

### Final public contract for Plans 19–21

- Use `FReEchoCsvDataRegistry` as the single CSV runtime entry point. Load complete domain tables into temporary structures, validate every required table and reference, then publish one immutable snapshot.
- Do not query or mutate CSV files directly from actors, widgets or gameplay subsystems.
- Add new C++ behavior IDs and effect kinds through `RegisterBehaviorId` / `RegisterEffectKind` before loading a table that references them.
- Keep formulas, scripts and natural-language parameter blobs out of CSV. Use finite typed values, `BehaviorId`, `EffectKind`, `ParamName`, `Add` / `Multiply` / `Override` and one-to-many child tables.
- Percent fields are decimal values. Distance/time units must remain in column names, for example `DistanceCm` and `DurationSeconds`.
- Preserve existing runtime IDs such as `J_SPADE`, `J_HEART`, `J_DIAMOND`, `J_CLOVER`, `J_CAT`, `W_J_01` etc. Workbook aliases like `J_01`-`J_04` must be handled by an explicit alias/migration table in the domain migration plan; do not silently renumber saved recordings, builds or config references.
- Legacy JSON remains read-only migration material until a later plan atomically switches that whole domain to CSV.

### Deviations from initial plan

- The main baseline commit did not yet contain Plan18, so the plan file was copied into the isolated execution branch from the planner working tree and then updated there for execution notes.
- Plan18 does not migrate any character/build/element/weapon domain values. The only production runtime table is the minimal smoke table required to prove loader, validator, fixture and packaging behavior.

### Remaining risks

- No known automated blocker remains for the Plan18 foundation.
- The current module startup uses fatal failure for invalid production CSV. This is deliberate for Plan18's no-partial/no-fallback policy, but later UX may add an editor-facing diagnostics panel before fatal startup.
- Domain tables for Plans 19-21 still need their own schemas, foreign keys and alias migration policy.

### Human validation requested

- Edit `Content/Data/runtime_smoke.csv` `RUNTIME_SMOKE.TestScalar`, restart the project and confirm the registry/automation sees the new value without recompiling C++.
- Confirm `Content/Data/README.md` and CSV error messages are readable enough for designers to locate row and field mistakes.

## 执行者启动 prompt

```text
你是 ReEcho 项目的执行者。先读 AGENTS.md，并按它的最小读取顺序读取；再读 plans/18-data-csv-runtime-foundation.md 和 shared/LESSONS.md §META 中与 worktree/可复现源码相关的条目。只有遇到实际失败诊断时才读 §DEBUG。

在独立 worktree 的 `plan/18-csv-foundation` 分支开发，策划参考工作簿位于仓库上一级 `../回响肉鸽数值与构筑体系.xlsx`。任务：建立唯一、可验证、可打包的 CSV 运行时基础，不迁移 Plan 19–21 的完整领域内容。

验收照 Plan 18 的 🔒 清单。公共类型、类名和加载细节由你根据当前 UE 5.8 代码决定；但不得维护多真源、不得执行任意公式、不得静默回退、不得手改 `.uasset`/`.umap`，也不得碰当前用户已有的资产改动。把最终公共 API、schema、打包路径、ID 映射和所有偏差写进 Plan 18 的 Execution notes，供 Plan 19–21 冷启动读取。

完成后只在本分支显式提交/push并告诉人，禁止修改或合并 main。需要运行 UE 前先按项目规则确认编辑器状态。
```
