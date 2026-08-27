# Plan 132 - 程序 - 开场角色与武器两阶段选择 UI

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner 与 Executor）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Accepted`。
- 人工验收：`PassedByUser`（用户完成最终父级/箭头布局微调并明确要求发布到远端主分支）。
- 规划基线：`origin/main@71b93f917b0946f6869a94c3d129a4564f4756a7`。
- 本地实现方式：独立 worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan132-start-loadout-two-stage-ui`，分支 `plan/132-start-loadout-two-stage-ui`。
- 依赖 / 阻塞：依赖现有 `WBP_ReEchoLoadoutSelection`、`WBP_ReEchoLoadoutEntry`、CSV 快照与 `OnLoadoutConfirmed(CharacterId, WeaponId)` 契约；执行 UE 作者ing、导入和验证前需关闭 Editor，并遵守同克隆 Unreal 锁。
- Writes:
  - `plans/132-start-loadout-two-stage-ui.md`
  - `Content/SourceArt/UI/LoadoutSelection/Plan132/*.png`
  - `Content/SourceArt/UI/LoadoutSelection/README.md`
  - `Content/ReEcho/Textures/UI/LoadoutSelection/*.uasset`
  - `Config/DefaultGame.ini`
  - `Content/ReEcho/UI/WBP_ReEchoLoadoutSelection.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoLoadoutEntry.uasset`
  - `Content/ReEcho/UI/WBP_ReEchoLoadoutTooltip.uasset`
  - `Source/ReEcho/Public/UI/ReEchoLoadoutSelectionWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoLoadoutSelectionWidget.cpp`
  - `Source/ReEcho/Public/UI/ReEchoLoadoutEntryWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoLoadoutEntryWidget.cpp`
  - `Source/ReEcho/Public/UI/ReEchoLoadoutTooltipWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoLoadoutTooltipWidget.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoLoadoutSelectionTests.cpp`
  - `scripts/ue/import_plan132_loadout_assets.py`
  - `scripts/ue/author_plan132_loadout_widgets.py`
  - `scripts/ue/audit_plan132_loadout_widgets.py`
  - `scripts/ue/migrate_plan132_entry_arrow_aspect.py`
  - `Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - 精选 Win64 Editor 预构建包及 manifest（仅最终发布门禁刷新）。
- Stable Reads:
  - `Content/Data/characters.csv`、`Content/Data/weapons.csv`、`Content/Data/weapon_types.csv`。
  - `Source/ReEcho/{Public,Private}/ReEchoGameMode.*`。
  - `Source/ReEcho/{Public,Private}/UI/ReEchoIndexedButton.*`、`ReEchoUIManagerSubsystem.*`。
  - 外部交付目录 `C:\Users\gavynqiu\Documents\miniGame\正式-UI视觉\正式-UI视觉\选择角色&武器\`。
- 影响模式：`Coordinated`；同时修改 UI C++ 状态机、两个 WBP、纹理资产及模块文档，但不改变 Run/武器权威数据或 GameMode 最终提交委托。
- 兼容承诺 / 下游操作：`AReEchoGameMode` 仍只在最终确认后收到一次 `(CharacterId, WeaponId)`；`UReEchoRunSubsystem::StartRun`、存档、预加载和首关开始顺序不变。角色与武器资格仍来自 CSV；展示顺序只在本页面按交付稿稳定 ID 排列，未知未来项追加到末尾且保留安全 fallback。
- 明确排除：不修改角色/武器数值、启用状态、默认武器、玩法描述真源、开局存档语义、Start Menu、战斗 HUD 或商店装配室；不把 4 张 `1-*.png` 合成稿直接当运行时整屏贴图；不新增交付稿没有表现的返回按钮或第三阶段。

## 锁定目标

把当前“角色和武器同页同时选择”的开场配装页改为两个连续阶段：

1. 角色阶段按交付稿从左到右显示 `勇者(J_HEART) / 智者(J_SPADE) / 诗人(J_CLOVER) / 猎手(J_DIAMOND)`。
2. 首次进入阶段时四项均使用明亮的“选中”素材，仅展示标题与选项，不显示说明、箭头和确认按钮；鼠标 Hover、键盘/手柄 Focus 或点击某项后，该项保持“选中”素材，其余项切换为“未选中”素材，并显示对应名称、箭头和确认按钮。鼠标说明复用商店原生 Tooltip，跟随当前条目并使用 20px、380px 宽自动换行；键盘/手柄 Focus 使用页面内大号锚定说明回退。
3. 第一次确认只锁定角色并切换到武器阶段，不广播最终 Loadout。
4. 武器阶段按交付稿从左到右显示 `镰刀(W_J_04) / 枪(W_J_09) / 长剑(W_J_01，界面显示“剑”) / 弓(W_J_08)`，沿用相同的初始、Hover/Focus/点击与确认表现。
5. 第二次确认才通过既有 `OnLoadoutConfirmed` 一次性提交角色与武器组合；不得提前启动 Run 或重复广播。

`1-角色选择.png`、`1-角色选择：hover.png`、`1-武器选择.png`、`1-武器选择：hover.png` 只作为 1920×1080 构图和状态参考。运行时使用 16 张角色/武器选中与未选中透明切图、`解释弹窗.png`、`选择箭头.png`、UMG 文本和现有正式确认按钮美术组合页面。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI`、`AREA-UI`。
- 权威状态：角色、武器资格和描述继续来自 CSV 快照；Widget 只拥有当前页面阶段、预览索引与未提交的两个稳定 ID。Run 和 GameMode 仍是最终开局事务权威。
- 公共契约：保留 `FReEchoLoadoutConfirmed(FName CharacterId, FName WeaponId)`，不要求 GameMode 新增中间角色确认端点。Entry 只扩展表现输入与 Preview/Selected 事件，不拥有玩法 ID。
- 依赖方向：UI 继续只读 `FReEchoCsvDataSnapshot` 并向 GameMode 发最终委托，不写 CSV、Run 或武器 Runtime。纹理由稳定资源路径加载，并由硬引用 WBP/AlwaysCook 进入打包。
- 表现顺序：交付稿顺序与 CSV 的 PromotionPriority/LoadoutOrder 不同；仅在本页按已知稳定 ID 应用美术顺序，未知新项按原数据顺序追加，避免用中文文案反查 ID 或改变全局数据排序。
- Hover 与可访问性：Hover、Focus 和点击统一进入同一 Preview 状态；点击仍是触屏/手柄确认候选的入口。无 Preview 时所有选项显示明亮版本；有 Preview 时只有当前项明亮，其他项使用未选中版本。鼠标 Hover 的说明由挂在 Entry Button 上的 Tooltip 自动定位，Focus 才启用 WBP 内的锚定回退，避免鼠标场景重复弹框。`WBP_ReEchoLoadoutTooltip` 是鼠标 Tooltip 的视觉权威，C++ 只写入标题与 CSV 说明；蓝图缺失时才使用原生最低可用树。
- 解释文字：角色读取 `characters.csv.Description`，武器读取对应 `weapon_types.csv.Description`；鼠标 Tooltip 按商店边框/底色策略自动换行并按内容增长，Focus 回退框使用 20px 正文且不再通过 ScaleBox 缩小文字。不在 C++ 复制一份中文玩法文案。合成稿中的“这里是一段……”视为占位说明，不进入正式内容。
- 失败与 fallback：缺少正式 WBP/纹理时保留原生最低可用树；单项正式纹理缺失时降级到既有角色/武器纹理，不改变选择资格或提交。
- 文档同步：维护 `MOD-ReEcho.md` 的开局 UI 流程和 `MOD-ReEchoUI.md` 的两阶段表现/状态所有者；更新 UI 修改指导。关闭前审阅 `ARCHITECTURE.md` 与 `CODEBASE_MAP/README.md`，若拓扑和索引不变则在本 Plan 记录无需修改。

