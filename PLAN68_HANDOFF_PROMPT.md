# Plan 68 分支接手说明（给队友 AI）

## 这个分支是干什么的

`origin/plan/68-ws1-5` 是 **Plan 68 的完整实现线**：以策划 `怪物.xlsx` 为权威源，重构怪物系统数据模型并落地到运行时。核心交付：

- **阵容对齐权威表**：普通怪 M_SLIME / M_RABBIT / M_FOX（双形态纯表现），Boss = **M_SHEEP**（两阶段，血清空变身回满 650）。
- **数据驱动化**：相对移速模型（γ-B，player=1.0，`BaseMoveSpeed=210` 编译期归一）、按场次成长（EnemyCombatStats，按 EncounterIndex 覆盖 MaxHealth/ContactDamage/AttackInterval）、仇恨范围 HateRangeCm、投射物扇形散射（ProjectileCount/SpreadAngleDegrees）全部由 xlsx→csv→不可变快照驱动。
- **WS4/WS5**：SHEEP 二阶段 + 致命伤拦截委托（血清空变身）+ 未战斗游走状态机；GM 调试命令 `GMShowEnemyHealth` / `GMShowEnemyRange`。

完整、最新的实现状态与已确认的与原提纲的差异，**以 `plans/68-reecho-enemy-align-authoritative.md` 末尾「实现现状总览与交接给下一个 AI」一节为准**，不要依赖本说明复述。

## 为什么由你接手（而不是发起方 AI）

发起方 AI 所在的实现分支**落后 `origin/main` 27 个提交**（main 已推进到 Plan84），且**基于 `plan/67-shop-drop` 而非直接基于 main**。它对 main 当前状态（Plan76~84 的 weapon 重构、敌群寻路、关卡/UI 大量改动）没有实时的完整上下文。

而**你一直在跟进 main 的演进**，更清楚 Plan76~84 引入的接口与契约。因此由你把这个 27-commit-behind 的分支 rebase 并合入 main，比发起方更不容易在冲突处静默选错边。这就是交给你做的原因——不是从零实现，而是把一条已有的、已自检过的实现线，正确地接回最新的 main。

## 接手时盯紧的几点

- **基线**：`plan/68-ws1-5` 共同祖先是 `b7c4212`（= `origin/plan/67-shop-drop` tip）。rebase 到最新 `origin/main` 后，重点核对与 Plan76~84 的 weapon/敌群/关卡改动在敌人定义编译链、enemy_abilities/csv schema、Boss 招式策略、SpawnResolver/CombatIndex 上的接口一致性。优先手动逐文件落位 + grep 核验，慎用 ort 自动 merge。
- **并行线**：远端另有 `origin/plan/68-ws4-sheep-boss`（`771db62`）是同一目标的另一种实现，**以本分支 `plan/68-ws1-5` 为准，不要把它并进来**。
- **已冻结的设计决策**（rebase 时不要被 main 的旧值带偏）：Boss 是 M_SHEEP（MaxHealth=1300，复用 Boss.TimeGuard 的 profile/presentation 仅换 id）；二阶段=血清空变身回满 650；移速=γ-B 相对模型。
- **数据链铁律**：运行时绝不直读 xlsx；若 rebase/手改使 csv 与 `Design/Data/ReEchoEnemyData.xlsx` 不一致，必须写回 xlsx 后 `sync_xlsx_to_csv.py` 再 `validate_project.py`。
- 提交身份与工程纪律你已熟悉，照仓库既有规则执行即可（单一 `[PROGRAMMER]` 标签、UTF-8 无乱码标题、`-FullRebuild` + validate 门禁）。

## 唯一门禁

合入候选准备好后，**不要自行 push `origin/main`**。把提交清单 + 改动摘要 + 构建/validate 证据交回 gavynqiu 审阅；等他本地实测并明确说「可以推 main」再推。最终审批权在 gavynqiu。
