# Plan 143 - 程序 - 抽卡与商店双段媒体过渡

## 协调

- Planner 负责人：当前程序用户授权的 Planner（Codex）。
- Executor 负责人：当前程序用户授权的 Executor（Codex）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@bf40050cb66161a7abd441e8822f51bb19938853`。
- 本地实现方式（可选，仅作交接说明）：Plan worktree `C:\tmp\ReEcho-plan143-card-shop-transition`；Plan 发布后继续在该任务专属 worktree 实施。
- 依赖 / 阻塞：源序列位于 `F:\frames`，已只读审计为 `frame_0000.png` 至 `frame_0120.png` 共 121 帧、无缺号、关键帧均为 `1920x1080 Format32bppArgb`；用户已确认第一段包含 `frame_0000.png`。依赖现有 `UReEchoEncounterTransitionWidget`、WmfMedia/HAP Alpha、UIFlow 最上层 Screen、Run CardChoice Phase 与战后商店入口。
- Writes: 本 Plan；`Source/ReEcho/{Public,Private}/ReEchoGameMode.*`；`Source/ReEcho/{Public,Private}/UI/ReEchoEncounterTransitionWidget.*`；对应聚焦测试；`Content/Movies/EncounterTransition/EncounterEndToCardChoiceV2.mov`、`CardChoiceToShop.mov`；`Content/ReEcho/UI/EncounterTransition/` 下两个新 MediaSource 资产及必要共享媒体材质引用；必要的幂等转码、作者ing与审计脚本；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoUI.md`；精选 Editor 预构建包。
- Stable Reads: `F:\frames\frame_0000.png` 至 `frame_0120.png`；旧 `Content/Movies/EncounterTransition/EncounterTransitionAlpha.mov` 及其 `FMS_EncounterTransition`、`IMS_EncounterTransition`、`MP_EncounterTransition`、`MT_EncounterTransition`、`M_UI_EncounterTransition`；Plan124/Plan133；`UReEchoRunSubsystem` CardChoice/Planning 状态与 `ShowPostTraitShop()`；UIFlow 层级与焦点契约。
- 影响模式：`SharedContract`，因为同一转场 Widget 将承载两个普通 HAP Alpha 播放目的，GameMode 需要在最终免费抽卡提交与商店打开之间增加完成门；不改变 Run 卡牌事务或商店交易权威。
- 兼容承诺 / 下游操作：旧 `EncounterTransitionAlpha.mov` 和旧 MediaSource/MediaPlayer/MediaTexture/材质资产全部保留，不删除、不覆盖；只把普通“关卡结束到抽卡”运行时引用切换到新第一段，并新增第二段。第一关继续走 Stage01To02 CG 特例，第八关继续走最终结算；商店内付费卡包选择不触发第二段。
- 明确排除：不修改卡牌池、刷新、价格、商店商品、Run Phase 规则、第一关 CG、倒计时后处理、音乐内容或关卡时长；不把 PNG 导入为 Sprite/Flipbook；不删除旧媒体资产；不在 PIE 验收和用户二次确认前发布实现。

## 锁定目标

