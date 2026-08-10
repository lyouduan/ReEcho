# Plan 23 - data - elements, statuses and reactions CSV migration

## Locked goal

将元素体系、必要状态定义和元素反应迁移到 Plan 21 的 CSV 注册表，使策划可以配置元素身份、触发/附着角色、反应配对、数值参数、状态引用与已注册反应行为，同时保持确定性伤害结算、GAS 权威属性和玩家/回响一致语义。

## Dependency status

Plan 21、22 已于 2026-08-10 人验、审查并本地合并，本 Plan 现在可以启动。Plan 22 的实际扩展契约是：`ReEchoCsvDataReader.*` 提供通用解析/manifest 原语，`ReEchoCharacterBuildCsvReader::ReadTables()` 是领域读取模板，`FReEchoCsvDataRegistry` 仍统一维护必需表、内建注册、跨领域验证和原子发布，`FReEchoCsvDataSnapshot` 是唯一公开只读快照。

Plan 23 在执行期独占生产 manifest/schema、`FReEchoCsvDataSnapshot`、注册表编排、内建行为注册、`ReEcho.Build.cs` 和静态校验入口。Plan 24 只能在本 Plan 合并后开始运行时接线。

## Source baseline

- `元素体系Y`：火/雷触发元素，草/水附着元素，以及灼烧、汽化、生长、导电、两种有顺序的强化。
- `状态Z`：元素免疫、灼烧、嘲讽、眩晕、减速、无敌、隐身、流血等反应/构筑会引用的状态。
- 当前 C++ 只把 Water/Flame/Grass 视为战斗元素，以少量双倍伤害配对模拟反应；与工作簿中的 Lightning、状态和六种反应并不等价。

## Locked acceptance

### 🤖 Automated

- [x] 元素表将 ID、角色（Trigger/Attachment 等）、显示文本键、颜色/视觉键和 enabled 分列；Reaction/Status 通过稳定 ID 引用，不靠中文名称匹配。
- [x] 状态表提供反应所需的持续时长、叠加/刷新/互斥策略和已注册状态行为；纯展示描述不作为运行时判定。
- [x] 反应表分离 ordered trigger/attachment、ReactionBehaviorId、FormulaId/有限公式类型、数值系数、范围、状态引用、暴击/回响修正规则。
- [x] 不执行 Excel/CSV 中的任意公式字符串。伤害/范围计算使用白名单 FormulaId 与类型化系数；独特行为通过注册 handler。
- [x] 至少实现并自动化覆盖工作簿六个反应：灼烧、汽化、生长、导电、草→水强化、水→草强化；顺序敏感配对不能被无序集合合并。
- [x] Lightning 成为可识别的战斗元素；元素免疫、附着清除/阻断、强化不叠加等状态机行为与表一致且确定。
- [x] 玩家与回响使用同一注册快照和反应结算；回响触发是否受 EchoEfficiency 影响、反应能否暴击等由表字段/已注册规则明确，不能散落在调用方。
- [x] 元素伤害仍经 GAS/现有权威伤害路径结算；Actor 只持目标状态与表现反馈，不重新成为数值权威。
- [x] 当前可玩元素武器与既有 Plan 14 自动化保持或按人批准的工作簿语义更新；任何有意行为变化必须在执行经验和 UI/遥测中可见。
- [x] 旧 `elements.json`、`reactions.json`、`statuses.json` 与 C++ 重复常量退出该领域运行时权威。
- [x] Editor build、元素/状态/反应自动化、全量 `ReEcho.*`、静态校验和 `git diff --check` 通过。

### 🎮 Human PIE

- [ ] 人逐一触发六种反应，确认颜色/状态提示、范围、持续时间、顺序差异和伤害可读。
- [ ] 人分别用玩家与回响触发反应，确认两者遵守同一表定义且回响修正规则符合设计。
- [ ] 人修改一项反应系数或持续时长、重启 Run 后确认无需重新编译即可生效。

## Step 0 gates

