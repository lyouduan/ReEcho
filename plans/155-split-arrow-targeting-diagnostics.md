# Plan 155 - 程序 - 分裂箭头目标诊断与修复

## 协调

- Planner 负责人：Codex。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`InProgress`。
- 人工验收：`PendingBeforeClose`（先由用户复现并提供诊断日志；确认根因并完成修复后再验证分裂实际命中其他怪物）。
- 本地规划 / 实现基线：`origin/main@fe6b0320ae1531d29b850bcf6f49cbd133c73a8e`。
- 本地实现方式（可选，仅作交接说明）：独立工作树 `ReEcho-plan155`、分支 `plan/155-split-arrow-targeting-diagnostics`。
- 依赖 / 阻塞：第一阶段只建立可关联母箭、子箭预定目标与实际命中目标的诊断证据；最终行为修复等待用户实测日志确认，不凭静态推断直接改变投射物碰撞语义。
- Writes:
  - `plans/155-split-arrow-targeting-diagnostics.md`
  - `Source/ReEcho/Public/Graybox/ReEchoProjectileActor.h`
  - `Source/ReEcho/Private/Graybox/ReEchoProjectileActor.cpp`
  - `Source/ReEcho/Public/Weapons/ReEchoWeaponActor.h`
  - `Source/ReEcho/Private/Weapons/ReEchoWeaponActor.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoWeaponRuntimeTests.cpp`
  - 如日志证明公共逻辑投射物需携带排除目标：`Source/ReEchoWeapons/Public/Weapons/ReEchoWeaponTypes.h`
  - 如日志证明命中扫描需消费排除目标：`Source/ReEchoWeapons/Public/Weapons/ReEchoProjectileLogicComponent.h`
  - 如日志证明命中扫描需消费排除目标：`Source/ReEchoWeapons/Private/Weapons/ReEchoProjectileLogicComponent.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
- Stable Reads:
  - `Content/Data/parts.csv` 的 `P_BOW_SPLIT_ARROWHEAD_I/II/III`
  - `Content/Data/part_effects.csv` 的 `Part.ProjectileSplitOnHit`
  - `ReEchoCombatTarget` 的路径相交与阵营过滤契约
  - `ReEchoHitResolver` 的最终伤害结果
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：普通弓箭、枪械、穿透、爆炸、元素、Echo 武器与非分裂投射物保持现状；诊断日志只存在于非 Shipping；最终修复不得让子箭递归分裂或重复结算母箭命中的目标。
- 明确排除：不改分裂数量、倍率、射程、爆炸范围、部件表或策划文本；不改分裂箭视觉资产；不借本任务调整自动索敌、敌人碰撞盒或其他武器符文。

## 锁定目标

1. 对装备分裂箭头的弓，记录母箭命中目标、母箭命中位置、候选排序、每支子箭的预定目标、生成位置/方向，以及子箭最终实际接触和结算的目标，使一次用户复现日志足以判断“生成时仍在母目标碰撞范围内”“方向/坐标错误”或“命中扫描选错对象”。
2. 日志确认后实施最小修复：母箭首次命中的怪物不得再次成为该批子箭的有效命中对象；子箭仍沿各自候选方向使用统一逻辑投射物与 Combat 结算，不创建武器专用第二套伤害逻辑。
3. 分裂候选继续排除母目标并按距离与稳定 TieBreak 排序；至多生成表中 `MaxChildren` 支。每支子箭只产生一次有效直接命中，不递归触发再次分裂。
4. 新增确定性自动化，必须实际推进子箭并断言母目标生命不再下降、至少一个其他候选受到子箭伤害，而不再只统计 Actor 数量。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoWeapons` / `AREA-Weapons`，`MOD-ReEcho` / `AREA-Presentation` / `AREA-Tests`；Combat 只作为稳定命中结算依赖，不修改 `MOD-ReEchoCombat`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md` 的分裂投射物目标/排除契约；维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 的世界 Projectile Host 诊断和装配职责。两者均已加入 `Writes`。
- 设计意图：候选选择、投射物移动/相交和最终伤害分别保留在既有所有者中；分裂效果只为逻辑投射物提供明确的来源命中排除信息，不通过表现位置、Niagara 或 Actor 遍历顺序决定伤害。
- 权威状态与依赖：当前候选列表由 `AReEchoWeaponActor::SpawnSplitProjectiles` 计算，实际首次接触由 `UReEchoProjectileLogicComponent` 决定，伤害仍由 Combat 结算。若确认需要排除母目标，排除集合属于该支逻辑投射物的瞬时规格/状态，不进入存档、CSV 或表现 Actor 的第二份真相；依赖方向仍为 `ReEcho -> ReEchoWeapons -> ReEchoCombat`。
- 决策记录：
  - 静态代码已确认候选选择显式排除 `Result.Target`，因此不把问题误归因于候选数组。
  - 当前高概率原因是子箭仅从命中点向外偏移 `18 cm`，Carrier 半径约 `13 cm`，且新投射物 `HitTargets` 为空；较大的母目标 Hurtbox 可能在第一逻辑步再次与全部子箭相交。
  - 第一阶段不直接扩大偏移距离，因为不同怪物碰撞尺寸会让魔法距离再次失效；日志若证实重叠，应优先使用类型化“初始排除目标”契约。
  - 子箭可被路径上的另一名合法敌人拦截，这是统一投射物语义；但绝不能再次命中产生它的母目标。是否需要强制命中各自预定目标，只有日志/产品证据证明现有直线拦截仍不符合预期时才另行决策。
  - 现有测试只断言生成数量与爆炸半径，未推进子箭或检查受伤对象，必须补行为回归测试。
- 相关文档同步范围：关闭前审阅并维护 `MOD-ReEchoWeapons.md` 与 `MOD-ReEcho.md`；审阅 `ARCHITECTURE.md` 和 `README.md`，预计模块拓扑、稳定标识与路由不变，无事实变化时只在执行记录说明无需修改。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEchoWeapons.md`：记录最终分裂目标排除/命中契约与测试入口；
  - `MOD-ReEcho.md`：记录 Projectile Host 的诊断关联或最终装配接口；
  - `ARCHITECTURE.md`、`README.md`：记录已审阅、已更新或无需修改及原因。

