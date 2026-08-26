# Plan 114 - 程序 - 正式音频资源替换与变体接入

## 协调

- Planner 负责人：Gavyn-side Planner（当前程序用户授权的同一 AI 规划与集成）。
- Executor 负责人：Gavyn-side Executor（同一 AI 分阶段执行）。
- Plan 编写方（AI 侧）：Gavyn-side AI（Codex）。
- 实现编写方（AI 侧）：Gavyn-side AI（Codex）。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：`origin/main@a04e4dec6dfdcdf7b1c164ddeed1840af657f2d5`。
- 本地实现方式（可选，仅作交接说明）：独立工作树 `ReEcho-plan114-audio`，本地分支 `plan/114-formal-audio-assets`；Plan-only 发布后在同一工作树继续分阶段实现。
- 依赖 / 阻塞：依赖已关闭 Plan46 的 31 个稳定 EventId、音频服务、显式 Cook 目录与资源验证工具；资源映射以用户交付的 `C:\Users\gavynqiu\Documents\miniGame\音频\ECHO音频配置说明.xlsx` 可见内容为需求来源。法杖变体没有当前生产 `WeaponId`，不得伪造，延期到玩法提供稳定 ID 后接入。
- Writes: 本 Plan；`Design/Audio/AUDIO_SOURCES.md`、`Design/Audio/Source/**`、必要的确定性裁切产物与工具；`Design/Data/ReEchoAudioEvents.xlsx`、`Content/Data/audio_events.csv`；`Content/ReEcho/Audio/**`；`scripts/audio/**`、`scripts/data/author_audio_events.py`；`Source/ReEchoAudio/**`；`Source/ReEcho/Public/Combat/ReEchoCombatAudioAdapterComponent.h`、配对实现、`Source/ReEcho/ReEchoGameMode.*` 及聚焦测试；`shared/CODEBASE_MAP/modules/MOD-ReEchoAudio.md`、`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`。
- Stable Reads: 用户交付的 `音频/音乐/**`、`音频/音效/**` 与 `ECHO音频配置说明.xlsx`；Plan46 音频资产与来源记录；`Content/Data/stages.csv` 的 `Stage.1/Stage.2/Stage.3/Stage.Boss`；`Content/Data/weapons.csv` 的现役 `W_J_04/W_J_01/W_J_08/W_J_09`；`MOD-ReEchoCombat` 发布的最终 Attack/Hit/Hurt/Kill/Death 与 ElementReaction 事件；当前 UI、拾取、商店和流程成功事务。
- 影响模式：`Exclusive`，因为同名 SoundWave、权威音频 XLSX/CSV、导入脚本和最终 Cook 结果必须作为一个完整发布单元审计，二进制资产不能静默覆盖并行修改。
- 兼容承诺 / 下游操作：保留现有 31 个基础 EventId、五总线、存档与录制字节；现有无 `VariantId` 请求继续命中基础目录项。目录扩展为 `EventId + 可选 VariantId` 后，严格先查精确变体，再回退同 EventId 的空变体；音频失败仍不得影响玩法。用户负责最终可听性、响度、循环与审美验收。
- 明确排除：不创建不存在的法杖 `WeaponId`；不修改伤害、反应、购买、拾取、关卡推进或 UI 成功条件；不让音频完成回调驱动玩法；不以文件名自由文本作为运行时判断；不替换需求表明确保留 Generated 占位的 `Combat.Block/Combat.Kill/Enemy.Attack/Boss.Spawn/Boss.Attack/Echo.*`；不声明未提供的来源平台或许可事实；不使用 Exchange，不推送任务分支。

## 锁定目标

把用户交付目录中已明确映射的终稿音频接入正式音频目录，优先覆盖无需玩法改动的占位资源，再处理裁切，最后接入变体和新增稳定语义；全部资源必须可预载、可 Cook、可由权威玩法事件触发，并保留无音频时的安全降级。

### 第一阶段：直接替换

保持现有 AssetPath 和 EventId 不变，正式替换以下资源：

- BGM：`Music.Menu -> MainMenu_v1.mp3`、`Music.Shop -> STAGE3_v3.mp3`、`Music.Boss -> STAGE3_v2.mp3`。
- UI：`UI.Hover -> 触发带叮响机关开关音效 01.mp3`、`UI.Confirm -> cilck.mp3`、`UI.Cancel/UI.Error -> 取消.wav`、`UI.Purchase -> 购买.wav`、`UI.CardSelect -> 选卡.wav`。
- 战斗/流程：`Combat.Hurt -> A_short_impact_hit,__#4-1787644382401.wav`、`Combat.Death -> 失败.wav`、`Enemy.Death -> 15通用受击音效.mp3`、`Boss.Death -> 胜利.wav`、`CameraMove -> 时钟转动.wav`、`Revive -> 取消.wav`。
- 未在交付表中指定终稿的 Death/Victory BGM、Ambience 和其他 Generated 事件保持现状，不因“正式替换”名义擅自选择候选文件。

### 第二阶段：确定性裁切

