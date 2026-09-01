# chenglexi 的 ReEcho 策划经验

## 2026-08-27 16:35 - 策划分支迁移与废弃（旧两段式 → 三段式）

- 结果：成功
- 任务与分支：分支治理；`designer/chenglexi/monster-rebalance-and-audio`（94ef44b5）、`designer/chenglexi/element-reaction-popup-size`（bb6dd2df）
- 目标：按新规则（`shared/DESIGNER_RULES.md`，2026-08-27 修订）把旧两段式 `designer/<任务>` 分支迁为三段式 `designer/<策划身份>/<任务>`，并废弃两条过时分支
- 生效映射：无数据/配置改动，纯远端引用迁移
- 修改：
  - 迁移 `designer/monster-rebalance-and-audio`(94ef44b5) → `designer/chenglexi/monster-rebalance-and-audio`，提交号一致后删旧引用
  - 迁移 `designer/element-reaction-popup-size`(bb6dd2df) → `designer/chenglexi/element-reaction-popup-size`，提交号一致后删旧引用
  - 废弃 `designer/character-base-stats`(c464b2a5) 与 `monster-20260825`(38efb135)：策划本人确认废弃；两者均有未合入 main 的独立提交（character-base-stats 领先 main 2、monster-20260825 领先 main 3），删除前已 fetch 全部对象到本地可恢复
- 刷新方式：无（纯 git 远端操作）
- 验证入口：`git ls-remote --heads origin` 确认新分支 SHA 与旧分支一致、旧引用已删除
- 策划确认：chenglexi 明确「monster + element-reaction 两条迁移，另外两条废弃」
- 限制与踩坑：
  - push 新分支必须 `-c credential.helper=` + URL 内嵌 token，否则 GCM 在无头环境卡死导致 SIGTERM
  - 本地创建/更新分支 ref 需 Python 直写（git branch/commit 更新 ref 会被 safe-delete shim 拦截丢 ref）
- 问题交接：无

## 2026-08-27 18:17 - 卡牌刷新音效入点修正 + 兔子每波投放减 7

- 结果：成功（数据层，sync + validate 已 PASS；策划已确认提交并交由项目秘书集成）
- 事项与分支：`merge/chenglexi/card-audio-and-rabbit-spawn`（从 9bf2cb66 新建）
- 目标：① 3 选 1 卡牌界面「刷新单个卡牌」音效有延迟，修正音频入点；② 第二关起（战斗 3–8）兔子每波投放各减 7 只
- 生效映射：
  - 在 `Design/Data/ReEchoAudioEvents.xlsx` 的 `AudioEvents` Sheet 改 `EventId=UI.CardReveal` 的 `StartTimeSeconds`（0 → 1.28），使「刷新单个卡牌」音效跳过源文件 1.28s 前置静音、即时起播（与 `UI.Purchase`/`UI.CardSelect` 对齐）
  - 在 `Design/Data/ReEchoEncounterData.xlsx` 的 `EncounterWaves` Sheet 改 `Encounter.3~8` 各行 `RangedCount`（每波 -7），使战斗 3–8 每波兔子少 7 只
- 修改：
  - `UI.CardReveal` StartTimeSeconds：`'0'` → `'1.28'`（源 wav 实测前置静音 1.28027s，ffmpeg silencedetect）
  - RangedCount 每波 -7：E3 16/12/18→9/5/11、E4 16/16/22→9/9/15、E5 18/16/22→11/9/15、E6 16/18/18→9/11/11、E7 22/18/26→15/11/19、E8 16/18/22→9/11/15（共 18 格，无负值）
- 刷新方式：`python scripts/data/sync_xlsx_to_csv.py`（重生成 `Content/Data/audio_events.csv`、`encounter_waves.csv`）→ `python scripts/validate_project.py`（PASS）
- 验证入口：`audio_events.csv` 第 16 列 StartTimeSeconds、`encounter_waves.csv` 第 6 列 RangedCount；新局进战斗 3–8 与卡牌刷新点
- 策划确认：chenglexi 明确「不是总投放，是每一波的投放均减 7」；2026-08-27 已确认提交 `59a5cf03` 并交由项目秘书集成
- 限制与踩坑：
  - 音效入点纯数据层（`StartTimeSeconds` 由 `ReEchoAudioBackend` 运行时消费），无需动 C++；`UI.CardReveal` 触发点 = `ReEchoGameMode.cpp:4715` `PostUiEvent(UiCardReveal)`
  - `git checkout -b` 新建分支后 `refs/heads/*` 再次被 safe-delete shim 拦截丢 ref（HEAD 已指新分支但 ref 文件缺失、`git rev-parse HEAD` 报 unknown revision），需 Python `os.makedirs`+`open().write(sha)` 直写 ref 修复
- 报告交接：无

## 2026-09-02 00:3x - 三档难度数据包：派对与噩梦数值首版落地

