# Plan 122 - 程序 - 构筑路径与伤害关联日志

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`NotRequired`；格式、覆盖点、稳定摘要和伤害关联可由自动化及日志证据验证，策划后续提供真实 session log 属于使用阶段而非关闭门禁。
- 本地规划基线：`origin/main@fb67ce38`；实现基线：`origin/main@5aa11dbc`（已先整合远端 Plan120/121 规划）。
- 本地实现方式（可选，仅作交接说明）：Plan 单独发布后，从准确远端主线建立 `plan/122-build-path-diagnostics` 独立 worktree。
- 依赖 / 阻塞：复用 Development 会话日志 `ReEcho-session-<本地开始时间>-pid*.log`、现有 `ShopPurchaseAudit`、`FReEchoBuildSnapshot`、`FReEchoAttackIdentity` 的 Source/Weapon/Sequence 关联；不依赖 Plan106/107 候选。
- Writes:
  - `plans/122-build-path-diagnostics.md`
  - `Source/ReEcho/Public/Diagnostics/ReEchoBuildTrace.h`
  - `Source/ReEcho/Private/Diagnostics/ReEchoBuildTrace.cpp`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Source/ReEcho/Private/Weapons/ReEchoWeaponActor.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoBuildTraceTests.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`
  - `shared/LESSONS.md`（仅在形成可复用且有证据的诊断经验时）
  - 最终 FullRebuild 刷新的 `Binaries/Win64/` 精选预构建包
- Stable Reads:
  - `Source/ReEcho/Public/Core/ReEchoTypes.h`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardTypes.h`
  - `Source/ReEchoCombat/Public/Combat/ReEchoCombatTypes.h`
  - `Source/ReEchoCombat/Private/Combat/ReEchoHitResolver.cpp`
  - `Source/ReEchoCombat/Private/Combat/ReEchoElementHitResolver.cpp`
  - `Source/ReEcho/Private/ReEcho.cpp`
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`
  - `shared/CODEBASE_MAP/README.md`
- 影响模式：`SharedContract`；新增供策划、程序和后续 AI 共同消费的稳定诊断行格式，但不改变玩法、存档或模块依赖。
- 兼容承诺 / 下游操作：保留现有 `ShopPurchaseAudit`、`SwordDamageTrace`、`RangedCritTrace`、`EnemyDamageTrace`；新增日志使用独立前缀并在非 Shipping 构建写入普通 UE/session 日志。旧日志仍可读取；新日志不得改变随机序列、构筑状态、攻击身份或伤害结果。
- 明确排除：不修任何具体卡牌/符文数值问题；不合入 Plan106/107；不改变伤害公式、暴击、攻速、商店事务、存档版本或录制格式；不把完整构筑重复写入每一层 Projectile/Resolver；不记录机器路径或用户隐私。

## 锁定目标

1. 一份 Development `ReEcho-session-*.log` 能还原本局关键构筑路径：开局、读档、遭遇开始、获得卡牌、武器切换、符文装备变化和商店成功事务后均留下完整有效构筑快照。
2. 每个快照至少记录事件、Encounter/Phase、角色、武器、已装备符文、卡牌及叠层、有效主要属性、关键卡牌运行时计数、规则摘要、数据 revision 和稳定构筑指纹。
3. 玩家与回响每次成功武器 Commit 记录 Source、锁定 Target、Origin、Direction、WeaponId、Attack Sequence、构筑指纹、RawDamage、Critical、Element 和攻击段；现有 Projectile/Resolver 日志可通过 Source/Weapon/Sequence 与该 Commit 一一关联，并可区分“已命中但伤害被拦截”和“锁敌/方向错误导致未接触”。
4. 构筑序列化与指纹计算必须确定性：无序容器和等价卡牌叠层不得因遍历顺序产生不同结果；日志只读且不消耗随机数。
5. 控制日志规模：完整快照只在构筑/关次生命周期边界输出，伤害路径只输出单行关联摘要，不在每个 Projectile/Resolver 层重复卡牌与符文列表。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`（`AREA-Run`、`AREA-Weapons`、`AREA-Tests`）；`MOD-ReEchoWeapons` 仅受主模块 WeaponActor 诊断适配影响，资源无关 Weapons 逻辑不修改。`MOD-ReEchoCombat` 的攻击身份和 Resolver 契约保持不变。
- 对应模块文档：更新 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，记录 BuildTrace 所有权、触发点、格式和日志位置；更新 `MOD-ReEchoWeapons.md`，明确 WeaponActor 只在主模块把构筑指纹关联到 Commit，Weapons/Combat 不依赖 Run/Card 类型。审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEchoCombat.md`，若拓扑和路由未变化则在执行记录说明无需修改。
- 设计意图：把“偶现问题所需的构筑上下文”从临时专用日志提升为统一诊断能力。构筑真值仍由 Run/Recording 的 `FReEchoBuildSnapshot` 拥有；诊断层只生成确定性文本和指纹，不建立第二份状态。
- 权威状态与依赖：新增 `ReEchoBuildTrace` 位于主模块，因为只有主模块同时认识 Run、Cards 和武器数据；Run 在权威提交完成后输出快照，WeaponActor 从其已钉住的 BuildSnapshot 计算同一指纹并记录 Commit。禁止让 `ReEchoCombat` 或 `ReEchoWeapons` 反向依赖 Run/Cards。
- 决策记录：不向 `FReEchoAttackIdentity` 增加构筑字段，避免为日志污染公共战斗消息；使用既有 Source/Weapon/Sequence 关联 Resolver。完整快照与单行 Commit 分离，兼顾可还原性和日志体积。指纹仅用于诊断关联，不作为存档身份、去重键或玩法判断。
- 相关文档同步范围：必审 `ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoWeapons.md`、`MOD-ReEchoCombat.md`；预计只需更新前两份模块文档，若实现未改变拓扑/路由/Combat 契约，其余记录“已审阅、无需修改”。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅模块拓扑和依赖方向；
  - `README.md`：待审阅 AREA 路由；
  - `MOD-ReEcho.md`：待记录 BuildTrace 诊断所有权与代码位置；
  - `MOD-ReEchoWeapons.md`：待记录主模块 Commit 关联边界；
  - `MOD-ReEchoCombat.md`：待确认攻击身份公共契约未被日志需求污染。

## 锁定验收

- [x] 确定性测试证明相同构筑得到相同指纹，卡牌/RuleFlags/符文容器顺序变化不改变指纹，真实内容变化会改变指纹。
- [x] 日志格式测试覆盖卡牌叠层、符文槽位、有效属性、关键 runtime 计数、数据 revision 和空集合。
- [x] Run 的开局、读档、遭遇开始、免费/付费/调试授卡、武器切换和符文装备成功路径均有明确 BuildTrace 触发点；失败事务不伪造提交后快照。
- [x] 玩家与 Echo 成功 Commit 输出可与现有 Resolver 通过 Source/Weapon/Sequence 关联的指纹摘要；失败/回滚 Commit 不输出成功关联行。
- [x] 日志代码不改变随机序列、BuildSnapshot、攻击伤害或存档；Shipping 不新增高频诊断输出。
- [ ] 聚焦自动化、Development Editor FullRebuild、项目校验、预构建检查和 `git diff --check` 通过。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@fb67ce38`，已包含最新 Boss Skill03、武器表现与 Boss 胜利修复。
- 引擎/构建可用性：UE 5.8 Win64；实现构建和自动化按 Git common-dir Unreal 锁串行执行。
- 现有聚焦测试结果：规划期只读审计；现有 `ShopPurchaseAudit` 能记录商店前后完整卡牌/符文，但免费授卡、开局/读档/遭遇边界没有统一快照，普通弓/枪也不会普遍触发符文伤害日志。
- 共享契约 / 难合并资源风险：远端近期修改 `ReEchoWeaponActor.*` 与 `MOD-ReEchoWeapons.md` 的表现布局；本 Plan 必须在最新实现上做最小日志适配，不覆盖任何 Transform/VFX/朝向变化。最终预构建包与所有程序发布共享。
- 基线损坏时的停止条件：若最新主线无法通过现有静态校验或 BuildTrace 之外的武器/Run 测试失败，先区分基线问题，不通过放宽日志验收或修改玩法掩盖。

