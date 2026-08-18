# Plan 47 - Cards - 卡牌构筑独立模块与新表全量实现

## 协调

- Planner 负责人：Codex。
- Executor 负责人：Codex（本对话不采用 Planner-Executor 分离，由同一 AI 规划、实现、评审与集成）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划基线：`origin/main@22df09ab90054971eb4dbb79ec3ad7b1be0b4ab2`；最终实现候选先整合 Plan48，再按用户确认以 `origin/main@39136cd` 为远端优先基线合并 Plan42 Gameplay Blueprint、敌人表现树与预构建更新。
- 本地实现方式：独立 worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan47-card-build`，分支 `plan/47-card-build-runtime`。
- 依赖 / 阻塞：依赖已关闭的 Plan18/22/24/25/30/31/41/43/44 所建立的数据快照、存档、Echo、Combat、Weapons、Enemies 契约；不修改 Plan45 的 WBP/纹理，只维护可合并的 C++ 功能适配；Plan48 当前仅发布计划和预构建刷新，已并入最终基线，后续实现须在本模块公共契约上继续集成。
- Writes：本 Plan；`ReEcho.uproject`；`Source/ReEchoCards/**`；`Source/ReEcho/ReEcho.Build.cs`；`Source/ReEcho/{Public,Private}/{Core,Data,Run,Recording,Encounter,Combat,Weapons,Player,Graybox,UI}/**` 与 `Source/ReEcho/{Public,Private}/ReEchoGameMode.*`；必要的 `Source/ReEchoCombat/**`、`Source/ReEchoEnemies/**`、`Source/ReEchoWeapons/**`；`Config/DefaultEngine.ini` 中精确反射重定向（仅确有类型迁移时）；`Design/Data/ReEchoData.xlsx`；与它同批生成的 `Content/Data/**`；`scripts/data/**`、相关静态校验/测试；`shared/CODEBASE_MAP/{ARCHITECTURE.md,README.md,modules/MOD-ReEcho.md,modules/MOD-ReEchoCards.md,modules/MOD-ReEchoCombat.md,modules/MOD-ReEchoEnemies.md,modules/MOD-ReEchoWeapons.md,modules/MOD-ReEchoUI.md}`；最终 `GIT_RULES.md` 允许的 Win64 Editor 预构建包。
- Stable Reads：外部策划源 `C:\Users\gavynqiu\Documents\miniGame\【开普勒】回响数值与构筑体系.xlsx`（确认时 SHA-256 `A7C53D56890B86627CAA0068E1598B1150A220E24D5BA1D776D3E8655A08720E`）；`ReEchoAudio` 公共 API；Plan45 的 UI 绑定/资产边界；现有角色、元素、武器、敌人、商店、Echo 和录制数据领域。
- 影响模式：`SharedContract`（卡牌状态、Combat 事件/命中接缝、Echo/Weapons/Enemies/Shop 适配、存档与录制）；`Exclusive`（`Design/Data/ReEchoData.xlsx`、同批生产 CSV、模块描述符和最终预构建包）。
- 兼容承诺 / 下游操作：保留当前角色选择、普通结算抽卡、锻炼、商店、Echo 存储/回放和武器装备主流程；普通卡池仍按现有确定性生成流程提供候选，Tier 用于具名的分级随机赠卡与过滤；旧 v8 保存/录制按旧运行时 ID 语义迁移到 v9，不把旧 `G_1_03`“应急格挡”误解释成新 `G_1_03`“物理强化”；新保存固定 Card 数据领域版本，后续不兼容数据明确拒绝恢复。
- 明确排除：不新增或重做美术、动画、音频和 WBP 二进制资产；不改变工作簿“效果详解（对程序）”明确给出的数值/行为；不引入任意文本脚本、表达式求值器或第二份平衡常量；不改变与卡牌无关的战斗、武器、敌人、Echo、商店玩法；不实施已从新表删除的 `G_2_01/G_2_02/G_2_03/G_2_11/G_3_15/G_3_18`；不保留旧运行时占位卡作为可抽取卡。

## 锁定目标

以外部工作簿 `构筑体系G` 的 39 个有效 ID 为新卡牌构筑权威，且“效果详解（对程序）”高于同表用户文案和旧实现。把卡牌目录、持有/冲突/抽取规则、事件状态和效果决策从 `UReEchoRunSubsystem` 抽到独立 Runtime Module `ReEchoCards`，然后用新表覆盖仓库工作簿中旧的卡牌设计与规范化 `tblCards` / `tblCardEffects`，事务式再生并校验生产 CSV。最终 39 张卡全部可被数据加载、合法获得、保存/恢复，并在其规定事件上产生可自动化观察的完整行为；主模块只负责把世界、Combat、Weapons、Enemies、Echo、Encounter、Shop 与 UI 输入适配到卡牌模块的类型化命令/结果。

权威数据顺序固定为：外部工作簿程序说明 → 仓库 `Design/Data/ReEchoData.xlsx` 规范化表 → 事务式生成的 `Content/Data/*.csv` → 类型化 C++ 快照。运行时不得解析中文说明，也不得在 C++ 中复制卡牌数值。

## 锁定产品语义

- 新表共 39 张：L1 8 张、L2 13 张、L3 18 张；`静默刻度` 的稳定 ID 为 `G_2_17`，`复调机括` 为 `G_3_22`。
- 普通结算候选沿用当前确定性抽取、低持有层数优先和候选确认流程；除卡牌自己的冲突/过滤规则外，39 张均为批准、启用且可进入普通卡池。L1/L2/L3 不额外引入未定义的关卡解锁表，只供 `命运跃迁I/II/III` 的精确 Tier 随机赠卡和测试筛选。
- `StackPolicy` 默认按表中同一卡可重复获得；语义上只能生效一次或会建立唯一长期模式的卡（例如双重回声、时空锚点、涅槃之身、代价丰收、复调机括）使用显式 `Unique`。冲突和过滤由稳定 Tags/ConflictPolicy 决定，不匹配显示文本。
- 计数除法按完整阈值结算：击杀数 `/5`、反应数 `/4`、击杀数 `/15`、关末击杀数 `10%` 均向下取整；未满阈值余数在该卡规定的同一统计窗口内保留。所有“永久”指当前 run 余下时间并进入保存/录制快照；“下一关”只作用于获得卡后的下一个 Encounter。
- 伤害前、受伤前、暴击、元素反应、击杀、Echo 死亡、购买、关卡时间点与关末结算均以类型化确定性事件驱动；同一个 Combat identity/Reaction identity 只结算一次。暂停不推进周期或冷却；保存恢复后不重放已经提交的阈值事件。
- 距离统一使用 Unreal 厘米换算：1m = 100cm，4m = 400cm。周期 2 秒、冷却 1.5 秒、眩晕 2 秒及第 10/20 秒触发均使用权威模拟/世界时间并保存剩余状态。
- `G_3_20` 按程序说明最低减到 0；不采用用户文案“最低 1”。
- `G_3_02` 的“时间锚点”保存一个稳定 RecordingId；玩家可从已有 Echo 记录中设定/替换/清除锚点，锚定后回放只解析该记录，直到卡牌状态或锚点被改变。功能入口复用现有 Echo 管理面与 C++ 控件，不修改 Plan45 的 WBP/纹理资产。

### 39 张卡的验收行为

| ID | 程序行为验收 |
|---|---|
| `G_1_01` | 获得时移动速度 `+10%`。 |
| `G_1_02` | 获得时生命上限和当前生命各 `+8`。 |
| `G_1_03` | 获得时物理攻击力 `+4`。 |
| `G_1_04` | 获得时元素攻击力 `+4`。 |
| `G_1_05` | 获得时暴击率与暴击效果各 `+25%`。 |
| `G_1_06` | 获得时元素反应效能 `+25%`。 |
| `G_1_07` | 获得时回响效能 `+10%`。 |
| `G_1_08` | 原子地随机授予一张合法 L2 卡；无合法候选时本卡申请失败且不产生半成品状态。 |
| `G_2_04` | 获得时在物攻/元攻之间确定性随机选择一项乘 `0.7`，另一项乘 `1.6`，选择结果进入状态快照。 |
| `G_2_05` | 统计下一关玩家与 Echo 合计击杀，每满 5 个在关末永久物攻 `+1`。 |
| `G_2_06` | 统计下一关玩家与 Echo 合计触发的元素反应，每满 4 次在关末永久元攻 `+1`。 |
| `G_2_07` | 每个存活 Echo 每 2 秒向 400cm 内存活敌人附着水元素。 |
| `G_2_08` | 每个存活 Echo 每 2 秒向 400cm 内存活敌人附着草元素。 |
| `G_2_09` | 400cm 内存活 Echo 光环使敌人移动速度降低 30%；离开所有光环后恢复，不叠加重复光环。 |
| `G_2_10` | 原子地随机授予一张合法 L3 卡。 |
| `G_2_12` | 带任意元素附着的敌人受到玩家或 Echo 暴击时，暴击效果额外 `+100%`。 |
| `G_2_13` | 玩家触发元素反应后恢复 2 生命，冷却 1.5 秒；Echo 触发不计。 |
| `G_2_14` | 每关玩家前三个合法伤害 Intent 结算为 0；第四个合法伤害正常结算并移除本关全部 Echo；计数每关重置。 |
| `G_2_15` | 获得时时间碎片归零；下一关敌人的基础掉落额外乘 `1.5`，关末失效。 |
| `G_2_16` | 商店价格统一折扣 20%；每次成功购买任意商品后当前生命和生命上限各 `+2`，失败购买不触发。 |
| `G_2_17` | 每关权威时间首次跨过 10 秒和 20 秒时，各使所有存活怪物眩晕 2 秒；每个阈值只提交一次。 |
| `G_3_01` | 下一关默认解析上一关和上上关两份有效录制，至多生成两个 Echo；不足两份时只生成已有记录。 |
| `G_3_02` | 可将一份已有 Echo 记录设为时间锚点；锚定后所有后续关卡只重复该记录，稳定 ID 随保存恢复。 |
| `G_3_03` | 禁止 Echo 出现；玩家全部基础属性乘 2；之后候选与随机赠卡排除带 `Echo` 标签的卡。 |
| `G_3_04` | Echo 最大/当前生命为玩家对应值的 3 倍、不能攻击、永久嘲讽；每次 Echo 被怪物击败后玩家当前/最大生命永久 `+3`。 |
| `G_3_05` | Echo 每累计击杀 15 个敌人，玩家物攻和元攻永久各 `+1`；玩家每累计击杀 15 个敌人，回响效能永久 `+2%`。两套余数独立保存。 |
| `G_3_07` | 敌人的元素免疫持续时长统一降为 0.5 秒；不改变玩家/Echo 的免疫时长。 |
| `G_3_09` | 玩家或 Echo 暴击的命中在结算前确定性随机覆盖为水/火/雷/草之一，覆盖武器原元素并保存随机序列状态。 |
| `G_3_10` | 元素伤害进入合法暴击判定；未持有时保持当前元素伤害不暴击规则。 |
| `G_3_11` | 玩家或 Echo 造成伤害前，按玩家与参与 Echo 的距离每完整 100cm 增伤 3%；没有存活/可解析 Echo 时增幅为 0。 |
| `G_3_12` | 玩家或 Echo 每次伤害执行两次独立且确定性的暴击判定，任一次成功即为暴击，不能把一次 Hit 复制成两次伤害。 |
| `G_3_13` | 原子地随机授予两张互不相同的合法卡，其中至少一张为 L3；候选不足时明确失败，不部分授予。 |
| `G_3_14` | 获得时生命上限增加当时物攻与元攻之和，然后恢复到满生命。 |
| `G_3_16` | 每关结束按玩家与 Echo 合计击杀数的 10% 向下取整增加免费商店刷新次数；免费次数优先消费并保存。 |
| `G_3_17` | 获得时增加 3000 时间碎片，并确定性随机永久施加一个代价：禁用刷新、禁购额外卡牌组、或敌人不再掉落时间碎片；代价进入快照且只选一次。 |
| `G_3_19` | 玩家或 Echo 每次造成伤害前，若至少持有 1 时间碎片则原子消费 1 并令该次伤害乘 `1.2`；同一 Hit 不重复消费。 |
| `G_3_20` | 玩家每次受到伤害前，若至少持有 5 时间碎片则原子消费 5 并令该次伤害减 1，最低为 0；Echo 受伤不消费。 |
| `G_3_21` | 记录玩家触发过的不同 ReactionId；每形成一组 4 种不同反应，反应效能永久 `+5%`，随后清空本组集合开始下一组。 |
| `G_3_22` | 每个非 `Core` 武器槽位容量变为原定义的 2 倍/至少可装备 2 个配件，`Core` 保持原容量；装备重建、换武器、保存和录制一致。 |

## 架构影响与设计决策

- 受影响架构标识：新增 `MOD-ReEchoCards` / `AREA-Cards`；直接修改主装配与数据/Run/Save/Echo/Shop 的 `MOD-ReEcho` / `AREA-Data` / `AREA-Run` / `AREA-Recording` / `AREA-UI`；公共命中与元素事件影响 `MOD-ReEchoCombat` / `AREA-AbilityCombat`；眩晕、减速、嘲讽目标和 Echo 不攻击策略影响 `MOD-ReEchoEnemies`；配件槽容量和有效武器定义影响 `MOD-ReEchoWeapons` / `AREA-Weapons`。
- 对应模块文档：创建 `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`；维护 `MOD-ReEcho.md`、`MOD-ReEchoCombat.md`、`MOD-ReEchoEnemies.md`、`MOD-ReEchoWeapons.md` 和 `MOD-ReEchoUI.md`，均已加入 `Writes`。
- 设计意图：卡牌模块负责“卡是什么、持有哪些、何时触发、状态如何推进、产生什么类型化效果”，主模块负责“世界中的谁/哪里/商店/存档如何执行”。卡牌模块不查询 Widget、CSV、RunSubsystem、GameMode、具体 Enemy/Echo/Weapon Actor 或表现资产。
- 权威状态与依赖：`ReEchoCards` 独占 `FReEchoCardCatalog`、`FReEchoCardBuildState`、`FReEchoCardRuntimeState`、确定性随机游标、计数/冷却/一次性标记、候选/冲突规则和事件到 Action 的映射；`ReEcho` 的 Build/Save/Recording 快照嵌入这些值并固定 CardDomainRevision，不复制可写卡牌状态。依赖固定为 `ReEcho -> ReEchoCards -> ReEchoCombat`；`ReEchoWeapons -> ReEchoCombat`、`ReEchoEnemies -> ReEchoCombat` 保持；Combat/Weapons/Enemies 都不得反向依赖 Cards 或 ReEcho。
- 公共契约：Cards 接收资源无关的 `OnCardGranted`、EncounterStarted/Tick/Completed、BeforeOutgoingHit、BeforeIncomingHit、HitResolved、ReactionResolved、PurchaseCompleted、EchoKilled 等类型化上下文，返回原子的 ActionBatch/不可变规则快照。Combat 新增通用、卡牌无关的命中前修正规则接口和 Reaction 结果事件；主模块 Actor 上的适配器实现接口并调用 Cards。Cards 不直接扣血、改 Enemy 状态、扣时间碎片或生成 Actor。
- 数据边界：CSV 读取/整包校验仍由 `ReEcho` Data Adapter 编排，再一次编译为 `ReEchoCards` 自有不可变 Definition/Catalog；Cards 不 include CSV Row。`cards` / `card_effects` 扩稳定的触发、行为和 typed parameter 契约，但不把中文说明或自由表达式当运行时规则。
- 事务边界：获得一张卡及其连锁赠卡先在候选 Build/卡牌状态/经济状态上完整求值，全部成功后一次提交；伤害前消费时间碎片和伤害修改也是单次 ActionBatch，执行失败不得出现“已扣货币但未增伤”或反向半状态。
- Echo 边界：Cards 只输出 Echo 数量、锚点、攻击许可、生命倍率、嘲讽和光环规则；`ReEchoGameMode`/Echo Actor 适配实际录制解析与世界对象。录制保存获得卡时固定的 Build/CardDomainRevision；运行中全局数据重载不改变已创建 Echo。
- Weapons 边界：Cards 输出非核心槽容量倍率/规则，主模块武器数据适配据此编译有效装备；Weapons 模块仍只消费资源无关 Definition，不读取卡牌 ID。
- Enemies 边界：Combat 持有眩晕/减速等受控状态，Enemies 从 Combat snapshot/通用命令消费移动与行动门控；嘲讽目标由主模块提供显式 sense 目标，Enemies 不识别卡牌 ID。
- UI 边界：沿用现有 Trait/Inventory/Shop/Echo 管理页面与委托；只增加展示/命令所需的 C++ 可合并契约和无资产 fallback 控件，不修改 Plan45 正在维护的 `.uasset`、纹理或视觉版式。
- 决策记录：不把 39 张卡写成 `switch(CardId)` 分散到 Run/GameMode/Actor；数据只选择已注册的 C++ BehaviorId，具体处理器在 Cards 集中注册后才加载生产表。相比让 Combat 依赖 Cards，采用 Combat 通用规则接口 + 主模块适配，以保护最终伤害裁决权和无循环依赖。
- 相关文档同步范围：更新 `ARCHITECTURE.md` 的五个玩法模块拓扑、Cards 事件流与跨模块不变量；更新 `README.md` 的 `MOD-ReEchoCards` / `AREA-Cards` 路由以及 `AREA-Run` 边界；创建/维护上述六份模块文档。`MOD-ReEchoAudio` 关闭前只读审阅，因为卡牌复用既有语义事件且不改 Audio API。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：记录新增依赖与状态流；
  - `shared/CODEBASE_MAP/README.md`：记录新增模块/AREA 路由；
  - `MOD-ReEchoCards.md`：记录存在原因、状态所有者、契约、依赖、流程、扩展、测试和代码位置；
  - `MOD-ReEcho.md`、`MOD-ReEchoCombat.md`、`MOD-ReEchoEnemies.md`、`MOD-ReEchoWeapons.md`、`MOD-ReEchoUI.md`：逐项记录已更新或已审阅无需修改及原因；
  - `MOD-ReEchoAudio.md`：已审阅无需修改或记录真实公共契约变化。

## 锁定验收

- [x] `ReEchoCards` 是可单独构建和自动化的 Runtime Module，依赖方向为 `ReEcho -> ReEchoCards -> ReEchoCombat`，且 Cards 不依赖 ReEcho/Weapons/Enemies/Audio/UI/资源资产。
- [x] 39 个表中 ID 与程序说明逐项对应；全部 Approved/Enabled/Offerable，六个删除 ID 和旧 `Runtime.G_1_03` 不在新卡池，`G_2_17`/`G_3_22` 正式存在。
- [x] 新表覆盖仓库 `构筑体系G` 设计区和规范化 `tblCards`/`tblCardEffects`；`_ExportMap`、Schema、manifest、C++ Reader 和行为注册同步；权威 `--check` 无漂移，XLSX/CSV 作为同一发布单元。
- [x] 所有 OnApply 卡、三张分级赠卡、冲突/Unique/回响过滤和确定性随机有原子性自动化；无合法候选时不部分修改状态。
- [x] 所有 Combat/Element 卡在统一 Hit/Reaction identity 上单次触发，Combat 继续独占最终伤害/生命/元素/死亡；Cards/Actor/Widget 不直接形成第二套结算。
- [x] 所有 Encounter/Echo/Enemy 卡覆盖跨关计数、10/20 秒阈值、2 秒光环、眩晕/减速/嘲讽、双 Echo/锚点/禁用 Echo、Echo 死亡成长与暂停/保存恢复。
- [x] 所有 Shop/Economy 卡覆盖折扣、购买成长、一次性下一关掉落、免费刷新、永久代价和伤害前原子时间碎片消费；无负货币、重复消费或失败半提交。
- [x] `G_3_22` 在装备、换武器、保存/恢复和录制快照中允许非核心双配件且核心不变；Weapons 不认识 CardId。
- [x] v8 保存/录制按旧表来源语义迁到 v9：旧 `G_1_04→G_1_03`、`G_1_05→G_1_04`、`G_1_08→G_1_07`，旧运行时 `G_1_03` 明确移除而不误映射；已物化的旧数值安全保留，新事件状态使用明确默认值。v9 固定 CardDomainRevision，坏 ID/坏状态/不兼容领域版本明确拒绝。
- [x] 现有角色晋升、Forge、普通候选 UI、商店、Echo 存储/选择、Combat、Weapons、Enemies、Save/Recording 自动化不回归；Plan45 WBP/纹理资产不被修改。
- [x] 聚焦数据/卡牌/Combat/Element/Encounter/Echo/Enemies/Weapons/Shop/Save/Recording 测试和完整 `ReEcho` 自动化 98/98 通过；UE 5.8 Editor 全量重建、项目校验、工作簿公式错误扫描及 `git diff --check` 通过。
- [ ] 用户在 PIE 验收：普通抽取和三种跃迁、卡牌说明/库存、商店折扣/刷新/购买、双 Echo/锚点/无 Echo、眩晕/减速/嘲讽、关键伤害前效果、保存退出/继续和双配件可用性。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物、临时工作簿、分析脚本或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@22df09ab90054971eb4dbb79ec3ad7b1be0b4ab2`；Plan 编号前远端最大为 46，本文件为 47。
- 引擎/构建可用性：UE 5.8 安装版与仓库 Windows 构建入口预计可用；发布 Plan 和每次程序推送均执行准确候选 `-FullRebuild`、刷新精选预构建包并运行项目校验。
- 现有聚焦测试结果：Plan 发布前只复用 main 的静态/构建基线，不把旧卡牌测试视作新 39 张行为证据；实现开始后先记录当前 Data/Run/Combat/Weapons/Echo/Save 聚焦基线。
- 共享契约 / 难合并资源风险：`ReEcho.uproject`、Build.cs、Combat types/events、Run/Save snapshot、`Design/Data/ReEchoData.xlsx`、生成 CSV 和预构建包均为共享热点；Plan45 的 WBP/纹理为并行二进制热点且明确不写。
- 基线损坏时的停止条件：发布前 fetch 发现 `origin/main` 在批准基线后前进；外部工作簿 hash/39 行语义变化；必须修改未确认的产品语义；无法保持 Combat/Weapons/Enemies 对 Cards 的零反向依赖；无法事务式发布 XLSX+CSV；UE Editor/commandlet 正被其他 worktree 使用。

## 实现提纲

1. 发布并核验 Plan 47，记录数据/Run/Combat/Weapons/Echo/Save 基线与 39 张卡行为清单。
2. 创建空 `ReEchoCards` Runtime Module、模块边界校验和 `MOD-ReEchoCards.md`，先证明单向依赖与独立构建。
3. 定义 Cards 自有 Definition/Catalog/BuildState/RuntimeState/Event/RuleSnapshot/ActionBatch；将候选生成、持有/Unique/冲突、Tier 随机和原子 Grant 从 Run 抽出。
4. 扩展规范化 `cards`/`card_effects` typed Schema、静态/C++ 校验和集中 Behavior 注册；迁移 Data Adapter 使其编译并发布不可变 Cards catalog。
5. 先实现八张 L1、三张跃迁和纯 OnApply/永久数值卡，建立候选-提交与装备基底重算不变量。
6. 建立 Combat 通用命中前规则接口、Reaction 结果事件和 Cards 主模块适配，完成暴击/元素/距离/时间碎片/免伤/恢复/击杀成长行为。
7. 建立 Encounter/Echo/Enemies 生命周期适配，完成下一关统计、周期光环、阈值眩晕、双 Echo、锚点、禁 Echo、嘲讽与 Echo 死亡成长。
8. 建立 Shop/Economy 与 Weapons 槽位适配，完成折扣、购买成长、掉落倍率、免费刷新/禁用规则、经济代价和非核心双配件。
9. 升级 Build/RunSave/Recording 到 v9，实施旧 ID 语义迁移、CardDomainRevision 固定和卡牌运行态恢复；覆盖旧存档、坏状态和全局数据重载。
10. 使用电子表格事务流程把外部 39 行映射进仓库工作簿的设计区与规范化表，保留其他领域表；整包生成/验证后原子发布 CSV，并渲染/检查卡牌表及公式错误。
11. 更新相关模块文档和架构索引，格式化 C++，运行聚焦测试、完整 ReEcho 自动化、FullRebuild、validator 和 diff-check；进入 `Review` 后交用户 PIE。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 工作簿输入 | SHA-256、39 行/39 ID、Tier 分布、程序说明抽取 | 与确认源一致，`G_2_17`/`G_3_22` 存在，0 缺 ID |
| 数据生成 | 聚焦 Python 数据测试；`python scripts/data/sync_xlsx_to_csv.py --check`（或实现后的等价权威入口） | XLSX、`_ExportMap`、Schema、manifest、CSV 无漂移，坏触发/参数/外键被拒绝 |
| 工作簿 QA | artifact-tool inspect/render；公式错误扫描 | `构筑体系G` 设计区和规范化表可读，0 `#REF!/#DIV/0!/#VALUE!/#NAME?/#N/A` |
| 模块依赖 | Build.cs/include 静态审计 | `ReEcho -> ReEchoCards -> ReEchoCombat`，Cards 无主模块/世界/资源反向依赖 |
| Cards 纯规则 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Cards` | 39 卡逐项、Tier/Unique/冲突/随机/原子 Action/状态序列通过 |
| 跨域聚焦 | Data、Run、Combat、Element、Encounter、Echo、Enemies、Weapons、Shop、Save、Recording filters | 事件只触发一次、状态所有者唯一、暂停/恢复/重载/装备一致 |
| 完整自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | 全套通过，无既有行为回归 |
| 格式/静态 | `.clang-format` 修改的 C++；`python scripts/validate_project.py`；`git diff --check` | 代码规范、项目不变量和差异通过 |
| 构建/发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 六个 Runtime Module 编译成功，精选 Editor manifest/DLL/源码指纹匹配 |
| 人工 | 用户 PIE 具名清单 | 抽取、卡牌可读性、商店、Echo/锚点、敌人状态、伤害、存档和双配件报告 Passed 或具体返工项 |

## 执行记录

### 变化

- 2026-08-17：用户确认以程序说明为准、`静默刻度` ID 已补、抽出卡牌构筑模块，并以新表覆盖原表后实现全部 39 张卡。
- 2026-08-17：用户确认基线、worktree、模块依赖、数据覆盖、旧 ID/保存迁移和完整验证范围；Plan 47 进入 `Ready`，发布后由同一 AI 执行。
- 2026-08-17：创建独立 `ReEchoCards` Runtime Module，将卡牌目录、合法授予、状态、确定性随机和类型化事件规则从 Run 中抽出；Run/GameMode/Combat/Echo/Enemies/Weapons/Shop 只保留领域适配。
- 2026-08-17：以外部工作簿程序说明覆盖仓库设计区及规范化表，发布 39 张 Trait 卡、3 张 Forge 卡和 54 条效果；存档升级至 v9，并加入旧 v8 ID 的语义迁移。
- 2026-08-17：将 Plan47 候选 rebase 到 `origin/main@95808e2`。与 Plan48 的冲突仅发生在共享 manifest/DLL，未出现源码或数据语义冲突；在组合源码上统一 FullRebuild 后解决。
- 2026-08-17：统一重排 `ReEchoData.xlsx` 的全部生产数据 Sheet：删除左侧重复说明区，将旧说明按稳定 ID 合入右侧生产 Table 的 `Description`，再把生产 Table 左移至 A 列；属性、怪物和经济等无生产 Table 的 Sheet 明确标记为只读说明页。
- 2026-08-17：为角色、元素、反应、状态和武器类型补齐 `Description` Schema/CSV/C++ 读取链路，并同步测试夹具；保留工作表保护、可编辑数据单元格和下拉校验。
- 2026-08-17：客观门禁通过，任务进入 `Review`；保留用户 PIE 为关闭前人工验收。
- 2026-08-17：按用户确认以远端为标准，将 `origin/main@46211dc` 的 Plan42 场景/演出更新先合入，再恢复 Plan47 工作簿、数据与构筑实现；文本冲突采用远端演出接口并保留 Cards 类型化适配，所有共享预构建产物由合并源码统一 FullRebuild 重建。
- 2026-08-17：为 `Content/Data/*.csv` 固定禁用 Git 文本换行转换，保留生成器要求的“记录 CRLF、引号内说明 LF”确定性字节，避免 `core.autocrlf=true` 在重新检出后制造 XLSX/CSV 漂移。
- 2026-08-18：再次按远端优先原则合并 `origin/main@39136cd`；采用远端 Box Collision、Gameplay Blueprint、敌人表现组件树和 Gameplay Plane，重新接入 Plan47 的卡牌眩晕/减速、Echo 嘲讽目标与通用 CombatTarget 命中，统一 FullRebuild 后完整自动化恢复 98/98。

### 证据

- 外部工作簿 SHA-256：`A7C53D56890B86627CAA0068E1598B1150A220E24D5BA1D776D3E8655A08720E`；`构筑体系G` 为 39 行、39 个 ID、0 缺失，Tier 为 8/13/18。
- 与仓库旧设计区比较：37 个共同 ID、2 个新 ID（`G_2_17`、`G_3_22`）、6 个删除 ID；共同项仅 13 个名称+程序效果完全相同。
- 当前生产表仅 32 张记录、17 条效果，只有 6 张 Trait + 3 张 Forge 可获得；运行时只支持 `OnApply` 与 `StatModifier/InstantRecovery`，不足以表达新表事件行为。
- 当前 v8 快照直接保存运行时 `Cards` ID；旧 `G_1_03` 是运行时占位“应急格挡”，新 `G_1_03` 是“物理强化”，已列为必须按版本迁移的冲突。
- 仓库工作簿现含 12 个工作表、18 个 Excel Table、0 个公式及 0 个公式错误；生产表均从 A 列开始，`tblCards=A3:N45`、`tblCardEffects=A48:K102`，旧说明已并入各表 `Description`。权威同步 `--check`、项目 validator 与 39 ID/Tier/删除 ID 静态断言通过。
- `ReEcho.Cards` 4/4 聚焦自动化通过，覆盖 Tier/原子授予、Encounter 阈值与资源、商店规则、持久进度及精确暴击随机；完整 `ReEcho` 自动化 97/97 通过，包含新增 v8 语义迁移、商店持久化和非核心双配件断言。
- UE 5.8 `Development -FullRebuild` 在 Plan48 合并及 CSV 权威重发后的最终组合候选上成功，6 个 Runtime Module 与精选预构建包统一刷新；项目 validator、XLSX/CSV `--check` 和 `git diff --check` 通过。
- 工作簿保护/可编辑性、数据校验与 12 个 Sheet 渲染逐页检查通过；电子表格同步测试 12/12 通过，最终完整 `ReEcho` 自动化仍为 97/97。
- 商店程序说明未定义付费刷新价格或新的“额外卡牌组”商品内容，因此未发明第二套平衡常量：现有商店消费免费刷新，`G_3_17` 可禁用刷新，并输出/展示额外卡牌组购买门禁供现有或后续商品入口统一消费。
- 远端优先合并后的 UE 5.8 `Development -FullRebuild` 再次成功并刷新 6 个 Runtime Module；XLSX/CSV 权威重发、同步测试 12/12、项目 validator、预构建指纹和 `git diff --check` 均通过，`ReEcho.Cards` 聚焦自动化 4/4 通过。
- `origin/main@39136cd` 已补齐 `/Game/ReEcho/Gameplay/CharacterPrefabs/*`、敌人 Box/表现树和对应预构建包；合并候选完整 `ReEcho` 自动化 98/98 通过，其中 `ReEcho.Presentation.Animation2D.AssetProfiles` 与 `ReEcho.Cards` 4/4 均通过。

### 剩余风险

- 这是跨 Cards/Data/Run/Combat/Echo/Enemies/Weapons/Shop/Save 的大范围改造；后续 Plan48 敌人与关卡实现必须从已合并的 Cards/Combat/Enemy 公共契约继续开发，并在最终候选重新生成共享预构建包。
- 本 Plan 只提供 C++ 功能入口和 fallback，不修改 WBP/纹理；最终视觉绑定和手感仍由 PIE 人工验收确认。
- 39 卡的自动化可以证明确定性和数值，不替代实际战斗手感、锚点可用性、信息可读性和多卡组合体验。

### 人工验收结果/请求

- `PendingBeforeClose`：客观门禁通过后，请用户按锁定验收中的 PIE 清单验证完整组合行为与可用性。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md`：已记录 `ReEcho -> ReEchoCards -> ReEchoCombat` 依赖、事件流、状态所有权和跨模块不变量。
- `shared/CODEBASE_MAP/README.md`：已增加 `MOD-ReEchoCards` / `AREA-Cards` 路由并收紧 Run 边界。
- `MOD-ReEchoCards.md`：已创建，记录模块职责、Catalog/BuildState/RuntimeState 所有权、事件契约、依赖、扩展方式、测试和代码位置。
- `MOD-ReEcho.md`、`MOD-ReEchoCombat.md`、`MOD-ReEchoEnemies.md`、`MOD-ReEchoWeapons.md`、`MOD-ReEchoUI.md`：已分别同步主装配、通用命中钩子、敌人状态消费、有效槽位容量和商店/锚点 C++ fallback 契约。
- `MOD-ReEchoAudio.md`：已只读审阅；卡牌复用现有语义事件且未改变 Audio 公共 API，因此无需修改。
