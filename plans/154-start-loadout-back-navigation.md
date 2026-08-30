# Plan 154 - 程序 - 开场配装返回导航

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner 与 Executor）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Complete`。
- 人工验收：`Passed`。
- 本地规划 / 实现基线：`origin/main@4eb46ce8c806a00235a9e1f68688c0420483bebb`。
- 本地实现方式（可选，仅作交接说明）：独立 worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan154-loadout-back-navigation`，分支 `plan/154-loadout-back-navigation`；Plan 发布后在该工作区基于最新 `origin/main` 继续实现。
- 依赖 / 阻塞：依赖 Plan132/Plan138 已落地的 `WBP_ReEchoLoadoutSelection` 两阶段页面、点击选中语义、`OnLoadoutConfirmed(CharacterId, WeaponId)` 最终提交契约与现有 Start Menu/UI Flow；修改和验证 WBP 前需关闭 Unreal Editor，并遵守同克隆 Unreal 锁。
- Writes:
  - `plans/154-start-loadout-back-navigation.md`
  - `Content/ReEcho/UI/WBP_ReEchoLoadoutSelection.uasset`
  - `Source/ReEcho/Public/UI/ReEchoLoadoutSelectionWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoLoadoutSelectionWidget.cpp`
  - `Source/ReEcho/Public/ReEchoGameMode.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoLoadoutSelectionTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoGameModeTests.cpp`
  - `scripts/ue/migrate_plan154_loadout_back_button.py`
  - `scripts/ue/audit_plan132_loadout_widgets.py`
  - `Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - 精选 Win64 Editor 预构建包及 manifest（仅最终发布门禁刷新）。
- Stable Reads:
  - `Content/ReEcho/UI/WBP_ReEchoStartMenu.uasset` 与 `Source/ReEcho/{Public,Private}/UI/ReEchoStartMenuWidget.*`。
  - `Source/ReEcho/{Public,Private}/UI/Framework/ReEchoUIFlowCoordinatorSubsystem.*`、`ReEchoUITypes.h`、`ReEchoUIManagerSubsystem.*`。
  - `Source/ReEcho/{Public,Private}/Run/ReEchoRunSubsystem.*`、`ReEchoRunSaveGame.h` 的存档槽选择、覆盖保存与预览图契约。
  - `plans/132-start-loadout-two-stage-ui.md`、`plans/138-loadout-click-selection.md`。
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md`。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：既有 `OnLoadoutConfirmed(CharacterId, WeaponId)` 签名和“第二次确认才启动 Run”的语义保持不变；角色/武器资格、排序、点击选中、Tooltip 和 Designer-owned 几何不变。返回只取消未提交选择，不得启动、保存或删除 Run；最终确认仍只提交一次并使用原有目标存档槽。
- 明确排除：不重做角色/武器 Entry、切图、Tooltip 或主界面构图；不改变角色/武器数据、默认装备或存档格式；不新增第三阶段；不把返回实现为 Hover、焦点移动、右键或仅键盘快捷键；不重跑会覆盖用户现有 WBP 微调的整页 Plan132 作者ing脚本。

## 锁定目标

