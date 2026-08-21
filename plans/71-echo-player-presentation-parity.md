# Plan 71 - 程序 - 回响复用玩家二维表现架构

## 协调

- Planner 负责人：当前对话程序 Planner。
- Executor 负责人：当前对话程序 Executor。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@a2817442`。
- 本地实现方式：用户已确认不采用一任务一 worktree，直接在当前本地 `main` 执行；Plan-only 发布使用临时干净副本，不承载实现。
- 依赖 / 阻塞：四组 Echo Flipbook 已交付至 `/Game/ReEcho/Art/Animation2D/Echos`；最终尺寸、脚点、朝向和攻击观感需要 PIE 人工验收。
- Writes:
  - `plans/71-echo-player-presentation-parity.md`
  - `Source/ReEcho/Public/Graybox/ReEchoEchoActor.h`
  - `Source/ReEcho/Private/Graybox/ReEchoEchoActor.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoSpadeAppearanceTests.cpp` 或独立 Echo 表现测试
  - `Content/ReEcho/Animation2D/DA_Echo_J_HEART.uasset`
  - `Content/ReEcho/Animation2D/DA_Echo_J_SPADE.uasset`
  - `Content/ReEcho/Animation2D/DA_Echo_J_CLOVER.uasset`
  - `Content/ReEcho/Animation2D/DA_Echo_J_DIAMOND.uasset`
  - `Content/ReEcho/Animation2D/DA_EchoPresentationCatalog.uasset`（若现有 Catalog 无法保持 Player/Echo 域隔离）
  - `scripts/ue/author_echo_presentation_profiles.py`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `Binaries/Win64/` 下 `GIT_RULES.md` 允许的最终预构建包文件
- Stable Reads:
  - `Source/ReEcho/{Public,Private}/Player/ReEchoPlayerPawn.*`
  - `Source/ReEchoPresentation/{Public,Private}/Presentation/Animation2D/*`
  - `Source/ReEcho/{Public,Private}/Recording/ReEchoPlaybackComponent.*`
  - `Source/ReEcho/{Public,Private}/Weapons/ReEchoWeaponActor.*`
  - `Content/ReEcho/Animation2D/DA_Character_J_*.uasset`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`
- 影响模式：`SharedContract`（Echo Host 改为消费既有 Presentation 公共组件与数据契约，不修改 Combat、Weapons 或 Recording 权威）。
- 兼容承诺 / 下游操作：动画或 Profile 缺失只能降低表现并回退静态 Billboard，不得阻止 Echo 生成、移动、攻击、伤害或回放。
- 明确排除：不修改 Player 的角色 Profile；不让动画帧或播放完成驱动攻击结算；不把 CharacterId/武器类型硬编码为 EchoActor 内的 Flipbook 选择；不在 Editor 外修改 `.uasset`。

## 锁定目标

1. 四个规范角色 ID 分别使用 Heart、Spade、Clover、Diamond 对应 Echo 资源，不串用 Player 资源或其他角色 Echo。
2. Echo 复用 Player 的 `Catalog/Profile/Controller/UReEcho2DAnimationComponent` 表现链以及脚点、相机面片、镜像、阴影和播放完成回退规则。
3. 默认与移动播放各自 Walk；近战普通攻击播放 Attack；Bow、Gun、Staff 普通攻击通过 `WeaponVisualKey` 解析 Attack_Arrow。
4. EchoActor 只提交移动、攻击和朝向等表现意图，不保存 Flipbook Map、攻击动画倒计时或按 AttackPattern 字符串选择资源。
5. 表现失败不改变 Recording、Weapons、Combat 和 Echo 生命周期。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho / AREA-Presentation`、`AREA-Recording`；只读消费 `MOD-ReEchoPresentation` 公共接口。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；关闭前审阅 `MOD-ReEchoPresentation.md`，公共接口事实未变则记录无需修改。
- 设计意图：让 Player 与 Echo 共享同一种数据驱动二维表现执行方式，同时用独立 Echo Profile/Catalog 隔离两套美术资产。
- 权威状态与依赖：移动与攻击事实仍由 Recording/Weapons/Combat 拥有；Presentation Controller 只执行动画语义。模块依赖仍为 `ReEcho -> ReEchoPresentation` 单向。
- 决策记录：
  1. 使用独立 Echo Profile，不覆盖 Player Profile，也不在 Actor 构造函数维护三组硬引用 Map。
  2. 武器差异使用现有 `WeaponVisualKey` Clip 路由；Melee VisualSet 指向 Attack，Bow/Gun/Staff VisualSet 指向 Attack_Arrow。
  3. Echo 组件树对齐 Player 的 `PresentationRoot -> FootRoot -> PresentationMotionRoot/ GroundRoot`，保留 Billboard 作为资源缺失回退。
  4. 攻击成功只向 Controller 请求 `Attack.Basic`；播放结束由状态机返回当前 Idle/Move，不使用手写秒表。
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
- [ ] 未提交不相关美术资产或精选预构建允许列表之外的生成产物。

## Step 0 门禁

- 基线分支/提交：`origin/main@a2817442`；已审计其相对旧基线的 Plan67/Plan63 商店、暂停与预构建变化，与 Echo 路径无源码/资产逻辑冲突。
- 引擎/构建可用性：UE 5.8；构建与 Editor 命令前确认交互式 Editor 已关闭。
- 现有聚焦测试结果：临时硬编码方案的四角色 Walk 映射测试通过，但不作为本 Plan 架构验收证据。
- 共享契约 / 难合并资源风险：四个新 Profile/Catalog 和 Echo 源资源为二进制独占写入面；只允许 Unreal Editor API 创建、修改、保存。
- 基线损坏时的停止条件：远端再次前进、同路径 Echo 资产/Profile 外部变化、Editor API 无法创建有效 Profile 或基础构建失败时停止审计。

## 实现提纲

1. 对齐 Player 组件树并为 Echo 装配 Controller、Animation、FrameCollision 与 SceneLighting。
2. 用 Editor 脚本创建四个 Echo Profile 和隔离 Catalog，配置 Walk、近战 Attack 及三类远程 VisualSet 的 Attack_Arrow。
3. 用回放位置变化、成功攻击提交和瞄准方向驱动 Controller 意图，移除硬编码 Map/Pattern/计时器。
4. 增加 Profile、武器 VisualSet、状态回退与安全 fallback 聚焦测试，维护模块文档。
5. 完成构建、自动化和静态检查后交付 PIE 人工验收。

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

### 证据

### 剩余风险

### 人工验收结果/请求

### 架构文档审阅结果
