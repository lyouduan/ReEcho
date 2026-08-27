# Plan 137 - 程序 - 枪口火花绑定最终武器挂点

## 协调

- Planner 负责人：Codex（程序路线）。
- Executor 负责人：Codex（程序路线）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Accepted`。
- 人工验收：`Accepted`（用户 PIE 反馈“暂时正常”，同意合入远程）。
- 本地规划 / 实现基线：Plan-only 已发布至 `origin/main@2b1fc6c5`，实现 worktree 直接基于该提交。
- 本地实现方式（可选，仅作交接说明）：`C:\tmp\ReEcho-plan137-gun-muzzle-vfx`，分支 `codex/plan137-gun-muzzle-vfx`。
- 依赖 / 阻塞：复用 Plan126 已发布并验收的 `WeaponAttackVfxRoot`、Gun `AttackVfxAnchorRatio` 与负 X Scale 镜像补偿；执行前 Plan-only 发布到最新 `origin/main`。
- Writes: `plans/137-programmer-gun-muzzle-vfx-anchor.md`；`Source/ReEcho/Public/Presentation/VFX/ReEchoCombatVfxCatalog.h`；`Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxCatalog.cpp`；`Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxComponent.cpp`；`Source/ReEcho/Private/Graybox/ReEchoProjectileActor.cpp`；`Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`；`Source/ReEcho/Private/Tests/ReEchoCombatPresentationTests.cpp`；`Source/ReEcho/Private/Tests/ReEchoRuntimeAssetPreloadTests.cpp`；`Content/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_Gun.uasset`；`Content/VFX/People/Bullet/Particle/NS_People_Bullet_spark.uasset`；用于安全修改上述资产的 `scripts/ue/` 最小自动化；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`；FullRebuild 刷新的 `Binaries/Win64/ReEchoEditor.prebuilt.json` 及其准确允许列表产物。
- Stable Reads: `Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`；`Source/ReEcho/Public/Weapons/ReEchoWeaponActor.h`；`Source/ReEcho/Private/Weapons/ReEchoWeaponActor.cpp`；`Source/ReEcho/Public/Presentation/Weapon/ReEchoWeaponPresentationProfile.h`；`Content/Data/weapons.csv`；`Content/Data/attack_steps.csv`；`/Game/VFX/People/Bullet/Particle/NS_People_Bullet_spark`。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：枪弹 `Travel`、投射物生成位置与伤害逻辑不变；弓、长剑、镰刀不变；表现缺失仍不得阻断攻击。保留最新主线的导电 VFX 端点和计时逻辑。
- 明确排除：不改变枪攻击频率、伤害、弹速、射程、碰撞、投射物数量；不修改枪贴图、已验收枪口锚点或其他武器 DA；不把 Niagara 位置用于玩法结算；不同时把同一火花作为命中反馈重复播放。

## 锁定目标

每次成功提交 `Pattern.GunShot` 时，`NS_People_Bullet_spark` 必须立即从该次枪的最终 `WeaponAttackVfxRoot` 播放：枪口点由 `GunSprite` 实际局部 Bounds 的中心沿最终可见枪管方向移动半个枪长，枪朝右位于最右端中心，枪朝左位于最左端中心。该火花是枪口释放表现而不是命中表现；不从角色中心、人物宽度、目标位置或固定世界偏移重新计算。

