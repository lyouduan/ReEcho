# Plan 153 - 程序 - 羊 Boss 十五秒速杀彩蛋三阶段

## 协调

- Planner 负责人：Codex（程序路线）。
- Executor 负责人：Codex（程序路线，Plan 发布后在独立实现 worktree 执行）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@f9a2aece376825f2c4d7be3ab8bd510079c846d4`。
- 本地实现方式（可选，仅作交接说明）：Plan worktree `C:\tmp\ReEcho-plan153-boss-phase3-easter-egg-plan`；Plan 发布后从准确最新 `origin/main` 创建专属实现 worktree。
- 依赖 / 阻塞：用户已确认 15 秒内击杀触发三阶段、三阶段只使用 `Skill03Moving` 且每轮随机连续下砸 1～3 次；已提供 8 帧黑羊行走和 8 帧右手拍地攻击源图。默认锁定三阶段回满 500 HP；当前没有三阶段变身动画，直接切入 Phase3 Walk。
- Writes:
  - `plans/153-sheep-boss-fast-kill-phase3.md`
  - `Design/Data/ReEchoEnemyData.xlsx`
  - `Content/Data/enemy_abilities.csv`
  - `Content/Data/boss_phases.csv`
  - `Content/Data/csv_schema.csv` 及同步工具实际要求的同事务数据 manifest
  - `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyTypes.h`
  - `Source/ReEchoEnemies/Public/Enemies/ReEchoEnemyLogicComponent.h`
  - `Source/ReEchoEnemies/Private/Enemies/ReEchoEnemyLogicComponent.cpp`
  - `Source/ReEchoEnemies/Private/Tests/ReEchoEnemyLogicTests.cpp`
  - `Source/ReEcho/Public/Graybox/ReEchoEnemyActor.h`
  - `Source/ReEcho/Private/Graybox/ReEchoEnemyActor.cpp`
  - `Source/ReEcho/Public/Presentation/Enemy/ReEchoEnemyPresentationComponent.h`
  - `Source/ReEcho/Private/Presentation/Enemy/ReEchoEnemyPresentationComponent.cpp`
  - `Source/ReEcho/Public/ReEchoGameMode.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Data/` 中 BossPhase/EnemyAbility 的 CSV 读取、编译和测试
  - `Source/ReEcho/Private/Tests/` 中 Boss Host、GameMode、动画和数据聚焦测试
  - `Content/ReEcho/Art/Animation2D/Enemies/Goat/Phase3/Walk/`
  - `Content/ReEcho/Art/Animation2D/Enemies/Goat/Phase3/GroundSlam/`
  - `Content/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_GoatPriest.uasset`
  - `scripts/ue/` 下本 Plan 的动画导入/审计脚本
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
- Stable Reads:
  - `F:\黑羊Boss_行走序列帧_8帧_完整\BlackSheepBoss_Walk_01.png` 至 `08.png`
  - `F:\黑羊Boss_技能_右手拍地_8帧\BlackSheepBoss_GroundSlam_01.png` 至 `08.png`
  - 现有 `M_SHEEP_BlinkSlam` 的锁点、预警、0.5 秒下落、地裂和圆形伤害实现
  - 现有羊 Boss Phase2 致命伤拦截、Boss encounter 计时、死亡和胜利结算链
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：普通怪、一二阶段羊 Boss、超过 15 秒后的正常 Boss 死亡、现有 Skill01～04、伤害/预警空间权威、存档恢复和 Boss 胜利门保持原语义；缺少新动画时必须安全降级但不得阻塞玩法。
- 明确排除：不修改 Skill01～04 数值与表现、不改变普通关卡时长或 Boss 出生规则、不制作新的 Niagara、不把粒子位置作为伤害权威、不制作未交付的三阶段变身/死亡动画、不改变其它怪物阶段机制。

## 锁定目标

1. 羊 Boss 一阶段致命伤仍按现有规则进入二阶段；若第二阶段致命伤发生在 Boss 自身 `BossEncounterElapsedSeconds <= 15.0` 时，拦截本次死亡并进入隐藏 Phase3，回满至 500 HP。超过边界则正常死亡；Phase3 再次致命伤也正常死亡。
2. Phase3 只允许新能力 `M_SHEEP_BlinkSlamMoving`（GM 名称 `Skill03Moving`）。每轮开始时以可保存的 Boss 攻击序号/稳定状态确定性选取 1～3 次连击，禁止依赖不可恢复的全局随机流。
3. 每次连击重新锁定当时存活目标的位置，复用 Skill03 的同源预警中心、落点、0.5 秒从上向下表现、圆形伤害与地裂；只有该次下落完成后才独立结算一次伤害。每击拥有可区分的攻击身份，不能被单次命中门吞并。
4. Phase3 Walk 使用交付的黑羊 8 帧行走序列循环播放；每次下砸使用交付的右手拍地 8 帧非循环动画，目标时长 0.5 秒（16 FPS），每一连击从首帧重新播放。当前没有变身动画，Phase3 生效时直接进入新 Walk。
5. Boss 未真正进入 Dead 前，Roster 和 GameMode 不得触发 Boss 胜利；三阶段转换、技能连击剩余次数/时序、阶段计时和当前能力必须支持现有保存/恢复契约。
6. 增加可重复测试入口：`GMBossPhase Phase3`、`GMBossSkill Skill03Moving [1|2|3]` 和用于验证 15 秒边界的窄调试入口；保留 `GMBossDamageRange` 对每次下砸同源圆形判定的显示。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoEnemies`、`MOD-ReEcho`、`MOD-ReEchoPresentation`、VFX 文档入口、`AREA-Enemies`、`AREA-Encounter`、`AREA-Presentation`。
- 对应模块文档：维护 `MOD-ReEchoEnemies.md` 的多阶段/连击权威，`MOD-ReEcho.md` 的 Host/胜利门和数据编译接线，`MOD-ReEchoPresentation.md` 的 Phase3 AnimationSet，`MOD-ReEchoVFX.md` 的 Skill03Moving 复用边界；均已加入 `Writes`。
- 设计意图：把“是否进入隐藏阶段、连击次数和技能节拍”留在资源无关 EnemyLogic，把致命伤拦截/Actor 位移与伤害提交留在 Host/Combat 边界，把帧动画留在 Presentation；GameMode 只消费真实死亡，不复制 Boss 阶段状态。
- 权威状态与依赖：XLSX 是 Phase3 与能力数值真源；EnemyLogic 拥有当前阶段、Boss encounter 计时、确定性连击状态和能力阶段；Combat 仍独占生命与最终死亡；Host 只执行世界移动/伤害；Presentation 只读事件。依赖方向不反转。
- 决策记录：采用独立 `Boss.BlinkSlamCombo` 而不是在 Actor 按 Phase3 硬编码重复调用 Skill03；采用数据化阶段适用范围和连击上下限；采用 `<=15.0` 包含边界；采用 Boss 逻辑时间而非 UI/平台真实时间；每击重新锁点；无变身资源时直接切换，不复用无关动画。
- 相关文档同步范围：上述四份模块文档必须逐项审阅；`ARCHITECTURE.md` 仅在依赖或全局拓扑实际变化时更新；`README.md` 仅在新增稳定架构路由标识时更新。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEchoEnemies.md`：待更新三阶段致命伤、Phase3 技能池和确定性连击状态。
  - `MOD-ReEcho.md`：待更新 Host 致命伤/胜利门及 GM 接线。
  - `MOD-ReEchoPresentation.md`：待更新 Phase3 AnimationSet 与动作语义。
  - `MOD-ReEchoVFX.md`：待更新 Skill03Moving 对现有预警/下砸/地裂的复用边界。
  - `ARCHITECTURE.md`、`README.md`：关闭前审阅并记录是否需要修改。

## 锁定验收

- [ ] `14.9s` 和恰好 `15.0s` 的 Phase2 致命伤进入 Phase3；`15.1s` 正常死亡。
- [ ] Phase1 致命伤仍只进入 Phase2；Phase3 致命伤正常死亡并触发一次 Boss 胜利。
- [ ] Phase3 技能池只包含 `Skill03Moving`，自然运行与 GM 强制的 1/2/3 连击均能完成。
- [ ] 每击重新锁点，预警、Boss 落点、地裂和伤害圆同心；下落完成前无伤害，每击最多伤害目标一次。
- [ ] 连击随机结果与中途 Snapshot/Restore 一致，不因帧切分改变。
- [ ] 两组 8 帧动画均作为独立 Phase3 资产加载；Walk 循环，GroundSlam 每击 0.5 秒非循环，缺失时玩法安全降级。
- [ ] 功能结果有自动化和 PIE 可观察证据；人工验收三阶段触发可辨识、动画节奏、连续追击压迫感与落点一致性。
- [ ] FullRebuild、聚焦自动化、XLSX/CSV 同步、`validate_project.py`、Prebuilt、LFS 和 `git diff --check` 通过。
- [ ] 未提交精选预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@f9a2aece376825f2c4d7be3ab8bd510079c846d4`；实现前再次 fetch 并以 Plan 发布后的最新 main 创建实现 worktree。
- 引擎/构建可用性：UE 5.8 Development Editor 在前序候选完成 FullRebuild；新资产导入/保存、最终 FullRebuild 和 PIE 前必须解析同克隆 Unreal 锁并要求相关 Editor 保存关闭。
- 现有聚焦测试结果：现有 Phase2、Boss BlinkSlam、Boss Victory 与动画 Profile 测试作为回归基线；实现开始时记录准确过滤器结果。
- 共享契约 / 难合并资源风险：`ReEchoEnemyData.xlsx`、生成 CSV、`DA_Enemy_GoatPriest.uasset` 和预编译 DLL 为二进制/生成热点；只做语义行合并，禁止整表/整资产覆盖远端新变化。
- 基线损坏时的停止条件：现有 Phase2/BlinkSlam/Victory 聚焦测试在未修改前失败、源图帧尺寸或透明边界不一致、远端出现同一 XLSX 行/Goat DA/阶段状态机逻辑变化且需要产品取舍时停止并报告。

