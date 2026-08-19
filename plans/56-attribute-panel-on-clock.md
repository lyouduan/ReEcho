# Plan 56 - 程序 - 时钟悬停属性面板

## 协调

- Planner 负责人：`Gavyn-side AI`（用户已明确程序身份，AI 兼任 Planner + 秘书）
- Executor 负责人：`Gavyn-side AI`
- Plan 编写方（AI 侧）：`Gavyn-side AI`
- 实现编写方（AI 侧）：`Gavyn-side AI`
- 任务状态：`Proposed`
- 人工验收：`PendingBeforeClose`（需 PIE 验证悬停面板内容与数值）
- 本地规划 / 实现基线：`origin/main` @ `bb7eeea`（Plan55 enemy facing 合并后）
- 本地实现方式：实现落在独立 worktree `ReEcho-plan56-attribute-panel`（分支 `plan/56-attribute-panel`），主工作树只接收 fast-forward 合并到 `main`。
- 依赖 / 阻塞：
  - 依赖策划真源 `Design/Data/ReEchoData.xlsx` 新增 `tblAttributes` 导出表（数据任务，由程序经脚本写入并 commit）。
  - 依赖 `命名图标/属性S` 下 8 张属性 PNG 导入 `Content/ReEcho/Textures/UI/Attributes/`。
