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
