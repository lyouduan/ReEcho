# Plan 81 - 程序 - 角色相对武器装配尺寸

## 协调

- Planner 负责人：Codex（程序 Planner）。
- Executor 负责人：Codex（程序 Executor）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@df356c10`。
- 本地实现方式（可选，仅作交接说明）：按当前对话确认，直接在本地 `main` 工作区实施；保护现有未提交美术、VFX、UI 和动画资源。
- 依赖 / 阻塞：Plan76 已建立六类武器 Presentation Profile；实施前必须确认本 Plan 已发布到 `origin/main`。
- Writes: `plans/81-character-relative-held-weapon-layout.md`；`Source/ReEcho/Public/Presentation/Weapon/ReEchoWeaponPresentationProfile.h`；`Source/ReEcho/Public/Weapons/ReEchoWeaponActor.h`；`Source/ReEcho/Private/Weapons/ReEchoWeaponActor.cpp`；`Source/ReEcho/Public/Player/ReEchoPlayerPawn.h`；`Source/ReEcho/Private/Player/ReEchoPlayerPawn.cpp`；`Source/ReEcho/Public/Graybox/ReEchoEchoActor.h`；`Source/ReEcho/Private/Graybox/ReEchoEchoActor.cpp`；`Source/ReEchoPresentation/Public/Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h`；聚焦自动化测试；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`。如需首版数值调校，额外包含 `Content/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_*.uasset` 与 `Content/ReEcho/DataAsset/Character/Profiles/DA_{Character,Echo}_J_*.uasset`，且只能通过 Unreal Editor 保存。
- Stable Reads: `Source/ReEchoPresentation/Private/Presentation/Animation2D/ReEcho2DAnimationComponent.cpp`；`Source/ReEcho/Public/Presentation/Weapon/ReEchoWeaponPresentationCatalog.h`；`Source/ReEcho/Private/Weapons/ReEchoWeaponVisualCatalog.cpp`；`Content/ReEcho/DataAsset/Weapon/Catalogs/DA_WeaponPresentationCatalog.uasset`；Plan76。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：已有 DA 未填写新增字段时使用稳定默认值；Player 与 Echo 共用同一布局入口；表现缺失不影响攻击；不要求每帧刷新。
- 明确排除：攻击范围、碰撞、伤害、攻速、投射物逻辑、Niagara 飞行/朝向、角色动画选择、敌人武器能力和角色受击形变。

## 锁定目标

取消手持武器固定 `250 UU` 的尺寸假设，使玩家和回响的武器根据角色 Presentation Profile 的稳定标准高度等比装配。角色切换、回响外观差异或 `CharacterScale` 调整后，武器与角色保持一致的视觉比例；不同武器仍可在 Weapon Presentation Profile 中独立配置长度主轴、相对长度、握持偏移和静态旋转修正。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoPresentation`、`MOD-ReEchoWeapons`、`AREA-Presentation`、`AREA-Weapons`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoPresentation.md`、`MOD-ReEchoWeapons.md`，均已加入 `Writes`。
- 设计意图：角色 Profile 持有“手在哪里”的稳定归一化挂点；武器 Profile 持有“武器多大、沿哪个资源轴度量、如何握持”的表现数据；主模块 WeaponActor 只在装配事件发生时把两者换算为世界表现 Transform。
- 权威状态与依赖：新增只读表现契约，不改变玩法状态所有者和模块依赖方向。`WorldHeight` 是角色稳定参考高度，不读取当前 Flipbook 帧 Bounds 作为实时尺寸真相；纹理只提供宽高比。
- 决策记录：拒绝按 Tick 或按当前动画帧 Bounds 重算，避免动画切换造成武器比例抖动；拒绝角色×武器组合表，避免配置矩阵；首版复用 WeaponActor 根作为稳定武器表现根，只有证据表明需跟随角色内部表现节点时才扩展专用挂点组件。
- 相关文档同步范围：审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `README.md`；维护上述三个 `MOD-*`。若拓扑和索引未变化，仅在执行记录注明无需修改。
- 关闭前逐项填写审阅结果：在执行记录中补齐。

