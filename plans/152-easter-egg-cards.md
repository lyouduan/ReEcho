# Plan 152 - 程序 - 彩蛋卡牌投放、效果与图标

## 协调

- Planner 负责人：Codex（程序路线，规划与执行合并）。
- Executor 负责人：Codex（程序路线，规划与执行合并）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@a742818b05a2b9a59d7f715fc16e2b2d55f4541d`。
- 本地实现方式（可选，仅作交接说明）：专属 worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-easter-egg-cards`，任务分支 `feature/easter-egg-cards`；正式实现前合入本 Plan 发布后的最新 `origin/main`。
- 依赖 / 阻塞：用户已确认 `构筑体系G!70:78` 九张彩蛋卡程序说明、九张 icon 的顺序、逐槽 1% 判定、三级卡底和四项随机规则；无产品阻塞。最终构建/自动化需要 Unreal Editor 已保存并关闭。
- Writes:
  - `plans/152-easter-egg-cards.md`
  - `Design/Data/ReEchoData.xlsx`
  - `Content/Data/cards.csv`
  - `Content/Data/card_effects.csv`
  - 与上述生成包同一事务实际变化的 `Content/Data/data_manifest.csv` / 数据修订文件
  - `Source/ReEchoCards/Public/Cards/ReEchoCardTypes.h`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardRuntime.h`
  - `Source/ReEchoCards/Private/Cards/ReEchoCardCatalog.cpp`
  - `Source/ReEchoCards/Private/Cards/ReEchoCardRuntime.cpp`
  - `Source/ReEchoCards/Private/Tests/`
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Source/ReEcho/Public/Run/ReEchoRunSaveGame.h`（仅在新增持久状态需要版本迁移时）
  - `Source/ReEcho/Public/ReEchoGameMode.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Public/Graybox/ReEchoEchoActor.h`
  - `Source/ReEcho/Private/Graybox/ReEchoEchoActor.cpp`
  - `Source/ReEcho/Public/UI/ReEchoTraitCardChoiceWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoTraitCardChoiceWidget.cpp`
  - `Source/ReEcho/Private/Data/ReEchoCsvDataRegistry.cpp`
  - `Source/ReEcho/Private/Tests/`
  - `Source/ReEchoCombat/Public/Combat/` 与 `Source/ReEchoCombat/Private/Combat/` 中实现“当前生命精确调整”或最终物理伤害契约所必需的窄接口及聚焦测试
  - `Content/SourceArt/UI/Cards/Icon/T_UI_CardIcon_G_4_1.png` 至 `T_UI_CardIcon_G_4_9.png`
  - `Content/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_G_4_1.uasset` 至 `T_UI_CardIcon_G_4_9.uasset`
  - `scripts/ue/` 下本 Plan 的图标导入/审计脚本
  - `Content/SourceArt/UI/Cards/README.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCombat.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
- Stable Reads:
  - `策划数据源/【开普勒】回响数值与构筑体系.xlsx` 的可见 `构筑体系G!A70:H78` 与 `投放系统!A1:D10`；程序说明列优先于展示描述列
  - 用户依次交付的九张 `codex-clipboard-*.png` 原图
  - `Content/Data/card_drop_levels.csv`、`Content/Data/shop_refresh_rules.csv`
  - 现有卡牌、商店卡组、关末免费三选一、逐槽刷新、构筑浮窗、伤害、Echo、状态与存档契约
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：普通一级/二级/三级卡池、智者周期额外选择、商店付款/领取、逐槽刷新次数与价格、已有卡牌存档继续工作；彩蛋卡不进入任何普通等级池，也不被“命运跃迁”/随机赠卡行为获得。旧存档不伪造历史彩蛋判定，缺失的新累计状态按零值迁移。
- 明确排除：不制作新的彩蛋卡专用卡底、不改三级卡底美术、不改九张 icon 的画面内容、不改普通卡牌数值、不让 UI 解释卡牌中文描述、不把彩蛋卡加入武器/符文商店商品槽。

## 锁定目标

1. 把可见 `构筑体系G` 第 70～78 行作为九张正式彩蛋卡接入生产 XLSX/CSV、类型化运行时、存档和构筑展示；稳定 ID 固定为 `G_4_1`～`G_4_9`，程序行为以程序说明列为准。
2. 彩蛋卡独立于一级、二级、三级普通牌池。每次生成关末免费卡牌组三个槽位、商店已投放卡牌组三个槽位，以及对其中单个槽位执行刷新时，每个实际生成槽各自独立进行一次 1% 彩蛋判定；命中后该槽从仍合法、未拥有且本组未展示过的彩蛋卡中等权无放回选择，未命中或彩蛋池耗尽时继续使用原等级普通池。
3. 彩蛋随机由本局以真实时间初始化并保存的 `RunSeed` 加稳定上下文/序列派生；同一已保存页面不因重复打开而重摇，读档后继续一致，新的本局不会固定复现旧局。刷新只重算所点槽位。
4. 九张彩蛋卡均遵循【独特】：拥有后不再出现；普通随机赠卡和 `Card.GrantTier` 排除彩蛋卡。彩蛋卡在卡面表现上复用三级卡底，但其玩法定义不属于三级池。
5. 九张卡按以下程序语义生效：
   - `G_4_1 策法爆培嘉`：每关结束时独立等概率选择 `floor(当前可用时间碎片 × 2.5)` 或 `floor(当前可用时间碎片 × 0.5)`，随后通过唯一货币入口增加 30；结果写入可保存实际效果。
   - `G_4_2 你觉得这个怎么样`：获得时对“暴击率 +30%”“物攻 +15”“当前生命调整为 3”各自独立做 50% 判定，允许全部未命中；一次事务原子提交并记录实际命中项。
   - `G_4_3 恋爱小脑`：Echo 每次新进入与敌人的接触时造成 12 物理伤害；每次新进入与玩家的接触时回复玩家 6 生命，同一连续重叠不逐帧重复。
   - `G_4_4 呕人？哕人？`：获得时失去全部当前可用时间碎片；每完整失去 15 点，等概率且可重复抽取一次 `生命+1 / 物攻+1 / 元攻+1 / 暴击率+5% / 暴击效果+10% / 回响效能+5% / 元素反应效能+5%`，余数只扣除不奖励；全部抽取在同一原子事务提交并记录汇总。
   - `G_4_5 帅气T傥`：遭遇进行中每 0.5 秒，从玩家身边 4m 内合法存活敌人中等权选 1 名施加 1 秒眩晕；没有目标时本次不生效。
   - `G_4_6 “野狗”的荣耀`：拥有后累计承受的最终实际伤害达到 55 时仅触发一次，从所有尚未获得、启用且合法的一至三级普通卡中等权无重复抽取并原子授予最多 5 张，明确排除彩蛋卡；不足 5 张时授予全部剩余合法卡并记录结果。
   - `G_4_7 绝不迟到之人！`：玩家侧（玩家与 Echo）造成的每次物理伤害在其他来源增益计算后、进入目标最终防御/生命裁决前，按 `15% / 35% / 40% / 8% / 2%` 最终改写为 `0 / 10 / 100 / 1000 / 10000`；每个命中独立判定且不修改元素伤害。
   - `G_4_8 【粥粥】之力`：仅统计遭遇进行期间所有正向获得的时间碎片毛收入，不扣除消费。关末将本关与上一已完成关比较：每多 5 点使下一关全部遭遇内正向碎片收入降低 1%，每少 5 点使下一关提高 5%，不足 5 的差值忽略，最终倍率最低为 0；首个完成关只建立基线。倍率和两关统计可保存且实际结果可查看。
   - `G_4_9 按时上课！`：每关结束永久增加最大生命 10、当前生命 10、元攻 1、物攻 1，沿既有受控属性/生命同步入口发布一次最终通知。
6. 九张用户交付 icon 按行顺序落为 `T_UI_CardIcon_G_4_1`～`T_UI_CardIcon_G_4_9`，导入为可 Cook 的 UI Texture2D；卡牌三选一、商店构筑槽和已拥有卡牌浮窗均按稳定 ID 自动解析。随机/累计类卡牌复用既有“实际效果”副浮窗展示已解析结果。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoCards` / `AREA-Cards`、`MOD-ReEcho` / `AREA-Data` / `AREA-Run` / `AREA-Recording`、`MOD-ReEchoCombat` / `AREA-AbilityCombat`、`AREA-UI`（文档入口 `MOD-ReEchoUI`）。
- 对应模块文档：`MOD-ReEchoCards.md`、`MOD-ReEcho.md`、`MOD-ReEchoCombat.md`、`MOD-ReEchoUI.md` 均已加入 `Writes`；不新增 Runtime Module。
- 设计意图：把“彩蛋池资格、逐槽概率、九种行为、随机结果和累计状态”保持为稳定 ID + 类型化数据/命令，不让 UI、描述文本或 Actor 各自复制概率与数值。Run 继续拥有卡牌组页面和本局真实时间种子；Cards 拥有目录、资格、纯行为与可保存状态；Combat 继续拥有最终伤害/生命；GameMode/Echo 只做世界适配。
- 权威状态与依赖：生产事实仍为 `Design/Data/ReEchoData.xlsx` 与同事务生成 CSV。新增 `EasterEgg` OfferGroup（或等价类型化分类）作为“玩法无等级”的权威分类；普通 Tier 仅决定原卡组。彩蛋的三级卡底通过只读展示字段投影，不把其伪装进三级玩法池。依赖方向不反转。
- 决策记录：
  - 采用“每个生成槽独立 1%”而非“每组最多一张”，因为用户已明确各自独立判定；同一组三槽理论上可命中多张不同彩蛋卡。
  - 单槽刷新重新进行该槽 1% 判定，同时沿用本组历史排除；重复打开页面不判定。
  - 使用本局真实时间初始化且可保存的 `RunSeed` 派生所有随机，禁止 `FMath::Rand`、固定测试种子进入生产默认路径或由 UI 重摇。
  - `G_4_2` 三项各 50% 且允许零项；`G_4_6` 只取未获得普通卡；`G_4_8` 统计毛收入，均采用用户本轮确认。
  - 数据写入以最新 `origin/main` 的 `Design/Data/ReEchoData.xlsx` 为基线。`b5751247` 同步了同一生产工作簿中的怪物/投放数据，虽无卡牌逻辑冲突但属于二进制共享写热点；禁止用旧工作簿整块覆盖，必须在最新表上增量写入 Cards/CardEffects 并通过 `--check` 验证完整生成包。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：关闭前审阅；预计模块拓扑不变，无事实变化则只在执行记录写“无需修改”。
  - `shared/CODEBASE_MAP/README.md`：关闭前审阅；预计稳定路由不变，无事实变化则只记录“无需修改”。
  - 四份上述模块文档：同步彩蛋池分类、随机/状态契约、世界/Combat适配、三级卡底与 icon 路径。
