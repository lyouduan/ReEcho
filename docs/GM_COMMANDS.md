# ReEcho GM 指令手册

本文档对应当前 `AReEchoGameMode` 中通过 `UFUNCTION(Exec)` 暴露的 GM 指令，共 20 条。

## 使用方式

1. 在 PIE 或 Development 构建中按 `~` 打开 Unreal 控制台。
2. 输入 GM 指令并按回车执行，例如 `GMStatus`。
3. 指令结果会以 `[GM]` 前缀同时显示在屏幕上并写入 Unreal 日志。

控制台打开时会暂停正在运行的游戏；仅当这次暂停由控制台触发时，关闭控制台才会恢复游戏。如果游戏原本已经暂停，关闭控制台会保持原暂停状态。GM 指令在 Shipping 构建中禁用。字符串参数不区分大小写；本文仍使用源码中的标准拼写。

## 快速索引

| 分类 | 指令 |
|---|---|
| 帮助与状态 | `GMHelp`、`GMStatus` |
| 玩家与资源 | `GMHeal`、`GMGod`、`GMAddShards`、`GMSetShards` |
| 关卡与敌人 | `GMEndEncounter`、`GMKillAll`、`GMSpawnFox`、`GMGotoEncounter`、`GMGotoBoss`、`GMBossSkill`、`GMBossDamageRange`、`GMShowEnemyHealth`、`GMShowEnemyRange` |
| 场景 | `GMWeather` |
| 元素 | `GMElement`、`GMReaction` |
| 构筑 | `GMGrantCard`、`GMEquipRune`、`GMUnequipRune` |

## 帮助与状态

### `GMHelp`

显示常用 GM 指令和元素反应组合。

> 当前内置帮助文本没有列出 `GMGrantCard`、`GMEquipRune` 和 `GMUnequipRune`，完整指令以本文档和源码为准。

### `GMStatus`

显示当前运行状态：

- 关卡序号 `Encounter`
- 玩家当前/最大生命 `HP`
- 无敌状态 `God`
- 当前攻击元素覆盖 `Element`
- 时间碎片 `TimeShards`
- 已生成回响数量 `Echoes`

```text
GMStatus
```

## 玩家与资源

### `GMHeal [Amount]`

恢复存活玩家的生命。

- 省略参数、填写 `0` 或负数：恢复至满生命。
- 填写正数：恢复对应生命值，最终不会超过最大生命。
- 不能复活已经死亡的玩家。

```text
GMHeal
GMHeal 30
```

### `GMGod [On|Off|Toggle]`

控制玩家的调试 God 模式。开启后仍显示正常结算的伤害数字和命中反馈，但玩家生命不会下降，也不会因此死亡。

- 默认参数：`Toggle`
- `On` 或 `1`：开启
- `Off` 或 `0`：关闭
- `Toggle`：切换当前状态
- 不影响正式限时无敌规则；关闭后立即恢复正常扣血。

```text
GMGod On
GMGod Off
GMGod
```

### `GMAddShards [Amount]`

增减时间碎片，默认增加 `100`。

- 可以填写负数进行扣减。
- 结果限制在 `0` 到 `MAX_int32`。
- 商店界面打开时会立即刷新碎片显示。

```text
GMAddShards
GMAddShards 500
GMAddShards -100
```

### `GMSetShards [Amount]`

直接设置时间碎片，默认设置为 `0`。负数会被限制为 `0`，商店界面打开时会立即刷新。

```text
GMSetShards 9999
GMSetShards 0
```

## 关卡与敌人

### `GMEndEncounter`

立即结束当前普通关卡，并进入正常的关卡完成与结算流程。

使用条件：

- 当前处于已初始化且正在进行的关卡。
- 玩家仍然存活。
- 当前不是 Boss 关；Boss 关应使用 `GMKillAll`。

```text
GMEndEncounter
```

### `GMKillAll`

杀死场上全部存活敌人。正常的关卡完成判断会在下一 Tick 继续执行，可用于普通关和 Boss 关。

```text
GMKillAll
```

### `GMSpawnFox <Count> [Distance]`

通过生产敌人配置生成一批 `M_FOX`，并逐只报告成功/失败数。

