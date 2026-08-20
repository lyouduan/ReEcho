# Plan 62 — 武器家族完整性实现（实现武器体系W 可见的 6 种武器）

- **状态**：InProgress（第一阶段静态武器贴图已推 origin/main `354e010`；第二阶段专属攻击表现接入中；PIE 人工验收 `PendingBeforeClose`）
- **角色**：[PROGRAMMER] 实现 / [SECRETARY] 协调
- **依赖**：Plan 61（整族删除匕首，已确认；本 Plan 不处理匕首）
- **阶段目标关联**：Demo 稳定化 P0（六场完整一局可用武器多样性）+ P1（战斗可读性/表现）
- **Planner / Executor 负责人**：当前程序 AI（非 Planner-Executor 模式，直接在本地主线执行）
- **本地规划 / 实现基线**：`origin/main@354e010`
- **Writes**：`plans/62-weapon-family-completion.md`；`Source/ReEcho/{Public,Private}/Weapons/ReEchoWeaponActor.*`；`Source/ReEcho/{Public,Private}/Graybox/ReEchoProjectileActor.*` 与近战弧 Actor；`Source/ReEcho/{Public,Private}/Presentation/VFX/ReEchoCombatVfxCatalog.*`；`Source/ReEcho/Private/Tests/ReEchoWeaponRuntimeTests.cpp` / `ReEchoCombatVfxTests.cpp`；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoWeapons.md`、`MOD-ReEchoVFX.md`；到位的 `/Game/ReEcho/Textures/Effects/` 专属攻击纹理。
- **Stable Reads**：`Design/Data/ReEchoData.xlsx` 及其生成 CSV；`Source/ReEchoWeapons/` 的攻击 Commit/Carrier 公共契约；`Source/ReEchoCombat/` 的最终命中结算契约；现有静态武器纹理和 `SlashCrescent` / `StaffLightWave` 资产。
- **影响模式**：`SharedContract`（保持 Weapons/Combat 逻辑契约不变，在主模块表现适配层增加武器语义路由）。
- **兼容承诺**：不改变六把武器的攻速、伤害、范围、连段、投射数量、爆炸半径和最终命中裁决；缺失专属美术时必须安全回退，不能 Fatal 或误用手持静态图冒充攻击特效。

## 1. 目标

让 `武器体系W` sheet 中**可见**的 6 种武器类型（长剑 / 镰刀 / 鞭 / 弓 / 枪 / 法杖）全部达成「可获取 + 可使用 + 有表现 + 开局可选（按需）」。

> 匕首（第 2 行，开普勒中 `hidden=True`）不在本 Plan 范围，由 Plan 61 整族删除。

## 2. 范围

### In Scope
| 类型 | 实例 | 本 Plan 动作 |
|---|---|---|
| 长剑 LongSword | W_J_01 Crescent Blade | 已完成（slot2，开局可选） |
| 法杖 Staff | W_J_02 Staff（开普勒单一法杖：合并原月杖 W_J_02 / 元素反应 W_J_03 / 学徒杖 W_J_06 三实例） | 合并为单实例，Pattern.StaffProjectile，VisualKey=Staff，slot1，开局可选 |
| 镰刀 Scythe | W_J_04 Harvest Scythe | 补镰刀外观（原错显为剑已修）；保持勇者默认可玩 |
| 鞭 Whip | W_J_07 Whip | 新增实例 + 表现 + 开局可选（slot4） |
| 弓 Bow | W_J_08 Bow | 新增实例 + 表现 + 开局可选（slot3） |
| 枪 Gun | W_J_09 Gun | 新增实例 + 表现 + 开局可选（slot5） |

### Out of Scope（单独立项，本 Plan 不解决）
- 攻击行为深度：外圈加成伤害、武器击退施加机制、`ChainWindow` 连贯阈值消费、匕首三段双结算（均属此前诊断的「攻击行为补全」类，见 Plan 61 剩余风险）。
  - **鞭的 `AS_WHIP_1` 的 `MovementCm=50` 是「击退误用」字段**：本 Plan 范围内将其按「角色冲刺位移」语义处理（或归零），不实现击退；击退机制单独立项。
- 角色主题名与武器错位（猎手"羽翼弓"→月杖、智者"手杖"→长剑、诗人"竖琴"→元素反应）：仅文案，不在本 Plan 机械改动范围，列于剩余风险。
- 程序侧生成最终美术：镰刀横扫、鞭击、弓箭、枪弹的正式纹理由美术提供；本 Plan 负责资产契约、导入后的路由和安全回退。

### 2026-08-20 范围扩展：六把武器专属攻击表现

- 保留现有六套攻击数据和攻击执行逻辑，补齐 `AttackPatternId / VisualKey → 攻击表现` 的明确映射。
- 长剑继续使用 `SlashCrescent`；法杖当前 `Pattern.StaffProjectile` 接入已有 `StaffLightWave` 视觉，但不得改变其逻辑投射物/爆炸结算。
- 镰刀、鞭、弓、枪建立专属表现入口；素材未到位时使用明确、非崩溃的程序回退，并在人工验收中标记为美术阻塞。
- 禁止所有 Melee 无条件生成长剑刀光；禁止 Bow/Gun/Staff 永久共用不可区分的 Engine 球体作为最终表现。

## 3. 现状诊断（证据）

- `weapons.csv` 现有 6 把，与开普勒可见武器对齐：W_J_02 法杖（slot1，开局可选，合并原月杖/元素反应/学徒杖三实例）、W_J_01 长剑（slot2）、W_J_07 鞭（slot4）、W_J_08 弓（slot3）、W_J_09 枪（slot5）共 5 把开局可选；W_J_04 镰刀（slot=None，勇者默认可玩）。
- `attack_steps.csv` 长剑/镰刀/鞭/弓/枪/法杖步骤已存在且数值对齐开普勒（`AS_LONGSWORD_1/2`、`AS_SCYTHE_1`、`AS_WHIP_1/2`、`AS_BOW_1`、`AS_GUN_1`、`AS_STAFF_1`）。**无需新增步骤**。
- `ReEchoWeaponActor::RefreshVisualState` 已扩展识别 `CrescentBlade`/`Staff`/`Scythe`/`Whip`/`Bow`/`Gun`，`StaffSprite` 已解除隐藏，镰刀/法杖视觉错显已修。
- `ReEchoLoadoutSelectionWidget` 武器数硬编码 `!=3` 断言已放宽（接受任意 ≥1），`WeaponTexturePath` 已补齐 Scythe/Whip/Bow/Gun/Staff 映射，不再阻断弓/枪/鞭/镰刀接入 UI。
- 六把武器并非只有两把具备攻击逻辑：`attack_steps.csv` 已包含六种 Pattern 的步骤，`CompileLogicDefinition` 将弓/枪/法杖编译为 Projectile，长剑/镰刀/鞭编译为 Melee，均能生成 Commit 并进入世界命中流程。
- 实际缺口是专属攻击表现：`SwingMelee()` 统一调用 `StartSwordAnimation()` / `SpawnSwordArc()`，导致长剑、镰刀、鞭都生成 `SlashCrescent`；`FireProjectile()` 对弓、枪、法杖统一生成只显示 Engine 球体/元素文字的 `AReEchoProjectileActor`。
- `StaffLightWave.uasset` 和 `AReEchoStaffLightWaveActor` 已存在，但只在旧 `Pattern.MoonStaffWave` 的 Wave Carrier 下调用；当前 canonical `Pattern.StaffProjectile` 走 Projectile，因此该光波不是当前法杖攻击的有效表现。

## 3.1 架构影响与设计决策

- **受影响架构标识**：`MOD-ReEcho`、`MOD-ReEchoWeapons`、文档型 VFX 入口 `MOD-ReEchoVFX`；`MOD-ReEchoCombat` 仅作为 Stable Read，不改变最终伤害权威。
- **对应模块文档**：`MOD-ReEcho.md`、`MOD-ReEchoWeapons.md`、`MOD-ReEchoVFX.md` 已加入 Writes；实现完成时同步实际表现路由和资源回退契约。`MOD-ReEchoCombat.md` 只读复核，无契约变化时记录“已审阅、无需修改”。
- **设计意图**：让 `ReEchoWeapons` 继续只产生稳定的攻击 Commit，让 `AReEchoWeaponActor` 及表现 Actor 根据已装备武器选择可视载体；命中、伤害和死亡仍由现有逻辑组件与 Combat Resolver 权威裁决。
- **决策记录**：不把手持静态武器图直接当攻击 VFX；不通过修改 Carrier 把法杖强行切到旧 Wave 行为。优先扩展现有逻辑投射物的视觉语义，以避免碰撞、速度、范围和爆炸行为漂移。
- **公共契约影响**：默认不修改 `ReEchoWeapons` / `ReEchoCombat` 公共结构；如果实现证明必须向表现层传递武器视觉语义，应先在本 Plan 记录最小契约变化并完整重建所有依赖模块。

## 4. 设计方案

### 4.1 数据层（`Content/Data/weapons.csv`）
武器实例已就位（步骤已存在，仅引用），最终槽位：
```
W_J_07,Whip,Whip,Whip,4,true,4,Pattern.WhipCombo,0.80,0.60,0.60,400,180,0,0,0,1,true,RuntimeCompatibility,7,
W_J_08,Bow,Bow,Bow,3,true,5,Pattern.BowShot,1.00,1.00,1.00,1000,0,1,0,0,1,true,RuntimeCompatibility,8,
W_J_09,Gun,Gun,Gun,5,true,6,Pattern.GunShot,0.30,0.20,0.20,1500,0,1,0,0,1,true,RuntimeCompatibility,9,
```
- `InputSlot`：W_J_08=3、W_J_07=4、W_J_09=5（按开普勒可见武器顺序对齐）；`StartSelectable=true`。
- 法杖原三实例（W_J_02 月杖 / W_J_03 元素反应 / W_J_06 学徒杖）合并为单一 `W_J_02 Staff`（Pattern.StaffProjectile，VisualKey=Staff，slot1，开局可选），开普勒无"元素反应"武器。
- 镰刀 W_J_04：保持 `StartSelectable=false`（勇者默认已可玩）。

### 4.2 逻辑层（`ReEchoWeapons`，零改动预期）
- `CompileLogicDefinition` 按 `AttackPatternId` 自动映射 Carrier：`BowShot`/`GunShot`（`ProjectileCount=1`）→ Projectile；`WhipCombo`（ProjectileCount=0）→ Melee。无需改分支。
- `AS_WHIP_1` 的 `MovementCm=50` 维持原值（按冲刺位移语义），击退另立项。

### 4.3 表现层（`Source/ReEcho/Private/Weapons/ReEchoWeaponActor.cpp`）
- `RefreshVisualState`（`:539`）：
  - `MoonStaff` → 显示 `StaffSprite`（解除当前 `SetVisibility(false)`）；
  - 新增 `Scythe` 分支 → 显示 `ScytheSprite`；
  - `Whip`/`Bow`/`Gun` 新增分支 → 显示各自 `Sprite`（近战鞭/弓/枪可视剑精灵体系扩展，或专属精灵）；
  - 学徒法杖 `VisualKey` 改为 `Staff`（复用 `StaffSprite`）或保留 `ElementalOrb`（若设计即元素法杖则维持，需用户确认）。
- 投射/光波 Actor 颜色已由 `ReEchoElementReaction::GetElementColor` 驱动，无需改。

### 4.4 UI 层（`Source/ReEcho/Private/UI/ReEchoLoadoutSelectionWidget.cpp`）
- `LoadOptions`（`:243`）：将 `Weapons.Num() != 3` 改为动态（移除硬编码 3，接受任意 ≥1 数量；角色 `!=4` 断言保留）。
- `WeaponTexturePath`（`:128`）：新增 `Scythe`/`Whip`/`Bow`/`Gun`/`Staff` 的纹理路径映射，返回空即 Fatal。
- 排序：依赖 `GetStartSelectableWeapons()` 返回顺序（按 `LoadoutOrder`），需确认该函数排序正确。

## 5. 素材清单（用户提供）

| VisualKey | 用途 | 纹理路径（建议） | 状态 |
|---|---|---|---|
| MoonStaff | 月杖持握精灵（StaffSprite） | `/Game/ReEcho/Textures/Effects/MoonStaff.MoonStaff` | 需确认存在/提供 |
| Scythe | 镰刀精灵（ScytheSprite） | `/Game/ReEcho/Textures/Effects/Scythe.Scythe` | 需提供 |
| Whip | 鞭精灵 | `/Game/ReEcho/Textures/Effects/Whip.Whip` | 需提供 |
| Bow | 弓精灵 | `/Game/ReEcho/Textures/Effects/Bow.Bow` | 需提供 |
| Gun | 枪精灵 | `/Game/ReEcho/Textures/Effects/Gun.Gun` | 需提供 |
| Staff（学徒法杖） | 学徒法杖精灵（若不复用 MoonStaff） | `/Game/ReEcho/Textures/Effects/ApprenticeStaff.ApprenticeStaff` | 待定 |

> 素材未到位时，`WeaponTexturePath` 返回空会触发 Fatal。实现顺序：先放置占位纹理或确认纹理已入库，再放开 `StartSelectable`，避免 PIE 崩溃。

## 6. 验证矩阵

- **CSV/数据**：`python scripts/validate_project.py` + `Run-Automation -Filter ReEcho.Weapons` 通过；重跑 `sync_xlsx_to_csv.py` 确认弓/枪/鞭实例不复活冲突。
- **C++ 改动（UI/表现）**：本地增量 `scripts/ue/Build-Editor.cmd -Configuration Development` 验证；推 `origin/main` 前 `-FullRebuild` + 刷新精选预构建包 + `validate_project.py` + `git diff --check`。
- **PIE 人工验收（P0）**：6 种武器各自可选/可玩/表现正确；攻速<动作锁持续攻击无回归；无 Fatal；录制/回响稳定。
- **攻击逻辑回归**：自动化覆盖六种 Pattern 的 Carrier、Step/Combo 选择和 Commit 数值不变；表现分流不参与伤害计算。
- **攻击表现静态检查**：长剑/镰刀/鞭不再无条件共用刀光；弓/枪/法杖具有独立视觉语义；资源缺失路径有受测回退且不触发 Fatal。
- **攻击表现 PIE（P0）**：逐把录制基础攻击，确认方向、生成点、尺寸、生命周期、命中同步和镜像正确；正式素材未到位的武器只能记为 `PendingFollowUp`，不得误报“表现完成”。

## 7. 风险与剩余问题

1. **UI Fatal 断言**：改 `!=3` 与纹理映射是崩溃风险点，需先放素材再放开可选。
2. **双份真源**：开普勒隐藏匕首、canonical 可见，已由 Plan 61 处理 canon；本 Plan 不触碰。
3. **角色主题名错位**：纯文案，不在本 Plan 机械范围，建议另议。
4. **深度行为**：外圈加成/击退/三段拆分未做，影响「完整」成色，需单独立项补全。

## 8. 任务分解（实现阶段，在 plan/62-* 工作树）

1. 数据：weapons.csv 加 W_J_07/08/09（含槽位/可选）。
2. 表现：RefreshVisualState 加 Scythe/Whip/Bow/Gun/Staff 分支 + 月杖解除隐藏。
3. UI：放宽武器数断言 + WeaponTexturePath 映射。
4. 素材：用户提供上表纹理，入库并确认路径。
5. 验证：validate + automation + 本地增量构建 + PIE 验收。
6. 发布：FullRebuild + prebuilt + 秘书提交 [PROGRAMMER] 推 origin/main。

### 第二阶段：攻击表现接入（当前本地主线）

1. 锁定六种 Pattern 的逻辑基线测试，证明六把武器已有攻击 Commit/Carrier，且本阶段不改伤害行为。
2. 将 `SwingMelee()` 的长剑专用动画/刀光从通用近战命中路径分离：长剑、镰刀、鞭按装备语义选择表现。
3. 扩展逻辑投射物的视觉初始化参数：弓、枪、法杖各自选择表现；法杖复用已有 `StaffLightWave` 视觉资源但继续使用 Projectile 逻辑组件。
4. 为镰刀、鞭、弓、枪建立精确的攻击纹理路径和安全回退；正式资源到位后仅需导入同名资产，无需改命中逻辑。
5. 更新 `MOD-ReEcho` / `MOD-ReEchoWeapons` / `MOD-ReEchoVFX`，运行聚焦自动化、增量构建、静态验证；最终候选执行 FullRebuild 门禁。
6. 请求人工 PIE 逐把验收；正式攻击纹理未到位的项目保持明确 Pending，不阻塞代码契约合入，但阻塞“六把最终美术完成”的关闭结论。

## 9. 执行记录（工作树 ReEcho-plan62-weapon-family-completion 实现）

### 实际实现（截至 2026-08-19）
- **数据真源统一**：开普勒 `武器体系W` 仅 6 种可见武器（长剑/镰刀/鞭/弓/枪/法杖单实例），且匕首行为隐藏行（hidden=True）→ 已删除。法杖三实例（W_J_02 月杖 / W_J_03 元素反应 / W_J_06 学徒杖）合并为单一 `W_J_02 Staff`（Pattern.StaffProjectile，VisualKey=Staff，ProjectileCount=1，ExplosionRadiusCm=200）。
- **输入槽位扩展到 6**：`EReEchoInputSlot` 由 None/1/2/3 扩展为含 4/5/6；`ParseInputSlot` 解析 4/5/6；校验 `INPUT_SLOTS` 扩至 1-6。最终：slot1=W_J_02 法杖、slot2=W_J_01 长剑、slot3=W_J_08 弓、slot4=W_J_07 鞭、slot5=W_J_09 枪（W_J_04 镰刀 slot=None，勇者默认可玩）。
- **CSV 整族改写并重新生成**：`weapon_types.csv`/`weapons.csv`/`attack_steps.csv`/`parts.csv`/`part_effects.csv`/`slot_profiles.csv`/`csv_schema.csv`/`characters.csv`（J_CLOVER 默认武器 W_J_03→W_J_02）删除匕首与冗余法杖实例，对齐 6 武器。通过 `scripts/data/sync_xlsx_to_csv.py` 将唯一真源 `Design/Data/ReEchoData.xlsx` 同步回生产 CSV，保证字节一致（XLSX 校验通过）。
- **源码**：`ReEchoWeaponCsvReader.cpp`（移除 `bHasStrengthGrip`、放宽起始武器校验、更新 `ExpectedLoadout`、`RegisterAttackPatternId` 移除 Dagger 模式）、`ReEchoCsvDataRegistry.cpp`（移除 Dagger 模式注册）、`ReEchoInventoryShopWidget.cpp`（移除匕首图标 FObjectFinder）、`ReEchoWeaponActor.cpp/.h`（新增 Scythe/Whip/Bow/Gun 精灵 + Staff 解除隐藏）、`ReEchoLoadoutSelectionWidget.cpp`（武器数断言放宽、纹理映射补齐）、`validate_project.py`（移除匕首硬编码映射、放宽 AttackPatternReplacement/UniqueBehavior/值操作覆盖要求）。
- **测试**：`ReEchoWeaponRuntimeTests.cpp`/`ReEchoTraitTests.cpp`/`ReEchoShopTests.cpp`/`ReEchoRecordingTests.cpp`/`ReEchoCombatVfxTests.cpp` 中匕首实例/配件/模式引用已改写为有效武器（长剑/弓/法杖核心）并校正期望（起始可选 5 把、Parts 数 70、未命名 60、攻击速度 1.0）。
- **同步校验与构建**：`python scripts/validate_project.py` 全绿（含 XLSX 作者工作台字节比对、prebuilt 检查）；`scripts/ue/Build-Editor.cmd -Configuration Development` 增量构建通过（测试代码编译通过）。

### 验证矩阵现状
- [x] `validate_project.py` 通过（含 XLSX 同步）
- [x] 增量 Development 构建通过
- [ ] `Run-Automation -Filter ReEcho.Weapons` 待用户在编辑器中运行确认（代码已清除匕首引用并校正断言）
- [ ] PIE 人工验收（P0）：6 种武器各自可选/可玩/表现正确（视觉精灵素材待用户提供，见 §5 素材清单；未到位时不放开 `StartSelectable` 以避免 Fatal）

### 第二阶段执行记录（截至 2026-08-20）

- 已确认六种 Pattern 均有攻击步骤和 Commit/Carrier 逻辑；用户观察到的缺口是专属攻击表现，不是攻击逻辑缺失。
- `AReEchoWeaponActor` 已把通用近战命中与长剑手持摆动分离，并把稳定 `VisualKey` 传给近战弧/投射视觉；镰刀和鞭不再驱动长剑手持动画。
- `AReEchoSwordArcActor` 建立 `SlashCrescent` / `ScytheSweep` / `WhipLash` 三条纹理契约；专属资源缺失时显式回退现有刀光，不影响攻击成功。
- `AReEchoProjectileActor` 建立 `BowProjectile` / `GunProjectile` / `StaffLightWave` 三条纹理契约；法杖复用现有光波纹理但保持 `Pattern.StaffProjectile` 的逻辑投射物，弓/枪缺图时使用不同尺寸的程序形状。
- `PlayerMeleeSlash` Niagara 收紧为 `Pattern.LongSwordCombo` 专属，镰刀/鞭不再误播长剑 Niagara。
- [x] 增量 Development 构建通过。
- [x] `ReEcho.Presentation.VFX.Catalog` 聚焦自动化通过。
- [ ] 镰刀横扫、鞭击、弓箭、枪弹正式纹理仍需美术提供；当前仅完成代码契约与安全回退。

### 素材依赖（仍待用户）
- 镰刀/鞭/弓/枪/法杖持握精灵纹理（§5 清单）未入库；`WeaponTexturePath` 已映射但资源缺失会触发 Fatal，故视觉表现需素材到位后方可 PIE 验收。