1. 在角色选择和武器选择两个阶段都提供可点击的“返回”按钮。返回按钮与现有“确定”按钮使用同一套正式按钮 Brush、尺寸、字体和状态表现，位于页面下方同一行：返回在左、确定在右；具体锚点和间距保留为 `WBP_ReEchoLoadoutSelection` 的 Designer-owned 可调属性。
2. 武器选择阶段点击返回，只回到角色选择阶段：保留已点击的角色作为当前角色候选，清空尚未提交的武器候选，并恢复角色页的选中、箭头、说明和确定按钮状态；不得广播最终 Loadout 或启动 Run。
3. 角色选择阶段点击返回，关闭 Loadout 页面并重新显示主界面，恢复主界面焦点、UI-only 输入和菜单暂停/能力屏蔽状态；不得创建、保存、覆盖或删除任何新 Run。
4. 返回按钮不依赖当前是否已有角色/武器候选，两个阶段都始终可见、可聚焦和可点击；确定按钮继续沿用“必须先点击一个候选才出现/可用”的现有规则。
5. 修正新增返回路径暴露出的存档安全问题：三槽已满时，点击“新游戏”进入角色页不得立即删除最旧存档；只有角色与武器最终确认并成功提交新存档时，才允许该目标槽被新 Run 覆盖。进入任一选择阶段后返回主界面，三个既有存档及预览图必须保持原样。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` / `AREA-Run` / `AREA-UI`，`MOD-ReEchoUI`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 的新游戏存档提交边界，以及 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 的 Loadout 返回状态机和跨屏幕委托；两者均已加入 `Writes`。
- 设计意图：Loadout Widget 继续只拥有未提交的页面阶段与候选；GameMode 继续拥有跨屏幕编排；RunSubsystem 继续拥有存档数据。新增返回动作不能让 UI 直接调用 Run 或自行操作 Viewport。
- 权威状态与依赖：Widget 增加窄的 `OnBackRequested` 类型化委托。武器页返回角色页属于 Widget 内部阶段迁移；只有角色页返回才广播该委托，由 GameMode 通过 UI Flow 关闭 Loadout、重开 Start Menu。既有最终确认委托保持原样。
- 决策记录：
  - 返回按钮作为 WBP 的 `BackButton` / `BackButtonLabel` 实体控件接入，复用现有确定按钮的正式 Style；C++ 只绑定行为、控制状态，不在运行时生成或重写 Designer 几何。
  - 武器页返回保留角色选择，避免用户重选；重新进入武器页时仍清空武器候选，避免旧武器被无意确认。
  - 当前新游戏流程在三槽已满时于打开 Loadout 前调用 `DeleteSavedRun()`。实现将移除该提前删除，只选择待覆盖槽；`StartRun + SaveRun` 仍在最终确认端点发生。现有 `SaveGameToSlot` 负责在提交时覆盖，首关激活后既有预览捕获会写入同槽新预览，返回路径不触碰存档或预览文件。
  - 若最终初始保存失败，禁止进入关卡；不得先删旧槽来制造“空槽”。本 Plan 不引入第二套待提交 Run 状态或存档 Schema。
  - 用幂等、定向的 Plan154 WBP 迁移只新增返回控件并调整确定按钮同排布局；禁止调用会重建页面的 Plan132 全量作者ing入口，以保护用户已完成的 Selection、Entry 和 Tooltip 微调。
- 相关文档同步范围：更新上述两个模块文档与 `Design/UI/ReEcho_UI修改指导.md`；关闭前审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 和 `shared/CODEBASE_MAP/README.md`。预计模块拓扑、依赖方向和路由索引不变，若事实未变则在执行记录中明确“已审阅、无需修改”。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEcho.md`：记录新游戏只在最终配装确认后覆盖目标槽，返回不产生持久化副作用；
  - `MOD-ReEchoUI.md`：记录 Character ↔ Weapon 与 Character → StartMenu 的返回契约、控件权威和委托边界；
  - `Design/UI/ReEcho_UI修改指导.md`：记录返回/确定按钮的 Designer 微调入口；
  - `ARCHITECTURE.md`、`README.md`：分别记录已更新或已审阅、无需修改及理由。

## 锁定验收

