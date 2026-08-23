# Prompt for the merge AI (Plan 68 → origin/main)

你是被指派来把 **Plan 68（基于策划怪物.xlsx 的怪物系统重构）** 合并入 `origin/main` 的 AI 助手。下面的信息足够你独立完成合并；但**最终推 `origin/main` 必须由人类 gavynqiu（仓库 owner / 程序身份）在本地实测确认后明确说「可以推 main」才执行**，你不得自行 push main。**你是「队友的 AI」，不是 gavynqiu 本机的 AI；所有合入候选必须先交回 gavynqiu 审阅，由他决定是否推 main。**

---

## 0. 环境与工作树（你在远程/队友机器上操作）

- 仓库（公开或队友已授权）：`https://github.com/lyouduan/ReEcho.git`
- Plan 68 实现分支（已发布到远端）：**`origin/plan/68-ws1-5`**（HEAD 约 `ba277ce`，8 个正式 `[PROGRAMMER]` 提交）。
- 你拿到代码的方式：
  ```bash
  git clone https://github.com/lyouduan/ReEcho.git
  cd ReEcho
  git fetch origin plan/68-ws1-5
  git checkout -b plan/68-ws1-5 origin/plan/68-ws1-5
  ```
  避免在 `main` 上直接做实现改动（main 只接收合并/fast-forward）。可在你自己的本地分支（例如 `plan/68-ws1-5-merge`）做 rebase/整合，再交回 gavynqiu。
- 权威规则入口（仓库内文件）：先读 `AGENTS.md` → `shared/PROJECT_RULES.md` → `shared/PLANNER_RULES.md` → `shared/GIT_RULES.md` → `plans/68-reecho-enemy-align-authoritative.md`。**GIT_RULES.md 与 PLANNER_RULES.md 是强制约束，优先于任何历史提示。**

---

## 1. 当前实现现状（权威快照）

完整、最新的实现现状、逐提交语义、未提交工作、已确认的与原提纲的差异、卡点与风险，**全部记录在 `plans/68-reecho-enemy-align-authoritative.md`**（尤其末尾「实现现状总览与交接给下一个 AI」一节）。**以该文档为准，不要依赖本 prompt 复述或任何更早的会话摘要。**

要点速览（仅导航用，细节看 plan 文档）：
- 目标：怪物阵容对齐 `怪物.xlsx`——普通怪 M_SLIME / M_RABBIT / M_FOX（双形态纯表现），Boss = M_SHEEP（两阶段，血清空变身回满 650）。
- 已实现：相对移速模型(γ-B, player=1.0, Base=210)、按场次成长(EnemyCombatStats)、HateRangeCm 数据驱动、投射物扇形散射数据驱动、WS4/WS5 的 SHEEP 二阶段 + 致命伤拦截委托 + 未战斗游走状态机。
- GM 调试命令：`GMShowEnemyHealth` / `GMShowEnemyRange`（AReEchoGameMode 上的 UFUNCTION(Exec)），用于 PIE 验证。

---

## 2. 分支拓扑（合并前必读）

- `plan/68-ws1-5` 的共同祖先 = `b7c4212`（`origin/plan/67-shop-drop` 的 tip），即**本分支基于 plan67，而非直接基于 origin/main**。
- 该分支领先其基线若干提交，落后 `origin/main` **27 个提交**（origin/main 已推进到 Plan84，含 Plan76~84 的 weapon/敌群/关卡/UI 大量改动）。
- 远端另有一条并行实现线 `origin/plan/68-ws4-sheep-boss`（`771db62`）。**本分支 `plan/68-ws1-5` 是更完整的整合线**，以它为准；若发现与 `origin/plan/68-ws4-sheep-boss` 内容重复/冲突，不要把它并进来，只合 `plan/68-ws1-5`。

---

## 3. 你要执行的合并流程（顺序）

1. **fetch 最新**：`git fetch origin`，确认 `origin/main` 当前 HEAD（可能又前进了）。
2. **把 `plan/68-ws1-5` rebase / merge 到最新 `origin/main`**。
   - 由于落后 27 提交且涉及 weapon/敌群/关卡，冲突面较大。**优先手动逐文件落位 + grep 核验，慎用 ort 自动 merge（易静默选边）**。
   - 重点核对：敌人定义编译/reader 链、enemy_abilities/csv schema、Boss 招式策略、SpawnResolver/CombatIndex、与 Plan76~84 weapon 重构的接口是否一致。
   - 若 rebase 中某提交依赖已删除的代码，按「以最新 main + plan68 设计意图」重建，不要保留死代码。
