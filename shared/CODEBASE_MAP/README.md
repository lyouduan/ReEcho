# ReEcho 架构与代码库地图

本目录是当前 `main` 的架构知识库。它把设计意图与真实代码位置放在同一套稳定标识下，但按阅读层级拆开，避免一份总文档同时承担全局拓扑、模块细节和任务路由。

## 最小阅读方式

1. 只想知道模块关系：读 [`ARCHITECTURE.md`](ARCHITECTURE.md)。
2. 要修改某个功能：从下表找到一个 `MOD-*` 或 `AREA-*` 标识，只读对应模块文档的相关章节。
3. 要新增、拆分或合并 Runtime Module：同时读全局架构、受影响模块文档和 [`MODULE_TEMPLATE.md`](MODULE_TEMPLATE.md)。
4. 具体行为仍以源码、测试、运行时数据和已关闭 Plan 为事实来源；本文档解释职责和阅读路线，不替代实现。

## Runtime Module 索引

| 架构标识 | 当前状态 | 设计与代码文档 | 一句话职责 |
|---|---|---|---|
| `MOD-ReEcho` | 当前 `main` Runtime Module | [`modules/MOD-ReEcho.md`](modules/MOD-ReEcho.md) | UE 世界与玩法装配根，组合主流程、局内/局外领域、表现与 UI |
| `MOD-ReEchoAudio` | 当前 `main` Runtime Module | [`modules/MOD-ReEchoAudio.md`](modules/MOD-ReEchoAudio.md) | 接收语义音频请求，独立管理目录、策略、总线状态与播放后端 |
| `MOD-ReEchoCombat` | 当前 `main` Runtime Module | [`modules/MOD-ReEchoCombat.md`](modules/MOD-ReEchoCombat.md) | 攻击控制、战斗状态和最终结算的逻辑权威 |
| `MOD-ReEchoWeapons` | 当前 `main` Runtime Module | [`modules/MOD-ReEchoWeapons.md`](modules/MOD-ReEchoWeapons.md) | 武器定义、唯一攻击节拍、步骤和攻击载体逻辑 |
| `MOD-ReEchoEnemies` | Plan43 `InProgress` 候选 Runtime Module | [`modules/MOD-ReEchoEnemies.md`](modules/MOD-ReEchoEnemies.md) | 怪物 AI、攻击节奏、引信、受击位移与行为快照的逻辑权威 |

`ReEcho.uproject` 中每个 Runtime Module 都必须在此表拥有唯一 `MOD-*` 标识和独立文档。尚未合入 `main` 的候选模块只能记录在对应 Plan/本地分支，不能提前加入本索引。

## 模块与内部领域索引

`AREA-*` 是稳定的功能检索标识，不要求永远属于主模块。Plan41 把战斗和武器领域迁入独立模块，其余领域仍由 [`modules/MOD-ReEcho.md`](modules/MOD-ReEcho.md) 说明。

| 架构标识 | 领域 | 代码主目录 | 首读章节 |
|---|---|---|---|
| `AREA-Core` | 公共类型与兼容契约 | `Source/ReEcho/{Public,Private}/Core/` | [Core](modules/MOD-ReEcho.md#area-corecore公共类型与兼容契约) |
| `AREA-Data` | XLSX/CSV 运行时适配 | `Source/ReEcho/{Public,Private}/Data/` | [Data](modules/MOD-ReEcho.md#area-datadata生产数据适配) |
| `AREA-AbilityCombat` | GAS、攻击控制、战斗与元素结算 | `Source/ReEchoCombat/`；主模块 `Combat/` 仅适配 | [Combat](modules/MOD-ReEchoCombat.md#代码位置与阅读路线) |
| `AREA-Weapons` | 武器定义、步骤与逻辑载体 | `Source/ReEchoWeapons/`；主模块 `Weapons/` 负责数据/表现适配 | [Weapons](modules/MOD-ReEchoWeapons.md#代码位置与阅读路线) |
| `AREA-Enemies` | 怪物行为逻辑、Roster 与主模块接线 | `Source/ReEchoEnemies/`；主模块 EnemyHost 位于 `Graybox/`、只读表现位于 `Presentation/Enemy/` | [Enemies](modules/MOD-ReEchoEnemies.md#代码位置与阅读路线) |
| `AREA-Encounter` | 遭遇固定步时钟 | `Source/ReEcho/{Public,Private}/Encounter/` | [Encounter](modules/MOD-ReEcho.md#area-encounterencounter遭遇时钟) |
| `AREA-Run` | 本局构筑、阶段与存档 | `Source/ReEcho/{Public,Private}/Run/` | [Run](modules/MOD-ReEcho.md#area-runrun本局状态与存档) |
| `AREA-Recording` | 玩家历史录制与 Echo 回放 | `Source/ReEcho/{Public,Private}/Recording/` | [Recording](modules/MOD-ReEcho.md#area-recordingrecording录制与回放) |
| `AREA-Player` | 输入、移动、相机与玩家装配 | `Source/ReEcho/{Public,Private}/Player/` | [Player](modules/MOD-ReEcho.md#area-playerplayer玩家宿主) |
| `AREA-Presentation` | 世界 Actor 与可见反馈 | `Source/ReEcho/{Public,Private}/{Graybox,Presentation}/` | [Presentation](modules/MOD-ReEcho.md#area-presentationgraybox--presentation世界表现) |
| `AREA-UI` | 屏幕框架、Widget 与只读展示 | `Source/ReEcho/{Public,Private}/UI/` | [UI](modules/MOD-ReEcho.md#area-uiui屏幕与交互) |
| `AREA-Tests` | 自动化与跨领域契约 | `Source/ReEcho/Private/Tests/`、各模块 `Private/Tests/` | [Tests](modules/MOD-ReEcho.md#area-teststests验证边界) |

## 文档职责

| 文档 | 回答的问题 | 不应包含 |
|---|---|---|
| `ARCHITECTURE.md` | 项目有哪些模块、为什么这样依赖、状态如何跨模块流动 | 单个类的逐行说明、候选分支目标冒充当前架构 |
| `modules/MOD-*.md` | 某模块为何存在、拥有什么、如何扩展、代码和测试在哪里 | 其他模块的第二份权威说明、任务流水账 |
| `MODULE_TEMPLATE.md` | 新模块文档必须具备哪些章节 | 具体模块结论 |
| `plans/<id>-*.md` | 一次任务如何选择、实现和验证 | 永久重复当前全局架构 |
| `LESSONS.md` | 经验证且可复用的陷阱与经验 | 模块目录索引和未验证推测 |

## 同步规则

- 每个程序 Plan 在实施前必须列出受影响的 `MOD-*` / `AREA-*`、设计意图和本目录的相关文档同步范围；对应 `modules/MOD-*.md` 必须进入同一候选的 Writes。
- 新增/删除/重命名 Runtime Module：同步更新 `.uproject`、全局架构、本索引、独立模块文档和校验器。
- 模块职责、权威状态、公共契约或依赖方向变化：更新全局架构与相关模块文档。
- 仅移动类或目录：更新对应模块文档中的代码位置；设计意图不变时不改全局拓扑。
- 每个程序 Plan 关闭前必须逐项审阅所有直接修改模块和受契约影响模块的详细文档，以及相关的全局架构和索引；无变化时在 Plan 中按文档记录原因。
- 模块文档必须描述当前 `main`。候选设计留在 Plan 分支，随实现一起验收并在合入时晋升。
