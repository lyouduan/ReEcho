# Plan 139 - 程序 - 枪械元素子弹飞行特效

## 协调

- Planner 负责人：Codex（程序路线）。
- Executor 负责人：Codex（程序路线）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@2b1fc6c5d45bd4d6625824c4fef6959571f3a04b`。
- 本地实现方式（可选，仅作交接说明）：`C:\tmp\ReEcho-element-reaction-setting-fix`，分支 `codex/element-reaction-setting-fix`。
- 依赖 / 阻塞：复用现有 Gun 逻辑投射物与逐弹 `ProjectileElement`；与 Plan137 共享 `ReEchoCombatVfxCatalog.*`、`ReEchoProjectileActor.cpp` 和测试文本路径，但不修改 Plan137 独占的 Gun Profile 二进制资产或枪口火花资源。
- Writes: `plans/139-programmer-elemental-gun-projectile-vfx.md`；`Content/VFX/People/Bullet/Particle/NS_People_Bullet_Fire_Fly.uasset`；`Content/VFX/People/Bullet/Particle/NS_People_Bullet_Thunder_Fly.uasset`；`Content/VFX/People/Bullet/Particle/NS_People_Bullet_Grass_Fly.uasset`；`Content/VFX/People/Bullet/Particle/NS_People_Bullet_Water_Fly.uasset`；`scripts/ue/fix_plan139_elemental_gun_flight_local_space.py`；`Source/ReEcho/Public/Presentation/VFX/ReEchoCombatVfxCatalog.h`；`Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxCatalog.cpp`；`Source/ReEcho/Public/Graybox/ReEchoProjectileActor.h`；`Source/ReEcho/Private/Graybox/ReEchoProjectileActor.cpp`；`Source/ReEcho/Public/Weapons/ReEchoWeaponActor.h`；`Source/ReEcho/Private/Weapons/ReEchoWeaponActor.cpp`；`Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`；`Source/ReEcho/Private/Tests/ReEchoRuntimeAssetPreloadTests.cpp`；`Source/ReEcho/Private/Tests/ReEchoWeaponRuntimeTests.cpp`；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`；FullRebuild 刷新的 `Binaries/Win64/ReEchoEditor.prebuilt.json` 及其准确允许列表产物。
- Stable Reads: `Content/Data/weapons.csv`；`Content/Data/parts.csv`；`Content/Data/part_effects.csv`；`Source/ReEchoWeapons/Public/Weapons/ReEchoWeaponTypes.h`；`Source/ReEcho/Public/Presentation/Weapon/ReEchoWeaponPresentationProfile.h`；`Content/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_Gun.uasset`；`plans/137-programmer-gun-muzzle-vfx-anchor.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：元素选择、伤害、反应、碰撞、弹速、射程、弹数与随机确定性不变；无元素枪弹和元素资产缺失时回退当前 `PlayerGunFlight`；Bow 与其他载体不变；Plan137 的 `PlayerGunMuzzle`、枪口锚点和 Gun Profile 迁移不被覆盖。
- 明确排除：不修改生产 XLSX/CSV、元素反应公式、枪伤害与攻击频率；不增加元素命中特效；不修改四个 Niagara 内部发射器；不把飞行表现作为玩法元素或位置权威；不修改 Plan137 的 Gun Profile 二进制资产。

## 锁定目标

装备枪 `W_J_09` 时，每颗逻辑子弹必须按该弹已经确定的最终战斗元素播放对应飞行 Niagara：`Flame → NS_People_Bullet_Fire_Fly`、`Lightning → NS_People_Bullet_Thunder_Fly`、`Grass → NS_People_Bullet_Grass_Fly`、`Water → NS_People_Bullet_Water_Fly`。固定元素核心和棱镜逐弹随机元素使用同一选择路径；无元素或资源不可用时继续播放现有 Gun Travel，不能阻止逻辑投射物、命中或伤害。Development `GMElement` 覆盖须在投射物初始化前参与最终元素选择，使飞行表现与实际 `HitIntent.Element` 一致。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` / `AREA-Presentation`、文档型逻辑入口 `MOD-ReEchoVFX`；只稳定读取 `MOD-ReEchoWeapons` 和 `MOD-ReEchoCombat` 的既有逐弹元素/命中契约。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`，均已加入 `Writes`；审阅 `MOD-ReEchoWeapons.md`，玩法模块契约不变时记录无需修改。
- 设计意图：逻辑投射物继续拥有位置、生命周期和最终元素；主模块 VFX Catalog 只把元素语义映射到美术资源，ProjectileActor 消费该只读映射，不建立第二份元素状态。
- 权威状态与依赖：`FReEchoWeaponAttackCommit` / `ResolveProjectileElement` 仍是正式元素权威；Development `GMElement` 只把已存在的调试覆盖提前到投射物初始化，正式构筑路径不变。Niagara 缺失只影响表现。
- 决策记录：不把四个元素槽写入 Gun Profile，避免与 Plan137 并行修改同一不可合并 `.uasset`；复用 `FReEchoCombatVfxCatalog` 的稳定语义路径和旋转策略。代价是本轮四个资源路径由 Catalog 维护，若将来需要逐资源可编辑 Transform，再单独迁移为 DA 配置。
- 相关文档同步范围：更新 `MOD-ReEcho.md` 与 `MOD-ReEchoVFX.md`；审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEchoPresentation.md`、`MOD-ReEchoWeapons.md`，拓扑、路由或 Runtime Module 依赖未变化时记录无需修改。
- 关闭前逐项填写审阅结果：待实现与 PIE 验收后补充。

