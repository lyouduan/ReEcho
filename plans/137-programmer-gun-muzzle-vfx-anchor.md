# Plan 137 - 程序 - 枪口火花绑定最终武器挂点

## 协调

- Planner 负责人：Codex（程序路线）。
- Executor 负责人：Codex（程序路线）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@9c32f80a5bd7be24ea4c82a4cfa2715f11fca6e3`。
- 本地实现方式（可选，仅作交接说明）：`C:\tmp\ReEcho-plan137-gun-muzzle-vfx`，分支 `codex/plan137-gun-muzzle-vfx`。
- 依赖 / 阻塞：复用 Plan126 已发布并验收的 `WeaponAttackVfxRoot`、Gun `AttackVfxAnchorRatio` 与负 X Scale 镜像补偿；执行前 Plan-only 发布到最新 `origin/main`。
- Writes: `plans/137-programmer-gun-muzzle-vfx-anchor.md`；`Source/ReEcho/Public/Presentation/VFX/ReEchoCombatVfxCatalog.h`；`Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxCatalog.cpp`；`Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxComponent.cpp`；`Source/ReEcho/Private/Graybox/ReEchoProjectileActor.cpp`；`Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`；`Source/ReEcho/Private/Tests/ReEchoCombatPresentationTests.cpp`；`Source/ReEcho/Private/Tests/ReEchoRuntimeAssetPreloadTests.cpp`；`Content/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_Gun.uasset`；用于安全修改该 DA 的 `scripts/ue/` 最小自动化；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`；FullRebuild 刷新的 `Binaries/Win64/ReEchoEditor.prebuilt.json` 及其准确允许列表产物。
- Stable Reads: `Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`；`Source/ReEcho/Public/Weapons/ReEchoWeaponActor.h`；`Source/ReEcho/Private/Weapons/ReEchoWeaponActor.cpp`；`Source/ReEcho/Public/Presentation/Weapon/ReEchoWeaponPresentationProfile.h`；`Content/Data/weapons.csv`；`Content/Data/attack_steps.csv`；`/Game/VFX/People/Bullet/Particle/NS_People_Bullet_spark`。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：枪弹 `Travel`、投射物生成位置与伤害逻辑不变；弓、长剑、镰刀不变；表现缺失仍不得阻断攻击。保留最新主线的导电 VFX 端点和计时逻辑。
- 明确排除：不改变枪攻击频率、伤害、弹速、射程、碰撞、投射物数量；不修改枪贴图、已验收枪口锚点或其他武器 DA；不把 Niagara 位置用于玩法结算；不同时把同一火花作为命中反馈重复播放。

## 锁定目标

