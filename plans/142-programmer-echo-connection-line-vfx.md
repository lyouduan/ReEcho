# Plan 142 - 程序 - 回响连接线卡牌特效

## 协调

- Planner 负责人：当前对话程序 Planner（Codex）。
- Executor 负责人：当前对话程序 Executor（Codex）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@73030025dc262fe82fc4f530c5b1f9154c946a7d`。
- 本地实现方式：独立 worktree `C:\tmp\ReEcho-plan142-echo-connection-vfx`，分支 `codex/plan142-echo-connection-vfx`。
- 依赖 / 阻塞：卡牌 `G_2_30` 已通过 `FReEchoCardRuleSnapshot::bConnectionLineDamage` 驱动正式穿线伤害；外部交付资产为 `F:\Content\VFX\Echo\Particle\NS_Echo_Chain.uasset`。
- Writes:
  - `plans/142-programmer-echo-connection-line-vfx.md`
  - `Content/VFX/Echo/Particle/NS_Echo_Chain.uasset`
  - `Content/VFX/Echo/MI/BaseVFX003_Inst25.uasset`
  - `Content/01_Textures/02_Turbulence/Tur_C080.uasset`
  - `Content/01_Textures/02_Turbulence/Tur_C090.uasset`
  - `Source/ReEcho/Public/Presentation/VFX/ReEchoCombatVfxCatalog.h`
  - `Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxCatalog.cpp`
  - `Source/ReEcho/Public/Presentation/VFX/ReEchoCombatVfxComponent.h`
  - `Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxComponent.cpp`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoGameModeTests.cpp`（仅在现有测试夹具可稳定覆盖同步入口时）
  - `scripts/ue/audit_plan142_echo_chain_vfx.py`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - FullRebuild 刷新的 `Binaries/Win64/ReEchoEditor.prebuilt.json` 及其准确允许列表产物
- Stable Reads:
  - `Content/Data/cards.csv`
  - `Content/Data/card_effects.csv`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardTypes.h`
  - `Source/ReEchoCards/Private/Cards/ReEchoCardRuntime.cpp`
  - `Source/ReEcho/Public/Graybox/ReEchoEchoActor.h`
  - `Source/ReEcho/Private/Graybox/ReEchoEchoActor.cpp`
  - `Source/ReEcho/Public/Player/ReEchoPlayerPawn.h`
  - `Source/ReEcho/Private/Player/ReEchoPlayerPawn.cpp`
  - `Source/ReEcho/Public/Combat/ReEchoCombatTarget.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - 已存在的 `/Game/VFX/Element/Elctricity/MI/BaseVFX003_Inst1`、`Inst27`、`Inst28` 与 `/Game/Mat/BaseVFX003`
- 影响模式：`SharedContract`；新增 Cards 规则到 VFX adapter 的持续连接表现，但不改变 Cards 或 Combat 公共状态所有权。
- 兼容承诺 / 下游操作：`G_2_30` 的伤害公式、穿越判定、敌人命中频率、玩家/回响位置和多回响循环保持现状；缺失或失效 VFX 只能少表现，不能阻止卡牌伤害。
- 明确排除：不修改 XLSX/CSV、卡牌数值、伤害公式、穿线算法、Echo 数量、回放、武器、导电特效或外部 Echo Water/Grass/LevelSequence 资产；不导入 `NS_Echo_Chain` 实际引用闭包之外的资源。

## 锁定目标

持有 `G_2_30「连接，连接！」` 时，角色本体与每个存活回响之间各维持一条 `NS_Echo_Chain` 链接；双方移动时端点连续跟随。双回响必须显示两条并与现有两条玩法伤害线一致。卡牌未生效、任一端死亡/失效、回响移除、遭遇结束或宿主 EndPlay 时，准确清理对应链接且无残留。资源缺失时玩法继续。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`AREA-Cards`、`AREA-Presentation`、`AREA-Tests`；`MOD-ReEchoCards` 仅稳定读取，不改变其契约。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md` 的卡牌持续链接生命周期和资产契约；维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 的 GameMode Cards→Presentation 接线；两者已加入 Writes。审阅 `MOD-ReEchoCards.md`，规则快照所有权不变则不制造正文 diff。
- 设计意图：Cards 继续只声明 `bConnectionLineDamage`，GameMode 在既有 Encounter tick 读取同一规则和存活 Echo 集合，VFX Component 只维护 Niagara 组件与端点；不得把表现 Actor 或 Niagara 状态写回 Cards/Combat。
- 权威状态与依赖：现有 Player/Echo Actor XY 仍是穿线伤害线段权威。视觉端点使用相同 XY，并只从双方 `HurtVfxRoot` 取得各自 Z 高度，避免表现偏移改变玩法几何。每个 Echo 的 Link 由 Player 的 VFX Component 以 weak Echo key 独立拥有。
- 决策记录：
  1. 外部包字符串闭包显示 Chain 直接引用 Echo `BaseVFX003_Inst25` 和仓库已有 Electricity `Inst1/27/28`；`Inst25` 再引用缺失 `Tur_C080/Tur_C090` 及仓库已有 `Hor_C062/Mas_C010/BaseVFX003`。仅导入 Chain、Inst25、Tur_C080、Tur_C090。
  2. 不把 Niagara Component 附着到 Player 或 Echo；以世界原点、identity rotation、World Space 参数更新两个端点，避免双重 Local/World Transform。资源导入后必须用 UE 读回确认真实参数类型和 End 语义；若不是可无损适配的双端点系统则停止接入并报告。
  3. 多回响一对一映射，新增/保留/移除均按存活集合差量同步；不每帧重新 Spawn。
  4. GameMode 只传规则开关与 Echo Actor 集合，不直接持有 Niagara Component；VFX 缺失 fail-open。
- 相关文档同步范围：更新 `MOD-ReEchoVFX.md` 与 `MOD-ReEcho.md`；审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEchoCards.md`，模块拓扑、稳定标识和 Cards 规则所有权不变时记录无需修改。
- 关闭前逐项填写审阅结果：待实现、自动化及 PIE 验收后补充。

