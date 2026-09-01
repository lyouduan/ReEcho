# `MOD-ReEchoCards` 详细设计

## 模块状态

- Runtime Module：`ReEchoCards`。
- 代码根：`Source/ReEchoCards/`。
- 架构标识：`MOD-ReEchoCards`；功能检索标识：`AREA-Cards`。
- 当前状态：Plan47 `Review` 候选；39 张新构筑、跨领域适配、商店 fallback 和存档 v9 已实现，等待用户 PIE 验收。

## 存在原因

卡牌定义、抽取资格、叠层状态和效果规则原先由主模块的 Run 流程直接解释，导致商店、战斗、Echo、武器和存档共同依赖隐式卡牌约定。本模块把卡牌构筑抽成无 UI、无世界 Actor、无工作簿读取的运行时边界，使生产卡牌可以由稳定 ID 和类型化行为统一验证、计算与测试。角色能力属于主模块 `AREA-Run/CharacterAbilities`，不伪装成卡牌；旧勇者 Forge 卡牌和效果已从生产目录删除。

## 职责与排除项

**负责：**

- 不可变 `FReEchoCardCatalog`、卡牌/效果定义和稳定行为白名单；
- `FReEchoCardBuildState`、每场遭遇卡牌运行状态和确定性随机序列；
- 持有状态、冲突、层级和抽取池资格；初次免费/商店投放允许 1 级卡重复并叠加，2、3 级卡进入 `OwnedCardIds` 后不可再次投放；免费初始三选一由 Run 在完整合法池上等权洗牌并无放回取样，不按已有叠层分桶或加权；免费与商店卡槽刷新由 Run 在同级资格池上额外排除所有已获得卡牌（包括 1 级）以及当前这一组三选一曾展示过的卡；
- 授予、属性变更、跨领域规则快照与类型化事件计算；
- 不依赖世界的纯规则测试。

**不负责：**

- XLSX/CSV 文件读取、屏幕路由、商店界面、玩家/敌人/Echo Actor；
- 最终伤害、生命、元素或死亡裁决；
- 武器槽位装配、敌人 AI、音频和表现资源；
- 保存文件 IO 和旧版本迁移编排。

## 权威状态

| 状态 | 唯一所有者 | 外部访问方式 |
|---|---|---|
| 卡牌定义和效果 | `FReEchoCardCatalog` | 数据适配器编译后发布的只读目录 |
| 已拥有卡牌、叠层、随机序号 | `FReEchoCardBuildState` | `FReEchoCardRuntime` 的纯命令/结果 |
| 遭遇内阈值、免伤、追踪进度、债务/武器历史/Echo套装进度与当前商店卡组页 | `FReEchoCardRuntimeState` | 遭遇生命周期命令；Run 按关次生成固定 `[Tier1, Tier2, Tier3]` 卡组状态，每组保存最多三个候选 ID、逐槽刷新用量、稳定基础价、付款已提交与最终领取标记 |
| 已解析的卡牌实际结果 | `FReEchoCardRuntimeState::ResolvedOutcomes` | Cards 在原子玩法事务中写入类型化随机、待结算或累计结果；Run 只读投影 |
| 商店、Echo、元素和槽位派生规则 | `FReEchoCardRuleSnapshot` | 主模块及领域适配器只读消费 |

主模块仍拥有整局流程和保存事务，但不得再次解释卡牌自由文本或维护第二份卡牌列表。卡牌运行时只返回候选变化；Combat、Weapons、Enemies 与世界宿主继续执行各自权威命令。

## 输入、输出与公共契约