- 关闭前逐项填写审阅结果：执行记录中逐份记录“已更新”或“已审阅、无需修改”。

## 锁定验收

- [x] 生产数据包含且只包含九张启用彩蛋卡 `G_4_1`～`G_4_9`，程序说明数值、`EasterEgg` 分类、【独特】和九个 Behavior 均被 Python 与 C++ 读取器验证；普通 Tier 计数不改变。
- [x] 关末免费组、商店卡组的三个初始槽和两类单槽刷新分别覆盖独立 1% 判定；自动化用注入/穷举种子证明命中、未命中、多槽命中、彩蛋池耗尽、已拥有/本组历史排除、读档/重复打开稳定。
- [x] `Card.GrantTier`、`G_4_6` 及其他随机赠卡不会递归获得彩蛋卡；九张卡拥有后不再投放。
- [x] 九张效果各有纯规则或集成自动化，覆盖阈值/边界、随机概率表、原子失败、玩家/Echo来源、接触去重、关末顺序、毛收入统计和存读档。
- [x] 九张 icon 的源图、规范命名、Texture2D UI 属性、Cook 路径和卡面/构筑解析路径通过导入审计；彩蛋卡显示三级卡底且不显示为三级玩法池成员。
- [x] 所有会产生具体随机/累计结果的彩蛋卡在已拥有卡牌 Tooltip 的第二块“实际效果”面板中显示当前已解析结果。
- [x] C++ 格式化、聚焦自动化、`Build-Editor.cmd -Configuration Development`、数据 `--check`、`python scripts/validate_project.py` 与 `git diff --check` 通过；发布 main 前在最终组合候选执行 `-FullRebuild` 并刷新精选预构建包。
- [ ] PIE 人工验收至少覆盖：免费组/商店组命中彩蛋、逐槽刷新、三级卡底与九 icon、九张效果的可观察行为、保存退出后恢复。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@a742818b05a2b9a59d7f715fc16e2b2d55f4541d`；Plan 编号以该远端最大 `151` 后的首个空闲编号 `152` 分配。
- 引擎/构建可用性：实现前先运行 `python scripts/setup_lfs.py --check`；涉及图标导入、自动化或构建前确认 Editor 已关闭并取得 Git-common-dir Unreal 锁。
- 现有聚焦测试结果：Plan 编写阶段仅完成代码/数据只读审计，尚未执行实现基线构建；执行者在首个 C++ 变化前记录卡牌、商店、Combat/Echo 聚焦基线。
- 共享契约 / 难合并资源风险：`Design/Data/ReEchoData.xlsx` 是与 `b5751247` 的怪物/投放同步共享的二进制权威文件；必须以最新表增量编辑并与生成 CSV 整体发布。九张用户原图是外部输入，首次复制后不得覆盖其他卡牌 icon。
- 外部变化审计：`b5751247` 修改敌人/Encounter CSV、三个生产 XLSX 和数据测试，与本任务卡牌表写入存在同工作簿耦合但无已知玩法冲突；采用组合适配。`a742818b` 仅增加可追溯 Windows 打包入口，与本任务无路径或逻辑冲突。Plan-only 候选基于准确最新 main，不携带二者的重写。
- 基线损坏时的停止条件：LFS 指针/缺对象、生产 XLSX 与 CSV 在未改动时已漂移、九行中任一可见程序说明/ID变化、当前 `main` 出现新的 Card/Run/Combat 同契约修改且无法组合适配，或用户交付 icon 无法读取时停止受影响部分并报告。

## 实现提纲

1. 在最新生产 XLSX 中增量增加九张卡及效果行，扩展静态/运行时白名单和测试，事务生成 CSV，锁定“普通 Tier 数量不变、彩蛋独立分类”。
2. 抽出统一“为一个卡牌组槽生成候选”的 Run/Cards 接缝：先做独立 1% 彩蛋判定，再按分类池无放回抽取；接入关末、商店初始页和两类逐槽刷新，并保存所有候选/历史/随机上下文。
3. 从简单到复杂实现 `G_4_9`、`G_4_1`、`G_4_2`、`G_4_4`、`G_4_8` 的授予/关末/货币与实际结果，再实现 `G_4_6` 多卡原子授予。
4. 实现 `G_4_7` 最终物理伤害变换、`G_4_5` 周期范围眩晕、`G_4_3` Echo 接触伤害/治疗，并保持 Combat 与世界宿主权威边界。
5. 复制九张 icon 到规范 SourceArt，编写幂等导入/审计脚本并用 Editor 导入 Texture2D；让彩蛋候选显式投影三级卡底和现有 Tooltip 实际效果。
6. 补齐数据、Cards、Run/Shop、Combat、Echo、Save、UI/资产自动化，更新四份模块文档和 Plan 执行记录，完成最终构建与人工验收清单。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| LFS | `python scripts/setup_lfs.py --check` | 工作簿、PNG、UAsset 均为真实对象而非指针 |
| 数据 | `python scripts/data/sync_xlsx_to_csv.py --check` 及本 Plan 聚焦 Python 测试 | XLSX/CSV 无漂移，九卡精确集合/行为/分类正确，普通 Tier 数量不变 |
| 纯规则 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Cards` | 九卡授予、随机、累计、规则快照、原子性通过 |
| 投放/商店 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Trait` 与 `-Filter ReEcho.Shop` | 免费/商店/刷新逐槽 1%、排除与页面稳定通过 |
| Combat/Echo | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Combat` 与彩蛋聚焦过滤 | 固定物理伤害概率表、眩晕、Echo接触、承伤阈值通过 |
| Save | 存档聚焦自动化 | 新累计状态和候选页面可恢复，旧存档安全默认迁移 |
| 图标 | 本 Plan Editor Python 导入/审计脚本 | 九源图、九 Texture2D、UI 属性、路径与卡 ID 一一对应 |
| C++ | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 退出码为 0 |
| 发布构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终组合候选与精选预构建包指纹一致 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目/源码/文档不变量通过，无空白错误 |
| 人工 | PIE：免费/商店/刷新命中彩蛋并逐张验证可观察效果、图标、三级卡底、Tooltip、存读档 | 用户记录准确候选的通过/失败现象 |