1. Encounter 2 至 7 的普通关末在权威倒计时到 0 后，从第 0 帧播放由 `frame_0000.png` 至 `frame_0057.png` 生成的新 HAP Alpha 过渡；旧普通过渡文件继续保留但不再作为该路径的运行时播放源。新媒体完整结束并按既有淡出节奏揭示免费抽卡页，打开/首帧/播放停滞失败时 fail-open 到抽卡，不得卡住 Run。
2. 玩家在免费抽卡页选择卡牌并点击确认后，必须先成功提交 `ApplyTraitCard()`。若提交失败则留在抽卡页且不播放；若本次关末仍有任何免费抽卡次数、Run 仍处于 `CardChoice`，则沿用现有下一轮抽卡流程且不播放。只有本次关末全部免费抽卡均已完成、最后一张免费卡提交成功、Run 已正式离开 `CardChoice` 并即将进入战后商店时，才从第 0 帧播放由 `frame_0058.png` 至 `frame_0120.png` 生成的新 HAP Alpha 过渡。
3. 第二段播放期间确认按钮和卡牌交互不可再次触发，商店尚不可操作；已确认抽卡页保留为透明媒体的底层画面。媒体完成后在最上层覆盖仍存在时关闭抽卡页、打开 `PostTraitIntermission` 商店，再淡出媒体层并把焦点交给商店。媒体失败时直接完成相同页面切换；商店创建失败时沿用现有 fail-open 进入下一 Encounter，不得黑屏或卡死。
4. 两段媒体均为 `1920x1080`、40 FPS、非循环、HAP Alpha MOV，并使用现有 Fill 等比居中裁切与 ZOrder 10000；第一段 58 帧（约 1.45 秒），第二段 63 帧（约 1.575 秒）。不使用视频内音轨，抽卡与商店间既有 `MusicShop` 连续播放，不重启音乐。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI`、`AREA-UI`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 的结算/Run 推进状态机；维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 的媒体播放目的、页面切换与焦点契约；两者均已加入 `Writes`。
- 设计意图：Gameplay/GameMode 继续拥有卡牌提交、Run Phase 判断、页面推进和失败降级；`UReEchoEncounterTransitionWidget` 只按显式播放目的选择 MediaSource、从 0 播放、监测首帧/时钟/完成并报告结果。媒体 Widget 不写 Run、不应用卡牌、不打开商店。
- 权威状态与依赖：不改变 `UReEchoRunSubsystem` 的卡牌/商店状态所有权。GameMode 在现有 `HandleTraitCardSelected()` 的成功分支根据提交后的 Run Phase 判断是否启动第二段，并新增独立的 CardChoiceToShop 播放/淡出状态；UIFlow 继续拥有 Screen 层级、关闭、焦点与输入模式。
- 决策记录：
  - 使用两个独立新 MOV 和两个新 MediaSource，不覆盖旧 MOV，满足可回退与旧内容保留。
  - 不导入 121 张 Texture/Sprite/Flipbook；沿用已验证的 HAP Alpha 媒体链，避免 121 个 UE 资产和逐帧 Slate 更新。
  - 第二段只挂在最终免费抽卡事务成功之后，不直接挂 `ConfirmButton`，避免提交失败、连续多次免费抽卡或商店付费卡包误触发。
  - 媒体完成前保持抽卡页在底层；完成时先在覆盖层下建立商店再淡出，避免黑屏、空帧和商店提前可操作。
  - 转码帧区间使用显式 staging 清单或独立临时目录，禁止用连续编号输入暗中把 `frame_0058` 混入第一段，或把 `frame_0057` 混入第二段。
- 相关文档同步范围：`MOD-ReEcho.md`、`MOD-ReEchoUI.md` 需要更新；`ARCHITECTURE.md` 已审阅预期无需修改，因为没有新增 Runtime Module 或依赖方向；`CODEBASE_MAP/README.md` 已审阅预期无需修改，因为没有新增稳定模块/AREA 标识。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEcho.md`：记录最终免费抽卡后的媒体完成门和商店激活顺序。
  - `MOD-ReEchoUI.md`：记录两个普通 Alpha 媒体播放目的、资源路径、Fill/层级与 fail-open。
  - `ARCHITECTURE.md`：复审是否仍无拓扑变化。
  - `CODEBASE_MAP/README.md`：复审是否仍无索引变化。

## 锁定验收