## 锁定验收

- [ ] 用户复现日志能以同一母箭/子箭身份串联：母目标、预定目标、生成点、方向、第一次实际接触目标和最终伤害结果。
- [ ] 日志确认后的修复使每支分裂子箭不再命中母箭首次命中的怪物，并能对其他合法怪物产生伤害。
- [ ] 无其他合法目标时不生成无目标子箭；候选数量、稳定排序、伤害倍率、爆炸/穿透组合和禁止递归分裂保持既有语义。
- [ ] 自动化实际推进子箭并验证受伤对象；Weapon/Projectile 聚焦测试、Development 构建、静态校验和最终发布构建通过。
- [ ] 用户在 PIE 使用分裂箭头复测，确认子箭明显朝其他怪物飞行并实际伤害其他怪物，母目标不再吃到全部分裂伤害。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物、完整运行日志或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@fe6b0320ae1531d29b850bcf6f49cbd133c73a8e`；远端最大已发布 Plan 为 `154`，本任务占用 `155`。
- 引擎/构建可用性：首次构建前运行 `python scripts/setup_lfs.py --check` 并确认该工作树 Editor 已关闭；诊断候选交付用户前执行 FullRebuild，确保日志进入可直接运行的 Editor 包。
- 现有聚焦测试结果：规划阶段只完成静态源码审计，尚未运行 `ReEcho.Weapons.Runes.ProjectileSplitPierceExplosion` 基线；该测试当前只统计 3 个子投射物和爆炸半径，未验证实际命中目标。
- 共享契约 / 难合并资源风险：`ReEchoWeaponActor.*`、`ReEchoProjectileActor.*` 与逻辑投射物规格是高频战斗路径；与其他远端武器/Projectile 修改存在逻辑耦合时需先审计。精选 DLL/manifest 不能选择任一侧，最终组合必须重建。
- 基线损坏时的停止条件：最新 main 已改变分裂箭产品语义、存在另一份未合入分裂修复、或用户日志表明问题并非母目标重叠而需要强制寻的/改碰撞体等产品选择时，停止越界修复并报告。

## 实现提纲

1. 第一阶段加入非 Shipping `[SplitArrowTrace]`：母命中摘要、候选排序、子箭生成与预定目标、子箭实际接触/结算；使用 Actor 名和 Projectile GUID/攻击序列形成可关联证据。
2. FullRebuild 后交给用户从 `ReEcho-plan155` 运行一次装备分裂箭头的复现；读取最新按会话留存的 `Saved/Logs/ReEcho-session-*.log`，只提取 `[SplitArrowTrace]` 及关联命中行。
3. 根据证据选择最小修复；优先为逻辑投射物注入初始排除目标，禁止以固定更大生成偏移掩盖不同 Hurtbox 尺寸。
4. 扩展现有 ProjectileSplit 自动化，实际推进子箭并检查母目标/其他目标生命与禁止递归，再更新模块文档和执行记录。
5. 完成 Development/FullRebuild、聚焦自动化、静态校验与用户 PIE 复测后再关闭和发布实现。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Plan/静态 | `python scripts/validate_project.py`、`git diff --check` | Plan 编号、Schema、源码与文档不变量通过 |
| LFS | `python scripts/setup_lfs.py --check` | 当前工作树资产完整 |
| C++ | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 诊断/最终候选编译成功且精选 Editor 包匹配源码 |
| Weapon | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Weapons.Runes.ProjectileSplitPierceExplosion` | 分裂生成、组合效果与最终目标行为通过 |
| 日志 | PIE 一次装备任一级分裂箭头、场上至少 4 个敌人的复现 | `[SplitArrowTrace]` 串联母目标、不同预定目标与实际命中目标 |
| 人工 | 修复后重复同一构筑/场景 | 子箭不再集中伤害母目标，能攻击其他怪物 |

