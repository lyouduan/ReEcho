# Plan 100 - 程序 - 武器特效位姿与玩家回响统一尺寸

## 协调

- Planner 负责人：Codex（程序 Planner）。
- Executor 负责人：Codex（程序 Executor；Plan 发布后在独立实现 worktree 执行）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`（手动/自动攻击输入源互斥候选已实现并通过自动化，等待 PIE 验收）。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@6b3262fa574d9ea684cad1c0ce8d6c8c96f25fc6`。
- 本地实现方式：规划分支 `codex/plan100-weapon-vfx-placement-plan`；发布本 Plan 后从最新 `origin/main` 创建一任务一 worktree 的 Executor 分支。
- 依赖 / 阻塞：复用 Plan78 的 Weapon Presentation DA 与 Plan81 的角色相对武器装配；长剑地面穿模候选值为 `AttackCommitted.Offset` 的 `Z=60 UU`、`Roll=-45°`，不合入旧候选分支上的语义硬编码。
- Writes:
  - `plans/100-weapon-vfx-placement-and-shared-sizing.md`
  - `Source/ReEcho/{Public,Private}/Presentation/Weapon/ReEchoWeaponPresentationProfile.*`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxCatalog.*`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxComponent.*`
  - `Source/ReEcho/{Public,Private}/Weapons/ReEchoWeaponActor.*`
  - `Source/ReEcho/Private/Tests/ReEchoCombatPresentationTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoWeaponRuntimeTests.cpp`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoEnemyActor.*`
  - `Source/ReEcho/Private/Tests/ReEchoEnemyHostTests.cpp`
  - `Source/ReEcho/Private/Player/ReEchoPlayerPawn.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoAttackModeTests.cpp`
  - `Source/ReEchoCombat/Private/Combat/ReEchoAttackControllerComponent.cpp`
  - `Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`
  - `scripts/ue/configure_plan100_weapon_presentation.py`
  - `Content/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_*.uasset`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoEnemies.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`
  - `Binaries/Win64/` 下 `shared/GIT_RULES.md` 允许的最终预构建包文件
- Stable Reads: `weapons.csv` 的 WeaponId/VisualKey/AttackPatternId；角色/Echo Presentation Profile 的 `WorldHeight`、`WeaponAnchorRatio`；现有 Niagara 资源与攻击事件时序；Plan96 山羊 VFX 候选 worktree。
- 影响模式：`SharedContract`。除既有武器表现契约外，长剑攻击提交可消费兔子宿主持有的敌方投射物逻辑状态；兔子球命中玩家后立即结束逻辑与视觉生命周期。
- 兼容承诺 / 下游操作：旧 DA 的单位 Transform、关闭绝对长度覆盖时保持现状；配置缺失只跳过表现；同一 WeaponVisualKey 在 Player/Echo 上使用相同绝对世界长度与特效世界尺寸；武器挂点仍由角色 Profile 决定。
- 明确排除：伤害数值、长剑既有 180°攻击范围、攻击方向、攻击/动画节拍、其他武器斩弹、非兔子敌方投射物、武器数值平衡、Niagara 内部发射器编辑、非武器 VFX、Plan96 山羊特效修改。

## 锁定目标

