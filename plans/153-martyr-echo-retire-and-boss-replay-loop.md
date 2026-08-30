# Plan 153 - 程序 - 殉身回响死亡退场与 Boss 回放循环

## 协调

- Planner 负责人：Codex（程序路线）。
- Executor 负责人：Unassigned。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Unassigned`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@3f82fd2d3c5dd02f83dd145be991dabd21e25621`。
- 本地实现方式（可选，仅作交接说明）：按当前程序工作模式选择；正式实现前以本 Plan 发布后的最新 `origin/main` 为基线。
- 依赖 / 阻塞：产品语义已由用户锁定，无待确认项；实现前需审计最新 main 是否又修改 Echo 死亡、卡牌规则快照或 Recording 回放游标。
- Writes:
  - `plans/153-martyr-echo-retire-and-boss-replay-loop.md`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardTypes.h`
  - `Source/ReEchoCards/Private/Cards/ReEchoCardRuntime.cpp`
  - `Source/ReEchoCards/Private/Tests/`
  - `Source/ReEcho/Public/Graybox/ReEchoEchoActor.h`
  - `Source/ReEcho/Private/Graybox/ReEchoEchoActor.cpp`
  - `Source/ReEcho/Public/Recording/ReEchoPlaybackComponent.h`
  - `Source/ReEcho/Private/Recording/ReEchoPlaybackComponent.cpp`
  - `Source/ReEcho/Public/ReEchoGameMode.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoRecordingTests.cpp`
  - `Source/ReEcho/Private/Tests/` 下新增或维护的 Echo 生命周期/Boss 回放聚焦测试
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`
- Stable Reads:
  - `Content/Data/cards.csv` 中 `G_3_04` 及 `Content/Data/card_effects.csv` 中 `Card.TauntEcho` / `OnEchoKilled` 现有定义
  - `Content/Data/encounters.csv` 中普通 Encounter 30 秒、`Encounter.8` 的 `BossOrPlayerDeath` 与无限时长语义
  - `Content/Data/boss_phases.csv` 当前 Boss 阶段 `EchoPolicy`
  - `FReEchoRecording`、`UReEchoPlaybackComponent`、`AReEchoEchoActor`、`AReEchoGameMode` 的现有录制、回放、死亡与清理契约
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：普通回响、普通 30 秒 Encounter、录制内容、回响构筑快照、暂停、存读档和已有 Boss 阶段策略继续工作；仅 `Card.TauntEcho` 对应的殉身回响死亡后退场，仅 Boss Encounter 让仍存活回响按 30 秒周期循环。
- 明确排除：不修改殉身回响的生命倍率/永久生命奖励数值，不修改卡牌文本或 XLSX/CSV，不让普通关卡循环，不重播回响出生动画，不改变 Boss 技能、血量、阶段数值或 Encounter 结束条件。

## 锁定目标

1. 当 `G_3_04 殉身回响` 的 `Card.TauntEcho` 规则生效时，该回响发生权威死亡后，在既有 `OnEchoKilled` 奖励只结算一次的前提下立即从战场退场；死亡对象不再显示、占据碰撞/索敌、参与迷雾/小地图/连线或继续回放。普通回响的死亡行为不因本 Plan 被暗改。
2. Boss Encounter 的权威时间超过首个 30 秒后，仍存活回响不再停在录制末帧，而是从该录制的第 0 秒重新回放位置和成功主动技能；此后每 30 秒继续循环，直到回响死亡、被明确 Boss `EchoPolicy` 退场或 Encounter 结束。
3. Boss 循环使用现有普通 Encounter/录制的权威 30 秒时长，不在 Echo Actor、GameMode 和 Playback 中各复制一份魔法数字。暂停期间循环时间和事件游标不推进。
4. 循环边界具备确定性：每一轮技能事件只触发一次，跨过 30 秒边界不会漏播上一轮末尾或重复触发新一轮开头；Boss 中途保存并恢复时进入正确循环轮次/局部时间，不补发已完成轮次的全部历史事件。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoCards` / `AREA-Cards`，`MOD-ReEcho` / `AREA-Recording` / `AREA-Encounter` / `AREA-Presentation` / `AREA-Tests`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md` 的 `Card.TauntEcho` 派生规则说明，以及 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 的 Echo 死亡退场和 Boss 循环回放契约；两者均已加入 `Writes`。
- 设计意图：Cards 只发布“该 Echo 死亡后应退场”的显式类型化规则，不用“不能攻击”或生命倍率反推卡牌身份；Combat 继续拥有死亡裁决；Echo Host 负责一次性卡牌死亡通知和安全退场请求；GameMode 负责移除世界级引用；Playback 独占位置时间和技能游标的循环语义。
- 权威状态与依赖：不新增持久状态或 Runtime Module。Boss/普通 Encounter 判定仍由 GameMode/Encounter 数据负责，回放轮次与局部游标属于 Playback 瞬时状态，卡牌规则仍由 `FReEchoCardRuleSnapshot` 提供。依赖方向保持 `ReEcho -> ReEchoCards`，Cards 不读取 Actor、Encounter 或 Recording。
- 决策记录：
  - 为 `Card.TauntEcho` 增加显式退场规则位，而不复用 `bEchoesCanAttack == false`，避免未来其他禁攻卡误触发死亡消失。
  - 死亡奖励与世界退场拆开：先用现有权威死亡路径保证奖励至多一次，再采用安全延迟销毁/统一清理点移除 Actor 和 GameMode 引用，禁止在死亡广播栈中留下悬空引用。
  - Boss 回放按 Encounter 权威时钟取 30 秒循环局部时间；普通 Encounter 继续把末帧保持作为兼容行为。
  - 循环只重置录制位置和成功主动技能游标，不重建 Echo、不重播出生特效、不刷新生命、不重新应用构筑，也不把自动普通攻击写回录制。
  - 当前 `boss_phases.csv` 的 `EchoPolicy=None` 与循环兼容；若未来显式触发 `RetireEncounterEchoes`，退场策略优先，已退场 Echo 不得被循环机制复活。
