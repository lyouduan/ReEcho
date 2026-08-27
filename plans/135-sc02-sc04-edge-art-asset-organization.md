# Plan 135 - 程序 - SC02-SC04 场景边缘美术资产整理

## 协调

- Planner 负责人：当前对话程序 Planner（Codex）。
- Executor 负责人：独立程序 Executor，Plan 发布后启动。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`OpenAI Codex`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：资产候选 `b0cd1d3e`，已按用户授权组合 `origin/main@4ef2ff94b6a99856350674fce771a671314f4fa0`（含已发布 Plan134 `0945217f`）。
- 本地实现方式：规划 worktree `C:\tmp\ReEcho-plan135-sc02-sc04-edge-assets-plan`；实现使用独立 Plan135 worktree，不复用 Plan134 或场景审查 worktree。
- 依赖 / 阻塞：以 `Content/ReEcho/Art/Scene/SC01/EdgeInserts` 与 `Materials` 的目录和资产关系为参考；外部源目录为 `F:\MiniGame\scene\插件sc02`、`插件sc03`、`插件sc04`。
- Writes:
  - `plans/135-sc02-sc04-edge-art-asset-organization.md`
  - `Content/ReEcho/Art/Scene/SC02/EdgeInserts/**`
  - `Content/ReEcho/Art/Scene/SC02/Materials/**`
  - `Content/ReEcho/Art/Scene/SC03/EdgeInserts/**`
  - `Content/ReEcho/Art/Scene/SC03/Materials/**`
  - `Content/ReEcho/Art/Scene/SC04/EdgeInserts/**`
  - `Content/ReEcho/Art/Scene/SC04/Materials/**`
  - `Content/ReEcho/Scene/Prefabs/BP_ArenaScene_SC02.uasset`
  - `Content/ReEcho/Scene/Prefabs/BP_ArenaScene_SC03.uasset`
  - `Content/ReEcho/Scene/Prefabs/BP_ArenaScene_SC04.uasset`
  - `scripts/ue/import_scene_edge_assets.py`
  - `scripts/ue/validate_scene_edge_assets.py`
  - 用于一次性初始放置与只读校验的 `scripts/ue/*plan135*scene*.py`
- Stable Reads:
  - `Content/ReEcho/Art/Scene/SC01/EdgeInserts/**`
  - `Content/ReEcho/Art/Scene/SC01/Materials/**`
  - `Content/ReEcho/Scene/Prefabs/BP_ArenaScene_SC01.uasset`
  - 已发布 Plan134 的 Level00、Catalog、C++、地图/MapRoot、玩法平面与切换契约。
- 影响模式：`Exclusive`；新增 SC02-SC04 美术资源并一次性修改三个场景 BP 二进制，但不改 Runtime Module、数据 Schema 或场景切换契约。
- 兼容承诺 / 下游操作：程序只完成本次参考 SC01 的初始分层放置；所有插片都是对应 BP 内可直接编辑组件。后续美术手工调整 Transform、scale、visibility、material 与 `TranslucencySortPriority`，重跑工具只补缺失组件且不得覆盖这些属性。
- 明确排除：不修改 SC01、Level00、场景 Catalog、C++、CSV/XLSX；不改变 Plan134 的 `GameplayPlaneWorldZ=0`、地图/MapRoot/视觉参数或切换契约；不替美术做最终精细构图；不因源图内容相同而合并不同语义文件；实现候选未经用户确认不推送远程。

## 锁定目标

参考 SC01，将外部 SC02、SC03、SC04 PNG 按场景分别整理到 `Content/ReEcho/Art/Scene/SCxx/EdgeInserts`，生成 Texture2D、场景级边缘插片母材质和逐图材质实例，并将每套插片按上、下、左、右及局部装饰语义一次性放入对应 Arena BP 的直接可编辑分层组件中。初始布局只提供可用起点；美术继续在 BP 中完成最终构图，任何重跑不得覆盖其已调整状态。

## 架构影响与设计决策

- 受影响架构标识：`None`；本任务只增加表现内容资产与 Editor 导入工具，不修改 `MOD-ReEcho` 或公共运行时契约。
- 对应模块文档：无；不修改 Runtime Module，因此不新增或维护 `MOD-*` Writes。
- 设计意图：让 SC02-SC04 具备与 SC01 一致的“源 PNG → Texture2D → Material Instance → 美术 BP 组件”资源链，同时保持场景表现的 Editor 权威。
- 权威状态与依赖：仓库内 PNG 是导入来源；Texture2D 与材质实例是 UE 可用资产；对应 Arena BP 仍是组件摆放和视觉参数权威。
- 决策记录：
  1. 每个场景独立目录与母材质，避免 SC02-SC04 反向依赖 SC01 的场景专属资产。
  2. 中文源文件名保留在仓库 PNG 与映射清单中；UE 资产使用稳定 ASCII 语义名。
  3. SC04 三张内容相同的草图保持三个独立语义条目，不静默去重。
  4. 一次性 BP 作者ing工具参考 SC01 的视觉层级和方向语义，只创建缺失的 SC02-SC04 插片组件；不修改 SC01，也不触碰地图根、玩法边界或切换配置。
  5. 重跑时保留既有组件 Transform、scale、visibility、material 和透明排序；发现既有组件/资产与映射冲突时失败并报告，不静默替换。