## 锁定验收

- [ ] 精确资源闭包可加载：Chain、Inst25、Tur_C080、Tur_C090；未导入无关 Echo 资产，仓库已有依赖未被覆盖。
- [ ] `NS_Echo_Chain` 的启用 Emitter 坐标空间、Renderer、材质和端点参数由 UE 回读锁定；运行时写入顺序与资产真实语义一致。
- [ ] 单回响一条、双回响两条；移动时视觉线段端点跟随角色和对应回响，并与玩法 XY 线段一致。
- [ ] 卡牌关闭、对象死亡/失效、Echo 集合变化、遭遇结束和 EndPlay 均准确清理；资源失败不影响现有物理伤害。
- [ ] Development FullRebuild、VFX/GameMode 聚焦自动化、`python scripts/validate_project.py`、预构建检查和 `git diff --check` 通过。
- [ ] 用户在 PIE 验收端点高度、遮挡、亮度、宽度、移动稳定性及双回响画面。

## Step 0 门禁

- 基线分支/提交：`origin/main@73030025dc262fe82fc4f530c5b1f9154c946a7d`。
- 引擎/构建可用性：UE 5.8 Windows；Editor/命令使用 Git-common-dir Unreal 锁。Plan140 导电候选保留在另一独立 worktree，本任务不得读取其未提交实现作为基线。
- 现有聚焦测试结果：静态确认 `G_2_30` 已编译为 `bConnectionLineDamage`；GameMode 对每个存活 Echo 执行独立穿线判定和 `0.5 * (PhysicalAttack + ElementalAttack)` 物理伤害；当前没有对应持续链接 VFX。
- 共享契约 / 难合并资源风险：`ReEchoGameMode.cpp` 是共享高碰撞文件，但本任务已从包含 Plan135 最新修改的远端基线创建；Niagara/MI/纹理为不可文本合并资产，发布前必须审计远端同路径变化。
- 基线损坏时的停止条件：外部资源还依赖未交付且仓库缺失的路径；资产端点不是可运行时更新的双端点契约；必须修改玩法伤害才能对齐表现；或远端并行修改相同 Chain/Inst25/纹理。

## 实现提纲

1. 发布本 Plan 后，把精确外部闭包复制到相同 `/Game/` 路径，并通过 UE 只读审计确认加载、依赖、Emitter/Renderer 和 User Parameter。
2. 在 VFX Catalog 注册卡牌链接语义；在 Player VFX Component 中维护 `Echo -> NiagaraComponent` 映射，按规则与存活集合差量同步，并每帧更新双方世界端点。
3. 在 GameMode 既有 Cards Encounter tick 中调用同步入口，复用其同一份 RuleSnapshot 和 Echo 集合；所有停止路径调用清理。
4. 扩充资产契约和生命周期自动化，更新模块文档与 Plan 执行记录。
5. 冻结候选后执行完整门禁，最后请求 PIE 视觉验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 资源闭包 | UE Plan142 审计 + Asset Registry/加载回读 | 精确四个新资源加载；全部硬依赖存在；参数类型、空间和 Renderer 材质明确 |
| C++ | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新精选 7 模块预构建包 |
| 自动化 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Presentation.VFX.Catalog` 及具名 GameMode/VFX 链接测试 | Catalog、端点更新、多 Echo 差量生命周期、清理和 fail-open 通过 |
| 静态 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 数据无漂移、预构建指纹和差异格式通过 |
| 人工 PIE | 授予 `G_2_30`，单/双 Echo 移动并让敌人穿线 | 视觉与伤害线一致、无抖动残留、亮度宽度可接受 |

## 执行记录

### 变化

### 证据

### 剩余风险

### 人工验收结果/请求

`PendingBeforeClose`：实现后请求用户在 PIE 验收持续链接的视觉质量和玩法线一致性。

### 架构文档审阅结果
