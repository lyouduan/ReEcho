# Plan 69 - [PROGRAMMER] - 卡牌选择页卡牌图片与 Icon 接入

## 协调

- Planner 负责人：Gavyn-side AI（用户已确认程序身份）
- Executor 负责人：Gavynqiu
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`（C++/契约），资源导入与 WBP 槽位为 Editor 人工步骤（用户提供美术）。
- 任务状态：`Proposed`。
- 人工验收：`PendingBeforeClose`（视觉/美术需 PIE 人工确认）。
- 本地规划 / 实现基线：`origin/main` `72ac3f8`。
- 本地实现方式：独立 plan 工作树 `plan/69-trait-card-art`（目录 `ReEcho-plan69-trait-card-art`）。
- 依赖 / 阻塞：用户提供 39 张卡面图与 39 个 icon 资源（命名约定见下）；`WBP_ReEchoTraitCardEntry` 需新增两个 `UImage` 槽（Editor 步骤）。
- Writes：`Source/ReEcho`（`ReEchoTraitCardChoiceWidget`/`ReEchoTraitCardEntryWidget` + 解析辅助）、`Content/ReEcho/Textures/UI/Cards/{Art,Icon}/`（导入纹理，用户提供）、`Content/UI/.../WBP_ReEchoTraitCardEntry.uasset`（Editor 编辑）。
- Stable Reads：`ReEchoCards` 的 `FReEchoCardDefinition` / `FReEchoTraitCardOffer` 公共契约尽量不变（仅给 Offer 追加可选纹理字段）；`cards.csv` 架构不变。
- 影响模式：`Isolated`。
- 兼容承诺 / 下游操作：不修改 `cards.csv`/xlsx 卡牌平衡数据，不与 Plan47 耦合；不改变商店；`FReEchoTraitCardOffer` 追加可选 `UTexture2D` 字段，向后兼容；缺资源时回退通用图标而非崩溃/空框。
- 明确排除：商店卡牌 UI、Plan47 卡牌数据与 xlsx、命名图标自动替换、Boss/VFX、卡面大图自动生成。

## 锁定目标

玩家在卡牌选择页（`ReEchoTraitCardChoiceWidget`）看到每张候选卡的**卡面图（主视觉）**与**小 icon**。资源由用户提供，按卡 `Id` 约定命名，由 C++ 经 `LoadObject` 解析；缺失时 icon 回退到通用卡图标、art 隐藏（保留框），不崩溃。

## 架构影响与设计决策

- 受影响架构标识：`None`（不新建 Runtime Module；UI 视为 `MOD-ReEchoUI` 文档型边界内）。`MOD-ReEcho` / `MOD-ReEchoCards` 为读侧，契约不变。
- 对应模块文档：无需新建 `MOD-*`；`shared/CODEBASE_MAP/README.md` 若列 UI 资源目录可补一行（可选，不阻塞）。
- 设计意图：复用项目既有"武器贴图按键解析 + 通用回退"约定（`AReEchoSwordArcActor::ResolveWeaponTexturePath` / `AReEchoProjectileActor::ResolveWeaponTexturePath`），让 UI 资源与卡牌平衡数据解耦，避免触碰 `cards.csv`/xlsx 与 Plan47。
- 权威状态与依赖：不改变状态所有者、公共契约或依赖方向；仅向 `FReEchoTraitCardOffer` 追加可选字段。
- 决策记录：
  - 不新增 `cards.csv` 列 → 用卡 `Id` 约定文件名 + `LoadObject` 解析（与武器贴图一致），完全解耦卡牌数据与美术资源。
  - 不依赖 `命名图标/`（那是武器配件/属性/元素/角色图标，非卡面）。
  - entry 用 `BindWidgetOptional` 两个新槽，缺槽时降级（构建不依赖 WBP 先改）。
  - 通用回退：icon 缺失回退到商店既有 `T_UI_Shop_CardIcon`；art 缺失则隐藏 `ArtImage`（保留既有 `ArtCardFrame`）。
- 相关文档同步范围：`ARCHITECTURE.md` 无需改（UI 资源属已知边界）；`README.md` 可补 UI 资源目录（可选）。
- 关闭前逐项填写审阅结果：（待实现后填）

## 锁定验收

- [ ] 卡牌选择页出现每张候选卡的卡面图与小 icon（PIE 可观察）。
- [ ] 必需自动化/构建检查通过（`validate_project.py` + 增量 `Build-Editor`）。
- [ ] 仅资源缺失时请求人工验收（视觉质量）。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main` `72ac3f8`。
- 引擎/构建可用性：本地 `scripts/ue/Build-Editor.cmd -Configuration Development` 可用（编辑前需关闭 UnrealEditor.exe）。
- 现有聚焦测试结果：无新增逻辑冲突；卡牌选择页现有测试不受影响（仅追加纹理绑定）。
- 共享契约 / 难合并资源风险：`Isolated`；纹理资源在独立 `Content/ReEcho/Textures/UI/Cards/` 目录，不与 Plan45/60/47 资源重叠。
- 基线损坏时的停止条件：若 `Build-Editor` 因本 Plan 失败，停止并回退 worktree 改动，先修复 C++。

