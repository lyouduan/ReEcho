# Plan 111 - 程序 - 开普勒可见卡牌与符文规则更新

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner）。
- Executor 负责人：Codex（规划与执行合并；用户已确认执行本 Plan）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`；27 张新卡、既有卡牌数值变化、符文平衡和组合机制需要用户在 PIE/Shipping 候选中验收。
- 本地规划 / 实现基线：`origin/main@49dc312facaf1be002ef6919aceb8e4445b2b08f`。
- 本地实现方式：一任务一 worktree；`plan/111-kepler-visible-update`，`C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan111-kepler-visible-update`。
- 依赖 / 阻塞：
  - 依赖 Plan47/88/97/101/105/108 的卡牌目录、逐级投放、卡包预付费、逐槽刷新、实际结果浮窗和存档契约；
  - 依赖 Plan76/104 的可见符文运行时、四武器生产集合、统一 `DamageCoefficient` 和核心伤害通道；
  - `诅咒银行` 的赊账/还款规则、`连接，连接！` 的伤害频率、`我们，合体！` 的距离曲线、`左右开弓` 的“伤害来源”粒度尚需策划/用户在对应复杂批次开始前确认；这些问题不阻塞前两个简单数据阶段。
- Writes:
  - `plans/111-kepler-visible-card-rune-update.md`
  - `策划数据源/【开普勒】回响数值与构筑体系.xlsx`
  - `Design/Data/ReEchoData.xlsx`
  - `Content/Data/{cards,card_effects,card_offer_rules,card_conflicts,parts,part_effects,statuses,shop_drop_levels,csv_schema,reecho_data_manifest}.csv` 中实际需要创建或更新的文件
  - `scripts/data/sync_xlsx_to_csv.py` 及本 Plan 聚焦数据测试
  - `scripts/validate_project.py`
  - `Source/ReEchoCards/{Public,Private}/Cards/` 与 `Source/ReEchoCards/Private/Tests/`
  - `Source/ReEchoCombat/{Public,Private}/` 与 `Source/ReEchoCombat/Private/Tests/`
  - `Source/ReEchoWeapons/{Public,Private}/` 与 `Source/ReEchoWeapons/Private/Tests/`（仅新卡所需稳定攻击来源/武器使用事件；不复制卡牌逻辑）
  - `Source/ReEchoEnemies/{Public,Private}/` 与 `Source/ReEchoEnemies/Private/Tests/`（仅稳定敌人类别/状态事件适配）
  - `Source/ReEcho/{Public,Private}/{Data,Run,Combat,Weapons,Graybox,UI,Tests}/`
  - `shared/CODEBASE_MAP/modules/{MOD-ReEcho,MOD-ReEchoCards,MOD-ReEchoCombat,MOD-ReEchoWeapons,MOD-ReEchoEnemies,MOD-ReEchoUI}.md`
  - 最终 FullRebuild 刷新的 `Binaries/Win64/` 精选预构建包
- Stable Reads:
  - `C:\Users\gavynqiu\Documents\miniGame\【开普勒】回响数值与构筑体系.xlsx`
  - `C:\Users\gavynqiu\Documents\miniGame\附件下载_【开普勒】回响数值与构筑体系\`
  - `plans/{47-card-build-runtime-module,76-weapon-rune-completion,88-card-drop-system,97-tiered-shop-card-pack-choice,101-separated-shop-card-pack-refresh,104-four-weapon-roster,105-card-outcome-tooltips,108-shop-card-pack-prepay}.md`
  - `shared/CODEBASE_MAP/{ARCHITECTURE,README}.md`
- 影响模式：`SharedContract`。同时改变卡牌目录、卡牌投放资格、跨领域玩法事件、Combat 状态、Run/Shop/Save、符文平衡和生产 XLSX/CSV；与卡牌、商店、武器、存档和预构建包并行工作均存在集成耦合。
- 兼容承诺 / 下游操作：
  - 运行时 `CardId`、现有 SaveGame、四武器 `WeaponId`、正式 `PartId` 和现有卡牌实际结果保持兼容；策划源 ID 与运行时稳定 ID 通过显式映射关联，不直接覆盖。
  - 最终启用构筑卡为 64 张，Tier 分布固定为 `8 / 32 / 24`；旧禁用卡 `碎时锋芒` 不进入投放，`碎时壁垒` 的稳定运行时 ID 可迁移为 `花钱消灾`。
  - 继续只保留长剑、镰刀、弓、枪四把生产武器；不恢复匕首、法杖、鞭。
  - `投掷召回链`、`群攻陨星剑刃` 和现有退役/占位符文保持禁用，除非用户另行明确改变产品决定。
  - 新卡没有正式独立 icon 时使用现有通用 fallback；不把策划表中的人物证件照占位图导入生产资源。
- 明确排除：
  - 所有 Excel hidden 行/列及其内容；当前已知包括 `武器体系W` 第 2 行、`武器符文C` 第 8-15 行和第 57-77 行。
  - hidden 中的匕首、匕首符文、法杖/鞭符文，以及任何依赖 hidden 文本才能成立的机制。
  - 新卡正式美术、卡面重绘、UI 整体换皮、武器/符文 VFX 重做。
  - 重新启用已被 Plan104/105 退役的武器或符文、重用退役稳定 ID、解析中文描述驱动玩法。

## 锁定目标

1. 把临时开普勒表的**可见差异**迁移到仓库策划源和生产 `ReEchoData.xlsx`；同步器、CSV、运行时和 UI 只消费类型化生产数据，hidden 内容视为不存在。
2. 先落实零风险改名和文案，再落实已有机制的配表数值、投放规则与符文平衡，最后分批实现 27 张新增卡牌，避免一次跨越全部系统。
3. 保留现有运行时稳定 `CardId`：已有卡按名称建立新的 `SourceWorkbookId → CardId` 一对一映射；新增二级卡分配追加式运行时 ID，`花钱消灾` 复用旧禁用 `碎时壁垒` 的稳定 ID，其余新增三级卡分配追加式 ID。不得让新策划源的 `G_2_14` 等 ID覆盖现有同号运行时卡。
4. 将新表 H 列投放语义转为正式数据：角色卡不进入构筑目录；1级普通卡可重复；标记【独特】的卡获得后不再出现；指定关卡限制和成对排斥由类型化表/字段表达，免费投放、商店卡包、命运跃迁和逐槽刷新共用同一资格判断。
5. 更新两张已有卡牌实际效果、24 行可见符文程序字段及第5关商店投放；程序说明是行为权威，用户说明同步为同一数值，不保留自相矛盾文案。
6. 新增 `Z_Cursed` 的正式 Combat 状态和27张新卡所需的类型化卡牌状态/事件；所有随机、跨关进度、债务、武器/核心历史和实际结算结果可存档、可恢复、可在已有“实际效果”浮窗展示。
7. 每张新增卡至少拥有数据校验、纯规则或领域聚焦自动化和一条 GM/PIE 可观察路径；组合卡还要覆盖组合前、部分组合、完整组合及存读档。

## 架构影响与设计决策

- 受影响架构标识：
  - `MOD-ReEcho`：`AREA-Data`、`AREA-Run`、`AREA-Cards`、`AREA-AbilityCombat`、`AREA-Weapons`、`AREA-UI`、`AREA-Tests`；
  - `MOD-ReEchoCards`：卡牌目录、投放资格、构筑状态、跨关结果和声明式规则；
  - `MOD-ReEchoCombat`：诅咒、溢出伤害/治疗、负面状态成功事件、临时超额生命和伤害来源快照；
  - `MOD-ReEchoWeapons`：稳定武器使用记录和攻击来源身份，不拥有卡牌状态；
  - `MOD-ReEchoEnemies`：稳定敌人类别与受状态结果，不拥有卡牌阈值；
  - `MOD-ReEchoUI`：只读显示新增卡牌、商店债务/免费状态和 `OutcomeText`，不执行玩法。
- 对应模块文档：维护 `MOD-ReEcho.md`、`MOD-ReEchoCards.md`、`MOD-ReEchoCombat.md`、`MOD-ReEchoWeapons.md`、`MOD-ReEchoEnemies.md`、`MOD-ReEchoUI.md`，均已加入 Writes；关闭前审阅 `ARCHITECTURE.md` 和 `README.md`。
- 设计意图：延续 `XLSX → CSV → 类型化目录/状态 → 领域权威执行 → UI只读投影`。新增卡牌可以扩展类型化事件和结果，但不得把自由文本、具体中文卡名或 UI 控件变成规则入口。
- 权威状态与依赖：
  - Cards 拥有卡牌定义、持有/排斥/投放资格、卡牌进度、确定性随机和 `ResolvedOutcomes`；
  - Run 拥有货币、债务事务、商店免费/刷新状态、武器/符文背包、整局武器/核心历史和 Save IO；
  - Combat 独占最终伤害、生命、治疗、元素、状态和死亡；Cards 只消费最终结果并返回声明式修正；
  - Weapons 独占攻击 Commit 与来源身份；Enemies 独占敌人类型；二者只发布稳定值事件，不读取 CardId；
  - UI 只消费 Run/Cards 投影并发送受控命令。
- 决策记录：
  1. **hidden 即废弃：**解析、同步、审计计数和验收均先过滤 hidden 行列；不会因为 hidden 内容仍存在于二进制工作簿而恢复旧玩法。
  2. **运行时 ID 不跟策划源重编号：**现有 `CardId` 是存档和资源契约；`SourceWorkbookId` 改为当前可见策划源的一对一审计键。新增二级卡使用 `G_2_18..G_2_36`，`花钱消灾` 使用既有禁用 `G_3_20`，其余新增三级卡使用 `G_3_23..G_3_29`；旧 `碎时锋芒` 保持禁用且不占用当前策划源键。
  3. **投放逻辑数据化：**新增 `card_offer_rules`（允许关次/直接授予/投放资格）和 `card_conflicts`（规范化无序卡牌对）或等价生产表；Catalog 编译后由所有投放入口统一消费，不解析 H 列文字。
  4. **确定性与存档：**随机二选一、下一反应、随机目标、任务进度、债务利息和商店免费状态使用本局种子与单调序列；为新增持久状态提升 SaveVersion，并为旧存档提供无增益、无债务的确定性默认迁移。
  5. **图标不阻塞逻辑：**125 张既有图继续沿用；新增27张卡暂用通用 fallback。策划表共用的人物证件照仅作为策划占位，不进入 SourceArt、UAsset 或 Shipping。
  6. **四武器契约优先：**新策划表删除法杖/鞭可见武器行与当前生产四武器一致；hidden 匕首和 hidden 法杖/鞭符文不参与任何同步。稳定四武器、Plan104 的 DamageChannel 和 Plan105 的符文禁用结果保持不变。
- 需要在复杂批次前确认的产品细节：
  1. `诅咒银行`：可赊额度、购买时先花余额还是直接记债、主动还款入口、10%利息舍入方式、离开商店能否还款；
  2. `连接，连接！`：伤害结算频率、同一敌人的重复命中冷却、多回响时连接对象；
  3. `我们，合体！`：获得100%增幅的距离、零增幅距离及中间插值曲线；
  4. `左右开弓`：来源按“玩家/所有回响”两类，还是按每个独立攻击者/武器/投射物区分；
  5. `武器大师`：“使用过”按装备成功、攻击成功还是造成伤害计数。
- 相关文档同步范围：必审 `ARCHITECTURE.md`、`README.md` 和上述六份模块文档；只有拓扑/依赖或索引变化时修改全局文档，模块事实变化必须同步模块文档。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅拓扑与跨模块不变量；
  - `README.md`：待审阅 `AREA-*` 路由；
  - 六份 `MOD-*`：待按最终状态所有者、数据表、事件、测试和代码位置更新或记录无需修改原因。

## 锁定验收

- [ ] 仓库策划源准确吸收临时表的可见变化；任何 hidden 行列均未进入生产 XLSX、CSV、运行时目录、测试精确集合或 UI。
- [ ] 最终生产构筑卡为64张启用卡，Tier精确为 `8 / 32 / 24`；每张可追溯到唯一策划源 ID，同时现有运行时 `CardId` 和旧存档不串卡。
- [ ] 四个改名、万象回春、殉身回响、24行可见符文程序字段和第5关商店 `1|3` 投放均与新表一致；程序/用户描述不再冲突。
- [ ] 投放资格统一覆盖免费三选一、商店三个卡包、命运跃迁和逐槽刷新；【独特】、关次限制、排斥关系和已拥有排除规则均有正反测试。
- [ ] `Z_Cursed` 对非 Boss 的下一次有效伤害触发直接死亡，对 Boss 无效；状态成功/失败、消费和存档行为有自动化证据。
- [ ] 27张新增卡逐张拥有有效 Effect/Behavior、类型化状态、自动化证据和GM可观察路径；未注册行为、缺失映射或不完整跨域接线会被数据校验拒绝。
- [ ] 新增卡牌随机、任务进度、债务、商店状态、核心/武器历史、反应记录、组合完成度和实际结果在保存/继续后保持一致；旧存档迁移不凭空授予收益。
- [ ] 商店和卡牌 Tooltip/实际结果浮窗能展示债务、随机选择、任务进度、累计属性和组合完成结果；UI 不解析卡牌说明或直接写玩法状态。
- [ ] 长剑/镰刀/弓/枪及现有符文组合回归通过；法杖、鞭、hidden 匕首及hidden符文未恢复，两枚产品禁用符文仍不可投放。
- [ ] 工作簿/CSV同步检查、聚焦测试、项目校验、`.clang-format`、Development FullRebuild、精选预构建检查和 `git diff --check` 通过。
- [ ] 用户完成分阶段PIE验收：已有卡/符文平衡、新卡单卡、经济/状态链、Echo组合和存读档；未通过前保持 `PendingBeforeClose`。

## Step 0 门禁

- 基线分支/提交：`origin/main@49dc312facaf1be002ef6919aceb8e4445b2b08f`；实现前再次 fetch，若 main 前进则按外部提交集成审计报告并等待用户选择。
- 引擎/构建可用性：规划阶段不启动 Editor；实现需要 C++/XLSX 时，先确认 Editor 已保存并关闭，再取得 Git common-dir Unreal 锁。
- 现有聚焦测试结果：尚未为本 Plan 建立可复用基线证据；实现开始先运行数据同步 `--check`、项目校验及 Cards/Run/Shop/Combat/Weapons 聚焦基线。
- 共享契约 / 难合并资源风险：`ReEchoData.xlsx`、Cards 类型、RunSubsystem/SaveGame、Combat结果、商店、多个模块文档和精选预构建包均为高耦合路径；各大阶段应形成独立可验证提交，避免把27张卡一次性堆成不可审查二进制/代码差异。
- 基线损坏时的停止条件：
  - 可见行过滤结果不是卡牌68行（含4角色卡）或符文可见范围与Plan76不一致；
  - 生产稳定ID无法无损映射到新策划源；
  - 同步器产生hidden/退役武器符文或覆盖非本任务工作簿变化；
  - 上述5项产品细节在对应复杂批次开始时仍未确认；
  - 新机制要求Combat/Weapons/Enemies反向依赖Cards，或必须解析中文描述才能执行。

## 实现提纲

按风险从低到高分七个大步骤执行；每步完成后先验证，再进入下一步。

### 1. 改名、文案与可见源表接入

1. 只读审计临时表可见行列，把可见差异更新进仓库同名策划源；hidden 内容不比较、不迁移、不删除依赖。
2. 建立完整 `SourceWorkbookId → 稳定 CardId` 映射，先修正已有卡映射，再分配新增卡运行时ID；添加一对一、无碰撞校验。
3. 完成四个低风险改名：`元素暴击→连携暴击`、`替身余响→我的替身`、`时砂豪赌→梭哈是智慧`、`复调机括→双重武器槽`；保留原运行时ID、拥有状态、图标和行为。
4. 同步程序/用户文案，明确“武器配件”统一称“武器符文”；新卡图先使用通用fallback，不导入证件照。

### 2. 已有机制的纯配表与投放规则

1. 更新万象回春、殉身回响的参数和测试，不新增第二套行为分支。
2. 更新24行可见符文程序字段：数值、阈值、概率、碎片数、范围、伤害倍率和叠层上限；纯文字变化只同步描述。对“可以叠加”按无策划上限执行，但仍受5秒层期限和数值类型安全约束。
3. 第5关商店卡包从不投放改为投放1级、3级；保持三个固定Tier入口和Plan108预付费流程。
4. 新增并编译类型化关次限制、【独特】和排斥配置；统一替换免费/商店/命运跃迁/刷新中的散落过滤，先覆盖已有卡再接新卡。

### 3. 新卡共享底座、状态、事件和存档

1. 在生产XLSX/CSV加入27张新卡定义、效果行、投放规则和冲突对；Catalog必须拒绝缺失行为、重复源ID、非法关次和无效冲突引用。
2. 在Combat实现 `Z_Cursed`、负面状态成功事件、溢出伤害、治疗/超额生命和稳定伤害来源快照；Boss免疫由状态应用/消费权威判断，不在卡牌代码按Actor类猜测。
3. 在Run/Cards增加债务、任务、免费商店、无限刷新、武器/核心历史、反应记录、敌人类别击杀和Echo组合的最小类型化状态；提升SaveVersion并完成旧存档迁移。
4. 为所有持久/随机结果接入 `ResolvedOutcomes` 与Shipping可用的统一玩法审计日志；UI只读投影先能显示占位状态。

### 4. 第一批低耦合新卡

先实现能复用现有授予、库存和商店事务的8张：

- `孤注二掷`：确定性随机一项翻倍、另一项减半；
- `重启任务`：移除全部已拥有/已装备武器符文，获得300碎片和5次商店免费刷新；
- `样样都通`：一次性复合属性授予；
- `花钱消灾`：复用旧碎时壁垒的受伤前扣5碎片、伤害-1事务并重新启用稳定ID；
- `喂，打劫！`：下一次完整商店访问免费，离店后消费；
- `就要那个！`：无限刷新状态持续到下一次成功购买；
- `龙魂，集齐！`：以已拥有六种核心为完成条件，声明式授予三项50%增益；
- `武器大师`：按确认后的“使用过”事件记录不同四武器并幂等重算永久收益。

这一阶段不引入空间持续碰撞或逐目标历史。

### 5. 第二批事件、经济与进度新卡

依次实现13张需要Combat/Run事件但不需要复杂空间几何的卡：

- `数字挑战`、`诅咒银行`；
- `德古拉第I课`、`德古拉第II课`；
- `损人利己`、`烈火之心`、`水火不容`、`雷鸣之心`、`生命虹吸`；
- `史莱姆杀手`、`兔兔杀手`；
- `自我赛跑`、`回归基本功`。

每张卡只消费最终领域事件：最终击杀、最终过量伤害、成功状态应用、最终元素反应、最终治疗、Encounter结束快照等；不得从碰撞回调、UI或描述文本推导结果。债务、任务、跨关比较和记录反应必须覆盖存读档与实际结果浮窗。

### 6. 第三批Echo组合与空间复杂卡

最后实现6张高耦合机制：

- `我来组成头部`、`我来组成身体`、`我来组成腿部`及三件套升级；
- `连接，连接！`；
- `我们，合体！`；
- `左右开弓`。

空间效果使用当前世界Player/Echo/Enemy位置和Combat最终结算；Cards只发布规则快照。完整组合必须覆盖0/1/2/3件、多个Echo、Echo缺失、角色死亡、关卡切换、暂停、存档恢复和效果移除；不得把视觉线条或碰撞表现当成伤害权威。

### 7. 全链集成、UI、GM与发布候选

1. 让商店、免费选卡、卡包预付费/继续选择、逐槽刷新、Tooltip和实际结果面板覆盖全部64张卡；候选不足时安全降级且不重复已有独特卡。
2. 增加表驱动GM验收入口：按稳定运行时ID授予卡牌、开始/结束关卡、查看卡牌状态/债务/任务/组合/实际结果；所有按钮和命令继续进入现有统一审计日志。
3. 执行逐卡测试矩阵、关键组合、八关投放、存读档、四武器/符文回归和全模块文档同步；最后完成FullRebuild、精选预构建包和人工验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 策划源可见性 | artifact-tool + OpenXML hidden审计，渲染所有可见Sheet | hidden行列未进入比较/迁移；可见卡牌、符文、状态和投放与临时表一致 |
| XLSX/CSV | `python scripts/data/sync_xlsx_to_csv.py --check` + 本Plan聚焦Python测试 | 64张启用卡、`8/32/24`、唯一源映射、24行符文变化、状态/投放/冲突表准确 |
| 静态 | `python scripts/validate_project.py`；`git diff --check` | Schema、ID、Behavior、外键、hidden过滤、四武器和禁用集合不变量通过 |
| Cards | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Cards` | 27张新卡注册、投放资格、随机/进度/组合和原子授予通过 |
| Run/Shop/Save | `ReEcho.Run`、`ReEcho.Shop`、存档迁移聚焦测试 | 债务、免费商店、刷新、任务、历史、预付卡包和SaveVersion通过 |
| Combat | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Combat` | 诅咒、过量伤害、负面状态、超额生命、元素链和伤害来源通过 |
| Weapons/Enemies | `ReEcho.Weapons`、`ReEcho.Enemies` | 符文新数值、武器历史、敌人类别阈值、四武器与退役集合不回归 |
| UI | `ReEcho.UI.TraitCard`、`ReEcho.UI.Shop` 及聚焦测试 | 64张卡展示、fallback图、投放状态、债务/实际结果只读显示正确 |
| C++格式/构建 | 修改的`.h/.cpp`运行仓库`.clang-format`；`scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT成功且diff无非目标格式变化 |
| 发布包 | `python scripts/ue/prebuilt_editor.py check` | 精选Editor预构建包与最终源码/内容指纹一致 |
| 人工 | 按三个卡牌批次和八关投放清单执行PIE/Shipping手测 | 用户确认已有平衡、新卡、组合、商店、存档和可观察结果正确 |

