# Plan 146 - 程序 - Blueprint 数据资产取代全部 CSV

## 协调

- Planner 负责人：Gavyn。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@00c376b5cd1828e21aa68edd2551e8772fb32b5f`。
- 本地实现方式（可选，仅作交接说明）：规划与执行由同一个 AI 完成；使用独立 `ReEcho-plan146-blueprint-data-authority` worktree。
- 依赖 / 阻塞：UE 5.8、Git LFS、可写入并保存 `.uasset` 的无头 Editor；创建资产前必须确认 UE 未占用并取得仓库 UE 锁。
- Writes:
  - `plans/146-blueprint-data-authority.md`
  - `Source/ReEcho/Public/Data/`、`Source/ReEcho/Private/Data/`
  - `Source/ReEcho/Private/ReEcho.cpp`
  - `Source/ReEchoAudio/Public/ReEchoAudioCatalog.h`、`Source/ReEchoAudio/Private/ReEchoAudioCatalog.cpp`
  - `Source/ReEchoAudio/Public/ReEchoAudioTypes.h`、`Source/ReEchoAudio/Private/ReEchoAudioService.cpp`
  - 直接消费数据注册表的 `Source/ReEcho/**` 与相关 Runtime Module 测试
  - `Content/ReEcho/DataAsset/Gameplay/`、`Content/ReEcho/DataAsset/Audio/`、必要的 Editor-only 导入记录资产
  - `Content/Data/`、仓库内全部已跟踪 `*.csv`
  - `scripts/data/`、`scripts/audio/`、`scripts/art/`、`scripts/ue/package_windows.py`、`scripts/validate_project.py`
  - `.gitattributes`、`Source/ReEcho/ReEcho.Build.cs`
  - `Content/Data/README.md`、相关 `Design/`、`docs/` 与源美术说明
  - `shared/ARCHITECTURE.md`
  - `shared/CODEBASE_MAP/README.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoAudio.md`
- Stable Reads:
  - 当前生产 CSV、XLSX 工作簿与生成脚本，仅作为一次性迁移输入和行为对照
  - 当前 `FReEchoCsvDataSnapshot`、各 CSV Reader、音频 Catalog、消费方和自动化测试
  - `shared/PROJECT_RULES.md`、`shared/PROGRAMMER_RULES.md`、`shared/PLANNER_RULES.md`、`shared/EXECUTOR_RULES.md`、`shared/GIT_RULES.md`
- 影响模式：`Exclusive`。
- 兼容承诺 / 下游操作：稳定 ID、数值、行顺序、引用关系、运行时查询结果、存档中的 ID 与 Shipping 行为保持兼容；策划以后只在 UE 数据资产/Blueprint 默认值中配置并保存，不再运行 XLSX 到 CSV 同步。迁移提交不保留 CSV 运行时回退。
- 明确排除：不重新设计平衡数值、卡牌/武器/敌人/遭遇/音频语义，不借迁移修复未确认玩法问题，不保留 JSON/CSV/隐藏生成文件作为第二权威，不要求策划编译 C++。

## 锁定目标

- 将当前全部 34 张生产 CSV 的数据完整迁移为类型化 UE 数据资产；资产字段可在 UE 编辑器中直接查看、修改和保存，修改后重启 PIE/项目即可生效。
- 运行时只从已 Cook 的 UE 数据资产原子加载并发布快照，不再按物理路径读取 CSV；Shipping 不再复制松散 CSV，也不会因缺少 CSV 启动失败。
- 仓库中全部已跟踪 `.csv` 清零，包括生产表、CSV 测试夹具、美术/音频/导入清单。自动化负例改为内存构造或 UE 测试资产；历史导入映射改为 UE 资产或非表格化说明，不制造换后缀的重复数据源。
- 删除 CSV 解析器、manifest/schema、XLSX→CSV 同步和仅为 CSV 投递存在的脚本/构建依赖；保留仍有独立价值的 XLSX 时必须明确降级为非权威归档且不参与运行时或门禁。
- 更新规则校验和架构文档，使“UE 数据资产是唯一运行时配置权威、仓库不得新增 CSV”成为可自动验证的事实。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoAudio`、`AREA-Tests`；Cards、Combat、Enemies、Weapons 继续消费主模块发布的只读值契约，模块依赖方向不反转。
- 对应模块文档：更新 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `MOD-ReEchoAudio.md`，两者均已加入 `Writes`；审阅其他 Runtime Module 文档，若公共消费契约未改变则记录无需修改。
- 设计意图：把面向策划的配置所有权放入 UE 资产系统，利用 Cook、反射、编辑器属性面板和软引用，而运行时仍使用不可变、经验证后原子发布的快照，避免 UI/玩法直接持有可写资产。
- 权威状态与依赖：新增类型化 `UDataAsset` 作者层作为唯一配置权威；注册表负责把资产编译为现有只读快照并验证 ID、范围、唯一性和跨表引用。消费方继续读取快照，不直接依赖 Blueprint 类或资产编辑状态。音频模块拥有自己的类型化音频资产，避免 ReEchoAudio 反向依赖主模块。
- 决策记录：采用类型化 DataAsset/Blueprint 可编辑属性，不采用 `UDataTable`、通用键值字符串或运行时 CSV 兼容层。DataAsset 能表达嵌套一对多关系、枚举、软对象引用和分组编辑，并避免策划在文本单元格编码逻辑；代价是二进制资产合并冲突，需要按领域拆分资产并用只读审计脚本提供确定性摘要。
- 相关文档同步范围：更新 `shared/ARCHITECTURE.md`、`shared/CODEBASE_MAP/README.md`、`MOD-ReEcho.md`、`MOD-ReEchoAudio.md`、数据使用说明、策划工作流和所有仍引用 CSV 的相关说明；清理 Shipping 松散 CSV 契约。
- 关闭前逐项填写审阅结果：
  - `MOD-ReEcho.md` 已更新：待实现后填写数据资产、验证和快照编译边界；
  - `MOD-ReEchoAudio.md` 已更新：待实现后填写音频资产与预加载边界；
  - `ARCHITECTURE.md` / `CODEBASE_MAP/README.md` 已更新：待实现后填写唯一权威与阅读路线；
  - 其余模块文档：待差异完成后逐项审阅并记录结论。

## 锁定验收

- [ ] `git ls-files '*.csv'` 无输出，工作区递归检查也不存在项目 CSV。
- [ ] 34 张生产表的全部有效数据、稳定 ID、顺序和引用迁移到类型化 UE 资产；迁移前后确定性摘要一致。
- [ ] 策划能在 UE 编辑器内找到并编辑角色、卡牌、元素/状态/反应、武器/配件、敌人/Boss、遭遇/出生、商店、属性、运行时 smoke 和音频事件配置。
- [ ] 修改一个代表性数值并重新打开 PIE/项目后运行时快照体现修改，无需执行生成脚本或编译 C++。
- [ ] 启动、Editor 自动化与 Shipping 不读取或暂存松散 CSV；删除 CSV 后完整构建、Cook、打包和 10 秒冒烟测试通过。
- [ ] 生产数据完整性、重复 ID、范围、注册行为、跨表引用、武器/元素/遭遇/音频契约仍由自动化覆盖；负例不依赖 CSV 文件。
- [ ] XLSX 同步、CSV parser、manifest/schema 和打包复制入口已删除或退役，校验脚本禁止重新引入 `.csv`。
- [ ] `python scripts/validate_project.py`、`git diff --check`、完整 Editor FullRebuild 与受影响自动化通过。
- [ ] 用户在 Blueprint/DataAsset 编辑器中完成人工可编辑性与代表性改值验收后关闭。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@00c376b5cd1828e21aa68edd2551e8772fb32b5f`；任务 worktree 创建时干净。
- 引擎/构建可用性：Plan 发布后执行 LFS、UE 锁、基线静态校验与 Editor 构建；资产生成必须使用可重复 UE 脚本并保存确定性审计输出。
- 现有聚焦测试结果：迁移前运行数据注册表、音频和遭遇/武器/卡牌相关自动化，记录基线；任何非本任务基线失败先隔离。
- 共享契约 / 难合并资源风险：数据资产为二进制且覆盖多个领域；按领域拆分、从最新 main 生成并在发布前审计远端新资产/数据变化。任何迁移期间发生的平衡数据更新必须先合入迁移输入再重新生成，不能静默覆盖。
- 基线损坏时的停止条件：LFS 指针异常、UE 无法加载当前数据、基线数据测试失败、迁移摘要不一致、UE 资产无法无头保存，或远端出现无法自动组合的数据逻辑冲突。

## 实现提纲

1. 固化迁移前所有生产表的规范化摘要、行数、顺序、ID 和引用；枚举全部 CSV 消费方、测试夹具与导入清单。
2. 为主模块和音频模块建立 BlueprintType 行结构、按领域拆分的 DataAsset、资产设置入口和资产到不可变快照的验证/编译路径。
3. 用一次性 UE 迁移脚本从当前 CSV 生成并保存生产 DataAsset，读取资产回写确定性摘要，与迁移前数据逐表比较。
4. 切换启动、运行时、音频、测试和打包链路到资产；将负例重写为内存变体，迁移或删除非运行时 CSV 清单及旧同步工具。
5. 删除仓库全部 CSV、CSV reader/manifest/schema/RuntimeDependencies/复制逻辑，更新静态校验、文档和策划入口。
6. 完成 FullRebuild、完整相关自动化、资产加载审计、Shipping Cook/包体无 CSV 检查和代表性人工改值入口。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| CSV 清零 | `git ls-files '*.csv'` 与递归文件检查 | 仓库和成品包均无 CSV |
| 迁移一致性 | 一次性迁移摘要 + DataAsset 只读审计 | 逐领域行数、稳定 ID、顺序、关键数值和引用一致 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 新权威、源码和文档不变量通过 |
| C++ 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 退出码为 0，精选预构建包匹配源码 |
| 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | 数据、音频、卡牌、武器、敌人、遭遇、商店和相邻契约通过 |
| 资产审计 | UE 命令行加载所有新 DataAsset 并运行验证 | 资产可加载、字段可反射、软引用和跨表引用有效 |
| Shipping | `python scripts/ue/package_windows.py --output <path>` | BuildCookRun、无 CSV 包体检查和 10 秒冒烟测试通过 |
| 人工 UE | 编辑代表性资产数值，重启 PIE/项目 | 无编译和同步脚本即可观察到改值 |

## 执行记录

### 变化

- 2026-08-28：用户要求将现有全部 CSV 搬到 Blueprint 中供策划直接配置，并删除全部 CSV。
- 2026-08-28：盘点到 34 张生产 CSV、20 张 CSV 测试夹具及 7 张美术/导入清单，共 61 个已跟踪 CSV；全部纳入清零边界。

### 证据

- 规划基线：`origin/main@00c376b5cd1828e21aa68edd2551e8772fb32b5f`。
- 当前主注册表在模块启动时通过物理路径读取并原子发布 `FReEchoCsvDataSnapshot`；音频模块另行解析 `audio_events.csv`；Shipping 脚本显式复制松散 CSV。

### 剩余风险

- 二进制 DataAsset 不适合文本合并，必须按领域拆分并强化远端变化审计。
- 现有 410 行 schema 和大量负例承载了重要验证知识，删除文件前必须把约束迁移为 C++/Python/资产验证测试，不能只删除测试。
- 策划电脑不能编译 C++，最终 main 必须提交与源码匹配的精选 Editor 预构建包。

### 人工验收结果/请求

- `PendingBeforeClose`：实现候选完成后，请用户在 UE 中验证资产可发现性、字段可编辑性和代表性改值即时生效。

### 架构文档审阅结果

- 待实现后填写。
