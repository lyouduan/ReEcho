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
- Writes: `Content/ReEcho/Art/Animation2D/Enemies/{Rabbit,Slime,Goat,Fox}/Born/**`、`Content/ReEcho/Art/Animation2D/Enemies/{Rabbit,Slime,Goat,Fox}/Flipbooks/*Born*.uasset`、`Content/ReEcho/DataAsset/Enemy/Profiles/{DA_Enemy_RabbitDoll,DA_Enemy_Slime,DA_Enemy_GoatPriest,DA_Enemy_Fox}.uasset`、本 Plan 执行记录。
- Stable Reads: `plans/82-animation2d-asset-cleanup.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`、`Source/ReEchoPresentation/{Public,Private}/Presentation/Animation2D/**`、四种怪物现有动画目录与 Profile、`scripts/ue/audit_plan82_animation_assets.py`、相邻导入/修复脚本。
- 影响模式：`Exclusive`。
- 兼容承诺 / 下游操作：保留 `Animation.Born` 可选、非玩法裁决的既有契约；不改变出生位置、预警、生成提交、AI、碰撞、伤害、死亡或存档。保留源 PNG，不覆盖四种怪物的其他动画与 DA 字段。
- 明确排除：`BadRabbit/BadSlime/BadGoat/BadFox`、出生预警 VFX、Runtime Module/C++/GameplayTag/FSM 公共契约、数值表、地图、音频及其他角色资产。

## 锁定目标

把用户提供的四组顺序 PNG 按自然数字顺序导入为 Paper2D Texture/Sprite，并分别组装为兔子、史莱姆、好羊、好狐狸的非循环 Born Flipbook。每种怪物的帧资产位于其 `Born` 目录，生成的 Born Flipbook 位于该怪物现有 `Flipbooks` 目录，对应 Enemy Presentation Profile 的 `Animation.Born` Clip 明确引用该 Flipbook，使生产生成流程请求 Born 语义时播放本次交付动画。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoPresentation`；仅更新该模块消费的内容资产，不修改 Runtime Module。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md` 已审阅；现有文档已声明 Born 等语义为可选能力、Profile/Flipbook 为相关资产，本任务不改变该事实，因此不写正文、不加入 Writes。
- 设计意图：保持“玩法只请求语义、Profile 决定具体表现”的边界；出生动画完成与否不得反馈或延迟怪物玩法生成。
- 权威状态与依赖：不改变状态所有者、公共 API、GameplayTag、模块依赖或目录职责；只补齐四个生产 Profile 的内容绑定。
- 决策记录：`兔子/史莱姆/好羊/好狐狸` 映射为 `Rabbit/Slime/Goat/Fox`，不是 `Bad*` 目录；兔子 6 帧，其余各 5 帧；Flipbook 非循环，帧率沿用现有 Born/Profile 导入惯例，若仓库无可证明惯例则在资产写入前由用户确认，不自行发明播放速度。
- 相关文档同步范围：`ARCHITECTURE.md`、`README.md` 与 `MOD-ReEchoPresentation.md` 关闭前逐项审阅；预计均无需正文修改，因为拓扑、路由和公共契约不变。
- 关闭前逐项填写审阅结果：待 Executor/Planner 根据最终候选补充。

## 锁定验收