- [x] `WBP_ReEchoLoadoutSelection` 在角色页和武器页都显示“返回”；其普通、Hover、Pressed、Disabled Brush、字体和尺寸与“确定”一致，两个按钮在底部同排且不遮挡选项、名称、箭头或说明。
- [x] 角色页无候选时返回仍可用；点击后 Loadout 被关闭，主界面重新显示并获得键盘/手柄焦点，鼠标输入正常，没有残留 Loadout Widget 或重复委托。
- [x] 武器页点击返回后准确回到角色页，之前点击的角色仍选中；不广播 `OnLoadoutConfirmed`，再次确认角色进入武器页时没有旧武器候选。
- [x] 反复执行“角色 → 武器 → 返回 → 武器”不会累计点击绑定、重复广播、遗留说明/箭头或产生不可见焦点；第二次最终确认仍只提交一次准确的 `(CharacterId, WeaponId)`。
- [x] 存档槽有空位和三槽全满两种情况下，未最终确认就返回主界面均不创建、删除、覆盖存档或预览图；全满时原最旧槽只在最终确认的新 Run 保存成功时被覆盖。
- [x] `ReEcho.UI.LoadoutSelection` 聚焦自动化、Loadout WBP Compile/Save、CompileAllBlueprints、Development 构建、静态校验及最终发布 `-FullRebuild` 通过。
- [x] 用户在 PIE 人工确认两个返回路径、按钮视觉/位置、焦点与存档无副作用符合预期。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@4eb46ce8c806a00235a9e1f68688c0420483bebb`；远端最大已发布计划号为 `153`，本任务占用下一个编号 `154`。
- 引擎/构建可用性：实现前运行 `python scripts/setup_lfs.py --check`；执行 Unreal 作者ing、自动化或构建前确认 Editor 已关闭，并按仓库规则取得同克隆 Unreal 锁。
- 现有聚焦测试结果：Plan 阶段完成源码、WBP 脚本、现有 Plan 和模块文档只读审计；实现前记录 `ReEcho.UI.LoadoutSelection` 与相关 Save/Start Flow 聚焦基线。
- 共享契约 / 难合并资源风险：`WBP_ReEchoLoadoutSelection.uasset` 是用户持续微调的二进制热点；`ReEchoGameMode.*` 是高频流程热点。Plan 发布后及最终发布前必须审计最新 main 对这三个路径、Plan132/138 状态机和存档槽流程的传入变化。
- 基线损坏时的停止条件：最新 main 已新增不同的返回产品语义、改变 Start Menu/Loadout 屏幕生命周期、改变满槽新游戏覆盖策略，或实现必须重建用户已微调的整个 WBP/修改存档 Schema 时，停止越界部分并报告。

## 实现提纲

1. 为 Loadout Widget 新增 `BackButton` / `BackButtonLabel` 绑定、`OnBackRequested` 委托和阶段敏感处理：Weapon → Character 在本地完成，Character → StartMenu 才向 GameMode 发命令。
2. 在 GameMode 绑定/解绑返回委托并新增关闭 Loadout、重开 Start Menu 的窄端点；复用 UI Flow 恢复屏幕、焦点、输入和菜单屏蔽状态。
3. 把满槽新游戏从“选中并立即删除最旧槽”改为“只选中待覆盖槽”；保留最终确认处 `StartRun + SaveRun` 的单次提交边界，并验证返回不写存档。
4. 通过定向 Plan154 迁移在 `WBP_ReEchoLoadoutSelection` 中复用确定按钮 Style 新增返回按钮，初始布局为底部同排返回在左、确定在右；扩展审计脚本验证 Brush/字体/尺寸同源且控件为 Designer-owned。
5. 扩展 Loadout Flow/Asset 测试与存档安全聚焦测试，更新 UI 指导、模块文档和 Plan 执行证据；人工验收后执行最终 FullRebuild 和发布门禁。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| LFS | `python scripts/setup_lfs.py --check` | 当前检出对象完整，无 LFS 指针或缺失对象 |
| C++ | `scripts\ue\Build-Editor.cmd -Configuration Development` | UHT/UBT 通过，新增返回委托和 GameMode 端点链接成功 |
| Loadout 自动化 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.UI.LoadoutSelection` | 两阶段返回、保留角色、清空武器、单次最终广播和 WBP 资产契约通过 |
| Save/Start Flow | 运行新增聚焦过滤及相关 `ReEcho.Run` / Save 测试 | 空槽/满槽返回零持久化副作用，最终确认才覆盖目标槽 |
| WBP | 定向迁移、资产审计、CompileAllBlueprints | `BackButton`/Label 存在，同源 Style，同排 Designer 几何，所有蓝图 0 error |
| 静态 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 项目、预构建、UTF-8 和生成物边界通过 |
| 最终发布 | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | 最新集成候选成功并刷新匹配的精选 Editor 包 |
| 人工 | PIE：角色页返回主界面；角色 → 武器 → 返回；三槽全满后返回 | 导航、布局、焦点正确，旧存档与预览未变化，最终确认仍正常进入首关 |

## 执行记录

### 变化