## 实现提纲

1. 扩展 BossPhase/EnemyAbility 的数据契约、可见 XLSX 行、CSV 生成与校验，加入 Phase3 条件、能力阶段范围及 Combo 参数。
2. 泛化致命伤阶段拦截：EnemyLogic 判定 Phase2→Phase3 的 15 秒窗口并保存转换状态；Host 仅将 Combat 致命伤转换为类型化阶段 Intent，真正 Phase3 死亡不拦截。
3. 为 Phase3 建立仅含 `M_SHEEP_BlinkSlamMoving` 的能力选择；实现确定性 1～3 连击、逐击重新锁点、独立攻击身份和保存恢复。
4. Host 复用 BlinkSlam 的落点求解、0.5 秒待结算和同源伤害几何，将单一 Pending 状态扩展为逐击推进，不改变普通 Skill03。
5. 导入两组 PNG，创建 Sprite/Flipbook，给 Goat Profile 增加 `Phase3` AnimationSet；Presentation 根据 Phase3 技能事件逐击播放 GroundSlam。
6. 增加 Phase3、Skill03Moving 连击数和边界计时 GM 指令，并让现有伤害范围 Debug 显示每一击。
7. 补齐纯逻辑、Host、GameMode、数据与动画资产测试，维护模块文档；完成 PIE 人工验收后才能关闭。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 数据 | `python scripts\data\sync_xlsx_to_csv.py --check` | 可见 XLSX 与生成 CSV 字节一致，Phase3/Skill03Moving 行通过 schema/引用检查 |
| 资产 | Plan153 导入/审计脚本 | 16 张源图、Sprite、两个 Flipbook、Goat `Phase3` AnimationSet 和帧率/循环属性正确 |
| C++ | `.clang-format` + `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新准确预编译包 |
| 逻辑自动化 | `ReEcho.Enemies.Logic` 下 Phase3/Combo 聚焦测试 | 15 秒边界、能力池、1～3 连击和 Snapshot 确定性通过 |
| Host/GameMode | Boss Phase3、BlinkSlamMoving、Victory 聚焦测试 | 死亡拦截、逐击伤害、真实死亡后唯一胜利通过 |
| 表现 | Boss Animation/VFX 聚焦测试 | Walk/GroundSlam 绑定、0.5 秒落地事件和同源几何通过 |
| 静态 | `python scripts\validate_project.py`、Prebuilt、LFS、`git diff --check` | 项目、生成物和提交边界通过 |
| 人工 | `GMGotoBoss` + Phase3/Skill03Moving/Timer GM 指令 | 用户确认触发、动画、1～3 连击节奏和落点表现 |

## 执行记录

### 变化

- 尚未开始实现。

### 证据

- 规划阶段只读确认：最新远端最大编号为 152；现有生产只含 Phase1/Phase2；两组交付目录各包含连续命名的 8 张 PNG；现有 Skill03 在 Host 延迟 0.5 秒后结算。

### 剩余风险

- Phase3 视觉没有独立变身动画，本 Plan 明确直接切入 Walk；若新增变身资源需更新 Plan 资产范围。
- 500 HP 与连击间隔仍需 PIE 手感验收；数值通过 XLSX 配置，不以 C++ 常量固化。

### 人工验收结果/请求

- `PendingBeforeClose`：等待实现后的 PIE 验收。

### 架构文档审阅结果

- 待实现完成后逐项填写。