## 实现提纲

1. 建立纯只读 BuildTrace 格式化/指纹工具，对容器排序并覆盖构筑身份、装备、卡牌、属性、规则和关键 runtime 状态。
2. 在 Run 权威状态成功提交后的生命周期边界统一调用快照日志；复用现有 ShopPurchaseAudit，不改变事务顺序。
3. 在 WeaponActor 成功 Confirm/发布 Commit 的单一出口记录轻量伤害关联行，玩家和 Echo 共用，失败 Commit 不记录。
4. 补充确定性与触发点自动化，更新模块文档和执行记录。
5. 完成聚焦自动化、静态校验、最终 FullRebuild 和预构建核验；发布前重新审计远端变化。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 纯逻辑/格式 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Diagnostics.BuildTrace` | 指纹确定性、内容敏感性和格式字段通过 |
| Run/Weapons | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Run`；`-Filter ReEcho.Weapons` | 触发点与武器 Commit 无玩法回归 |
| 静态 | `python scripts/validate_project.py`；`git diff --check` | 项目、日志契约、文本不变量通过 |
| C++/发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`；`python scripts/ue/prebuilt_editor.py check` | UHT/UBT 成功并刷新匹配精选预构建包 |
| 远端审计 | `git fetch origin` 后比较候选与最新 `origin/main` | 无遗漏的 Run/Weapon/文档逻辑冲突 |

## 执行记录

### 变化

- 2026-08-26：用户确认随着构筑复杂度增加，偶现问题必须能从日志还原卡牌与符文路径；只读审计确认现有能力仅在商店审计和少数武器专用 Trace 中部分覆盖。
- 2026-08-26：远端依次占用 Plan120、Plan121；按远端编号权威将本任务顺延为 Plan122 并先发布规划。实现从 `origin/main@5aa11dbc` 创建独立 worktree。
- 2026-08-26：新增主模块只读 `ReEchoBuildTrace`，Run 在权威边界写完整构筑快照，WeaponActor 在成功 Confirm 出口写轻量 Commit 关联；未修改 Combat/Weapons 公共消息或玩法数值。
- 2026-08-26：读取用户提供的旧会话 `ReEcho-Run-Brave-Gun-ExplosiveMuzzle-20260826.log`。全日志玩家投射物 152 个 Contact sequence 均进入 Resolver 且正伤害，`applied=0`、`blocked=1`、`incomingAdjustedRaw=0` 均为 0；无伤害片段是 `hitTargets=0` 的射程耗尽。旧日志显示枪弹运行于 `Z=213.12`，高于史莱姆/兔子碰撞盒顶面并可命中半高 130 的狐狸，定位为命中几何前置问题而非构筑/Resolver 清零。由此给新 `[BuildCommitTrace]` 补齐锁定 Target、Origin、Direction，供后续精确区分锁敌和碰撞平面错误；具体玩法修复不混入本诊断 Plan。

### 证据

- `scripts/ue/Build-Editor.cmd -Configuration Development`：成功；最新增量代码编译通过并刷新精选预构建包。
- `scripts/ue/Run-Automation.cmd -Filter ReEcho.Diagnostics.BuildTrace`：3/3 Success（Determinism、Mutations、Summary）。
- `scripts/ue/Run-Automation.cmd -Filter ReEcho.Run`：18/19 Success；唯一失败 `EnemyShardDrops.DataContract` 仍硬编码旧掉落区间，与当前策划 XLSX/CSV 不一致，未触及本 Plan Writes，记录为传入基线问题。
- `scripts/ue/Run-Automation.cmd -Filter ReEcho.Weapons`：发现 17 项；多项生产符文数量/启用状态和旧文本替换断言与当前策划表不一致，最终在 `ReEchoWeaponRuntimeTests.cpp:184` 的旧字符串 `check` 中断。失败均不经过 BuildTrace 代码；本 Plan 不恢复已禁用符文、不改策划表、不扩大范围修旧测试。
- `python scripts/validate_project.py`：PASS；XLSX/CSV 一致、工作流与工程描述符检查通过。
- `python scripts/ue/prebuilt_editor.py check`：PASS，7 个 Editor 模块与当前源码匹配。
- `git diff --check`：通过；机器路径扫描无命中。`ReEcho.Run`、`ReEcho.Weapons` 和最终 FullRebuild 待共享 UE 锁释放后执行。

### 剩余风险

- 日志能还原已记录的状态与触发链，但不能代替确定性复现；未来新增 CardRuntime 字段时需同步审阅诊断摘要覆盖面。

### 人工验收结果/请求

- `NotRequired`；策划后续遇到问题时提交对应 session log 即可。

### 架构文档审阅结果

- `ARCHITECTURE.md`：已审阅；模块拓扑与依赖方向未变化，无需修改。
- `README.md`：已审阅；AREA 路由未变化，无需修改。
- `MOD-ReEcho.md`：已记录 BuildTrace 的主模块所有权、边界触发点、日志位置和只读约束。
- `MOD-ReEchoWeapons.md`：已记录 WeaponActor 的 Commit 关联边界，Weapons 模块不反向依赖 Run/Card。
- `MOD-ReEchoCombat.md`：已审阅；未向公共攻击身份或 Resolver 契约加入构筑字段，无需修改。
