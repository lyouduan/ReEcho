# Plan 133 - 程序 - 第1至第2关过渡CG与商店跳过

## 协调

- Planner 负责人：当前程序用户授权的 Planner（Codex）。
- Executor 负责人：当前程序用户授权的 Executor（Codex，同一任务分阶段执行）。
- Plan 编写方（AI 侧）：Gavyn-side AI（Codex）。
- 实现编写方（AI 侧）：Gavyn-side AI（Codex）。
- 任务状态：`Review`。
- 人工验收：`Passed`。
- 本地规划 / 实现基线：`origin/main@89ac0166ff3bd60588d92fa1f696eacb75249457`；实现将组合用户尚待验收的 Plan130 本地候选，组合前必须审计同文件变化。
- 本地实现方式（可选，仅作交接说明）：规划 worktree `C:\tmp\ReEcho-plan133-stage1-to2-cg-plan`；执行使用独立 Plan133 worktree，不与 Plan130 worktree 共用目录。
- 依赖 / 阻塞：复用 Plan124/Plan130 的最上层媒体宿主和完成门，但第一关不进入其普通关末透明媒体/CardChoice链；源视频 `F:\MiniGame\过渡cg\video(66).mp4` 存在，约 15 秒、15,723,153 bytes，原文件只读保留。Plan130 人工验收结论仍独立存在，不由本 Plan 代替。
- Writes: 本 Plan；`Source/ReEcho/{Public,Private}/ReEchoGameMode.*`；`Source/ReEcho/{Public,Private}/Run/ReEchoRunSubsystem.*` 的第一关奖励跳过窄命令；`Source/ReEcho/{Public,Private}/UI/ReEchoEncounterTransitionWidget.*`；`Source/ReEcho/Private/Tests/ReEchoEncounterTransitionTests.cpp`、必要 Run/Stage 测试；`Content/Movies/EncounterTransition/Stage01To02.*`；对应 `Content/ReEcho/UI/EncounterTransition/**` MediaSource资产；必要 Unreal 作者ing/审计脚本；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoUI.md`；精选 Editor 预构建包。
- Stable Reads: `plans/124-encounter-card-transition.md`、`plans/130-countdown-ghost-postprocess.md`；`ReEchoRunSubsystem.*`、`ReEchoStageTransition.*`、`ReEchoUIFlowCoordinatorSubsystem.*`；现有 `EncounterTransitionAlpha.mov` 和 Transition Screen 生命周期。
- 影响模式：`SharedContract`，因为 Encounter 1 结算后需要显式跳过其 CardChoice Phase、原透明媒体和一次 Shop，并增加媒体完成门；不改变其他 Encounter 的通用 Phase/交易/存档结构。
- 兼容承诺 / 下游操作：仅当前完成关次 `EncounterIndex==1` 时跳过原权威00媒体、免费 CardChoice 和战后商店。顺序锁定为“Encounter 1结算 → Stage01To02 CG → BeginEncounter 2”。CG 必须从0播放到末帧；打开或播放失败 fail-open 进入第2关。Encounter 2及以后保持原透明转场、CardChoice/Shop 流程。
- 明确排除：不删除通用抽卡/商店系统、卡牌/商店数据或其他关次奖励；不改战斗计时、敌人、Stage连续性、存档Schema、Plan130后处理参数或第二关及以后透明关末动画；不覆盖源视频；不在用户PIE验收前发布实现。

## 锁定目标

第1关完成结算后不播放原权威00秒媒体、不发放免费抽卡、不打开战后商店，立即全屏 Fill 播放 `video(66).mp4` 对应的项目内过渡 CG；CG 到达末帧后才开始第2关。CG打开/解码/播放失败时记录明确错误并幂等进入第2关，不能卡死。

“删除1-2关卡商店流程”只表示删除这一次页面导航和等待，不删除商店代码、资产、报价、货币或其他关次行为。第2关结束后的战后商店及后续所有商店照常出现。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho / AREA-Encounter`、`AREA-Run`、`AREA-UI`，以及文档入口 `MOD-ReEchoUI`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `MOD-ReEchoUI.md`，两者均加入 `Writes`。
- 设计意图：GameMode 继续拥有页面/关末编排；Run 继续拥有 Phase、CardChoice提交、商店交易与EncounterIndex。新增CG是GameMode消费当前关次和媒体完成状态的表现门，不把视频播放状态写入Run或存档。
- 权威状态与依赖：`EncounterIndex` 和 `Phase` 仍来自 `UReEchoRunSubsystem`；CG Widget只报告 `Finished/Failed`。`BeginNextEncounter()`仍是进入第2关的唯一现有端点。跳过商店不得伪造Shop Phase或调用购买接口。
- 决策记录：
  - CG直接放在第1关结算之后，使它成为进入第2关前唯一全屏阶段；该关配置的免费卡不生成候选、不发放。
  - 复用现有 Transition Screen 的ZOrder/Fill和动态MediaPlayer生命周期，但必须为普通不透明MP4提供独立播放配置，不能让Hap Alpha材质错误解释普通视频。
  - 项目内媒体使用语义化文件名 `Stage01To02`，不沿用 `video(66)`；源文件只读复制。若UE媒体探测证明编码不兼容，则用可追溯脚本生成兼容MP4，保留分辨率/帧率/音频并记录转换参数。
  - CG结束门只认 `OnEndReached` 或明确失败；不使用固定15秒Timer代替媒体状态。
  - Run 提供第一关专用窄命令，清理待选卡缓存并把 `CardChoice/Planning` 规范化为 `Planning`；不伪造卡牌提交。