1. 从已包含本地合并 `ddc785a` 的 main 创建 `plan/23-elements-reactions-csv`；读取 Plan 21、22 最终 Execution notes、`ReEchoCsvDataReader.*`、`ReEchoCharacterBuildCsvReader.*`、当前快照/注册表、22 项测试基线和静态校验器。
2. 在协调板独占本 Plan 的公共数据文件；不得复制或绕过 Plan 22 留下的通用读取/原子发布路径。
3. 新建 `ReEchoElementReactionCsvReader.*`，沿用 `ReadTables(DataDirectory, ManifestEntries, Snapshot, Issues)` 领域入口；解析细节不得回填进 `ReEchoCsvDataRegistry.cpp`。将本领域的 `BehaviorId`、`EffectKind`、`FormulaId`/状态行为通过显式内建注册函数注册，并保证发生在 `LoadAndPublishDefault()` 之前。
4. 采集当前 `ReEchoElementReaction`、敌人附着状态、投射物元素传递、GAS damage source 和 Plan 14 自动化基线。
5. 对照工作簿与现有 JSON/C++ 列出有意差异，特别是 Lightning 缺失、当前双倍伤害简化和工作簿 Formula 描述。行为改变必须由人确认，不以“表里写了”自动覆盖已验收原型。
6. 先确定 FormulaId 白名单与单位：元素攻击、ReactionEfficiency、DamageIncrease、范围、秒、UE 厘米如何组合。若汽化平方公式等存在量纲/数值歧义，设为阻断 gate 请人拍板，禁止自行改成看似合理的数值。
7. 先确定顺序敏感状态机和抗递归规则，确保导电/范围附着不会因新反应无限递归或因遍历顺序产生不确定结果。
8. 本任务不修改武器槽表；只提供 Plan 24 可消费的稳定 ElementId/Reaction API，并在最终 Execution notes 写清。
9. Plan 22 的负例目录已开始重复完整生产表。新增元素表前先把自动化/Python 夹具改为“生产基线包 + 该负例的局部覆盖”，运行时在临时目录组装完整包；不要把三个元素表复制进每个既有负例目录。

## Target table responsibilities

生产表文件固定为 `elements.csv`、`statuses.csv`、`reactions.csv`；具体列名服从 Plan 22 已落地的领域读取约定，语义至少包括：

- Element：ID、role、display/color/presentation keys、enabled。
- Status：ID、behavior ID、duration/stack/refresh policy、tags、enabled。
- Reaction：ID、ordered incoming/attached element IDs、behavior ID、formula ID、coefficients/radii、status references、crit/echo flags、enabled。
- 可变数值为独立字段或类型化参数；描述文本只服务 UI/策划，不参与计算。
- 运行时范围、枚举、顺序配对、外键和 FormulaId 白名单由类型化 C++ 元素领域读取器校验；`csv_schema.csv` 同步服务静态校验/文档，但不是运行时公式解释器。

## Implementation outline

1. 新增 `ReEchoElementReactionCsvReader.*`，扩展共享快照与注册表编排；只在注册表增加必需表汇总、领域调用和必要跨域验证，不放领域逐行解析。
2. 先建立基线＋覆盖的临时夹具组装方式，再导入/验证三个生产表与引用图；manifest 缺任一必需表或任一领域失败时整包不发布。
3. 将纯判定核心改为读取不可变反应定义，保留一个确定的结算入口。
4. 以有限 handler 实现六种反应和所需状态，不按 ReactionId 在 Actor 中散布 switch。
5. 接回敌人状态、GAS 伤害、投射物/武器元素、表现反馈和回响调用方。
6. 添加顺序、递归、范围目标排序、状态刷新/阻断、原子失败、回响修正和表变值自动化。
7. 移除该领域旧运行时重复值，更新数据 README、manifest/schema/RuntimeDependencies、静态领域校验、CODEBASE_MAP、PROJECT_STATE、LESSONS 和 Execution notes。

## Expected ownership

- `Content/Data/elements.csv`、`statuses.csv`、`reactions.csv` 及生产 manifest/schema/README/打包依赖。
- 独立元素领域读取器、共享快照扩展、注册表编排与显式内建行为/公式注册。
- `Combat/ReEchoElementReaction.*` 及新增的领域 registry/handler。
- 必要的 `Core/ReEchoTypes.*`、GAS effect/source、Enemy/Projectile 状态接线。
- 元素反应自动化和静态验证。
- 不修改武器攻击模组/插槽内容；与 Plan 24 重叠的公共文件需提前在协调板声明。

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| CSV static | `python scripts/validate_project.py` | 元素/状态/反应 ID、配对、公式、引用与范围通过；负例精确失败 |
| Build | format + `scripts/ue/Build-Editor.cmd` | UE 5.8 Editor build 成功 |
| Automation | 六反应、顺序、状态、递归/目标排序、玩家/回响 + 全量 `ReEcho.*` | 结算确定，表参数变值可见，无旧测试回归 |
| Diff | 初版↔终版、`git diff --check` | Plan 21 适配和批准的设计差异记录完整 |
| Human | 六反应 PIE | 伤害、范围、持续、提示和回响语义通过人验 |

