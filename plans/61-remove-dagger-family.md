# Plan 61 - 程序 - 移除匕首武器整族（类型/实例/步骤/配件/槽位/测试）

## 协调

- Planner 负责人：Gavyn-side AI（本项目 Planner + 秘书）
- Executor 负责人：Gavyn-side AI（同 AI 在用户 程序 指导下执行）
- Plan 编写方（AI 侧）：`Gavyn-side AI | ReEcho teammate-side AI`
- 实现编写方（AI 侧）：`Gavyn-side AI`
- 任务状态：`Proposed`
- 人工验收：`PendingBeforeClose`（删除后需确认 3 把可玩武器 + 其余武器配件/构筑套件仍可玩、测试套件通过）
- 本地规划 / 实现基线：`origin/main @ 1ed2615`（fetch 确认本地 main 与 origin/main 一致，无基线外提交）
- 本地实现方式：一任务一 worktree（`plan/61-remove-dagger-family`），主工作树仅快进合并；实现先在 worktree 完成并自验，再快进进 main。
- 依赖 / 阻塞：无外部依赖；需用户确认 canonical `ReEchoData.xlsx` 匕首行的处理方式（本 Plan 采用物理删除 canonical 匕首行 + CSV 整族删除，开普勒那份保留隐藏状态，不再同步）。
- Writes:
  - `Content/Data/weapon_types.csv`（删除 `Dagger` 类型行）
  - `Content/Data/weapons.csv`（删除 `W_J_05` Training Dagger 行）
  - `Content/Data/attack_steps.csv`（删除 `AS_DAGGER_1/2/3` 与 `AS_DAGGER_DASH_ONLY` 行）
  - `Content/Data/slot_profiles.csv`（删除 `SP_DAGGER_CORE/GRIP/BLADE` 行）
  - `Content/Data/parts.csv`（删除 8 行 `P_DAGGER_*`）
  - `Content/Data/part_effects.csv`（删除 4 行 `PE_DAGGER_*`）
  - `Content/Data/csv_schema.csv`（删除 5 处枚举：`Dagger`、`Pattern.DaggerCombo`、`Pattern.DaggerDashOnly`、相关槽位/配件枚举项）
  - `Design/Data/ReEchoData.xlsx`（`武器体系W` 第4行 `Dagger/匕首` 物理删除；`武器插槽C` 中匕首专属槽位删除；`_SystemData` 中 Dagger 枚举项删除）
  - `Source/ReEcho/Private/Weapons/Tests/ReEchoWeaponRuntimeTests.cpp`（改写/删除 `W_J_05`/`P_DAGGER_*` 相关测试：含 `ReEcho.Weapons.DaggerPartsReplacePatternMoveInvulnerableAndHealOnKill` 整个专属测试类及其 2 个 Fixture）
  - `Source/ReEcho/Private/UI/ReEchoInventoryShopWidget.cpp`（匕首配件图标 `FObjectFinder` 分支 108-144 移除或改判空，避免孤立引用）
  - `Source/ReEcho/Private/Combat/ReEchoCombatVfxCatalog.cpp`（第33行 `Pattern.Contains("Dagger")` 分支：数据删除后该分支恒 false，可保留或清理，本 Plan 保留以兼容潜在回归）
- Stable Reads:
  - `Source/ReEcho/Private/Data/ReEchoWeaponCsvReader.cpp`（CSV 校验入口；确认删除后无悬空外键报错）
  - `scripts/data/sync_xlsx_to_csv.py`（确认 canon 删除后 sync 不再复活匕首；`LOCKED_REFERENCE_SHEETS` 无需改）
  - `Source/ReEcho/Private/Weapons/Tests/ReEchoWeaponRuntimeTests.cpp`（确认其余武器测试不受影响）
- 影响模式：`SharedContract`（涉及 CSV schema 枚举 + 自动化测试套件契约；数据表面为 `Isolated`，测试为 `SharedContract`）
- 兼容承诺 / 下游操作：不改动 `sync_xlsx_to_csv.py` 读取逻辑与 `LOCKED_REFERENCE_SHEETS`；不影响长剑/月杖/元素球/学徒杖 4 把现有武器（含已启用 3 把）的数据与运行；不删除非匕首任何配件/槽位；不改动 `FReEchoWeaponLogic`/`AReEchoWeaponActor` 逻辑代码。
- 明确排除：不修改武器攻击执行逻辑（近战/投射/光波三路）；不修改 `FReEchoWeaponLogic` 节拍；不改动非匕首武器的 CSV 行；不处理"开普勒"xlsx（保留其隐藏状态）；不改动 MOD-ReEchoWeapons / MOD-ReEchoCombat 模块职责。