- 相关文档同步范围：关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`modules/MOD-ReEcho.md`、`modules/MOD-ReEchoUI.md`；预计模块拓扑和稳定索引不变，前两者若无事实变化只记录“已审阅、无需修改”。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEcho.md`：记录Encounter 1结算后的直接CG门和奖励页面跳过；
  - `MOD-ReEchoUI.md`：记录普通不透明CG的Fill、层级、播放/失败生命周期；
  - `ARCHITECTURE.md` / `README.md`：审阅拓扑和索引是否变化。

## 锁定验收

- [ ] 第1关自然结束后不播放现有00秒透明媒体、不打开CardChoice/Shop，直接从0播放Stage01To02 CG。
- [ ] CG在视口最上层按Fill等比裁切、从0开始、不循环；声音按源视频播放；到达末帧前不得开始第2关。
- [ ] CG结束后关闭媒体层、恢复Gameplay输入并通过现有 `BeginNextEncounter()` 开始Encounter 2；无重复开始、黑屏、残留Widget或暂停状态。
- [ ] 第1关CG打开/解码/播放失败时记录明确日志并fail-open进入Encounter 2；失败终点幂等。
- [ ] 第1关不创建/聚焦CardChoice或战后商店；Run 清理该关待选卡并以 Planning 状态保存，不伪造卡牌领取。
- [ ] Encounter 2及以后仍按配表进入免费CardChoice和战后商店；Boss、死亡、胜利、重启、读档和主菜单路径不变。
- [ ] 源视频不被覆盖；项目内媒体和MediaSource经过UE加载/播放审计并进入Cook。
- [ ] `.clang-format`、Editor Development构建、聚焦自动化、项目校验、`git diff --check`、最终 `-FullRebuild`、精选预构建检查及必要Shipping Cook/烟测通过。
- [ ] 用户PIE验收CG内容、声音、Fill裁切、衔接时机、商店确实跳过以及第2关可正常操作；验收前不关闭或发布实现。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的UE生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@89ac0166ff3bd60588d92fa1f696eacb75249457`；远端最大编号Plan132，本Plan使用Plan133。
- 引擎/构建可用性：UE 5.8 Editor构建链可用；源文件存在，Windows Shell报告时长约15秒。规划阶段未声称UE解码或音频轨验证通过。
- 现有聚焦测试结果：规划阶段未运行新候选测试；执行前记录 `ReEcho.UI.EncounterTransition`、`ReEcho.Encounter`、`ReEcho.StageTransition` 基线。
- 共享契约 / 难合并资源风险：Plan130本地候选已修改 `ReEchoGameMode.*`、Transition Widget、媒体/后处理资产和预构建DLL，但尚未进入origin/main；Plan133必须从其已验收候选或最新main组合，不得用旧main整文件覆盖。新增媒体二进制独占路径，原视频不得回写。
- 基线损坏时的停止条件：Plan133编号被远端占用；Plan130验收导致关末媒体状态机重做；源视频UE无法解码且环境无可审计转码工具；跳过奖励必须修改Save Schema。

