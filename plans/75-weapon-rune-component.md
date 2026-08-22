# Plan 75 - 程序 - 武器符文组件（Weapon Rune Component）

## 协调

- Planner 负责人：Gavyn（程序，Planner-Executor 模式）；本 Plan 由 AI 承担 Planner 职责。
- Executor 负责人：待分配（实现阶段在独立工作树进行）。
- Plan 编写方（AI 侧）：`Gavyn-side AI | ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Unassigned`（设计评审通过后分配）。
- 任务状态：`Proposed`。
- 人工验收：`PendingBeforeClose`（符文装配/卸下/存盘恢复需 PIE 验收手感与正确性）。
- 本地规划 / 实现基线：待发布时从 `origin/main` 最新提交开出独立工作树 `ReEcho-plan75-weapon-rune-component`（分支 `plan/75-weapon-rune-component`）。本文件当前暂存于 `ReEcho-plan67-shop-drop` 工作树供评审，遵循 plan 工作树纪律，不污染主工作树 `ReEcho/main`。
- 本地实现方式：实现期在 `ReEcho-plan75-weapon-rune-component` 工作树增量 `Development` 构建；推 `origin/main` 前才跑 `-FullRebuild` 发布门禁。
- 依赖 / 阻塞：
  - 下游依赖：Plan67（商店投放重构）消费本 Plan 的 `AReEchoWeaponActor::EquipRune` 接口——"购买符文 = 买到组件（填入 3 槽之一）"。本 Plan 必须先合入，Plan67 才能接。
  - 无代码阻塞性依赖；数据层（parts.csv / slot_profiles.csv）已就绪。
- Writes：`Source/ReEcho/Public/Weapons/ReEchoWeaponActor.h`、`Source/ReEcho/Private/Weapons/ReEchoWeaponActor.cpp`、`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`（若需补武器装配职责说明）、本 Plan 文档。
- Stable Reads：`Source/ReEcho/Private/Weapons/ReEchoWeaponRuntime.cpp`（`TryEquipParts`/`ApplyPartEffect`/`BuildEffectiveWeaponDefinition`/`IsPartCompatibleWithWeapon`/`FindEffectiveSlotLimit`）、`Source/ReEcho/Public/Data/ReEchoCsvDataRegistry.h`（`FReEchoCsvPartRow`/`FReEchoEquippedPartSnapshot`/`FReEchoBuildSnapshot`/`FReEchoCsvSlotProfileRow`）、`Source/ReEchoWeapons/Public/Weapons/ReEchoWeaponLogic.h`、`Source/ReEchoWeapons/Public/Weapons/ReEchoWeaponTypes.h`。
- 影响模式：`SharedContract`（改动 `AReEchoWeaponActor` 的公共装配接口，主模块内契约变化；`ReEchoWeapons` 纯逻辑层**不改动**）。
- 兼容承诺 / 下游操作：保留 `FReEchoBuildSnapshot::EquippedParts` 作为存盘/回放唯一真值；新增的 `EquippedRunes` 成员必须与 `BuildSnapshot.EquippedParts` 在存盘点保持同步，保证存档恢复与回放重建不被破坏。下游（Run 子系统、商店、UI）改为经 actor 的 `EquipRune`/`UnequipRune` 接口操作符文，而非直接写 `BuildSnapshot.EquippedParts`。
- 明确排除：
- 不在 `ReEchoWeapons` 模块内实现装配或 Part Effect 应用（架构禁止，见设计意图 D3）。
- 绝不从 xlsx 读取 hidden 行/列作为有效符文数据（数据读取铁律，见设计意图 D6）。
  - 不为每个表项/每个符文建 C++ 子类（数据驱动铁律，见 D2）。
  - 不改动商店投放数值逻辑（属 Plan67，本 Plan 只暴露消费接口）。
  - 不为符文新增独立 `UActorComponent` 类（用户决策：数组直接作 `AReEchoWeaponActor` 数据成员，见 D3）。

## 锁定目标

玩家（及系统）对"武器 + 符文"的对外表现是一体的：**装了符文 A 的武器，对外表现就是"装了符文 A 的武器"**，而不是"武器本体"再临时把符文 A 叠算上去。具体交付：

