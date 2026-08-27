# Plan 140 - 程序 - 导电特效持续时间与亮度

## 协调

- Planner 负责人：Codex（程序路线）。
- Executor 负责人：Codex（程序路线）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@06513a0fab7c3900e27f8698bf8588b84c63a3d3`。
- 本地实现方式（可选，仅作交接说明）：`C:\tmp\ReEcho-plan140-conduct-visibility`，分支 `codex/plan140-conduct-visibility`。
- 依赖 / 阻塞：复用 Plan89/90 已修正的 Conduct World Space + LWC `NiagaraPosition` 端点契约；依赖 `/Game/VFX/Element/Elctricity/Particle/NS_Element_Electricity` 及其 Electricity 目录内专属材质实例可编辑。
- Writes: `plans/140-programmer-conduct-duration-and-brightness.md`；`Source/ReEcho/Public/Presentation/VFX/ReEchoCombatVfxComponent.h`；`Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxComponent.cpp`；`Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`；`Content/VFX/Element/Elctricity/Particle/NS_Element_Electricity.uasset`；必要时仅限该 System 已引用的 `Content/VFX/Element/Elctricity/MI/BaseVFX003_Inst1.uasset`、`BaseVFX003_Inst24.uasset`、`BaseVFX003_Inst27.uasset`、`BaseVFX003_Inst28.uasset`；`scripts/ue/configure_plan140_conduct_visibility.py`；`shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`；FullRebuild 刷新的 `Binaries/Win64/ReEchoEditor.prebuilt.json` 及其准确允许列表产物。
- Stable Reads: `Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`；`Source/ReEchoCombat/Private/Combat/ReEchoElementHitResolver.cpp`；`Source/ReEcho/Private/Presentation/VFX/ReEchoElementReactionVfxCatalog.cpp`；`Source/ReEcho/Public/Presentation/Weapon/ReEchoWeaponPresentationProfile.h`；生产武器 Presentation Profile；`shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`；`plans/089-vfx-audit-fixes.md`；`plans/090-editable-vfx-test-scene.md`。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：导电伤害、反应半径、受影响目标、BFS Link 顺序、`ConductLinkPropagationDelaySeconds`、世界坐标端点和激活前写入顺序不变；每条 Link 只延长表现窗口并提高 Emissive；调参失败不得阻止战斗反应。
- 明确排除：不修改 XLSX/CSV、元素反应公式、伤害、附着清除、传播目标或传播延迟；不把 Niagara 生命周期作为玩法状态；不修改 Electricity 目录外的共享材质；不改变 `User.StartPosition` / `User.EndPosition` 类型、组件世界原点或 World Space Emitter。

## 锁定目标

每条正式 Conduct 闪电连接从激活起清晰可见 `1.0s`，结束前约 `0.15s` 自然淡出；默认亮度倍率为 `1.5`。`NS_Element_Electricity` 新增并实际消费 float `User.ConductDurationSeconds` 与 `User.ConductBrightnessMultiplier`，`SpawnConductLink` 必须在写入两个 LWC 世界端点之后、`Activate(true)` 之前写入这两个值。多段导电仍按既有 BFS 顺序与武器 Presentation 延迟生成，伤害仍在 Combat 权威链即时完成。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` / `AREA-Presentation`、文档型逻辑入口 `MOD-ReEchoVFX`；只稳定读取 `MOD-ReEchoCombat` 的 ReactionLinks 与伤害完成事件。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md` 并加入 Writes；审阅 `MOD-ReEcho.md` 与 `MOD-ReEchoCombat.md`，玩法和模块拓扑不变时记录无需修改。
- 设计意图：Combat 继续一次性完成伤害与传播发现；VFX adapter 只提供统一默认表现参数，Niagara/其专属材质消费持续时间和亮度，避免把一秒计时引入玩法状态。
- 权威状态与依赖：`ReactionLinks`、端点和传播延迟的所有者不变。新增两个 User Parameter 是表现契约；默认值由程序常量设置，资产内也保留相同安全默认值，缺失参数时 Niagara 仍按作者配置播放而不影响逻辑。
- 决策记录：采用显式 User Parameter，而非只把现有 Emitter Duration 写死或修改全局后处理。亮度优先绑定 Niagara 粒子颜色/动态材质参数；若现有父材质没有独立 Emissive 标量，仅允许在 Electricity 目录内建立/调整专属实例和绑定，不修改其他 VFX 共用父材质。若 UE API 无法无损绑定参数，停止资产修改并报告，不以无效 `SetVariableFloat` 冒充完成。
- 相关文档同步范围：更新 `MOD-ReEchoVFX.md`；审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoCombat.md`，未改变模块拓扑、路由或玩法契约时不修改。
- 关闭前逐项填写审阅结果：待实现、自动化与 PIE 验收后补充。

