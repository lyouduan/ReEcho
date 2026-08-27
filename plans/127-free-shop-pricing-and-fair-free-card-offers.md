# Plan 127 - 程序 - 商店免费定价、免费卡牌公平投放与智者卡组计数

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner 与 Executor）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`7b63217ccb32a04f022d2870e65c51e6fb547ea8`（建立 worktree 时的最新 `origin/main`）。
- 本地实现方式（可选，仅作交接说明）：独立 worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan127-free-shop-random-offers`，分支 `plan/127-free-shop-random-offers`。
- 依赖 / 阻塞：延续 Plan87 的数据驱动角色能力与智者每第5组额外选择契约；延续 Plan88 的逐关 `FreeTier`、1级可重复/2—3级持有排除、同一三选一无重复与候选页存档稳定契约；延续 Plan111 的 `G_2_21`【喂，打劫！】“当前战后商店内容物免费”状态。无产品阻塞。
- Writes:
  - `plans/127-free-shop-pricing-and-fair-free-card-offers.md`
  - `Source/ReEcho/Public/Run/ReEchoShopCatalog.h`
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/UI/ReEchoInventoryShopWidget.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoCharacterPromotionTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopLogicBlockTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoTraitTests.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
- Stable Reads:
  - `Content/Data/cards.csv` 的 `G_2_21`
  - `Content/Data/card_effects.csv` 的 `G_2_21_FREE_SHOP`
  - `Content/Data/shop_drop_levels.csv`
  - `Content/Data/shop_refresh_rules.csv`
  - `Content/Data/character_abilities.csv` 的 `SAGE_BONUS_CHOICE`
  - `Source/ReEcho/Public/Run/CharacterAbilities/ReEchoCharacterAbilityRuntime.h`
  - `Source/ReEcho/Private/Run/CharacterAbilities/ReEchoCharacterAbilityRuntime.cpp`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardRuntime.h`
  - `Source/ReEchoCards/Private/Cards/ReEchoCardRuntime.cpp`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardTypes.h`
  - `plans/87-forge-cleanup-and-character-abilities.md`
  - `plans/88-card-drop-system.md`
  - `plans/111-kepler-visible-card-rune-update.md`
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：不改变稳定 Character/Card/Weapon/Rune ID、CSV Schema、智者配置间隔 `5`、卡牌 Tier/冲突/持有资格、商店刷新费用或次数；继续保存 `TraitOfferSeed`、当前免费三选一候选、展示历史、逐槽刷新次数与既有智者计数 RuleFlag，不提升 SaveVersion。旧存档恢复后保持已展示报价和累计计数，不因本修复重新抽取或清零。
- 明确排除：不修改策划工作簿或生成 CSV，不调整卡牌/武器/符文价格与数值，不让【喂，打劫！】免除刷新按钮本身的刷新费用，不更改商店卡组投放规则，不允许已拥有2/3级卡重新出现，不把关闭界面、重复查询或读档当成免费重抽机会。

## 锁定目标

1. 【喂，打劫！】绑定当前战后商店时，所有可购买内容物的实际价格均为 `0`：武器、符文、旧兼容商品以及一级/二级/三级卡牌组都必须在余额不足原价时仍可点击并成功购买；显式刷新后出现的新武器/符文内容物仍为零价。UI 显示、按钮可用性、购买审计和后端提交必须使用同一份 Run 权威实际价格，不能由 Widget 另按原价与折扣计算。
2. 战斗结束免费三选一的初始三张卡，改为从当前关 `FreeTier` 的完整合法候选池中等权、无放回抽取。删除现有“按已拥有叠层数分桶并优先最低叠层”的人为偏置；合法的已拥有1级卡与未拥有1级卡具有相同的单卡入选概率，已拥有2/3级卡仍由 Cards 资格层排除。
3. “公平随机”采用新局开始时从 UTC 实时时钟与高精度时钟取样一次的本局独立种子，随后驱动确定性伪随机流：新开局之间应产生不同序列；同一局、同一关、同一构筑状态与同一随机状态应可复现。首次生成后的三张候选继续保存，重复打开界面和存读档不得重摇；只有已存在的显式逐槽刷新事务可以替换对应槽位。
4. 同组三张卡不能出现重复 CardId；候选不足、配置非法和牌池为空时继续沿用现有安全降级，不跨 Tier 补牌，也不改变战后流程路由。
5. 智者“每第5次获得卡牌组时，可以额外选择1张”的累计来源同时包含战后免费卡牌组与商店购买卡牌组。商店卡牌组在玩家成功领取其中一张卡、卡组购买事务完整提交时累计一次；仅预付卡组、打开/取消界面、失败重试和逐槽刷新不累计。命中第5组后复用现有智者额外三选一流程，额外选择本身不再次累计，也不递归触发。
6. 符文背包只展示与当前装备武器兼容、且属于所点击槽位的已拥有未装备符文。长剑与镰刀共享 `Grip` 槽位时，不能仅因槽位同名而互相展示专属符文；Run 投影和 UI 展示都保留武器类型兼容条件，后端装备事务继续作为最终校验。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`（直接修改），`MOD-ReEchoCards`（抽取资格与抽样边界契约受影响），`AREA-Run`、`AREA-Cards`、`AREA-UI`、`AREA-Tests`；角色能力仍属于 `MOD-ReEcho/AREA-Run/CharacterAbilities`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoCards.md` 与 `MOD-ReEchoUI.md`，均已加入 `Writes`。
- 设计意图：让 Run 成为商店实际价格与购买资格的唯一投影源，让 Widget 只展示和发命令；让 Cards 继续只决定合法候选池，Run 只对该池做无权重、无放回抽样；让免费与付费卡组在成功领取卡牌后通过同一组获取计数入口驱动智者能力。由此消除前后端定价分叉、隐藏叠层权重和卡组来源漏计，同时保留可保存、可复现的事务随机性。
- 权威状态与依赖：`UReEchoRunSubsystem` 继续拥有 `FreeShopEncounterIndex`、货币、商店事务、`TraitOfferSeed`、已生成候选页与智者卡组累计 RuleFlag；`ReEchoCharacterAbilityRuntime` 继续只按数据驱动间隔解析是否产生额外选择；`ReEchoCardRuntime::BuildOfferPool` 继续拥有 Enabled/Tier/关次/冲突/持有资格；UI 不新增权威状态。模块依赖方向不变，不增加 Runtime Module。
- 决策记录：
  - 已定位【喂，打劫！】后端 `GetDiscountedShopPrice` 会正确返回 `0`，但 Widget 的 `GetEffectiveShopPrice` 未接收免费商店状态，并在请求发出前按原价禁用按钮。实现应发布类型化的实际价格/可购买投影，购买提交仍由后端重新验证，不能只在 UI 特判 CardId。
  - 已定位免费投放原先使用每局 GUID 派生的 `TraitOfferSeed`；按用户最终确认改为在 `StartRun` 从 UTC 实时时钟与高精度时钟取样一次。玩家感知的“假随机”同时通过移除 `GenerateTraitCardOffers` 的持有叠层优先分桶解决；不删除每局种子和候选缓存。
  - “等权”按 CardId 计，每个合法 CardId 一张票；不按卡牌阵营、效果、当前叠层、目录顺序或历史未出现次数加权。抽样为洗牌后取前三张，等价实现可以优化，但必须满足相同验收。
  - 免费选择的逐槽刷新继续从同 Tier 剩余合法候选中选择，并排除当前组已展示历史与所有已获得卡牌；本 Plan 只消除初始三选一的叠层分桶偏置，不放宽刷新资格。
  - 【喂，打劫！】描述中的“包括刷新出的内容”解释为显式刷新后生成的新商品仍零价；刷新动作本身继续消费 `shop_refresh_rules.csv` 配置的次数/碎片。
  - 当前智者逻辑从 `character_abilities.csv/SAGE_BONUS_CHOICE` 正确读取间隔 `5`，但只在 `ApplyTraitCard` 成功后增加 `NormalTraitSelections`；`ClaimPaidShopCardChoice` 完全绕过该计数。实现应把“成功完成一个普通卡组的单张领取”收敛为 Run 内单一计数入口，免费与商店领取各调用一次。
  - 商店卡组采用“领取成功时计数”而不是“预付成功时计数”：付款后取消仍保留待领取卡组，不应提前产生智者奖励；同一卡组恢复后领取只提交一次，因此天然避免重复计数。第5组触发后沿用现有由当前关 `FreeTier` 生成的额外三选一，不把购买卡组改为同组三张中拿第二张。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：关闭前审阅；预期无模块拓扑、状态所有者或依赖方向变化。
  - `shared/CODEBASE_MAP/README.md`：关闭前审阅；预期无架构标识或阅读路线变化。
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：更新 Run 的商店实际价格投影、免费卡牌等权抽样与智者免费/商店卡组统一计数契约。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`：明确 Cards 提供合法候选池，Run 负责初始免费三选一的等权无放回抽样。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：更新商店 Widget 只消费权威实际价格/可购买状态、不得重算定价的约束。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：已审阅；本次未新增 Runtime Module、改变依赖方向或迁移状态所有者，无需修改。
  - `README.md`：已审阅；架构标识与阅读路线未变化，无需修改。
  - `MOD-ReEcho.md`：已更新 Run 权威报价、免费投放抽样和普通卡组统一计数契约。
  - `MOD-ReEchoCards.md`：已更新 Cards 合法池与 Run 等权无放回抽样边界。
  - `MOD-ReEchoUI.md`：已更新 Widget 只消费权威实际价格与购买资格的约束。