1. 每把武器持有**三个符文槽**（1 核心 Core + 2 武器专属命名槽，如弓=弓弦+箭头），以 `AReEchoWeaponActor` 上的 `EquippedRunes` 数组（长度 3，按 `SlotTypeId` 索引）为活权威。
2. 提供装配/卸下接口 `EquipRune(PartId)` / `UnequipRune(SlotTypeId)`，含槽容量与武器兼容性校验（复用现有 `FindEffectiveSlotLimit` / `IsPartCompatibleWithWeapon`）。
3. 任何符文变动后，武器重新编译出"已含符文的有效武器定义"（复用现有 `BuildEffectiveWeaponDefinition`），战斗侧看到的始终是统一后的有效武器，符文效果不另算。
4. 符文状态可存盘、可回放重建：`EquippedRunes` 在存盘点回写 `BuildSnapshot.EquippedParts`。

## 架构影响与设计决策

- 受影响架构标识：
  - `MOD-ReEcho`（武器 Actor 装配职责所在，本 Plan 主改动模块）。
  - `MOD-ReEchoWeapons`：**不受影响**，保持纯逻辑、无 UObject/数据依赖；本 Plan 明确不把装配/Effect 下沉进该模块。
  - `MOD-ReEchoCombat`：只读有效武器定义，契约不变。
- 对应模块文档：`MOD-ReEcho.md` 需补"武器符文装配（Actor 层）"职责段，确认已加入 `Writes`；`MOD-ReEchoWeapons.md` 审阅、无需修改（纯逻辑边界不变）。
- 设计意图：
  - **D1 范围分层**：武器符文系统本应是武器模块的本职，商店只是下游消费者。先把它作为独立 Plan 设计清楚，再让 Plan67 商店接"购买=填槽"，避免商店逻辑反过来定义符文模型。
  - **D2 数据驱动（非继承）**：本项目铁律"数值单源 = xlsx→CSV，不得硬编码 C++/JSON"（规则 + 经验库 `GAME-25`/`GAME-33`）。符文效果是同构数据（`EffectKind`/`Target`/`ValueOp`/`Value`），由现有 `ApplyPartEffect` 解释器统一求值；特殊行为走 `BehaviorId`（`ReEchoCsvDataRegistry::RegisterBehaviorId`，代码已见 `Part.OnKillHealPercent`）。每表项一个子类 = 70 个近义虚方法、改数重编译、破回放重建，故否决。
  - **D3 落点 = Actor 直接成员（非独立组件类 / 非 Weapons 模块）**：用户决策——3 槽数组直接作为 `AReEchoWeaponActor` 数据成员。因 Actor 在主模块 `ReEcho`，调用 `ReEchoWeaponRuntime::ApplyPartEffect` / `BuildEffectiveWeaponDefinition` 不跨模块边界，故可"全包"（持有数组 + 装配/卸下校验 + 触发 Effect 编译）且完全合规。否决 (a) 新建独立 `UReEchoWeaponRuneComponent`——无必要边界；(b) 放进 `ReEchoWeapons` 的 `FReEchoWeaponLogic`——会破该模块"不拥有装配/Effect"的架构铁律。
  - **D4 "一体"身份**：符文效果在**编译期**烤进 `FReEchoEffectiveWeaponDefinition`（现有 `BuildEffectiveWeaponDefinition` 已如此：读 `Build.RuleFlags`/基础 stats 产出有效定义），战斗只消费统一后的有效武器，符文不于攻击时另算。本 Plan 强化此契约：actor 符文变动必须触发 `RebuildEffectiveDefinition`，使"一体"恒成立。
  - **D5 持久真值单一写者**：`FReEchoBuildSnapshot::EquippedParts` 仍是跨局存盘/回放真值（确定性重建依赖它）。actor 的 `EquippedRunes` 是**局内活权威**；阶段化单写者——加载时从 `BuildSnapshot.EquippedParts` 读入 `EquippedRunes`；局内变动只改 `EquippedRunes`；存盘/检查点时由 `EquippedRunes` 回写 `BuildSnapshot.EquippedParts`。避免双真值漂移。
  - **D6 数据读取铁律（hidden = 废除）**：openpyxl 读 `开普勒表`/`ReEchoData.xlsx` 时 `iter_rows` 含隐藏行，`ws.row_dimensions[r].hidden==True` 或 `ws.column_dimensions[c].hidden==True` 的行/列一律视为**废除(retired)数据，必须排除，绝不发布进 CSV**（Gavyn 2026-08-22：开普勒表所有 hidden 项都是废除的，全局规则）。注意 `sync_xlsx_to_csv.py` 当前**无 hidden 过滤**（按 `table.ref` 遍历 `min_row+1..max_row`），现有数据干净靠 Plan61 人工删废案 sheet，不靠过滤。本 Plan 及下游 Plan67（从开普勒表导入「投放系统」等新 sheet）任何读 xlsx 脚本必须在读取层显式跳过 hidden 行/列，以 Excel 实际可见视图为准。