## 执行记录

### 变化

- 第一阶段诊断已加入：分裂母箭命中、候选排序、子箭预定目标/生成信息，以及子箭第一次实际结算目标均使用 `[SplitArrowTrace]` 和 Projectile GUID 串联。
- 本阶段未改变候选、生成、碰撞或伤害行为，等待用户日志确认根因。

### 证据

- `python scripts/setup_lfs.py --check`：通过，3 个 LFS 文件均已 hydrated。
- `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`：通过，97 个 action 完成，精选 Editor 预构建包已刷新。
- `python scripts/validate_project.py`：构建后通过；XLSX/CSV、规则 Schema、预构建指纹均一致。
- `git diff --check`：通过（仅报告工作树的预期 LF/CRLF 提示）。
- `scripts/ue/Run-Automation.cmd -Filter ReEcho.Weapons.Runes.ProjectileSplitPierceExplosion`：在测试发现前被本机 UE 5.8 `ValidatePlatforms -AllPlatforms` 的 LinuxArm64/VisionOS `SDK.json MainVersion` 环境门禁阻断；与 Plan111 已记录的本机问题一致，不冒充测试通过。诊断源码已由 UHT/UBT 完整编译。

### 剩余风险

### 人工验收结果/请求

- `PendingBeforeClose`：请从 `ReEcho-plan155` 启动 PIE，装备任一级分裂箭头并在至少 4 名敌人靠近时触发一次分裂；退出 Editor 后由 Planner 读取最新 session log 的 `[SplitArrowTrace]`，再确认并实施根因修复。

### 架构文档审阅结果

- 待实现/关闭阶段填写。
