# Plan 120 - 程序 - Bad 怪物变形 Flipbook 接入

## 协调

- Planner 负责人：当前程序侧 Planner。
- Executor 负责人：当前程序侧 Executor（Plan 发布后执行）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Closed`。
- 人工验收：`Passed`。
- 本地规划 / 实现基线：`origin/main` @ `fb67ce38dc37e1efde8943eab0f67af90ec49fbe`。
- 本地实现方式（可选，仅作交接说明）：任务专属 worktree `C:\tmp\ReEcho-plan120-bad-enemy-transform`，分支 `codex/plan120-bad-enemy-transform`。
- 依赖 / 阻塞：源帧来自用户指定的 `F:\MiniGame\兔子变形序列帧`、`F:\MiniGame\史莱姆变形`、`F:\MiniGame\狐狸变形关键帧`；运行 Unreal 导入前必须确认本克隆没有其他进程持有 Editor 锁。
- Writes:
  - `plans/120-bad-enemy-transform-flipbooks.md`
  - `scripts/ue/import_plan120_bad_enemy_transform.py`
  - `scripts/ue/audit_plan120_bad_enemy_transform.py`
  - `Content/ReEcho/Art/Animation2D/Enemies/BadRabbit/Transform/**`
  - `Content/ReEcho/Art/Animation2D/Enemies/BadRabbit/Flipbooks/Transform.uasset`
  - `Content/ReEcho/Art/Animation2D/Enemies/BadSlime/Transform/**`
  - `Content/ReEcho/Art/Animation2D/Enemies/BadSlime/Flipbooks/Transform.uasset`
  - `Content/ReEcho/Art/Animation2D/Enemies/BadFox/Transform/**`
  - `Content/ReEcho/Art/Animation2D/Enemies/BadFox/Flipbooks/Transform.uasset`
  - `Content/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_RabbitDoll.uasset`
  - `Content/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_Slime.uasset`
  - `Content/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_Fox.uasset`
- Stable Reads:
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`
  - `Source/ReEchoPresentation/Public/Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h`
  - `Source/ReEchoPresentation/Private/Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.cpp`
  - `Content/ReEcho/DataAsset/Common/Animation2D/SM2D_DefaultCharacter.uasset`
  - 三个目标 Profile 现有动画集及邻近 Bad 怪物 Sprite/Flipbook 导入设置。
- 影响模式：`Exclusive`（三个 Profile 与对应 Transform 目录均为二进制难合并资源）。
- 兼容承诺 / 下游操作：仅补齐现有 `Transform.Phase2` 可选表现语义；不改变 Profile ID、状态机、玩法事件、动画完成回调语义或既有 Idle/Move/Attack/Hit/Death 映射。下游需在 PIE 触发三种怪物变形并确认画面节奏和脚点。
- 明确排除：不生成补间帧，不重绘源图，不修改非 Bad 怪物，不改玩法变形时机、AI、伤害、碰撞、比例规则、C++、Schema 或其他动画状态。

## 锁定目标

将用户提供的三组 PNG 按文件名数字升序接入 `BadRabbit`、`BadSlime`、`BadFox`：每帧生成对应 Texture2D 与 PaperSprite，在各自怪物目录中新建 `Transform/Textures` 和 `Transform/Sprites`，在既有 `Flipbooks` 目录生成非循环 `Transform` PaperFlipbook，并把该 Flipbook 绑定到对应敌人 Profile 的 `Transform.Phase2` Clip。变形由现有玩法事件驱动，表现资产不得反向决定玩法结果。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoPresentation`、`AREA-Presentation`；只消费既有 Profile/Clip/Flipbook 契约，不修改 Runtime Module 或公共 API。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md` 列为 Stable Read；由于本 Plan 不改模块职责、契约、依赖或代码位置，预计审阅后无需正文修改。
- 设计意图：为三个 Bad 怪物补齐统一的 `Transform.Phase2` 可选表现资源，继续保持玩法事件拥有变形时序、Presentation Profile 只拥有可播放资产映射。
- 权威状态与依赖：源 PNG 是本次导入输入；导入后的 Content 资产和三个 Profile 是 Cook 可见运行时配置。状态所有者和依赖方向不变。
- 决策记录：
  - 直接使用 `1.png…N.png` 数字升序，不生成或推断补间；狐狸目录虽名为“关键帧”，仍按用户给出的四帧组装。
  - 每个源帧保留为项目内 PNG，并通过 Unreal API 导入 Texture2D、创建 PaperSprite、组装非循环 Flipbook，禁止在 Editor 外手改 `.uasset`。
  - 使用项目邻近 Bad 怪物资产的像素密度、材质/透明度、Pivot 和帧率约定；若三个目标现有资产无法给出一致约定，停止并请用户决定，不暗中混用。
- 相关文档同步范围：关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`；预计均无需修改，因为只补资产实例与既有 Clip 映射。
- 关闭前逐项填写审阅结果：待实现后记录。

## 锁定验收

- [x] `BadRabbit/Transform` 恰有 7 个按序 Texture2D 与 7 个 PaperSprite，`Flipbooks/Transform` 恰有 7 个顺序一致的 Key Frame，且 Profile Clip 不循环。
- [x] `BadSlime/Transform` 恰有 5 个按序 Texture2D 与 5 个 PaperSprite，`Flipbooks/Transform` 恰有 5 个顺序一致的 Key Frame，且 Profile Clip 不循环。
- [x] `BadFox/Transform` 恰有 4 个按序 Texture2D 与 4 个 PaperSprite，`Flipbooks/Transform` 恰有 4 个顺序一致的 Key Frame，且 Profile Clip 不循环。
- [x] `DA_Enemy_RabbitDoll`、`DA_Enemy_Slime`、`DA_Enemy_Fox` 的 `Phase2` 动画集均把 `Transform.Phase2` 映射到各自 `Transform` Flipbook，且导入脚本只改该 Clip。
- [x] Unreal 加载/只读审计、`python scripts/validate_project.py`、`git diff --check` 与最终 `-FullRebuild`/预构建包门禁通过。
- [x] 用户在 PIE 中确认三段变形动画的帧序、速度、透明边缘、比例、朝向和脚点可接受。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main` @ `fb67ce38dc37e1efde8943eab0f67af90ec49fbe`。
- 引擎/构建可用性：待 Executor 确认 UE 5.8 安装版、Editor 进程和 Git-common-dir Unreal 锁。
- 现有聚焦测试结果：尚未执行；先只读审计三个 Profile 和源帧尺寸/透明通道。
- 共享契约 / 难合并资源风险：三个目标 Profile 与新增 `.uasset` 是二进制；发布前 fetch 后必须对同路径外部变化做物理、逻辑与耦合审计。
- 基线损坏时的停止条件：任一源帧缺失/不可读/非 PNG，Profile 缺少可绑定的 `Transform.Phase2` 契约，邻近资产约定互相冲突，目标资产已被外部提交新增，或 Unreal Editor/锁正被其他任务使用。

## 实现提纲

1. 读取 Profile 公共头/实现和最小相关导入脚本；以 Unreal 只读脚本记录三个 Profile 的现有 Clip、邻近资产帧率/Pivot/像素密度，并记录源 PNG 尺寸与 Alpha。
2. 编写可重跑导入脚本：复制源 PNG 到准确 `Transform/Textures`，通过 Unreal API 导入 Texture2D、创建/更新 PaperSprite、创建非循环 Flipbook，并仅更新三个 Profile 的 `Transform.Phase2` Clip。
3. 编写只读审计脚本，证明每组资产数量、Key Frame 顺序、循环标志、源 Texture 绑定和 Profile 引用准确，同时检查非目标 Clip 未被改写。
4. 冻结候选范围，执行项目校验、diff 审计和程序发布构建门禁；记录构建刷新出的精选预构建文件，排除其他生成物。
5. Planner 审阅实际 diff、外部变化、架构文档与人工 PIE 结果后决定关闭、集成和发布。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 输入静态 | PowerShell/Pillow 读取三组 PNG | 数量分别 7/5/4，数字顺序连续，尺寸/Alpha 可读 |
| Unreal 资产 | UE 5.8 `-ExecutePythonScript=scripts/ue/audit_plan120_bad_enemy_transform.py` | Texture/Sprite/Flipbook 数量、顺序、非循环和三个 Profile 引用全部准确 |
| 项目静态 | `python scripts/validate_project.py` | 项目/资产路径不变量通过 |
| 发布构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新匹配精选 Editor 预构建包 |
| Diff | `git diff --check` 与精确路径清单审计 | 无空白错误、无范围外资产/生成物 |
| 人工 PIE | 分别触发 BadRabbit、BadSlime、BadFox 的变形 | 帧序、速度、透明边缘、比例、朝向和脚点获得用户确认 |

## 执行记录

### 变化

- 用户确认按各怪物源帧数量设置 FPS：BadRabbit 7 FPS、BadSlime 5 FPS、BadFox 4 FPS；每段 Transform 均约 1 秒。
- 只读 UE 基线探针确认三个 Profile 均有且仅有一个 `Phase2` AnimationSet；BadRabbit/BadSlime 的 Transform 当前使用暗形态 Move 临时回退，BadFox 尚无 Transform Clip。本实现仅替换/新增该 `Phase2` Clip，不触碰默认形态或其他语义。
- 新增可重跑导入脚本，将 16 张源 PNG 复制为稳定 `Transform_XX.png`，通过 Unreal API 创建/更新 16 个 Texture2D、16 个中心 Pivot PaperSprite 和 3 个 Transform PaperFlipbook，并绑定三个目标 Profile。
- 新增只读审计脚本，独立加载已保存资产并验证数量、顺序、Sprite SourceTexture、1 px/uu、FPS、FrameRun 与 `Phase2` Clip 的非循环/重启策略。

### 证据

- 输入静态：兔子 7 帧、史莱姆 5 帧、狐狸 4 帧；全部 PNG 可由 System.Drawing 读取，均为 `Format32bppArgb`。
- UE 只读基线：`PLAN120_BASELINE_RESULT profiles=3 read_only=true`。
- UE 导入：`PLAN120_IMPORT_RESULT`，BadRabbit/BadSlime/BadFox 分别生成 7/5/4 帧并保存三个 Profile。
- 全新 UE 进程后验：`PLAN120_AUDIT_RESULT assets_ok=true profiles_ok=true audited=[('BadRabbit', 7, 2), ('BadSlime', 5, 2), ('BadFox', 4, 2)]`。
- 项目静态：`python scripts/validate_project.py` 通过；`git diff --check` 通过。
- 最终构建：`scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` 94/94 成功；`prebuilt_editor.py check` 通过，`modules=7 build_id=55116800 source=9050699833cd`。

### 剩余风险

视觉节奏、透明边缘和逐帧脚点只能由 PIE 人工验收；客观资产审计不能替代该结论。

### 人工验收结果/请求

`Passed`：用户于 2026-08-26 确认三个 Bad 怪物变形动画“没问题”，同意关闭并推送。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md` 已审阅、无需修改：未改变模块拓扑、依赖或跨模块不变量。
- `shared/CODEBASE_MAP/README.md` 已审阅、无需修改：未新增或移动稳定架构标识/代码路线。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md` 已审阅、无需修改：继续消费既有 Profile、`Transform.Phase2` 与 PaperFlipbook 契约，只补资产实例和映射。