- 权威状态与依赖：不改变存档/回放状态所有者（`BuildSnapshot` 仍为 Run 子系统持有）；不改变模块依赖方向（`ReEchoWeapons` 仍不反向依赖主模块）。
- 决策记录：
  - 替代方案 (1) 每表项继承子类：否决（D2 理由：破数据驱动铁律、70 类、回放重建碎）。
  - 替代方案 (2) 独立 `UReEchoWeaponRuneComponent`（UActorComponent）：否决（用户选 Actor 直接成员；且现有 `FReEchoWeaponLogic` 已是纯类无组件，新增组件无收益）。
  - 替代方案 (3) 符文数组放 `FReEchoWeaponLogic`（ReEchoWeapons）：否决（D3 理由：破该模块架构，Effect 无法合法落点）。
  - 选择代价：Actor 直接成员意味着符文状态与 Actor 生命周期绑定，需显式在存盘点同步到 `BuildSnapshot`（已实现 D5 单写者规则消化此代价）。
- 相关文档同步范围：`MOD-ReEcho.md`（需补武器符文装配职责，已入 Writes）；`MOD-ReEchoWeapons.md`（审阅无需改）；`ARCHITECTURE.md`（若提及商店报价/装备结构需更新，提交时核对）；`modules/MOD-ReEchoUI.md`（商店 UI 后续接 EquipRune，属 Plan67，本 Plan 不强制改）。
- 关闭前逐项填写审阅结果：（实现后回填）

## 锁定验收

- [ ] `AReEchoWeaponActor` 持有 `EquippedRunes`（3 槽数组），按 `SlotTypeId` 索引，初始化自 `BuildSnapshot.EquippedParts`。
- [ ] `EquipRune`/`UnequipRune` 经槽容量（`FindEffectiveSlotLimit`）与兼容性（`IsPartCompatibleWithWeapon`）校验；非法装配返回错误且不改状态。
- [ ] 装配/卸下后 `RebuildEffectiveDefinition` 被调用，有效武器定义含新符文效果（`FReEchoWeaponLogic` 看到的 stats/RuleFlags 已烤入）。
- [ ] 存盘→读档后 `EquippedRunes` 与存档一致；回放重建出的有效武器与存盘时一致（确定性证据）。
- [ ] 不破坏 `ReEchoWeapons` 模块边界（该模块无装配/Effect 代码新增）。
- [ ] 纯 Markdown/JSON 变更豁免 FullRebuild；C++ 变更经增量构建 + `validate_project.py`。

## Step 0 门禁

- 基线分支/提交：发布时从 `origin/main` 最新提交开出 `ReEcho-plan75-weapon-rune-component`（分支 `plan/75-weapon-rune-component`）。
- 引擎/构建可用性：`scripts/ue/Build-Editor.cmd -Configuration Development` 可增量构建（编辑器关闭时）。
- 现有聚焦测试结果：复用现有武器/装配自动化（`scripts/ue/Run-Automation.cmd -Filter ReEcho`）。
- 共享契约 / 难合并资源风险：`AReEchoWeaponActor` 公共接口变化属易合并文本；无二进制资源风险。
- 基线损坏时的停止条件：若 `BuildEffectiveWeaponDefinition` 在重构后产出与基线不同的有效定义（符文效果丢失/重复），立即停止并报告。

## 实现提纲

1. **数据载体确认（零新增）**：复用 `FReEchoEquippedPartSnapshot{ SlotTypeId, PartId }` 作为数组元素；符文 = `parts.csv` 中被 `PartId` 引用的一行数据，不实例化任何 UObject/类。数组放进 `AReEchoWeaponActor`：
   - `UPROPERTY() TArray<FReEchoEquippedPartSnapshot> EquippedRunes;`（长度 = 该武器槽数，按 `SlotTypeId` 索引；初始化自 `BuildSnapshot->EquippedParts`）。