- 输入是不可变目录、构筑状态副本、确定性种子和类型化生命周期上下文。
- 输出是新的构筑/遭遇状态、属性与货币候选变化，以及领域无关的规则快照。
- `Card.TauntEcho` 通过 `FReEchoCardRuleSnapshot::bRetireEchoOnDefeat` 显式声明殉身回响死亡后的世界退场意图；该字段不能由 `bEchoesCanAttack`、生命倍率或自由文本反推。Cards 仍只计算一次 `OnEchoKilled` 属性结果，不销毁 Actor 或清理世界引用。
- 任何永久修改 `HpMax` / `HpPoint` 的卡牌生命周期结果都通过 `EReEchoHealthAdjustment` 返回类型化意图；精确采用结果 StatBlock 时返回 `SetToStatPoint`，“血肉铸锋”等要求回满时返回 `FillToMax`。嵌套或批量授卡必须合并所有子意图，优先级为 `FillToMax > SetToStatPoint > None`，不得让后续无生命变化的结果覆盖已有意图。Cards 只计算意图，不直接写 Actor、ASC 或 UI。
- 稳定随机结果、跨关延迟结算和永久累计收益同时写入 `ResolvedOutcomes`；它不记录每击、每脉冲等瞬时日志。
- 所有命令均以稳定卡牌 ID 和 `BehaviorId` 分派；描述文本不进入规则判断。
- 授予失败不修改输入；层级赠卡、随机权衡和资源变更属于同一原子结果。

## 依赖方向

```text
ReEcho ─────────→ ReEchoCards ─────────→ ReEchoCombat

ReEchoCards ─/─→ ReEcho / ReEchoWeapons / ReEchoEnemies / ReEchoAudio / UI / Presentation
```

`ReEchoCards` 依赖 Combat 仅复用稳定战斗值类型；CSV Reader 和工作簿适配留在 `ReEcho`。其他领域通过主模块适配，不反向成为 Cards 的依赖。

## 运行时流程

```text
ReEchoData.xlsx → cards.csv + card_effects.csv
  → ReEcho 数据适配器校验并编译 FReEchoCardCatalog
  → Run 以本局固定目录生成候选并提交授予
  → Cards 原子返回 BuildState / StatBlock / TimeShards / RuleSnapshot / HealthAdjustment
  → 主模块把结果交给 Combat、Weapons、Enemies、Echo、Shop 和 UI
```

运行时只执行已注册 `BehaviorId`，`Description` 仅供展示。随机授予以本局种子和 `RandomSequence` 为唯一序列，存读档后必须得到相同结果。

敌人基础碎片数值不属于 Cards。Cards 只通过 `FReEchoCardRuleSnapshot::bDisableEnemyShardDrops` 和 `BonusShardDropEncounterIndex` 声明禁掉落/下一场逐只 1.5 倍修正；`UReEchoRunSubsystem::ResolveEnemyDeathTimeShardDrop` 是唯一规则消费端，只返回世界拾取物应携带的数额，不直接写余额。关末只清理一次性标记，不再固定发放货币。

## 代码位置与阅读路线

| 目的 | 代码 |
|---|---|
| 类型化状态与结果 | `Public/Cards/ReEchoCardTypes.h` |
| 不可变目录和白名单 | `Public/Cards/ReEchoCardCatalog.h`、`Private/Cards/ReEchoCardCatalog.cpp` |
| 候选、授予和规则 | `Public/Cards/ReEchoCardRuntime.h`、`Private/Cards/ReEchoCardRuntime.cpp` |
| CSV 编译适配 | `Source/ReEcho/Private/Data/ReEchoCsvDataRegistry.cpp` |
| 自动化 | `Source/ReEchoCards/Private/Tests/` 与主模块 Run/Save 集成测试 |

## 扩展

