# ReEcho 策划用户路线

策划 AI 提交遵循 `shared/GIT_RULES.md` 的身份规则；可按 `shared/PROJECT_RULES.md` 推送 `designer/<task>` 远端协作分支，但这不授予 `main` 发布权限。

本文件在用户确认“策划”后适用。策划 AI 负责策划意图、表格编写和 QA；它不是项目的 Git Planner。

本文件中的风险性禁止项和例外统一遵循 `shared/PROJECT_RULES.md` 的“风险操作的人类确认与执行”：先提醒风险并提出是否建议问程序，获得有权限的人对准确操作的确认后执行。

策划路线的文件范围不是本地写权限：策划 AI 可在本地读取、创建、修改、编译和试验仓库内任何文件。文件类型不会阻止本地工作；真正的强制边界是策划不得直接推送或合并 `origin/main`，进入 main 必须交由程序集成路线或项目秘书审计发布。本地试改不自动扩大策划的产品、程序或美术决策权。

## 读取路线

1. 生产表格读取 `Design/Data/ReEchoData使用说明.md`；首次设置或验收时还需读取 `Design/Data/ReEchoData策划验收清单.md`。
2. 只读取 `shared/CODEBASE_MAP/README.md` 中匹配的数据行及其链接的模块小节和指定策划/Plan 范围。
3. 默认只加载完成当前任务所需的最小上下文；为定位、验证或试验可继续读取并修改 C++、资产、工具或其他任意本地文件。

## 允许的工作

- 将策划意图转化为明确规则、示例、边界情况、调优目标和人工验收标准。
- 大任务先确认正式 Plan 已发布；随后可在本地编辑 `Design/Data/ReEchoData.xlsx` 中策划拥有的生产 Table 数据。
- 使用 `python scripts/data/sync_xlsx_to_csv.py` 生成完整 CSV 包；XLSX 与生成的 CSV 是一个变更单元。
- 执行 `--check`、`python scripts/validate_project.py` 和用户指定的新局 PIE 检查。手感、可读性和平衡由策划用户判断，不由 AI 判断。
- 本地工作方式自由，可自行选择分支、worktree 和提交；报告准确的 Sheet、Table、稳定 ID、字段、旧值和新值。
- 可在本地直接试改 C++、生成器、Schema、WBP、`.uasset`、`.umap` 或其他任何文件，也可按 `PROJECT_RULES.md` 将需要跨机保存或交接的候选非强制推送到 `designer/<task>`。

## 硬边界

- 禁止策划路线直接推送、合并或发布 `origin/main`；需进入 main 的本地提交或 `designer/<task>` 候选统一交由程序集成路线或项目秘书审计、验证和发布。
- 现有 `BehaviorId`、`EffectKind`、`FormulaId`、`AttackPatternId` 和外键选项是可选择的契约，不是发明逻辑的位置。
- 若策划需求需要新行为、字段、Schema、资产绑定或玩法实现，可本地制作或验证原型；交接时必须说明预期语义、示例、试验性质和需要程序/美术确认的部分。
- 一次性 QA 改动应还原；只有用户明确需要可发布的策划变更时才保留。XLSX 与完整生成 CSV 作为一个发布单元交给程序集成路线；不得静默整块覆盖远端表格变化。需要跨机器保存、评审或交接时可按 `PROJECT_RULES.md` 非强制推送 `designer/<task>`，不得直接推送或发布 `main`。

## 交接内容

- 策划目标和不变假设。
- 变更的 Sheet/Table/ID/字段及生成的 CSV 路径。
- 验证命令/结果，以及带 `Sheet:Table:row:column` 定位的错误。
- 必需的人工 PIE 场景和主观问题。
- 程序依赖或新逻辑/Schema 请求，与已完成调优明确分开。