## 锁定验收

- [ ] Gun 的火、雷、草、水逐弹元素分别解析到四个新 Niagara；元素映射和资源路径有自动化覆盖。
- [ ] 棱镜多弹丸继续按 `Attack.Sequence + ProjectileIndex` 得到确定性元素，并使每颗弹的表现与自身 `HitIntent.Element` 一致。
- [ ] 无元素、非 Gun、资产缺失均安全回退且不改变逻辑载体；Bow Travel/Impact 不回归。
- [ ] 元素 Niagara 成功生成时隐藏字母式 `ElementLabel`；回退时仍保留可诊断表现。
- [ ] Development `GMElement` 在生成阶段与命中阶段使用同一元素，正式构筑路径不变。
- [ ] FullRebuild、聚焦自动化、项目校验、LFS、预构建检查和 `git diff --check` 通过。
- [ ] 用户在 PIE 验收四元素方向、长度、尺寸、遮挡、连续射击与棱镜逐弹变化；未提交精选预构建允许列表之外的生成物。

## Step 0 门禁

- 基线分支/提交：`origin/main@2b1fc6c5d45bd4d6625824c4fef6959571f3a04b`。
- 引擎/构建可用性：UE 5.8 Windows；Git LFS checkout 已通过；四个外部 `.uasset` 已按 SHA-256 原样复制且尚未打开/重存；Editor/命令前取得 Git-common-dir Unreal 锁。
- 现有聚焦测试结果：静态检查确认 Gun 当前固定解析 `PlayerGunFlight`；WeaponActor 已把逐弹 `ProjectileElement` 传给 ProjectileActor；`GMElement` 当前仅在命中阶段覆盖，表现可能滞后。
- 共享契约 / 难合并资源风险：Plan137 尚未实现且将修改 Gun Profile；本 Plan 不写该 DA。若 Plan137 先进入 main，只对共享 C++/测试做文本语义合并并重跑证据。
- 基线损坏时的停止条件：四个 Niagara 无法由 UE 5.8 加载、作者轴无法用统一/语义旋转表达、Plan137 改写 Gun Travel 所有权、或必须更改玩法元素/随机规则。
- 人工门禁例外：当前程序用户已明确确认本任务先本地执行，例外跳过 Plan139 实现前的 Plan-only `origin/main` 发布；发布前发现远端已占用 Plan138，本任务已在合并前机械重编号为 Plan139。

## 实现提纲

