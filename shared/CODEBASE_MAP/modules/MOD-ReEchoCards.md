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
- 持有状态、冲突、层级和抽取池资格；1级卡可在后续免费/商店投放中重复并叠加，2、3级卡进入 `OwnedCardIds` 后不可再次投放；
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
| 遭遇内阈值、免伤、追踪进度与当前商店卡牌页键/ID | `FReEchoCardRuntimeState` | 遭遇生命周期命令；Run 按关次与刷新序号生成/消费稳定页面 |
| 商店、Echo、元素和槽位派生规则 | `FReEchoCardRuleSnapshot` | 主模块及领域适配器只读消费 |

主模块仍拥有整局流程和保存事务，但不得再次解释卡牌自由文本或维护第二份卡牌列表。卡牌运行时只返回候选变化；Combat、Weapons、Enemies 与世界宿主继续执行各自权威命令。

## 输入、输出与公共契约

- 输入是不可变目录、构筑状态副本、确定性种子和类型化生命周期上下文。
- 输出是新的构筑/遭遇状态、属性与货币候选变化，以及领域无关的规则快照。
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
  → Cards 原子返回 BuildState / StatBlock / TimeShards / RuleSnapshot
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
- 新抽取限制进入 Catalog/Runtime 的统一资格判断，不在 UI 或商店复制过滤规则；免费选择与商店共同调用该入口，因此“1级可重复、2/3级持有排除”必须在此保持一致。
- 新跨领域效果返回声明式结果；Cards 不因此增加对 Weapons、Enemies、Audio 或 UI 的依赖。

## 验证

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