## 锁定验收

- [ ] 构造函数中不再存在手持武器固定 `250 UU` 尺寸分支，六类武器使用统一相对布局入口。
- [ ] Player 与 Echo 在初始化、武器选择/恢复和 Presentation Profile 刷新后得到一致布局，不依赖 Tick 重算。
- [ ] 角色 Profile 可配置归一化武器挂点；武器 Profile 可配置尺寸主轴、相对长度、相对偏移、旋转和可选绝对长度覆盖。
- [ ] 切换 Idle/Move/Attack 不因单帧 Bounds 差异改变武器基础尺寸；角色基础比例变化会更新武器尺寸。
- [ ] 武器表现调整不改变攻击范围、碰撞、伤害、投射物或 VFX 逻辑。
- [ ] 聚焦自动化、完整 Editor 构建、项目校验和 `git diff --check` 通过。
- [ ] 用户在 PIE 检查四角色与 Player/Echo 的代表武器比例、挂点和朝向；未验收前保持 `PendingBeforeClose`。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`main@df356c10`，由 `git fetch origin main` 后确认。
- 引擎/构建可用性：实现候选需关闭 Editor 后执行 UE 5.8 `Build-Editor.cmd -Configuration Development -FullRebuild`；Plan-only 发布按纯文档例外执行静态验证。
- 现有聚焦测试结果：Plan-only 阶段不复用旧运行时证据；实现后新增/更新角色相对武器布局测试。
- 共享契约 / 难合并资源风险：工作区存在大量用户资源；`DA_WeaponPresentation_Gun.uasset` 已修改，默认不覆盖，确需保存时通过 Editor 合并并单独审计二进制路径。
- 基线损坏时的停止条件：UHT/UBT 基线错误、现有 Profile 无法加载、或布局必须依赖玩法数据时停止并报告，不以硬编码恢复旧表现。

## 实现提纲

1. 为角色 Profile 增加稳定的归一化武器挂点配置，为武器 Profile 增加主轴、长度比例、偏移、旋转及稀有绝对覆盖字段。
2. 在 WeaponActor 建立唯一布局函数：以 `Profile.WorldHeight` 和角色基础比例计算目标武器长度，按纹理宽高比设置各可见组件，并更新稳定挂点 Transform。
3. Player/Echo 在武器初始化、选择/恢复及 Profile 刷新事件中调用同一入口；不增加 Tick 布局更新。
4. 增加纯计算/宿主接缝测试，验证主轴换算、比例缩放、默认兼容和 Player/Echo 调用路径。
5. 仅在默认值不足以形成可验收首版时，通过 Unreal Editor 调校指定 DA；保护已有 Gun 和其他本地资源修改。
6. 维护相关模块文档和执行记录，完成构建与静态门禁后交由用户 PIE 验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 格式/静态 | 对改动 `.h/.cpp` 运行 `.clang-format`；`python scripts/validate_project.py`；`git diff --check` | 风格、项目规则及空白检查通过 |
| C++ 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 退出码为 0，精选预构建包匹配候选 |
| 聚焦自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation` 与武器表现聚焦用例 | 相对布局计算和宿主接缝通过 |
| 资产 | Editor/Asset Registry 加载涉及的 Character/Weapon Profile | 新字段可读取，原软引用有效 |
| 人工 PIE | 四角色抽查六武器，覆盖 Player/Echo、非 1.0 `CharacterScale`、Idle/Move/Attack | 比例稳定、挂点可接受、朝向无回归 |

## 执行记录

### 变化

待执行。

### 证据

Plan-only 候选待执行静态验证。

### 剩余风险

不同武器源贴图透明边界和美术留白不同；统一几何比例后仍可能需要按武器 DA 做少量视觉调校。

### 人工验收结果/请求

`PendingBeforeClose`：实现后请求用户执行具名 PIE 检查。

### 架构文档审阅结果

待实现完成后逐项记录。