## Risks and exclusions

- 公式量纲和平方成长可能导致极端数值；没有人拍板不擅自“平衡修正”。
- 连锁和范围附着必须使用稳定目标排序/已访问集合，不把 TSet/TMap 非稳定遍历暴露到结算结果。
- 不在本 Plan 重做武器架构或所有状态系统；只实现这些表和 Plan 22/24 已确认依赖的公共状态能力。
- 表驱动不等于无代码：新的独特反应行为仍需 C++ handler 和自动化后才能 enabled。
- 本 Plan 与 Plan 22/24 串行；不得为了并行而复制快照、manifest、行为注册或静态校验系统。
- 不得继续按“每个负例一整套全部领域 CSV”扩张夹具；这会让后续每加一张表都必须机械修改所有旧负例。

## Recommended executor model

强模型。该任务涉及确定性状态机、GAS 伤害、范围/连锁目标选择和工作簿公式翻译。

## Execution notes

### Changed

- Continued in the isolated Plan 23 worktree on branch `plan/23-elements-reactions-csv`; the planner/main worktree still has unrelated `.uasset` changes and this branch does not claim or edit binary assets.
- Added production `elements.csv`, `statuses.csv` and `reactions.csv`; extended manifest, schema, README, `ReEcho.Build.cs`, static validation and data automation coverage.
- Added `ReEchoElementReactionCsvReader.*` following the Plan 22 domain-reader shape. `FReEchoCsvDataRegistry` remains the only manifest/orchestration/atomic publish boundary.
- Extended `FReEchoCsvDataSnapshot` with typed element/status/reaction rows and explicit `FormulaId` registration. Built-ins now register status and reaction behaviors plus `Element.ElementAttackDot`, `Element.ElementAttackSquared`, `Element.AttachInRadius`, `Element.ChainElementAttack` and `Element.EnhanceNextReaction` before default load.
- Reworked `ReEchoElementReaction` to resolve enabled combat elements, labels, colors and ordered reactions from the published CSV snapshot. The pure state machine now handles attachment roles, reaction status output, elemental-immunity blocking, ordered non-stacking enhancement and table metadata; world execution handles DOT, squared vaporize damage, growth range attachment and conduct chain traversal.
- Kept elemental damage on the existing GAS path: `AReEchoEnemyActor::ReceiveElementalDamage()` builds a shared hit context for player/echo sources, then `ApplyHitToWorld()` calls `ReceiveGrayboxDamage()` / `ReEchoGameplayEffects::ApplyDamage()` for every immediate or scheduled damage application.
- Updated weapon 3's deterministic elemental sequence and Poet random elements to include Lightning and cover the six ordered reaction pairs through normal projectile play.
- Replaced complete copied CSV fixture packages with Python/C++ temporary assembly from production baseline plus local fixture overrides. Existing negative fixtures now keep only their changed CSV, and new element fixtures cover unknown formula, duplicate ordered pair and unsupported `CanCrit`.
- Updated `shared/CODEBASE_MAP.md`, `shared/PROJECT_STATE.md` and `shared/LESSONS.md`.

### Evidence

- `python scripts/validate_project.py` passes: production character/build/element tables, fixtures, IDs, references, behavior/effect/formula allowlists, UTF-8, staging deps and workflow guards.
- `.clang-format` was run on changed C++ files.
- `scripts/ue/Build-Editor.cmd -Configuration Development` passes with UE 5.8 installed build.
- After the remote integration, `scripts/ue/Run-Automation.cmd -Filter ReEcho` exited 0 for the pre-correction automation baseline, including `ReEcho.Combat.ElementReactions`, `ReEcho.Combat.ElementReactionWorld`, `ReEcho.Run.SaveSnapshot` and `ReEcho.Shop.PostDrawCurrencyCanPurchase`.
- `git diff --check` passes.

### Plan 21/22 contract adaptation