- 默认数量：`1`，安全上限：`16`。
- 默认距离：`350` cm。
- 输入距离限制在 `150` 到 `1000` cm。
- 生成位置沿玩家朝竞技场中心的确定性弧线分散，最终限制在敌人生成边界内。
- 无参数仍生成一只；单个 `1..16` 参数按新数量语法解释，单个大于 `16` 的参数按旧距离语法兼容并生成一只。
- 每只狐狸都复用生产 `M_FOX` Definition、EnemyHost 和 Roster，不建立测试专用怪物。
- 要求当前玩家存活。

```text
GMSpawnFox
GMSpawnFox 5 350
GMSpawnFox 600
```

### `GMGotoBoss`

结束当前进度并直接启动配置中的最终 Boss 关。

要求当前运行、关卡导演和玩家均已初始化，玩家存活且正处于活动关卡。

```text
GMGotoBoss
```

### `GMGotoEncounter <EncounterNumber>`

清理当前战场并直接从指定关卡开始，关卡号从 `1` 起算，最大值取当前配置的总关卡数。指令复用正式的场景切换、玩家配置、回响生成、敌人波次、关卡计时、录制和音乐启动流程，不经过上一关结算、抽卡、商店或转场。

要求当前运行、关卡导演和玩家均已初始化，玩家存活且正处于活动关卡；超出有效范围时不会改变当前关卡。

```text
GMGotoEncounter 1
GMGotoEncounter 3
GMGotoEncounter 8
```

### `GMBossSkill`

让当前存活的 `M_SHEEP` 在当前技能结束后，通过正式 Boss 状态机使用指定技能。该命令不直接播放特效或造成伤害，也不会取消已经开始的技能。

```text
GMBossSkill Skill01
GMBossSkill Skill02
GMBossSkill Skill02Moving
GMBossSkill Skill03
GMBossSkill Skill04
```

- `Skill01`：近战挥杖。
- `Skill02`：站定四连弹；也可写 `Skill02Stationary`。
- `Skill02Moving`：移动三向散射。
- `Skill03`：闪现下砸。
- `Skill04`：蓄力光束。
- 数字别名：`1`、`2`、`2M`、`3`、`4`。
- 必须已有存活羊 Boss；可先运行 `GMGotoBoss`。

### `GMBossDamageRange [On|Off|Toggle]`

显示或隐藏羊 Boss 技能的权威伤害判定范围。它只读取实际 Host/Combat 判定参数，不改变技能、伤害或特效。

- 红色圆和轨迹：Skill02 每颗逻辑弹的扫掠路径与单弹碰撞半径；Skill02 不产生一次性圆形 AOE。
- 绿色圆：Skill03 闪现下砸，以锁定预警中心为圆心，半径直接使用配表 `RadiusCm`。
- 青色范围：Skill01 以羊的武器挂点为圆心、沿 Boss 朝向覆盖前方 180° 半圆；Skill04 为锁定预警中心向世界 +X 延伸的光束矩形。
- 默认参数：`Toggle`
- 支持 `On`/`1`、`Off`/`0`、`Toggle`

```text
GMBossDamageRange On
GMBossSkill Skill03
GMBossDamageRange Off
```

### `GMShowEnemyHealth [On|Off|Toggle]`

显示或隐藏每个存活敌人头顶的剩余生命信息。

- 默认参数：`Toggle`
- 支持 `On`/`1`、`Off`/`0`、`Toggle`

```text
GMShowEnemyHealth On
GMShowEnemyHealth Off
GMShowEnemyHealth
```

### `GMShowEnemyRange [On|Off|Toggle]`

显示或隐藏敌人的伤害范围调试图形。

- 红色：接触或近战伤害范围。
- 橙色：远程攻击最大伤害范围。
- 默认参数：`Toggle`
- 支持 `On`/`1`、`Off`/`0`、`Toggle`

```text
GMShowEnemyRange On
GMShowEnemyRange Off
GMShowEnemyRange
```

## 场景

### `GMWeather [Clear|Rain|Fog]`

切换天气表现，默认值为 `Clear`。

| 参数 | 行为 |
|---|---|
| `Clear`、`Off` | 清除天气，使用竞技场声音景 |
| `Rain` | 显示雨天并切换为雨声音景 |
| `Fog` | 显示雾天，使用竞技场声音景 |

```text
GMWeather Rain
GMWeather Fog
GMWeather Clear
```

## 元素

### `GMElement [Element]`

强制玩家后续所有攻击使用指定元素，默认值为 `Flame`。

