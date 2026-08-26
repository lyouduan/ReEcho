# Plan 121 - 程序 - 四种怪物出生动画资产接入

## 协调

- Planner 负责人：当前程序 Planner。
- Executor 负责人：待分配独立 Executor。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Unassigned`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@1544d98831c7015eb199e3f19725ea16fd54b326`。
- 本地实现方式（可选，仅作交接说明）：按当前对话确认使用本任务专属 worktree；不得在主工作区直接实现。
- 依赖 / 阻塞：源序列位于 `F:/MiniGame/兔子出生/`、`F:/MiniGame/史莱姆出生/`、`F:/MiniGame/好羊出生序列/`、`F:/MiniGame/好狐狸出生序列/`；执行机必须可读取这些目录并可独占运行 Unreal Editor/Commandlet。
- Writes: `Source/ReEchoPresentation/{Public,Private}/Presentation/Animation2D/ReEcho2DAnimationTags.{h,cpp}`、`Source/ReEcho/{Public,Private}/Presentation/Enemy/ReEchoEnemyPresentationComponent.{h,cpp}`、`Source/ReEcho/Private/Graybox/ReEchoEnemyActor.cpp`、`Source/ReEcho/Private/Tests/ReEcho2DAnimationTests.cpp`、`Content/ReEcho/DataAsset/Common/Animation2D/SM2D_DefaultCharacter.uasset`、`Content/ReEcho/Art/Animation2D/Enemies/{Rabbit,Slime,Goat,Fox}/Born/**`、`Content/ReEcho/Art/Animation2D/Enemies/{Rabbit,Slime,Goat,Fox}/Flipbooks/*Born*.uasset`、`Content/ReEcho/DataAsset/Enemy/Profiles/{DA_Enemy_RabbitDoll,DA_Enemy_Slime,DA_Enemy_GoatPriest,DA_Enemy_Fox}.uasset`、`shared/CODEBASE_MAP/modules/{MOD-ReEchoPresentation,MOD-ReEcho}.md`、本 Plan 执行记录。
- Stable Reads: `plans/82-animation2d-asset-cleanup.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`、`Source/ReEchoPresentation/{Public,Private}/Presentation/Animation2D/**`、四种怪物现有动画目录与 Profile、`scripts/ue/audit_plan82_animation_assets.py`、相邻导入/修复脚本。
- 影响模式：`Exclusive`。
- 兼容承诺 / 下游操作：新增 `Animation.Born` 为可选、非玩法裁决的公共表现语义；配置成功的怪物在生成装配完成后尝试播放，缺少 Clip 或播放失败必须安全 no-op，不得回滚、延迟或阻止出生提交。不改变出生位置、预警、生成提交、AI、碰撞、伤害、死亡或存档。保留源 PNG，不覆盖四种怪物的其他动画与 DA 字段。
- 明确排除：`BadRabbit/BadSlime/BadGoat/BadFox`、出生预警 VFX、数值表、地图、音频、玩家/Echo 的 Born 绑定及其他角色资产；不让动画完成回调控制怪物可伤害、碰撞、AI 激活或任何玩法状态。

## 锁定目标

把用户提供的四组顺序 PNG 按自然数字顺序导入为 Paper2D Texture/Sprite，并分别组装为兔子、史莱姆、好羊、好狐狸的非循环 Born Flipbook。新增原生 `Animation.Born` 公共表现语义和共享状态机状态；每种怪物的帧资产位于其 `Born` 目录，生成的 Born Flipbook 位于该怪物现有 `Flipbooks` 目录，对应 Enemy Presentation Profile 的 Born Clip 明确引用该 Flipbook。生产怪物完成 Definition/Profile 装配后立即尝试播放 Born；动画缺失或播放失败不改变怪物已经提交的玩法状态。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoPresentation`、`MOD-ReEcho` / `AREA-Presentation`、`AREA-Enemies`；Presentation 注册和播放公共 Born 语义，主模块 Enemy Host Adapter 在生成装配边界发出表现请求。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 必须加入 Writes，记录公共语义、完成归宿与 Enemy Host 触发边界。
- 设计意图：保持“玩法只请求语义、Profile 决定具体表现”的边界；出生动画完成与否不得反馈或延迟怪物玩法生成。
- 权威状态与依赖：Presentation 模块新增原生 GameplayTag 和 FSM 状态，但仍独占表现播放；ReEcho 主模块只发出请求，不等待完成、不把结果反馈给玩法。模块依赖方向不变。
- 决策记录：`兔子/史莱姆/好羊/好狐狸` 映射为 `Rabbit/Slime/Goat/Fox`，不是 `Bad*` 目录；兔子 6 帧，其余各 5 帧；已通过 Unreal 回读证明 Rabbit 现有 Born 惯例为 4 FPS、Center Pivot、PPU 1、Translucent，四组统一沿用。2026-08-26 用户在获知 C++/公共契约/FullRebuild 风险后明确确认扩展 Plan，拒绝仅交付未绑定资产的降级方案。
- 相关文档同步范围：更新 `MOD-ReEchoPresentation.md` 与 `MOD-ReEcho.md`；`ARCHITECTURE.md` 和 `README.md` 关闭前审阅，预计无需修改，因为拓扑和稳定路由不变。
- 关闭前逐项填写审阅结果：待 Executor/Planner 根据最终候选补充。

## 锁定验收

- [ ] `Rabbit/Slime/Goat/Fox` 各自存在 `Born` 目录，源 PNG 对应的 Texture 与 Sprite 数量分别为 6/5/5/5，命名稳定且帧序正确。
- [ ] 四个非循环 Born Flipbook 位于各自 `Flipbooks` 目录，逐帧引用对应 Sprite，不跨怪物借用资源。
- [ ] 四个生产 Enemy Profile 的 `Animation.Born` Clip 分别引用本怪物 Born Flipbook；资产保存后重新加载仍保持引用，缺失资源不会影响玩法生成。
- [ ] `Animation.Born` 以原生 GameplayTag 注册并存在于共享 FSM；它是可抢占的非循环瞬时表现，完成后回到 Move，Death 仍保持最高优先级/终结独占。
- [ ] 新怪物完成 `ConfigureFromDefinition` 的 Profile 装配后尝试一次 Born；缺少 Born 的 Profile 安全 no-op，生成成功、碰撞、可伤害、AI/Encounter 提交均不依赖播放结果。
- [ ] 不修改 `Bad*`、其他动画、出生预警/生成时序、Schema、表格或无关 DA 字段；C++ 差异仅限 Writes 中的 Born 公共语义与 Enemy Host 表现触发。
- [ ] `audit_plan82_animation_assets.py`（必要时以只读扩展检查覆盖新 Born）、`python scripts/validate_project.py`、资产加载检查和 `git diff --check` 通过。
- [ ] 用户在 PIE 中观察四种怪物的出生动作：帧序、速度、尺寸、脚点、朝向、首帧闪烁、结束后回到基础循环均可接受。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@1544d98831c7015eb199e3f19725ea16fd54b326`。
- 引擎/构建可用性：执行前检查 Git-common-dir Unreal 锁与 `UnrealEditor` 进程；任何需要关闭 Editor 的命令先请用户保存并关闭。
- 现有聚焦测试结果：Executor 的 UE 回读证明当前基线未注册 `Animation.Born`、四个生产 Profile 均无 Born Clip；Rabbit 旧 Born 为 4 FPS、6 帧、Center Pivot、PPU 1、Translucent。该事实触发原 Plan 停止条件，用户已确认扩展。
- 共享契约 / 难合并资源风险：四个 Profile 与 Paper2D `.uasset` 为二进制独占写面；发布前必须对远端同路径变化做物理及逻辑审计，禁止静默覆盖。
- 基线损坏时的停止条件：源帧不可读/尺寸或透明通道异常、Controller/FSM 无法在不改变玩法状态的情况下实现 Born 完成归宿、Editor/锁被占用、或远端修改同一 C++/Profile/目标目录时停止并报告。