- Plan 21 smoke tables and Plan 22 character/build tables remain required production CSV and load through the same `FReEchoCsvDataRegistry` package.
- Required production manifest tables are now: `RuntimeSmoke`, `RuntimeSmokeEffects`, `Characters`, `CharacterAliases`, `Cards`, `CardEffects`, `Elements`, `Statuses`, `Reactions`.
- Plan 22's domain reader pattern was preserved: registry owns required table ordering and publish atomicity; element/status/reaction parsing lives in `ReEchoElementReactionCsvReader.*`.
- Fixture handling was adapted before adding required element tables. Python and C++ tests assemble full temporary packages from production CSV plus fixture overrides, so future domain tables do not need mechanical copies in every existing negative fixture.
- Legacy `elements.json`, `statuses.json` and `reactions.json` remain in the repo as migration-only review material; current runtime reaction semantics no longer read them.

### Implemented semantic differences under planner review

- Plan 14's hardcoded unordered `Water+Grass` and `Grass+Flame` double-damage prototype has been replaced by six ordered CSV rows from the element/status/reaction migration source.
- Flame and Lightning are trigger elements; Grass and Water are attachment elements. Trigger-only hits no longer become persistent attachments by themselves.
- Lightning is now recognized by projectiles, labels, colors and reaction resolution.
- Current reaction math uses finite typed formulas:
  - `Element.ElementAttackDot`: schedules deterministic one-second DOT ticks for the configured status duration and uses `ElementalAttack * DamageMultiplier * ReactionEfficiency`.
  - `Element.ElementAttackSquared`: uses the workbook elemental-attack squared model, `ElementalAttack * ElementalAttack * DamageIncrease * ReactionEfficiency`.
  - `Element.AttachInRadius`: selects targets by stable distance/location/name order and attaches Grass in the configured radius.
  - `Element.ChainElementAttack`: performs stable one-meter chain traversal with a visited set and applies `ElementalAttack * DamageMultiplier * ReactionEfficiency` to each target.
  - `Element.EnhanceNextReaction`: removes the ordered attached element, blocks that element from reattaching until the next non-enhancement reaction, and gives that next reaction +100% final damage before clearing itself.
- `AffectedByEchoEfficiency` is consumed in reaction damage execution when a row enables it. Current production reaction rows set it to `false`, so player and echo reactions with the same elemental attack remain consistent.
- `CanCrit=true` is rejected during CSV load because crit-enabled reaction damage is not implemented yet; it is not merely surfaced as an unused field.
- The workbook `属性S` sheet includes `S_E_Attack_Power` and reaction-relevant efficiency attributes; vaporize now follows the user-confirmed squared elemental-attack formula.

### Public element/reaction contract for Plan 24

- Use `FReEchoCsvDataRegistry::GetSnapshot()` as the only public runtime data source. Do not read element/reaction CSV files directly from actors, weapons, UI or future slot code.
- Stable element IDs for Plan 24 are `Flame`, `Lightning`, `Grass`, `Water`; `ReEchoElementReaction::GetElementId(EReEchoElement)` exposes the enum-to-ID bridge.
- Reactions are ordered by `(TriggerElementId, AttachmentElementId)` and can be queried through `FReEchoCsvDataSnapshot::FindReaction()`. Do not canonicalize or sort the pair.
- Runtime weapon/slot migration may consume `elements.csv` roles and `reactions.csv` flags (`CanCrit`, `AffectedByEchoEfficiency`, `RadiusCm`) without adding a second reaction registry.
- Enabled unique reaction behavior still requires a registered C++ handler/formula and automation before CSV can turn it on.

### Planner review after remote integration

Plan 23 is integrated onto the remote-feature baseline but is not yet accepted. The executor must correct and test these items before another review:

- Conduct must implement the workbook contract `(ElementalAttack + 2) * ReactionEfficiency * DamageIncrease`; do not model the `+2` term as a multiplicative CSV value.
- Growth radius must scale by `ReactionEfficiency`. Growth attachment must go through `AttachElementIfAllowed` (or the shared equivalent) and must not grant elemental immunity.
- Elemental immunity applies only to Burn, Vaporize and Conduct. Growth and both Enhance directions must not receive it through a universal reaction path.
- Conduct must use the configured radius exactly; remove the hidden `Max(RadiusCm, 100)` floor.
- Burn automation must advance world time and verify real GAS damage plus `RefreshOnly` non-stacking/refresh behavior, not merely verify that timers were scheduled.
- Save data must store `ActiveStatusUntilSeconds` as remaining durations and rebase them on restore, matching save version 3's existing treatment of elemental immunity. If Burn uses transient timers, restoring a mid-Burn save must reconstruct the remaining deterministic ticks or otherwise implement and test an explicit equivalent continuation policy.