2. **装配接口（主模块，全包合规）**：在 `AReEchoWeaponActor` 新增
   - `bool EquipRune(FName PartId, FString& OutError)`：校验槽容量 + 兼容性（`ReEchoWeaponRuntime::GetEffectiveSlotCapacity` / `IsPartCompatibleWithWeapon`），写入 `EquippedRunes` 对应槽，成功则调用 `RebuildEffectiveDefinition()`。
   - `bool UnequipRune(FName SlotTypeId, FString& OutError)`：清对应槽并 `RebuildEffectiveDefinition()`。
   - `const TArray<FReEchoEquippedPartSnapshot>& GetEquippedRunes() const`。
   - 装配/卸下**不直接写 `BuildSnapshot`**（D5 单写者），只改 `EquippedRunes` + 触发编译。
3. **Effect 编译复用（不搬逻辑）**：`RebuildEffectiveDefinition` 已调用 `BuildEffectiveWeaponDefinition`（其内部走 `ApplyPartEffect` 把符文效果烤进 `Build` → 有效定义）。本 Plan 仅确保 `EquippedRunes` 在调用前已同步进传给 `BuildEffectiveWeaponDefinition` 的 `Build` 副本（即在 actor 内部构造临时 `FReEchoBuildSnapshot` 其 `EquippedParts = EquippedRunes`，再编译）。**Effect 解释器代码保留在 `ReEchoWeaponRuntime`（主模块），不进 `ReEchoWeapons`。**
4. **持久同步（D5 单写者）**：
   - 加载：actor 初始化时 `EquippedRunes = BuildSnapshot->EquippedParts`（Copy）。
   - 存盘/检查点：Run 子系统在写 `BuildSnapshot` 前，由 actor 导出 `EquippedRunes` → `BuildSnapshot.EquippedParts`（提供 `void ExportRunesToBuild(FReEchoBuildSnapshot& OutBuild)` 或 Run 侧直接读 `GetEquippedRunes`）。
   - 回放：回放重建时读 `BuildSnapshot.EquippedParts` → actor `EquippedRunes` → 同一编译路径，保证确定性一致。
5. **下游接缝（供 Plan67）**：`PurchaseShopItem`（商店）购买符文时改为调用 `AReEchoWeaponActor::EquipRune(PartId)` 填入首个空且兼容的槽；"通用符文"= `WeaponTypeId==Any` 的 Core 件，"当前武器符文"= 当前武器类型的任意槽件，"其他武器符文/其他武器"= 其他武器类型的件——分类由 `WeaponTypeId` 运行时判定（详见 Plan67 决策节）。本 Plan 只暴露 `EquipRune`，商店逻辑在 Plan67 实现。
6. **文档**：更新 `MOD-ReEcho.md` 补"武器符文装配（Actor 层）"职责段；审阅 `MOD-ReEchoWeapons.md` 确认纯逻辑边界未变。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | 项目/源码不变量通过 |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0 |
| 运行时行为 | `scripts/ue/Run-Automation.cmd -Filter ReEcho`（武器/装配用例） | 受影响报告通过 |
| 需要人工时 | 具名 PIE：装配/卸下符文观察有效武器 stats 变化；存档→读档符文保留；回放重建一致 | 记录人工结果 |

## 执行记录

### 变化

- 2026-08-22（设计定稿，Gavyn 与 AI 对齐）：
  - 确立范围：武器符文组件先行设计，商店（Plan67）为下游消费者（D1）。
  - 行为模型：数据驱动，否决每表项继承子类（D2）。
  - 落点：`EquippedRunes` 数组作 `AReEchoWeaponActor` 直接数据成员，主模块内可"全包"且不破 `MOD-ReEchoWeapons`（D3）。
  - "一体"身份：符文编译期烤进有效武器定义，战斗只看统一有效武器（D4，复用现有 `BuildEffectiveWeaponDefinition`）。
  - 持久真值单写者：`BuildSnapshot.EquippedParts` 仍为真值，`EquippedRunes` 局内活权威，存盘点回写（D5）。
  - 本文件为设计文档（Proposed），实现在独立工作树 `ReEcho-plan75-weapon-rune-component` 进行，遵循 plan 工作树纪律。

### 证据

（实现后回填；本 Plan 当前为设计阶段，无代码变更）

### 剩余风险

- `AReEchoWeaponActor` 初始化路径需确认 `BuildSnapshot` 在 `InitializeWeapon` 时已持有正确 `EquippedParts`，否则 `EquippedRunes` 加载会空。实现 Step 1 前先核对 `InitializeWeapon` 调用链。
- 现有代码若有无经 actor、直接写 `BuildSnapshot.EquippedParts` 的装配点（如 Run 子系统初始化构筑），需在 D5 同步点统一收口，避免双写。

### 人工验收结果/请求

### 架构文档审阅结果