1. 增加四种 Gun 元素飞行语义、稳定资源路径、旋转策略和预加载覆盖。
2. ProjectileActor 按 `WeaponVisualKey + ProjectileElement` 解析元素 Travel；只在元素资源成功生成后隐藏文字回退。
3. 将 Development `GMElement` 覆盖提前合入每颗 Gun 投射物初始化元素，同时保留命中阶段防线。
4. 增加映射、回退、随机逐弹一致性和非 Gun 负例自动化，维护模块文档与执行记录。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| LFS / 资产 | `python scripts/setup_lfs.py --check`、`git lfs status`、`git lfs fsck`、运行时预加载测试 | 四个 Niagara 为完整可加载对象并进入预加载集合 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目、源码、文档和资源路径不变量通过 |
| C++ | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新精选预构建包 |
| 聚焦自动化 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Presentation.VFX.Catalog`、`scripts\ue\Run-Automation.cmd -Filter ReEcho.Weapons` | 元素路径/回退、逐弹元素和既有 Bow/Gun 载体通过 |
| 人工 PIE | 火/雷/草/水核心、棱镜、无核心、左右方向连续射击 | 飞行资源、方向、尺寸和命中元素一致，缺失时回退 |

## 执行记录

### 变化

- Combat VFX Catalog 新增火、雷、草、水四个 Gun Flight 语义，并把四个原始 Niagara 纳入默认预加载。
- `AReEchoProjectileActor` 按该颗逻辑投射物的最终元素选择专用 Gun Travel；专用资源成功生成时隐藏字母标签，失败时回退默认 Travel 并保留标签。
- `AReEchoWeaponActor` 在逐弹初始化前应用 Development `GMElement`，保证棱镜多弹与固定核心的飞行表现和后续命中元素一致；Shipping 路径忽略调试覆盖。
- 四个元素 Gun Flight 的全部启用 Emitter 统一改为 Local Space，使粒子继承权威 ProjectileActor 的位移和发射方向，不再停留在出生世界位置。
- 增加路径唯一性、资源加载/Local Space、预加载和逐弹调试覆盖自动化；未修改生产 XLSX/CSV、伤害公式、反应规则或 Gun Profile。

### 证据

- `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`：UE 5.8 Win64 Development UHT/UBT 成功，101/101 actions，刷新 7 模块预构建包，source fingerprint `c918405feb5a`。
- `python scripts/ue/prebuilt_editor.py check`：通过，build id `55116800`、source fingerprint `c918405feb5a`。
- `python scripts/validate_project.py`：通过，包含生产 XLSX→CSV 字节同步、Schema/ID/引用/公式白名单与项目规则校验；首次沙箱运行仅因 `C:\tmp` 下临时目录权限失败，授权后同候选重跑成功。
- `python scripts/setup_lfs.py --check`、`git lfs status`、`git lfs fsck`：通过；四个未跟踪 Niagara 保持原始文件，未由 Editor 重存。
- `git diff --check`：通过。
- `scripts\ue\Run-Automation.cmd -Filter ReEcho.Presentation.VFX.Catalog`：修复前准确失败并列出四个资源的 `Fountain002/004/005/006` 均非 Local Space；资产修复后发现 1 项并以 `Result={Success}`、`TEST COMPLETE. EXIT CODE: 0` 结束，证明四资源路径、可加载性及全部启用武器 Emitter 的 Local Space 契约通过。
- 获得 `main-publish-lock` 后合入 `origin/main@1511d04e`，保留 Plan133 CG 与远端 Plan138 开局点击选择；最终组合 `Development -FullRebuild` 95/95 成功，刷新 7 模块预构建包，source fingerprint `4c7d4155ac47`。
- 最终组合专项自动化均发现 1 项并以 `Result={Success}`、`TEST COMPLETE. EXIT CODE: 0` 结束：`ReEcho.Presentation.VFX.Catalog`、`ReEcho.Presentation.RuntimeAssetPreload.Catalog`、`ReEcho.Weapons.Gems.PrismMultishotUsesPerProjectileElements`。

### 剩余风险

- 四个交付 Niagara 已修正为随 ProjectileActor 移动，但 authored forward axis、屏幕尺寸和遮挡仍需 PIE 人工验收；资源加载与 Local Space 自动化不能替代真实渲染确认。
- Plan137 与本 Plan 共享部分 C++/测试文本；最终集成前必须基于最新 `origin/main` 做语义合并并重跑全部证据，不能覆盖 Plan137 的 Gun Profile 二进制修改。

### 人工验收结果/请求

`PendingBeforeClose`：请求用户在 PIE 使用 Gun 分别装备火、雷、草、水核心，并测试棱镜多弹、无元素回退、左右方向连续射击；确认方向、长度、尺寸、遮挡和逐弹表现与实际元素一致。

### 架构文档审阅结果

- 已更新 `MOD-ReEcho.md` 与 `MOD-ReEchoVFX.md`，记录逐弹元素权威、Catalog 映射、默认回退及 GM 调试边界。
- 已审阅 `ARCHITECTURE.md`、根 `README.md`、`MOD-ReEchoPresentation.md` 与 `MOD-ReEchoWeapons.md`：模块拓扑、运行时依赖和玩法元素契约未变化，无需修改。
