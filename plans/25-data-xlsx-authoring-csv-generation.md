# Plan 25 - data - XLSX authoring and deterministic CSV generation

## Coordination

- Planner owner: Gavyn-side Planner.
- Intended executor: unassigned; start only after a separate human handoff.
- Status: `Proposed`.
- Planning ref: `origin/coord/planning-broadcast-20260811`.
- Expected implementation branch: `plan/25-xlsx-authoring`; base will be the locally accepted main containing final Plan 24.
- Depends on: accepted Plans 21–23 and final Plan 24 weapon schema. The generator framework and non-weapon sheet analysis may start earlier, but production weapon CSV generation must wait for Plan 24 schema freeze.
- Blocks: unified data-generation verification and the deferred combined Human PIE pass.
- Writes: canonical XLSX authoring artifact/location, deterministic generator and check mode, generated production CSVs, manifest/schema generation validation, data documentation, fixtures and packaging verification.
- Reads: all accepted typed CSV schemas/readers and `回响肉鸽数值与构筑体系.xlsx` as the current design source/reference.
- Shared-contract impact: every migrated production data table becomes generated output. Runtime Unreal code continues to consume validated CSV and must not gain an XLSX or Excel dependency.
- Downstream guidance: other Planners may continue UI/gameplay work through typed read-only APIs. Do not independently establish another workbook schema, hand-edit generated CSV as a competing truth, or change shared stable IDs without coordination.
- Explicit exclusions: live Excel automation, runtime `.xlsx` parsing, balance redesign, new gameplay behavior, UI redesign and unrelated asset changes.

## Locked goal

让策划以一个版本化的 XLSX 工作簿作为角色、构筑、元素、反应、武器和插槽领域的唯一编辑源，通过确定性工具生成当前 Unreal 运行时使用的严格 CSV 包；生成失败不污染生产数据，生成结果可复查、可在 CI/本地检查，并保持 Plans 21–24 已验收的稳定 ID、行为和存档兼容语义。

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

## Locked acceptance

### Automated

- [ ] 版本库内有明确、稳定、无本机绝对路径的 canonical XLSX 位置；工作簿版本和每个 sheet 的 schema/version 可识别。
- [ ] 覆盖 Plans 21–24 的生产领域：runtime smoke、characters/aliases、cards/effects、elements/statuses/reactions、weapon types/weapons/attack steps、slot types/profiles、parts/effects；纯 manifest/schema 元数据可由工具生成或一致性校验。
- [ ] 每个 sheet 使用稳定表名、机器可读字段行和策划说明；稳定 ID、外键、枚举、单位、必填/可选、enabled/disabled/DisabledReason 与既有 CSV 契约一致。
- [ ] 生成器使用仓库内声明的 Python 依赖或项目已配置的可复现运行时；不要求打开 Excel，不使用 COM，不写用户目录。
- [ ] 生成顺序、行顺序、浮点/布尔格式、UTF-8、换行和 CSV quoting 确定；相同 XLSX 连续生成得到字节一致结果。
- [ ] 生成先写经过校验的临时完整包，全部成功后才原子替换目标；任一 sheet/schema/外键/handler/范围失败时，生产 CSV 零部分更新并清楚报告 sheet、row、column。
- [ ] 提供 `generate` 和只读 `check` 模式；`check` 能识别 XLSX 与已提交 CSV 漂移、意外手改、缺表和陈旧生成物，且不会修改文件。
- [ ] 初次迁移生成的 CSV 与 Plan 24 最终生产数据在稳定 ID、enabled 批次和运行时数值语义上等价；任何有意格式/顺序变化记录在 Execution notes。
- [ ] 现有生产基线＋局部覆盖 fixtures 继续工作；fixture 不要求复制整本 XLSX，也不能被生产生成器误覆盖。
- [ ] 工作簿源行追踪可复查，尤其保留武器插槽 78 行审计、62 条无名 disabled 行和批准启用批次。
- [ ] 武器领域只有在 Plan 24 最终 schema/hash 契约冻结后接入；生成结果不得改变三个旧热键、三个 StartSelectable、`W_J_04` 或历史 WeaponId 语义。
- [ ] 静态验证将 XLSX→CSV check 纳入 `scripts/validate_project.py` 或等价快速入口；失败证据区分“工作簿错误”“生成漂移”“运行时 schema 错误”。
- [ ] 干净 clone 可执行生成/check、Editor Development build、全量 `ReEcho.*`、Shipping Cook/Pak/Archive CSV staging smoke 和 `git diff --check`。