让每把武器都能在现有 Weapon Presentation DA 的各 VFX Slot 中直接配置 Niagara 的相对位置、旋转和大小，运行时真实消费该配置。首个落地配置将长剑刀光抬高并倾斜，使刀光完整显示且不穿入地面。同时将六把武器的手持视觉尺寸改为武器 DA 拥有的绝对世界长度，使同一武器在玩家与回响上大小一致；角色 Profile 仍只控制挂点位置。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoVFX`、`MOD-ReEchoWeapons`、`AREA-Presentation`、`AREA-Weapons`。
- 对应模块文档：维护 `MOD-ReEcho.md`、`MOD-ReEchoVFX.md`、`MOD-ReEchoWeapons.md`，均已加入 `Writes`。
- 设计意图：`FReEchoWeaponVfxSlot::Offset` 已是完整 `FTransform`，继续作为单一 DA 配置入口；VFX Catalog 负责把武器语义解析成 Slot/Placement，VFX Component 只执行统一的附着与世界尺寸补偿。武器 Profile 的 `HeldLengthOverrideCm` 作为同一 WeaponVisualKey 的尺寸真相，WeaponActor 不再让宿主 Actor Scale 二次改变最终世界长度。
- 权威状态与依赖：只调整表现数据及适配器；Combat 事件继续拥有时机，Weapon Definition 继续拥有玩法，Niagara 不拥有伤害/命中/冷却。
- 决策记录：不新增重复的 Offset/Scale 字段；不把长剑修正写死在 `PlayerMeleeSlash` 枚举分支；不按当前动画帧 Bounds 重算；通过现有绝对长度覆盖统一 Player/Echo，保留角色 `WeaponAnchorRatio` 以允许各角色握持位置不同。
- 相关文档同步范围：审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`MOD-ReEchoPresentation.md`；若模块拓扑与 Presentation 公共字段没有变化，在执行记录注明无需修改。维护上述三个直接受影响模块文档。
- 关闭前逐项填写审阅结果：在执行记录中补齐实际实现与文档审阅结论。

## 锁定验收

- [ ] Weapon Profile 的 `Charge/Travel/DamageApplied/AttackCommitted.Offset` 中平移、旋转、缩放均由运行时消费；单位 Transform 保持旧表现。
- [ ] 长剑 `AttackCommitted` 使用 DA 配置 `Location.Z=60 UU`、`Rotation.Roll=-45°`，Catalog/Component 不存在长剑专属位姿硬编码。
- [ ] VFX Slot 可选择保持世界尺寸；开启后 Player/Echo 的父级缩放不会二次放大或缩小 Niagara，DA Scale 仍作为艺术缩放生效。
- [ ] 六个生产 Weapon Profile 启用正数 `HeldLengthOverrideCm`；同一 WeaponVisualKey 在 Player 与 Echo 上计算出的最终世界长度一致，纹理宽高比和尺寸主轴仍生效。
- [ ] Player/Echo 可以有不同角色挂点，但武器尺寸不读取宿主 Actor Scale 作为第二份尺寸真相。
- [ ] 不改变攻击事件时机、方向、伤害数值、长剑范围或动画节拍；投射物变化仅限本次兔子球结束条件与长剑斩弹。
- [ ] 兔子球与玩家的扫掠碰撞成功结算一次伤害后立即发布 `Ended` 并从逻辑数组移除，视觉代理同步消失。
- [ ] 仅 `Pattern.LongSwordCombo` 在既有提交范围和 180°弧内移除兔子球；弧外球与其他武器不受影响。
- [ ] 手动触发的主动攻击成功 Commit 后与普通攻击共用 `AttackCommitted` 事件出口并释放对应特效；镰刀召回不重复发布。
- [ ] 手动与自动攻击输入互斥：模式切换先释放旧来源，非当前模式入口被拒绝，迟到的旧来源 Release 不会中止新来源。
- [ ] 配置脚本可重复运行，只修改本 Plan 锁定字段，不重置艺术已调整的其他 Profile 字段。
- [ ] 聚焦自动化、C++ 格式化、Development FullRebuild、`validate_project.py`、预构建一致性和 `git diff --check` 通过。
- [ ] 用户在 PIE 对比 Player/Echo 同武器大小，并确认长剑刀光离地、完整显示、方向和攻击同步观感；未验收前保持 `PendingBeforeClose`。
- [ ] 未提交精选 `GIT_RULES.md` 允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@6b3262fa574d9ea684cad1c0ce8d6c8c96f25fc6`；已审计该次外部 Plan63 仅涉及怪物出生预警，无 Weapon/VFX 路径直接重叠。
- 引擎/构建可用性：实施前确认 Editor 已关闭；DA 只能由 Unreal Editor API 加载、修改和保存，禁止文本编辑 `.uasset`。
- 现有聚焦测试结果：旧长剑候选分支已证明 `Z=60、Roll=-45°` 可编译且相关静态检查通过，但其语义硬编码不得直接合入；本 Plan 必须重新在最新基线完整验证。
- 共享契约 / 难合并资源风险：六个 Weapon Profile 是二进制独占资源；执行前记录哈希并复查主工作区未存在这些资产的用户改动。若出现重叠，停止并请求人工选择。
- 基线损坏时的停止条件：远端再次前进、Editor 占用资产、DA 字段无法由 Editor API 稳定保存、或修改将跨入玩法语义时，停止并报告准确证据。

## 实现提纲