## 锁定目标

将匕首武器整族从工程真源与运行时中彻底移除：canonical `ReEchoData.xlsx`（武器体系W/武器插槽C/_SystemData）+ 8 个 `Content/Data/*.csv` + 源码中的匕首专属测试与 UI 引用。移除后：
- `validate_project.py` 通过（无悬空外键、无 schema 残留）；
- `Run-Automation -Filter ReEcho.Weapons` 通过（改写后的测试套件无编译/断言失败）；
- 当前 3 把可玩武器（长剑/月杖/元素球）+ 学徒杖的数据与运行不受影响；
- 下次 `sync_xlsx_to_csv.py` 不会从 canonical 复活匕首。

## 架构影响与设计决策

- 受影响架构标识：数据层 `AREA-Data`（CSV/xlsx 真源）与测试 `AREA-Test`；逻辑/表现层 `MOD-ReEchoWeapons`/`MOD-ReEchoCombat` 不变。无新 `MOD-*` 标识。
- 对应模块文档：`MOD-ReEchoWeapons.md`/`MOD-ReEchoCombat.md` 无需改写（逻辑不变）。`CODEBASE_MAP/README.md` 若武器计数文字涉及则更新，否则关闭时说明"无需修改"。
- 设计意图：匕首在"开普勒"被策划隐藏（hidden 行），视为废弃；canonical 与其不同步，本次以 canonical 为准对齐删除，使数据真源单一、不再保留死武器。同时明确"开普勒"与 canonical 双份真源的现状风险（见剩余风险）。
- 权威状态与依赖：不改变模块职责、公共契约、依赖方向；仅删除数据与测试。
- 决策记录：
  - 选用"整族删除（含配件+改测试）"（用户 q2 确认）：因匕首是配件/构筑体系**唯一完整测试覆盖**的武器，仅删武器层会让 `parts/slot_profiles` 的 `P_DAGGER_*`/`SP_DAGGER_*` 成为孤立数据，且 `ReEchoWeaponRuntimeTests.cpp` 大量 `W_J_05`/`P_DAGGER_*` 测试会编译/断言失败。整族删除使 `validate + 自动化` 仍能通过。
  - 选用"直接改 canonical 即可"（用户 q1 确认）：`sync_xlsx_to_csv.py` 的 `CANONICAL_XLSX` 指向 `ReEchoData.xlsx`，删除 CSV 不删 canon 会被 sync 复活；故必须先在 canon 删除匕首行。"开普勒"那份保留隐藏状态，不再同步（避免无谓双份编辑）。
  - 已知代价：删除后，配件/构筑体系的自动化测试覆盖从"匕首独占"退化为"通用配件 + 其他武器"，需确认其余武器配件测试仍能代表该子系统（见剩余风险）。
- 相关文档同步范围：
  - `ARCHITECTURE.md`：无拓扑变化 → 关闭时审阅说明"无需修改"。
  - `README.md`（CODEBASE_MAP 索引）：无路由变化 → 关闭时审阅说明。
  - `modules/MOD-ReEchoWeapons.md`：审阅，若武器计数/配件示例涉及匕首则更新，否则说明"无需修改"。
- 关闭前逐项填写审阅结果（见末尾"架构文档审阅结果"）。

## 锁定验收

- [ ] 功能结果有可观察证据：3 把可玩武器 + 学徒杖数据完整，游戏可启动并选择武器正常。
- [ ] `python scripts/validate_project.py` 通过（无悬空 WeaponTypeId / WeaponId / SlotProfile / Part / Pattern 枚举）。
- [ ] `scripts/ue/Run-Automation.cmd -Filter ReEcho.Weapons` 通过（改写后测试套件编译+运行无失败）。
- [ ] 本地增量 `scripts/ue/Build-Editor.cmd -Configuration Development` 通过（UHT/UBT 退出码 0）。
- [ ] 推 `origin/main` 前 `-FullRebuild` + 精选预构建包刷新 + `validate` + `git diff --check` 通过。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。
- [ ] 删除后再次运行 `sync_xlsx_to_csv.py` 确认匕首不复活（或确认 canon 已无匕首行）。

## Step 0 门禁

- 基线分支/提交：`origin/main @ 1ed2615`（fetch 确认无 ahead/behind，无基线外提交）
- 引擎/构建可用性：Editor 可启动，Development 构建可用（Plan59/60 已验证门禁链路）
- 现有聚焦测试结果：`ReEcho.Weapons.*` 当前须记录基线（实现前先跑一次确认绿灯，作为删除后对照）
- 共享契约 / 难合并资源风险：CSV 为文本，xlsx 为二进制但仅本 Plan 改 canon；避免与 Plan60（UI 换皮）在同一 `Content` 区域冲突（CSV 与 WBP 不重叠，风险低）
- 基线损坏时的停止条件：若 `main` 在实现期间前进（他人提交），立即 fetch 重做外部提交集成审计，不静默合并