## 锁定验收

- [x] 页面初次打开只显示角色阶段；构图接近 `1-角色选择.png`，四个角色均为明亮版本，解释框、箭头、确认按钮隐藏。
- [x] Hover/Focus/点击角色后，当前角色使用对应“选中”图，其余三项使用各自“未选中”图；鼠标 Tooltip 跟随当前条目且文字清晰，Focus 回退框、箭头和确认按钮层级正确。
- [x] 第一次确认只进入武器阶段并保留所选角色，不调用 `OnLoadoutConfirmed`。
- [x] 武器阶段初始及 Hover/Focus/点击表现分别接近两张武器参考稿，顺序为镰刀、枪、剑、弓。
- [x] 第二次确认只广播一次准确的 `(CharacterId, WeaponId)`；Run 启动、预加载和首关切换契约不变。
- [x] 18 张运行时切图保持交付原始像素/Alpha，不重绘或重采样；4 张整屏合成参考图不导入为运行时 Texture2D。
- [ ] `WBP_ReEchoLoadoutSelection` 与 Entry Compile/Save 成功；1920×1080、16:9 低分辨率和超宽屏下主体等比居中，无选项变形或按钮不可点击。
- [ ] 聚焦 UI 自动化、CompileAllBlueprints、项目静态校验及最终 `-FullRebuild` 发布门禁通过。
- [x] 用户人工确认两阶段导航、Hover/选中状态、文案可读性、构图和最终进入首关均可接受。