### Human unified verification

- [ ] 策划在 XLSX 修改一个角色数值、一个卡牌数值、一个反应数值、一个武器数值和一个配件数值，生成 CSV 后无需重新编译即可在新 Run 生效。
- [ ] 统一回归开始/继续、局中保存退出、角色/武器选择、抽卡→商店、武器切换、玩家/回响、元素反应和代表性配件。
- [ ] 策划能从生成错误直接定位到工作表和单元格，不需要阅读 C++ 或 Python 堆栈。

## Step 0 gates

1. 读取 Plans 21–24 最终 Execution notes、全部生产 manifest/schema、领域读取器和 `scripts/validate_project.py`；记录最终表集合、列契约和稳定 ID，不凭旧工作簿猜测。
2. 检查仓库与工作区中所有 `.xlsx`，确认 canonical 文件策略、是否需要从现有 `回响肉鸽数值与构筑体系.xlsx` 派生项目内版本，以及二进制文件的 Git 体积/更新方式；有歧义先交回 Planner/人。
3. 在 Exchange 认领 canonical XLSX、生成脚本、生产 CSV、manifest/schema 和静态验证入口。若 Plan 24 仍在修改武器 schema，只做生成框架与 Plans 21–23 sheet，不写最终武器导出。
4. 先用一个小 sheet 打通 XLSX→临时 CSV→既有 reader/validator→原子发布，再扩展全部领域。
5. 初次全量生成前保存已接受 CSV 的语义快照，逐表比较 ID、外键、enabled 批次和数值；不能用“大量格式变化”掩盖语义漂移。

## Implementation outline

1. 定义工作簿布局和版本元数据，复用现有 CSV 列契约，不制造另一套同义字段。
2. 实现通用 sheet reader、类型解析、稳定排序、CSV writer、临时包和 check/diff 报告。
3. 先迁 accepted Plans 21–23 sheets，再接 Plan 24 最终武器/插槽 sheets。
4. 接入静态验证、依赖声明、文档和干净 clone/打包验证。
5. 完成统一自动化后交给人做一次集中 PIE，而不是每张表重复人工验收。

## Risks and exclusions

- XLSX 是二进制文件，Git 无法友好合并；同一 canonical workbook 是 Active ownership 独占资源，其他 Planner 通过公告协调，不并行编辑同一文件。
- 不把所有逻辑做成万能表解释器；XLSX 只配置已注册行为及其类型化参数。
- 不在本 Plan 顺手重平衡现有数值。发现源表与已验收 CSV 冲突时先列差异，由人决定权威值。
- 不让生成工具依赖个人路径、已登录 Excel、网络服务或不可复现的插件。

## Recommended executor model

强模型。工作跨二进制 authoring、确定性生成、全部数据 schema、原子发布、迁移对比和打包验证；可把纯 sheet 映射机械工作交给较便宜模型，但公共生成器与验收由强模型负责。

## Execution notes

### Changed

### Evidence

### Workbook and schema decisions

### Intentional CSV differences

### Remaining risks

### Human validation requested

## 执行者启动 prompt

```text
你是 ReEcho Plan25 执行者。只有人在 Plan25 Coordination 状态改为 Ready 并给出最终 base 后才开工。先按 AGENTS.md 最小顺序读取 shared/PLANNER_EXCHANGE.md 的公告/所有权、plans/25-data-xlsx-authoring-csv-generation.md、Plans 21–24 最终 Execution notes 和相关数据读取器/校验入口。

从指定最终 base 创建 `plan/25-xlsx-authoring` 独立分支/worktree，按 Coordination 认领 canonical XLSX、生成器、生产 CSV、manifest/schema 和校验入口。任务是建立 XLSX 唯一策划编辑源→确定性/原子生成严格 CSV→既有 Unreal 快照的完整链路；保持稳定 ID、启用批次、运行时语义和打包方式。Plan24 武器 schema 未冻结时只做通用框架和 Plans21–23 领域，不擅自定义另一套武器表。

验收照 Locked acceptance。禁止运行时读取 XLSX、禁止 COM/Excel 依赖、禁止手改生成 CSV 成为第二真源、禁止修改玩法或 `.uasset/.umap`。完成后更新 Execution notes，提交并推送自己的任务分支；禁止合并 main。
```