3. **恢复 XLSX 真源一致性**（若 rebase/手改导致 csv 与 `Design/Data/ReEchoEnemyData.xlsx` 不一致）：按 plan 文档「WS4/WS5 恢复交接记录」第四节写回 xlsx → 跑 `python scripts/data/sync_xlsx_to_csv.py` → `python scripts/validate_project.py`。
4. **构建门禁**（推 main 前必须）：`scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`（编辑器须先关闭，否则脚本 exit 3）。成功后 `prebuilt_editor.py update` 会刷新 manifest 与 `Binaries/Win64/UnrealEditor-*.dll` 等精选二进制。
5. **静态校验**：`python scripts/validate_project.py` 必须绿灯（含 prebuilt 指纹、CSV schema、UTF-8）。注意它只做静态检查，不替代构建。
6. **不推送**：把 rebase 后的候选留在本地，跑构建和 validate 通过后，**把提交清单 + 改动摘要 + 构建/validate 证据交给人类用户审阅**，并明确告知用户可在主工作树双击 `ReEcho.uproject` 实测。等用户说「可以推 main」才 `git push origin <merge-result>:main`（fast-forward）。
7. **人工验收（PendingBeforeClose）**：PIE 验证 SHEEP 血清空→变黑二阶段(满血650)→再打死→击败结算；四怪双形态；按场次成长；未战斗游走。未通过则作为新缺陷处理，不要复活旧 Plan 掩盖。

---

## 4. 提交身份与历史纪律（GIT_RULES.md）

- 进入 `origin/main` 的**每个正式提交标题必须以且仅以一个标签开头**：`[PROGRAMMER]`（本任务用这个）。
- Author 必须 `JosephLE910 + Codex` / `JosephLE910@users.noreply.github.com`（仓库已配，勿改）。
- `origin/plan/68-ws1-5` 的 8 个提交已整理为合规正式 `[PROGRAMMER]` 提交，无 WIP / 无乱码标题（历史已由发起方重写并 force 等价替换）。若你在 rebase 时引入新提交，须同样遵守单一 `[PROGRAMMER]` 标签 + UTF-8 无乱码标题。不得仅为补标签而重写已发布历史。
- 纯 Markdown/规则/plan 文档改动不豁免 FullRebuild 门禁的判定：只有「仅 Markdown + workflow」才豁免（见 `df356c1` 规则）。本任务含 C++ 与 CSV，必须 FullRebuild。

---

## 5. 必须守住的工程纪律（来自 plan 文档「接手须知」）

- **运行时绝不直读 xlsx**；禁止把手改 CSV 当真源长期保留——必须 `sync_xlsx_to_csv.py` 回一致。
- 改 `Design/Data/*.xlsx` 前先备份（openpyxl 写表可能丢公式/格式），只改目标单元格，勿重建整表。
- 遍历 xlsx 行**必查 `ws.row_dimensions[r].hidden`**，隐藏行=废除数据，绝不读入/发布。
- `enemy_abilities.BehaviorId` 必须落在 C++ 硬编码 5 个（`Boss.MeleeSweep/Projectile/BlinkSlam/PrayerBeam/ElementCleanse`）；SH_04 用 `PrayerBeam`。
- 模块边界：EnemyLogic 不得 include GameMode/PlayerController/presentation；血量由 host(EnemyActor) 注入 `Sense.CurrentHealthRatio`。
- 致命伤拦截委托签名 `FReEchoFatalDamageIntercept = bool(float&)`，改动须同步所有绑定/调用点。
- 远端仅允许 `origin`（`origin/main` 是唯一可推分支）；不要添加其他 remote。

---

## 6. 给人类的交付摘要模板（合入候选就绪后发给用户）

```
【Plan68 合入 origin/main 候选】
分支：plan/68-ws1-5（已 rebase 到 origin/main <hash>）
提交数：N（squash 后的正式 [PROGRAMMER] 提交清单）
改动面：C++ X 文件 / CSV Y 文件 / xlsx Z 文件 / 二进制（精选 DLL + prebuilt.json）
构建：-FullRebuild 结果 Succeeded
validate_project.py：PASS
已知冲突处理：<列出 rebase 解决的冲突点>
PIE 验收项：<列出需用户实测的点>
风险/遗留：<e45242d 去留、并行 plan/68-ws4 线、任何未决>
请本地实测；确认后回复「可以推 main」。
```