## 锁定验收

- [ ] `NS_Element_Electricity` 精确暴露两个 float User Parameter，两个 World Space Emitter 实际消费时长和亮度，不仅是程序写入无效名字。
- [ ] `SpawnConductLink` 在 `Activate(true)` 前依次写入 Start/End Position、`1.0s` 和 `1.5x`，位置、长度和斜向连接不回归。
- [ ] 每条 Link 显示一秒并自然淡出；连续/多段 Conduct 可重叠显示，新批次只取消尚未生成的传播 timer，不提前销毁已经生成的 Link。
- [ ] Conduct 伤害、目标数、BFS 顺序、传播延迟和元素状态清理保持现状。
- [ ] FullRebuild、VFX Catalog/Conduct 聚焦自动化、项目校验、预构建检查和 `git diff --check` 通过。
- [ ] 用户在 PIE 比较调整前后亮度、持续时间、近距离/远距离、多怪连锁和重复触发，确认不过曝且不遮挡角色。

## Step 0 门禁

- 基线分支/提交：`origin/main@06513a0fab7c3900e27f8698bf8588b84c63a3d3`。
- 引擎/构建可用性：UE 5.8 Windows；Editor/命令前使用 Git-common-dir Unreal 锁；资产保存前确认没有本项目 Editor 占用。
- 现有聚焦测试结果：源码确认 Conduct 只暴露 Start/End Position；`SpawnConductLink` 以世界原点和 `autoActivate=false` 生成，写端点后激活，生命周期由 Niagara 自身决定。既有 Catalog 自动化锁定端点类型与 World Space。
- 共享契约 / 难合并资源风险：`NS_Element_Electricity.uasset` 是不可文本合并的单资产；发布前必须审计远端是否改变该文件。四个 Electricity MI 仅在实际被该 System 引用且需要专属 Emissive 参数时进入候选。
- 基线损坏时的停止条件：父材质参数化会影响 Electricity 目录外资产；无法证明两个 Emitter 实际消费新参数；一秒显示需要延迟玩法伤害；或远端并行修改同一 Niagara/MI。

## 实现提纲

1. 通过 UE 资产读回记录两个 Emitter、Renderer、材质实例、现有生命周期模块和可用参数，确认最小无损绑定路径。
2. 在 VFX Component 中集中定义 `1.0s` / `1.5x` 默认值，并在两个 Position 参数之后、激活之前写入 Niagara Component。
3. 用受支持 Editor API 为 Conduct System 增加/绑定 Duration 与 Brightness 参数；只保存实际变化的 Electricity 资源。
4. 扩充 Catalog 自动化，验证参数类型、默认值、组件 override 读回、World Space 和既有端点契约；验证已生成 Link 与传播 timer 生命周期相互独立。
5. 更新模块文档与执行记录，冻结候选后执行完整发布门禁并请求 PIE 验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 资产读回 | Plan140 UE Python 配置/审计脚本 + `ReEcho.Presentation.VFX.Catalog` | 两个 float 参数存在且被两个 Emitter/Renderer 实际消费；World Space 和 Position 类型不变 |
| C++ | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新精选 7 模块预构建包 |
| 自动化 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Presentation.VFX.Catalog`、Conduct 世界链路相关测试 | 参数写入顺序/读回、端点、BFS timer 和一秒表现契约通过 |
| 静态 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 项目、XLSX/CSV、预构建指纹和差异格式通过 |
| 人工 PIE | `GMEnemyElementAll Water` + `GMElement Lightning`，两怪/多怪/重复触发 | 每条 Link 约 1 秒、亮度约 1.5x、自然淡出、不过曝且端点准确 |

## 执行记录

### 变化

### 证据

### 剩余风险

### 人工验收结果/请求

`PendingBeforeClose`：实现后请求用户在 PIE 确认一秒可见窗口与 1.5 倍亮度的主观可读性。

### 架构文档审阅结果
