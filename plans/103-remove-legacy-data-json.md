# Plan 103 - 程序 - 清理迁移期旧 JSON 数据

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`NotRequired`。
- 本地规划 / 实现基线：`origin/main@32e2ce0d2ea80e64d94b3e1692e8961f57dff437`。
- 本地实现方式：Plan 单独发布后，从最新 `origin/main` 创建独立 `plan/103-remove-legacy-data-json` worktree。
- 依赖 / 阻塞：依赖 Plan21–24 已完成的 XLSX/CSV 迁移；Plan98 的选卡与 GM 候选独立，不读取或修改这些 JSON。
- Writes:
  - `plans/103-remove-legacy-data-json.md`
  - 删除 `Content/Data/{global_balance,characters,weapons,cards,encounters,elements,reactions,statuses}.json`
  - `scripts/validate_project.py`
  - `Content/Data/README.md`
  - `shared/PROJECT_RULES.md`
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - 最终 FullRebuild 刷新的 `Binaries/Win64/` 精选预构建包
- Stable Reads:
  - `Design/Data/*.xlsx` 与 `Content/Data/*.csv`
  - `plans/21-data-csv-runtime-foundation.md`
  - `plans/22-data-character-build-csv.md`
  - `plans/23-data-elements-reactions-csv.md`
  - `plans/24-data-weapons-slots-csv.md`
  - `shared/GIT_RULES.md`
- 影响模式：`SharedContract`。不改变生产数值或运行时 API，但收紧数据目录与项目校验契约，旧 JSON 路径不再允许回归。
- 兼容承诺 / 下游操作：存档迁移不依赖这些 JSON；当前 XLSX/CSV、配置、运行时加载与打包行为保持不变。Git 历史继续提供旧快照审阅能力。
- 明确排除：不删除动画碰撞标注等其他用途 JSON，不修改生产 XLSX/CSV，不借清理调整数值，不清理测试 fixture 或旧 Plan 历史文字。

## 锁定目标

删除已退出运行时权威的 8 个迁移期数据 JSON，消除旧 6 关、旧角色、旧武器和旧卡牌快照造成的误读。校验器不再验证旧内容，改为明确禁止这些具名 JSON 返回；生产数据继续只由可见 XLSX 与生成 CSV/配置负责。

## 架构影响与设计决策

- 受影响架构标识：`AREA-Data`、`AREA-Run` 的数据入口说明；不修改 Runtime Module 依赖或状态所有者。
- 对应模块文档：维护 `MOD-ReEcho.md` 的数据链与禁用项，明确迁移期 JSON 已物理删除。
- 设计意图：历史快照应由 Git 历史承担，不应和生产 CSV 并列放在 `Content/Data`，更不能让项目校验输出旧统计造成第二事实来源错觉。
- 权威状态与依赖：XLSX 仍是策划编辑权威，生成 CSV 仍是运行时权威；校验器由“要求旧 JSON 存在并检查旧值”改为“要求旧 JSON 不存在”。
- 决策记录：不迁移到新的 archive 目录，因为 Git 已保存完整历史，新增 archive 仍会保留可被误读的第二份数据；只对 9 个已迁移的具名数据 JSON 建回归禁令，不禁止其他明确用途 JSON。
- 相关文档同步范围：更新 `PROJECT_RULES.md`、`ARCHITECTURE.md`、`Content/Data/README.md` 与 `MOD-ReEcho.md`；`CODEBASE_MAP/README.md` 的模块路由不变，关闭前具名审阅。
- 关闭前逐项填写审阅结果：
  - `PROJECT_RULES.md`：待更新已迁移 JSON 的物理删除与禁止回归规则；
  - `ARCHITECTURE.md`：待更新数据流水线事实；
  - `Content/Data/README.md`：待补充目录不接受迁移期 JSON；
  - `MOD-ReEcho.md`：待更新数据链和旧数据禁令；
  - `CODEBASE_MAP/README.md`：待审阅是否需要修改 AREA 路由。

## 锁定验收

- [ ] 8 个迁移期 JSON 已从工作树删除，`enemies.json` 继续不存在，Git 历史可恢复旧内容。
- [ ] 运行时、Build.cs、配置和数据同步脚本不存在对上述 JSON 的读取或打包引用。
- [ ] `validate_project.py` 不再解析/统计旧内容，并明确拒绝 9 个已迁移具名 JSON 回归。
- [ ] 权威 XLSX/CSV 字节同步检查与现有 CSV schema/fixture 校验继续通过。
- [ ] 最终 Development FullRebuild、项目校验、预构建检查和 `git diff --check` 通过。
- [ ] 未提交精选允许列表外 UE 生成物、机器本地路径或无关 Plan98/99–102 修改。

## Step 0 门禁

- 基线分支/提交：`origin/main@32e2ce0d`；fetch 后本地 `main` 与远端一致，无外部提交。
- 引擎/构建可用性：UE 5.8 与标准 FullRebuild 脚本可用；实现发布前执行完整门禁。
- 现有聚焦测试结果：规划阶段 `validate_project.py` 仍通过，但其首行暴露旧 JSON `encounters=6`，即本任务要移除的误导输出。
- 共享契约 / 难合并资源风险：`scripts/validate_project.py` 与项目规则是共享热点；Plan98 不改这些文件，当前已知 Plan99–102 也不以旧 JSON 为权威。
- 基线损坏时的停止条件：发现任何生产 C++、打包配置或同步脚本仍读取目标 JSON，停止删除并先更新 Plan/上报兼容需求。

## 实现提纲

1. 用全仓引用审计确认 8 个 JSON 只被校验器和历史文档引用。
2. 删除目标文件，把校验器改为具名禁止回归检查并修正成功输出。
3. 同步当前数据源、规则与 Codebase Map 文档；不改历史 Plan。
4. 运行 XLSX/CSV 检查、项目校验、FullRebuild、预构建检查和文本门禁。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 引用审计 | `rg` 目标 JSON 名称及 `RuntimeDependencies` | 生产运行时和打包链无引用 |
| 数据 | `python scripts/data/sync_xlsx_to_csv.py --check` | 权威工作簿与生产 CSV 一致 |
| 静态 | `python scripts/validate_project.py` | 旧 JSON 不存在且 CSV/规则/工作流校验通过 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终组合源码与精选预构建包一致 |
| 收尾 | `python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 包指纹与文本门禁通过 |

## 执行记录

### 变化

- 2026-08-25：用户确认清理 8 个迁移期 JSON。只读审计确认生产运行时没有读取；旧文件仅由 `validate_project.py` 强制解析，并在输出中把旧 6 场快照误呈现为统计。

### 证据

- 当前生产 `encounters.csv` 为 8 场，旧 `encounters.json` 为 6 场；运行时 `GetTotalEncounterCount()` 读取启用 CSV 行，默认配置也是 8。

### 剩余风险

- 删除是 Git 可恢复操作；主要风险是外部私人脚本仍可能引用旧路径，但仓库内没有该依赖，且旧数据本身早已不是权威。

### 人工验收结果/请求

- `NotRequired`：纯数据卫生与校验契约清理，无视觉或手感验收。

### 架构文档审阅结果

- 待实现后填写。