- 新卡牌行为先增加类型化上下文/结果和注册 ID，再由主模块接到唯一领域权威入口。
- 新抽取限制进入 Catalog/Runtime 的统一资格判断，不在 UI 复制过滤规则；免费选择与商店初次生成共同调用该入口，因此“1级可重复、2/3级持有排除”保持一致。Cards 只发布无顺序的合法池，Run 以本局种子对完整池做等权无放回抽样并保存候选页。免费与付费逐槽刷新属于更严格的 Run 事务：复用同级资格池后排除全部 `OwnedCardIds`、当前候选与本组已展示历史，并只写回一个槽位及其用量；Cards 不拥有货币、价格、展示历史或刷新按钮。
- 新跨领域效果返回声明式结果；Cards 不因此增加对 Weapons、Enemies、Audio 或 UI 的依赖。
- Plan111 的64张启用卡固定为 Tier `8/32/24`。空间线、Echo三件套、反应受影响目标、溢出生命和商店赊账均由主模块/Combat消费类型化规则；Cards只计算规则快照、确定性进度与可保存结果。

## 验证

- `ReEcho.Cards.Grant.GrantAllTierOneAddsEveryEnabledCard` 使用符合 Catalog 契约的禁用且不可抽取卡，先断言夹具初始化成功，再检查全一级卡授予对已持有、未持有与禁用卡的区别。
- 数据测试覆盖 39 张卡的精确集合、Tier 数量和全部行为注册；纯规则覆盖属性/层级赠卡原子性、阈值、伤害资源、经济、跨关余数和精确暴击次数。
- 数据校验覆盖工作簿、CSV Schema、精确 ID 集合、Tier 数量和行为白名单。
- 集成自动化覆盖 Run、Shop、Combat、Echo、Weapons、Enemies 与 Save v8→v9。
- 候选发布前执行项目校验、CSV 漂移检查、编辑器编译和 ReEcho 自动化。

## 不变量与常见错误

- 39 张构筑卡以程序说明和稳定 ID 为准；运行时不解析中文描述。
- 新构筑的权威列表是 `FReEchoCardBuildState`，旧 `BuildSnapshot.Cards` 只允许用于 v8→v9 迁移。
- 一个事件只由 Cards 计算一次，并只由对应领域权威执行一次。
- 唯一/冲突/层级授予必须原子失败或原子成功，不能留下半张卡或半次属性修改。
- Cards 不读取文件、不创建 Actor、不操作 Widget，也不决定最终伤害或死亡。
- 殉身回响的死亡奖励与退场是两个边界：Cards 计算奖励并发布显式规则，Combat 裁决死亡，主模块 Echo/GameMode 负责幂等通知与安全世界清理。

## Plan152：彩蛋卡牌

- `G_4_1`～`G_4_9` 是玩法 Tier 为 `0`、`OfferGroup=EasterEgg`、`StackPolicy=Unique` 的独立牌池；三级仅是 UI 卡底投影，不能用 Tier 3 查询或赠卡获得。
- `SelectOfferForSlot` 是免费与商店卡牌组三槽共用的逐槽选择契约：先按调用方固定的 `1%` 独立判定彩蛋，再回到原等级普通池；已拥有卡、本组展示历史和正常牌池耗尽都不能把彩蛋概率提升为必出。单槽刷新只重算该槽。
- `FReEchoCardRuntimeState` 保存彩蛋承伤阈值、眩晕脉冲、上一关/本关碎片毛收入和下一关收入倍率；`ResolvedOutcomes::RandomDetails` 保存三项独立判定、随机属性、关末倍率及实际赠卡结果。
- `G_4_2` 的三项各自独立 50%；`G_4_6` 仅从未拥有的普通 `Trait` 卡授予最多五张；`G_4_7` 只改写 Player/Echo 的物理伤害；`G_4_8` 只统计遭遇内正向碎片毛收入。
- `Card.ExpectedOutcome` 的授予事务同时永久增加角色物攻与元攻，并把 `EnemyAttackFlatBonus` 发布到规则快照；三项实际结果写入 `ResolvedOutcomes`，供构筑浮窗展示，不能从卡面描述反推。
- 所有彩蛋随机消费由 Run 的真实时间 `RunSeed` 派生的稳定上下文种子或 Cards 的持久 `RandomSequence`；UI、Actor 和自由文本不得另起随机源。
