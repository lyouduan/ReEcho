# Plan 83 - 程序 - SC01 可编辑地图边缘插片

## 协调

- Planner 负责人：当前对话程序 Planner。
- Executor 负责人：独立程序 Executor，待 Plan 发布后启动。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Unassigned`。
- 任务状态：`InProgress`（首轮候选人工评审失败，返工由 Plan84 接管）。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@eab92341219463fbb8acd9b11c658114ed2bc5b4`。
- 本地实现方式：用户已确认一任务一 worktree；使用 `codex/plan83-sc01-edge-inserts` / `C:\Users\binnanliang\Documents\ReEcho-worktrees\plan83-sc01-edge-inserts`。
- 依赖 / 阻塞：源美术位于 `F:\MiniGame\sc01插件组合`，共 7 张带透明通道 PNG；最终构图、遮挡范围和层级观感需用户在 Editor/PIE 验收。当前 `Level00` 使用 SC02，本 Plan 不把 SC01 插片烘焙到该地图实例。
- Writes:
  - `plans/83-sc01-editable-edge-inserts.md`
  - `Content/ReEcho/Art/Scene/SC01/EdgeInserts/` 下 7 张保留原像素/透明通道的源 PNG、对应 Texture2D 及透明插片材质实例
  - `Content/ReEcho/Scene/Prefabs/BP_SC01EdgeInserts.uasset`（新增，可编辑插片组合）
  - `Content/ReEcho/Scene/Prefabs/BP_ArenaScene_SC01.uasset`（接入 SC01 专属插片组合）
  - `scripts/ue/author_plan83_sc01_edge_inserts.py`（新增幂等 Editor 作者ing工具）
  - `scripts/ue/verify_plan83_sc01_edge_inserts.py`（新增只读资产链与编辑契约校验）
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
- Stable Reads:
  - `Source/ReEcho/{Public,Private}/Presentation/Scene/ReEchoArenaSceneActor.*`
  - `Source/ReEcho/{Public,Private}/Presentation/Scene/ReEchoArenaSceneProfile.*`
  - `Content/ReEcho/Scene/Prefabs/BP_ArenaScene.uasset`
  - `Content/ReEcho/Scene/Profiles/DA_ArenaScene_SC01.uasset`
  - `scripts/ue/{build_plan52_scene_assets.py,author_plan52_decorations.py,verify_plan52_scene_assets.py}`
  - `plans/52-scene-map-blueprint-authoring.md`
- 影响模式：`Exclusive`（修改 SC01 Blueprint 二进制资产；不宣称远端写锁，但集成前必须审计同路径变化）。
- 兼容承诺 / 下游操作：插片只影响 SC01 表现；不得改变 Arena 边界、碰撞、出生、相机、深度排序权威或 SC02–SC04。插片资产缺失时 SC01 地图仍可进入，最多降低边缘装饰表现。
- 明确排除：不实现 Stage→SC01–SC04 运行时切换；不修改 `Level00.umap` 或当前 SC02；不重绘、合并、裁掉或降采样源图；不把七张图烘焙成单张不可编辑边框；不让插片参与碰撞、导航、命中或玩法遮挡判定。

## 锁定目标

1. 将 `F:\MiniGame\sc01插件组合` 的 7 张透明 PNG 原样导入项目，保持像素尺寸、Alpha 和独立资源身份。
2. 建立 SC01 专属可编辑插片组合：每张插片在 Blueprint Editor 中可独立选中并调整位置、旋转、缩放、可见性与透明排序，不需要程序改代码或重新合成图片。
3. 构图以用户参考图为目标：上、下、左、右插片围绕战斗区域边缘布置，允许越出地图并适度压入画面；中央战斗区不被大面积不透明图形遮挡。
4. 插片随 `BP_ArenaScene_SC01` 存在，不泄漏到当前 SC02 或其他场景；未来 Stage 场景切换接入 SC01 时自动获得完整边缘构图。
5. 该组合纯表现、无碰撞，不改变 `BackdropHalfExtents`、`CameraClampHalfExtents`、`PlayerHalfExtents`、`EnemySpawnHalfExtents` 或 Gameplay Plane。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` / `AREA-Presentation`，仅扩展 Arena Scene 的 Editor 作者ing资产与工具，不修改 Runtime Module 公共 API。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`，记录 SC01 专属可编辑边缘插片的资产归属和验证入口；已加入 `Writes`。
- 设计意图：让美术直接在 SC01 Blueprint 内编辑地图边缘构图，同时保持场景归属、玩法边界与表现层清晰分离。
- 权威状态与依赖：玩法边界仍由 `AReEchoArenaSceneActor` 的 Bounds/Collision 配置拥有；插片组合只持有表现 Transform、纹理、材质和排序。没有新 Runtime Module、公共契约或反向依赖。
- 决策记录：
  1. 使用 SC01 自有的组合 Blueprint 并由 `BP_ArenaScene_SC01` 接入，不继续向 `Level00` 烘焙散落 Actor；原因是当前 Level00 使用 SC02，地图级烘焙会混淆场景所有权。
  2. 七张源图保持独立组件/Actor，不合成单张边框；美术可以逐片修改 Transform、排序或替换纹理。
  3. 复用 Arena 的 `MidDecoration` / `Foreground` 分层语义：上方和左右中景进入中景层，下方压屏元素进入前景层；具体最终层级允许美术在锁定的纯表现边界内调整。
  4. 使用无碰撞、双面、透明表现材质；不新增 C++ 运行时管理器。若 UE Python 无法稳定作者ing目标 Blueprint 的组件树，Executor 必须停止并报告，不得退化为修改 Level00 或文本改 `.uasset`。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：关闭前审阅；预计模块拓扑和跨模块不变量不变，无需修改正文。
  - `shared/CODEBASE_MAP/README.md`：关闭前审阅；预计稳定标识和路由不变，无需修改正文。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：维护 Arena Scene 资产作者ing、场景归属和验证路线。
