# Plan 133 - 程序 - 第1至第2关过渡CG与商店跳过

## 协调

- Planner 负责人：当前程序用户授权的 Planner（Codex）。
- Executor 负责人：当前程序用户授权的 Executor（Codex，同一任务分阶段执行）。
- Plan 编写方（AI 侧）：Gavyn-side AI（Codex）。
- 实现编写方（AI 侧）：Gavyn-side AI（Codex）。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@89ac0166ff3bd60588d92fa1f696eacb75249457`；实现将组合用户尚待验收的 Plan130 本地候选，组合前必须审计同文件变化。
- 本地实现方式（可选，仅作交接说明）：规划 worktree `C:\tmp\ReEcho-plan133-stage1-to2-cg-plan`；执行使用独立 Plan133 worktree，不与 Plan130 worktree 共用目录。
- 依赖 / 阻塞：依赖 Plan124/Plan130 的普通关末媒体完成门和 CardChoice 链；源视频 `F:\MiniGame\过渡cg\video(66).mp4` 存在，约 15 秒、15,723,153 bytes，原文件只读保留。Plan130 人工验收结论仍独立存在，不由本 Plan 代替。
- Writes: 本 Plan；`Source/ReEcho/{Public,Private}/ReEchoGameMode.*`；`Source/ReEcho/{Public,Private}/UI/ReEchoEncounterTransitionWidget.*` 或新增窄 CG Widget；`Source/ReEcho/Private/Tests/ReEchoEncounterTransitionTests.cpp`、必要 Run/Stage 测试；`Content/Movies/EncounterTransition/Stage01To02.*`；对应 `Content/ReEcho/UI/EncounterTransition/**` MediaSource/材质资产；必要 Unreal 作者ing/审计脚本；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoUI.md`；精选 Editor 预构建包。
- Stable Reads: `plans/124-encounter-card-transition.md`、`plans/130-countdown-ghost-postprocess.md`；`ReEchoRunSubsystem.*`、`ReEchoStageTransition.*`、`ReEchoUIFlowCoordinatorSubsystem.*`；现有 `EncounterTransitionAlpha.mov` 和 Transition Screen 生命周期。
- 影响模式：`SharedContract`，因为改变 Encounter 1 的 CardChoice 完成终点、跳过一次 Shop，并增加媒体完成门；不改变 Run 的通用 Phase/交易/存档结构。
- 兼容承诺 / 下游操作：仅当前完成关次 `EncounterIndex==1` 时跳过战后商店；免费 CardChoice 仍按配表发放且必须完整提交。顺序锁定为“现有权威00转场 → 免费抽卡（若配置）→ Stage01To02 CG → BeginEncounter 2”。CG 必须从0播放到末帧；打开或播放失败 fail-open 进入第2关。Encounter 2及以后保持原 CardChoice/Shop 流程。
- 明确排除：不删除通用商店系统、商店数据或其他关次商品；不跳过第1关免费抽卡；不改战斗计时、敌人、Stage连续性、存档Schema、Plan130后处理参数或现有透明关末动画；不覆盖源视频；不在用户PIE验收前发布实现。

## 锁定目标

第1关完成后保留现有权威00秒转场与免费抽卡奖励。第1关最后一张免费卡提交后不再打开战后商店，而是全屏 Fill 播放 `video(66).mp4` 对应的项目内过渡 CG；CG 到达末帧后才开始第2关。若第1关没有配置免费卡，则现有00秒转场完成后直接播放CG。CG打开/解码/播放失败时记录明确错误并幂等进入第2关，不能卡死。

“删除1-2关卡商店流程”只表示删除这一次页面导航和等待，不删除商店代码、资产、报价、货币或其他关次行为。第2关结束后的战后商店及后续所有商店照常出现。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho / AREA-Encounter`、`AREA-Run`、`AREA-UI`，以及文档入口 `MOD-ReEchoUI`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `MOD-ReEchoUI.md`，两者均加入 `Writes`。
- 设计意图：GameMode 继续拥有页面/关末编排；Run 继续拥有 Phase、CardChoice提交、商店交易与EncounterIndex。新增CG是GameMode消费当前关次和媒体完成状态的表现门，不把视频播放状态写入Run或存档。
- 权威状态与依赖：`EncounterIndex` 和 `Phase` 仍来自 `UReEchoRunSubsystem`；CG Widget只报告 `Finished/Failed`。`BeginNextEncounter()`仍是进入第2关的唯一现有端点。跳过商店不得伪造Shop Phase或调用购买接口。
- 决策记录：
  - CG放在第1关免费抽卡完成之后，使它成为进入第2关前最后一个全屏阶段；若无免费卡则直接进入CG。
  - 复用现有 Transition Screen 的ZOrder/Fill和动态MediaPlayer生命周期，但必须为普通不透明MP4提供独立播放配置，不能让Hap Alpha材质错误解释普通视频。
  - 项目内媒体使用语义化文件名 `Stage01To02`，不沿用 `video(66)`；源文件只读复制。若UE媒体探测证明编码不兼容，则用可追溯脚本生成兼容MP4，保留分辨率/帧率/音频并记录转换参数。
  - CG结束门只认 `OnEndReached` 或明确失败；不使用固定15秒Timer代替媒体状态。
  - 第1关卡牌提交后若Run仍处于连续/奖励 `CardChoice`，继续抽卡；只有CardChoice链真正结束才启动CG。
- 相关文档同步范围：关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`modules/MOD-ReEcho.md`、`modules/MOD-ReEchoUI.md`；预计模块拓扑和稳定索引不变，前两者若无事实变化只记录“已审阅、无需修改”。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEcho.md`：记录Encounter 1的CardChoice后CG门和一次性Shop跳过；
  - `MOD-ReEchoUI.md`：记录普通不透明CG的Fill、层级、播放/失败生命周期；
  - `ARCHITECTURE.md` / `README.md`：审阅拓扑和索引是否变化。

## 锁定验收

- [ ] 第1关自然结束或GM预览均保持现有00秒透明转场；若有免费卡，先完成全部CardChoice；随后不打开战后商店而播放Stage01To02 CG。
- [ ] CG在视口最上层按Fill等比裁切、从0开始、不循环；声音按源视频播放；到达末帧前不得开始第2关。
- [ ] CG结束后关闭媒体层、恢复Gameplay输入并通过现有 `BeginNextEncounter()` 开始Encounter 2；无重复开始、黑屏、残留Widget或暂停状态。
- [ ] 第1关CG打开/解码/播放失败时记录明确日志并fail-open进入Encounter 2；失败终点幂等。
- [ ] 第1关不创建/聚焦战后商店，不生成等待玩家关闭商店的页面流程；免费卡奖励、保存和卡牌提交仍完整。
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
- 基线损坏时的停止条件：Plan133编号被远端占用；Plan130验收导致关末媒体状态机重做；源视频UE无法解码且环境无可审计转码工具；跳过商店必须修改Run Phase/Save Schema；CardChoice与CG先后顺序被用户改变。

## 实现提纲

1. 发布并核验Plan133；读取Executor规则、相关LESSONS小节及Plan130最终候选，审计其与最新main的物理/逻辑耦合。
2. 在独立Plan133执行worktree组合Plan130候选；复制源视频到语义化Movies路径，使用UE媒体工具探测轨道、时长、分辨率、帧率与音频；必要时只生成项目内兼容副本。
3. 扩展/新增全屏CG播放宿主：普通MP4使用不透明MediaTexture路径、Fill布局、从0播放、OnEndReached/Failed和幂等Reset；保留现有Hap Alpha路径。
4. 在GameMode建立窄的Stage01To02完成门：仅当前完成关次1，等待CardChoice链结束后跳过 `ShowPostTraitShop()`，转为CG；结束/失败统一调用现有下一关入口。
5. 增加纯策略和Widget聚焦测试，覆盖关次资格、连续CardChoice、直接无卡、CG完成/失败、一次性Shop跳过、后续关次不变。
6. 更新模块文档与Plan执行记录；格式化并执行增量构建、聚焦自动化、媒体资产审计、项目校验和diff检查。
7. 用户PIE验收后才关闭；随后在最新main组合上执行FullRebuild、预构建检查、必要Shipping Cook/烟测和发布锁流程。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Plan-only | `python scripts/validate_project.py`、`git diff --check` | Plan133编号、结构、范围和中文正文通过 |
| 源媒体 | 文件哈希/大小只读检查、UE媒体轨道审计 | 原文件未改；项目内CG可解码，轨道/时长/尺寸/帧率/音频有日志 |
| Widget | `ReEcho.UI.EncounterTransition`及新增CG聚焦测试 | Alpha与Opaque两种模式、Fill、0秒、完成/失败和Reset通过 |
| 状态机 | `ReEcho.Encounter`、新增Stage01To02策略测试 | CardChoice后CG、一次性Shop跳过、完成/失败进入2、后续关不变 |
| Stage回归 | `ReEcho.StageTransition`、相关Run/Card/Shop测试 | 同Stage连续性、奖励提交、存档和其他商店无回归 |
| C++ | `.clang-format`、`scripts\ue\Build-Editor.cmd -Configuration Development` | UHT/UBT成功并刷新匹配预构建包 |
| 静态 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 项目和候选一致 |
| Cook | Windows Shipping Cook/Package、manifest和烟测 | CG与MediaSource进入包且成品可启动 |
| 人工PIE | 第1关自然/GM结束、带卡/无卡、正常/失败媒体 | 用户确认内容/声音/Fill、无商店、CG后第2关正常 |

## 执行记录

### 变化

### 证据

### 剩余风险

- Windows Shell只确认约15秒时长与文件大小；编码、分辨率、帧率、音轨和UE解码尚待执行阶段审计。
- Plan130本地实现尚待人工验收/发布；Plan133执行必须保留其最新关末状态机，不能从远端main旧版本覆盖。

### 人工验收结果/请求

- `PendingBeforeClose`：实现后由用户验收第1关抽卡完成→CG→第2关，确认不出现商店。

### 架构文档审阅结果

- 待实现后逐项填写。
