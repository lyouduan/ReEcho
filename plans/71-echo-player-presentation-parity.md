# Plan 71 - 程序 - 回响复用玩家二维表现架构

## 协调

- Planner 负责人：当前对话程序 Planner。
- Executor 负责人：独立程序 Executor（待 Plan 修订发布后启动）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Ready`（原候选视觉验收发现空间表现不一致，按本次锁定范围返工）。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@a475b44d`。
- 本地实现方式：用户已确认采用一任务一 worktree；本次返工使用 `C:/Users/binnanliang/Documents/ReEcho-worktrees/echo-spatial-presentation`，分支 `codex/echo-spatial-presentation`。
- 依赖 / 阻塞：四组 Echo Flipbook 已交付至 `/Game/ReEcho/Art/Animation2D/Echos`；玩家同 `CharacterId` 的空间表现作为本次唯一视觉基准，最终视觉一致性需要 PIE 人工验收。
- Writes:
  - `plans/71-echo-player-presentation-parity.md`
  - `Source/ReEcho/Public/Graybox/ReEchoEchoActor.h`
  - `Source/ReEcho/Private/Graybox/ReEchoEchoActor.cpp`
  - `Source/ReEcho/Public/Player/ReEchoPlayerPawn.h`
  - `Source/ReEcho/Private/Player/ReEchoPlayerPawn.cpp`
  - `Source/ReEchoPresentation/Public/Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h`（仅在现有字段无法表达公共空间配置时扩展）
  - `Source/ReEchoPresentation/Private/Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.cpp`（仅配对实现需要时）
  - `Source/ReEcho/Private/Weapons/ReEchoWeaponActor.cpp`（只读取 Echo 独立瞄准方向）
  - `Source/ReEcho/Private/Tests/ReEchoSpadeAppearanceTests.cpp`、`Source/ReEcho/Private/Tests/ReEcho2DAnimationTests.cpp` 或独立 Echo 空间表现测试
  - `Content/ReEcho/Animation2D/DA_Echo_J_HEART.uasset`
  - `Content/ReEcho/Animation2D/DA_Echo_J_SPADE.uasset`
  - `Content/ReEcho/Animation2D/DA_Echo_J_CLOVER.uasset`
  - `Content/ReEcho/Animation2D/DA_Echo_J_DIAMOND.uasset`
  - `Content/ReEcho/Animation2D/DA_EchoPresentationCatalog.uasset`（若现有 Catalog 无法保持 Player/Echo 域隔离）
  - `scripts/ue/author_echo_presentation_profiles.py`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`（若公共 Profile/API 发生变化）
  - `Binaries/Win64/` 下 `GIT_RULES.md` 允许的最终预构建包文件
- Stable Reads:
  - `Source/ReEcho/{Public,Private}/Player/ReEchoPlayerPawn.*`
  - `Source/ReEchoPresentation/{Public,Private}/Presentation/Animation2D/*`
  - `Source/ReEcho/{Public,Private}/Recording/ReEchoPlaybackComponent.*`
  - `Source/ReEcho/{Public,Private}/Weapons/ReEchoWeaponActor.*`
  - `Content/ReEcho/Animation2D/DA_Character_J_*.uasset`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`
- 影响模式：`SharedContract`（Echo Host 改为消费既有 Presentation 公共组件与数据契约，不修改 Combat、Weapons 或 Recording 权威）。
- 兼容承诺 / 下游操作：动画或 Profile 缺失只能降低表现并回退静态 Billboard，不得阻止 Echo 生成、移动、攻击、伤害或回放；Echo 可以保留独立动画素材与颜色/透明度差异，但不得拥有一套与玩家漂移的空间参数真源。
- 明确排除：不修改 Recording、Combat、攻击时序和伤害权威；不改变玩家已经确认的空间表现；不让动画帧或播放完成驱动攻击结算；不把 CharacterId/武器类型硬编码为 EchoActor 内的空间补偿；不在 Editor 外修改 `.uasset`。

## 锁定目标

1. 四个规范角色 ID 分别使用 Heart、Spade、Clover、Diamond 对应 Echo 资源，不串用 Player 资源或其他角色 Echo。
2. Echo 复用 Player 的 `Catalog/Profile/Controller/UReEcho2DAnimationComponent` 表现链以及脚点、相机面片、镜像、阴影和播放完成回退规则。
3. 默认与移动播放各自 Walk；近战普通攻击播放 Attack；Bow、Gun、Staff 普通攻击通过 `WeaponVisualKey` 解析 Attack_Arrow。
4. EchoActor 只提交移动、攻击和朝向等表现意图，不保存 Flipbook Map、攻击动画倒计时或按 AttackPattern 字符串选择资源。
5. 表现失败不改变 Recording、Weapons、Combat 和 Echo 生命周期。
6. 同一 `CharacterId` 下，Echo 与玩家使用同一空间表现权威：角色位置/尺寸/缩放、武器持有位置/缩放、阴影位置/尺寸、攻击与受击特效根位置/缩放必须一致；Echo 独立 Profile 只允许提供动画素材和回响材质差异。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho / AREA-Presentation`、`AREA-Recording`；只读消费 `MOD-ReEchoPresentation` 公共接口。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；关闭前审阅 `MOD-ReEchoPresentation.md`，公共接口事实未变则记录无需修改。
- 设计意图：让 Player 与 Echo 共享同一种数据驱动二维表现执行方式；独立 Echo Profile/Catalog 只隔离动画素材，不再复制角色空间布局。空间权威按 `CharacterId` 来自玩家角色配置，Player/Echo Host 通过同一应用路径消费。
- 权威状态与依赖：移动与攻击事实仍由 Recording/Weapons/Combat 拥有；Presentation Controller 只执行动画语义。模块依赖仍为 `ReEcho -> ReEchoPresentation` 单向。
- 决策记录：
  1. 使用独立 Echo Profile，不覆盖 Player Profile，也不在 Actor 构造函数维护三组硬引用 Map。
  2. 武器差异使用现有 `WeaponVisualKey` Clip 路由；Melee VisualSet 指向 Attack，Bow/Gun/Staff VisualSet 指向 Attack_Arrow。
  3. Echo 组件树对齐 Player 的 `PresentationRoot -> FootRoot -> PresentationMotionRoot/ GroundRoot`，保留 Billboard 作为资源缺失回退。
  4. 攻击成功只向 Controller 请求 `Attack.Basic`；播放结束由状态机返回当前 Idle/Move，不使用手写秒表。
  5. 空间一致不是简单复制 Actor 世界 Transform：统一脚底原点后，角色、Weapon Anchor、GroundRoot/GroundShadow、AttackVfxRoot/HurtVfxRoot 均消费同一组相对参数；Echo 回放继续只写玩法根世界位置。
  6. 武器最终布局仍由“角色公共 Weapon Anchor + 武器 Profile 自身 HeldOffset/尺寸”组合，不能在 EchoActor 增加武器 ID 特判。
  7. 阴影继续由渲染 Bounds 推导时，Player 与 Echo 必须调用同一计算契约并使用相同作者ing基准；若 Echo 素材原始像素尺寸不同，则先规范化角色显示尺寸，再计算阴影，避免阴影反向成为第二尺寸来源。
  8. Combat VFX 继续挂接语义化 Attack/Hurt 根；本 Plan 只统一根的相对 Transform 和继承缩放，不改变 Niagara 资产、伤害事件或生命周期。
- 相关文档同步范围：审阅 `ARCHITECTURE.md` 与 `README.md`，预计模块拓扑和 AREA 路由不变；更新 `MOD-ReEcho.md`；审阅 `MOD-ReEchoPresentation.md`。
- 关闭前逐项填写审阅结果：在执行记录中记录上述文档已更新或无需修改的理由。

## 锁定验收

- [ ] 四个 CharacterId 均解析到自己的 Echo Profile，Walk/Attack/Attack_Arrow 共 12 个 Flipbook 可加载。
- [ ] Melee 与 Bow/Gun/Staff 分别选择 Attack 与 Attack_Arrow，攻击结束自动返回当前 Idle/Move。
- [ ] Echo 移动、停止和左右瞄准只提交表现意图，不旋转或缩放玩法根 Actor。
- [ ] Profile/Catalog 缺失时静态回退可见且 Echo 玩法仍可初始化。
- [ ] 修改的 C++ 完成 `.clang-format`，Development 构建、聚焦自动化、`validate_project.py` 与 `git diff --check` 通过。
- [ ] 最终发布候选完成 Development FullRebuild 并刷新精选预构建包。
- [ ] PIE 人工验收四角色尺寸、脚点、阴影、左右朝向、移动/停止及近远程攻击切换。
- [ ] 对每个规范 `CharacterId`，Player/Echo 的角色显示包围盒尺寸、脚底基准与相对缩放一致；左右镜像后不新增位置漂移。
- [ ] 相同角色与 `WeaponVisualKey` 下，Player/Echo 的武器 Actor Anchor、Held Visual 相对位置与最终显示尺寸一致，不存在 Echo 专用武器补偿。
- [ ] Player/Echo 的 GroundRoot 中心、GroundShadow 最终宽高一致，并保持相同的脚底相对关系。
- [ ] Player/Echo 的 AttackVfxRoot/HurtVfxRoot 相对 Transform 与继承缩放一致；相同测试特效的生成原点和最终尺寸一致。
- [ ] 自动化使用可读的空间快照逐字段比较以上参数，容差具名且不以截图或人工目测替代；最终视觉质量仍由用户 PIE 确认。
- [ ] 未提交不相关美术资产或精选预构建允许列表之外的生成产物。

## Step 0 门禁

- 基线分支/提交：`origin/main@a475b44d`；已审计 `55a15d15..a475b44d` 的 Plan87、商店 UI 与预构建更新，与 Player/Echo 空间表现源码没有直接路径冲突；`MOD-ReEcho.md` 与最终预构建包存在集成耦合，发布时必须基于最新远端重建。
- 引擎/构建可用性：UE 5.8；构建与 Editor 命令前确认交互式 Editor 已关闭。
- 现有聚焦测试结果：临时硬编码方案的四角色 Walk 映射测试通过，但不作为本 Plan 架构验收证据。
- 共享契约 / 难合并资源风险：优先通过 C++ 公共空间契约消除漂移；若必须调整 Profile/Catalog `.uasset`，只能通过 Unreal Editor API 修改并在执行记录列出准确资产。Plan87 会修改 Player Host，实施与集成时需审计其实际落地差异。
- 基线损坏时的停止条件：远端再次前进、同路径 Echo 资产/Profile 外部变化、Editor API 无法创建有效 Profile 或基础构建失败时停止审计。

## 实现提纲

1. 对齐 Player 组件树并为 Echo 装配 Controller、Animation、FrameCollision 与 SceneLighting。
2. 用 Editor 脚本创建四个 Echo Profile 和隔离 Catalog，配置 Walk、近战 Attack 及三类远程 VisualSet 的 Attack_Arrow。
3. 用回放位置变化、成功攻击提交和瞄准方向驱动 Controller 意图，移除硬编码 Map/Pattern/计时器。
4. 增加 Profile、武器 VisualSet、状态回退与安全 fallback 聚焦测试，维护模块文档。
5. 完成构建、自动化和静态检查后交付 PIE 人工验收。
6. 本次返工先提取 Player/Echo 可比较的空间快照，确认四类差异的真实来源；再建立公共空间应用路径，禁止以 Echo 专用常量逐项对齐截图。
7. 为角色、武器、阴影、Attack/Hurt VFX 根增加逐字段一致性测试，覆盖四个 CharacterId、至少一个近战与一个远程 WeaponVisualKey，以及左右朝向。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| C++ 格式 | 仓库 `.clang-format` | 修改文件符合项目格式 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码 0 |
| 聚焦自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Presentation.EchoAppearance` | 映射、VisualSet、状态与 fallback 通过 |
| Editor 资产 | 作者ing 脚本幂等执行两次并重新加载 | 四 Profile/Catalog/12 Flipbook 引用完整 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目与文本不变量通过 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终预构建包匹配源码 |
| 人工 | PIE 四角色、近远程、移动/停止、左右朝向 | 记录 Passed 或明确延期 |

## 执行记录

### 变化

- EchoActor 已改为 Player 同构的 Presentation/Foot/Motion/Flipbook/Ground 组件树，并装配 PresentationController、FrameCollisionDriver 与 SceneLighting。
- 新增独立 Echo Catalog 与四个 Profile；四角色分别绑定自己的 Walk/Attack/Attack_Arrow，Staff/Bow/Gun 通过 WeaponVisualKey 覆盖远程攻击动画。
- 移除 EchoActor 内三组 Flipbook Map、AttackPattern 字符串分支、攻击动画秒表和程序缩放脉冲；攻击成功只提交 `Animation.Attack.Basic`，播放完成由 Controller 回到 Idle/Move。
- Echo 自动索敌写入独立 `AttackAimDirection` 并驱动相机横向镜像；WeaponActor 读取该方向，玩法根 Actor 不再为表现或瞄准旋转。
- 保留 Billboard 作为 Profile/Flipbook 缺失时的静态安全回退。
- 2026-08-24 返工：用户明确“一致”仅指角色、武器、阴影、特效四类空间位置与大小，不涉及录像内容、动作时序或战斗权威。审计确认当前 Echo 独立 Profile 与 Host 基准仍可形成第二套空间参数，Plan 重新进入 `Ready`。

### 证据

- `author_echo_presentation_profiles.py` 连续执行后日志确认 `[Plan71] Echo presentation profiles and isolated catalog verified`。
- 隔离未发布 Sage 源码后的最终 Development FullRebuild 通过，95 个 action 完成并刷新预构建源码指纹 `9ddb7f5f3fa1`。
- `ReEcho.Presentation.EchoAppearance.CharacterMappings` 发现 1 项并 `Success`；覆盖四 Catalog 映射、12 个 Flipbook 加载、近战 Attack、Bow Attack_Arrow、Actor 实际 Walk 与未知 ID 拒绝。

### 剩余风险

- 自动化不能判断四角色在 SC02 中的最终视觉尺寸、脚点、阴影宽度和攻击动作观感，仍需 PIE 人工验收。
- 人工 PIE 视觉验收仍未记录；按用户要求先发布 Review 候选，Plan 保持未关闭。
- Echo 与 Player 使用不同像素尺寸的动画素材时，仅复制组件 Scale 不能保证最终包围盒一致；实现必须比较规范化后的渲染尺寸与脚底坐标。
- Plan87 后续会写 Player Host；若其在本 Plan 实现前落地，必须重新审计并组合适配，不能覆盖角色能力事件路由。

### 人工验收结果/请求

`PendingBeforeClose`：PIE 逐一检查 Heart/Spade/Clover/Diamond 的移动、停止、左右朝向，以及近战和 Staff/Bow/Gun 攻击切换。

### 架构文档审阅结果

- `MOD-ReEcho.md`：已更新，记录 Echo 与 Player 同构表现链、独立 Catalog/Profile、WeaponVisualKey 路由和独立瞄准方向。
- `MOD-ReEchoPresentation.md`：已审阅、无需修改；本 Plan 只消费既有 Catalog/Profile/Controller 公共契约，没有修改 Presentation 模块接口或所有权。
- `ARCHITECTURE.md`：已审阅、无需修改；Runtime Module 拓扑与 `ReEcho -> ReEchoPresentation` 单向依赖不变。
- `README.md`：已审阅、无需修改；仍属于现有 `AREA-Presentation` 与 `AREA-Recording` 路由。