## 执行记录

### 变化

- 生产 `ReEchoData.xlsx` / CSV 新增 `G_4_1`～`G_4_9` 与 27 条效果，运行时以 `EasterEgg` 独立牌池识别，不改变普通等级池；九张 icon 已按稳定 ID 导入 Texture2D。
- 免费组、商店卡组及其逐槽刷新统一走逐槽候选选择器：每槽独立 1%，页面缓存与真实时间初始化的 RunSeed 保证读档/重开稳定，刷新仅重摇目标槽。
- 九张卡的授予、关末、货币、承伤、最终物理伤害、Echo 接触、周期眩晕与实际效果记录均已接入；存档版本升至 v25 并提供旧档中性迁移。
- 彩蛋卡业务 Tier 保持为所在卡组等级，新增只读 `PresentationTier` 投影三级卡底，避免把彩蛋误归入三级玩法池。

### 证据

- 2026-08-28：用 `artifact-tool` 读取可见 `构筑体系G!A70:H78`，确认九张卡、程序说明、稳定源 ID 和【独特】；读取 `投放系统!A1:D10`，确认彩蛋卡独立牌池和任意卡牌组的 1% 规则。
- 2026-08-28：用户确认三个槽各自独立 1%，复用三级卡底；`G_4_2` 三项各 50% 且允许零项；`G_4_1` 向下取整；`G_4_6` 只抽未拥有普通卡并排除彩蛋；`G_4_8` 统计正向毛收入。
- 2026-08-28：`sync_xlsx_to_csv.py --check`、`validate_project.py`、`setup_lfs.py --check` 与 `git diff --check` 通过；九卡、27 效果及 XLSX/CSV 字节一致性通过。
- 2026-08-28：`ReEcho.Trait` 11/11、`ReEcho.Shop` 16/16、`ReEcho.Cards` 20/20、`ReEcho.Run.SaveSnapshot` 1/1 通过；SaveSnapshot 覆盖 v25 彩蛋持有/运行状态/商店缓存页及 v23 中性迁移。
- 2026-08-28：Development `-FullRebuild` 104/104 action 成功，精选预构建包已刷新（`source=16d37203dead`）。
- 2026-08-28：扩大回归发现两个不属于 Plan152 修改面的基线失败：敌人碎片投放测试的硬编码期望与当前生产数据不一致；`ElementReactionWorld` 的导电世界测试未形成伤害。其余 `ReEcho.Run` 18/20、`ReEcho.Combat` 12/13 通过，Plan152 聚焦套件无失败。
- 2026-08-28：发布候选已组合 `origin/main@bbf6c22e`；在组合结果上重新执行 Development `-FullRebuild`，119/119 action 成功，精选预构建包刷新为 `source=70317c57463b`。随后 `sync_xlsx_to_csv.py --check`、`validate_project.py`、`setup_lfs.py --check`、`git diff --check` 通过，`ReEcho.Cards` 20/20、`ReEcho.Trait` 11/11、`ReEcho.Shop` 16/16、`ReEcho.Run.SaveSnapshot` 1/1 全部通过。

### 剩余风险

- 发布候选已组合并验证 `origin/main@bbf6c22e`；推送阶段仍需按发布锁规则再次确认远端 main 未前移。
- 1% 人工复现不稳定；PIE 重点验证真实时间种子下页面稳定、刷新只影响单槽，以及九张效果的可观察表现。

### 人工验收结果/请求

- `PendingBeforeClose`：等待最终候选后由用户执行锁定验收中的 PIE 清单。

### 架构文档审阅结果

- `MOD-ReEchoCards.md`：已更新彩蛋分类、逐槽选择器、随机/累计状态与九行为归属。
- `MOD-ReEcho.md`：已更新 Run/Save/GameMode 的页面、货币、世界适配和 v25 迁移契约。
- `MOD-ReEchoCombat.md`：已更新最终物理伤害、生命精确调整、眩晕/接触/承伤接缝。
- `MOD-ReEchoUI.md`：已更新三级卡底投影、九 icon 与实际效果副浮窗。
- `ARCHITECTURE.md`、`CODEBASE_MAP/README.md`：已审阅；Runtime Module、依赖拓扑和稳定路由未变化，无需修改。