| 参数 | 行为 |
|---|---|
| `Flame`、`Fire` | 强制火元素 |
| `Lightning`、`Electricity` | 强制雷元素 |
| `Grass` | 强制草元素 |
| `Water` | 强制水元素 |
| `None`、`Clear` | 关闭覆盖，恢复武器原本配置的元素 |

```text
GMElement Flame
GMElement Electricity
GMElement None
```

### `GMReaction [Reaction]`

在距离玩家最近的存活敌人上直接预览指定元素反应特效。

- 默认反应：`Burn`
- 使用前场上必须至少有一个存活敌人
- 不写入或清除怪物的元素附着、燃烧、强化等战斗状态
- 不造成伤害，也不发布正式元素反应结算事件
- 特效直接绑定到目标怪物的受击表现根，并在约 2 秒后停止
- 为兼容旧用法，第二个数值参数仍可输入，但会被忽略

| Reaction | 预览表现 |
|---|---|
| `Burn` | 最近敌人的 Burn 特效 |
| `Vaporize` | 最近敌人的 Vaporize 特效 |
| `Growth` | 最近敌人的 Growth 特效 |
| `Conduct` | 最近敌人的 Conduct 特效，不生成正式连锁目标关系 |
| `EnhanceGrass` | 最近敌人的 EnhanceGrass 特效 |
| `EnhanceWater` | 最近敌人的 EnhanceWater 特效 |

```text
GMReaction Burn
GMReaction Conduct
GMReaction EnhanceWater
```

## 卡牌与武器插件

### `GMGrantCard <CardId>`

直接向当前构筑授予指定卡牌。

- `CardId` 必填，来源为 `Content/Data/cards.csv` 的 `Id` 列。
- 卡牌必须存在，并通过当前调试授予接口的冲突和锁局检查。
- 失败时会提示卡牌不存在、冲突或当前局已锁定。

```text
GMGrantCard G_1_01
GMGrantCard G_2_17
```

### `GMEquipRune <PartId>`

给玩家当前持有的武器装备指定插件。

- `PartId` 必填，来源为 `Content/Data/parts.csv` 的 `PartId` 列。
- 优先同步当前局的权威构筑，使成功的调整可参与存档/恢复。
- 随后立即应用到当前武器，无需等待下一次攻击。
- 没有活动武器时不能完成实时装备。
- 如果权威构筑同步失败，仍会尝试把插件应用到当前武器，并明确输出失败原因。

```text
GMEquipRune P_CORE_FLAME
GMEquipRune P_BOW_SPLIT_ARROWHEAD
```

### `GMUnequipRune <SlotTypeId>`

卸下指定槽位中的全部插件，并同步当前局构筑与当前武器。

当前生产数据中的有效槽位：

```text
Arrowhead
Bowstring
Core
Grip
GunAction
Muzzle
RotaryBlade
SwordBlade
```

```text
GMUnequipRune Core
GMUnequipRune Arrowhead
GMUnequipRune SwordBlade
```

## 数据与源码位置

| 内容 | 路径 |
|---|---|
| GM 指令声明和默认参数 | `Source/ReEcho/Public/ReEchoGameMode.h` |
| GM 指令实现 | `Source/ReEcho/Private/ReEchoGameMode.cpp` |
| 卡牌 ID | `Content/Data/cards.csv` |
| 插件 ID 与槽位 | `Content/Data/parts.csv` |
| 控制台按键 | `Config/DefaultInput.ini` |

## 强制维护要求

任何 GM 指令变更都必须在同一提交或同一发布候选中同步更新本手册，不得把文档更新留到后续任务。包括但不限于：

- 新增、删除或重命名 GM 指令。
- 修改参数数量、参数名称、参数类型、默认值或合法取值。
- 修改指令的实际效果、作用对象、使用条件、失败条件或数值限制。
- 修改控制台输出、调试显示、Shipping 可用性或暂停行为。
- 修改卡牌、插件等外部数据 ID 的来源或格式。

每次修改 GM 指令时必须完成以下检查：

1. 以 `AReEchoGameMode` 中实际存在的 `UFUNCTION(Exec)` 为准，确认本手册没有遗漏或失效指令。
2. 同步更新指令说明、参数表、示例、限制条件和数据来源。
3. 同步检查并更新 `GMHelp()` 的内置帮助文本。
4. 核对源码中的 GM 指令数量与本文档逐条说明的数量一致。
5. 按变更表面执行适用构建、测试、项目校验与 `git diff --check`。
