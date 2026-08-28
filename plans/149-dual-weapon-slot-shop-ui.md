# Plan 149 - 程序 - 双重武器槽商店装配界面

## 协调

- Planner 负责人：Codex（程序路线，规划执行者合一）。
- Executor 负责人：Codex（程序路线，规划执行者合一）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`InProgress`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@ae548af1c15c0a8123f19e66fb86cc9bd27fa014`。
- 本地实现方式（可选，仅作交接说明）：独立分支 `plan/149-dual-weapon-slot-shop-ui` 与独立 worktree `ReEcho-plan149-dual-weapon-slot-shop-ui`。
- 依赖 / 阻塞：UE 内容资产写入前需要编辑器关闭；视觉验收需要在获得与未获得 `G_3_22` 的两种局内状态下手测。
- Writes: `plans/149-dual-weapon-slot-shop-ui.md`；`Source/ReEcho/Public/UI/ReEchoInventoryShopWidget.h`；`Source/ReEcho/Private/UI/ReEchoInventoryShopWidget.cpp`；相关 `Source/ReEcho/Private/Tests/*.cpp`；`Content/ReEcho/UI/WBP_ReEchoInventoryShop.uasset`；Plan149 导入的商店双重武器槽纹理资产；相关 UE 内容编写/审计脚本；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`；构建门禁允许的精选 Editor 预构建产物。
- Stable Reads: `Content/Data/cards.csv`；`Content/Data/card_effects.csv`；`Content/Data/slot_profiles.csv`；`Content/Data/slot_types.csv`；`Content/Data/parts.csv`；`Source/ReEcho/Private/Weapons/ReEchoWeaponRuntime.cpp`；`Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`；`Source/ReEcho/Private/Tests/ReEchoWeaponRuntimeTests.cpp`；`C:/Users/gavynqiu/Downloads/商店.svg`；`C:/Users/gavynqiu/Documents/miniGame/正式-UI视觉/正式-UI视觉/商店/核心武器装配.png`。
- 影响模式：`SharedContract`（不改变装备状态权威和存档 schema，但扩展商店 UI 对既有容量契约的完整投影）。
- 兼容承诺 / 下游操作：未获得 `G_3_22` 时维持核心 1 + 两类非核心各 1 的三槽表现与行为；获得后显示核心 1 + 两类非核心各 2 的五个实际可装备槽；旧存档直接兼容，无需迁移。
- 明确排除：不修改商店刷新、商品抽取、价格、购买、时间碎片、悬停说明、武器切换、保存并离开或其他投放逻辑；不把容量误实现为 9；不允许核心晶体装备两个。

## 锁定目标

获得卡牌“`双重武器槽`”（`G_3_22`）后，商店装配区切换为 `核心 1 + 非核心类别 A 2 + 非核心类别 B 2` 的五槽布局，并使用正式素材 `核心武器装配.png` 呈现。五个可视槽各自只承载一枚符文；核心晶体仍只能装备一枚。未获得卡牌时继续使用当前三槽布局。所有既有商店业务逻辑保持一致。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI`、`MOD-ReEchoWeapons`。Cards 已提供 `G_3_22` 规则，不改变其所有权，只读取武器商店视图中的有效容量。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoUI.md`、`MOD-ReEchoWeapons.md`，均已加入 `Writes`。
- 设计意图：装备数组和有效槽容量继续由 Run/Weapon 规则层负责；UI 只把每个 `SlotTypeId` 的 `Capacity` 展开为带 occurrence 的可视槽，不复制或推断卡牌状态。
- 权威状态与依赖：不改变状态所有者、存档 schema 或依赖方向。`FReEchoPartShopView` 仍是 UI 的只读输入，UI 通过既有委托提交装备请求。
- 决策记录：保留 `FReEchoEquippedPartSnapshot` 数组顺序作为同类槽内的稳定 occurrence；用五个蓝图可编辑的按钮/图层呈现扩展布局；未选运行时生成九槽或把一个类别容量 2 解释为每个视觉格再容纳 2，因为这与玩家规格不符。
- 相关文档同步范围：模块边界不变，因此 `ARCHITECTURE.md` 与根 `README.md` 只审阅不改；上述三个模块文档补充五槽 UI 投影与兼容说明。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEcho.md` 待更新：记录商店视图消费既有容量契约；
  - `MOD-ReEchoUI.md` 待更新：记录三槽/五槽作者化布局及 occurrence 映射；
  - `MOD-ReEchoWeapons.md` 待更新：确认 `G_3_22` 的 1+2+2 规则由 UI 完整呈现；
  - `ARCHITECTURE.md` 待审阅：预计无需修改，因无模块拓扑或状态所有者变化；
  - `README.md` 待审阅：预计无需修改，因无用户安装/启动流程变化。

## 锁定验收

- [ ] 未获得 `G_3_22` 时装配区保持三个实际槽位，容量为 `1+1+1`。
- [ ] 获得 `G_3_22` 后装配区切换为正式五槽视觉，容量为 `1+2+2`，而不是 9 个或视觉五槽但逻辑仍三槽。
- [ ] 五个槽分别显示对应 occurrence 的装备图标、悬停说明，并能从对应类别背包进行装备/替换；同类第二枚不再被第一枚覆盖。
- [ ] 核心第二枚仍按容量 1 处理；非核心同类第三枚仍按既有规则替换/回收，不突破容量 2。
- [ ] 保存/读取后五槽装备与布局恢复正确，旧三槽存档兼容。
- [ ] 商店刷新、商品、价格、购买、武器切换和保存离开回归无变化。
- [ ] 功能结果有可观察证据。
- [ ] 必需自动化/构建检查通过。
- [ ] 视觉切换与交互经人工 PIE 验收。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`plan/149-dual-weapon-slot-shop-ui`，基于 `origin/main@ae548af1c15c0a8123f19e66fb86cc9bd27fa014`。
- 引擎/构建可用性：仓库标准 UE 5.8 构建脚本可用；LFS hydration 检查通过。
- 现有聚焦测试结果：现有 Weapons 自动化已覆盖 `G_3_22` 同类别双符文可同时编译；实现后补充商店五槽投影与交互测试。
- 共享契约 / 难合并资源风险：`WBP_ReEchoInventoryShop.uasset` 为二进制热点；只在本 Plan worktree 由 UE Editor/命令let 修改，并在合入最新主线后重做内容验证。
- 基线损坏时的停止条件：若既有三槽装备、商店刷新或保存读取在未修改基线上失败，先记录并停止扩大范围；不借本 Plan 重写既有商店业务。

## 实现提纲

1. 将商店可视槽映射由“数组下标等于 SlotView 下标”改为 `(SlotTypeId, OccurrenceIndex)`，按有效容量生成 3 或 5 个展示描述符。
2. 扩展五个作者化槽按钮、图标和点击绑定；同类装备按 occurrence 显示，点击指定槽时保持目标 occurrence 的替换语义。
3. 导入 `核心武器装配.png`，在 `WBP_ReEchoInventoryShop` 中建立可编辑、所见即所得的双重武器槽布局，并按有效容量切换标准/扩展视觉。
4. 增补 1+1+1、1+2+2、核心上限、第三枚替换及存档恢复的聚焦自动化；回归商店其他逻辑。
5. 更新模块文档与执行记录，完成构建、静态验证、LFS 和 PIE 人工验收交接。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目/源码不变量与文本格式通过 |
| LFS/内容 | `python scripts/setup_lfs.py --check`、`git lfs status`、`git lfs fsck` | WBP/纹理是真实 LFS 内容且对象完整 |
| C++ 变化时构建 | `scripts/ue/Build-Editor.cmd`；发布前按规则执行 `-Configuration Development -FullRebuild` | UHT/UBT 退出码为 0，精选预构建刷新 |
| 运行时行为变化时自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho`，至少聚焦 Shop/Weapons/Save | 1+2+2 容量与既有商店回归通过 |
| 内容审计 | Plan149 UE 审计脚本检查 WBP 五槽节点、素材引用和可见性契约 | 作者化布局可在蓝图中编辑且引用正确 |
| 人工验收 | PIE 分别测试未获得/已获得 `G_3_22` | 三槽与五槽切换、五枚符文显示/悬停/替换及保存恢复正确 |

## 执行记录

### 变化

- 已审计现有规则：`G_3_22` 已将非核心 `SlotTypeId` 的有效容量扩为 2，核心保持 1；装备快照数组与保存结构已能容纳五枚符文。
- 已定位 UI 缺口：当前蓝图/C++ 只绑定 `DesignerAttachmentSlot0..2`，且每类只查找第一枚已装备符文。

### 证据

- `cards.csv` 与 `card_effects.csv` 明确 `G_3_22`/`NonCoreSlotCapacity Multiply 2`。
- `ReEchoWeaponRuntime::FindEffectiveSlotLimit` 与既有 Weapons 自动化确认非核心双容量、核心不扩容。

### 剩余风险

- 同类别两个视觉槽需要稳定 occurrence 替换语义，不能继续依赖“满容量时替换最早一枚”来冒充指定槽替换。
- WBP 为共享二进制热点，实施前后都需远端重叠审计。

### 人工验收结果/请求

- 待实现完成后请求用户在 PIE 测试获得 `G_3_22` 前后两种状态。

### 架构文档审阅结果

- 待实现完成后填写。
