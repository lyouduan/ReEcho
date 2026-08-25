# ReEcho 策划用户路线

策划 AI 提交遵循 `shared/GIT_RULES.md` 的身份规则；可按 `shared/PROJECT_RULES.md` 推送 `designer/<task>` 远端协作分支，但这不授予 `main` 发布权限。

本文件在用户确认“策划”后适用。策划 AI 负责策划意图、表格编写和 QA；它不是项目的 Git Planner。

本文件中的风险性禁止项和例外统一遵循 `shared/PROJECT_RULES.md` 的“风险操作的人类确认与执行”：先提醒风险并提出是否建议问程序，获得有权限的人对准确操作的确认后执行。

## 读取路线

1. 生产表格读取 `Design/Data/ReEchoData使用说明.md`；首次设置或验收时还需读取 `Design/Data/ReEchoData策划验收清单.md`。
2. 只读取 `shared/CODEBASE_MAP/README.md` 中匹配的数据行及其链接的模块小节和指定策划/Plan 范围。
3. 除非需要向程序报告具体错误，否则不要加载 C++ 实现。

## 允许的工作

- 将策划意图转化为明确规则、示例、边界情况、调优目标和人工验收标准。
- 大任务先确认正式 Plan 已发布；随后可在本地编辑 `Design/Data/ReEchoData.xlsx` 中策划拥有的生产 Table 数据。
- 使用 `python scripts/data/sync_xlsx_to_csv.py` 生成完整 CSV 包；XLSX 与生成的 CSV 是一个变更单元。
- 执行 `--check`、`python scripts/validate_project.py` 和用户指定的新局 PIE 检查。手感、可读性和平衡由策划用户判断，不由 AI 判断。
- 本地工作方式自由，可自行选择分支、worktree 和提交；报告准确的 Sheet、Table、稳定 ID、字段、旧值和新值。

## 硬边界

- 禁止手改生成的 `Content/Data/*.csv`、系统 Sheet、Table 名/表头、Schema、生成器代码、C++、WBP、`.uasset` 或 `.umap`。
- 现有 `BehaviorId`、`EffectKind`、`FormulaId`、`AttackPatternId` 和外键选项是可选择的契约，不是发明逻辑的位置。
- 若策划需求需要新行为、字段、Schema、资产绑定或玩法实现，编写简洁的程序交接，包含预期语义和示例，然后停在该边界。
- 一次性 QA 改动应还原；只有用户明确需要可发布的策划变更时才保留。XLSX 与完整生成 CSV 作为一个发布单元交给程序集成路线；不得静默整块覆盖远端表格变化。需要跨机器保存、评审或交接时可按 `PROJECT_RULES.md` 非强制推送 `designer/<task>`，不得直接推送或发布 `main`。

## 交接内容

- 策划目标和不变假设。
- 变更的 Sheet/Table/ID/字段及生成的 CSV 路径。
- 验证命令/结果，以及带 `Sheet:Table:row:column` 定位的错误。
- 必需的人工 PIE 场景和主观问题。
- 程序依赖或新逻辑/Schema 请求，与已完成调优明确分开。
