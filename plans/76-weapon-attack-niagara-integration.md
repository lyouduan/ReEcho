# Plan 76 - 程序 - 武器攻击 Niagara 接入与长剑旧表现清理

## 协调

- Planner 负责人：当前对话程序 Planner。
- Executor 负责人：当前对话程序 Executor。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@085a3ca9d029b418597ff28e94d14d08d53fd12e`。
- 本地实现方式：用户已确认不采用一任务一 worktree，直接在当前本地 `main` 工作区执行；现有大量本地美术/VFX 修改均受保护，只显式处理本 Plan 路径。
- 依赖 / 阻塞：使用现有 `/Game/VFX/People/{Sword,Sickle,Bow,Bullet}` Niagara；正式接线前确认准确根资产可加载。远端 Plan75 后续也会修改 `AReEchoWeaponActor`，本 Plan 先完成现有攻击表现接线，Plan75 实现集成时必须保留本 Plan 的 VFX 触发边界。若资源自身无法运行或缺少必要依赖，停止对应武器接入并报告，不恢复旧缺失贴图。
- Writes:
  - `plans/76-weapon-attack-niagara-integration.md`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxCatalog.*`
  - `Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxComponent.*`
  - `Source/ReEcho/{Public,Private}/Weapons/ReEchoWeaponActor.*`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoProjectileActor.*`
  - `Source/ReEcho/Private/Presentation/Loading/ReEchoRuntimeAssetPreloader.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoRuntimeAssetPreloadTests.cpp`
  - `Content/VFX/People/{Sword,Sickle,Bow,Bullet}/**` 中四种攻击正式 Niagara 根及其准确递归依赖
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`
  - `Binaries/Win64/` 下 `GIT_RULES.md` 允许的最终预构建包文件
- Stable Reads:
  - `Content/Data/weapons.csv`
  - `Source/ReEchoWeapons/{Public,Private}/Weapons/**`
  - `Source/ReEcho/{Public,Private}/Graybox/ReEchoSwordArcActor.*`
- 影响模式：`SharedContract`（修改主模块武器世界表现与 VFX Catalog；不改变 Weapons/Combat 公共玩法契约）。
- 兼容承诺 / 下游操作：攻击节拍、方向、伤害、碰撞、穿透、爆炸和元素附着保持不变；逻辑 Actor 继续拥有飞行与命中真相，Niagara 缺失只能缺表现。
- 明确排除：不处理鞭和法杖；不修改武器数值、数据表、元素反应、手持武器资源或 Niagara 内部美术参数；不提交未被四种正式根引用的公共纹理库和实验资产。

## 锁定目标

1. 长剑每次攻击只播放 `NS_People_Sword_Attack_01`，不再同时生成旧 `SlashCrescent` 平面刀光。
2. 镰刀攻击播放 `NS_People_Sickle_Attack_01`，保持现有近战查询、方向和伤害时序。
3. 弓箭逻辑投射物附着 Bow 飞行 Niagara，并在命中或爆炸位置播放 Bow 命中特效；表现不拥有第二份飞行模拟。
4. 枪械逻辑投射物附着 Bullet 飞行 Niagara，并在命中位置播放 Bullet spark；表现不改变命中结算。
5. 四种正式 Niagara 进入首场异步预加载和 Shipping 资产引用闭包，缺失时输出一次性明确告警而不回退到旧攻击贴图。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho / AREA-Presentation`、文档型 `MOD-ReEchoVFX`、`MOD-ReEchoWeapons / AREA-Weapons` 的主模块表现适配。
- 对应模块文档：维护 `MOD-ReEcho.md`、`MOD-ReEchoVFX.md`、`MOD-ReEchoWeapons.md`，均已加入 `Writes`。
- 设计意图：用稳定 AttackPattern/WeaponVisualKey 选择 Niagara；玩法 Commit 和逻辑载体保持权威，表现层仅创建、附着和销毁 Niagara。
- 权威状态与依赖：不新增玩法状态或模块依赖；主模块继续依赖 `ReEchoWeapons`，Niagara 不进入资源无关的 Weapons 公共类型。
- 决策记录：近战瞬时 Niagara 由 AttackCommitted 事件统一生成；投射物飞行 Niagara 绑定逻辑 Actor，命中特效由逻辑 Actor 的既有终止点触发；长剑禁止继续调用旧 SwordArc，而镰刀也不再回退长剑贴图。
- 相关文档同步范围：更新上述三个模块文档；关闭前审阅 `ARCHITECTURE.md` 与 `CODEBASE_MAP/README.md`，预计依赖拓扑和 AREA 路由不变。
- 关闭前逐项填写审阅结果：在执行记录中记录每份文档“已更新”或“已审阅、无需修改”及原因。

## 锁定验收

- [x] 长剑单次攻击只创建一个 Niagara 斩击且旧 `SwordArcActor` 不再用于长剑攻击表现。
- [x] 镰刀攻击 Niagara 的方向、位置和触发次数与权威 Commit 一致。
- [x] 弓、枪飞行 Niagara 跟随逻辑投射物，命中特效在权威命中位置只播放一次。
- [ ] 四种武器的攻击节拍、伤害、碰撞、穿透、爆炸和元素行为回归不变。
- [ ] 四种 Niagara 根可加载并进入预加载清单；正式依赖闭包可从 Git/Shipping 获得。
- [ ] 修改 C++ 完成格式化，并通过 Development `-FullRebuild`、聚焦自动化、`validate_project.py` 和 `git diff --check`。
- [ ] 用户在 PIE 验收四种武器的尺寸、朝向、层级、移动跟随和命中可读性。
- [ ] 未提交精选 `GIT_RULES.md` 允许列表之外的 UE 生成产物或本 Plan 外本地脏内容。

## Step 0 门禁

- 基线分支/提交：`origin/main@085a3ca9d029b418597ff28e94d14d08d53fd12e`；已审计并接收远端 Plan75 设计与实现，当前任务顺延为 Plan76；Plan75 新增符文接口必须完整保留。
- 引擎/构建可用性：UE 5.8 安装版；执行独占 Editor/构建命令前检查进程和 Git common-dir 锁。
- 现有聚焦测试结果：当前 VFX Catalog 仅锁定长剑 Niagara；镰刀/弓/枪尚无正式映射测试，本 Plan 不复用旧证据。
- 共享契约 / 难合并资源风险：本地 `Content/VFX/People/**` 含大量未提交二进制依赖，必须以准确根资产的递归引用闭包显式选择，禁止目录级暂存。
- 基线损坏时的停止条件：远端新增提交、目标 Niagara 无法加载、依赖闭包与无关素材无法隔离、或本地同路径资产存在不明覆盖时停止并报告。

## 实现提纲

1. 审计四个 Niagara 根的可加载性、依赖、循环/自动销毁、空间模式和可用参数。
2. 扩展武器 VFX 语义映射，按 AttackPattern 触发长剑与镰刀瞬时攻击 Niagara。
3. 给弓、枪逻辑投射物附着飞行 Niagara，并在既有命中/终止路径播放命中特效。
4. 移除长剑旧贴图刀光触发，禁止镰刀回退旧长剑贴图；更新预加载与聚焦测试。
5. 维护模块文档和执行记录，完成构建、自动化、静态检查与 PIE 验收交接。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 资产审计 | Editor/AssetRegistry 加载四种 Niagara 根及准确依赖 | 根资产可加载、用途和生命周期明确 |
| C++ 格式 | 仓库 `.clang-format` 处理修改的 `.h/.cpp` | diff 仅含预期语义与格式 |
| VFX 聚焦 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.VFX` | 四种语义映射、路径和一次性触发契约通过 |
| 武器回归 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Weapons` | 攻击节拍、载体和命中语义不变 |
| 预加载 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.RuntimeAssetPreload` | 四种正式根进入规范化去重清单 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 退出码 0并刷新精选预构建包 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目与文本不变量通过 |
| 人工 | PIE 逐一测试长剑、镰刀、弓、枪 | 用户确认尺寸、方向、层级、跟随与命中可读性 |

## 执行记录

### 变化

- Combat VFX Catalog 新增长剑、镰刀、弓飞行/命中、枪飞行/命中六个明确语义；长剑与镰刀按稳定 AttackPattern 在 AttackCommitted 后分别播放专用 Niagara。
- `AReEchoWeaponActor` 只为尚未迁移的 Whip 保留 `SwordArcActor`；长剑与镰刀不再创建旧平面刀光，Plan75 新增符文接口与状态完整保留。
- `AReEchoProjectileActor` 为 Bow/Gun 隐藏旧贴图/程序形状，将飞行 Niagara 绑定权威逻辑 Actor，并在首次 `OnProjectileImpacted` 回调播放一次命中特效；爆炸多目标回调不会重复生成表现。
- 首轮 PIE 发现长剑/镰刀层级和方向错误、弓/枪缺少可见飞行过程；四个方向/移动 System 的全部内嵌发射器已通过 Unreal Editor 版本化 Emitter 数据接口改为 Local Space，武器战斗特效前景排序下限提高到 `1000`。
- 弓箭后续 PIE 反馈确认只应修改完整特效的整体旋转，不应改变资源内部表现；已撤销三个 Sprite Renderer 的 `VelocityAligned` 改造并恢复原 `Automatic`，Catalog 使用 System 实际 authored `+X` 整体轴。飞行 Niagara 生成后只对完整 Component 设置一次绝对世界旋转，使其对齐 Commit 已锁定的“角色射向目标”方向；直线弹道不做 Tick 更新。
- Weapon Visual Catalog 不再枚举或同步读取长剑、镰刀、弓、枪旧攻击贴图；GameInstance 预加载通过 Combat VFX Catalog 纳入六个正式语义。

### 证据

- 修改后 Development 增量构建通过（9 actions），UHT/UBT 退出成功并刷新预构建包。
- 格式化后的最终候选 Development `-FullRebuild` 通过（99 actions），7 个模块精选预构建包刷新成功。
- 恢复弓箭原始 Renderer 并改为完整 Component 单次世界旋转后的最终 Development `-FullRebuild` 再次通过（99 actions）；`ReEcho.Presentation.VFX.Catalog` 返回 `Result={Success}`，同时验证 authored `+X` 整体轴旋转后等于锁定目标方向、三个 Sprite Renderer 均保持原 `Automatic`；`prebuilt_editor.py check` 返回 7 个模块、`build_id=55116800`、`source=f6e6728f66d2`，`validate_project.py` 与 `git diff --check` 均通过。
- 静态二进制引用闭包审计从六个 Niagara 根解析到 55 个现存资产；本地待发布依赖集中在 People/Bow、People/Bullet、People/Sickle、必要 Sword/Rabbit MI 与五张公共纹理，未把整套公共素材库自动纳入。
- 仓库 `Run-Automation.cmd` 仍被本机 LinuxArm64/VisionOS `MainVersion` SDK 校验阻断；改用 Win64 Editor 命令行与内存 DDC 后，`ReEcho.Presentation.VFX.Catalog` 已完成并返回 `Result={Success}`，同时锁定长剑/镰刀/弓飞行/枪飞行全部启用发射器为 Local Space。

### 剩余风险

- 客观门禁已完成；仍需用户在 PIE 对四种武器做最终视觉验收。
- Niagara 自身尺寸、朝向轴、Local/World Space、透明层级和飞行跟随必须由用户在 PIE 主观验收；弓的 Boom 对爆炸多目标只在第一个权威命中回调播放一次，避免视觉重复。
- 鞭和法杖不在本 Plan 范围，继续保留旧攻击表现。

### 人工验收结果/请求

`AcceptedWithKnownLimitations`：用户确认本轮暂时按当前表现发布；弓箭只保留完整 Component 单次旋转方案，最终方向观感尚未作为完全通过项关闭，后续如继续调整应保持 Niagara 内部原表现不变。

### 架构文档审阅结果

- `MOD-ReEcho.md`：已更新四武器 Niagara 路由及玩法/表现边界。
- `MOD-ReEchoVFX.md`：已更新六个武器语义、事件来源、生命周期与代码落点。
- `MOD-ReEchoWeapons.md`：已更新主模块 Niagara 适配；资源无关 Weapons 逻辑契约未变化。
- `ARCHITECTURE.md`、`CODEBASE_MAP/README.md`：已审阅，无模块依赖拓扑或 AREA 路由变化。