- [ ] `frame_0000`–`0057` 与 `frame_0058`–`0120` 分别被准确编码为 58 帧和 63 帧的 40 FPS、1920x1080、带 Alpha、非循环 HAP MOV；审计脚本验证编码、帧数、时长、Alpha 和源区间，源 PNG 不改动。
- [ ] Encounter 2–7 普通关末使用新第一段，从第 0 帧完整播放后才显示抽卡；旧媒体文件与旧 UE 媒体资产仍存在且未被覆盖。
- [ ] 本次关末全部免费抽卡完成后才播放第二段：前序免费抽卡确认均只进入下一轮抽卡，最后一张免费卡提交使 Run 离开 `CardChoice` 后才启动；完整结束后才揭示并聚焦商店。视频期间无法重复确认或操作商店，MusicShop 连续。
- [ ] 卡牌提交失败、仍有下一次免费抽卡、第一关特殊 CG、第八关结算和商店付费卡包均不触发第二段。
- [ ] 两段媒体任一打开失败、首帧超时或播放停滞时按目标页面 fail-open，不黑屏、不重复提交、不跳过商店、不阻塞下一关。
- [ ] 1920x1080、超宽和窗口化 PIE 中 Fill、透明合成、首尾衔接、最上层覆盖、淡出及焦点由用户验收；人工验收前不发布实现。
- [ ] 必需构建、项目校验、媒体审计、聚焦自动化、预构建一致性和 `git diff --check` 通过，且不包含允许列表外生成物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@bf40050cb66161a7abd441e8822f51bb19938853`；实现前再次 fetch 并审计远端变化。
- 引擎/构建可用性：UE 5.8 标准 Editor 构建链、WmfMedia 与现有 HAP Alpha 普通过渡可用；实现前确认本任务 Editor/Commandlet 未占用资产。
- 现有聚焦测试结果：Plan 发布阶段不复用旧运行证据；实现阶段先运行/扩展 `ReEcho.UI.EncounterTransition.Policy`，并记录基线结果。
- 共享契约 / 难合并资源风险：`ReEchoGameMode.*`、`ReEchoEncounterTransitionWidget.*`、媒体资产目录和精选 DLL 是共享热点；二进制 MediaSource 只允许由本任务作者ing脚本在独占 Editor 中生成。旧 MOV/资产受保护，不得删除或原位覆盖。
- 基线损坏时的停止条件：现有普通过渡无法播放、Run Phase 与本 Plan 假设不符、源帧缺失/尺寸或 Alpha 不一致、FFmpeg/HAP 编码不可用、目标 UE 资产被其他 Editor 占用，或远端出现同路径逻辑冲突时停止并报告。

## 实现提纲

1. 从 `F:\frames` 生成两个显式 staging 序列并编码 HAP Alpha MOV；用 ffprobe/解码抽帧审计 58/63 帧、40 FPS、1920x1080、Alpha、首尾帧哈希与时长。复制到两个新 Movies 路径，旧文件不动，并按仓库 LFS 规则处理新 MOV。
2. 用幂等 Editor 作者ing脚本创建两个新 FileMediaSource；复用现有 MediaPlayer/MediaTexture/HAP UI Material，不创建第二套播放状态所有者。脚本验证资产软路径、FilePath、非循环和加载结果。
3. 把 Widget 的泛化入口收敛为显式播放目的：关末到抽卡、抽卡到商店、Stage01To02 CG；每次 Reset 后从 0 打开，沿用首帧/停滞/完成/淡出监测，并记录目的、源、媒体时间与失败原因。
4. GameMode 为 CardChoiceToShop 增加独立 Playing/Fading 状态和幂等门。每次免费卡牌事务提交成功后读取提交后的 Run Phase；只有全部免费抽卡耗尽、Phase 已离开 `CardChoice` 时才启动媒体。完成时在覆盖层下关闭抽卡、打开商店并淡出；失败直接执行同一目标页面切换。前序免费抽卡和付费卡包保持原路径。
5. 扩展聚焦自动化覆盖关次范围、Run Phase 判定、一次性触发、媒体完成/失败和页面顺序；维护 `MOD-ReEcho.md`、`MOD-ReEchoUI.md` 与 Plan 执行记录。
6. 执行 `.clang-format`、Editor Development 构建、媒体/资产审计、`ReEcho.UI.EncounterTransition.Policy` 及相关 TraitCard/UIFlow 测试、`validate_project.py`、预构建检查和 `git diff --check`；最后由用户 PIE 验收，验收前不发布实现。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 源序列 | 帧清单、尺寸/Alpha/哈希审计 | 0–57 共58帧，58–120 共63帧，无交叉、缺帧或源改动 |
| 编码媒体 | `ffprobe` 与首尾抽帧比较 | 两个 MOV 均为1920x1080、40 FPS、HAP Alpha、非循环，时长约1.45s/1.575s |
| UE资产 | 幂等作者ing/审计脚本 | 两个新 MediaSource 指向新 Movies；旧 MOV 和旧资产仍存在且未覆盖 |
| C++格式/构建 | `.clang-format`；`scripts\ue\Build-Editor.cmd -Configuration Development` | UHT/UBT成功，精选预构建包刷新 |
| 聚焦自动化 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.UI.EncounterTransition.Policy`；必要 TraitCard/UIFlow 用例 | 两个播放目的、最终免费抽卡触发门、失败降级和页面顺序通过 |
| 项目静态 | `python scripts\validate_project.py`；`python scripts\ue\prebuilt_editor.py check`；`git diff --check` | 项目、预构建与差异检查通过 |
| LFS | `python scripts/setup_lfs.py --check`；`git lfs status`；`git lfs fsck` | 新 MOV 是已还原对象和正确 LFS 指针，无损坏对象 |
| 人工 PIE | Encounter 2–7、失败预览、1920/超宽/窗口化 | 两段从0播放、透明Fill、无黑帧、商店不提前可操作、音乐连续、焦点正确 |

## 执行记录

### 变化

- Plan-only 阶段；尚未实现。

### 证据

- `F:\frames` 已只读确认共121张连续PNG，范围 `frame_0000.png`–`frame_0120.png`；用户确认第一段包含第0帧。关键边界帧为1920x1080、32-bit ARGB。

### 剩余风险

- 需在实现环境定位可用 FFmpeg/HAP 编码器并验证真实 Alpha 解码；PNG 文件时间戳不作为 FPS 权威，FPS 已由用户确认的现有普通过渡契约锁为40。
- 第二段透明内容对抽卡页和商店页的具体视觉遮挡仍需 PIE 判断；自动化不能替代透明合成观感。

### 人工验收结果/请求

- `PendingBeforeClose`：等待实现后验收两段动画内容、透明合成、切页时点、Fill、焦点和音乐连续性。

### 架构文档审阅结果

- Plan-only 阶段已识别 `MOD-ReEcho.md` 与 `MOD-ReEchoUI.md` 为实现 Writes；关闭前按“架构影响与设计决策”逐项填写。