## 锁定验收

- [x] 在现有碎片为 `0`、商品原价大于 `0` 且【喂，打劫！】绑定当前商店时，武器、符文与三个卡牌组中所有实际存在且未拥有的报价均显示免费、按钮可用，并由详细购买接口以 `EffectivePrice=0` 成功提交；余额与债务均不增加。
- [x] 显式刷新武器/符文页后，新出现商品仍免费；进入下一关后免费商店状态按现有生命周期清除，后续商店恢复正常价格和余额门禁。
- [x] UI 不再用原价/折扣自行决定购买资格；自动化覆盖逻辑块与目标表现层的武器/符文、兼容商品和卡牌组入口，证明显示、按钮和后端结果一致。
- [x] 免费三选一初始候选从完整合法池等权无放回抽取；测试构造“已拥有1级卡 + 至少3张未拥有1级卡”的候选池，证明已拥有1级卡不会再被最低叠层分桶系统性排除。
- [x] 同一随机状态生成顺序可复现且同组三张不重复；不同新局种子能产生多于一种候选签名；存读档、重复打开界面保持原候选及逐槽刷新历史。
- [x] 逐关 `FreeTier`、1级重复、2/3级持有排除、候选不足安全降级、免费逐槽刷新与商店卡组投放既有回归全部通过。
- [x] 智者前4次普通卡组领取不产生额外选择，第5次产生恰好1次额外三选一；五次累计可以由“免费卡组 + 商店卡组”任意组合构成，连续购买多个不同 Tier 商店卡组时每个成功领取各累计一次。
- [x] 商店卡组仅预付、取消后重开、购买/领取失败、逐槽刷新以及智者额外选择均不增加普通卡组计数；保存并恢复待领取卡组或累计到第4组的进度后，后续成功领取只增加一次并在正确时点触发。
- [x] 非智者角色成功领取商店卡组不产生额外选择；智者的间隔和值继续来自 `character_abilities.csv`，不得硬编码 `5` 或角色中文名。
- [x] 镰刀的 `Grip` 背包不展示 `LongSword` 专属剑柄符文，长剑背包同样不展示 `Scythe` 专属握柄；`Any` 通用符文仍按槽位正常展示，切换武器后背包投影随当前武器刷新，后端仍拒绝任何不兼容装备请求。
- [x] `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`、聚焦自动化、`python scripts/validate_project.py` 与 `git diff --check` 通过。
- [ ] 人工 PIE 验收低余额下【喂，打劫！】购买和多次新局免费三选一的玩家可见结果。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`7b63217ccb32a04f022d2870e65c51e6fb547ea8`；该提交是建立本 worktree 时的最新 `origin/main`。
- 引擎/构建可用性：上一程序候选已在 UE 5.8 Development `-FullRebuild` 成功并刷新精选 Editor 包；本 Plan 实现后的最终组合必须重新执行完整构建，不能复用旧证据。
- 现有聚焦测试结果：上一程序候选的棱镜多弹、重启任务刷新与诅咒银行显示聚焦自动化通过；本 Plan 的 Shop/Trait 基线与新增用例在实现阶段重新执行。
- 共享契约 / 难合并资源风险：`ReEchoRunSubsystem.cpp`、`ReEchoGameMode.cpp`、`ReEchoInventoryShopWidget.cpp`、Character/Shop/Trait 测试和精选 DLL 为高频路径；Plan126 仅新增武器特效挂点计划，无物理或逻辑重叠。发布前仍须重新 fetch、审计最新 main 并按发布锁规则组合。
- 基线损坏时的停止条件：远端修改商店/卡牌随机/角色能力路径形成真实产品冲突；策划更改【喂，打劫！】免费范围、Tier持有资格、随机权重或智者“第5组”定义；现有存档候选页/智者累计不能兼容恢复。遇到这些情况先报告并等待取舍，不静默改变锁定目标。