每次成功提交 `Pattern.GunShot` 时，`NS_People_Bullet_spark` 必须立即从该次枪的最终 `WeaponAttackVfxRoot` 播放：枪朝右位于枪贴图右侧枪口，枪朝左位于枪贴图左侧枪口。该火花是枪口释放表现而不是命中表现；左右位置只消费 Plan126 已验收的武器局部锚点和镜像换算，不从角色中心、人物宽度、目标位置或固定世界偏移重新计算。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` / `AREA-Presentation`、文档型逻辑入口 `MOD-ReEchoVFX`；不修改 `MOD-ReEchoCombat` 或 `MOD-ReEchoWeapons` 的运行时模块契约。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`，均已加入 `Writes`。
- 设计意图：让 Gameplay/Weapons 继续只发布资源中立的 `FReEchoAttackCommittedEvent`，由主模块 VFX Adapter 将 `Pattern.GunShot` 映射到枪 DA 的释放槽并附着最终武器挂点。
- 权威状态与依赖：枪口位置权威仍为 `AReEchoWeaponActor::WeaponAttackVfxRoot`；Gun Profile 只拥有资源、尺寸和局部表现修正；攻击提交、方向与伤害仍由 Combat/Weapons 拥有。公共事件结构和模块依赖方向不变。
- 决策记录：使用明确的 `PlayerGunMuzzle` 语义替代误导性的 `PlayerGunImpact`；Gun DA 将 `NS_People_Bullet_spark` 从 `DamageApplied` 迁至 `AttackCommitted`，`SpawnMode=AttachToAttackRoot`。不复用目标命中位置，也不增加第二套枪口坐标。
- 相关文档同步范围：更新 `MOD-ReEcho.md` 与 `MOD-ReEchoVFX.md`；审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEchoPresentation.md` 与 `MOD-ReEchoWeapons.md`，拓扑、路由或模块契约未变化时记录无需修改。
- 关闭前逐项填写审阅结果：待实现与 PIE 验收后补充。

## 锁定验收

- [ ] `Pattern.GunShot` 的每次成功 AttackCommitted 立即解析为 `PlayerGunMuzzle`，其他 Pattern 不误入该语义。
- [ ] `NS_People_Bullet_spark` 从 Gun Profile 的 `AttackCommitted` Slot 解析，`DamageApplied` 不再把它声明为枪命中特效。
- [ ] 枪口火花附着 `WeaponAttackVfxRoot`；朝右位于枪右侧、朝左位于枪左侧，且复用已验收 DA 锚点和负缩放补偿。
- [ ] 枪弹 `Travel`、生成位置和伤害保持不变；弓、长剑、镰刀相关自动化不回归。
- [ ] FullRebuild、聚焦自动化、项目校验、LFS、预构建检查和 `git diff --check` 通过。
- [ ] 用户完成左右朝向 PIE 视觉验收；未提交精选预构建允许列表之外的生成物。

## Step 0 门禁

- 基线分支/提交：`origin/main@9c32f80a5bd7be24ea4c82a4cfa2715f11fca6e3`。
- 引擎/构建可用性：UE 5.8 Windows；Git LFS checkout 已通过；Editor/命令前取得 Git-common-dir Unreal 锁。
- 现有聚焦测试结果：静态诊断确认 `NS_People_Bullet_spark` 当前位于 Gun `DamageApplied/PlayerGunImpact`；`HandleAttackCommitted` 只接受长剑/镰刀 Pattern，未提供枪口提交路径；`ReEchoProjectileActor` 仍在枪弹首次正伤害时直接生成该 Impact，迁移时必须删除枪的目标点重复播放而保留弓命中。
- 共享契约 / 难合并资源风险：Gun DA 为二进制资产；只通过 Editor Python API 修改并提交精确该资产。最新主线的导电端点修复修改同一 VFX Component/测试，当前 worktree 已直接包含并必须保留。
- 基线损坏时的停止条件：生产 Gun Profile 或 Niagara 资源缺失/LFS 未还原；远程改变枪口根节点所有者；资产实际需要同时保留命中语义而产生产品取舍。

## 实现提纲

1. 新增枪口释放语义与 `Pattern.GunShot` AttackCommitted 路由，立即附着 `ResolveWeaponAttackVfxRoot()`。
2. 通过 Editor 自动化把 Gun Profile 的火花迁至 `AttackCommitted`，保持既有枪口锚点、Held 配置与 Travel Slot 不变；ProjectileActor 删除枪目标点 Impact，弓 Impact 不变。
3. 增加路径、Pattern、Slot、左右挂点/镜像和其他武器负例自动化。
4. 维护模块文档和执行记录，完成构建、校验与 PIE 交接。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| LFS | `python scripts/setup_lfs.py --check`、`git lfs status`、`git lfs fsck` | Gun DA 与 Niagara 依赖均为完整对象 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目、文档和源码不变量通过 |
| C++ | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新精选预构建包 |
| 聚焦自动化 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Presentation.Combat` | 枪口语义、DA Slot、武器根与既有近战表现通过 |
| 人工 PIE | 枪分别朝右、朝左连续攻击 | 每次火花位于当前枪口侧，不卡在人物中心或目标命中点 |

## 执行记录

### 变化

### 证据

### 剩余风险

### 人工验收结果/请求

`PendingBeforeClose`：实现后请求用户验收枪左右朝向和连续射击时的枪口火花位置。

### 架构文档审阅结果
