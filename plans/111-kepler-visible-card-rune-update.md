# Plan 111 - 程序 - 开普勒可见卡牌与符文规则更新

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner）。
- Executor 负责人：Codex（规划与执行合并；用户已确认执行本 Plan）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`；27 张新卡、既有卡牌数值变化、符文平衡和组合机制需要用户在 PIE/Shipping 候选中验收。
- 本地规划 / 实现基线：`origin/main@49dc312facaf1be002ef6919aceb8e4445b2b08f`。
- 本地实现方式：一任务一 worktree；`plan/111-kepler-visible-update`，`C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan111-kepler-visible-update`。
- 依赖 / 阻塞：
  - 依赖 Plan47/88/97/101/105/108 的卡牌目录、逐级投放、卡包预付费、逐槽刷新、实际结果浮窗和存档契约；
  - 依赖 Plan76/104 的可见符文运行时、四武器生产集合、统一 `DamageCoefficient` 和核心伤害通道；
  - 五项复杂卡规则已由用户确认并锁定，无待确认产品阻塞；仍需PIE/Shipping人工验收。
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
5. 更新两张已有卡牌实际效果、24 行可见符文程序字段及第5关商店投放；对于本轮最新外部策划表与仓库旧表/旧代码的冲突，最新外部策划表完全覆盖旧值；同一新表内部重复定义冲突时，以对应专业 Sheet（角色体系、武器体系、元素体系等）为准，并回写构筑 Sheet 消除自相矛盾文案。
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
  7. **最新表完全覆盖冲突：**2026-08-26 最新外部表是本轮数据权威。角色数值/能力以 `角色体系J` 为准，武器参数以 `武器体系W` 为准，导电范围以 `元素体系Y` 为准，卡牌参数以 `构筑体系G` 为准；若构筑 Sheet 中的角色卡摘要与 `角色体系J` 冲突，先按角色体系修正摘要，再同步生产 XLSX/CSV/运行时和测试。旧表与旧代码不作为保留旧值的理由。
  8. **卡组刷新历史去重：**单次免费三选一和每个商店 Tier 卡组分别持有本组已展示历史；逐槽刷新除排除已拥有卡和当前三张外，还排除本组此前被替换掉的卡。新开一组时重置历史，存档/恢复必须保留当前组历史。
- 已确认的复杂卡产品细节：
  1. `诅咒银行`：赊账无上限，余额优先支付、后续碎片优先还债，每完成一关按当前欠款10%向下取整计息；
  2. `连接，连接！`：敌人每次穿越一条玩家—存活回响线段结算一次；
  3. `我们，合体！`：0m为+100%，30m为+0%，中间线性插值；
  4. `左右开弓`：只按Player/Echo两类区分，以上一次实际造成有效伤害的来源为准；
  5. `武器大师`：使用某武器成功完成一关后才计入历史。
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

- 2026-08-26：产品确认并锁定五项补充规则：诅咒银行赊账无上限、所有后续碎片优先还债、每关按欠款 10% 向下取整计息；连接按敌人每次穿越玩家—回响线段结算一次；我们，合体！按 0m +100% 到 30m +0% 线性插值；武器大师以“使用该武器完成过一关”为历史口径；左右开弓仅区分 Player/Echo，且以前一次实际生效伤害为准。

### 变化

- 2026-08-25：对比临时开普勒表与 `origin/main` 策划数据源同名表；确认内容变化为27张新增卡、4个改名、2张已有卡数值、24行可见符文程序字段、`Z_Cursed`、流血负面分类和第5关商店投放。
- 2026-08-25：确认两份表的hidden集合一致：`武器体系W`第2行，`武器符文C`第8-15、57-77行；按用户要求全部排除。
- 2026-08-25：确认新表126张内嵌图中125张与旧表一致；唯一新增图为27张新卡共用的人物证件照占位，决定不进入生产美术。
- 2026-08-25：原 Plan110 草案创建后，远端新增正式商店 UI 的 Plan110；完成外部提交审计并经用户确认，本任务改号为 Plan111、变基到最新主分支并迁移专属 worktree。
- 2026-08-25：用户确认执行；Plan111 进入 `Ready`，等待发布后开始生产实现。
- 2026-08-25：Plan111 已发布到 `origin/main@b88e4e2f`，实现 worktree 变基到该提交后进入 `InProgress`。
- 2026-08-25：完成步骤1及步骤2的已有卡/符文/第5关投放数据部分：仓库策划源替换为临时表可见内容，既有卡源ID显式重映射，四项改名、万象回春、殉身回响和可见符文数值写入生产XLSX/CSV。artifact-tool 导出会丢失生产保护属性，经用户明确确认后采用 Plan53 同类的定向 OOXML 后处理，仅恢复既有样式与 sheetProtection；内容编辑、渲染和公式检查仍由 artifact-tool 完成。
- 2026-08-25：发现新表删除旧“连发枪口”导致枪械行号整体前移，改为按 DisplayName 对齐可见符文；旧禁用连发枪口转为 `Legacy武器插槽C` 审计记录。新表第56行“连射移速枪机”是可见正式内容，新增稳定 `P_GUN_MOVESTACK_GUNACTION` 与 `Part.MoveSpeedOnAttack`，同时保留旧 hidden 第57行占位为禁用迁移记录。
- 2026-08-25：按程序列将“流血旋刃”从每三次命中改为暴击触发；“可以叠加”且未声明上限的符文统一用0表示无配置上限，运行时解析为 `int32` 安全上限，并补充超过30层的编译期自动化断言。
- 2026-08-25：完成共享投放资格、`Z_Cursed`、卡牌效果表移至 `Q:AA`、`孤注二掷`、`喂，打劫！`、`重启任务`、`就要那个！`、`样样都通` 与 `龙魂，集齐！`。六核心条件按已拥有且启用的不同核心ID计算，旧存档恢复后会幂等补算。
- 2026-08-25：第二批先落地 `数字挑战`、`德古拉第I课/II课` 与 `自我赛跑`。Combat新增最终命中结果只读回传；溢出伤害以 `RawDamage-AppliedDamage` 计算，伤害赛跑只累计最终实际伤害，数字挑战三个条件共享一次性完成状态。
- 2026-08-25：完成 `损人利己` 及负面状态成功事件链。灼烧、眩晕、流血、诅咒只在状态实际应用成功后通知来源角色或Echo，并统一由Run/Cards事务结算玩家回复；Boss诅咒免疫、刷新覆盖和无效应用不会误触发。
- 2026-08-25：完成 `史莱姆杀手`、`兔兔杀手` 的共享敌人类别进度底座。CombatTarget提供稳定敌人定义ID，Run永久保存玩家专属分类击杀数；达到50/25阈值后，Combat在对应目标承伤前施加`Z_Cursed`，Echo击杀和其他敌人类别均不计数、不误触发。
- 2026-08-25：完成 `回归基本功`。第一次由玩家触发的元素反应只记录稳定ReactionId且本次伤害不变；之后相同反应最终倍率为2、其他反应为0，Echo共享该已记录规则，记录值随卡牌运行时存档并投影到实际结果。
- 2026-08-26：完成剩余Plan111卡牌：五项已确认复杂卡、Echo头/身/腿三件套、烈火之心、水火不容、雷鸣之心、生命虹吸；生产启用卡达到64张（Tier `8/32/24`），SaveVersion升至21并为v20提供零新收益迁移。
- 2026-08-26：按用户“冲突完全采用新表”的最终口径重新同步最新外部工作簿。角色以`角色体系J`、武器以`武器体系W`、导电以`元素体系Y`、卡牌以`构筑体系G`为权威；旧表和旧代码中的冲突数值全部被覆盖，并回写构筑Sheet中的角色摘要以消除新表内部冲突。
- 2026-08-26：完成最新表对应运行时更新：角色初始属性及贤者每5组/猎人暴伤+30%，长剑/镰刀/枪攻速、倍率与范围，导电200cm，以及`时距共鸣`每200cm增加4%的阶梯伤害。SaveVersion升至22，并为免费三选一及各商店Tier卡组保存“本组已展示历史”，逐槽刷新不会重新刷出本组此前展示过的卡。
- 2026-08-26：将用户提供的“连接，连接！”图导入SourceArt与UE纹理`T_UI_CardIcon_G_2_30`；源图与导入源SHA256一致，临时UE工程验证纹理可加载且尺寸为512×512。
- 2026-08-26：发布前审计并合入`origin/main@fbfa8cc9`。传入范围包含Plan112-119、战斗/Boss修复、伤害数字与元素反应表现；物理冲突集中在Run卡牌授予、Cards结果结构、Combatant生命接口、模块文档和精选二进制。组合保留远端的`HealthAdjustment`/血肉铸锋实时回满、GMGod伤害表现与完整StatBlock同步，同时保留Plan111的符文清空事务、超额生命和卡组展示历史；精选二进制不二选一，统一在最终组合上FullRebuild。

### 证据

- 主分支策划源工作簿与原审计基线 `origin/main@42624169` blob一致；临时表与附件只读审计完成。
- 外部提交审计：`49dc312f` 仅新增 `plans/110-formal-shop-ui.md`，与本任务生产写集无内容冲突；编号冲突已通过改号解决。
- 当前生产运行时仍为39张卡、四武器和Plan105禁用符文基线；新策划源ID与现有运行时ID存在编号碰撞，因此Plan锁定显式源映射和追加式运行时ID。
- 生产工作簿全Sheet公式错误扫描为0；`sync_xlsx_to_csv.py --check`、`validate_project.py`、`prebuilt_editor.py check` 和 `git diff --check` 通过。UE 5.8 Development FullRebuild 98/98通过并刷新7模块预构建包（BuildId `55116800`，source fingerprint `3ce7288712d7`）。
- 当前48张启用卡（Tier `8/20/21`）生产XLSX/CSV同步和静态校验通过；Development FullRebuild 102/102通过并刷新7模块预构建包（BuildId `55116800`，source fingerprint `c75107a7080d`）。新增Cards/Combat断言已编译。
- 当前49张启用卡（Tier `8/21/21`）生产XLSX/CSV同步和静态校验通过；Development FullRebuild 99/99通过并刷新7模块预构建包（BuildId `55116800`，source fingerprint `92094efffd27`）。`损人利己` 的玩家/Echo负面状态回复断言已编译。
- 当前51张启用卡（Tier `8/23/21`）生产XLSX/CSV同步和静态校验通过；Development FullRebuild 101/101通过并刷新7模块预构建包（BuildId `55116800`，source fingerprint `ab4be0cd5124`）。分类击杀阈值、Echo不计数和跨类别隔离断言已编译。
- 当前52张启用卡（Tier `8/23/22`）生产XLSX/CSV同步和静态校验通过；Development FullRebuild 99/99通过并刷新7模块预构建包（BuildId `55116800`，source fingerprint `24890ac7b6ac`）。首次记录、同类×2、异类×0和记录不可覆盖断言已编译。
- 当前64张启用卡（Tier `8/32/24`）生产XLSX/CSV同步和静态校验通过；Development FullRebuild 101/101通过并刷新7模块预构建包（BuildId `55116800`，source fingerprint `807438cceeed`）。Echo套装、空间/来源规则、赊账与武器历史、溢出生命断言已编译。
- `Run-Automation.cmd -Filter ReEcho.Weapons.Runes` 与无编译直启均在测试发现前被本机 UE 5.8 `ValidatePlatforms -AllPlatforms` 的 LinuxArm64/VisionOS `SDK.json MainVersion` 环境门禁阻断；新增断言已被 UHT/UBT 编译，但本轮不冒充运行通过。
- 最新策划源与生产工作簿已通过artifact-tool全公式错误扫描（0项）和关键Sheet渲染目检；生产Sheet保护已恢复。`sync_xlsx_to_csv.py --check`、18项同步器Python测试、`validate_project.py`和`git diff --check`通过。
- UE 5.8 Development Editor构建通过并刷新精选预构建包（BuildId `55116800`，source fingerprint `ffe624ad0d10`）。绕开本机跨平台SDK发现门禁后，聚焦自动化实际运行通过：时距共鸣阶梯、商店卡组刷新历史、免费三选一刷新历史、全部角色、元素反应及存档连续性、武器攻击步骤和宝石伤害矩阵。
- 最终组合候选UE 5.8 Development FullRebuild 94/94通过，精选包BuildId `55116800`、source fingerprint `0671f740aa10`；XLSX/CSV同步、18项同步器Python测试、项目校验、预构建包校验和`git diff --check`通过。聚焦自动化实际运行通过：`ReEcho.GAS.HealthAdjustmentPreservesTransientState`、`ReEcho.Combat.OverhealCapacity`、`ReEcho.Cards.Grant.BloodForgingFillsHealth`、`ReEcho.Cards.Runtime.ProximityAndAlternatingSources`、`ReEcho.Shop.RefreshesWeaponRunesAndCardSlotsIndependently`、`ReEcho.Traits.FreeChoiceSlotsRefreshIndependently`、`ReEcho.Weapons.Gems.DamageCoefficientMatrix`。

### 剩余风险

- 仓库包装脚本仍会被本机UE 5.8跨平台SDK发现门禁影响；本轮已用等价Editor命令实际运行上述聚焦自动化，完整PIE/Shipping表现仍需人工验收。
- `ReEchoData.xlsx`、Run Save、Combat结果和卡牌模块均为高耦合表面，应按大步骤分批提交、验证和集成。
- 27张新卡没有正式独立icon，逻辑交付只能使用通用fallback并另行向美术交接。

### 人工验收结果/请求

- `PendingBeforeClose`：等待实现后的分批PIE/Shipping验收。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅、无需修改；本轮没有新增模块、依赖方向或跨模块拓扑。
- `shared/CODEBASE_MAP/README.md`：已审阅、无需修改；稳定 `MOD-*` / `AREA-*` 路由和代码入口未变化。
- `MOD-ReEcho.md`：已更新；记录Plan111权威生产数据、商店/免费投放、存档、武器/角色/元素接线及SaveVersion 22展示历史。
- `MOD-ReEchoCards.md`：已更新；记录64张卡牌目录、统一投放资格、复杂卡行为和逐组三选一刷新历史边界。
- `MOD-ReEchoCombat.md`：已更新；记录诅咒、最终伤害/治疗/状态事件和Plan111卡牌消费的领域权威结果。
- `MOD-ReEchoWeapons.md`：已审阅、无需修改；本轮最新表只调整既有四武器数据值，Weapons继续消费现有CSV与统一伤害契约，没有新增公共接口或代码落点。
- `MOD-ReEchoEnemies.md`：已审阅、无需修改；敌人类别与Boss状态边界未变化。
- `MOD-ReEchoUI.md`：已审阅、无需修改；UI继续只读消费既有商店/卡牌投影与实际效果字段，没有新增UI公共契约。
