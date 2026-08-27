# Plan 126 - 程序 - 武器最终偏移特效挂点

## 协调

- Planner 负责人：Codex（程序路线）。
- Executor 负责人：Codex（程序路线）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Review`。
- 人工验收：`Accepted`。
- 本地规划 / 实现基线：`origin/main@46adef87a5149f00b8df585b4cdbc674c951ad0d`。
- 本地实现方式（可选，仅作交接说明）：`C:\tmp\ReEcho-plan126-weapon-vfx-anchor`，分支 `codex/plan126-weapon-vfx-anchor`。
- 依赖 / 阻塞：沿用 Plan100 已发布的共享左右手挂点、`HeldOffsetRatio`、长剑三挥、镰刀旋转和 Boss 武器特效根节点契约；实现前须确认本 Plan 已进入 `origin/main`。
- Writes: `plans/126-programmer-weapon-vfx-final-anchor.md`；`Source/ReEcho/Public/Presentation/Weapon/ReEchoWeaponPresentationProfile.h`；`Source/ReEcho/Public/Weapons/ReEchoWeaponActor.h`；`Source/ReEcho/Private/Weapons/ReEchoWeaponActor.cpp`；`Source/ReEcho/Public/Presentation/VFX/ReEchoCombatVfxComponent.h`；`Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxComponent.cpp`；Player/Echo 武器与 VFX 根节点接线的最小相关 `.h/.cpp`；`Source/ReEcho/Private/Tests/ReEchoCombatPresentationTests.cpp`；`Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`；FullRebuild 刷新的 `Binaries/Win64/ReEchoEditor.prebuilt.json` 及其准确允许列表产物；若安全资产自动化可用，四把生产武器的 `DA_WeaponPresentation_*.uasset`。
- Stable Reads: `Source/ReEchoCombat/Public/Combat/ReEchoCombatContracts.h`；四把生产武器现有 DA；Boss `BossWeaponVfxRoot` / `BossWeaponTipRoot` 接线；现有 Player/Echo `AttackVfxRoot`；武器贴图与 Plane/Billboard 渲染约定。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：不改变攻击节拍、伤害、范围、命中时机、投射物逻辑或 Boss 技能；旧 DA 缺少新字段时使用中心挂点安全默认值；命中特效继续使用命中世界位置，Travel 继续依附载体。
- 明确排除：不调整角色公共左右手挂点、不重做武器贴图/材质/Niagara、不改变武器玩法几何、不把 Boss Staff 的独立左右偏移并入玩家共享挂点。

## 锁定目标

四把生产武器的释放类攻击特效必须以武器已经应用共享手部挂点、朝向镜像、`HeldOffsetRatio`、最终尺寸和当前挥舞旋转后的最终位置为基础生成。朝右时：镰刀取武器中心、长剑取武器下方中心、弓和枪取武器右侧中点；朝左时按武器局部水平轴镜像为对应位置。Player 与 Echo 使用同一计算；Boss 继续使用其现有独立武器特效根节点。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` / `AREA-Weapons`、`MOD-ReEchoPresentation` / `AREA-Presentation`、`MOD-ReEchoWeapons`（公共行为说明受影响，资源无关逻辑不修改）。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoPresentation.md`、`MOD-ReEchoWeapons.md`，均已加入 `Writes`。
- 设计意图：建立“武器最终视觉 Transform 是释放类 VFX 原点唯一权威”的边界，消除角色 `AttackVfxRoot` 与武器实际偏移、缩放和动作之间的第二套位置计算。
- 权威状态与依赖：武器 Profile 新增归一化局部挂点数据；`AReEchoWeaponActor` 持有并更新最终 `WeaponAttackVfxRoot`；主模块 Player/Echo 仅把该根节点提供给 Combat VFX。`ReEchoCombat` 的攻击事件契约不新增资源或组件依赖，`ReEchoWeapons` 逻辑模块不依赖表现模块。
- 决策记录：采用与 Boss 武器特效根节点相同的显式组件根节点模式。挂点以武器贴图中心为 `(0,0)`、半宽/半高为 `0.5` 的局部比例描述；左向只镜像局部水平分量。拒绝从人物中心、Flipbook Bounds 或角色宽度重新推算，也不把武器组件指针塞入 Combat 公共事件。
- 相关文档同步范围：审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `README.md`；维护上述三个 `MOD-*`。若拓扑和稳定路由未变化，关闭时在本 Plan 记录无需修改原因。
- 关闭前逐项填写审阅结果：待实现后补充。

## 锁定验收

- [x] 自动化证明武器挂点先消费最终 `HeldOffsetRatio`，再按当前武器局部 Transform 求锚点；不得回退为角色中心或人物宽度计算。
- [x] 朝右挂点为：镰刀中心、长剑下方中心、弓/枪右侧中点；朝左局部水平镜像，垂直分量不变。
- [ ] 长剑三挥和镰刀旋转期间，特效根节点跟随当前武器组件 Transform；弓/枪翻转后根节点位于视觉枪口/出箭一侧。
- [x] Player 与 Echo 的释放类 `AttackCommitted` 特效消费武器根节点；Travel 与 DamageApplied 的原有载体/命中世界位置语义不变。当前生产 Profile 未配置玩家 Charge 槽，未来 AttachToAttackRoot 槽可复用同一根节点。
- [x] Boss 武器继续消费 `BossWeaponVfxRoot`，行为与 DA 左右偏移不回归。
- [x] C++ 构建、聚焦自动化、项目校验、预构建包检查和 `git diff --check` 通过。
- [ ] 用户在 PIE 中确认四把武器左右朝向的特效起点与武器贴图对齐。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@46adef87a5149f00b8df585b4cdbc674c951ad0d`。
- 引擎/构建可用性：沿用仓库 UE 5.8 Windows `.cmd` 入口；最终发布候选必须 `-FullRebuild`。
- 现有聚焦测试结果：实现前只做静态路径审计；不复用 Plan100 的旧构建或自动化结果。
- 共享契约 / 难合并资源风险：Profile 头、WeaponActor、CombatVfx 与二进制 DA 可能与外部武器工作重叠；发布边界必须 fetch 并逐路径审计。二进制 DA 只通过 Editor/仓库安全自动化修改。
- 基线损坏时的停止条件：最新 main 无法构建、武器 DA 无安全编辑路径、实际 Plane UV/局部轴与设计假设不一致且需要美术取舍，或远端出现真实武器表现语义冲突。