- `Enemy.Spawn` 使用 `用后半段，怪物出生.wav` 的后半段；裁切必须由仓库脚本以固定参数生成，保留原始交付文件和派生记录，可重复校验哈希/时长。
- 对第一阶段文件做静音前导审计；只有表格备注或客观波形/时长证据明确要求时才裁切，不用主观猜测批量破坏源文件。

### 第三阶段：变体与耦合语义

- 音频目录从锁定 14 列迁移为新增可选 `VariantId` 的 15 列 Schema；允许同一 EventId 有一个空变体基础行和多个唯一非空变体行，禁止重复 `(EventId, VariantId)`。
- `Combat.Attack` 使用现有攻击提交的 `WeaponId`：`W_J_08 -> 弩.mp3`、`W_J_09 -> 枪.wav`、`W_J_01 -> 剑快速地劈砍两下-YS070510.mp3`、`W_J_04 -> 镰刀.wav`；当前无生产法杖 ID，保留为未接入资源。
- `Combat.Hit` 使用最终 `FReEchoDamageEvent.Element` 转换出的稳定元素 ID：`Flame -> 火元素攻击A.mp3`、`Lightning -> 电.wav`、`Grass -> 草元素.wav`、`Water -> 草元素 (2).wav`。该映射严格照交付表执行；如用户认为 Water 文件名是表格笔误，需在人工验收时明确改选。
- `Music.Encounter` 使用权威 `StageId` 作为变体：`Stage.1 -> STAGE1_v2.mp3`、`Stage.2 -> STAGE2_v1.mp3`、`Stage.3 -> STAGE3_v1.mp3`；Boss 继续使用 `Music.Boss`，基础 `Music.Encounter` 保留为未知 Stage 的回退。
- 扩展音乐状态 API 保存期望 `VariantId`，在异步预载和 World 替换重建后仍以同一 `(StateId, VariantId)` 恢复；旧调用保持源码兼容。
- 对交付表建议新增的 `Combat.Reaction.*`、`Flow.Victory`、`Item.Pickup`、`UI.Equip/UI.Unequip` 和卡牌出现音，只允许消费现有权威成功/完成事件。若任一玩法语义尚无稳定发布点，只把资源纳入来源清单并记录后续项，不在本 Plan 内制造玩法状态或用 Widget/Actor 具体类型反向污染 `ReEchoAudio`。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoAudio`、`MOD-ReEcho`；`MOD-ReEchoCombat` 仅 Stable Read 其既有值类型和事件，不修改其结算权威。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEchoAudio.md`、`shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已加入 Writes。
- 设计意图：资源选择继续由音频目录拥有；玩法只提供稳定 EventId、VariantId、位置和来源。变体解析属于音频目录，不在武器、元素、Stage 或 UI 调用点写 Sound 路径。
- 权威状态与依赖：音频 XLSX/CSV 继续是 EventId 到资源的唯一表格权威；新增 `VariantId` 只扩展查找键，不改变 Combat/Weapons/Run/UI 的状态所有者。依赖方向保持 `MOD-ReEcho -> MOD-ReEchoAudio`，Audio 不反向依赖玩法模块。
- 决策记录：采用“可选 VariantId + 基础回退”，不为每把武器/元素/Stage 增加散乱 EventId，也不在 C++ 建资源路径字典。代价是一次 14→15 列 Schema 迁移和状态 API 的兼容扩展，但能复用请求中已存在的 `VariantId` 并保持 31 个基础语义稳定。
- 相关文档同步范围：关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`MOD-ReEchoAudio.md`、`MOD-ReEcho.md`；仅在拓扑/索引事实变化时修改前两者。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEchoAudio.md`：待记录目录 Schema、变体回退、状态恢复与资产验证变化。
  - `MOD-ReEcho.md`：待记录 Weapon/Element/Stage VariantId 的稳定翻译位置及新增语义触发结果。
  - `ARCHITECTURE.md` / `README.md`：待审阅；预期依赖拓扑和模块索引不变。

## 锁定验收