## 实现提纲

1. 注册原生 `Animation.Born`，在共享 FSM 增加非终结 Born 状态并定义完成回到 Move、低于 Death 的优先级；增加 Controller 单元测试覆盖播放、完成归宿、缺失 Clip no-op 与 Death 抢占。
2. 在 Enemy Presentation Host Adapter 暴露一次性 Born 请求，并在 `ConfigureFromDefinition` 完成 Profile 装配后调用；不得以播放结果控制函数返回值、碰撞、受伤、AI 或 Encounter 提交。
3. 在专属 Executor worktree 回读四个 Profile、现有 Rabbit/Slime Born 资产、同类 Flipbook 的循环/帧率/材质/像素密度与导入设置；冻结准确替换和新增清单。
4. 通过 Unreal Editor Python/仓库脚本复制源 PNG 并导入 Texture，按项目 Paper2D 惯例创建逐帧 Sprite；不得在 Editor 外生成或手改 `.uasset`。
5. 在四个怪物的 `Flipbooks` 目录创建 4 FPS 非循环 Born Flipbook，按自然数字顺序装帧；只修改四个生产 Profile 的 Born Clip 绑定，保存重载并回读其余字段。
6. 更新两份模块文档和执行记录，运行验证矩阵并交由 Planner 审查源码/二进制范围；PIE 人工验收通过前保持 `Review`。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 源输入 | 读取四组 PNG 的数量、自然顺序、尺寸、RGBA/透明通道 | 分别为 6/5/5/5，输入无缺帧且可解码 |
| 资产结构 | Unreal Python/Asset Registry 回读 Texture、Sprite、Flipbook、Profile | 数量、路径、帧序、非循环与四个 Born 引用准确，保存重载后一致 |
| 聚焦审计 | `python scripts/ue/audit_plan82_animation_assets.py`（必要时使用 Plan 内新增只读审计） | 四个生产 Profile 与源贴图导入链通过 |
| C++ 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.Animation2D` | Born tag/状态、完成归宿、缺失 Clip no-op、Death 抢占与既有表现测试通过 |
| 完整构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新匹配最终源码的精选预构建包 |
| 项目静态 | `python scripts/validate_project.py` | 项目/资产规则通过 |
| 差异卫生 | `git status --short`、精确路径清单、`git diff --check` | 仅 Plan Writes 与获准的导入资产，无机器产物/意外删除 |
| 人工 PIE | 逐个生成 Rabbit/Slime/Goat/Fox | 出生动作视觉可接受并正常回到基础循环 |

## 执行记录

### 变化

- 待 Executor 填写。

### 证据

- Planner 已确认四组源序列可读取，帧数为 6/5/5/5；尚未导入或修改 UE 内容资产。

### 剩余风险

- 主体脚点与最终视觉节奏必须通过 PIE 人工验收确认；公共语义扩展需要最终 FullRebuild 与聚焦自动化证明未破坏既有状态优先级。

### 人工验收结果/请求

- `PendingBeforeClose`：四种怪物的出生动画视觉与结束过渡。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`：需更新 Born 原生语义、FSM 完成归宿与测试边界。
- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：需更新 Enemy Host 在 Definition/Profile 装配后发出 Born 表现请求且不参与玩法裁决。
- `shared/CODEBASE_MAP/ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md`：关闭前由 Planner 复审。
