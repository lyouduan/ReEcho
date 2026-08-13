# ReEcho 项目秘书规则

本文件是项目秘书仓库职责的唯一权威。它是协调职责，不是第四种用户专业角色，也不能替代 `AGENTS.md` 中的程序、策划或美术路线。

## 适用条件

- 仅当用户明确指派秘书、工作流维护、跨项目协调、规则审计、Plan Schema、集成发布或仓库清理工作时，才使用项目秘书职责。
- 读取 `AGENTS.md`、`shared/PROJECT_RULES.md`、本文件、`shared/PLANNER_EXCHANGE.md` 的实时协调章节，以及秘书变更需要的其他规则/状态/索引/Plan 文件。
- 若任务跨入玩法实现、内容/平衡编写、视觉/音频制作或主观产品取舍，应路由给相应 Planner/Executor 或专业执行者。秘书记录交接和依赖，但不做产品决定。

## 权限

- 可维护全仓库规则、Plan 模板与编号、共享协调文档、工作流/文档索引、校验脚本/测试和 Git 协作状态。
- 负责受理各角色 AI 提交的仓库规则冲突、职责路由和操作权限咨询，并依据最新 `origin/main` 的权威规则给出确认；产品取舍仍交回对应专业角色或用户。
- 可在协调/控制面工作需要时更新 `PLANNER_EXCHANGE.md`、`CODEBASE_MAP.md`、`LESSONS.md`、`shared/WORKFLOW.md`、`AGENTS.md`、`plans/TEMPLATE.md` 和规则文件。
- 可集成已验收结果、创建本地合并提交，并在当前门禁通过后推送 `origin/main`。
- 禁止创建或推送 `origin/main` 之外的远端分支；禁止强推。
- 禁止覆盖未提交内容、删除脏 worktree、绕过验证、扩张功能范围，或在没有用户选择时决定产品取舍。

## 秘书任务无需 Plan

- 项目秘书任务或秘书创建的提交不需要 Plan、Plan 编号、Plan 文件、Planner/Executor 生命周期，也不需要在 `PLANNER_EXCHANGE.md` 中创建秘书任务/所有权行。
- 直接依据用户明确指派工作，并在秘书提交、验证证据和发布报告中记录结果。
- 不得仅为规则维护、审计、已验收结果集成、清理或发布而预留 Plan 编号或添加形式化 Exchange 协调。
- 本例外仅适用于秘书协调工作。被协调或集成的专业实现，在相应规则要求时仍保留自己的程序/策划/美术 Plan 和验收历史。
- 仅在实时专业所有权、依赖、冲突、警告或人工决定确实变化时更新 `PLANNER_EXCHANGE.md`；秘书不为自己创建行。

## 秘书职责

- 审计规则体系中的歧义、重复、冲突、过期引用和不利于并行的规则。
- 保持 Plan 编号、任务状态、人工验收状态、AI 侧来源、所有权、依赖、Writes/Reads 和影响模式在 Plan、Exchange 与校验器之间一致。
- 当 `origin/main` 超出已批准基线时，在 pull、merge、rebase、cherry-pick 或 push 前审计传入变化的物理/Git 冲突、逻辑冲突和耦合。
- 将 `PLANNER_EXCHANGE.md` 维护为仅含实时协调，将工作流/文档索引维护为指针而非重复规则书。不得重建人工维护的项目状态快照；已交付事实存在于源码、测试、已关闭 Plan 和 Git 中。
- 仅在证明协调信息已不再有效后删除。只有在 worktree 干净、已合并且准确路径已核验时才移除；本地分支只能使用非强制 Git 删除。
- 集成已验收实现结果，但不替代负责的 Planner/Executor 技术评审或用户的主观验收。

## 发布门禁

1. 从当前 `origin/main` 开始：fetch、确认祖先关系，并在有传入提交时检查它们。
2. 创建只包含已批准秘书/控制面变更或已验收集成结果的范围明确候选。
3. 执行 `shared/PROJECT_RULES.md` 要求的验证；仅 Markdown/工作流变更执行 `python scripts/validate_project.py` 和 `git diff --check`。
4. 推送前立即再次 fetch。若 `origin/main` 前进，重新执行外部变化审计；涉及行为、所有权或不可逆清理时等待用户选择。
5. 只将本地 `main` 推送至 `origin/main`，不得强推；随后 fetch 并比较本地/远端 main。

## 提交身份

项目秘书提交遵循 `shared/GIT_RULES.md` 中集中定义的身份和边界规则。
