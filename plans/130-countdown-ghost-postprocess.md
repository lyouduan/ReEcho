# Plan 130 - 程序 - 关末倒计时重影模糊后处理

## 协调

- Planner 负责人：当前程序用户授权的 Planner（Codex）。
- Executor 负责人：当前程序用户授权的 Executor（Codex，同一任务分阶段执行）。
- Plan 编写方（AI 侧）：Gavyn-side AI（Codex）。
- 实现编写方（AI 侧）：Gavyn-side AI（Codex）。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@afed0e11e9b9f627ba79168e192cf3eb4f912068`。
- 本地实现方式（可选，仅作交接说明）：`codex/plan130-countdown-postprocess`，`C:\tmp\ReEcho-plan130-countdown-postprocess`。
- 依赖 / 阻塞：依赖 Plan124 已发布的权威 00 秒门、Hap Alpha 媒体链、Transition Screen 和 `OnEndReached` 后局 UI 门禁；视觉强度必须由用户 PIE 验收。
- Writes: 本 Plan；`Source/ReEcho/{Public,Private}/Presentation/Scene/ReEchoArenaCameraActor.*`；`Source/ReEcho/{Public,Private}/ReEchoGameMode.*`；`Source/ReEcho/{Public,Private}/UI/ReEchoEncounterTransitionWidget.*`；`Source/ReEcho/Private/Tests/ReEchoEncounterTransitionTests.cpp`；新增后处理材质及必要实例位于 `Content/ReEcho/Materials/PostProcess/EncounterTransition/**`；新增/维护 Unreal 作者ing 与资产审计脚本；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoUI.md`；必要的精选 Editor 预构建包。
- Stable Reads: `plans/124-encounter-card-transition.md`；`ReEchoEncounterDirector.*`；`ReEchoUIManagerSubsystem.*`；`ReEchoUIFlowCoordinatorSubsystem.*`；现有 `Content/ReEcho/UI/EncounterTransition/**` 与 `Content/Movies/EncounterTransition/EncounterTransitionAlpha.mov`。
- 影响模式：`SharedContract`，因为调整 GameMode 关末表现状态投影、相机 Post Process Blendable 和 Transition Widget 职责，但不改变 Encounter、Run、Cards、Shop 或 MediaPlayer 完成门。
- 兼容承诺 / 下游操作：Director 继续独占权威剩余时间；普通限时关 03→01 后处理从无到强、01→00 保持峰值，00 时先清除后处理并从媒体第 0 帧开始播放；只有媒体 `OnEndReached` 或明确失败才进入现有 TraitChoice/Shop 终点。HUD 保持 UMG 清晰，不被场景后处理模糊。
- 明确排除：不实现历史帧 RenderTarget/SceneViewExtension；不修改倒计时、波次、敌人、录制、卡牌、商店、Boss、死亡或媒体编码；不修改相机跟随/构图；不在 Editor 外手改 `.uasset`；不在用户 PIE 验收前推送实现。

## 锁定目标

把 Plan124 当前由 UMG 纯色 `Border` 模拟的 03→00 倒计时效果替换为真正作用于场景颜色的 Post Process Material。普通限时 Encounter 在剩余时间从 3 秒降到 1 秒时，方向性空间重影与轻度模糊平滑增强；1 秒到权威 0 秒保持峰值，HUD 继续清晰且战斗继续。权威时间到 0 后立即移除效果、冻结局间状态并从 `frame_0000` 播放现有透明转场，媒体结束后才进入抽卡或商店。

第一版采用当前帧 `PostProcessInput0` 的非对称多点采样，不保存历史帧：以较低成本形成稳定的时间拖影观感，避免新增 RenderTarget、SceneViewExtension、跨分辨率历史缓存和额外显存生命周期。若该观感经 PIE 证明不能满足美术目标，停止在人工验收点，不暗中扩大为历史帧系统。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho / AREA-Encounter`、`AREA-Presentation`、`AREA-UI`，以及文档入口 `MOD-ReEchoUI`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 和 `MOD-ReEchoUI.md`，两者均已加入 `Writes`。
- 设计意图：Gameplay 只把 Director 时间投影为规范化强度；`AReEchoArenaCameraActor` 独占场景后处理材质实例与 Blendable 生命周期；`UReEchoEncounterTransitionWidget` 回归只管理 00 后的媒体显示。避免让 UMG 伪装场景后处理，也不让渲染效果反向决定 Encounter 或 Run 状态。
- 权威状态与依赖：不改变时间或结算权威。新增相机表现状态只包含运行时 MID、当前强度和相位；相机消费 GameMode 命令，不回写玩法。Post Process Material 是 `ReEcho` 到引擎渲染资产的单向依赖，不新增 Runtime Module。
- 决策记录：
  - 使用 Post Process Domain 材质采样 `PostProcessInput0`，以中心样本和非对称偏移样本混合空间重影/模糊；不使用 MotionBlur Override，因为静止背景与正交相机下原生速度模糊不能稳定形成所需全屏重影。
  - 运行时创建 MID，并由 ArenaCamera 的 `PostProcessSettings.WeightedBlendables` 承载；效果常驻绑定但 `EffectStrength=0` 时视觉旁路，避免阈值附近反复增删 Blendable。
  - 参数至少包含 `EffectStrength`、`GhostOffsetPixels`、`BlurRadiusPixels`、`GhostOpacity`、`PulsePhase`；像素偏移按视口倒数换算 UV，避免分辨率改变效果尺度。
  - 03→01 使用 SmoothStep 强度曲线，01→00 保持 1，00 和所有重置路径强制清零。视觉调参不改变状态阈值。
  - Transition Widget 删除 `CountdownWash`、`CountdownPulse` 及叠色更新，只保留在 `RebuildWidget()` 的媒体绘制树、Fill、播放、结束/失败与淡出；不得回归到 `NativeConstruct()` 后建树。
- 相关文档同步范围：关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`modules/MOD-ReEcho.md`、`modules/MOD-ReEchoUI.md`；预计全局模块拓扑和稳定索引不变，前两者若无事实变化只在执行记录写明无需修改。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEcho.md`：记录 Director 强度投影、ArenaCamera 后处理所有权与 00 清理顺序；
  - `MOD-ReEchoUI.md`：记录 Transition Widget 不再拥有 03→00 叠色，只拥有媒体层；
  - `ARCHITECTURE.md` / `README.md`：待审阅并记录是否需要修改。

## 锁定验收

- [ ] 普通限时 Encounter 剩余时间大于 3 秒时 `EffectStrength=0`；3→1 秒按 SmoothStep 从 0 增至 1；1→0 秒保持 1；权威 0 秒在媒体启动前清零。
- [ ] 重影模糊由 Post Process Material 作用于场景颜色，不再由 UMG 纯色 Border 模拟；角色、敌人和场景产生可见空间重影，HUD/倒计时文字保持清晰。
- [ ] 后处理使用按像素定义的非对称多点采样，在 16:9、超宽和窗口缩放时强度尺度稳定；无全黑、默认材质、UV 越界闪烁或明显亮度累积。
- [ ] 每个普通限时关均生效；Boss、死亡、非计时结束不生效。下一 Encounter、重启、主菜单、travel、媒体失败和异常关闭均幂等清零，不残留 Blendable 强度。
- [ ] 权威 00 后才从 `frame_0000` 播放现有 Hap Alpha 动画；未收到 `OnEndReached` 且无明确失败时不得进入 TraitChoice/Shop，现有 Fill、透明和淡出表现不回归。
- [ ] 后处理资产由 Unreal 作者ing 保存、重载、编译和参数审计；Windows Cook 收集该资产，Shipping 包可启动。
- [ ] `.clang-format`、Editor Development 构建、聚焦自动化、项目校验、`git diff --check`、最终 `-FullRebuild` 和精选预构建检查通过。
- [ ] 用户在 PIE 验收重影方向、强度、模糊程度、HUD 可读性、00 切换和媒体衔接；验收前不关闭或发布实现。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@afed0e11e9b9f627ba79168e192cf3eb4f912068`；远端最大编号为 Plan129，本 Plan 使用 Plan130。
- 引擎/构建可用性：Plan124 已在 UE 5.8 完成 Editor/Shipping/媒体与人工视觉验证；本 Plan 不复用其构建结果作为新候选证据。
- 现有聚焦测试结果：规划阶段未运行新候选测试；先记录 `ReEcho.UI.EncounterTransition` 基线，再修改实现。
- 共享契约 / 难合并资源风险：远端 `e44caaea..afed0e11` 修改了 `ReEchoGameMode.*` 的商店/免费卡路径及 `MOD-ReEcho*` 文档，但没有修改关末转场函数、Transition Widget 或 ArenaCamera。当前判定为同文件不同语义区域、可组合；发布前仍须重新审计。
- 基线损坏时的停止条件：Plan130 被远端占用；最新 main 改写关末状态机或相机所有权；材质必须依赖历史帧才能满足用户要求；作者ing 无法在 UE 5.8 生成可编译 Post Process 材质；实现需要改变权威时序或媒体完成门。

## 实现提纲

1. 发布并核验 Plan130；读取 Executor 规则、`LESSONS` 的相机/材质与资产 Cook 小节，以及准确最新的相机、GameMode、Transition Widget 配对实现。
2. 新增 Unreal 作者ing/审计脚本，创建 Post Process Domain 基础材质，连接中心与非对称多点 `PostProcessInput0` 采样并暴露运行时参数；保存、重载、编译和校验资产。
3. 扩展 `AReEchoArenaCameraActor`：创建运行时 MID、加入 Camera Weighted Blendables、更新强度/像素偏移/模糊/相位，并提供幂等 Reset；相机重新 Configure 不重复加入实例。
4. 将 GameMode 03→00 投影改为调用 ArenaCamera；在 00 媒体启动前以及所有离开/失败/重置路径清零。保持 `OnEndReached`/失败完成门和 Run/Shop 逻辑不变。
5. 从 Transition Widget 移除 `CountdownWash`、`CountdownPulse` 与强度接口，只保留预先构建的媒体 Slate 树和现有透明播放链。
6. 扩展聚焦自动化与资产审计，验证强度曲线、适用资格、00 清理、幂等 Reset、参数/材质存在性和媒体完成门。
7. 格式化并执行增量 Editor 构建、聚焦自动化、资产加载/编译、项目校验与 `git diff --check`；更新模块文档和 Plan 执行记录。
8. 用户 PIE 验收后才关闭 Plan；随后在最新 main 组合上执行 `-FullRebuild`、精选预构建检查、必要 Shipping Cook/烟测和发布锁流程。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Plan-only | `python scripts/validate_project.py`、`git diff --check` | Plan130 编号、结构、范围和中文正文通过 |
| 材质资产 | Unreal 作者ing脚本 create/check、资产重载与 Material 编译 | Post Process Domain、Blendable Location、参数和表达式有效 |
| 状态机 | `ReEcho.UI.EncounterTransition` 聚焦自动化 | 03→01 SmoothStep、01→00 保持、00 清零、Boss/死亡排除、媒体门通过 |
| C++ | `.clang-format`、`scripts\ue\Build-Editor.cmd -Configuration Development` | UHT/UBT 成功并刷新匹配预构建包 |
| 回归 | `ReEcho.Encounter`、`ReEcho.StageTransition` 与相关 UI 聚焦集合 | 关末冻结、下一 UI、Boss/死亡和下一关无回归 |
| 静态 | `python scripts/validate_project.py`、`git diff --check`、预构建检查 | 项目和发布候选一致 |
| Cook | Windows Shipping Cook/Package、manifest 和 10 秒烟测 | 后处理材质与 Hap Alpha 媒体进入包且成品可启动 |
| 人工 PIE | GM/自然倒计时、多分辨率与连续关卡 | 用户确认重影模糊观感、HUD 清晰、00 无残留及媒体衔接 |

## 执行记录

### 变化

- 2026-08-27：完成最新远端审计和方案锁定；确认现有效果仍是 Transition Widget 的两个透明 Border，不是真正后处理。Plan130 从 `origin/main@afed0e11` 建立，等待 Plan-only 发布后进入实现。
- 2026-08-27：Plan-only 提交 `1812dfb1` 已发布并核验 `origin/main`；在独立 worktree 完成第一版候选。新增 After Tonemapping Post Process Material，以 `PostProcessInput0` 中心、前向两层、反向一层和垂直模糊两层构成非对称空间重影；ArenaCamera 创建运行时 MID 并按 Director 时间更新强度、像素偏移、模糊、透明混合与相位。
- 2026-08-27：GameMode 的 `CountdownPostProcess` 阶段不再提前创建 Transition Screen，而是只更新 ArenaCamera；权威 00 秒在创建/播放媒体前清零相机效果。Transition Widget 删除两个倒计时 Border 和强度接口，保留 `RebuildWidget()` 前构建的媒体 Slate 树、Fill、Hap Alpha 播放、完成/失败和淡出。
- 2026-08-27：作者ing首次因 UE Python `CustomInput` 不接受构造参数而失败，日志门捕获到命令行进程退出 0 下的 `LogPython Error`；改为构造结构体后设置 `input_name`，材质创建、保存、重载和第二次幂等检查通过。

### 证据

- `e44caaea..afed0e11` 的传入范围只在 `ReEchoGameMode.*` 商店/免费卡区域与模块文档产生同文件耦合，没有修改 `UpdateEncounterTransitionPresentation()`、`ReEchoEncounterTransitionWidget.*` 或 `ReEchoArenaCameraActor.*`。
- Editor Development 增量构建通过：首轮 99 actions，补充 SmoothStep 中间点断言后第二轮 4 actions；精选预构建检查通过，`build_id=55116800`、源码指纹 `50e4d3960b40`。
- `author_plan130_countdown_postprocess.py` 连续两次通过：资产为 Post Process Domain、After Tonemapping，材质编译无错误，标量参数为 `BlurRadiusPixels/EffectStrength/GhostOffsetPixels/GhostOpacity/PulsePhase`。
- 自动化通过：`ReEcho.UI.EncounterTransition` 1/1、`ReEcho.Encounter` 4/4、`ReEcho.StageTransition` 3/3，三次日志均为 `TEST COMPLETE. EXIT CODE: 0`。
- `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check` 与 `git diff --check` 通过。

### 剩余风险

- 空间多点采样能形成稳定拖影，但不是真实历史帧残留；是否满足视觉目标必须由 PIE 验收。
- 后处理发生在 UMG 合成前，因此 HUD 保持清晰；若后续要求 HUD 一并重影将构成产品与实现范围变化。
- 尚未执行最终 `-FullRebuild` 或 Windows Shipping Cook/烟测；这些是用户视觉验收后的最终发布候选门禁，不以当前增量构建代替。

### 人工验收结果/请求

- 2026-08-27：为保留进入 03 秒阈值前的一秒观察窗口，调试入口由 `GMTransition3` 改为 `GMTransition4`，并将 Director 权威剩余时间推进到 4 秒；生产路径的 3 秒后处理阈值不变。
- 2026-08-27：首次 PIE 验收反馈后处理无可见效果。日志确认 `GMTransition4` 从权威 4 秒运行到 0；新增相机 MID 创建、Blendable 绑定和首次非零强度日志。旧材质原地重建触发 UE 5.8 `DeleteAllMaterialExpressions` 的 `!IsRooted()` 编辑器断言且未写盘，因此保留旧资产，安全创建 `M_PP_EncounterCountdownGhost_V2`；V2 使用更大的像素偏移/模糊和 RGB 分离重影，并由 ArenaCamera 改为加载 V2。
- 2026-08-27：按验收反馈增加最终画面定格。权威倒计时仍正常到 0；媒体成功启动后将后处理锁定峰值并暂停 World，GameMode 允许暂停时继续轮询媒体。媒体完成/失败时只恢复本流程主动设置的暂停，再清除后处理并进入抽卡或商店；已有外部暂停不会被误恢复。
- 2026-08-27：按后续验收将后处理清除点从媒体完成后移到整条 CardChoice 链结束。媒体完成时只恢复 World，峰值后处理继续作为抽卡背景；Run 仍为 `CardChoice` 时跨连续/奖励抽卡保留，最终卡牌提交并返回/进入商店时清除。无 CardChoice 的直接商店路径在打开商店前清除。
- 2026-08-27：确认采用中心清晰、四周渐进模糊方案。保留 V2，新增 `M_PP_EncounterCountdownGhost_V3`；按视口宽高比计算椭圆径向距离，通过 `ClearCenterRadius=0.28`、`EdgeBlurRadius=0.62`、`EdgeMaskPower=1.4` 形成平滑边缘遮罩，只将重影/模糊结果混合到外围，中心原始场景色保持清晰。
- 2026-08-27：确认删除无引用的初版与 V2 材质，只保留当时运行中的 V3；随后新增 V4。`ArenaCameraActor` 在同一 `Arena Camera|Encounter Countdown Post Process` Details 分类暴露中心清晰半径、边缘过渡半径、遮罩曲线、重影强度、RGB 分离勾选与分离强度；V4 增加 `RGBSeparationEnabled/RGBSeparationStrength` 参数，关闭分离时仍保留无色偏重影。
- 2026-08-27：V4 完成材质编译、C++ 构建与转场自动化后，复查确认运行时代码、作者脚本和模块文档均不再引用 V3；经用户再次确认删除 V3。`EncounterTransition` 后处理目录最终只保留 `M_PP_EncounterCountdownGhost_V4.uasset`。
- `PendingBeforeClose`：请在 `C:\tmp\ReEcho-plan130-countdown-postprocess\ReEcho.uproject` 进入普通 Encounter，执行 `GMTransition4`，验收 04→03 前后处理启用边界、03→01 重影方向/模糊强度、01→00 峰值、HUD 可读性、00 清除及透明媒体衔接。验收前不关闭或推送实现。

### 架构文档审阅结果

- `MOD-ReEcho.md` 已更新：记录 Director 只投影强度、ArenaCamera 独占后处理 MID/Blendable 生命周期、00 先清除再播放 Hap Alpha 媒体。
- `MOD-ReEchoUI.md` 已更新：Transition Widget 不再拥有倒计时 Border/后处理状态，只管理 00 后媒体层；阅读路线加入 ArenaCamera 与后处理材质。
- `ARCHITECTURE.md` 已审阅、无需修改：仍只有既有 `ReEcho` Runtime Module，未新增模块或改变依赖拓扑。
- `README.md` 已审阅、无需修改：`MOD-ReEcho`、`AREA-Encounter`、`AREA-Presentation` 与 UI 稳定路由均未新增标识。
