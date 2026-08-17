# Plan 42 - 程序 - Editor 可编辑的第一关竞技场

## 协调

- Planner 负责人：Codex。
- Executor 负责人：Codex，用户于 2026-08-14 明确要求执行。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`，由用户在 PIE 中验收第一关构图、镜头、背景覆盖、角色可读性和场景调整体验。
- 本地规划 / 实现基线：`origin/main@59255cb`（已关闭 Plan41 的 ReEcho/ReEchoCombat/ReEchoWeapons/ReEchoAudio 四模块主线）；`Content/ReEcho/Art/Scene/**` 是当前未跟踪的用户美术输入，实施前必须原样隔离并通过 Unreal Editor 接入，不得覆盖。
- 实现分支：`plan/42-editor-authored-first-arena`。
- 依赖 / 阻塞：依赖当前 `Level00`、`AReEchoGameMode::CreateArena()`、玩家边界与怪物生成契约；阻塞后续第一关场景美术布置。实施前必须确认没有其他 `Content/Level00.umap` Active 独占所有者。
- Writes：新增 `Source/ReEcho/{Public,Private}/Presentation/Scene/**`；窄改 `Source/ReEcho/{Public,Private}/ReEchoGameMode.*` 与 `Player/ReEchoPlayerPawn.*`；`Content/Level00.umap`；`Content/ReEcho/Art/Scene/map01.png` 与经 Editor 维护的 `map01.uasset`；场景/相机聚焦测试和 Editor 资产工具；必要的 `Config/DefaultGame.ini` 兼容清理；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；本 Plan 执行记录；最终允许列表中的 Win64 Editor 预构建包。场景 Actor 内同时建立 `Ground`、`GroundDetail`、`MidDecoration`、`Foreground`、`Atmosphere`、`SceneEffects` 与 `Collision` 分层根节点，以及脚点排序、接触阴影和视差的只读表现契约。
- Stable Reads：`Content/ReEcho/Art/Scene/map02..04`、`AReEchoEncounterDirector`、敌人出生与玩家 Clamp 逻辑、天气/UI、Plan40 Animation2D 表现边界、Plan41 的 ReEchoCombat/ReEchoWeapons 攻击宿主接口、`Content/Level00.umap` 中现有灯光。
- 影响模式：`Content/Level00.umap`、`Content/ReEcho/Art/Scene/map01.uasset` 与最终预构建包为 `Exclusive`；Arena Scene 公共读取契约为 `SharedContract`。
- 兼容承诺 / 下游操作：保持现有战斗流程、玩家移动、敌人生成、伤害、录制、天气、UI 和 2D 表现语义；场景缺失时必须明确诊断并安全停止战斗初始化，不静默生成第二套竞技场。
- 明确排除：重新绘制 `map01`；接入 `map02..04`；在缺少素材时复制 `map01` 伪造装饰层；设计后续关卡切换；改变遭遇数值、角色尺寸、武器、动画或碰撞语义；把 Flipbook 播放或攻击判定迁入场景模块；大规模 GameMode 拆分；由 AI 代替用户判断最终构图。

## 锁定目标

将第一关的地图背景、正交玩家跟随相机、战斗地板与边界从 `AReEchoGameMode` 的运行时生成逻辑迁移到 `/Game/Level00` 中一个可在 Unreal Editor 直接选择、预览和调整的 Arena Scene Actor。第一关背景权威资源固定为 `/Game/ReEcho/Art/Scene/map01`。相机在安全区域锁定玩家为画面焦点并跟随移动；玩家接近地图边缘时，相机焦点必须先钳制在不会露出地图外侧的范围内，玩家可继续走到玩法边界而镜头停止。调整镜头或背景表现不得隐式改变玩家活动边界和怪物生成区域。

同一 Arena Scene Actor 负责第一关 2.5D 分层场景骨架：`map01` 只进入 `Ground`；`GroundDetail`、`MidDecoration`、`Foreground`、`Atmosphere` 与 `SceneEffects` 是 Editor 可编辑的可选表现容器，资源缺失时允许为空。`Collision` 与全部视觉层分离。场景模块提供基于角色脚点的排序参数、接触阴影默认参数和可选视差参数，但不持有角色动画、武器、伤害或碰撞权威状态。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` / `AREA-Presentation`、`AREA-Player`；GameMode 仍是流程编排器，Arena Scene Actor 只拥有第一关空间/表现配置。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，已加入 Writes。
- 设计意图：建立 Editor authored scene 与玩法消费者之间的窄契约，消除 GameMode 动态生成相机/背景/墙体以及 Pawn 重复相机参数。
- 权威状态与依赖：`Level00` 中唯一 Arena Scene Actor 拥有相机变换、正交宽度、背景组件、玩家边界和敌人生成边界；GameMode 只读取并应用，Player 只接收活动边界。
- 决策记录：
  - 使用专用 `AReEchoArenaSceneActor`，包含可编辑 Camera、Backdrop、Floor、四面边界和可视化范围组件；`OnConstruction()` 在 Editor 中更新布局。运行时只移动 Camera 分支，背景、地板、边界和装饰保持世界固定。
  - Actor 内部明确拆成 `VisualRoot` 与 `GameplayRoot`；视觉侧下分 `Ground`、`GroundDetail`、`MidDecoration`、`Foreground`、`Atmosphere`、`SceneEffects`，玩法侧下设 `Collision` 与范围可视化。视觉层禁用碰撞，碰撞层不承担可见美术。
  - 脚点排序由 Arena Scene 提供确定性换算契约：依据世界脚点在配置轴上的位置生成有限 Translucent Sort Priority；表现 Actor 可消费该值，但场景模块不直接驱动 Flipbook。接触阴影和局部特效只作为表现接口，缺失不影响玩法。
  - 视差默认关闭；启用后只允许移动可选装饰根节点，倍率和最大偏移在 Editor 暴露，`Ground`、`Collision`、玩法边界与相机安全区绝不参与视差。
  - 相机视野、背景尺寸、玩家活动边界、怪物生成边界为独立可编辑字段，不再复用单个 `ArenaSceneWorldHeight`。
  - 第一关直接引用 `/Game/ReEcho/Art/Scene/map01`；旧 `ArenaGround3D` 不再作为第一关权威资源。
  - GameMode 不再调用运行时 `SpawnActor<ACameraActor/AStaticMeshActor>` 创建场景；发现零个或多个 Arena Scene Actor 时输出明确错误并阻止不完整战斗初始化。
  - 相机使用固定旋转、固定 OrthoWidth 的平滑或直接跟随，不采用透视缩放。跟随权威是相机中心射线与战斗平面 `Z=0` 的焦点，而不是把 Camera Actor 简单附着到玩家。
  - 相机安全范围根据当前正交视锥四角投射到战斗平面的实际 footprint 与地图可见边界计算；焦点逐轴 Clamp。不得重新引入写死的 `±450`、假定 16:9 的半尺寸或仅按 OrthoWidth 猜测倾斜相机地面覆盖。
  - 当任一地图轴小于相机 footprint 时，该轴相机固定在地图中心并输出一次诊断；不得抖动、翻转 Clamp 区间或露出地图外。
  - 不采用 Actor Label、Tag 或软字符串拼接作为核心发现契约；使用专用类型并验证关卡中恰好一个实例。
- 相关文档同步范围：逐项审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md`、`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；预计只需更新 `MOD-ReEcho.md` 的场景所有权、运行流程、代码位置和测试路线。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅，无需修改；本次仍在单一 `MOD-ReEcho` 内部调整 Presentation/Player 组合关系，未改变模块拓扑或跨模块依赖。
  - `shared/CODEBASE_MAP/README.md`：已审阅，无需修改；未新增模块或稳定 AREA 标识。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：已更新并审阅，补充 Arena Scene 所有权、运行流程、Player 相机边界、代码位置与测试路线。

## 锁定验收

- [ ] 打开 `/Game/Level00`、不进入 PIE 时，可以在 Outliner 中选择唯一 Arena Scene Actor，并直接调整相机初始位置/旋转/OrthoWidth、跟随/边缘限制参数、`map01` 背景、地板和四面边界。
- [ ] Arena Scene 组件树中可直接找到 `Ground`、`GroundDetail`、`MidDecoration`、`Foreground`、`Atmosphere`、`SceneEffects` 与 `Collision`；`map01` 仅位于 `Ground`，其他表现层允许为空且不影响战斗启动。
- [ ] 视觉层无碰撞，玩法碰撞不依赖可见贴图；调整或隐藏任何装饰/前景/氛围层不会改变玩家 Clamp、怪物出生或攻击结算。
- [ ] 脚点排序换算在同一位置稳定、沿配置轴单调且被限制在配置范围内；接触阴影/视差接口缺失或关闭时安全退化，视差不得移动 Ground 与 Collision。
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

- 基线分支/提交：从 freshly fetched `origin/main@59255cb` 建立本地 `plan/42-editor-authored-first-arena`。
- 引擎/构建可用性：UE 5.8 Win64 Development FullRebuild 与四模块精选 Editor 包在 Plan41 关闭候选上通过；Executor 开始后需在自己的准确基线重跑增量构建。
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
9. 在 Arena Scene 内建立视觉/玩法双根节点与六类可选视觉容器；把现有 `map01` Backdrop 归入 Ground，把地板、墙和边界归入 Collision/GameplayRoot。
10. 增加脚点排序、接触阴影默认值和可选视差的 Editor 参数与聚焦测试；只提供场景契约，不修改 Flipbook 状态机或战斗判定。

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

- 新增 Editor authored `AReEchoArenaSceneActor`，独立暴露背景、相机安全区、玩家活动区和敌人出生区；其相机以实际倾斜正交视锥四角投影到 `Z=0` 的 footprint 计算安全移动范围。
- 通过 Unreal Editor 自动化在 `/Game/Level00` 放置唯一 `ArenaScene_Map01` 并绑定 `/Game/ReEcho/Art/Scene/map01`；`map01.png/uasset` 是本 Plan 唯一接入的 Scene 美术输入。
- GameMode 删除运行时相机、背景、地板和四墙生成，只验证/消费关卡 Scene 契约；玩家与敌人边界同时支持非原点的 Editor 场景中心。Pawn 删除备用 Camera，并按实际 PlayerCameraManager 计算鼠标横向朝向。
- 删除已经失去权威性的 `ArenaSceneWorldHeight` 与 `EnemySpawnEdgeInset` DeveloperSettings，避免配置与关卡 Actor 形成第二套场景事实来源。
- 新增 Arena Scene 聚焦自动化和幂等 Editor 写入工具，并同步 `MOD-ReEcho.md`。
- 首轮人工验收未通过后重新逐项审查 Plan42：修正 `map01` 的 U/V 轴到世界/画面轴映射，并补充相机安全区、玩家活动区、敌人出生区三套 Editor 线框可视化组件；四组独立范围不再只能通过数值猜测。
- 后续 Editor 调整复查移除 Arena Actor 上重复的 `CameraOrthoWidth`/`CameraAspectRatio` 字段；`ArenaCamera` 组件现在直接拥有位置、旋转、Ortho Width 与 Aspect Ratio，Construction 与运行时 Clamp 只读取组件当前值，不再覆盖美术/关卡调整。
- 单局流程出现 Pawn/第一视角回退后，PIE 日志确认关卡相机未朝向玩法平面，触发 `HasValidConfiguration()` 安全停止。已通过一次性 Editor 自动化把 `ArenaCamera` 恢复为 `(-630,0,900)`、Pitch `-55`、OrthoWidth `2800`、1376:768，并由 Editor 保存 `Level00`；后续仍由组件直接编辑，但必须保持镜头朝向 `Z=0` 玩法平面。
- 用户确认原拟 Plan44 的 2.5D 分层场景属于 Plan42 同一改造面后，将范围合并回本 Plan：Arena Scene 新增 `VisualRoot` / `GameplayRoot`，并建立 `Ground`、`GroundDetail`、`MidDecoration`、`Foreground`、`Atmosphere`、`SceneEffects`、`Collision` 分层。现有 Backdrop/map01 只归入 Ground，碰撞地板/墙归入 Collision，缺少素材的视觉层保持为空。
- 新增场景侧脚点排序换算、可选接触阴影默认参数和默认关闭的视差参数；这些接口不驱动 Flipbook、不裁决攻击/碰撞，Ground 与 Collision 不参与视差。
- 分层候选 PIE 截图显示角色/怪物血条和攻击特效正常，但启用序列动画的主体不可见。诊断确认场景生成与玩法正常，缺陷来自 Plan40 Flipbook 固定旋转未适配 Editor 可调倾斜相机；修复留在 Animation2D 内根据实际 ViewTarget 朝向渲染面，Arena Scene 不反向控制角色动画。

### 证据

- `scripts/ue/Build-Editor.cmd -Configuration Development`：通过；UHT/UBT 编译 Arena Scene、GameMode、Player 与测试，刷新四模块 Win64 Editor 预构建包。
- Editor Python commandlet：通过；`Level00` 成功保存唯一 Arena Scene 并绑定 `map01`，0 error。
- `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.ArenaScene`：发现 1 项，`ReEcho.Presentation.ArenaScene.Contract` 通过。
- `.clang-format`：已对全部修改 C++ 执行。`git diff --check`：通过。
- 首轮人工验收失败后的修正候选重新通过 Arena 聚焦自动化与 UE 5.8 Win64 Development FullRebuild；期间一次增量编译发现并移除了不可访问的非必要 `LineThickness` 设置，最终构建结果为通过。
- 分层场景候选：`scripts/ue/Build-Editor.cmd -Configuration Development` 通过并刷新四模块 Win64 Editor 预构建包；`python scripts/validate_project.py` 与 `git diff --check` 通过。
- 分层场景候选的 Arena 聚焦测试已补充脚点排序单调性与范围 Clamp 断言；本机 `Run-Automation.cmd` 在进入测试前被全平台 SDK `ValidatePlatforms` 预检拦截，因此本轮自动化记录为未运行，不能复用此前通过结果证明新增断言。

### 剩余风险

- Editor commandlet 启动时仍报告非 Win64 平台 SDK 缺失，但 Win64 有效且本次脚本与自动化均正常完成；这不是 Win64 候选阻塞。
- 当前机器的全平台 SDK 预检会在 Automation 启动前返回失败；新增分层/排序自动化尚无运行证据，需在 SDK 条件可用后补跑，或由程序对该具名门禁确认跳过并记录未验证。
- Flipbook 相机朝向修复已编译，但角色/怪物是否重新可见仍需用户重新进入 PIE 人工确认。
- 用户要求后续完成 UE 表现校验后自动打开项目。新增 `scripts/ue/Verify-And-Open-Editor.cmd` 作为本地验收入口：构建、项目校验和 diff 检查全部通过后才启动 Editor；该入口不代替发布 FullRebuild 或人工 PIE 结论。
- 用户要求场景组件位置也能在 Editor 中直接调整。Arena Scene 新增 `bAutoLayoutBackdrop` 与 `bAutoLayoutCollision`：默认开启以保留范围参数驱动布局；关闭后 Construction 不再覆盖 Backdrop 或 Floor/四墙的组件 Transform，关卡实例保存的 Editor 位置、旋转和缩放成为表现权威。相机与三套范围可视化仍保持各自原有契约。
- 根据 Editor 组件树复核，新增统一 `ArenaContentRoot`，将 `VisualRoot` 与 `GameplayRoot`（包括 Backdrop、Floor、四墙、范围和 ArenaCamera）全部挂在其下。用户可在 Editor 只移动该根节点完成整套场景平移；GameMode、相机 Clamp、脚点排序与视差统一读取该根节点世界中心，不再继续使用 Arena Actor 原点作为隐含中心。
- 上述结构未满足“地图与相机解耦”的复验要求，现改为 `MapRoot`：`VisualRoot`、Collision 和三套玩法范围属于 MapRoot；`ArenaCamera` 独立直属 `SceneRoot`。Editor 调整 MapRoot 的位置/XY Scale 时，地图、墙体和玩法范围同步变化，GameMode 与相机 Clamp 消费缩放后的中心/范围；Camera Transform 与 Ortho Width 保持不变。
- 第二次 Editor 组件树复验发现已放置的 Level00 Actor 仍保留旧 Attachment，且 Backdrop/Floor/四墙并非 MapRoot 的直接子节点。现将这六个组件的构造 Attachment 直接指向 MapRoot，并在 `OnConstruction()` 校正旧实例的序列化 Attachment；ArenaCamera 同时强制保持 SceneRoot 直属且保留世界 Transform。
- 用户确认镜头应锁定玩家并随玩家移动。新增默认开启的 `bLockPlayerToCameraCenter`：在地图相机安全区内直接令相机地面焦点等于玩家脚点，不受旧 `bSmoothCameraFollow` 插值影响；到达地图边缘/角落后仍由 Clamp 优先，允许玩家离开镜头中心走到玩法边界且不露出地图外。
- “80°俯视完整场景・高密度人怪精确比例 v5”锁定相机参考参数为 1920×1080、Pitch `-80°`、Ortho Width `2560 UU`、可见高度 `1440 UU`（`0.75 px/UU`）。ArenaCamera 新实例默认值同步为该参数；相机仍由 Editor 组件直接拥有，地图 MapRoot 缩放不会改写 Ortho Width。
- 用户提供新的夜景 `map01` 与同构灰度 AO 图。场景资产替换后，`ArenaScene_Map01.CharacterAOTexture` 按 MapRoot/Backdrop 的同一归一化 UV 将角色脚点映射到 AO；白色保持原亮度、暗部按 Editor 可调 `MinimumCharacterLight`/`MaximumCharacterLight` 压低显示亮度。采样由独立 `ReEcho2DSceneLightingComponent` 消费并只写 Flipbook 顶点色，不改 Actor、动画状态、碰撞或伤害；AO 缺失或格式不可读时回退全亮。
- 场地最终显示贴图按用户指定切换为 `/Game/ReEcho/Art/Scene/map00`；由于现有 AO 是按 `map01` 空间制作，切换地图后清空该错误绑定并将角色最低亮度恢复为 0.35。待美术提供与 `map00` 同 UV/空间匹配的 AO 后，再通过 Arena Scene Actor 的 Character AO 属性接入。
- 遭遇启动门禁：角色选择后，只有 Run Index 已推进、玩家/Director/Arena 有效且至少生成一个敌人时，才启动 Director 时钟并允许“敌人全灭”结束判定。敌人使用 AdjustIfPossibleButAlwaysSpawn，避免场景碰撞导致全体生成失败；日志记录关卡编号、玩家类、敌人数和时长，HUD 不再停留在 `0/6、0 秒` 的伪运行状态。
- PlayerSpawn 恢复路径：若引擎默认 Pawn 因 PlayerStart/场景碰撞未能生成，GameMode 在 Arena 配置完成后使用 `DefaultPawnClass`、Arena 中心和 Gameplay Plane 强制生成 Gameplay Blueprint，并由 PlayerController 显式 Possess。恢复仍失败则中止 Level00，不创建伪正常 HUD/遭遇。
- 单一玩家实例：Level00 不允许直接摆放 `BP_PlayerGameplay`，因为它与 GameMode 根据 `DefaultPawnClass` 创建并 Possess 的玩家是两个独立 Actor。场景维护脚本会删除直接摆放的 Player Pawn；Editor 调整通过打开 `BP_PlayerGameplay` 蓝图完成。
- Level00 增加 `EditorPreview_Player` 与 `EditorPreview_Enemy` 两个 `ReEcho2DEditorPreviewActor`。它们直接显示代表性玩家/Grunt Flipbook、目标世界高度和双层脚底阴影，可在 Editor 调整 Transform、Flipbook、WorldHeight 与阴影比例；Actor 标记为 EditorOnly，不进入 PIE、Cook、碰撞、AI 或战斗流程。
- Level00 的唯一 Arena Scene 已替换为 `/Game/ReEcho/Scene/Prefabs/BP_ArenaScene_Map00` 实例；替换脚本在保存前复制原 Actor 的可编辑场景参数、MapRoot/Camera/Backdrop/Collision 关键 Transform，并保持原 Label。美术可在该 Blueprint 的 `GroundDetail`、`MidDecoration`、`Foreground`、`Atmosphere` 与 `SceneEffects` 层新增组件；GameMode 仍按专用基类查找，不依赖 Blueprint 路径或 Label。
- 两个 Editor Preview 现仅通过一个 ChildActor 实例化真实 Gameplay Blueprint，关卡预览与 PIE 使用同一角色/怪物组件树；已移除重复的旧 Flipbook/FSM/阴影组件。Construction 只在目标类真实变化时设置 ChildActorClass，Level 写入脚本会先删除旧 Preview Actor 再各重建一个，清理历史重复子实例。
- 场景/角色相对关系复查修复：`GameplayPlaneZ` 现作为 MapRoot 局部脚底平面转换到世界高度；玩家、怪物、存档恢复和相机地面焦点统一消费该高度，Actor 中心按各 Gameplay Blueprint Box 半高对齐。移除玩家 `Z=112` 与敌人 `Z=50` 固定高度。角色 SceneLighting 以脚点更新透明排序；Editor Preview ChildActor 标记为 Visualization 并在 BeginPlay 显式销毁，避免预览碰撞、AI 和 FSM 进入 PIE。
- Review 修正后，Level 写入脚本不再依赖 `EditorPreview_Player/Enemy` 精确 Label，而是删除关卡内全部 `ReEcho2DEditorPreviewActor` 后重建；保存前断言类型数量恰好为 2 且 Label 集合准确，包含数字后缀的历史 Preview 也会被清除。
- 美术整体比例入口统一为 Blueprint：场景通过 `BP_ArenaScene_Map00.MapRoot` 调整地图/玩法范围整体比例且相机独立；角色/怪物通过各自 `BP_*Visual_*.VisualScale` 同步缩放身体、挂点与阴影。Profile `WorldHeight` 只提供基础标准尺寸，局部组件 Transform 不作为整体比例权威。
- 构图、地图边缘是否完全不露底、角色/敌人可读性和参数调整体验仍需用户在 PIE/Editor 中人工确认；在确认前不得关闭或发布本 Plan。

### 人工验收结果/请求

- 2026-08-17 空间契约候选：Level00 Arena Actor 与 MapRoot 已归零，GameplayPlane 固定为世界 `Z=0`，Floor 顶面与角色脚底共面；正交相机固定 `Pitch=-45°` 且保持与 MapRoot 解耦，只通过现有跟随/Clamp 逻辑平移，等待 PIE 构图验收。
- 怪物移动修正：Floor 与 Box 脚底共面时 `BlockAll` 会让水平 Sweep 持续处于接触/轻微穿透状态；Floor 改为 `NoCollision`，角色高度仍由 GameplayPlane 保持，四面墙继续负责边界阻挡。
- 地图编辑改造：用户后续明确取消地图 AO。Arena Scene 删除 `MapTexture`/`CharacterAOTexture` 与角色亮度采样，改由可编辑 `MapMaterial` 直接驱动 Backdrop；`ReEcho2DSceneLightingComponent` 仅保留脚点透明排序。`Content/ReEcho/Art/Scene/Map` 中的 Map00/Map01 分别生成材质实例，可通过 Arena Actor 属性切换。
- 植物编辑改造：Arena Scene 新增 `PlantRoot`；`build_editable_map_and_plants.py` 从 `Content/ReEcho/Art/Scene/Plants` 导入 16 种透明植物材质，以固定种子 42001 在 Level00 烘焙 36 个无碰撞卡片。每个卡片都是可独立编辑的 StaticMeshActor，带生成标签，重复执行只替换生成集合，不触碰美术手工添加的无标签植物。
- 植物朝向修正：36 个生成卡片不再写死角度，而是读取 `ArenaCamera` 当前 Pitch/Yaw，将 Engine Plane 的正面法线对准相机；生成器根据实际倾角、Plane 尺寸与缩放计算中心 Z，使卡片下缘落在 `GameplayPlaneZ=0`。相机只平移时朝向保持有效；聚焦资产校验覆盖卡片数量、相机朝向和贴地位置。
- 后续表现一致性修正：保留 Plan43 的 EnemyLogic、Capsule、事件和存档契约，仅把 Enemy Host 的可见组件恢复为本地 Animation2D 分层；怪物 FlipbookRoot 按实际 ViewTarget 对齐，程序化位移/拉伸与 GroundShadow 解耦，并接入脚点透明排序。该修正不改变怪物玩法碰撞或动画状态选择。

首轮人工验收：`Failed`。复查确认贴图轴向与 Editor 范围可视化未完整满足 Plan42，已形成追加修正候选；请用户重新在 PIE 中检查中心跟随、四边/四角锁定、地图外不曝光，以及 Editor 中四组独立边界和相机参数是否便于调整。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅，无需修改；模块拓扑未变化。
- `shared/CODEBASE_MAP/README.md`：已审阅，无需修改；标识与路由未变化。
- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：已更新并审阅；与候选实现一致。
