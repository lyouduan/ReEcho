# Plan 64 - 程序 - 回响轨迹迁移到右上角小地图

## 协调

- Planner 负责人：Gavyn（程序）
- Executor 负责人：Gavyn（程序）
- Plan 编写方（AI 侧）：`Gavyn-side AI | ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Proposed`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：`origin/main @ 07fd624`（ReEcho 主工作树）。
- 本地实现方式（可选，仅作交接说明）：在独立工作树 `ReEcho-plan64-echo-trajectory-minimap`（分支 `plan/64-echo-trajectory-minimap`）基于 `origin/main` 实现，main 仅接收 fast-forward。
- 依赖 / 阻塞：无外部依赖；本 Plan 不依赖数据层（XLSX/CSV 不变）。
- Writes：
  - `Source/ReEcho/Public/UI/Framework/ReEchoUIScreenTypes.h`（新增 `EReEchoUIScreen::Minimap`、`EReEchoUILayer::Minimap`）
  - `Source/ReEcho/Private/UI/Framework/ReEchoUIManagerSubsystem.cpp`（`ScreenClasses` / `GetScreenLayer` / `GetLayerZOrder` 注册）
  - `Source/ReEcho/Public/UI/ReEchoMinimapWidget.h`（新增）
  - `Source/ReEcho/Private/UI/ReEchoMinimapWidget.cpp`（新增）
  - `Source/ReEcho/Private/Graybox/ReEchoEchoActor.cpp`（移除世界轨迹 spawn）
  - `Source/ReEcho/Public/Graybox/ReEchoEchoActor.h`（移除 `#include` 与 `Trajectory` 成员）
  - `Source/ReEcho/Private/Graybox/ReEchoTrajectoryActor.cpp`（如确认无他用则删除）
  - `Source/ReEcho/Public/Graybox/ReEchoTrajectoryActor.h`（如确认无他用则删除）
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`（`OpenScreen(Minimap)` + 成员）
  - `Source/ReEcho/Public/ReEchoGameMode.h`（成员 `MinimapWidget`）
  - `Source/ReEcho/Private/Tests/ReEchoMinimapTests.cpp`（新增，坐标映射单测 + 屏幕创建）
- Stable Reads：
  - `Source/ReEcho/Public/Core/ReEchoTypes.h`（`FReEchoRecording::Positions`、`FReEchoPositionSample{Time,Position}`）
  - `Source/ReEcho/Public/ReEchoGameMode.h`（`Player`、`Echoes`、`ClearEchoes`）
  - `Source/ReEcho/Public/UI/ReEchoEncounterHudWidget.h/.cpp`（右上角布局与 C++ 建树范式，Minimap 照此）
  - `Source/ReEcho/Public/Graybox/ReEchoArenaSceneActor.h` / `Source/ReEcho/Public/Player/ReEchoPlayerPawn.h`（世界平面 X/Y、竞技场中心与半范围）
- 影响模式：`Isolated`（纯 UI + 视觉迁移；不改数据/XLSX/CSV；删除的 `AReEchoTrajectoryActor` 经搜索仅被 `ReEchoEchoActor` 引用）。
- 兼容承诺 / 下游操作：删除世界轨迹 actor 不影响回放逻辑（`ReEchoPlaybackComponent::LoadRecording` 仍驱动回响行为，轨迹 actor 仅作地面视觉）。
- 明确排除：
  - 攻击特效 / 投射物贴图（刀光、龙卷风、弓箭、枪弹、月波）不在本 Plan 范围。
  - 完整小地图（敌人位置、竞技场装饰、迷雾）不在本 Plan 范围；本 Plan 小地图仅含回响轨迹 + 玩家与回响实时位置。
  - 不改动武器/构筑/数值数据。

## 锁定目标

玩家可见结果：

1. 回响（Echo）历史轨迹**不再**以地面实例化立方体渲染于 3D 世界（移除 `AReEchoTrajectoryActor` 的地面投影）。
2. 右上角新增一个小地图面板，实时显示：
   - 各回响的历史轨迹折线（取自 `FReEchoRecording.Positions`）；
   - 玩家当前位置（live，取 `AReEchoPlayerPawn` 世界坐标）；
   - 各回响当前位置（live，取 `AReEchoEchoActor` 世界坐标）；
   - 遭遇进行中持续刷新（节流 ~20Hz）。
3. 六场遭遇流程不回退：暂停 / 存档退出 / 继续 / 死亡与胜利结算 / 输入恢复均不受影响。

## 架构影响与设计决策

- 受影响架构标识：`AREA-UI`（运行时 HUD 层）、`AREA-VisualGraybox`（地面轨迹视觉）。无新增 `MOD-*`，纯属现有 UI 与 Graybox 视觉的迁移。
- 对应模块文档：UI 与 Graybox 现有 `modules/MOD-*.md`（如有）需在关闭前确认是否提及世界轨迹；本 Plan 不改变模块职责边界，仅将「回响轨迹可视化」职责从 Graybox 地面 actor 迁移到 UI 小地图 widget。
- 设计意图：将回响轨迹从「世界空间地面投影」收拢为「屏幕空间俯视总览」，减少世界视觉噪声、提升可读性，并让玩家在右上角一眼看到回响走过的路径与当前位置。
- 权威状态与依赖：不改变数据所有者或公共契约；`FReEchoRecording` 仍为唯一数据真源，小地图只读其 `Positions`，不写。
- 决策记录：
  - **世界轨迹移除 vs 保留**：用户明确「抽出来」= 移除世界地面渲染，仅在小地图显示（减少噪声）。故删除 `AReEchoTrajectoryActor` 的 spawn 与 `InitializeTrajectory` 调用。
  - **小地图内容范围**：用户选「回响轨迹 + 玩家与回响实时位置」，故小地图读取 GameMode 的 `Player` 与 `Echoes` 实时坐标，并画各回响 `Recording.Positions` 折线（多回响时按索引着色区分）。
  - **渲染方式**：采用 UMG 自定义 `SLeafWidget`（`SReEchoMinimapCanvas`）在 `OnPaint` 中用 `FSlateDrawElement::MakeLines` / `MakeBox` 绘制矢量折线与点，分辨率无关、live 刷新成本低；优于动态 `UTexture2D` 位图重绘。
  - **是否需 WBP 资产**：照 `ReEchoEncounterHudWidget` 范式，Minimap 纯 C++ 建树，`UIManager` 注册时回退到 `UReEchoMinimapWidget::StaticClass()`，**无需新建 WBP 资产**，避免引入二进制资源。
  - **坐标映射**：世界平面为 X/Y（Z 为高度，忽略）。`Local = (WorldXY - ArenaCenter.XY) / (2 * ArenaHalfExtents) * MinimapSize + MinimapSize/2`，clamp 到 `[0, MinimapSize]`。`ArenaCenter` / `ArenaHalfExtents` 取自 `AReEchoPlayerPawn`（已由 `ConfigureArenaBounds` 配置），无需遍历场景。
  - **创建触发点**：在 `ReEchoGameMode.cpp` 创建 `EncounterHud` 同处（约 line 500）调用 `UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::Minimap, false, false)`，存为成员，初始 `SetVisibility(Collapsed)` 至遭遇开始（照 `PlayerHud` 模式）。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：若提及回响轨迹地面渲染，需更新为「小地图」。
  - `shared/CODEBASE_MAP/README.md`：若 AREA-UI 列有轨迹说明，需更新。
  - 各 `modules/MOD-*.md`：仅当明确描述 `AReEchoTrajectoryActor` 时才需更新；否则注明「不相关」。
- 关闭前逐项填写审阅结果：
  - `<文档>` 已更新：列出与验收实现一致的具体变化；
  - `<文档>` 已审阅、无需修改：说明该实现为何不改变此文档中的事实。

## 锁定验收

- [ ] 功能结果有可观察证据（PIE 中右上角小地图显示轨迹折线 + 玩家/回响点，世界地面无立方体轨迹）。
- [ ] 必需自动化/构建检查通过（`Build-Editor` 退出码 0，`validate_project.py` 全绿）。
- [ ] 仅在手感、可读性、视觉质量或可用性需要判断时请求人工验收（本 Plan 需人工 PIE 验收）。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径（本 Plan 不产生新二进制资源）。

## Step 0 门禁

- 基线分支/提交：`origin/main @ 07fd624`。
- 引擎/构建可用性：需 UnrealEditor 关闭后跑 `Build-Editor.cmd`（构建/导入类引擎操作纪律）。
- 现有聚焦测试结果：实现前先跑 `python scripts/validate_project.py` 与现有 `ReEcho.UI` / `ReEcho.UIManager` 自动化确认基线绿。
- 共享契约 / 难合并资源风险：删除 `AReEchoTrajectoryActor` 为二进制无关的头/源删除；`EReEchoUIScreen` / `EReEchoUILayer` 枚举为稳定身份，新增项向后兼容。
- 基线损坏时的停止条件：`Build-Editor` 失败或 `validate_project.py` 报 prebuilt/schema 不一致时停止并报告，不强行推送。

## 实现提纲

1. **确认引用面**：`search_content` 全仓确认 `AReEchoTrajectoryActor` 仅被 `ReEchoEchoActor` 引用（含测试/蓝图），决定整体删除还是仅停用。
2. **枚举扩展**：在 `ReEchoUIScreenTypes.h` 的 `EReEchoUIScreen` 加 `Minimap`；`EReEchoUILayer` 加 `Minimap`（z-order 高于 `GameplayHud`）。
3. **UI 注册**：`ReEchoUIManagerSubsystem.cpp` 中
   - `ScreenClasses.Add(EReEchoUIScreen::Minimap, UReEchoMinimapWidget::StaticClass());`
   - `GetScreenLayer` 加 `case EReEchoUIScreen::Minimap: return EReEchoUILayer::Minimap;`
   - `GetLayerZOrder` 加 `EReEchoUILayer::Minimap` 的 z 值（确认不与既有层冲突）。
   - 若 `GetScreenLayer` / `GetLayerZOrder` 为 exhaustive（带 `default: checkf` 或 `static_assert`），补对应 case，否则编译失败。
4. **Minimap Widget（新文件）**：
   - `UReEchoMinimapWidget : public UUserWidget`：`RebuildWidget` 建 `SBox`（固定尺寸，如 220×220）包裹 `SReEchoMinimapCanvas`；`NativeTick` 节流 ~20Hz 读取 `AReEchoGameMode` 的 `Player` / `Echoes`，重算并 `Invalidate(EInvalidateWidget::Paint)`。
   - `SReEchoMinimapCanvas : public SLeafWidget`：`OnPaint` 用 `FSlateDrawElement` 画：竞技场轮廓矩形 + 各回响轨迹折线（颜色按回响索引）+ 玩家点 + 回响点。
   - 暴露 `MapWorldToLocal(const FVector& WorldPos) const` 纯函数（坐标映射公式），便于单测。
   - 从 `UGameplayStatics::GetGameMode` 取 `AReEchoGameMode` 读取 `Player->GetActorLocation()` 与 `Echoes[i]->GetActorLocation()`；空数组安全跳过。
5. **GameMode 接线**：`ReEchoGameMode.cpp` 创建 `EncounterHud` 同处加 `MinimapWidget = UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::Minimap, false, false)`；`ReEchoGameMode.h` 加 `UPROPERTY() TObjectPtr<UReEchoMinimapWidget> MinimapWidget;`，初始 `SetVisibility(Collapsed)` 至遭遇开始（照 `PlayerHud`）。
6. **移除世界轨迹**：`ReEchoEchoActor.cpp::InitializeEcho` 删除 lines 109-120（`TActorIterator<AReEchoTrajectoryActor>` 销毁 + `SpawnActor` + `InitializeTrajectory`）；`ReEchoEchoActor.h` 移除 `#include "Graybox/ReEchoTrajectoryActor.h"` 与 `Trajectory` 成员；确认无其他引用后删除 `ReEchoTrajectoryActor.h/.cpp`。
7. **布局（右上角）**：`UReEchoMinimapWidget` 锚定右上 (1,0)，位置 `(-28, 28 + EncounterHudHeight + Gap)`，置于 `EncounterHud` 下方，尺寸 220×220，留出 margin，避免遮挡。
8. **测试**：新增 `ReEchoMinimapTests.cpp`：
   - `MapWorldToLocal` 单测（中心→中心、边界→边角、越界 clamp）。
   - Minimap 屏幕可创建（`CreateScreen(EReEchoUIScreen::Minimap)` 非空）。
   - 若 `ReEchoUIManagerSubsystemTests.cpp` 的 enum switch 缺 `Minimap` case，补之。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | 项目/源码不变量通过（无数据变更，应已绿） |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0 |
| 发布门禁 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` + `prebuilt_editor.py update` | 全量构建成功，prebuilt 刷新 |
| 运行时行为变化时自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI` | Minimap/UI 报告通过 |
| 需要人工时 | 具名 PIE 任务 | 六场遭遇各跑一遍：世界无地面轨迹；右上角小地图显示轨迹折线+玩家/回响点且 live 移动；暂停/存档/死亡结算不崩、输入不锁；多回响时多条折线颜色区分 |

## 执行记录

### 变化

### 证据

### 剩余风险

- 删除 `AReEchoTrajectoryActor` 前须确认全仓（含蓝图、测试）无残留引用。
- `GetScreenLayer` / `GetLayerZOrder` 若 exhaustive 缺 case 会编译失败，需补。
- Minimap 读取 `GameMode` 的 `Player` / `Echoes` 须保证遭遇期间指针有效；空时安全跳过。
- 本 Plan 不改数据层，故不触发 `sync_xlsx_to_csv.py` 校验；但删除 C++ 类后仍需 `-FullRebuild` 作为发布门禁。

### 人工验收结果/请求

### 架构文档审阅结果