- 关闭前逐项填写审阅结果：上述三份文档分别记录“已更新”或“已审阅、无需修改”及原因。

## 锁定验收

- [ ] 七张 PNG 均成功导入，Texture2D 尺寸与源文件一致：`4072×917`、`3423×735`、`831×1043`、`765×980`、`436×820`、`382×862`、`721×658`；全部保留 Alpha。
- [ ] `BP_SC01EdgeInserts` 包含七个具名、可独立编辑的插片项，并能逐项调整 Transform、可见性和透明排序。
- [ ] `BP_ArenaScene_SC01` 接入且仅接入该组合；`BP_ArenaScene_SC02/03/04` 与 `Level00` 不发生二进制变化。
- [ ] 所有插片碰撞关闭、不投射阴影，不改变 Arena 的 Bounds/Collision/Gameplay Plane 配置。
- [ ] Editor 资产校验确认 Source PNG→Texture→Material→插片组合→SC01 Arena 引用链完整，脚本重复执行不会复制组件或产生散落 Actor。
- [ ] 用户在 Editor/PIE 验收参考构图：地图四周覆盖完整、中央战斗区可读、角色不会被错误遮挡、宽高比和镜头移动下无明显穿帮。
- [ ] `python scripts/validate_project.py` 与 `git diff --check` 通过；最终程序发布候选按 `GIT_RULES.md` 完成 `-FullRebuild` 和精选预构建包刷新。
- [ ] 未提交 `Saved/`、`Intermediate/`、Derived Data、日志或其他机器本地产物。

## Step 0 门禁

- 基线分支/提交：`origin/main@eab92341219463fbb8acd9b11c658114ed2bc5b4`；Plan 发布后重新 fetch 并核验。
- 引擎/构建可用性：UE 5.8 安装版；运行 Editor 作者ing、构建或资产验证前必须确认用户已保存并关闭本项目 Editor，并使用 Git common-dir Unreal 锁。
- 现有聚焦测试结果：本 Plan 不复用旧 Plan52 的资产验证作为新资源证据；新候选重新验证七张源图和 SC01 引用链。
- 共享契约 / 难合并资源风险：`BP_ArenaScene_SC01.uasset` 为 Exclusive 二进制写入面；当前主工作区另有未提交 `DA_ArenaScene_SC01.uasset` 修改但不在本 Plan Writes，必须保持不变且不得拷入隔离 worktree。
- 基线损坏时的停止条件：远端同路径变化、源 PNG 缺失/内容变化、Editor 无法安全创建可编辑组件树、SC01 Blueprint 编译失败，或必须修改本 Plan 排除路径时停止并报告。

## 实现提纲

1. 在隔离 worktree 复制七张源 PNG，并用 Unreal Editor API 幂等导入 Texture2D、创建透明双面材质实例。
2. 创建/更新 `BP_SC01EdgeInserts`，生成七个稳定命名的独立插片组件，设置无碰撞、无阴影和初始中景/前景排序；初始 Transform 参考用户截图并以战斗安全区为约束。
3. 在 `BP_ArenaScene_SC01` 内接入单一插片组合，确保场景切换时整体随 SC01 生命周期变化；不写 Level00。
4. 新增只读验证脚本，检查尺寸、Alpha/材质、七项唯一命名、可编辑组件、碰撞/阴影、引用链和非目标资产无变化。
5. 完成资产编译/加载校验、项目校验、最终 FullRebuild，并交给用户做 Editor/PIE 构图验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 源资源 | 脚本读取 7 张 PNG 尺寸、颜色格式与 Alpha | 文件数量、尺寸和透明通道与锁定清单一致 |
| Editor 作者ing | `UnrealEditor-Cmd` 执行 `author_plan83_sc01_edge_inserts.py` 两次 | 首次创建/更新，第二次幂等且不复制插片 |
| 资产链 | `UnrealEditor-Cmd` 执行 `verify_plan83_sc01_edge_inserts.py` | 七项引用、可编辑性、碰撞/阴影与 SC01-only 归属通过 |
| Blueprint | 加载并编译 `BP_SC01EdgeInserts`、`BP_ArenaScene_SC01` | 无编译错误；SC02–SC04/Level00 无改动 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目与文本不变量通过 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 准确最终候选通过并刷新允许的预构建包 |
| 人工 | Editor/PIE 检查四边构图、中央可读性、遮挡、镜头/宽高比 | 用户记录 `Passed` 或明确返修项 |

## 执行记录

### 变化

- 2026-08-23 首轮候选使用 `BP_SC01EdgeInserts` Child Actor 承载七张插片；用户评审确认无法在 `BP_ArenaScene_SC01` 中直接逐片编辑，因此该结构未接受、未提交实现。

### 证据

### 剩余风险

- 首轮 Child Actor 结构不满足父 Arena Blueprint 直接编辑要求；不得发布。Plan84 将插片改为 `BP_ArenaScene_SC01` 的直接组件，并补齐本 Plan 原先排除但用户现已明确要求的 CSV Stage→Scene 运行时切换。

### 人工验收结果/请求

`PendingBeforeClose`：等待实现后的 Editor/PIE 构图与遮挡验收。

### 架构文档审阅结果