## 实现提纲

1. 在武器 Profile 增加中心化二维挂点比例，并提供缺省中心值。
2. 在 WeaponActor 建立独立 `WeaponAttackVfxRoot`，绑定当前可见武器组件；使用最终组件 Transform、实际尺寸与朝向镜像计算局部位置，并随长剑/镰刀动作更新。
3. 仿照 Boss 根节点，为 Player/Echo 的 CombatVfx 配置武器特效根；只把释放类语义改到新根节点。
4. 为四类挂点、左右镜像、最终偏移继承、动作跟随与 Boss/Travel/DamageApplied 保持不变补充聚焦测试。
5. 维护模块文档和执行记录，完成构建与静态验证，交由用户 PIE 验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目、Plan、源码与格式不变量通过 |
| C++ | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新准确预构建包 |
| 预构建 | `python scripts/ue/prebuilt_editor.py check` | manifest、源码指纹和允许 DLL 匹配 |
| 聚焦自动化 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Presentation.Combat.Capabilities` | 武器挂点、VFX 根与现有表现能力回归通过 |
| 人工 PIE | 四把武器分别朝左/右攻击，并观察长剑三挥、镰刀旋转、弓箭与枪口起点 | 特效从指定武器局部点释放且左右可明确区分 |

## 执行记录

### 变化

- `UReEchoWeaponPresentationProfile` 新增可选 `AttackVfxAnchorRatio` 覆盖；旧 DA 不需要二进制迁移，按 VisualKey 使用镰刀中心、长剑下方中心、弓/枪右侧中点默认值。
- `AReEchoWeaponActor` 新增 `WeaponAttackVfxRoot` 并绑定当前可见武器组件；局部 X 随 `VisualFacingSign` 镜像，组件层级自动继承最终手部挂点、`HeldOffsetRatio`、尺寸、贴图旋转及近战动作。
- Player/Echo 把武器根节点注册给 CombatVfx；长剑/镰刀延迟和立即 `AttackCommitted` 均从该根节点生成。Boss、Travel、DamageApplied 路径未修改。
- 新增四武器默认值、左右镜像和 DA 覆盖自动化断言；维护三个相关模块文档。
- 首轮 PIE：长剑、镰刀位置正确；弓、枪位置错误，朝右尤为明显。诊断确认投射物仍从角色固定偏移生成，且弓左/枪右的 DA 负 X Scale 会把局部挂点再次镜像。
- 修正候选：弓/枪投射物首帧直接使用 `WeaponAttackVfxRoot` 世界位置；局部挂点在父视觉组件 X 为负时抵消一次缩放镜像，确保最终屏幕侧仍按朝向选择。
- 修正候选增量验证：Development Editor 编译通过；`ReEcho.Presentation.Combat.Capabilities` 返回 `Success`。该证据将在接入最新 `origin/main` 后由最终门禁重新生成。
- 已接入 `origin/main@772b054c`；`ReEchoWeaponActor.cpp/.h` 按函数级同时保留 Plan126 最终挂点和 Plan131 镰刀斩弹，预构建包由合并后的 FullRebuild 统一刷新。

### 证据

- 合并后 `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`：`Result: Succeeded`，预构建 source fingerprint `07c2933199ef`。
- 合并后 `ReEcho.Presentation.Combat.Capabilities`：`Success`，覆盖弓/枪负缩放镜像补偿与投射物从最终武器挂点生成。
- 合并后 `ReEcho.Weapons.Runtime.MeleeProjectileCutEligibility`：`Success`，证明 Plan131 镰刀斩弹资格未被覆盖。
- 合并后 `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check` 与 `git diff --check`：通过；首次沙箱内校验仅因 C:\tmp Content 临时目录写权限失败，放行同一命令后通过，非项目缺陷。

- `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`：100/100，成功；预构建源码指纹 `46d56825feb2`。
- `scripts\ue\Run-Automation.cmd -Filter ReEcho.Presentation.Combat.Capabilities`：发现 1 项，`Result={Success}`。
- `python scripts\validate_project.py`：通过；`python scripts\ue\prebuilt_editor.py check`：7 模块、Build ID `55116800`、源码指纹匹配。
- `git diff --check`：通过。系统未提供 `clang-format` 可执行文件，修改代码按仓库格式人工检查并由 UHT/UBT 完整编译。

### 剩余风险

- 武器贴图透明边缘与 Plane 几何边缘可能不一致，最终视觉微调应使用 DA 的 `AttackVfxAnchorRatio` 或各 VFX Slot `Offset`，不能重新引入角色空间偏移。
- 自动化和 FullRebuild 不能证明 PIE 中透明像素边缘的视觉对齐；发布与关闭前仍需用户完成四武器左右朝向验收。

### 人工验收结果/请求

`Accepted`：用户确认修正后的弓/枪左右朝向表现无问题；验收期间保存的 `DA_WeaponPresentation_Gun.uasset` 属于本 Plan 已允许的枪表现配置，并纳入最终发布候选。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已更新：记录最终武器 Transform 是释放类 VFX 原点权威及阶段边界。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md` 已更新：记录 Player/Echo 武器根、角色通用回退与 Boss 独立根职责。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md` 已更新：记录默认局部点、镜像规则及逻辑模块排除项。
- `shared/CODEBASE_MAP/ARCHITECTURE.md` 已审阅、无需修改：模块拓扑与依赖方向未变化。
- `shared/CODEBASE_MAP/README.md` 已审阅、无需修改：稳定模块/AREA 路由未变化。
