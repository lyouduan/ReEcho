# Plan 66 - 程序 - Shipping 包 VFX 资产入包（消除“怪物无特效”）

## 协调

- Planner 负责人：Gavyn-side AI（按 shared/PLANNER_RULES.md 履行）
- Executor 负责人：Gavyn-side AI（待分配；本 Plan 实现可与规划同一 AI）
- Plan 编写方（AI 侧）：`Gavyn-side AI | ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`Unassigned | Gavyn-side AI | ReEcho teammate-side AI`。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）；实现已完成、IoStore 客观验证通过（包内 81 个 VFX 资产），待发布门禁（`-FullRebuild` + 预构建包 + `validate_project.py`）后推 main、再视秘书流程关闭。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：`origin/main` 在分配编号时的最大值之后下一空闲编号（本 Plan = 66）；实现基线发布后从 `origin/main` 建立。
- 本地实现方式（可选，仅作交接说明）：建议一任务一 worktree（如 `ReEcho-plan66`），不在主工作区直接实现。
- 依赖 / 阻塞：无外部阻塞；受 `shared/GIT_RULES.md` 发布门禁约束（推 main 前须 `-FullRebuild` + 精选预构建包 + `validate_project.py`）。
- Writes：
  - `Config/DefaultGame.ini`（新增 `/Game/VFX` 与 `/Game/ReEcho/Materials/VFX` 的 `DirectoriesToAlwaysCook`，让 cook 枚举并 stage VFX 资产树）
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`（补充「Shipping 打包：VFX 资产如何入包」一节）
- Stable Reads：
  - `Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxComponent.cpp`（运行时 `LoadObject<UNiagaraSystem>` 路径加载，行 211 附近）
  - `Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxCatalog.cpp`（`ResolvePath` / `ResolveRabbitProjectile*` 资产路径，行 200+）
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`（VFX 资产目录与映射契约）
  - 包内 `ReEcho-Windows.pak`（`UnrealPak.exe -List` 验证 `/Game/VFX` 资产是否在包内）
- 影响模式：`Isolated`（`Isolated | ReadOnly | SharedContract | Exclusive`，仅说明集成影响，不是写锁）。
- 兼容承诺 / 下游操作：不改动 VFX 语义、资产路径映射、资产命名、运行时 `LoadObject` 契约；仅在打包层保证这些已存在资产到达 Shipping 包，不影响 Development/PIE 既有路径。
- 明确排除：不引入新运行时数据表；不修改 `ReEchoCombatVfxCatalog` 的路径映射；不新增占位资产；不触及 Plan 59 的 CSV 入包逻辑（两件事是同类但独立子系统）。

## 锁定目标

让 `scripts/ue/package_windows.py` 产出的 Win64 Shipping 包**包含 `/Game/VFX` 资产树**，使运行时 `LoadObject<UNiagaraSystem>` / 材质 / 纹理能成功加载，达成：

1. Shipping 包 `ReEcho-Windows.pak` 内含 `/Game/VFX` 资产（用 `UnrealPak.exe -List` 验证非空，至少含怪物蓄力 `NS_Rabbit_Charging_01`、玩家长剑 `NS_People_Sword_Attack_01` 等）。
2. 兔子子弹材质 / 纹理（`/Game/ReEcho/Materials/VFX/M_RabbitProjectileGlow`、`BaseVFX003_Inst12`，`/Game/VFX/Monster/Rabbit/Tex`、`/Game/VFX/Monster/Rabbit/MI`）同样入包。
3. 实机运行怪物（蓄力、攻击、子弹拖尾）与玩家（长剑挥砍）VFX 可见；启动/运行日志不再出现 `[VFX] Missing semantic asset`（降级警告）。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoVFX`（运行时 VFX 目录映射与资产加载，见 `Source/ReEcho/Private/Presentation/VFX/*`）。本 Plan 仅补打包投递，不改映射本身。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md` 需补充「Shipping 打包：VFX 资产如何入包」一节（已列入 Writes）。
- 设计意图：`MOD-ReEchoVFX.md` 已明确 VFX 是「纯资产 + 目录映射驱动的运行时挂载，不写进任何数据表 / 不构造成 GAS 装配」。组件在运行时用 `LoadObject<>` 按 C++ 字符串路径加载 Niagara/材质/纹理。cooker 只 cook「从根地图 `/Game/Level00` 出发沿资产引用图可达」的资产；这些 VFX 资产只在 `.cpp` 字符串里出现、没有任何 cooked 资产（地图/蓝图/DataAsset）硬/软引用它们，因此被 cook 漏掉。这与 Plan 59 的 CSV 漏包是同一根因家族（cook 不枚举非资产引用），但分属不同子系统。
- 实证（本次打包前已确认）：用 `UnrealPak.exe -List ReEcho-Windows.pak | Select-String /Game/VFX` 结果为 **0 条**；源 `Content/VFX` 有 79 个资产（`NS_*`、`M_*`、`MI_*`、`T_*`、`BP_*`），全未进包。运行时 `LoadObject` 返回 null，`ReEchoCombatVfxComponent.cpp:212-219` 走降级分支打 `Warning: [VFX] Missing semantic asset ... gameplay continues without it`——不崩、玩法照常、仅缺特效，与“怪物无特效”现象完全吻合。
- 决策记录：
  - 候选方案 A（推荐先验证）：在 `Config/DefaultGame.ini` 增加 `+DirectoriesToAlwaysCook=(Path="/Game/VFX")` 与 `+DirectoriesToAlwaysCook=(Path="/Game/ReEcho/Materials/VFX")`。优点：纯 ini 配置、符合 UE 资产范式、cook 会递归枚举该目录全部资产并 stage。代价：每次新增 VFX 子目录须确认在覆盖范围内（当前 `/Game/VFX` 递归已覆盖 People/Monster/Weapon/Common 全部）。
  - 候选方案 B：在 `/Game/VFX` 放一个被 cook 引用的 `PrimaryAssetLabel` 或占位 DataAsset，强制 cook 枚举。优点：可在资产层管理白名单。代价：需维护资产、且与方案 A 功能重叠；方案 A 一次通过即停。
  - 候选方案 C：运行时改为软引用资产注册表 + `AssetManager` 异步加载。代价：改动运行时契约、超出“打包投递”范围，不作为本 Plan 方案。
  - 选择：实现者按 A→B→C 顺序验证，以“pak 内 `/Game/VFX` 非空且实机 VFX 可见”为通过判据，命中其一即停。
- 相关文档同步范围：
  - `modules/MOD-ReEchoVFX.md`：补充「Shipping 打包：VFX 资产如何入包」一节（列入 Writes）。
  - `ARCHITECTURE.md` / `README.md`：路由无变化，审阅说明。
- 关闭前逐项填写审阅结果。

## 锁定验收

- [ ] Shipping 包 `ReEcho-Windows.pak` 含 `/Game/VFX` 资产（`UnrealPak.exe -List | Select-String /Game/VFX` 非空，含 `NS_Rabbit_Charging_01`、`NS_People_Sword_Attack_01`）。
- [ ] 兔子子弹材质/纹理（`/Game/ReEcho/Materials/VFX/*`、`/Game/VFX/Monster/Rabbit/Tex`、`/Game/VFX/Monster/Rabbit/MI`）入包。
- [ ] 实机运行怪物 VFX（蓄力/攻击/子弹拖尾）与玩家长剑挥砍可见；日志无 `[VFX] Missing semantic asset` 降级警告。
- [ ] `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` 通过（发布门禁，推 main 前必跑）。
- [ ] `python scripts/validate_project.py` 通过（静态不变量 / CSV schema / UTF-8）。
- [ ] 未提交 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。
- [ ] 人工验收 `PendingBeforeClose`：在目标机器实际启动打包 exe 确认 VFX 可见，记录结果或明确延期跟进。

## Step 0 门禁

- 基线分支/提交：从 `origin/main` 最新提交（`f79ee64` Plan 65）建立实现分支。
- 引擎/构建可用性：UE 5.8 位于 `C:\Program Files\Epic Games\UE_5.8`；`UnrealPak.exe` 位于 `C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealPak.exe`。
- 现有聚焦测试结果：漏包根因已定位（见执行记录）；尚未有自动化覆盖“包内 VFX 资产完整性”。
- 共享契约 / 难合并资源风险：本 Plan 仅改 `DefaultGame.ini`（文本）+ 模块文档，与二进制构建产物无同文件冲突。
- 基线损坏时的停止条件：若对齐后发现其它 Plan 改动 `/Game/VFX` 资产或 `ReEchoCombatVfxCatalog` 路径映射产生逻辑冲突，停止并报告。

## 实现提纲

1. 先解决基线：发布 Plan 前 `git fetch`，确认本地 `main` 与 `origin/main` 对齐。
2. 实现 VFX 入包：在 `Config/DefaultGame.ini` 紧接现有 `+DirectoriesToAlwaysCook=(Path="/Game/ReEcho/Audio")` 之后，新增 `/Game/VFX` 与 `/Game/ReEcho/Materials/VFX` 两项 `DirectoriesToAlwaysCook`。
3. 重新打包并验证：`UnrealPak.exe -List ReEcho-Windows.pak | Select-String /Game/VFX` 非空；可选启动 exe 确认 VFX 可见。
4. 更新 `MOD-ReEchoVFX.md` 的「Shipping 打包」一节，并填写执行记录。
5. 发布门禁：`-FullRebuild` + 刷新预构建包 + `validate_project.py`，提交并推 main。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py` | 项目/源码不变量、CSV schema、UTF-8 通过 |
| 发布门禁 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` + 刷新精选预构建包 | 推 main 前通过 |
| 打包 | `python scripts/ue/package_windows.py --output <dir> --no-smoke` | `BUILD SUCCESSFUL` |
| 包内资产 | `UnrealPak.exe -List <pak> \| Select-String /Game/VFX` | 非空，含 `NS_Rabbit_Charging_01` 等 |
| 运行时行为 | 启动打包 exe（真实窗口） | 怪物/玩家 VFX 可见；无 `[VFX] Missing semantic asset` 警告 |

## 执行记录

### 变化

- 已实现（方案 A 命中即停）：`Config/DefaultGame.ini` 紧接现有 `+DirectoriesToAlwaysCook=(Path="/Game/ReEcho/Audio")` 之后，新增 `+DirectoriesToAlwaysCook=(Path="/Game/VFX")` 与 `+DirectoriesToAlwaysCook=(Path="/Game/ReEcho/Materials/VFX")`，强制 cook 枚举并 stage 整个 VFX 资产树（含兔子子弹材质/纹理）。方案 B/C 未走。
- 未走 Plan 59 的 `package_windows.py` 显式拷贝路线：VFX 是 uasset 二进制（需 cook 后的平台格式），必须经由 cook/stage 入 IoStore，不能与 csv 松散文件同样处理；ini `DirectoriesToAlwaysCook` 是适用且最小侵入的修法。

### 证据

- 打包实测（plan/66 分支，`scripts/ue/package_windows.py --output c:\Users\gavynqiu\Desktop\ReEchoPackage`）：`BUILD SUCCESSFUL`（ExitCode=0）；日志 `Staged 28 runtime CSV(s) into 1 Content/Data dir(s)`（Plan 59 的 csv 投递仍生效）。
- VFX 入包验证（纠正工具误用）：UE 5.8 默认 IoStore，游戏资产在 `ReEcho-Windows.utoc/.ucas`，**不能**用 `UnrealPak -List *.pak` 验证（`.pak` 仅含 ini，`/Game/VFX` 命中 0 是误导）。正确命令：
  `UnrealPak.exe ReEcho-Windows.utoc -List | Select-String "ReEcho/Content/VFX"`，结果 **81 个 VFX 资产**（源 `Content/VFX` 79 个 + `Materials/VFX` 等），含怪物 `NS_Rabbit_Charging_01`/攻击/被击、玩家 `NS_People_Sword_Attack_01`、兔子子弹 `M_RabbitProjectileGlow`/`BaseVFX003_Inst12` 及 `Rabbit/Tex`/`Rabbit/MI` 共 31 个材质纹理——IoStore 容器内路径形如 `../../../ReEcho/Content/VFX/...`。
- 过程乌龙记录：首次打包因编辑器（PID 5484 `UnrealEditor.exe`「ReEcho - 虚幻编辑器」）占用 8000 端口导致 cook 命令令 `HttpListener unable to bind` 退出（ExitCode=25）；关闭编辑器后重打成功。这与 ini 改动无关（ini 只加 cook 目录，不碰端口）。

### 剩余风险

- `DirectoriesToAlwaysCook` 为对 cook 的显式指令，机制成熟可靠；风险低。若未来新增 VFX 子目录落在 `/Game/VFX` 之外，须同步补 ini 项（方案 A 递归已覆盖当前全部子目录）。
- 验证工具陷阱已写入 `MOD-ReEchoVFX.md`：后续任何人用 `UnrealPak -List *.pak` 查 VFX 会得到误导性的 0，应以 `*.utoc` 为正确对象。

### 人工验收结果/请求

- 待：在目标机器实际启动 Shipping 包确认怪物/玩家 VFX 可见（PendingBeforeClose）。资产入包已用 IoStore 清单客观验证（81 个 VFX 资产），运行时表现由人工实机确认。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`：已新增「Shipping 打包：VFX 资产如何入包」一节（cook 漏包根因 + `DirectoriesToAlwaysCook` 修复 + 正确 IoStore 验证手段）。
- `shared/CODEBASE_MAP/ARCHITECTURE.md` / `README.md`：路由无变化，未修改。
