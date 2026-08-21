# Plan 72 - 程序 - 首场战斗资源异步预加载

## 协调

- Planner 负责人：当前程序侧 Planner。
- Executor 负责人：Plan 发布后分配。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Unassigned`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@bfc2384fbf6a1c7fe6c2cfc4e3b309c4ab01a019`。
- 本地实现方式：用户已确认不采用一任务一 worktree；Plan-only 在临时干净副本发布，随后直接在本地 `main` 实现。
- 依赖 / 阻塞：依赖已发布 Plan71 Echo Catalog/Profile；保护当前本地 Sage 与用户美术脏工作。
- Writes：本 Plan；`Source/ReEcho/{Public,Private}/Presentation/Loading/ReEchoRuntimeAssetPreloader.*`；`Presentation/VFX/ReEchoCombatVfxCatalog.*`；主模块武器表现路径 Catalog/Actor；`ReEchoGameMode.*`；聚焦测试；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoVFX.md`、`MOD-ReEchoWeapons.md`。
- Stable Reads：Plan71 Echo Catalog/Profile；`ReEchoWeaponActor`、`ReEchoProjectileActor`、`ReEchoSwordArcActor`、`ReEchoStaffLightWaveActor`；`Source/ReEchoPresentation` 公共契约；现有 cook 配置。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：不改变伤害、攻击节拍、资源路径、cook 目录或缺失表现资源时继续游戏的降级语义；原同步读取保留为安全回退，正常首场路径命中已驻留资源。
- 明确排除：不新增完整 Loading Screen；不预载整个 Content；不修改 `.uasset`/`.umap`、音频、存档、XLSX/CSV 或玩法契约；不承诺消除 Shader/PSO hitch。

## 锁定目标

开始菜单/装载选择阶段异步预热首场战斗会首次使用的武器轻量表现、Combat VFX 及依赖。确认角色/武器时若资源尚未完成，则保留选择界面直至完成后只进入战斗一次；首次攻击、受击及首个相关敌人技能不再承担这些 UObject 的同步磁盘读取。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho / AREA-Presentation / AREA-Player`；文档型 `MOD-ReEchoVFX`；主模块武器表现适配和受说明影响的 `MOD-ReEchoWeapons`。
- 对应模块文档：维护 `MOD-ReEcho.md`、`MOD-ReEchoVFX.md`、`MOD-ReEchoWeapons.md`，均已加入 Writes。
- 设计意图：具体软路径仍由 VFX/武器 Catalog 拥有；GameInstance 生命周期预加载器只聚合去重路径、持有异步句柄和发布完成结果；GameMode 仅编排启动与等待。
- 权威状态与依赖：加载状态由预加载器独占，不进入 Combat/Enemies 逻辑，不改变玩法状态所有者、公共玩法契约或模块依赖方向。
- 决策记录：采用显式 Catalog 枚举和 `FStreamableManager::RequestAsyncLoad`，不递归扫描资产目录、不在 GameMode 复制路径。攻击点继续同步回退，异步失败只降级视觉而不阻塞玩法。
- 相关文档同步范围：审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEchoPresentation.md`；预计拓扑/路由/公共 Presentation 契约不变。更新上述三份 Writes 文档。
- 关闭前逐项填写审阅结果：见执行记录。

## 锁定验收

- [ ] 清单包含全部 Combat VFX 根、兔子代理材质/纹理及六武器首用表现，路径规范化、去重且无空项。
- [ ] 完成/失败回调幂等；空清单或异步失败不会永久阻塞开始流程。
- [ ] 未完成时 `HandleLoadoutConfirmed` 不提前进入战斗；完成后只调用一次；已完成快路径不增加强制等待。
- [ ] 现有缺失资源降级语义保持，攻击与遭遇不因表现资源失败而失败。
- [ ] FullRebuild、聚焦自动化、项目校验和 `git diff --check` 通过。
- [ ] 用户在 PIE/打包冷启动确认首次攻击、受击和兔/狐代表技能无新增明显卡顿，选择流程不重复或卡死。
- [ ] 未提交精选预构建允许列表外的生成物。

## Step 0 门禁

- 基线分支/提交：`main@bfc2384f`，fetch 后与 `origin/main` 一致。
- 引擎/构建可用性：UE 5.8 Development FullRebuild 已用于 Plan71 发布；Plan72 最终候选重跑。
- 现有聚焦测试结果：`ReEcho.Presentation.EchoAppearance.CharacterMappings` 成功。
- 共享契约 / 难合并资源风险：本地大量 `.uasset` 与 Sage 修改；只显式暂存 Plan72 路径，禁止批量暂存。
- 基线损坏时的停止条件：句柄无法可靠驻留、Shipping 路径不可达、开始流程重复/永久等待，或需改变玩法/存档/资产时停止回报。

## 实现提纲

1. 将武器轻量表现路径集中到可枚举 Catalog；VFX Catalog 提供同一映射的预加载路径枚举。
2. 新增 GameInstance 生命周期预加载器，规范化/去重软路径、异步加载、持有句柄并提供幂等完成结果。
3. 菜单阶段尽早启动；确认装载以完成状态门控 `BeginSelectedRun`，保留当前 UI 和安全失败回退。
4. 增加清单、回调和开始门控测试与耗时日志；维护模块文档。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目不变量通过 |
| C++ | `.clang-format`、`Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功且预构建指纹匹配 |
| 自动化 | `Run-Automation.cmd -Filter ReEcho.Presentation.RuntimeAssetPreload` | 清单、去重、回调和门控通过 |
| 回归 | EchoAppearance、CombatVfx 相关用例 | Plan71/VFX 映射无回归 |
| 人工 | PIE/打包冷启动代表路径 | 无明显首用卡顿、重复进入或卡死 |

## 执行记录

### 变化

待执行。

### 证据

待执行。

### 剩余风险

Shader/PSO、驱动缓存与 OS 冷盘读取不完全等同于 UObject 异步加载，人工验证需区分 hitch 来源。

### 人工验收结果/请求

`PendingBeforeClose`。

### 架构文档审阅结果

待执行。