## 实现提纲

1. 发布并核验Plan133；读取Executor规则、相关LESSONS小节及Plan130最终候选，审计其与最新main的物理/逻辑耦合。
2. 在独立Plan133执行worktree组合Plan130候选；复制源视频到语义化Movies路径，使用UE媒体工具探测轨道、时长、分辨率、帧率与音频；必要时只生成项目内兼容副本。
3. 扩展/新增全屏CG播放宿主：普通MP4使用不透明MediaTexture路径、Fill布局、从0播放、OnEndReached/Failed和幂等Reset；保留现有Hap Alpha路径。
4. 在GameMode建立窄的Stage01To02完成门：当前完成关次1时让Run清理该关CardChoice状态，直接转为CG；结束/失败统一调用现有下一关入口。
5. 增加纯策略和Widget聚焦测试，覆盖关次资格、直接CG、CG完成/失败、奖励页面跳过和后续关次不变。
6. 更新模块文档与Plan执行记录；格式化并执行增量构建、聚焦自动化、媒体资产审计、项目校验和diff检查。
7. 用户PIE验收后才关闭；随后在最新main组合上执行FullRebuild、预构建检查、必要Shipping Cook/烟测和发布锁流程。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Plan-only | `python scripts/validate_project.py`、`git diff --check` | Plan133编号、结构、范围和中文正文通过 |
| 源媒体 | 文件哈希/大小只读检查、UE媒体轨道审计 | 原文件未改；项目内CG可解码，轨道/时长/尺寸/帧率/音频有日志 |
| Widget | `ReEcho.UI.EncounterTransition`及新增CG聚焦测试 | Alpha与Opaque两种模式、Fill、0秒、完成/失败和Reset通过 |
| 状态机 | `ReEcho.Encounter`、新增Stage01To02策略测试 | 第一关直接CG、完成/失败进入2、后续关不变 |
| Stage回归 | `ReEcho.StageTransition`、相关Run/Card/Shop测试 | 同Stage连续性、第一关奖励跳过、存档和其他商店无回归 |
| C++ | `.clang-format`、`scripts\ue\Build-Editor.cmd -Configuration Development` | UHT/UBT成功并刷新匹配预构建包 |
| 静态 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 项目和候选一致 |
| Cook | Windows Shipping Cook/Package、manifest和烟测 | CG与MediaSource进入包且成品可启动 |
| 人工PIE | 第1关自然/GM结束、正常/失败媒体 | 用户确认无原转场/抽卡/商店、内容/声音/Fill、CG后第2关正常 |

## 执行记录

### 变化

- 用户确认CG内容已正常播放后，第一关CG沿用局间门停止玩家、Echo、敌人模拟与关卡推进，并显式停止当前Music State；不暂停World媒体时钟。独立CG SoundWave从首个有效视频帧开始播放，只有CG完整结束或明确失败才进入第二关，新关卡重新建立玩法和关卡音乐。

- 模块级复审确认旧实现把 `Play()`/`IsPlaying()`误当作可见视频证据，并把不透明CG的HAP视频与PCM音轨交给同一WmfMedia时钟；现将 `Stage01To02.mov` 无损拆为454帧Hap1纯视频和独立 `S_Stage01To02` SoundWave。Widget明确区分 Opening、首帧等待、Playing、Completed、Failed，只有MediaTexture出现有效表面后才启动独立音频，并监测实际媒体时间推进。

