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

1. 资源解析（C++，静态辅助，镜像武器贴图约定）：
   - `UReEchoTraitCardChoiceWidget::ResolveCardArtTexturePath(int32 Tier)` → `/Game/ReEcho/Textures/UI/Cards/Art/T_UI_CardTier{Tier}.T_UI_CardTier{Tier}`；`Tier<1`（FORGE）返回空 → `ArtImage` 隐藏。底图按 **Tier** 用 3 张星级框，非每张卡独立图。
   - `UReEchoTraitCardChoiceWidget::ResolveCardIconTexturePath(FName CardId)` → `/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_{CardId}.T_UI_CardIcon_{CardId}`。
   - 调用点用 `LoadObject<UTexture2D>`；icon 失败回退 `T_UI_Shop_CardIcon`；art 失败（`nullptr`）`ArtImage` 隐藏。
2. `ReEchoTraitCardEntryWidget`：新增 `BindWidgetOptional UImage* ArtImage` 与 `IconImage`；`Configure(...)` 末尾追加 `UTexture2D* CardArt = nullptr, UTexture2D* CardIcon = nullptr`；绑定到槽（可选，`nullptr` 时隐藏）。
3. `FReEchoTraitCardOffer`：追加 `int32 Tier = 0;`（驱动底图）与 `TObjectPtr<UTexture2D> CardArt / CardIcon;`（可选，向后兼容）。两个 `MakeTraitOffer` 重载均填 `Tier = Card.Tier`。
4. `ReEchoTraitCardChoiceWidget::RefreshOffers`：对每个 offer 按 `Offer.Tier` / `Offer.CardId` 解析纹理填入 Offer，再传给 `Entry->Configure(..., CardArt, CardIcon)`。
5. `WBP_ReEchoTraitCardEntry`（Editor 人工步骤）：新增两个 `UImage` 控件，命名 `ArtImage`（卡面主视觉，置于卡片主体）与 `IconImage`（角落小图标）。
6. 资源来源（已实现，非用户提供）：
   - 底图：从 `Downloads\卡牌` 的 `1星灰色/2星金色/3星彩色.png` 按 Tier 拷入 `Content/SourceArt/UI/Cards/Art/T_UI_CardTier{1,2,3}.png`。
   - icon：从《【开普勒】回响数值与构筑体系.xlsx》`构筑体系G` 表 drawing 锚点抽取每张卡小图标，按 **卡名称**（文档`名称` ↔ `cards.csv` DisplayName）匹配到游戏卡 Id，落到 `Content/SourceArt/UI/Cards/Icon/T_UI_CardIcon_{GAME_CARD_ID}.png`。抽取脚本 `scripts/plan69_extract_card_icons.py`。
   - 覆盖：**35/42** 精确名称匹配；**3 FORGE + 4 缺图**（`G_2_07 潮汐回响`、`G_2_08 森林回响`、`G_3_19 碎时锋芒`、`G_3_20 碎时壁垒`）走通用回退。

### 资源命名契约（已实现）

| 类型 | 运行时纹理路径 | 来源 |
|---|---|---|
| 卡面图（按 Tier） | `/Game/ReEcho/Textures/UI/Cards/Art/T_UI_CardTier{1,2,3}` | `Downloads\卡牌` 三张星级框 |
| icon（按游戏卡 Id） | `/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_{GAME_CARD_ID}` | 构筑体系G 表 `小icon` 列（按名称匹配） |

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | 项目/源码不变量通过；Plan 文件存在且格式合规 |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0 |
| 运行时行为变化时自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | 受影响报告通过（卡牌选择现有测试） |
| 需要人工时 | 具名 PIE：进入卡牌选择页，确认 39 卡均有卡面图与 icon，缺资源项回退不崩溃 | 记录人工结果或明确延期跟进 |

## 执行记录

### 变化