### Planner review corrections completed locally

- Conduct now implements the workbook formula `(ElementalAttack + 2) * ReactionEfficiency * DamageIncrease`; production `Y_ER_L_W` uses `DamageIncrease=2`, `DamageMultiplier=1`, and runtime conduct damage ignores `DamageMultiplier` for the `+2` term. The `ReactionValueChanged` fixture sets `DamageIncrease=3` and `RadiusCm=75` to prove value reload and exact-radius behavior.
- Growth now uses `RadiusCm * ReactionEfficiency`, attaches only through `AttachElementIfAllowed`, respects elemental immunity and blocked attachments, and records only actually attached targets as affected. Growth no longer grants elemental immunity.
- Elemental immunity is now limited to Burn, Vaporize and Conduct in both pure `ResolveHit()` state and world execution. Growth and both Enhance directions do not receive immunity through the common reaction path.
- Conduct chain traversal now uses the configured radius exactly; the old hidden `Max(RadiusCm, 100)` floor was removed.
- Burn DOT is no longer a set of transient timer callbacks. Burn keeps one deterministic status stream in `FReEchoElementState` (`BurnTickDamage`, `BurnNextTickTimeSeconds`) and `AReEchoEnemyActor::Tick()` advances it against World time through `TickElementStatuses()`. RefreshOnly refreshes the status end time without adding an independent DOT stream, while preserving the next scheduled one-second boundary.
- `AReEchoEnemyActor::CaptureRuntimeState()` now converts every `ActiveStatusUntilSeconds` entry, `ImmunityUntil`, and `BurnNextTickTimeSeconds` to remaining durations for save data. `RestoreRuntimeState()` rebases those remaining values onto the new World time, matching SaveVersion 3's immunity semantics and allowing mid-Burn resume to continue deterministic remaining ticks.
- Added focused automation:
  - `ReEcho.Combat.ElementReactionBurnRefresh`: advances World time, verifies real GAS health loss, RefreshOnly non-stacking/refresh behavior, and end/final-tick boundaries.
  - `ReEcho.Combat.ElementReactionSaveContinuity`: captures mid-Burn remaining status/tick times, restores in a new World timebase, and verifies continued GAS ticks.
  - Existing `ReEcho.Combat.ElementReactionWorld` now also covers Growth radius scaling/attachment limits, Conduct exact configured radius, Conduct workbook damage, and no-immunity Enhance/Growth behavior.
- Updated `shared/PROJECT_STATE.md` and `scripts/validate_project.py` from the previous 25-test baseline to the new 27-test baseline; no automation coverage was merged or removed to preserve the old count.

### Boundary review corrections completed locally

- Burn now has an explicit active flag in `FReEchoElementState`, so `BurnNextTickTimeSeconds == CurrentTimeSeconds` and a saved remaining next-tick value of `0` are valid due-now states rather than inactive sentinels.
- Burn refresh now settles any existing due tick before extending `RefreshOnly` duration. A refresh at `NextTickTime == CurrentTime` preserves the boundary tick exactly once, then continues the same single DOT stream on later one-second boundaries.
- Burn status submission now has one clear World-execution boundary. `ResolveHit()` returns `AppliedStatusId` and duration metadata but no longer writes `ActiveStatusUntilSeconds`; `StartOrRefreshBurn()` therefore sees the old `Z_Burn` end time, settles only ticks within that old window, and only then commits the new Burn end time.
- Burn DOT context is stored with the deterministic status stream. In-run ticks use the original `SourceLocation` and valid runtime `SourceActor`; save capture persists deterministic `BurnSourceLocation` and intentionally clears `BurnSourceActor`, because actor object identity is not valid across a process/save boundary. Restored Burn therefore falls back to `nullptr` SourceActor while still using the persisted SourceLocation for directional damage.
- The Burn tick path no longer calls `ReceiveGrayboxDamage()` with the target's own location. Shield enemies burned from behind continue to receive DOT through GAS instead of becoming permanently front-blocked by a self-location fallback.
- Focused automation was added inside the existing 27-test suite, without changing automation count:
  - `ReEcho.Combat.ElementReactionBurnRefresh` now includes exact-tick-boundary refresh coverage and verifies the due boundary tick is neither lost nor duplicated before the refreshed single stream continues.
  - `ReEcho.Combat.ElementReactionBurnRefresh` also covers a stalled old Burn whose Enemy Tick has not run by the time a new Flame-over-Grass hit arrives after the old end. The old stream settles only ticks through the old end, produces no ghost ticks from the fourth second to the current frame, and starts one new DOT stream from the current frame.
  - `ReEcho.Combat.ElementReactionSaveContinuity` now includes exact-tick-boundary save/restore coverage and verifies due-now restore plus the final boundary tick.
  - `ReEcho.Combat.ElementReactionWorld` now includes Shield-from-behind Burn DOT context coverage, including in-run SourceActor/SourceLocation retention and restored SourceActor fallback with persisted SourceLocation.