- 已在独立执行 worktree 将 Plan130 候选与最新 `origin/main@41da5187` 合并，保留倒计时后处理、透明关末动画和主线新增音频/UI变化。
- 源视频保持只读；项目副本最初原样复制，PIE 证明 HEVC 只能 Open/Play、MediaTexture 永远停在 2x2 且无结束事件后，项目副本转码为 H.264 High 4.1/YUV420P + AAC，仍为1920x1080/30 FPS/约15秒。新增 `FMS_Stage01To02` 和可复跑的 UE 作者ing脚本，并把 MP4 声明为 RuntimeDependency。
- Transition Widget 新增普通不透明媒体路径：复用现有 ZOrder 10000/Fill/MediaPlayer 完成门和已验证的动态 UI 材质采样链；普通 MP4 由 ElectraPlayer 解码并提供不透明视频样本，GameMode MediaSoundComponent 绑定同一播放器输出源音轨。
- GameMode 在 `EncounterIndex==1` 结算后直接进入 Stage01To02 CG；Run 通过窄命令清除该关待选免费卡并回到 Planning，不创建原透明媒体、CardChoice 或 Shop 页面。完成或失败后幂等调用既有下一关入口；其他 Encounter 的流程不变。
- CG 播放期间不使用全局 World Pause：Gameplay 由既有局间悬停和菜单能力阻挡冻结。普通 MP4 显式使用 ElectraPlayer；MediaTexture 尺寸只保留诊断，不再作为首帧或失败依据。完成以 OnEndReached 为主、媒体末尾容差为兜底，播放时间连续5秒不推进才明确失败并fail-open。

### 证据

- World Pause与独立UI音频候选完成clang-format并通过Editor Development构建，精选预构建包已刷新；“游戏内容/游戏音乐暂停、CG音乐播放、末帧前不进入第二关”仍由用户PIE验收。

- FFmpeg 7.1拆轨审计：纯视频仍为1920x1080、30 FPS、454帧、15.13秒、Hap1，音频WAV为44.1kHz双声道PCM、15.14秒；UE作者ing成功导入 `/Game/ReEcho/UI/EncounterTransition/S_Stage01To02`，构建为BINKA SoundWave。
- 拆轨与状态诊断候选的 Editor Development 构建成功并刷新精选预构建包；`ReEcho.UI.EncounterTransition.Policy` 1项成功，项目校验、预构建一致性与 `git diff --check` 通过。实际连续视频帧、独立声音和结束后进入第二关仍为人工PIE门禁。