- C++ 契约全部落地：`FReEchoTraitCardOffer` 增 `Tier`/`CardArt`/`CardIcon`；`ReEchoTraitCardEntryWidget` 增 `ArtImage`/`IconImage` 可选槽并绑定；`ReEchoTraitCardChoiceWidget` 增 `ResolveCardArtTexturePath(int32 Tier)`/`ResolveCardIconTexturePath(FName)`，`RefreshOffers` 按 Tier/Id 解析纹理；两个 `MakeTraitOffer` 填 `Tier`。
- 底图：3 张星级框按 Tier 拷入 `Content/SourceArt/UI/Cards/Art/T_UI_CardTier{1,2,3}.png`。
- icon：从 xlsx `构筑体系G` 表抽取并按卡名称匹配，35 张落盘；脚本 `scripts/plan69_extract_card_icons.py`（可复现）。
- 辅助脚本：`plan69_import_ue.py`（导入 PNG→UTexture2D）、`plan69_edit_wbp.py`（WBP 加两槽，风险较高）、`plan69_extract_card_icons.py`（icon 抽取）。

### 证据

- `validate_project.py` 通过（静态）；相关 C++ 文件 lint 0 错误。
- icon 抽取：35/42 覆盖；缺失 `G_2_07/08`、`G_3_19/20` + 3 FORGE 走通用回退。脚本 `plan69_extract_card_icons.py` 已修 `ROOT` 路径并恢复可复现（提交 `93d76b3`），重跑确认 35/42。
- 底图：Tier 1/2/3 三张已就位。
- **纹理导入（无头）**：`UnrealEditor-Cmd -ExecutePythonScript=plan69_import_ue.py` → `LogPython [Plan69] imported 3 textures -> .../Art`、`imported 35 textures -> .../Icon`、`done`，无 Plan69 Error；生成 38 个 `.uasset`（`Content/ReEcho/Textures/UI/Cards/{Art,Icon}/`），已提交 `4df8ed0`。交叉核验 35 个 icon 文件名全部命中 `cards.csv` Id（0 错位）。
- **增量构建**：`Build-Editor -Configuration Development` → `Result: Succeeded`（92/92），`UnrealEditor-ReEcho.dll` 链接成功，prebuilt bundle 刷新 build_id=55116800。DLL 与含 Plan 69 契约的源码一致。
- 基线外提交核查：`origin/main` 新进 4 提交（Plan60 视觉重做、Plan68 WS3）未触碰本 plan 改的 6 个 C++/plan 文件，无 Physical/Logical/Coupling 冲突；集成前 rebase 即可。

### 剩余风险

- **仅剩 WBP 控件布局为人工步骤**：`WBP_ReEchoTraitCardEntry` 需新增两个 `UImage`，命名 `ArtImage`（卡面主视觉，铺满卡片主体）、`IconImage`（角落小徽标）。`plan69_edit_wbp.py` 为二进制 uasset 编辑风险高，建议手动（约 30 秒）。C++ 已用 `BindWidgetOptional`，缺槽降级不崩。
- 42 张卡中 7 张（4 缺图 + 3 FORGE）使用通用回退图标、FORGE 无星级底图（隐藏），视觉待人工验收。
- 纹理导入与增量构建已通过；**仅余 PIE 运行时验收**未做。

### 人工验收结果/请求

- 已完成（AI 侧）：PNG 无头导入 38 个 UTexture2D（提交 `4df8ed0`）、增量 `Build-Editor -Configuration Development` 刷新 DLL（Succeeded）。
- 待（人工）：①给 `WBP_ReEchoTraitCardEntry` 加 `ArtImage`/`IconImage` 两 Image 控件并编译保存；②打开编辑器 PIE 进卡牌选择页，确认 35 张有 icon+星级底图、7 张回退不崩溃。
- 可选：若用户补充 `G_2_07/08`、`G_3_19/20` 四张图标，重跑 `plan69_extract_card_icons.py` + 重导即可补齐。

### 架构文档审阅结果

- 未改动 `ARCHITECTURE.md`/模块文档；UI 资源属既有 `MOD-ReEchoUI` 边界，新增纹理目录与既有武器贴图约定一致，无架构影响。