### Final correction evidence

- `python scripts/validate_project.py` passes after the correction pass.
- `.clang-format` was run on changed C++ files using Visual Studio LLVM clang-format.
- `scripts/ue/Build-Editor.cmd -Configuration Development` passes with UE 5.8 installed build.
- `scripts/ue/Run-Automation.cmd -Filter ReEcho` exits 0; log evidence: `Found 27 automation tests based on 'ReEcho'`, `ReEcho.Combat.ElementReactionBurnRefresh`, `ReEcho.Combat.ElementReactionSaveContinuity` and `ReEcho.Combat.ElementReactionWorld` completed with `Result={Success}`, final line `TEST COMPLETE. EXIT CODE: 0`. The 27-test count is unchanged because the new boundary coverage lives inside the existing focused automation tests.
- `git diff --check` passes.

### Correction deviations

- Burn continuation is implemented as a semantic replacement for transient timers rather than reconstructing `FTimerHandle` objects after restore. The deterministic state carries the next tick time and tick damage, and automation proves resumed GAS damage across the new World timebase.
- `BurnSourceActor` is runtime-only and is deliberately not persisted across save/restore. Restored Burn DOT uses `nullptr` SourceActor plus persisted SourceLocation; automation proves this fallback still drives shield direction and GAS damage correctly.
- The automation fixture manually aligns `UWorld::TimeSeconds` when the commandlet test world does not advance it by the supplied delta; this keeps tests asserting World-time semantics without changing runtime gameplay worlds.

### Remaining risks

- Human PIE is still needed for readability of the new 12-step element weapon cycle, Lightning labels/colors, reaction pacing and status hints.

### Human validation requested

- Trigger Burn, Vaporize, Growth, Conduct, Grass-over-Water Enhance and Water-over-Grass Enhance in PIE; confirm color/label readability, damage numbers, radius expectations and duration/status hints.
- Trigger the same reactions with a player and an echo; confirm both follow the same CSV definitions and echo behavior matches the explicit `AffectedByEchoEfficiency=false` rows.
- Edit `reactions.csv` `Y_ER_L_W.DamageMultiplier` or a status duration, restart the project/run, and confirm the changed value appears without recompiling C++.
- Confirm the new ElementalOrb sequence including Lightning still feels acceptable for current playable pacing.
## 执行者启动 prompt

```text
你是 ReEcho 项目的执行者。先读 AGENTS.md，再按其最小读取顺序读 plans/23-data-elements-reactions-csv.md、已完成的 Plan 21 与 Plan 22 最终 Execution notes、Plan 14 执行经验，以及 shared/LESSONS.md §GAME 中元素/状态相关匹配条目；只有诊断失败时才读 §DEBUG。

前置：main 已合并 Plan 21 和经人验的 Plan 22，Plan 23 是当前唯一 CSV 领域迁移任务；Plan 24 尚未开工。否则停止并告诉人。然后在独立 worktree 的 `plan/23-elements-reactions-csv` 分支工作；参考工作簿为仓库上一级 `../回响肉鸽数值与构筑体系.xlsx`。

任务：沿用 `ReEchoCsvDataReader.*`，按 `ReEchoCharacterBuildCsvReader::ReadTables()` 的形态新增独立元素领域读取器，并扩展唯一快照/注册表编排。先把重复整包负例夹具改成“生产基线＋局部覆盖”的临时组包，再迁移元素、必要状态和六个反应。显式注册所有内建行为/公式并保证先注册后启动加载。验收照 Plan 23 的 🔒 清单；不得执行任意公式、不得在 Actor 中按 ReactionId 散布特判、不得自行修正有歧义的平方公式/量纲，也不得修改 `.uasset`/`.umap`。把 Plan 21/22 适配和语义差异写入 Execution notes。

完成后显式提交到本地分支并告诉人；未经人明确确认不得 push，禁止修改或合并 main。
```