- 相关文档同步范围：关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`modules/MOD-ReEcho.md`；预计均无需修改，因为模块拓扑、运行时职责和代码位置不变。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md` 已审阅、无需修改：待执行后确认。
  - `README.md` 已审阅、无需修改：待执行后确认。
  - `MOD-ReEcho.md` 已审阅、无需修改：待执行后确认。

## 锁定验收

- [x] SC02 的 4 张、SC03 的 8 张、SC04 的 12 张源 PNG 全部进入各自 `EdgeInserts`，像素尺寸和内容哈希与外部输入一致。
- [x] 每张源图都有稳定命名的 Texture2D 和 Material Instance；材质实例引用本场景母材质及正确纹理。
- [x] SC04 三张内容相同的草图仍保留三个独立语义资产入口。
- [x] SC02、SC03、SC04 的全部插片作为对应 BP 内直接可编辑组件存在，并按 SC01 的层级与方向语义形成可用初始布局。
- [x] 重跑作者ing工具只补缺失组件，保持既有 Transform、scale、visibility、material 与 `TranslucencySortPriority`。
- [x] SC01、Level00、Catalog、C++ 与数据文件无新增变化；Plan134 的四场 `GameplayPlaneWorldZ=0`、地图/MapRoot/视觉参数和切换契约保持。
- [x] Editor 资产校验、`python scripts/validate_project.py`、`git diff --check` 和最终 FullRebuild 通过。
- [ ] 用户在 Content Browser/Blueprint Editor 确认资产命名、透明显示与手工选用符合预期后，人工验收才可设为 `Passed`。
- [ ] 未提交精选预构建允许列表之外的 UE 生成物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：本地组合提交 `3269cbed`，包含 Plan135 资产候选 `b0cd1d3e` 与 `origin/main@4ef2ff94`；本 Plan 保持未发布。
- 引擎/构建可用性：用户已说明 Editor 关闭；执行器仍须在使用 Editor 命令前检查同克隆 Unreal 锁与进程。
- 现有聚焦测试结果：不复用 Plan134 的资产验证；本 Plan 对 24 张源图单独建立清单和 Editor 读取证据。
- 共享契约 / 难合并资源风险：`.uasset` 为二进制；但本 Plan 只新增 SC02-SC04 路径。若远端或其他候选已新增同路径资产，先审计语义，不覆盖。
- 基线损坏时的停止条件：外部源文件变化/缺失、源图无法解码、目标路径已有不兼容资产、Editor/命令锁被占用、需要修改 Blueprint 构图或运行时契约。

## 实现提纲

1. 按用户明确授权，以本地提交 `cf05b81a` 作为未发布 Plan135 基线；Executor 读取 Plan、执行规则、SC01 资产结构和现有 UE Python 工具的最小相关部分。
2. 将 24 张外部 PNG 精确复制到对应场景 `EdgeInserts`，以清单记录原名、稳定资产名、尺寸和哈希。
3. 编写可重跑 Editor 导入工具，创建 Texture2D、每场景母材质与逐图材质实例；遵守透明插片所需的材质和纹理设置。
4. 编写只读资产验证，检查包可加载、纹理尺寸、材质父子关系、纹理参数与 BP/SC01 不变性。
5. 更新 Plan 执行记录，完成静态检查、FullRebuild 和用户 Content Browser/Blueprint 人工验收请求。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| Plan-only | `python scripts/validate_project.py`、`git diff --check` | Plan135 编号、结构、范围和中文正文通过 |
| 输入 | PNG 清单、尺寸与 SHA-256 比对 | 24/24 仓库副本与外部输入一致 |
| Editor 导入 | 项目标准 Unreal Editor Python 入口 | 24 个 Texture2D、24 个 MI 和 3 个场景母材质可加载 |
| Editor 只读 | `scripts/ue/validate_scene_edge_assets.py` | 路径、尺寸、父材质和纹理引用完全匹配；BP 无作者ing |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目与文档不变量通过 |
| 发布候选 | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`、`python scripts/ue/prebuilt_editor.py check` | 最终候选与精选 Editor 包一致 |
| 人工 Content Browser/BP | 打开 SC02-SC04 资产并在对应 BP 手工选择验证 | 用户确认透明显示、命名与可编辑性正确 |