1. 为武器 VFX Slot 补充明确的世界尺寸策略默认值，并让 VFX Catalog 同时解析资源与 Slot Placement；保持非武器语义现有 Placement 路径。
2. 统一 `SpawnAttached` 对配置的平移、旋转、缩放和父级缩放补偿的组合顺序；增加 Slot 缺失/单位 Transform/世界尺寸测试。
3. 通过幂等 Editor Python 将长剑刀光写入 `AttackCommitted.Offset`，并为六个生产武器 Profile 固化当前认可的绝对长度；只更新锁定字段。
4. 收敛 WeaponActor 的尺寸换算，保证 Player/Echo 同 Weapon Profile 得到同一世界长度，同时保留挂点、纹理宽高比、主轴与旋转配置。
5. 更新聚焦测试、模块文档和执行记录，完成格式化、自动化、FullRebuild、静态/预构建检查，再提交 PIE 候选供人工验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| DA/解析 | Weapon Presentation 聚焦自动化 + 配置脚本复跑 | 六个 Profile 唯一解析；锁定字段准确；复跑无额外变化 |
| VFX | `ReEcho.Presentation.VFX` 聚焦自动化 | Slot Transform 全量消费；长剑值来自 DA；父级不同缩放时世界尺寸一致 |
| 武器 | `ReEcho.Weapons` / Weapon Presentation 聚焦自动化 | Player/Echo 同武器最终世界长度一致；旧单位配置兼容 |
| C++ | clang-format + `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新允许的预构建包 |
| 静态 | `python scripts/validate_project.py`、`scripts/ue/prebuilt_editor.py check`、`git diff --check` | 项目、源码、预构建包一致 |
| 人工 | Editor 中检查 DA；PIE 对比 Player/Echo 与长剑斩击 | 刀光不穿地且完整；同武器大小一致；挂点、朝向、同步无回归 |

## 执行记录

### 变化

- `FReEchoWeaponVfxSlot` 新增 `bPreserveWorldSize`，武器语义的 `ResolvePlacement` 现在消费对应 Slot 的完整 `Offset` Transform 与世界尺寸策略；非武器语义路径保持原契约。
- 六个生产 Weapon Profile 已通过 Editor API 固化绝对世界长度；长剑 `AttackCommitted` 配置为 `Z=60 UU`、`Roll=-45°` 并保持世界尺寸。配置脚本只修改锁定字段，可重复执行。
- 自动化直接比较不同宿主缩放下的六把武器解析长度，Player/Echo 使用同一 Profile 时结果一致；同步更新三份受影响模块文档。
- 用户明确要求不合并远端新增 9 个提交并继续在本 Plan 执行；候选仍基于当前 Plan100 分支。兔子球命中玩家后立即发布 `Ended` 并从 EnemyHost 逻辑数组移除，对应 VFX 代理同步结束。
- `Pattern.LongSwordCombo` 在既有近战提交中复用同一 Origin、AimDirection、RangeCm 与 ArcDegrees 查询所有兔子 Host 的逻辑球；弧内球由 Host 权威结束。其他攻击模式和非兔子投射物不进入该路径。
- 普通攻击与主动攻击的成功 Commit 统一由 `PublishAttackCommittedEvent` 组装并发布表现事件；此前缺事件的手动主动攻击现在会释放对应武器特效，镰刀召回仍不产生第二次 Commit/特效。
- `UReEchoAttackControllerComponent` 成为共享 GAS 普攻输入的唯一来源所有者：模式切换统一释放旧 held；手动/自动入口在接管前清理异常旧来源；任一来源的迟到 Release 只释放自身，不能误停另一来源。Pawn 移除重复的模式切换后二次释放分支。
- 发布前按用户确认合入 `origin/main@e34f8f5f`：远程商店、Boss/敌人数值、角色变形与 Combat 属性修复全部保留；Pawn、WeaponActor 与模块文档自动组合，无文本冲突。预构建包不做二进制语义合并，统一由最终集成源码 FullRebuild 重生。用户确认将 `BP_ArenaScene_SC01.uasset` 纳入候选，`WBP_ReEchoEncounterHud.uasset` 继续排除。

### 证据

- Development FullRebuild 通过：`99/99` actions，预构建 source fingerprint `af03798737dd`。
- `ReEcho.Presentation.VFX.Catalog` 与 `ReEcho.Presentation.Combat.Capabilities` 精确自动化通过；覆盖长剑 DA 位姿/世界尺寸以及六武器 Player/Echo 不同宿主缩放下的相同世界长度。
- Editor 配置结果：`PLAN100_WEAPON_PRESENTATION_RESULT profiles=6 sword_z=60 sword_roll=-45`；六个资产 Data Validation 通过。
- `prebuilt_editor.py check` 与 `git diff --check` 通过。
- 扩展候选 Development FullRebuild 通过：`99/99` actions，预构建 source fingerprint `4047a64998fb`。
- `ReEcho.Enemies.Host.RabbitProjectilePipeline` 聚焦自动化找到 1 项并通过；覆盖命中玩家后逻辑/视觉立即结束，以及 180°扇区移除剩余兔子球。启动日志仍包含 LinuxArm64/VisionOS `MainVersion` 警告，但本次 Win64 测试实际执行并返回 `Success`。
- 扩展候选 `validate_project.py`、`prebuilt_editor.py check` 与 `git diff --check` 通过。
- 手动主动攻击特效候选 Development FullRebuild 通过：`96/96` actions，预构建 source fingerprint `f7953bfef1ba`。首次构建因测试 getter 误置于 `PublishHurt` 函数体内产生 C2270/C2601，移动到组件测试区后重建通过；运行时事件实现本身未出现编译错误。
- `ReEcho.Weapons.Runes.GroupOuterAndScytheHandlers` 聚焦自动化找到 1 项并通过：首次主动镰刀 Commit 发布一次 `Pattern.ScytheSweep`，召回不重复发布。`validate_project.py`、预构建一致性与 `git diff --check` 同步通过。
- 手动/自动互斥候选 Development FullRebuild 通过：`96/96` actions，预构建 source fingerprint `17225d9af900`；`ReEcho.AttackMode.InputSource` 找到 1 项并通过，覆盖自动模式拒绝物理输入、手动模式拒绝自动入口，以及切换模式释放旧 held。`validate_project.py`、预构建一致性与 `git diff --check` 通过。
- 最终远程集成候选 Development FullRebuild 通过：`109/109` actions，预构建 source fingerprint `29b4632d124d`。`ReEcho.AttackMode.InputSource`、`ReEcho.Enemies.Host.RabbitProjectilePipeline`、`ReEcho.Weapons.Runes.GroupOuterAndScytheHandlers` 各找到 1 项并通过；兔子测试改为按远程最新能力数据的权威半径/速度动态布置扫掠样本，Rune 测试跳过策划已禁用的陨星与投掷召回效果，避免对禁用内容解引用空 Weapon。最终 `validate_project.py`、预构建一致性与 `git diff --check` 通过。
- `validate_project.py` 的非 XLSX 检查完成，但总结果受 worktree `Content/reecho_xlsx_package_*` 创建权限拒绝阻塞；本 Plan 未修改 XLSX/CSV。
- 全量 `ReEcho.Weapons` 暴露既有非本任务失败：生产 rune 数预期 47/实际 46、`P_GUN_RAPID_MUZZLE` 已禁用，以及 WeaponRuntime 临时 CSV 断言；精确受影响测试已独立通过。

### 剩余风险

视觉效果仍需要用户 PIE 验收；自动化只能验证 Transform 与世界尺寸计算契约。当前环境缺少 clang-format 可执行文件，FullRebuild 已验证编译格式但未取得独立 clang-format 工具证据。全量 Weapons 与项目校验的既有环境/数据失败见上节。

斩弹手感仍需 PIE：确认长剑正面 180°内球消失、背后球不受影响、兔子球命中角色后不残留。按用户选择未合并 `origin/main` 的 9 个外部提交，发布前必须重新审计并集成。

### 人工验收结果/请求

`PendingBeforeClose`：请在 PIE 对比 Player/Echo 同一武器大小，并观察长剑斩击是否完整离地、方向与攻击同步无回归。

### 架构文档审阅结果

- `MOD-ReEcho.md` 已更新：记录生产武器绝对长度及 Player/Echo 宿主缩放不再二次影响尺寸。
- `MOD-ReEchoVFX.md` 已更新：记录武器 Slot Transform/世界尺寸策略是唯一位姿真相。
- `MOD-ReEchoWeapons.md` 已更新：记录主模块 DA 的绝对长度和完整 VFX Slot 表现契约。
- `MOD-ReEchoWeapons.md` 已补充长剑复用提交几何斩断兔子逻辑球的边界；`MOD-ReEchoEnemies.md` 已更新兔子球命中即结束和 EnemyHost 移除权威。
- `MOD-ReEchoCombat.md` 已审阅并补充所有成功的普通/主动 Commit 共用 `AttackCommitted` 表现出口；测试计数器仅在 Development 自动化宏内存在，不改变运行时事件 ABI。
- `MOD-ReEchoPresentation.md`、`ARCHITECTURE.md`、`CODEBASE_MAP/README.md` 已审阅、无需修改：本实现未改变模块拓扑、Presentation 模块公共类型或索引路由。