- `WBP_ReEchoLoadoutSelection` 通过定向、幂等迁移新增 Designer-owned `BackButton` / `BackButtonLabel`；按钮复用确定按钮四态 Style、字体和内部对齐，底部布局为返回在左、确定在右，未重建角色/武器 Entry、Tooltip 或用户微调页面。
- 用户手测后把返回按钮微调为同一正式按钮资产族的深色版本，并移动到作者面右下角、把按钮与文字分别缩小到约 `230×90` / `28px` 后保存；最终候选保留该已验收构图。审计约束调整为两个按钮的四态均只能使用正式浅/深按钮资源、DrawType、字体资源与文字颜色契约一致，返回按钮必须位于 1920×1080 作者面且不能遮挡确定按钮，不再错误要求两者每一态指向同一纹理、同尺寸、同字号或同排。
- Loadout Widget 新增窄的 `OnBackRequested` 委托：武器页返回在 Widget 内恢复角色页、保留角色并清空武器；角色页返回由 GameMode 经 UI Flow 关闭配装页并重开 Start Menu。
- 满存档槽的新游戏入口不再提前删除最旧存档；现在只稳定选择空槽或最旧目标槽，仍由最终配装确认后的 `StartRun + SaveRun` 执行覆盖。
- 补充返回阶段、WBP 几何/样式契约及新游戏目标存档槽选择的自动化；同步 Designer 微调指导与 Run/UI 模块文档。

### 证据

- 发布锁内先合入 `origin/main@fe6b0320ae1531d29b850bcf6f49cbd133c73a8e` 的 Plan153 运行时：GameMode 自动合并保留双方语义，旧二进制未选取任一侧，而是从组合源码 FullRebuild。最终又合入 `origin/main@cdba442d` 的 Plan155-only 文档提交；该提交不改 Source/Content/Config/预构建包，因此只重跑静态门禁，不使刚完成的组合源码构建与自动化失效。
- Plan154 定向迁移首次日志：`created=True back=(601,892,370,128.5285) confirm=(999,892,370,128.5285)`；二次迁移为 `created=False` 且两组几何完全不变，证明重跑不会复位用户手调位置。
- 最终手调资产审计：`style=formal-family designer_owned=True`；Back 位于 `(1680,976)`、尺寸约 `230×90`、正式深色四态、正式宋体 `28px`，Confirm 位于 `(772,892)`、尺寸 `370×128.5`、正式浅色四态、同字体 `42px`；两者均在 1920×1080 作者面内且互不遮挡。
- FullRebuild 后 `ReEcho.UI.LoadoutSelection.{Assets,Flow}` 2/2、`ReEcho.GameMode.NewGameSaveSlot` 1/1 均为 `Result={Success}`；与传入 Plan153 重叠的 `ReEcho.Recording` 4/4、`ReEcho.Echo.Lifecycle` 1/1、`ReEcho.Encounter` 4/4、`ReEcho.Run.EchoReplayResolver` 3/3 也全部为 `Result={Success}`，所有测试进程 `EXIT CODE: 0`。
- UE 5.8 Development Editor `-FullRebuild` 成功（95/95 actions），精选包刷新为 7 modules、Build ID `55116800`、源码指纹 `92e661808ec6`；`CompileAllBlueprints` 为 `0 errors / 0 warnings / 0 blueprints that failed to load`。
- 最终 `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`python scripts/setup_lfs.py --check`、`git lfs fsck`、`git lfs status` 与 `git diff --check` 全部通过。

### 剩余风险

- 自动化可验证委托、阶段和持久化边界，但按钮间距、不同分辨率构图及鼠标/手柄实际焦点仍需人工验收。

### 人工验收结果/请求

- `Passed`：用户完成本地手测后明确要求按规则推送并合入远端主分支；手测后保存的 `WBP_ReEchoLoadoutSelection` 微调一并保留在最终候选。

### 架构文档审阅结果

- `MOD-ReEcho.md` 已记录新游戏满槽只选择待覆盖槽，取消配装不产生持久化副作用，最终确认才覆盖。
- `MOD-ReEchoUI.md` 已记录 Weapon → Character 与 Character → StartMenu 的返回契约、Designer 控件权威和委托边界。
- `Design/UI/ReEcho_UI修改指导.md` 已记录返回/确定按钮在 `WBP_ReEchoLoadoutSelection` 中的微调入口与不可由 C++ 重写的边界。
- `ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md` 已审阅：模块拓扑、依赖方向及路由索引未变化，无需修改。