- 结果：成功（数据层，`sync_difficulty_xlsx_to_csv.py` + `validate_project.py` 已 PASS；策划已实机验收战斗 1-5 并确认推送）
- 事项与分支：`merge/chenglexi/update-level`（从 `f306fe3d` 新建）
- 目标：Plan162 三档难度数据包上线后，为 `Party（派对）` 与 `Nightmare（噩梦）` 填入首版数值；`Standard（常规）` 与 origin/main 逐行一致不动
- 生效映射（新链路，与旧表无关）：
  - 在 `Design/Data/ReEchoDifficultyParty.xlsx` / `ReEchoDifficultyNightmare.xlsx` 改数据行 → 运行 `python scripts/data/sync_difficulty_xlsx_to_csv.py` → 生成 `Content/Data/Difficulty/<档>/*.csv` → 该档新局生效
  - **注意不是** `sync_xlsx_to_csv.py`；`ReEchoEnemyData.xlsx` / `ReEchoEncounterData.xlsx` 中同领域内容已降级为启动兼容数据，新局以 Difficulty 工作簿为准，不要两边重复调
  - 改 `EnemyCombatStats.MaxHealth` 使某关某怪血量生效（会覆盖 `Enemies.MaxHealth` 基础值）
  - 改 `EncounterWaves.MeleeCount/RangedCount/EliteCount` 使该波投放数量生效
  - 改 `Encounters.ActiveUnitLimit` 使场上同屏怪物上限生效；`RangedBurstLimit` 控制 1.2s 窗口内同时开火的远程数
  - 改 `Enemies.M_SHEEP.MaxHealth` 使 Boss **一阶段**血量生效；改 `BossPhases.M_SHEEP_Phase2.PhaseMaxHealth` 使**二阶段**生效
  - 改 `EnemyShardDrops.<Melee|Ranged|Elite><Min|Max>` 使该关单只怪碎片掉落生效
- 修改（派对档，全 8 关）：
  - HP = floor(常规×0.6)，兔子保底 2：史 3,3,6,7,9,14,18,24 / 兔 2,2,2,2,2,3,6,7 / 狐 3,12,12,18,24,48,84,120
  - 投放 近战远程 ×0.7、精英 ×0.5（ceil）；同屏上限 ×0.6（16,17,29,34,38,39,46,30）；齐射上限统一 3
  - Boss：`Enemies.M_SHEEP`=1800、`Phase2`=1200；碎片掉落各档 +1
- 修改（噩梦档，仅战斗 3-8，战斗 1-2 与常规逐格相同）：
  - HP：史 5,5,13,16,21,40,60,120 / 兔 2,2,3,3,6,30,30,30 / 狐 6,20,26,39,52,80,140,200（狐狸 C6-C8 = 常规原值，精英不加强）
  - 投放：E3-E5 ×1.2、E6-E8 手工抬到 60/50/60、80/70/80、80/60/70；同屏上限 26,28,60,70,78,100,120,100；齐射 E6-E8 = 6
  - Boss：`Enemies.M_SHEEP`=20000、`Phase2`=12000
  - 移速、全部伤害、前摇、攻击间隔、技能冷却、碎片掉落一律 = 常规（策划限定只动血量/投放/同屏上限三维度）
- 刷新方式：`python scripts/data/sync_difficulty_xlsx_to_csv.py` → `--check` → `python scripts/validate_project.py` → **重启 Unreal Editor**（只停 PIE 不重载 CSV）→ 设置页选难度 → **新游戏**（Continue 会沿用存档难度）
- 验证入口：日志 `LogReEcho: Display: [Difficulty] Run locked to Party|Standard|Nightmare`；投放看 `[EncounterSpawn] warning wave=... role=... requested=N`；血量看 `enemy=BP_EnemyGameplay_<Kind>_C_N ... maxHealth=`；也可在 PIE 控制台用 `GMShowEnemyHealth` 直接显示血量、`GMGotoEncounter <N>` / `GMGotoBoss` 跳关
- 策划确认：chenglexi 定档「战斗 1-2 持平常规、3-5 逐步提升、6-8 显著提升，只动血量/投放/同屏上限」；实机验收战斗 1-5 逐格通过后确认推送
- 限制与踩坑：
  - **三档工作簿内所有数值都是字符串**（`'5'` 而非 `5`）。用 `isinstance(v, int)` 做键匹配会让 `EnemyCombatStats`/`EncounterWaves`/`Encounters`/`EnemyShardDrops` 全部静默跳过（实测应改 244 格只改到 15 格）。键匹配一律 `int(str(v))`，写入一律 `str(val)`
  - **`BossPhases.M_SHEEP_Phase1.PhaseMaxHealth = '0'` 是语义值**，含义「沿用 `Enemies.MaxHealth`」（该行 `RefillHealthPolicy='None'`）。改 Boss 一阶段血量必须改 `Enemies.M_SHEEP.MaxHealth`，Phase1 恒为 0 不要动
  - **`EnemyAbilities.CooldownSeconds` 按技能一行、不分关**，做不到「前 N 关不变、后面加快」；只能按怪物出场关卡区分（兔子从战斗 1 出场故不可改，狐狸仅 3+、Boss 仅 8 可改）。`EnemyCombatStats.AttackIntervalSeconds` 是分关字段，可按 CombatIndex 精确区分
  - **验血量时会被跨关保留怪干扰**：同 Stage 内小关切换保留存活怪（`PreserveEnemiesBetweenEncounters`），日志里「每关首次出现的血量」可能是上一关遗留怪。应收集每关出现过的血量集合，期望值在集合内即通过
  - **倍率取整必须整数运算**（ceil(v×1.3) 写 `-(-(v*13)//10)`）。曾把 12×1.6=19.2 错填 19（应为 20）、兔子 2×1.6=3.2 错填 3（应为 4）
  - **改完生产表必须回写设计稿**，否则下一轮「按文档应用」会把旧值当新意图（已实际踩坑：设计稿 Boss 血量停在旧值 3900/2600，若照字面执行会覆盖已确认的新值）
  - Excel 会把以 `=` 开头的文本当公式（设计稿里的 `= main` 变成 `#NAME?`），应写全角 `＝main` 或 `同 main`
  - `Boss` 存在 `boss_phases.csv` 中未定义的 `phaseIndex=3` 状态，其 `maxHealth` = (一阶段+二阶段)×5，常规档同样存在（5000→25000），属既有行为；抬高 Boss 血量会等比放大该值，需留意
- 报告交接：无（本次为需求性数值填充，非 Bug）