Gun 当前可见模型只区分左右，枪口 Sprite 方向也只消费瞄准向量在相机水平轴上的符号；向上/向下瞄准不得让枪口视觉主体绕 Pivot 上下浮动。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` / `AREA-Presentation`、文档型逻辑入口 `MOD-ReEchoVFX`；不修改 `MOD-ReEchoCombat` 或 `MOD-ReEchoWeapons` 的运行时模块契约。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`，均已加入 `Writes`。
- 设计意图：让 Gameplay/Weapons 继续只发布资源中立的 `FReEchoAttackCommittedEvent`，由主模块 VFX Adapter 将 `Pattern.GunShot` 映射到枪 DA 的释放槽并附着最终武器挂点。
- 权威状态与依赖：枪口位置权威仍为 `AReEchoWeaponActor::WeaponAttackVfxRoot`；Gun Profile 只拥有资源、尺寸和局部表现修正；攻击提交、方向与伤害仍由 Combat/Weapons 拥有。公共事件结构和模块依赖方向不变。
- 决策记录：使用明确的 `PlayerGunMuzzle` 语义替代误导性的 `PlayerGunImpact`；Gun DA 将 `NS_People_Bullet_spark` 从 `DamageApplied` 迁至 `AttackCommitted`，`SpawnMode=AttachToAttackRoot`。不复用目标命中位置，也不增加第二套枪口坐标。
- 相关文档同步范围：更新 `MOD-ReEcho.md` 与 `MOD-ReEchoVFX.md`；审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEchoPresentation.md` 与 `MOD-ReEchoWeapons.md`，拓扑、路由或模块契约未变化时记录无需修改。
- 关闭前逐项填写审阅结果：待实现与 PIE 验收后补充。

## 锁定验收

- [x] `Pattern.GunShot` 的每次成功 AttackCommitted 立即解析为 `PlayerGunMuzzle`，其他 Pattern 不误入该语义。
- [x] `NS_People_Bullet_spark` 从 Gun Profile 的 `AttackCommitted` Slot 解析，`DamageApplied` 不再把它声明为枪命中特效。
- [x] 枪口火花附着 `WeaponAttackVfxRoot`；位置由 Gun StaticMesh 局部 Bounds 中心加减实际半枪长得到，朝右为最右端中心、朝左为最左端中心；Niagara Sprite renderer 使用屏幕方向参数，使左向贴图相对右向精确反转 180 度。
- [x] 枪弹 `Travel`、生成位置和伤害保持不变；实现只删除枪目标点火花分支，保留投射物及弓命中路径，Gun DA 脚本断言 Travel 不变。
- [x] FullRebuild、聚焦自动化、项目校验、LFS、预构建检查和 `git diff --check` 通过。
- [x] 用户完成左右朝向、不同瞄准高度的 PIE 视觉验收；未提交精选预构建允许列表之外的生成物。

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

- 新增 `PlayerGunMuzzle` 与 `Pattern.GunShot` 的 AttackCommitted 路由，复用 `WeaponAttackVfxRoot` 立即附着播放。
- Gun Profile 将 `NS_People_Bullet_spark` 从 `DamageApplied` 迁移到 `AttackCommitted`，SpawnMode 为 `AttachToAttackRoot`；DA 垂直修正归零，枪口位置改由 `GunSprite` StaticMesh 局部 Bounds 中心加减实际半枪长计算，Travel 保持不变。
- `NS_People_Bullet_spark` 的全部启用发射器改为 Local Space，全部启用 Sprite renderer 绑定 `User.DirectionSpriteRotationDegrees`；运行时按相机屏幕基写入攻击方向，使朝左贴图相对朝右精确反转 180 度，不依赖负缩放或组件旋转是否被 FaceCamera 消费。
- ProjectileActor 不再在枪弹命中目标时重复播放该火花，弓命中逻辑保持不变。

### 证据

- `FullRebuild`：通过；最终 Bounds 枪长端点与 SpriteRotation 参数方案再次通过并刷新预构建包为 `build_id=55116800`、`source=5921b6fcfbcd`。
- 自动化：`ReEcho.Presentation.VFX.Catalog`、`ReEcho.Presentation.Combat.Capabilities`、`ReEcho.Presentation.RuntimeAssetPreload.Catalog` 均 Success。
- `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`python scripts/setup_lfs.py --check`、`git diff --check`：通过。
- 首次自动化命令因漏写 `-Filter` 误跑全套，并在既有 `ReEchoWeaponRuntimeTests.cpp:184` CSV 变体断言处终止；改用正确过滤参数后上述三项目标测试独立通过。锚点调整后的首次 Capabilities 采用精确浮点比较而失败，改为分量近似比较后复测 Success；资产读回值为 `(0.500000,-0.100000)`。

### 剩余风险

- `-nullrhi` 自动化已锁定实际半枪长左右端点换算、Local Space、Sprite renderer 参数绑定与 DA 配置，但不能证明 Niagara 在真实相机中的最终画面；仍需 PIE 对左右朝向和连续开枪进行最终视觉确认。

### 人工验收结果/请求

`Accepted`：用户在 PIE 中复测左右朝向、枪长端点和瞄准高度后反馈“暂时正常”，并明确要求合入远程。

### 架构文档审阅结果

- 已更新 `MOD-ReEcho.md` 与 `MOD-ReEchoVFX.md`，记录枪口语义、最终武器根和取消目标点重复火花。
- 已审阅 `ARCHITECTURE.md`、代码库入口及 Presentation/Weapons 模块边界；本次未改变模块依赖、公共 Combat 事件或 Weapons 契约，无需扩大文档修改范围。