## 执行记录

### 变化

- 2026-08-25：对比临时开普勒表与 `origin/main` 策划数据源同名表；确认内容变化为27张新增卡、4个改名、2张已有卡数值、24行可见符文程序字段、`Z_Cursed`、流血负面分类和第5关商店投放。
- 2026-08-25：确认两份表的hidden集合一致：`武器体系W`第2行，`武器符文C`第8-15、57-77行；按用户要求全部排除。
- 2026-08-25：确认新表126张内嵌图中125张与旧表一致；唯一新增图为27张新卡共用的人物证件照占位，决定不进入生产美术。
- 2026-08-25：原 Plan110 草案创建后，远端新增正式商店 UI 的 Plan110；完成外部提交审计并经用户确认，本任务改号为 Plan111、变基到最新主分支并迁移专属 worktree。
- 2026-08-25：用户确认执行；Plan111 进入 `Ready`，等待发布后开始生产实现。

### 证据

- 主分支策划源工作簿与原审计基线 `origin/main@42624169` blob一致；临时表与附件只读审计完成。
- 外部提交审计：`49dc312f` 仅新增 `plans/110-formal-shop-ui.md`，与本任务生产写集无内容冲突；编号冲突已通过改号解决。
- 当前生产运行时仍为39张卡、四武器和Plan105禁用符文基线；新策划源ID与现有运行时ID存在编号碰撞，因此Plan锁定显式源映射和追加式运行时ID。

### 剩余风险

- 5项复杂卡产品细节尚待确认；相关批次开始前不得自行猜测。
- `ReEchoData.xlsx`、Run Save、Combat结果和卡牌模块均为高耦合表面，应按大步骤分批提交、验证和集成。
- 27张新卡没有正式独立icon，逻辑交付只能使用通用fallback并另行向美术交接。

### 人工验收结果/请求

- `PendingBeforeClose`：等待实现后的分批PIE/Shipping验收。

### 架构文档审阅结果

- 待实现完成后逐项填写。
