# Plan 42 - 程序 - Editor 可编辑的第一关竞技场

## 协调

- Planner 负责人：Codex。
- Executor 负责人：`Unassigned`。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Unassigned`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`，由用户在 PIE 中验收第一关构图、镜头、背景覆盖、角色可读性和场景调整体验。
- 本地规划 / 实现基线：`origin/main@52084b1`；`Content/ReEcho/Art/Scene/**` 是当前未跟踪的用户美术输入，实施前必须原样隔离并通过 Unreal Editor 接入，不得覆盖。
- 实现分支：`plan/42-editor-authored-first-arena`。
- 依赖 / 阻塞：依赖当前 `Level00`、`AReEchoGameMode::CreateArena()`、玩家边界与怪物生成契约；阻塞后续第一关场景美术布置。实施前必须确认没有其他 `Content/Level00.umap` Active 独占所有者。
- Writes：新增 `Source/ReEcho/{Public,Private}/Presentation/Scene/**`；窄改 `Source/ReEcho/{Public,Private}/ReEchoGameMode.*` 与 `Player/ReEchoPlayerPawn.*`；`Content/Level00.umap`；`Content/ReEcho/Art/Scene/map01.png` 与经 Editor 维护的 `map01.uasset`；场景/相机聚焦测试和 Editor 资产工具；必要的 `Config/DefaultGame.ini` 兼容清理；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；本 Plan 执行记录；最终允许列表中的 Win64 Editor 预构建包。
- Stable Reads：`Content/ReEcho/Art/Scene/map02..04`、`AReEchoEncounterDirector`、敌人出生与玩家 Clamp 逻辑、天气/UI、Plan40 Animation2D 表现边界、`Content/Level00.umap` 中现有灯光。
- 影响模式：`Content/Level00.umap`、`Content/ReEcho/Art/Scene/map01.uasset` 与最终预构建包为 `Exclusive`；Arena Scene 公共读取契约为 `SharedContract`。
- 兼容承诺 / 下游操作：保持现有战斗流程、玩家移动、敌人生成、伤害、录制、天气、UI 和 2D 表现语义；场景缺失时必须明确诊断并安全停止战斗初始化，不静默生成第二套竞技场。
- 明确排除：重新绘制 `map01`；接入 `map02..04`；设计后续关卡切换；改变遭遇数值、角色尺寸、武器、动画或碰撞语义；大规模 GameMode 拆分；由 AI 代替用户判断最终构图。

## 锁定目标