- [ ] `Rabbit/Slime/Goat/Fox` 各自存在 `Born` 目录，源 PNG 对应的 Texture 与 Sprite 数量分别为 6/5/5/5，命名稳定且帧序正确。
- [ ] 四个非循环 Born Flipbook 位于各自 `Flipbooks` 目录，逐帧引用对应 Sprite，不跨怪物借用资源。
- [ ] 四个生产 Enemy Profile 的 `Animation.Born` Clip 分别引用本怪物 Born Flipbook；资产保存后重新加载仍保持引用，缺失资源不会影响玩法生成。
- [ ] 不修改 `Bad*`、其他动画、出生预警/生成时序、C++、Schema、表格或无关 DA 字段。
- [ ] `audit_plan82_animation_assets.py`（必要时以只读扩展检查覆盖新 Born）、`python scripts/validate_project.py`、资产加载检查和 `git diff --check` 通过。
- [ ] 用户在 PIE 中观察四种怪物的出生动作：帧序、速度、尺寸、脚点、朝向、首帧闪烁、结束后回到基础循环均可接受。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@1544d98831c7015eb199e3f19725ea16fd54b326`。
- 引擎/构建可用性：执行前检查 Git-common-dir Unreal 锁与 `UnrealEditor` 进程；任何需要关闭 Editor 的命令先请用户保存并关闭。
- 现有聚焦测试结果：规划阶段未运行；Executor 先回读四个 Profile、现有 Rabbit/Slime Born 资产与导入设置，记录替换清单。
- 共享契约 / 难合并资源风险：四个 Profile 与 Paper2D `.uasset` 为二进制独占写面；发布前必须对远端同路径变化做物理及逻辑审计，禁止静默覆盖。
- 基线损坏时的停止条件：源帧不可读/尺寸或透明通道异常、现有 Born 绑定语义与本 Plan 冲突、缺少可复用帧率惯例、Editor/锁被占用、或远端修改同一 Profile/目标目录时停止并报告。

## 实现提纲

1. 在专属 Executor worktree 回读四个 Profile、现有 Rabbit/Slime Born 资产、同类 Flipbook 的循环/帧率/材质/像素密度与导入设置；冻结准确替换和新增清单。
2. 通过 Unreal Editor Python/仓库脚本复制源 PNG 并导入 Texture，按项目 Paper2D 惯例创建逐帧 Sprite；不得在 Editor 外生成或手改 `.uasset`。
3. 在四个怪物的 `Flipbooks` 目录创建非循环 Born Flipbook，按自然数字顺序装帧；保存并重新加载检查引用。
4. 仅修改四个生产 Profile 的 Born Clip 绑定，回读其余字段确保未被重写；扩展或新增只读审计时不得硬编码机器路径。
5. 更新执行记录，运行验证矩阵并交由 Planner 审查二进制范围；PIE 人工验收通过前保持 `Review`。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 源输入 | 读取四组 PNG 的数量、自然顺序、尺寸、RGBA/透明通道 | 分别为 6/5/5/5，输入无缺帧且可解码 |
| 资产结构 | Unreal Python/Asset Registry 回读 Texture、Sprite、Flipbook、Profile | 数量、路径、帧序、非循环与四个 Born 引用准确，保存重载后一致 |
| 聚焦审计 | `python scripts/ue/audit_plan82_animation_assets.py`（必要时使用 Plan 内新增只读审计） | 四个生产 Profile 与源贴图导入链通过 |
| 项目静态 | `python scripts/validate_project.py` | 项目/资产规则通过 |
| 差异卫生 | `git status --short`、精确路径清单、`git diff --check` | 仅 Plan Writes 与获准的导入资产，无机器产物/意外删除 |
| 人工 PIE | 逐个生成 Rabbit/Slime/Goat/Fox | 出生动作视觉可接受并正常回到基础循环 |

## 执行记录

### 变化

- 待 Executor 填写。

### 证据

- Planner 已确认四组源序列可读取，帧数为 6/5/5/5；尚未导入或修改 UE 内容资产。

### 剩余风险

- 播放帧率、主体脚点与最终视觉节奏必须通过既有惯例回读和 PIE 人工验收确认。

### 人工验收结果/请求

- `PendingBeforeClose`：四种怪物的出生动画视觉与结束过渡。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`：规划阶段已审阅、无需修改；现有文档已覆盖可选 Born 语义、Profile/Flipbook 所有权与非玩法裁决边界。
- `shared/CODEBASE_MAP/ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md`：关闭前由 Planner 复审。