- 相关文档同步范围：关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 和 `shared/CODEBASE_MAP/README.md`；预计模块拓扑和路由不变，无事实变化时在执行记录中说明“已审阅、无需修改”。更新 `MOD-ReEcho.md` 与 `MOD-ReEchoCards.md` 的上述运行时契约。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEcho.md`：记录实际死亡清理所有者、Boss 循环边界和存读档行为；
  - `MOD-ReEchoCards.md`：记录显式殉身退场规则；
  - `ARCHITECTURE.md`、`README.md`：分别记录已更新或已审阅、无需修改及理由。

## 锁定验收

- [ ] 未持有 `G_3_04` 时，现有普通回响行为保持不变；持有后回响被击杀只触发一次既有永久生命奖励，并在同一死亡流程结束后不可见、不可碰撞、不可索敌且不再回放。
- [ ] 殉身回响的重复伤害/重复死亡通知不能重复奖励或重复退场；GameMode 的 Echo 容器、迷雾、连线、小地图及遭遇清理中不存在失效引用。
- [ ] 普通 Encounter 在 30 秒内保持现有回放；Boss Encounter 在 30 秒处从头开始第二轮，在 60 秒处开始第三轮，位置和技能事件均按轮次重复且每轮只触发一次。
- [ ] Boss 在 30 秒附近低帧率跨界、暂停/恢复、超过 30 秒后保存并继续、多个回响并行及回响中途死亡均有自动化或确定性测试覆盖。
- [ ] 现有 `RetireEncounterEchoes` 若被数据启用仍优先清理，循环逻辑不会复活退场回响。
- [ ] `ReEcho.Cards`、Recording/Echo/Save/Boss 聚焦自动化、Development 构建、项目静态校验和最终发布构建通过。
- [ ] 用户在 PIE 验证殉身回响死亡立即消失，以及 Boss 战超过 30 秒后回响重新移动并再次释放录制技能。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@3f82fd2d3c5dd02f83dd145be991dabd21e25621`；远端最大计划号为 `152`，本任务占用下一个编号 `153`。
- 引擎/构建可用性：实现前运行 `python scripts/setup_lfs.py --check`；运行 Unreal 自动化或构建前确认 Editor 已关闭并按仓库规则取得同克隆 Unreal 锁。
- 现有聚焦测试结果：Plan 编写阶段仅完成源码、数据和架构文档只读审计，未运行实现基线自动化或构建；执行者在首个 C++ 改动前记录 `ReEcho.Cards`、`ReEcho.Recording` 及 Echo/Boss 聚焦基线。
- 共享契约 / 难合并资源风险：`FReEchoCardRuleSnapshot` 是 Cards 到主模块的公共契约；`ReEchoPlaybackComponent.*`、`ReEchoEchoActor.*`、`ReEchoGameMode.*` 是高频运行时热点。实现前如 main 前进，必须审计卡牌、Echo、Recording、Boss/Encounter 与存档恢复的逻辑耦合，不能只依赖 Git 无文本冲突。
- 基线损坏时的停止条件：最新 main 已改变 `Card.TauntEcho` 产品语义、Boss Encounter 不再以 30 秒录制为回放周期、回响死亡奖励所有者被迁移、或出现需要改变普通回响死亡语义/卡牌数值/存档 Schema 的新要求时，停止越界部分并报告。

## 实现提纲

1. 为 `Card.TauntEcho` 编译显式“死亡后退场”规则，并补 Cards 纯规则测试，保持现有生命倍率、禁攻和 `OnEchoKilled` 奖励值不动。
2. 在 Echo 权威死亡通知中建立一次性门：提交既有卡牌奖励后标记退场，通过安全世界清理点停用碰撞/表现/回放并从 GameMode 相关引用中移除。
3. 为 Playback 增加显式的单次/循环策略或等价窄接口：根据 Boss Encounter 注入的循环条件计算轮次与局部时间，正确重置技能游标并处理跨界。
4. 接入新建和存档恢复两条 Echo 装配路径；普通 Encounter 使用单次策略，Boss 使用 30 秒循环策略，显式 Boss 退场策略仍有最高优先级。
5. 补齐卡牌、死亡生命周期、Recording 循环、Boss 时间边界、暂停/恢复和多 Echo 测试，维护模块文档和执行记录，再完成构建与用户 PIE 验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| LFS | `python scripts/setup_lfs.py --check` | 当前检出对象完整；无指针或缺失对象 |
| Cards | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Cards` | `Card.TauntEcho` 显式退场规则及既有数值/奖励语义通过 |
| Recording | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Recording` | 单次保持末帧、Boss 30/60 秒循环、技能游标边界通过 |
| Echo/Boss/Save | 运行新增聚焦过滤及相关 `ReEcho.Run` / Save 测试 | 死亡只结算一次、引用清理、多 Echo、Boss 恢复和退场优先级通过 |
| C++ | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目/源码/Plan/架构文档不变量通过，无空白错误 |
| 发布构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终组合源码与精选预构建包一致 |
| 人工 | PIE：获得殉身回响后让其死亡；Boss 战分别观察 30 秒和 60 秒边界 | 回响死亡立即消失且奖励一次；Boss 回响重新走位并再次释放录制技能 |

## 执行记录

### 变化

### 证据

### 剩余风险

### 人工验收结果/请求

- `PendingBeforeClose`：等待实现完成后由用户执行锁定 PIE 验收。

### 架构文档审阅结果

- 待执行阶段填写。