## 实现提纲

1. 为 Run 的武器/符文、兼容商品与卡牌组只读报价补充统一的实际价格/可购买投影；投影复用 `GetDiscountedShopPrice`，详细购买接口仍在提交时二次验证。
2. 改造 InventoryShop Widget 的所有购买入口、价格文本与点击前门禁，只消费 Run 投影；删除会遗漏 `FreeShopEncounterIndex` 的本地定价判断。
3. 将免费三选一初始候选从“按叠层分桶后抽取”改为完整合法池的等权无放回抽样，保留 `TraitOfferSeed`、候选缓存、展示历史、逐槽刷新和 SaveGame 恢复契约。
4. 抽取 Run 内“普通卡组成功领取”计数入口；免费 `ApplyTraitCard` 与商店 `ClaimPaidShopCardChoice` 仅在各自原子事务成功时调用，按数据驱动间隔登记智者额外选择。GameMode 在商店领取触发奖励时关闭商店卡组浮层并转入现有额外三选一，完成后返回原商店。
5. 增加 Shop/ShopLogicBlock 自动化，覆盖零余额零价购买、刷新后仍免费、下一关恢复定价、卡牌组与武器/符文按钮/后端一致。
6. 增加 Trait/Character/Shop 自动化，覆盖完整池公平抽样、同组三张去重、同状态复现、新局多样性、存读档稳定、智者免费/商店混合五组节奏以及失败/取消不计数。
7. 在符文背包只读投影和 UI 二次过滤中加入当前武器类型兼容条件，并增加共享槽位专属符文不串包的回归测试。
8. 更新相关模块文档，执行格式化、聚焦自动化、完整构建、项目校验和差异卫生检查；请求用户完成 PIE 人工验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 卡牌与数据静态 | `scripts/data/sync_xlsx_to_csv.py --check` | 本 Plan 未改策划数据，生产 XLSX/CSV 无漂移 |
| 格式 | 对修改的 `.h/.cpp` 运行仓库 `.clang-format` | 目标文件格式一致，无无关语义噪声 |
| Shop 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Shop`、`-Filter ReEcho.UI.Shop` | 免费定价、按钮资格、后端提交、生命周期与既有商店回归通过 |
| Trait 自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Traits`、`-Filter ReEcho.Cards.Offer` | 等权无放回抽样、Tier/持有资格、刷新和存读档稳定通过 |
| 角色能力自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Characters.SageBonusCadence` | 免费/商店混合卡组均计数，第5组奖励、非递归、失败与存档恢复通过 |
| C++ 发布构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新与最终候选匹配的精选 Editor 包 |
| 静态 | `python scripts/validate_project.py` | 项目、数据、存档/预构建不变量通过 |
| 差异卫生 | `git diff --check` | 无空白、冲突标记或非法路径问题 |
| 人工 PIE | 低余额【喂，打劫！】商店；多次新局免费三选一 | 商品显示免费且可购；免费候选无固定最低叠层偏置，页面重开不重摇 |

## 执行记录

### 变化

- 2026-08-27：完成只读定位并创建 Plan127。确认【喂，打劫！】后端实际价已为零，但 UI 自行按原价/折扣禁用余额不足的购买按钮。
- 2026-08-27：确认免费三选一已有每局独立且可保存的随机种子；“假随机”来自初始候选按持有叠层分桶并强制优先最低叠层，而不是所有新局共用固定种子。
- 2026-08-27：fetch 发现远端仅新增 `plans/126-programmer-weapon-vfx-final-anchor.md`，无源码、数据、编号或逻辑冲突；Plan127 基于最新远端主线建立独立 worktree。
- 2026-08-27：按用户补充把智者商店卡牌组计数纳入 Plan127。确认表中 `SAGE_BONUS_CHOICE` 已配置 `Interval=5`；现有免费领取路径会计数，商店 `ClaimPaidShopCardChoice` 不计数。
- 2026-08-27：Run 的所有商店报价统一发布 `EffectivePrice` 与 `bCanPurchase`；Widget 删除本地价格重算，武器、符文、兼容商品和卡牌组共享同一权威投影与后端复核。
- 2026-08-27：免费三选一改为完整合法池确定性洗牌后取前 N 张，移除按已拥有叠层优先级分桶；保留种子、候选缓存、刷新历史和存档恢复。
- 2026-08-27：免费与商店卡组领取统一进入普通卡组计数；商店命中智者第5组时复用额外三选一，领取后回到同一商店。补充预付、刷新、重复领取、存读档与非递归验证。
- 2026-08-27：Run 与 UI 的符文背包投影同时按当前武器类型和槽位过滤；兼容性后端校验保持不变。
- 2026-08-27：回归中发现通用商店测试随机选到【重启任务】后把其 300 碎片和免费刷新奖励误判为扣费异常；已隔离该测试的即时经济副作用，生产逻辑无需回退。
- 2026-08-27：按策划确认将免费投放的新局 `TraitOfferSeed` 从 GUID 改为 UTC 实时时钟与高精度时钟共同取样；只在 `StartRun` 取一次，存档继续持久化该值，重开与读档不会按时间重抽。
- 2026-08-27：发布前取得 `main-publish-lock`，审计并无冲突合入 `origin/main@f6a61d44`；传入范围仅为 Plan128、Plan129 两份文档，无源码、数据或运行时语义重叠。

### 证据

- 只读源码审计：`GetDiscountedShopPrice` 在 `FreeShopEncounterIndex == EncounterIndex` 时返回 `0`；`ReEchoInventoryShopWidget.cpp` 多处仍通过本地 `GetEffectiveShopPrice` 与 `CurrentTimeShards >= EffectivePrice` 提前拦截。
- 源码审计与实现：`GenerateTraitCardOffers` 使用 `BuildTraitOfferSeed(TraitOfferSeed, EncounterIndex, OwnedCardIds)`；初始叠层分桶已移除，`StartRun` 改为从真实时间生成 `TraitOfferSeed`，SaveGame 保存并恢复该种子与当前候选页。
- 只读源码审计：智者 `ResolveExtraTraitChoices` 已按 `character_abilities.csv` 的 `OnTraitChoiceApplied / Character.EveryNth / Interval=5` 正确计算；`ApplyTraitCard` 在普通免费领取后增加 `NormalTraitSelections` 并排除奖励选择递归，`ClaimPaidShopCardChoice` 只授予卡牌和标记卡组已购，没有同等计数或奖励路由。
- 最终集成构建：合入最新 `origin/main` 后执行 `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` 成功，95 个动作完成，精选 Editor 包刷新到源码指纹 `08f85377cc7f`。
- 最终自动化：`ReEcho.Shop`、`ReEcho.UI.Shop`、`ReEcho.Traits`、`ReEcho.Cards.Offer`、`ReEcho.Characters.SageBonusCadence` 全部通过。
- 实时时间种子回归：`ReEcho.Traits.OffersAreDeterministicAndDiverse` 验证连续新局捕获多个不同种子，同时保存/恢复保持候选顺序一致。
- 最终静态证据：`sync_xlsx_to_csv.py --check`、`validate_project.py`、`git diff --check` 全部通过；无冲突文件或额外未跟踪生成物。

### 剩余风险

- 公平随机的玩家感受仍需多局人工观察；自动化已锁定“无隐藏叠层优先级、等权无放回与事务可复现”，但不承诺短样本内肉眼均匀。
- 商店触发智者奖励的逻辑与流程自动化已通过，最终屏幕层级、焦点和视觉反馈仍需 PIE 人工确认。

### 人工验收结果/请求

- 已完成程序实现与自动化验收，请用户按交接清单完成 PIE 人工验收；结果仍为 `PendingBeforeClose`。

### 架构文档审阅结果

- `ARCHITECTURE.md`：已审阅，无模块拓扑、依赖方向或权威状态变化，无需修改。
- `README.md`：已审阅，无架构标识或阅读路线变化，无需修改。
- `MOD-ReEcho.md`：已同步 Run 权威定价、等权投放、智者统一计数与符文兼容投影。
- `MOD-ReEchoCards.md`：已同步合法池/抽样职责边界。
- `MOD-ReEchoUI.md`：已同步只读权威报价与购买资格契约。