- [ ] 第一阶段 15 个既有 EventId 的目标 SoundWave 已由对应正式源文件重新导入，路径、循环/OneShot、声道和来源哈希可审计。
- [ ] `Enemy.Spawn` 裁切产物可由固定脚本重建，开始播放即进入表格要求的后半段主内容。
- [ ] 四把现役武器、四元素和三普通 Stage 的精确 VariantId 能解析到各自资源，未知/空变体可靠回退基础资源。
- [ ] 菜单、商店、Boss、普通关卡 BGM 及直接替换的 UI/战斗/流程音均有自动化或结构化触发证据；音频不可用不改变玩法结果。
- [ ] 目录 XLSX 与生成 CSV 同步发布，15 列 Schema、复合唯一键、预载和 Cook 验证通过。
- [ ] 最终集成候选执行 `-FullRebuild`，提交匹配精选 Editor 预构建包，并生成干净本地策划测试包；包内音频清单与目录一致。
- [ ] 用户在 PIE/测试包完成可听性、响度、循环、Water 映射和各变体区分度验收。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@a04e4dec6dfdcdf7b1c164ddeed1840af657f2d5`；本地主线已由用户确认后安全 fast-forward。
- 引擎/构建可用性：Plan46 已证明 UE 5.8 音频导入、资产验证、FullRebuild 与 Windows Cook 路径可用；本 Plan 不复用其最终证据，准确候选仍须重跑。
- 现有聚焦测试结果：Plan46 的 31 路由/资产检查和 `ReEcho.Audio` 自动化为历史基线；本 Plan 先运行当前基线静态检查，再按阶段补充验证。
- 共享契约 / 难合并资源风险：`ReEchoAudioEvents.xlsx`、生成 CSV、同名 `.uasset` 和精选 DLL 是二进制/生成热点；发布前必须 fetch、逐路径审计并在最终集成版本重建。
- 基线损坏时的停止条件：当前基线无法加载既有 31 行目录、导入脚本无法覆盖 SoundWave、用户交付文件缺失/损坏，或远端出现同路径音频/Schema 修改且存在真实语义冲突时停止受影响阶段并报告。

## 实现提纲

1. 第一阶段复制明确终稿源文件到仓库来源目录，更新来源清单，以 UE Python 覆盖同路径 SoundWave；验证后形成独立本地检查点。
2. 第二阶段实现并验证固定裁切，只替换 `Enemy.Spawn`；对其他前导延迟只输出审计结果。
3. 第三阶段迁移权威 XLSX/CSV 与目录复合键，接通 Weapon/Element/Stage VariantId，再逐项接入确有稳定语义发布点的新事件。
4. 更新聚焦自动化、路由/资产校验和模块文档；执行最终 FullRebuild、干净 Cook、包内清单与启动烟测。
5. 交给用户完成主观听感验收；通过后关闭 Plan、发布最终候选并按规则清理本地任务分支/worktree。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Plan-only | `python scripts/validate_project.py`、`git diff --check` | Plan 结构与仓库静态规则通过 |
| 来源/裁切 | 来源哈希清单、裁切工具 `--check`、音频头/时长/声道审计 | 原始交付可追溯，派生产物确定性一致 |
| XLSX/CSV | `python scripts/data/sync_xlsx_to_csv.py --check`、聚焦数据测试 | 15 列 Schema 与复合唯一键一致，无手改 CSV |
| 路由 | `python scripts/audio/validate_audio_event_routes.py` | 基础 EventId、VariantId 来源和生产触发无孤立映射 |
| UE 资产 | 同克隆 Unreal 锁下运行 `scripts/audio/validate_audio_catalog_assets.py` | 全部目录行解析为 SoundWave；Loop/OneShot、3D 单声道与路径正确 |
| C++ | `.clang-format`、`scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0，刷新聚焦预构建包 |
| 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Audio` 及受影响 ReEcho/Combat/UI 聚焦套件 | 变体精确命中、基础回退、状态恢复和语义路由通过 |
| 最终发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`、`python scripts/validate_project.py`、预构建检查、`git diff --check` | 最终集成候选与精选 Editor 包匹配 |
| Cook | 干净 Windows 策划测试包、IoStore/manifest 音频清单、启动烟测 | 目录资产全部进包且可启动 |
| 人工 | 菜单/商店/Stage1-3/Boss、四武器、四元素和主要 UI/战斗流程试听 | 用户记录 Passed，或明确列出需返工映射/响度/裁切 |

## 执行记录

### 变化

- 2026-08-26：完成只读盘点。交付工作簿含 3 个可见 Sheet；“项目音频事件清单”覆盖现有 31 个基础事件，“音频配置说明”另列武器、元素、Stage 与建议新增语义。发现现役武器仅 4 把，法杖没有生产稳定 ID。
- 2026-08-26：按用户决定将工作拆为直接替换、确定性裁切、变体/耦合语义三阶段，优先执行无运行时耦合的资源替换。

### 证据

- 远端最大编号为 Plan113，本 Plan 使用下一个空闲编号 114。
- `FReEchoAudioEventRequest` 已有 `VariantId`；`Combat.Attack` 已传 WeaponId，但当前 Catalog 仍只按 EventId 查找；`Combat.Hit` 的最终事件已携带 Element；Stage CSV 已提供稳定 `Stage.1/2/3/Boss`。

### 剩余风险

- Water 映射在交付表中指向 `草元素 (2).wav`，文件名与语义不一致；先按权威表实施，人工试听时必须明确确认。
- 若源文件包含不可接受的静音前导、爆音或不平滑循环，程序检查只能定位并给出裁切/循环证据，最终取舍由用户试听决定。
- 交付文件的外部许可/署名信息未提供；来源清单只记录“用户交付、许可待用户确认”，不作可发布权利声明。
- 二进制 SoundWave 和 XLSX 难以自动合并；任何远端同路径变化都会使相关导入、Cook 与人工试听证据失效。

### 人工验收结果/请求

- `PendingBeforeClose`：最终资源映射、Water 文件、响度、循环、裁切起点和变体区分度由用户试听确认。

### 架构文档审阅结果

- 待实现完成后逐项填写。