将第一关的地图背景、正交玩家跟随相机、战斗地板与边界从 `AReEchoGameMode` 的运行时生成逻辑迁移到 `/Game/Level00` 中一个可在 Unreal Editor 直接选择、预览和调整的 Arena Scene Actor。第一关背景权威资源固定为 `/Game/ReEcho/Art/Scene/map01`。相机在安全区域锁定玩家为画面焦点并跟随移动；玩家接近地图边缘时，相机焦点必须先钳制在不会露出地图外侧的范围内，玩家可继续走到玩法边界而镜头停止。调整镜头或背景表现不得隐式改变玩家活动边界和怪物生成区域。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` / `AREA-Presentation`、`AREA-Player`；GameMode 仍是流程编排器，Arena Scene Actor 只拥有第一关空间/表现配置。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，已加入 Writes。
- 设计意图：建立 Editor authored scene 与玩法消费者之间的窄契约，消除 GameMode 动态生成相机/背景/墙体以及 Pawn 重复相机参数。
- 权威状态与依赖：`Level00` 中唯一 Arena Scene Actor 拥有相机变换、正交宽度、背景组件、玩家边界和敌人生成边界；GameMode 只读取并应用，Player 只接收活动边界。
- 决策记录：
  - 使用专用 `AReEchoArenaSceneActor`，包含可编辑 Camera、Backdrop、Floor、四面边界和可视化范围组件；`OnConstruction()` 在 Editor 中更新布局。运行时只移动 Camera 分支，背景、地板、边界和装饰保持世界固定。
  - 相机视野、背景尺寸、玩家活动边界、怪物生成边界为独立可编辑字段，不再复用单个 `ArenaSceneWorldHeight`。
  - 第一关直接引用 `/Game/ReEcho/Art/Scene/map01`；旧 `ArenaGround3D` 不再作为第一关权威资源。
  - GameMode 不再调用运行时 `SpawnActor<ACameraActor/AStaticMeshActor>` 创建场景；发现零个或多个 Arena Scene Actor 时输出明确错误并阻止不完整战斗初始化。
  - 相机使用固定旋转、固定 OrthoWidth 的平滑或直接跟随，不采用透视缩放。跟随权威是相机中心射线与战斗平面 `Z=0` 的焦点，而不是把 Camera Actor 简单附着到玩家。
  - 相机安全范围根据当前正交视锥四角投射到战斗平面的实际 footprint 与地图可见边界计算；焦点逐轴 Clamp。不得重新引入写死的 `±450`、假定 16:9 的半尺寸或仅按 OrthoWidth 猜测倾斜相机地面覆盖。
  - 当任一地图轴小于相机 footprint 时，该轴相机固定在地图中心并输出一次诊断；不得抖动、翻转 Clamp 区间或露出地图外。
  - 不采用 Actor Label、Tag 或软字符串拼接作为核心发现契约；使用专用类型并验证关卡中恰好一个实例。
- 相关文档同步范围：逐项审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md`、`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；预计只需更新 `MOD-ReEcho.md` 的场景所有权、运行流程、代码位置和测试路线。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：待审阅。
  - `shared/CODEBASE_MAP/README.md`：待审阅。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：待更新并审阅。

## 锁定验收

- [ ] 打开 `/Game/Level00`、不进入 PIE 时，可以在 Outliner 中选择唯一 Arena Scene Actor，并直接调整相机初始位置/旋转/OrthoWidth、跟随/边缘限制参数、`map01` 背景、地板和四面边界。
- [ ] Editor 视口和 Camera Preview 能看到 `map01`；背景维持 1376:768 宽高比，不再出现相机视野尺寸是背景两倍的计算失配。
- [ ] 相机视野、背景显示尺寸、玩家活动边界与怪物生成边界是独立字段；修改相机 OrthoWidth 不改变玩法边界，只重新计算相机安全移动范围。
- [ ] 玩家位于相机安全区域时，相机焦点跟随玩家；玩家向四边和四角移动并越过相机安全阈值后，相机分别锁定对应边缘/角落，玩家仍可继续移动至玩法边界。
- [ ] 相机在边缘锁定时 `map01` 保持世界固定，画面不露出地图外侧，不因玩家继续移动而拉伸、缩放或跟随背景。
- [ ] 相机安全范围来自实际 ViewProjection/正交视锥在战斗平面上的 footprint；至少覆盖目标窗口比例、16:9 和地图小于视野的退化情形。
- [ ] `AReEchoGameMode` 不再运行时生成固定相机、背景、地板或四面墙，只查找、验证并消费 Level00 的 Arena Scene 契约。
- [ ] Pawn 不再保存或每帧更新已停用的备用相机配置；鼠标瞄准和所有面向相机的世界表现继续使用实际 PlayerCameraManager/ViewTarget。
- [ ] 玩家 Clamp、碰撞墙和敌人生成保持在 Arena Scene 声明的安全范围内；场景中零个或多个 Arena Scene Actor 时测试得到确定性失败诊断。
- [ ] 第一关战斗流程、天气、HUD、Animation2D、攻击、录制和存档行为不因场景 Editor 化改变。
- [ ] `/Game/Level00`、`/Game/ReEcho/Art/Scene/map01` 和新增 C++/资产均可加载并进入 Cook 依赖；`.umap`/`.uasset` 只通过 Unreal Editor 或审阅后的 Editor 自动化保存。
- [ ] UE 5.8 Editor FullRebuild、项目校验、预构建包校验和 `git diff --check` 通过。
- [ ] 用户在 PIE 中确认第一关构图、镜头、背景覆盖、角色/敌人可读性及 Editor 调整体验后，人工验收才能设为 `Passed`。

## Step 0 门禁

- 基线分支/提交：从 freshly fetched `origin/main@52084b1` 建立本地 `plan/42-editor-authored-first-arena`。
- 引擎/构建可用性：UE 5.8 Win64 Development FullRebuild 在 Plan40 发布候选上通过；Executor 开始后需在自己的准确基线重跑增量构建。
- 现有聚焦测试结果：Plan40 Animation2D 自动化入口受本机全平台 SDK 预检阻塞；本 Plan 必须新增不依赖视觉判断的 Arena Scene 契约测试，并记录实际可运行证据。
- 活跃独占所有权或共享契约批准：发布 Exchange 中的 `Level00.umap`、`map01.uasset` Reserved 行；Executor 启动前改为 Active，并确认无其他 Active 写入者或 Unreal 锁。
- 基线损坏时的停止条件：`map01` 无法由 Editor 加载、Level00 已有未隔离修改、场景资产所有权冲突、现有战斗基线不能启动，或必须改变玩法边界语义时停止并报告。

## 实现提纲

1. 隔离当前工作区未跟踪/已修改美术资产，在独立 worktree 中恢复 `map01.png/uasset` 作为明确输入。
2. 新增 `AReEchoArenaSceneActor` 及可编辑组件/字段；在 `OnConstruction()` 中更新 Camera、Backdrop、Floor、Walls 和范围可视化。
3. 提供只读 Scene 契约：Camera/ViewTarget、地图可见边界、玩家边界、敌人生成边界和背景资源；验证字段有限、非退化且场景实例唯一。
4. 通过 Editor 自动化把 Actor 放入 `Level00`，绑定 `map01`，保存关卡并检查引用；保留现有 Level00 灯光。
5. GameMode 改为发现和消费关卡 Actor，删除 `CreateArena()` 动态场景创建与重复尺寸计算。
6. 将实际 Camera 的地面焦点跟随玩家并按正交视锥 footprint 钳制；删除 Pawn 中停用的 Camera 组件/`UpdateFollowCamera()`，让瞄准和表现继续读取实际 ViewTarget。
7. 适配玩家 Clamp、怪物出生和必要测试，证明中心跟随、四边/四角锁定及相机参数变化不改变玩法边界。
8. 更新 `MOD-ReEcho.md` 和执行记录，构建、验证、提交 Review 候选并请求用户 PIE。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py`；`git diff --check` | 模块文档、路径、预构建和项目不变量通过 |
| C++ | `scripts/ue/Build-Editor.cmd -Configuration Development` | Arena Scene 类型、GameMode/Player 适配通过 UHT/UBT |
| 场景资产 | Editor 自动化加载 `/Game/Level00` 与 `/Game/ReEcho/Art/Scene/map01` | 唯一 Arena Scene、Camera/Backdrop/墙体组件和 Cook 引用存在 |
| 聚焦自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.ArenaScene` | 独立尺寸、唯一实例、视锥 footprint、中心跟随、四边/四角锁定、小地图退化和失败诊断通过 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`；预构建 check | 最终合并候选刷新允许列表产物 |
| 人工 PIE | 第一关目标窗口比例与 16:9、窗口缩放、中心跟随、四边/四角锁定、敌人生成、天气/动画/攻击 | 用户确认构图、地图外不曝光、可读性和 Editor 可调性 |

## 执行记录

### 变化

### 证据

### 剩余风险

- 当前 `Content/ReEcho/Art/Scene/**` 整体未被 Git 跟踪；实施时只接入明确的 `map01` 发布单元，不把 `map02..04` 顺带纳入本 Plan。
- 当前主工作区存在大量本地 `.uasset` 修改，不能直接用其作为干净实现 worktree。

### 人工验收结果/请求

等待实现候选后请求用户 PIE。

### 架构文档审阅结果

待实现后逐项填写。