## Step 0 门禁

- 基线：独立 worktree 准确基于 `origin/main@71b93f917b0946f6869a94c3d129a4564f4756a7`。
- 当前实现：`UReEchoLoadoutSelectionWidget` 在同一页动态生成 `CharacterRow` 与 `WeaponRow`，一次确认要求两个 ID 均非空后广播；Entry 只有单张纹理和背景色选中状态。
- 交付审计：4 张 1920×1080 合成参考图；4 角色 × 2 状态、4 武器 × 2 状态、解释框和箭头共 18 张透明运行时切图。稳定名称映射完整，实际角色/开局武器均为 4 项。
- 风险：二进制热点为两个 Loadout WBP；C++ 热点为 Loadout 两个 Widget。发布前必须审计最新 main 对这些路径、GameMode 开局流程和 UI 文档的传入变化。
- 停止条件：交付切图与稳定 ID 无法唯一映射、必须修改 CSV/Run 权威语义才能实现、或最新主线对同一 WBP/开局状态机存在真实逻辑冲突时停止并报告。

## 实现提纲

1. 原字节归档 18 张运行时切图并导入规范 Texture2D；保留 4 张整屏图为外部视觉参考，不运行时导入。
2. 扩展 Entry，使其接收选中/未选中两张纹理、数据驱动描述，并把 Hover/Focus/点击统一转为稳定索引事件。
3. 将 Loadout 父 Widget 改为 Character/Weapon 两阶段状态机：按交付顺序生成当前阶段条目，初始无 Preview，第一次确认切阶段，第二次确认广播既有最终委托。
4. 作者ing Selection、Entry 和独立 Tooltip 三个 WBP：页面使用 1920×1080 设计面与响应式 ScaleBox，Tooltip 的宽度、边框、内边距、字体和换行由 Designer 管理；原生树只作资产失效 fallback。
5. 新增或扩展聚焦自动化，更新 UI 指导、模块文档和 Plan 证据；人工验收后合并最新 main，执行最终 FullRebuild 并发布。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 素材 | SHA-256、PNG 尺寸/Alpha、稳定 ID 映射 | 18/18 运行时切图原字节归档；4 张合成稿仅作参考 |
| C++ | `scripts\ue\Build-Editor.cmd -Configuration Development` | UHT/UBT 通过，委托签名与 GameMode 调用不变 |
| WBP | 导入/作者ing脚本、CompileAllBlueprints | 两个 WBP Compile/Save，无缺失绑定或资源加载错误 |
| 聚焦自动化 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.UI.LoadoutSelection`（新增后） | 初始阶段、两次确认边界、单次最终广播、稳定 ID 顺序通过 |
| 静态 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 项目、预构建、UTF-8 和生成物边界通过 |
| 最终发布 | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | 最终集成候选成功并刷新匹配的精选 Editor 包 |
| 人工 | PIE 新游戏完整走角色 → 武器 → 第 1 关 | 两阶段构图、Hover/Focus、说明、按钮、最终组合和首关进入正确 |

## 执行记录

### 变化

- 已原字节归档并导入 18 张角色/武器 Selected/Unselected、解释框与箭头切图；四张整屏稿只用于构图核对。
- `WBP_ReEchoLoadoutSelection` 已改为 1920×1080 响应式设计面，角色/武器 Stage Panel 分离，并加入正式标题字体、解释框、动态解释文字宿主、箭头和确认按钮；`WBP_ReEchoLoadoutEntry` 以透明按钮承载等比切图与名称。
- `UReEchoLoadoutSelectionWidget` 已实现 Character → Weapon 两阶段状态机、固定稳定 ID 表现顺序、CSV 描述读取和单次最终广播；Entry 将 Hover、Focus 与点击统一投射为 Preview，并按 Preview 切换成对纹理。用户复核后，鼠标说明改为复用商店原生 Tooltip 的跟随/屏幕避让策略，正文扩大到 20px、380px 宽；Focus 继续使用不缩字的大号页面回退框。
- 鼠标 Tooltip 的视觉层已从 `UReEchoLoadoutEntryWidget::BuildTooltip()` 的硬编码树拆为独立 `WBP_ReEchoLoadoutTooltip`；`TooltipRootSizeBox` / `TooltipFrame` / `TooltipSurface` / `TitleText` / `DescriptionText` 均可在 Designer 中微调，C++ 只写入当前标题和 CSV 说明并保留蓝图缺失时的原生回退。
- 用户复核指出仅暴露层级仍不是所见即所得；Tooltip WBP 的 Designer Preview 改为 `Desired` 内容实际尺寸，并从 `characters.csv` 读取 `J_HEART` 的真实标题与长说明作为设计期示例。打开 WBP 即显示运行时同宽、按内容撑高的框体，实际运行仍由当前条目文案覆盖，不写死高度。
- 用户再次复核明确指出目标是完整 Selection 页面而非 Tooltip 子框。`WBP_ReEchoLoadoutSelection` 已直接承载四个角色和四个武器的实际 `WBP_ReEchoLoadoutEntry` 实例，C++ 改为复用而非清空重建；Class Defaults 暴露 Designer Preview Stage/Index，可在同一 Designer 切换两阶段和初始/选中状态。
- 用户微调后发现单张 `SelectionArrow` 仍被状态刷新中的硬编码坐标覆盖；箭头契约改为角色/武器各四张 Designer-owned Image，运行时只切换可见性。定向迁移保留用户已调整的 `SelectionArrow` 作为猎手箭头，仅补齐其余七张，不重跑整页作者ing。
- 鼠标悬停说明的外框改为直接使用交付的 `T_UI_Loadout_DescriptionPanel` 九宫格 Brush，避免纯色 Border 在运行时缩放后丢失视觉边界；迁移仅修改 Tooltip WBP，不触碰用户继续微调的 Selection WBP。
- 角色/武器 Designer 切页从 Class Defaults 自定义预览枚举改为 UMG 原生 `StageSwitcher`。角色与武器两个实际 Stage Panel 都是 Switcher 的直接页面，设计者在 Details 中切换 `Active Widget Index=0/1` 即可即时编辑对应运行布局；运行时只设置同一个索引，不复制或重建页面。
- 用户继续微调 Selection 后反馈枪图被武器固定槽拉长、公共 Canvas 上的箭头不跟随条目 Hover 缩放。Entry 的 `PortraitScale` 契约现强制为 `ScaleToFit`；`EntrySelectionArrow` 与 `SelectButton` 改为 `EntryVisualOverlay` 的同级子项，复用全局按钮反馈对 Overlay 整组缩放。定向迁移只包装 Entry、添加内部箭头并移除 Selection 的 8 张脱离箭头，未重跑整页作者ing，用户本轮 Selection 微调保留在候选中。
- 首轮反馈后用户实图复核证明仅设置 `ScaleToFit` 仍不够：UE 5.8 会在 ScaleBox 子 Slot 为 Fill 时重新填满另一轴，且未完成平台编译的 Texture `GetSizeX/Y()` 可暂时返回 0。最终契约改为 `PortraitImage` 双轴 Center，并用 `GetImportedSize()` 每次重写 Brush 的真实源尺寸；Entry 自身使用 Desired Size Designer Preview，默认显示内部箭头，使单独打开 WBP 也能按实际比例编辑。
- 后续复核发现 Entry 的高度仍非蓝图权威：`NativePreConstruct` 每次设计器编译会把 `EntryRootSizeBox` 重写为 `DesignerPreviewHeight=560`，运行时 `Configure` 也会再次写入 `560/480`。修复后删除设计预览高度字段与所有高度回填，仅保留角色/武器横向布局所需的宽度注入；`EntryRootSizeBox.Height Override` 和 `PortraitSize.Height Override` 保存值在设计编译及运行配置前后保持不变。
- 增加幂等导入、作者ing、审计脚本及 `ReEcho.UI.LoadoutSelection.{Assets,Flow}` 自动化；Packaging AlwaysCook 收集正式 Loadout 纹理目录。

### 证据

- 源图 SHA/字节核对为 `18/18` 与交付一致；正式 Texture2D 导入日志为 `imported=18 verified=18`。
- `author_plan132_loadout_widgets.py` 成功编译并保存两个 WBP；`audit_plan132_loadout_widgets.py` 确认 Selection 17 个节点、Entry 7 个节点、所有 BindWidget 变量和正式宋体引用完整。
- Development Editor 增量构建成功，新增 Flow/Assets 测试、原生 Tooltip 与两阶段 Widget 源码均由 UHT/UBT 编译；当前精选预构建源码指纹为 `3586ec06e12f`。
- 用户反馈轮已把固定且缩字的鼠标说明替换为商店同款原生 Tooltip；作者ing/审计确认 Focus 回退 `DescriptionText` 为 20px 且不再 ScaleToFit。Flow 自动化新增 Hover 不重复显示锚定框、Focus 启用回退框的断言。
- `ReEcho.UI.LoadoutSelection` 找到 2 项测试，Assets 与 Flow 均为 `Result={Success}`；反馈轮后的 `python scripts/validate_project.py`、预构建一致性和 `git diff --check` 通过。
- `CompileAllBlueprints` 完成：`0 errors / 0 warnings / 0 blueprints that failed to load`；命令汇总的 4 条既有引擎/Legacy 警告不属于 Blueprint 编译失败或 Plan132 新增资产。
- Tooltip 蓝图化轮的 Development Editor 增量构建成功，预构建源码指纹刷新为 `cb9fe0ceb381`；作者ing/审计确认新 WBP 为 6 节点、单根 SizeBox、默认宽 380、标题 22px、正文 20px。更新后 `ReEcho.UI.LoadoutSelection.{Assets,Flow}` 再次均为 `Result={Success}`，`CompileAllBlueprints` 再次为 `0 errors / 0 warnings / 0 load failures`，项目校验、预构建检查和 `git diff --check` 通过。
- 所见即所得反馈轮将 Tooltip 的 Editor-only `DesignSizeMode` 固化为 `Desired`，并避免 `NativePreConstruct` 在设计期清空真实示例文案；资产审计输出 `tooltip_desired_preview=True`。Development Editor 增量构建成功，精选预构建源码指纹刷新为 `2b34590ccb99`；Loadout Assets/Flow 两项自动化均为 `Result={Success}`，全蓝图编译为 `0 errors / 0 warnings / 0 load failures`，项目静态校验、预构建一致性与 `git diff --check` 均通过。
- 完整 Selection 所见即所得返工后，WBP 审计确认 Selection 为 25 节点，角色与武器各有四个准确命名、类型为 `WBP_ReEchoLoadoutEntry_C` 的直接 Row 子项；Assets 自动化进一步实例化 WBP 并确认运行配置前后复用同一 Entry 对象。Development Editor 增量构建成功，精选预构建源码指纹为 `ea2eb0c53626`；Assets/Flow 均为 `Result={Success}`，全蓝图编译为 `0 errors / 0 warnings / 0 load failures`，项目静态校验、预构建一致性和 `git diff --check` 通过。
- SelectionArrow 回位修复轮通过定向迁移补齐 8 个 Designer-owned 箭头并保留用户微调后的 `SelectionArrow`；审计确认角色/武器各四张箭头均为设计面直接子项。Development Editor 增量构建成功，精选预构建源码指纹为 `cce07464ec44`；Assets/Flow 两项聚焦自动化均为 `Result={Success}`，全蓝图编译为 `0 errors / 0 warnings / 0 load failures`，项目静态校验、Python 脚本语法、预构建一致性和 `git diff --check` 均通过。
- Tooltip 外框修复轮将交付的 `T_UI_Loadout_DescriptionPanel` 绑定为 `TooltipFrame` 的九宫格 Brush；资产审计确认资源路径和 `DrawAs=Box`，Assets 自动化实例化实际 Tooltip 并验证运行时仍保留该 Brush。Development Editor 增量构建成功，精选预构建源码指纹为 `fa96a54f27f2`；Assets/Flow 均为 `Result={Success}`，全蓝图编译为 `0 errors / 0 warnings / 0 load failures`。用户继续微调但未纳入本轮提交的 Selection WBP 保持原样。
- 武器页所见即所得修复轮保留用户对 Selection 与 Tooltip 的最新微调，并只把两个既有 Stage Panel 移入原生 `StageSwitcher`；资产审计确认 `active=1`、页面顺序为角色/武器，打开 WBP 默认直接显示武器页。Development Editor 增量构建成功，精选预构建源码指纹为 `aab359f4d5c5`；Assets 自动化验证运行时从角色索引 `0` 正确切到武器索引 `1`，Assets/Flow 均为 `Result={Success}`，全蓝图编译为 `0 errors / 0 warnings / 0 load failures`。
- Entry 高度权威修复轮保留用户已保存的 `EntryRootSizeBox=600`、`PortraitSize=480` 微调；针对性脚本执行 WBP Compile + Save 后数值不变，新进程重新加载仍审计为 `600/480`。Development Editor 构建成功，精选预构建源码指纹为 `d6ec688a8a46`；保存后的 `ReEcho.UI.LoadoutSelection.{Assets,Flow}` 均为 `Result={Success}`，项目静态校验、预构建一致性、Python 语法与 `git diff --check` 通过。
- 枪图与箭头整组缩放修复轮：源图审计确认枪 Selected/Unselected 均为 `249×227` 近方形画布，其余三种武器为纵向画布；资产审计确认 Entry 为 9 节点、`PortraitScale=ScaleToFit`，`SelectButton` 与 `EntrySelectionArrow` 都直属 `EntryVisualOverlay`，Selection 中脱离条目的旧箭头为 0。Development Editor 增量构建成功，精选预构建源码指纹为 `0c11d45e8a87`；`ReEcho.UI.LoadoutSelection.{Assets,Flow}` 均为 `Result={Success}`，全蓝图编译命令以 `Success - 0 error(s), 4 warning(s)` 完成（4 条为既有引擎/Legacy 警告），未出现蓝图失败加载或 Plan132 错误。
- 上述首轮候选被用户视觉复核否决后，自动化新增枪 Brush 必须为 `249×227`、ScaleBox Slot 双轴 Center、Entry Desired Preview 和独立预览箭头可见断言。最终资产审计输出 `portrait_stretch=ScaleToFit portrait_align=Center`、`entry_desired_preview=True`、`entry_arrow_default=HitTestInvisible`；Development Editor 增量构建成功，精选预构建源码指纹为 `1639ddf6f78e`，`ReEcho.UI.LoadoutSelection.{Assets,Flow}` 两项再次均为 `Result={Success}`。

### 剩余风险

- 自动化已覆盖两次确认边界、Hover/Focus 说明模式与 18 张资源可加载，但不替代 PIE 主观验收。仍需人工复核角色/武器初始全亮、鼠标 Tooltip 跟随/屏幕边缘避让、长描述可读性、1920×1080/低分辨率/超宽屏构图，以及第二次确认后进入首关的完整体验。
- 最终 `-FullRebuild`、最新 main 集成和精选预构建一致性只在用户手测通过并授权发布后执行。

### 人工验收结果/请求

- `PendingBeforeClose`。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅；Runtime Module 拓扑、依赖方向和 UI/Run 权威边界未变，无需修改。
- `shared/CODEBASE_MAP/README.md`：已审阅；仍由现有 `AREA-UI` 路由到 `MOD-ReEchoUI.md`，索引无需修改。
- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：已同步新游戏的角色阶段确认 → 武器阶段确认 → 单次最终提交流程。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：已同步 Plan132 的阶段状态、Preview 表现、数据来源与最终委托边界。
- `Design/UI/ReEcho_UI修改指导.md`：已同步 WBP 绑定、可调锚点、切图目录、两阶段状态和 Designer 调整边界。