## 实现提纲

不修改锁定目标、验收或已发布契约；可优化实现细节。建议顺序：

1. **先取测试基线**：在 worktree 跑 `Run-Automation -Filter ReEcho.Weapons` 记录绿灯，作为删除后对照。
2. **canonical xlsx 删除**：在 `ReEchoData.xlsx` 的 `武器体系W` 物理删除 `Dagger/匕首` 行；在 `武器插槽C` 删除匕首专属槽位；在 `_SystemData` 删除 `Dagger` 枚举项。用 openpyxl 脚本精确删除（保留其他行），避免误伤。
3. **CSV 整族删除**（8 个文件，按 Writes 清单逐行删）：
   - `weapon_types.csv`：删 Dagger 类型行（注意 SourceSheet=武器体系W/SourceRow=2 已无意义）
   - `weapons.csv`：删 `W_J_05`
   - `attack_steps.csv`：删 `AS_DAGGER_1/2/3` + `AS_DAGGER_DASH_ONLY`
   - `slot_profiles.csv`：删 `SP_DAGGER_CORE/GRIP/BLADE`
   - `parts.csv`：删 8 行 `P_DAGGER_*`
   - `part_effects.csv`：删 4 行 `PE_DAGGER_*`
   - `csv_schema.csv`：删 5 处枚举（Dagger / Pattern.DaggerCombo / Pattern.DaggerDashOnly / Dagger 槽位 / Dagger 配件）
4. **源码测试改写**：`ReEchoWeaponRuntimeTests.cpp` 中删除 `W_J_05`/`P_DAGGER_*` 相关测试（含 `DaggerPartsReplacePatternMoveInvulnerableAndHealOnKill` 类及其 Fixture）；确认其余武器测试不受影响。选择性保留通用配件测试（若有非匕首配件用例）。
5. **UI 引用清理**：`ReEchoInventoryShopWidget.cpp:108-144` 匕首配件图标 `FObjectFinder` 分支移除或加 `IsValid` 判空，避免孤立引用（资源仍在，但不应再指向已删配件）。
6. **验证**：`validate_project.py` → `Run-Automation -Filter ReEcho.Weapons` → 增量 `Build-Editor`。
7. **复活确认**：跑一次 `sync_xlsx_to_csv.py` 确认匕首不会从 canon 复活（或人工确认 canon 已无匕首行）。
8. **发布**：worktree 自验通过后，快进合并进 main；推 `origin/main` 前走 `-FullRebuild` + 预构建包 + `validate` + `git diff --check` 门禁；秘书提交 `[SECRETARY]` 发布 Plan 状态，程序提交 `[PROGRAMMER]`。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | 无悬空外键 / schema 残留，项目不变量通过 |
| 数据同步 | 重跑 `scripts/data/sync_xlsx_to_csv.py` | 匕首不复活（canon 已无匕首行） |
| 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Weapons` | 改写后套件编译+运行通过 |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd -Configuration Development`（本地）/ `-FullRebuild`（发布门禁） | UHT/UBT 退出码 0 |
| 运行时 | 具名 PIE 任务（选武器/构筑） | 3 把可玩武器正常，无崩溃 |

## 执行记录

### 变化

### 证据

### 剩余风险

- **双份真源风险**："开普勒"与 canonical `ReEchoData.xlsx` 不同步（开普勒隐藏匕首、canon 可见）。本 Plan 仅删 canon，开普勒保留隐藏。后续任一武器的"开普勒 vs canon"差异都需先对齐，否则 sync 行为与策划视图不符。建议另立议题统一两份真源（不在本 Plan 范围）。
- **测试覆盖退化**：匕首是配件/构筑唯一完整测试覆盖的武器。整族删除后，该子系统自动化覆盖下降。若其余武器（如长剑）无等效配件测试，建议在本 Plan 或后续补充非匕首配件测试以保住验证矩阵（当前阶段目标 P2 要求 Plan 与 main 状态对齐、验证矩阵稳定）。
- **隐性引用**：除已列明的 `ReEchoWeaponRuntimeTests.cpp` / `ReEchoInventoryShopWidget.cpp` / `ReEchoCombatVfxCatalog.cpp` 外，可能仍有字符串 `"Dagger"` 散落（grep 已确认源码仅此 3 处，但蓝图/WBP 中可能引用 `P_DAGGER_*` 资产名，需 PIE 验证）。

### 人工验收结果/请求

### 架构文档审阅结果