## 执行记录

### 变化

- 将外部 SC02 的 4 张、SC03 的 8 张、SC04 的 12 张 PNG 原名复制到各自 `EdgeInserts`；未对 SC04 三张同内容草图去重。
- 新增 `import_scene_edge_assets.py`：使用 UE Editor API 创建 24 个 Texture2D、3 个场景独立透明 Unlit 双面母材质和 24 个 Material Instance；已存在资产只读取，不覆盖设置。
- 新增 `validate_scene_edge_assets.py`：只读验证纹理尺寸、母材质属性、材质父子关系和 `InsertTexture` 引用。
- 材质目录采用实际 SC01 结构 `EdgeInserts/Materials`；该路径包含于锁定 Writes 的 `EdgeInserts/**`，未修改任何 Blueprint。
- 按用户后续授权扩大范围：在 SC02/SC03/SC04 分别新增 4/8/12 个 `StaticMeshComponent`，挂接于 `MidDecoration` 或 `Foreground`，使用对应材质实例并保持 BP 内直接可编辑。
- 初始布局沿用 SC01 的 Plane 朝向、按源图宽高换算缩放、上/下/左/右边缘位置和透明排序分层；脚本只创建缺失组件，已有同名组件不写任何属性。

### 证据

- 输入复制：24/24 仓库 PNG 与 F 盘对应源文件 SHA-256 完全一致。
- Editor 导入：日志确认创建/验证 24 个语义条目；UE 自动内容校验覆盖本任务 51 个 `.uasset`。
- Editor 只读验证：`[Plan135] Validation passed for 24 textures, 3 master materials, and 24 material instances`。
- 可重跑性：第二次执行导入工具后 51 个 `.uasset` 的 SHA-256 全部保持不变。
- Python 静态语法：两个工具均通过 `ast.parse`；`py_compile` 因沙箱不允许在该 worktree 生成 `__pycache__` 未作为证据使用。
- 远端审计：执行前 fetch 确认 `origin/main` 仍为 `50275bda156bce2fb2c2380883205e208244c0f2`，未出现同路径传入变化。
- 用户明确授权 Plan135 在未发布到 `origin/main` 的情况下仅本地实现和验证；当前候选不声称正式发布或关闭。
- 最终构建：`scripts\\ue\\Build-Editor.cmd -Configuration Development -FullRebuild` 成功，95/95 actions；`prebuilt_editor.py check` 通过（7 modules，BuildId `55116800`，source `c75ec9c0bd8b`）。
- 最终静态：FullRebuild 后 `python scripts/validate_project.py` 与 `git diff --check` 再次通过。
- Blueprint 作者ing：UE 日志确认创建 24 个缺失直接组件，并对 SC02-SC04 三个 BP 完成内容校验。
- BP 只读验证：24/24 组件的父层、Plane、对应 MI、NoCollision、无阴影、初始 Transform/缩放及透明排序通过。
- 重跑保护：再次运行作者ing后，51 个美术资源 `.uasset` 与 3 个 Arena BP 共 54 个文件 SHA-256 全部不变。
- Plan134 回归：四场 `GameplayPlaneWorldZ=0.000`；Catalog、地图材质、视觉根与玩法边界只读审计通过。
- 聚焦自动化：`ReEcho.Presentation.ArenaScene.Contract` 1/1、`ReEcho.StageTransition` 3/3 通过。
- 扩围后最终构建：FullRebuild 95/95；预构建检查 7 modules、BuildId `55116800`、source `ec777c5d939d`；项目、XLSX/CSV、LFS 与 `git diff --check` 通过。

### 剩余风险

- 自动化可以证明资产链和引用，不能替代美术对构图、缩放、遮挡与透明排序的判断。
- 外部 F 盘目录不是仓库权威；实现后以仓库内源 PNG 和映射清单作为可复现输入。

### 人工验收结果/请求

- `PendingBeforeClose`：用户在 Content Browser 和至少一个对应 Arena BP 中确认纹理/材质实例可正常选择并透明显示。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md` 已审阅、无需修改：本任务未改变模块拓扑、依赖方向或运行时状态权威。
- `shared/CODEBASE_MAP/README.md` 已审阅、无需修改：未新增或移动 Runtime Module / `AREA-*` 代码路线。
- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已审阅、无需修改：Arena Blueprint、场景切换和运行时装配均未修改，仅新增供美术手工选择的内容资产。