- UE 作者ing日志：`[Plan133] PASS`，保存 `/Game/ReEcho/UI/EncounterTransition/FMS_Stage01To02`，项目副本大小 15,723,153 bytes。
- Windows 媒体属性：1920x1080、30 FPS、约15秒、视频数据率约8190 kbps、音频比特率约189 kbps；当前系统识别视频压缩标识为 HEVC，实际 UE 画面/声音解码留待 PIE 验收。
- Editor Development 构建成功并刷新精选预构建包；`ReEcho.UI.EncounterTransition.Policy`、4项 `ReEcho.Encounter.*`、3项 `ReEcho.StageTransition.*` 均通过。
- 用户纠正顺序后再次构建成功；更新后的 `ReEcho.UI.EncounterTransition.Policy` 直接实例化 Run，证明第一关结算后的 CardChoice 状态可清理为 Planning，且第二关拒绝该跳过命令；3项 StageTransition 回归继续通过。
- 失败PIE日志明确记录 `Stage01To02.mp4` 已打开并从0调用Play，但 MediaTexture 只从清屏变为 `2x2`，直到退出都没有真实尺寸或 `OnEndReached`；FFmpeg探测源视频为 HEVC Main/hvc1。项目副本用 `libx264 -crf 18 -pix_fmt yuv420p -profile:v high -level 4.1` 与 AAC 192 kbps 转码，输出 SHA256 `5E5953AE7F239B08C5D733375782770C5E95EE6B4D73B3EF23CA350439DA80B9`。
- H.264复验仍在全局暂停后停留2x2，确认普通MP4的WmfMedia采样不能复用HAP暂停行为；移除Stage CG的World Pause并增加3秒首帧看门狗后，Editor Development构建、转场聚焦测试、项目校验、diff与预构建检查再次通过。实际首帧/末帧仍待PIE。
- 非暂停H.264复验仍停留2x2，且自动播放器选择日志没有证明进入Windows视频后端；Stage01To02单视频路径现显式指定 `WmfMedia`，仍复用旧转场的同一个Runtime MediaPlayer/MediaTexture/Widget，透明MOV继续使用Auto/HAP。
- Review 返工后启用并显式选择 `ElectraPlayer`；最新失败日志证明媒体时钟已从0推进到2.646秒，但直接 Slate MediaTexture 画刷仍停在2x2。候选因此统一改回旧转场已验证的动态材质显示链，并移除错误的3秒尺寸看门狗，待新一轮PIE确认实际画面与末尾完成事件。
- Review 修复候选的 Editor Development 构建成功并刷新精选预构建包；`ReEcho.UI.EncounterTransition.Policy` 1项和 `ReEcho.StageTransition` 3项均成功。`prebuilt_editor.py check` 与 `git diff --check` 通过；`validate_project.py` 首次因沙箱拒绝在执行 worktree 的 Content 下创建临时目录而失败，授权环境复跑后通过全部项目/XLSX同步校验。
- 用户复验发现 PIE 运行时黑屏、点击暂停后立刻出现内容；对应日志证明暂停瞬间 MediaTexture 从2x2变为1920x1080，并且每次 OnMediaOpened 后存在两次连续 `SetRate(1.000)`。引擎源码确认 UMediaPlayer 默认 `PlayOnOpen=true` 且在广播 OnMediaOpened 之后自动 Play；Widget 回调又执行 Seek(0)+Play，形成 Electra 双启动竞争。候选现关闭 PlayOnOpen，只保留回调中的单次从0启动。
- 关闭 PlayOnOpen 后 Editor Development 构建成功并刷新精选预构建包，`ReEcho.UI.EncounterTransition.Policy` 1项成功，预构建一致性与 `git diff --check` 通过；实际运行日志必须确认每次打开只出现一次 `SetRate(1.000)`，并由用户验收无需暂停即可连续显示。
- 单启动复验仍证明持续播放阶段表面保持2x2，而每次人工暂停后约10ms即提交1920x1080样本；候选据此改为普通MP4打开后保持0速率并Seek(0)，等 MediaTexture 获得首个真实尺寸样本后才单次Play，从而保证从0开始且首帧已可见。完成只认 OnEndReached；若预滚5秒无可见帧或实际播放时长超过媒体Duration加5秒仍无结束事件才fail-open。该候选 Editor Development构建、Transition Policy 1项、预构建一致性与diff检查通过，等待PIE。
- 首帧预滚复验后画面可见但停留首帧，Electra内部播放位置实际从0推进至15.138秒，证明剩余问题位于把普通H.264送入HAP透明材质的绘制路径。普通MP4现于预滚成功后直接由UImage绘制实时MediaTexture；透明MOV继续使用原HAP材质，两类媒体不再共享不兼容的像素解释路径。
- MP4直绘复验仍冻结首帧，排除UMG材质和画刷缓存后确认是UE5.8当前Electra/D3D12持续样本提交不兼容。项目从同一视频副本生成单个HAP1 MOV（454帧、1920x1080、30 FPS、15.13秒、PCM立体声），改用已由原转场验证的WmfMedia/HAP解码管线；这仍是视频CG而非序列帧。Transition Widget内普通HAP与透明HAP均显式选择WmfMedia，其他MediaPlayer和Electra插件配置保持不变。
- 扩大运行 `ReEcho.Run` 时，18项相邻测试通过，2项既有 EnemyShardDrops 测试因当前 CSV 数值与测试硬编码期望不一致失败（例如当前第一关 Ranged 为3..3，测试仍期望4..6）；失败与本候选未修改的掉落数据/测试有关，未纳入本任务修复。
- `git diff --check`、`prebuilt_editor.py check` 与授权环境复跑的 `validate_project.py` 均通过；首次静态校验失败仅为沙箱拒绝在执行 worktree 的 Content 下创建临时目录，并非项目数据错误。

### 剩余风险

- 项目副本已改为 UE/WmfMedia 稳定支持的 H.264/AAC，仍需 PIE 确认 MediaTexture 进入1920x1080、到达末帧并正常切入第二关；人工验收前不声称画面/声音完成。
- Plan130本地实现尚待人工验收/发布；Plan133执行必须保留其最新关末状态机，不能从远端main旧版本覆盖。

### 人工验收结果/请求

- `Passed`：用户确认CG画面与对应音乐正常，CG期间局间玩法和游戏音乐停止，CG完整结束后才进入第二关；并已二次确认发布。

### 架构文档审阅结果

- `MOD-ReEcho.md`：已记录第一关结算后的直接CG门及奖励页面跳过。
- `MOD-ReEchoUI.md`：已记录不透明 MediaTexture、MediaSound、Fill、层级和完成/失败生命周期。
- `ARCHITECTURE.md` / `README.md`：已审阅；没有新增 Runtime Module、AREA 或稳定索引，均无需修改。