- Writes：
  - `Design/Data/ReEchoData.xlsx`（新增 `tblAttributes` 表）
  - `Content/Data/Attributes.csv`（新增，由 sync 生成）
  - `scripts/data/sync_xlsx_to_csv.py`（TABLE_TO_CSV + 校验 schema）
  - `Source/ReEcho/Public/Data/ReEchoCsvDataRegistry.h`（新增 `FReEchoCsvAttributeRow` + 注册表条目）
  - `Source/ReEcho/Private/Data/ReEchoCsvDataRegistry.cpp`（解析 + 注册）
  - `Source/ReEcho/Public/UI/ReEchoInventoryShopWidget.h` / `.cpp`（新增属性面板构建 + 挂时钟 tooltip + 接收玩家属性块）
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`（向商店组件传入当前玩家 `FReEchoStatBlock`）
  - `Content/ReEcho/Textures/UI/Attributes/*`（8 张导入纹理）
  - 预构建 DLL + `ReEchoEditor.prebuilt.json`（实现后 `-FullRebuild` 刷新）
- Stable Reads：
  - `Source/ReEchoCombat/Public/Combat/ReEchoCombatTypes.h`（`FReEchoStatBlock` 字段契约，只读消费）
  - `Source/ReEcho/Private/UI/ReEchoStatsWidget.cpp`（玩家属性快照获取方式，参考同源）
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`（UI 职责边界）
- 影响模式：`SharedContract`（新增一个数据域契约 `Attributes`，并扩展 UI 组件公开方法 `SetPlayerStats`；不改动既有战斗/数值计算逻辑）
- 兼容承诺 / 下游操作：
  - 不改 `FReEchoStatBlock` 字段、不改战斗数值计算；属性面板只读消费运行时快照。
  - 不改动 `属性S`（ReferenceOnly 字典）与 `LOCKED_REFERENCE_SHEETS` 契约；新增独立导出表，避免误伤策划参考表。
- 明确排除：
  - 不新增运行时属性计算；`暴击效果`/`元素反应效能` 等二级属性数值直接复用 `FReEchoStatBlock` 现有字段（`CriticalEffect`/`ReactionEfficiency`），不扩展结构体。
  - 不实现点击展开/常驻面板，仅悬停 tooltip。
  - 不接入 Boss/Trait 之外的属性源；仅展示"角色一级 + 二级属性"（即 `属性S` 中 `属性等级 ∈ {角色一级属性, 角色二级属性}`，等价于用户所称"icon 不为空"的 8 个属性）。

## 锁定目标

玩家在商店屏幕（`WBP_ReEchoInventoryShopScreen`）将鼠标移到 `DesignerShopClock` 时钟控件时，弹出属性面板：面板按数据驱动顺序列出角色一级与二级属性，每行左侧为属性图标，右侧为属性名称 + 当前真实数值（来自当前玩家构建的 `FReEchoStatBlock`）。面板要展示哪些属性、其名称/图标/层级/排序，全部来自 `ReEchoData.xlsx` 属性表（`属性S`），仅取 icon/属性非空的一级+二级属性。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoUI`（新增面板构建与 tooltip 装配）、`MOD-ReEcho`（CSV 注册表新增 `Attributes` 域）。其余 `MOD-*` 不受影响（只读消费）。
- 对应模块文档：`MOD-ReEchoUI.md` 需补充"属性面板（时钟 tooltip）"条目；`MOD-ReEcho.md`（数据注册表）需补充 `Attributes` 域。均加入 Writes。
- 设计意图：
  - 满足用户"面板参数来自 xlsx 属性表"的明确要求，且不违反"已迁移领域数值不得复制进 C++/JSON/配置/UI"的硬规则。
  - 属性目录（名称/图标/层级/排序/数值种类）为数据驱动，经 `xlsx → csv → 类型化不可变快照` 管线；当前真实数值为运行时只读快照（`FReEchoStatBlock`），C++ 不持有任何权威数值。
  - UI 仅消费只读摘要与事件、发送受控命令，符合 `MOD-ReEchoUI` 职责边界。
- 权威状态与依赖：本 Plan 不改变任何状态所有者；`FReEchoStatBlock` 仍为战斗权威数值源，UI 仅读取。
- 决策记录：
  - **决策 A（数据域落地方式）**：在 `ReEchoData.xlsx` 新增独立 `tblAttributes` 导出表（而非改动 `属性S` ReferenceOnly 表）。理由：`属性S` 被 `sync_xlsx_to_csv.py` 列为 `LOCKED_REFERENCE_SHEETS` 且标注"不参与 CSV 导出"；新增独立表既保留策划参考字典不被误改，又提供干净的生产 CSV。代价：属性名称/层级等结构性信息在 `tblAttributes` 与 `属性S` 间存在"同源两份"，但均为展示/结构数据，不含权威战斗数值，不违反数值不重复规则。
  - **决策 B（数值来源）**：8 个一级/二级属性全部对应 `FReEchoStatBlock` 现有字段——`S_HP→HpMax`、`S_P_Attack_Power→PhysicalAttack`、`S_E_Attack_Power→ElementalAttack`、`S_Movement_Speed→MovementSpeed`、`S_Critical_Hit_Rate→CriticalRate`、`S_Critical_Hit_Effect→CriticalEffect`、`S_Echo_Efficiency→EchoEfficiency`、`S_Elemental_Reaction_Efficiency→ReactionEfficiency`。百分比类（`MovementSpeed/CriticalRate/CriticalEffect/EchoEfficiency/ReactionEfficiency`）以 0–1 存储，展示时 ×100 加 `%`；绝对类（`HpMax/PhysicalAttack/ElementalAttack`）按整数展示。理由：避免扩展结构体、不动战斗逻辑，全部数值已有运行时来源。替代方案（扩展 `FReEchoStatBlock` 加二级属性字段）被排除——当前字段已覆盖，新增会引入无谓耦合。
  - **决策 C（图标加载）**：`命名图标/属性S` 下 PNG 命名为属性名称（如 `生命值.png`），导入 `Content/ReEcho/Textures/UI/Attributes/` 后资产名即属性名称；CSV `IconName` 列存该资产名，C++ 以软路径 `/Game/ReEcho/Textures/UI/Attributes/<IconName>` 加载。理由：与 `命名图标` 命名直接对应，无需额外映射表。
  - **决策 D（面板实现形式）**：采用与既有 `BuildSlotTooltip` 一致的纯 C++ 程序化 tooltip（Border + VerticalBox 行），不新增 WBP。理由：与现有商店 tooltip 同构、最小文件面、符合 MOD-ReEchoUI "C++ 管只读展示/布局构建" 约定。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：需在数据域清单补 `Attributes`（如确为新增域）。
  - `shared/CODEBASE_MAP/README.md`：AREA 不变，无需改（如后续需要再补）。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：补充属性面板条目 → Writes。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：补充 `Attributes` CSV 域 → Writes。
  - `Design/UI/ReEcho_UI修改指导.md`：如有控件绑定约定需补 `DesignerShopClock` 的 tooltip 约定 → 视情况 Writes。
- 关闭前逐项填写审阅结果：（实现后填写）

## 锁定验收

- [ ] 商店屏幕悬停 `DesignerShopClock` 弹出属性面板，列出 8 个一级+二级属性（与 `属性S` 一级/二级行一致）。
- [ ] 每行左侧图标与属性名称对应（`命名图标/属性S` 映射），无错位/缺失。
- [ ] 数值为当前构建的真实值（与 Tab 打开的 `StatsWidget` 一致），百分比类显示为整数 + `%`。
- [ ] 面板参数（属性集合/名称/图标/排序）来自 `Attributes.csv`，非 C++ 硬编码数值。
- [ ] `python scripts/validate_project.py` 通过；`-FullRebuild` 退出码 0。
- [ ] 未提交预构建允许列表之外的产物。

## Step 0 门禁

- 基线分支/提交：`origin/main` @ `bb7eeea`
- 引擎/构建可用性：待实现前确认 `Build-Editor.cmd` 可用（复用 Plan54 已验证环境）
- 现有聚焦测试结果：无（本 Plan 不触及战斗逻辑）
- 共享契约 / 难合并资源风险：`ReEchoData.xlsx` 二进制 + 预构建 DLL 为常见耦合点；实现在独立 worktree，合并前以远端为准重建二进制（`属性S` ReferenceOnly 不动，降低风险）。
- 基线损坏时的停止条件：`-FullRebuild` 非 0 或 `validate_project.py` 报错则停止推送，回 worktree 修复。

## 实现提纲

1. **数据层**
   - 1.1 在 `ReEchoData.xlsx` 新增表 `tblAttributes`（新 sheet，列：`Id`(属性词条代码)、`DisplayName`(属性名称)、`Tier`(1/2)、`IconName`(资产名=属性名称)、`ValueKind`(0=绝对,1=百分比)、`DisplayOrder`、`Explanation`）。数据取自 `属性S` 一级/二级 8 行。
   - 1.2 `sync_xlsx_to_csv.py`：`TABLE_TO_CSV` 增加 `Attributes` 映射与 schema 校验；运行生成 `Content/Data/Attributes.csv`。
   - 1.3 `ReEchoCsvDataRegistry.h/.cpp`：新增 `FReEchoCsvAttributeRow`、`TMap<FName,...> Attributes`、`AttributeOrder`、`FindAttribute`；接入 `FReEchoCsvDataSnapshot`。
2. **资源层**
   - 2.1 将 `命名图标/属性S` 下 8 张 PNG 导入 `Content/ReEcho/Textures/UI/Attributes/`（资产名=属性名称）。
3. **UI 层**
   - 3.1 `ReEchoInventoryShopWidget.h`：新增 `CurrentPlayerStats`（存储）、`SetPlayerStats(const FReEchoStatBlock&)`、`BuildAttributePanel(const FReEchoStatBlock&) -> UWidget*`（或复用现有 tooltip 构建风格）。
   - 3.2 `ReEchoInventoryShopWidget.cpp`：实现 `BuildAttributePanel`——遍历 `FReEchoCsvDataRegistry::GetSnapshot()->Attributes`（按 `Tier` 然后 `DisplayOrder`），每行 `[UImage(<IconName>)] [UTextBlock("DisplayName：Value")]`；`Value` 由 `AttributeId→字段` 小映射 + `ValueKind` 格式化得出。在 `SetPlayerStats` 内对 `DesignerShopClock`（`GetWidgetFromName<UImage>`）调用 `SetToolTip(BuildAttributePanel(Stats))`。
   - 3.3 `ReEchoGameMode.cpp`：在 `ToggleShopMenu` 创建 `InventoryShopWidget` 后，调用 `InventoryShopWidget->SetPlayerStats(Player->Combatant->Stats)`（与 `StatsWidget->InitializeStats` 同源）。
4. 每次风险变更后立即验证（见验证矩阵）。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 数据 | `python scripts/data/sync_xlsx_to_csv.py` | 生成 `Content/Data/Attributes.csv`，schema 通过 |
| 静态 | `python scripts/validate_project.py` | 项目/源码不变量通过（含 CSV schema/UTF-8） |
| C++ 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 退出码 0，刷新预构建 |
| 运行/人工 | PIE：进入一局→按 M 开商店→悬停 `DesignerShopClock` | 弹出 8 属性面板，图标/名称/数值正确 |
| 变更面 | `git diff --check` | 无空白/行尾问题 |

## 执行记录

### 变化

（实现时填写）

### 证据

（实现时填写）

### 剩余风险

- `ReEchoData.xlsx` 二进制合并：因 `属性S` ReferenceOnly 不动、仅新增 `tblAttributes` 表，冲突面小；合并前以远端为准重建二进制。
- 中文资产名加载：若 UE 资产名含中文导致软路径失败，回退为 C++ 内置 `AttributeId→已知纹理路径` 映射（同 `MOD-ReEchoUI` 既有图标加载风格）。

### 人工验收结果/请求

（实现后填写）

### 架构文档审阅结果

（实现后填写）