## 实现提纲

1. 资源解析（C++，新建静态辅助，镜像武器贴图约定）：
   - `UReEchoTraitCardChoiceWidget::ResolveCardArtTexturePath(FName CardId)` → `/Game/ReEcho/Textures/UI/Cards/Art/T_UI_Card_{CardId}.T_UI_Card_{CardId}`。
   - `UReEchoTraitCardChoiceWidget::ResolveCardIconTexturePath(FName CardId)` → `/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_{CardId}.T_UI_CardIcon_{CardId}`。
   - 调用点用 `LoadObject<UTexture2D>`；icon 失败回退 `T_UI_Shop_CardIcon`；art 失败返回 `nullptr`。
2. `ReEchoTraitCardEntryWidget`：新增 `BindWidgetOptional UImage* ArtImage` 与 `IconImage`；`Configure(...)` 末尾追加 `UTexture2D* CardArt = nullptr, UTexture2D* CardIcon = nullptr`（保留原参数）；绑定到槽（可选，`nullptr` 时隐藏）。
3. `FReEchoTraitCardOffer`：追加 `TObjectPtr<UTexture2D> CardArt; TObjectPtr<UTexture2D> CardIcon;`（可选，向后兼容）。
4. `ReEchoTraitCardChoiceWidget::RefreshOffers`：对每个 offer 解析纹理填入 Offer，再传给 `Entry->Configure(..., CardArt, CardIcon)`。
5. `WBP_ReEchoTraitCardEntry`（Editor 人工步骤）：新增两个 `UImage` 控件，命名 `ArtImage`（卡面主视觉，置于卡片主体）与 `IconImage`（角落小图标）。
6. 资源导入（Editor 人工步骤）：用户将 png 按约定放入 `Content/SourceArt/UI/Cards/{Art,Icon}/`，命名 `T_UI_Card_{CardId}.png` / `T_UI_CardIcon_{CardId}.png`，批量导入到 `Content/ReEcho/Textures/UI/Cards/{Art,Icon}/`（统一压缩/分组设置）。

### 资源命名契约（用户提供）

| 卡 `Id`（取 `cards.csv` 的 `Id` 列） | 卡面图文件名 | icon 文件名 |
|---|---|---|
| `G_1_01` … `G_1_08`（8 张） | `T_UI_Card_G_1_01.png` … | `T_UI_CardIcon_G_1_01.png` … |
| `G_2_04` … `G_2_17`（14 张） | 同上规则 | 同上规则 |
| `G_3_01` … `G_3_22`（14 张，跳过无数据项） | 同上规则 | 同上规则 |
| `FORGE_LIGHT` / `FORGE_MEDIUM` / `FORGE_EXTREME` | `T_UI_Card_FORGE_LIGHT.png` … | `T_UI_CardIcon_FORGE_LIGHT.png` … |

完整 39 个 `Id`：`G_1_01`,`G_1_02`,`G_1_03`,`G_1_04`,`G_1_05`,`G_1_06`,`G_1_07`,`G_1_08`,`G_2_04`,`G_2_05`,`G_2_06`,`G_2_07`,`G_2_08`,`G_2_09`,`G_2_10`,`G_2_12`,`G_2_13`,`G_2_14`,`G_2_15`,`G_2_16`,`G_2_17`,`G_3_01`,`G_3_02`,`G_3_03`,`G_3_04`,`G_3_05`,`G_3_07`,`G_3_09`,`G_3_10`,`G_3_11`,`G_3_12`,`G_3_13`,`G_3_14`,`G_3_16`,`G_3_17`,`G_3_19`,`G_3_20`,`G_3_21`,`G_3_22`,`FORGE_LIGHT`,`FORGE_MEDIUM`,`FORGE_EXTREME`。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | 项目/源码不变量通过；Plan 文件存在且格式合规 |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0 |
| 运行时行为变化时自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | 受影响报告通过（卡牌选择现有测试） |
| 需要人工时 | 具名 PIE：进入卡牌选择页，确认 39 卡均有卡面图与 icon，缺资源项回退不崩溃 | 记录人工结果或明确延期跟进 |

## 执行记录

### 变化

（待实现）

### 证据

（待实现）

### 剩余风险

（待实现）

### 人工验收结果/请求

（待实现）

### 架构文档审阅结果

（待实现）
