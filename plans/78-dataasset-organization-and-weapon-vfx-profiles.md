# Plan 78 - 程序 - DataAsset 分类整理与武器三阶段特效配置

## 协调

- Planner / Executor：当前对话同一程序 AI；用户已确认不采用规划者-执行者拆分。
- 工作区：一任务一 worktree，`codex/dataasset-organization`。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`。
- 规划基线：`origin/main@bda8985077f447bf8bf155caf6fb0d9e3dda3286`。
- 前置结果：Plan77 已建立统一表现生命周期与一武器一表现 Profile 的 C++ 过渡实现；本 Plan 将武器表现迁移为 Editor 可配置 DataAsset，并整理角色/怪物相关 DA 目录。
- Writes:
  - `plans/78-dataasset-organization-and-weapon-vfx-profiles.md`
  - `Source/ReEcho/{Public,Private}/Presentation/Weapon/ReEchoWeaponPresentationProfile.*`
  - `Source/ReEcho/{Public,Private}/Presentation/Weapon/ReEchoWeaponPresentationCatalog.*`
  - `Source/ReEcho/{Public,Private}/Weapons/ReEchoWeaponVisualCatalog.*`
  - `Source/ReEcho/{Public,Private}/Weapons/ReEchoWeaponActor.*`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxCatalog.*`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxComponent.*`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoProjectileActor.*`
  - `Source/ReEcho/{Public,Private}/Presentation/Loading/ReEchoRuntimeAssetPreloader.*`
  - `Source/ReEcho/{Private}/Player/ReEchoPlayerPawn.cpp`
  - `Source/ReEcho/{Private}/Graybox/ReEchoEchoActor.cpp`
  - `Source/ReEcho/{Private}/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Tests/ReEcho2DAnimationTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoWeaponPresentationTests.cpp`
  - `scripts/ue/migrate_plan78_dataassets.py`
  - 受新目录影响的现有 `scripts/ue/` Animation2D Profile/Catalog 配置与审计脚本
  - `Content/ReEcho/DataAsset/Character/**`
  - `Content/ReEcho/DataAsset/Enemy/**`
  - `Content/ReEcho/DataAsset/Weapon/**`
  - `Content/ReEcho/DataAsset/Common/**`
  - 从 `Content/ReEcho/Animation2D/` 搬迁的现有 Character/Echo/Enemy Profile、Catalog、FSM
  - 从 `Content/ReEcho/Gameplay/CharacterPrefabs/` 搬迁的 `DA_EnemyGameplayClassRegistry`
  - `Config/DefaultEngine.ini` 中新旧准确资产路径的 Core Redirect（仅在 UE 重命名不能覆盖字符串/旧存档引用时保留）
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`
  - `Binaries/Win64/` 下 `shared/GIT_RULES.md` 允许的最终预构建包文件
- Stable Reads:
  - `Content/Data/weapons.csv` 的 `VisualKey / AttackPatternId / Carrier`
  - `Content/VFX/People/{Sword,Sickle,Bow,Bullet}/**`
  - Plan74、Plan76、Plan77 的现有动画/VFX/生命周期契约
- 影响模式：`SharedContract`。新增武器表现 DataAsset 公共类型，调整主模块资产解析和既有 DA 路径；不改变 Combat、Enemies、Weapons 的玩法权威或依赖方向。
- 明确排除：不移动 Scene/UI/Audio 等非战斗主体 DataAsset；不改武器数值、伤害、节拍、碰撞、弹道；不修改 Niagara 内部资产；不为普通怪物创建武器；不把狐狸/兔子 Ability VFX 混入武器 Profile。

## 锁定目录

```text
/Game/ReEcho/DataAsset/
  Character/
    Profiles/        DA_Character_*、DA_Echo_*
    Catalogs/        DA_CharacterPresentationCatalog、DA_EchoPresentationCatalog
  Enemy/
    Profiles/        DA_Enemy_*
    Catalogs/        DA_EnemyPresentationCatalog、DA_EnemyGameplayClassRegistry
  Weapon/
    Profiles/        DA_WeaponPresentation_*
    Catalogs/        DA_WeaponPresentationCatalog
  Common/
    Animation2D/     SM2D_DefaultCharacter
```

混合角色与怪物的旧 `DA_PresentationCatalog` 拆为 Character/Enemy 两个 Catalog：Player 只加载 Character Catalog，GameMode/Enemy Host 只加载 Enemy Catalog；Echo 保持独立 Catalog。旧路径只通过准确 Redirect/升级兼容，不继续作为第二份正式资产。

## 武器 Profile 契约

每个 `WeaponVisualKey` 唯一解析一个 `UReEchoWeaponPresentationProfile`，Profile 只拥有表现：

- 手持武器 Soft Reference、武器 MotionMode/Duration；
- `Charge`、`Travel`、`DamageApplied` 三个可选 `FReEchoWeaponVfxSlot`；
- 每个 Slot 包含 `bEnabled`、Niagara Soft Reference、SpawnMode、AttachPoint、相对 Transform、排序偏移和阶段结束清理策略。

运行规则：

1. 未勾选：不加载、不预热、不播放；即便字段残留资源也忽略并告警。
2. 已勾选且资源有效：只在对应权威阶段播放。
3. 已勾选但资源为空：Editor/自动化报告配置错误；运行时跳过表现，玩法继续。
4. Charge 由 Windup 开始，在 Commit/Cancel/Death/换武器停止。
5. Travel 只能绑定权威 Projectile/Wave Carrier，在 Impact/Expired/Cancel/Carrier 销毁时停止；Melee 勾选 Travel 为非法配置。
6. DamageApplied 只消费 `AppliedDamage > 0` 的 Combat 最终结果，并以 `AttackIdentity + Target` 去重；格挡、无敌、零伤害不播放。

## 架构影响与原则

- `weapons.csv` 继续拥有玩法 Definition 和 `VisualKey`；DA 不保存攻击间隔、伤害、范围、碰撞或弹道。
- `UReEchoWeaponPresentationCatalog` 是 `WeaponVisualKey -> Profile` 唯一入口，Player/Echo/显式持武器 Boss 可复用；普通怪物强制无 Weapon Track。
- 武器 VFX 从 `FReEchoCombatVfxCatalog` 的武器硬编码路径迁入 Weapon Profile；怪物 Ability VFX、受击与元素 VFX 继续由 Combat VFX Catalog 管理。
- Soft Reference 进入 Catalog、Cook 和预加载引用闭包；同步 `LoadObject` 只保留为明确的安全降级，不再按武器写死路径。
- `.uasset` 只通过关闭 Editor 后的 Unreal Editor Python 重命名/创建/保存，禁止文件系统直接移动。

## 实现步骤

1. 新增 Weapon Presentation Profile/Catalog 类型、三阶段 Slot、配置校验与运行时只读解析接口。
2. 创建六把武器 Profile 和 Catalog，将长剑、镰刀、弓、枪现有 Niagara 迁入对应阶段；鞭/法杖保持阶段未勾选并保留已知旧表现兼容。
3. 将 WeaponActor、ProjectileActor、Combat VFX 和预加载器切换到 DA；保留游戏逻辑事件与载体权威。
4. 使用 Editor Python 创建目标目录、拆分 Character/Enemy Catalog、移动现有 Profile/FSM/Registry，并保存引用者；更新 C++、脚本、测试中的准确路径。
5. 执行资产加载/引用审计、聚焦自动化、FullRebuild、静态校验和 PIE 人工验收。

## 锁定验收

- [ ] Content Browser 中战斗主体 DA 只位于 `DataAsset/Character|Enemy|Weapon|Common` 约定目录，旧正式路径不存在重复资产。
- [ ] Character、Enemy、Echo Catalog 分域加载正确，全部生产 Profile、Gameplay Blueprint Class 与共享 FSM 可解析。
- [ ] 六个 WeaponVisualKey 各唯一解析一个 Weapon Profile；未知 Key 安全返回空。
- [ ] 三阶段未勾选时不加载、不预热、不播放；勾选但空资源可检测且不影响玩法。
- [ ] 长剑/镰刀 DamageApplied、弓/枪 Travel + DamageApplied 按现有正式 Niagara 配置工作；若当前长剑/镰刀仍要求 Commit 斩击，使用明确 `AttackCommitted` Slot 扩展而不是伪装成 DamageApplied，并在实现记录说明。
- [ ] Melee Travel 非法配置、DamageApplied 零伤害不播放、同 Attack+Target 去重均有测试。
- [ ] 普通怪物无 Weapon Profile；Boss 只在显式配置时允许；狐狸/兔子 Ability VFX 不进入 Weapon Catalog。
- [ ] 通过 C++ 格式化、资产审计、相关自动化、Development FullRebuild、`validate_project.py`、`git diff --check` 和预构建包一致性检查。
- [ ] 用户在 Editor/PIE 验收目录、Profile 可编辑性及长剑、弓实际表现。

## Step 0 门禁

- 当前检测到 Unreal Editor 正在运行；在关闭并确认无未保存内容前，不执行任何 `.uasset` 创建、移动、重命名、保存或构建。
- Plan 发布前重新 fetch；若 `origin/main` 出现外部提交，先报告重叠并由用户决定。
- 资产迁移前记录旧路径、目标路径和 Git 恢复点；失败时保留 worktree，不删除旧资产或 Redirector。
- 若拆分 Catalog 会造成无法自动修复的 Blueprint/Level 二进制引用，停止并报告准确引用者，不强行删除旧资产。

## 验证矩阵

| 层级 | 检查 | 预期证据 |
|---|---|---|
| C++ | clang-format、FullRebuild | UHT/UBT 通过并刷新预构建包 |
| DA 配置 | `ReEcho.Presentation.Weapon` | 唯一映射、阶段开关、非法组合和缺资源校验 |
| 动画目录 | `ReEcho.Presentation.Animation2D` + Plan74 audit | Character/Enemy/Echo Profile 与 FSM 新路径可加载 |
| VFX | `ReEcho.Presentation.VFX` | 武器阶段映射、零伤害和去重，怪物 VFX 不回归 |
| 武器 | `ReEcho.Weapons` | 节拍、载体、命中不变 |
| 资产引用 | Editor AssetRegistry / Redirector audit | 旧路径无正式引用，目标资产可加载，Catalog 软引用完整 |
| 静态 | validate_project、diff check、prebuilt check | 项目和发布包一致 |
| 人工 | Editor + PIE | 目录清晰，DA 可编辑，长剑/弓阶段表现正确 |

## 执行记录

### 变化

- 已将 Character、Echo、Enemy Profile/Catalog、Enemy Gameplay Registry 与共享 FSM 迁入锁定的 `DataAsset` 分域目录；原混合 Catalog 拆为 Character/Enemy，并修复所有生产加载路径。
- 新增 Weapon Presentation Profile/Catalog 类型与六把武器 DA。长剑/镰刀使用明确的 `AttackCommitted`；弓/枪使用 `Travel + DamageApplied`；鞭/法杖对应阶段保持未勾选。
- 武器 Actor、Projectile、Combat VFX 与预加载闭包改从 Weapon DA 解析；`DamageApplied` 仅在最终正伤害后触发。普通怪物与狐狸/兔子 Ability VFX 未进入 Weapon Catalog。
- 迁移脚本可重复运行；缺失的 Whip/WhipLash 原始贴图按可选缺口保留为空，不创建伪造回退。

### 证据

- `migrate_plan78_dataassets.py` 连续复跑成功：`PLAN78_DATAASSET_RESULT character=8 enemy=8 weapon=6`，0 error；仅报告既有 Whip/WhipLash 缺失告警。
- `ReEcho.Presentation.Combat`、`ReEcho.Presentation.VFX`、`ReEcho.Weapons` 通过。
- `ReEcho.Presentation.Animation2D` 的目录迁移、Character/Enemy Catalog 解析均通过；套件仍仅因迁移前已知的 TimeGuard Phase2 四语义素材不完整失败。
- 最终 FullRebuild、静态校验与预构建一致性见提交前验证记录。

### 剩余风险与人工验收

- 待用户在 Editor/PIE 验收目录可编辑性，以及长剑 Commit、弓 Flight/Impact 实际画面与角色动画的同步观感。
- TimeGuard Phase2 缺失素材属于既有基线问题，不在本次 DA 分类与武器 VFX 配置范围内。
- Whip/WhipLash 源贴图当前不存在，因此相关字段保持未配置；补入正式素材后可直接在 Weapon Profile 勾选并绑定。
