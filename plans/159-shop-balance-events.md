# Plan 159 - 程序 - 商店最终余额事件同步

## 协调

- Planner / Executor：JosephLE910 + Codex，同一 AI 规划、实现和自审。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Closed`（本地集成与技术/人工验收完成；本次远端发布结果以 Git 引用核验）。
- 人工验收：`Passed`（2026-08-31 用户确认“当前表现是对的，应该按当前表现来算。然后合并远端主分支”；不代表 AI 完成 PIE 主观验收）。
- 规划基线：`ac4bbb3e`（origin/main）；实现可组合本对话上一轮的本地提交 `8b36a501`（刷新文字作者化）。该组合不随本次 Plan-only 发布进入远端。
- 本地实现方式：独立 `ReEcho-plan159-shop-balance-events` / `fix/shop-balance-events`。
- 依赖 / 阻塞：既有 `Card.EasterShardThreshold` 静态清单漏项；先前 Plan-only 豁免未复用，用户在本轮风险说明后另行明确确认先发布实现、再处理该项，准确候选与范围见“本次实现发布的具名静态例外”。其余发布门禁均已执行。
- Writes：本 Plan；`Source/ReEcho/{Public,Private}/Run/ReEchoRunSubsystem.*`；`Source/ReEcho/{Public,Private}/ReEchoGameMode.*`；`Source/ReEcho/{Public,Private}/UI/ReEchoInventoryShopWidget.*`；`Source/ReEcho/Private/Tests/` 内本功能聚焦测试；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoUI.md`；必要时同步 `shared/CODEBASE_MAP/ARCHITECTURE.md`；源码匹配的精选 Win64 Editor 预构建包。
- Stable Reads：Run 的卡牌、商店和存档契约；`ReEchoShopCatalog.h`；Cards 纯逻辑与生产 XLSX/CSV；现有商店/选卡 WBP；`ReEchoUIFlowCoordinatorSubsystem.*`；现有购买、债务和选卡测试。
- 影响模式：`SharedContract`（同 Runtime Module 中 Run 到 UI 的只读通知契约）。
- 兼容承诺：不改存档结构、价格、随机种子、支付资格、卡牌效果、交易顺序或蓝图几何/字体；已发布入口保留必要初始化兼容。
- 明确排除：战斗拾取/HUD 链路改造、数据表迁移或校验器修复、重新导入/重排 WBP、自动发布未验收实现、其他任务资源。

## 锁定目标

1. 商店显示统一响应非战斗经济事务的最终余额，包括持有碎片与债务，而不是由各个按钮处理器分别写数字。
2. 买卡包后、在已付款选卡弹窗内付费刷新后，底层商店立即同步；不选卡返回时显示正确余额。
3. 清理各入口重复的余额更新；商品、装备、卡牌和刷新预算等非余额状态仍走各自必要的投影刷新。
4. 余额通知不能重摇商品、重置卡牌分页、关闭背包/悬浮/选卡层、重建控件或覆盖作者样式。

## 架构影响与设计决策

- 受影响：`MOD-ReEcho` / `AREA-Run` / `AREA-UI`；`MOD-ReEchoUI` 仍为同一 Runtime Module 的逻辑文档入口。
- 权威状态仍属于 Run：`TimeShards` 与 `CurrentBuild.CardState.Runtime.TimeShardDebt`。新增只读快照/提交后通知，显示余额继续为现金减非负债务。
- 通知比较现金和债务各自的前后值，不能只比较净余额。调用者不能通过事件改写交易结果；UI 购买资格继续消费 Run 的投影，不能复制资格公式。
- 在完整外层操作前后比较，合并内部嵌套扣款、授卡、还债等变更为一次最终通知；失败且状态未变化不广播。底层辅助步骤不得发布半完成状态。
- 商店打开时取得一次完整初值并订阅；关闭/销毁时解绑。用确定性提交事件而不是 Tick 轮询；叠加选卡弹窗时不解除商店订阅。
- 余额事件仅更新余额缓存、文字和已有按钮的购买/刷新可用性；非余额商品/装备投影单独保留。GM直接改碎片进入 Run 命令，不能再直接赋值后整页刷新。
- 文档同步：维护 Writes 中两份模块文档；审阅 `ARCHITECTURE.md` 与 `README.md`，若拓扑与路由未变则在执行记录说明无需修改。

## 锁定验收

- [x] 购买武器/符文、购买卡包、卡槽刷新、主商店刷新、正常/GM授卡的碎片变化、结算收益与债务、重开/读档初值均有正确通知或初始化。
- [x] 每笔发生余额变化的完整事务最多一次最终通知；无变化与原子失败不广播；组合状态一致。
- [x] 买卡包→刷新卡牌→返回商店，现金、债务、显示余额和购买可用性同步。
- [x] GM改余额、债务变化不重置第二页、不改变商品身份与刷新序列、不关闭已打开子层。
- [x] 关闭商店不再接收回调，重新打开只有一个订阅并读取最新余额。
- [x] 保留 WBP 作者文字样式和既有按钮悬停交互；清理旧余额写入路径。
- [x] 格式、构建、聚焦测试、LFS与 diff 检查有真实证据；静态失败按具名人工例外记录，不伪报通过。

## Step 0 门禁

- 远端基线：`ac4bbb3e`；相对 `fad6d081` 仅新增 Plan158 文档，无代码/二进制物理冲突、经济逻辑冲突；后续 Boss HUD 实现可能与 GameMode 存在文件级耦合，发布前重新审计。
- 本对话 `8b36a501` 改刷新标签绑定及测试，可在 Plan 发布后按本地依赖组合；本任务不覆盖其作者化层级。
- UE：Windows 安装版 UE5.8；构建前检查 LFS、用户 Editor 进程和 Git-common Unreal 锁。不能关闭用户未保存会话。
- 已知损坏：`validate_project.py` 在 `Content/Data/card_effects.csv:78` 报未知 `Card.EasterShardThreshold`；原始数据不改。若出现新增构建/经济测试失败，先定位不得宣称通过。

## 实现提纲

1. Run 增加余额快照和外层提交通知边界，覆盖非战斗 mutation 入口及 GM窄命令。
2. 商店接入生命周期绑定和局部余额/资格投影；拆开初始化、商品刷新与余额刷新职责。
3. 清理 GameMode 重复余额写入与纯余额操作的整页刷新，保留交易结果、保存、装备和选卡流程。
4. 增加 Run 事件与商店投影测试；构建、执行聚焦回归并记录证据。

## 验证矩阵

| 层级 | 检查 | 预期 |
|---|---|---|
| 静态 | clang-format；`python scripts/validate_project.py`；`git diff --check` | 格式和差异正确，具名既有失败与新增失败分开 |
| 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 成功，精选预构建包指纹一致 |
| 聚焦 | 新增余额事件/订阅测试；`ReEcho.Shop`、`ReEcho.UI.Shop` 相关套件 | 交易结果不变，事件与缓存同步，作者样式/分页/生命周期保持 |
| 保存兼容 | 现有 Save/Trait 相关测试或聚焦保存恢复用例 | 保存结构未变，恢复后的初值和通知正确 |
| 人工 | 商店第二页开选卡并付款刷新、返回；GM改余额；重新打开商店 | 无陈旧数值、跳页或浮窗被关闭 |

## 执行记录

### Plan-only 发布授权与审计

- 用户确认原文：“允许”。授权对象仅为本次新增 Plan159 的文档发布；对当前基线的 `Card.EasterShardThreshold` 静态失败按“经人工确认跳过/未验证”处理，不称校验通过。
- 仅推送本 Plan，不含上一轮未发布实现或本任务源码；按编号 Plan-only 规则使用普通 push，不获取、检查或改变 main-publish-lock。
- 纯文档候选不执行 FullRebuild；实现与后续发布按各自矩阵重新验证。

### 实现 / 证据 / 剩余风险

- Plan-only 提交 `ac28c32d796fedfa486ff215d4403dc26a29099f` 已普通推送到 origin/main 并核验。本地合入上一轮刷新文字提交 `8b36a501` 的组合基线为 `3b589619`；本次实现尚未发布。
- Run 已实现现金/债务快照及外层事务 RAII 通知，覆盖非战斗经济入口，嵌套命令合并通知；GM不再直接写现金字段。商店首次初始化后幂等订阅，余额文字只有 `RefreshCurrencyText` 一个写入口；删除无调用的 `SetTimeShards` API及分散写入。
- 保留购买、换装、授卡、刷新等非余额内容投影；卡包付款后更新待选状态，覆盖零价付款没有余额事件的情况。纯余额事件不调用完整 Refresh、不重建浮窗/控件或复位卡牌页。
- `RemoveFromParent` 在实际关闭边界立即解绑，即使 Slate 仍持有焦点引用也不再接收事件；`NativeDestruct` 幂等兜底。关闭选卡覆盖层不会解除底层商店的订阅。
- 新增 `ReEcho.Shop.BalanceTransactions`、`ReEcho.Shop.BalanceOfferCommands`、`ReEcho.UI.Shop.BalanceSubscription`：覆盖嵌套经济授卡、无变化/拒绝、债务与同净值恢复、战斗拾取排除、真实武器/符文购买、主商店免费/付费刷新、卡组付款/付费刷新/领取、免费页刷新/正常授卡、真实 WBP 样式保持、分页/浮窗和移出页面后的解绑重绑。
- 编译过程中修正了新测试误用 UE5.8 的 TextBlock getter 和武器槽投影类型；没有为通过测试改动产品规则。此前用户 Plan158 Editor 占用时停止构建，用户关闭并确认进程退出后才重新取得 Git-common Unreal 锁。
- 最终 `scripts/ue/Build-Editor.cmd -Configuration Development` 成功，证据 `Saved/Plan159/build-final-fixed.log`；精选 Win64 Editor 包7模块全部更新，Build ID `55116800`，源码指纹 `1169a89079e6`（完整值见 manifest）；`prebuilt_editor.py check` 通过。此为本地开发构建，不冒充最终 main 发布所需 FullRebuild。
- 最终自动化过滤器 `ReEcho.Shop+ReEcho.UI.Shop+ReEcho.Traits+ReEcho.Run.Save`：45项、36通过/9失败。三个新增余额测试全部通过；商店 UI 7/7通过；SaveSnapshot及其他四个存档迁移用例通过。日志 `Saved/Plan159/regression-final.log`，进程测试退出码 -1（shell 255），不称全套通过。
- 对照干净 `ReEcho-shop-refresh-authored-text` 的准确 `8b36a501`，先核验其预构建包和 LFS，再用相同过滤器复测：42项、33通过/9失败；失败测试集合与本次完全一致。源码、资产和表均未在基线 worktree 改动，复测后 git status仍干净。证据 `Saved/Plan159/baseline.log`。
- 相同的九项既有失败：`Run.SaveV22PromotionRemovalMigration`；`Shop.CardRulesAreAtomicAndPersistent`、`Shop.OwnedWeaponBackpackSwitchesAtomically`、`Shop.PageUsesFixedPartSlotsAndConfiguredCardTiers`、`Shop.RuneBackpackFiltersCurrentWeapon`、`Shop.WeaponPartPageRemainsStableAfterSequentialPurchases`、`Shop.WeaponPartsPurchaseThenSaveThreeSlotLoadout`；`Traits.CsvEffectsApply`、`Traits.ResolvedOutcomesPersistAndProject`。涉及已有断言与价格/效果、当前符文ID/报价契约不匹配；本任务不改这些配表或断言。
- `setup_lfs.py --check` 通过（3个LFS文件已还原）；修改范围已 clang-format，`git diff --check` 通过；没有修改 WBP、Content/Data 或 XLSX。所有测试进程退出、持有的 Unreal 锁已释放。
- `validate_project.py` 仍报 `Content/Data/card_effects.csv:78: Card.EasterShardThreshold` 未在校验白名单注册。本次只记录、不改验证器；此前 Plan-only 豁免不覆盖实现发布，也不把上述既有自动化失败视为通过。
- 状态为本地代码评审候选，不宣称全部技术门禁或人工验收已通过。后续需用户验证商店第二页开卡组→刷新→返回、GM改余额以及重新打开；实现进入主线前还需处理或针对准确候选明确接受既有失败，重新取得 main 发布锁、合入最新基线并执行 FullRebuild。

### 当前行为确认与发布准备（2026-08-31）

- 用户在收到九项失败的逐项分析（含旧存档按当前角色表差值混算风险）后，确认原文：“当前表现是对的，应该按当前表现来算。然后合并远端主分支。”
- 本轮仅调整三个测试文件，保持现有运行时、生产 CSV/XLSX、存档算法及 Blueprint 不变。测试读取现有配表效果/价格，改用正式分级符文 ID；保留未购报价不变、已购符文槽为空、每次付款只消费一份卡组、最终用尽后不可再付款的断言。
- v22 测试继续保留历史生命20/元攻5输入，预期改为用户接受的“当前角色表差值”语义，并补充装备基础属性及再次存读档不重复应用差值的断言。这是当前行为特征测试，不宣称解决任意旧版本平衡数据迁移；历史构筑保真风险仍记录。
- 静态校验的 `Card.EasterShardThreshold` C++ 已登记但 Python 清单缺项，与本轮九项测试独立；是否补齐该检查登记正向用户单独确认，不沿用此前 Plan-only 豁免。
- 发布前 fetch 到 `origin/main@a7eb82a5`：相对原批准基线仅新增 Plan160 文档，无规则、源码、数据或资源传入变化。Plan160 后续会调整符文购买入包/合成和存档，涉及本轮 Shop/Save 测试；本次不提前实现它，后续集成需重新审计并按新规则重跑。
- 本轮 Development 开发构建成功（`Saved/Plan159/build-accepted.log`），精选包7模块校验通过，Build ID `55116800`，源码指纹 `8ebca95f113e3d8fb3c8bf3683f634f5f0e44bf975b59bdab856863bb1606afc`。
- 相同过滤器 `ReEcho.Shop+ReEcho.UI.Shop+ReEcho.Traits+ReEcho.Run.Save` 45/45通过，退出码0；此前九项失败全部通过，新增余额测试及商店UI仍通过。日志 `Saved/Plan159/accepted.log`（对应 `accepted-console.log`）。本轮既不改产品逻辑，也不移除测试；这是用户批准的预期更新后的新证据，不覆盖此前失败记录。
- `git diff --check`、LFS还原、`git lfs fsck`、`prebuilt_editor.py check` 通过。`validate_project.py` 仍被独立的 `Card.EasterShardThreshold` 登记缺项阻止；等待具名补登记授权。
- 尚未获取 main 发布锁、未合入传入 Plan160、未推送、未删除工作分支；不占锁等待人工答复。最终发布仍须独占锁后 merge 最新主线、FullRebuild，并重跑准确候选的验证。

### 本次实现发布的具名静态例外（2026-08-31）

- 用户已知 `Card.EasterShardThreshold` 已在 C++ 注册、静态校验清单漏项，并在风险提示后确认：“这个是什么？先合并，再解决。”
- 此确认针对本地实现/测试候选 `641694f67bab4f239274091ac8b5b5cf77303f0a` 及当前远端 `a7eb82a55d0bce7fd87db03f2568f1c4779912e6`（仅传入 Plan160 文档）。本次只对 `validate_project.py` 的该具名登记缺项记录“经人工确认暂缓处理；校验未通过”，不把失败伪报为通过。
- 其他门禁全部保留：准确租约 main 发布锁、获锁后 merge 最新 main、Development FullRebuild、匹配精选二进制、45项专项测试、diff/LFS检查和发布后的远端提交核验。若出现其他错误或实质性远端变动，重新审计，不扩大此例外。
- 合入远端后再处理登记清单；本次先发候选不夹带校验器修补，顺序遵从用户要求。

### 发布集成与最终技术证据（2026-08-31）

- 抢锁正式候选 `e3e63cbc65b4700213ee7e4b9f47c699890ae2cd`；准确空租约创建远端 `main-publish-lock` 后核验持有，再 fetch 并 merge `origin/main@a7eb82a5`，集成提交 `88439a4072c006c3545a60d9d13cbff2043a9e28`。无冲突，仅传入 Plan160 文档。
- 在该准确源码/内容组合执行 `Build-Editor.cmd -Configuration Development -FullRebuild`：96/96 actions，退出0；Development Win64、UE5.8 Build ID `55116800`、7模块精选包全部更新；`prebuilt_editor.py check` 通过，源码指纹 `8ebca95f113e3d8fb3c8bf3683f634f5f0e44bf975b59bdab856863bb1606afc`。
- 构建后再次执行同一45项 Shop/UI/Trait/Save过滤器，45/45通过，测试退出0。证据 `Saved/Plan159/publish-full-build.log`、`publish-tests.log`、`publish-test-console.log`。
- `validate_project.py` 再次仅报告具名 `Card.EasterShardThreshold` 清单漏项，按本轮确认暂缓，不称通过；LFS还原、fsck与diff检查通过。生产表/其他任务 WBP 未改；上一轮作者化刷新 WBP 与实现一起进入本次发布候选。
- 本地集成与验收已闭环；最后证据/精选包提交后，严格按普通 push 更新锁分支、再检查 main 祖先与准确锁值、普通 push main、核验并释放准确锁。远端发布成功后再进行已合并工作分支清理。
- 删除工作树前，保留本轮日志到 Git common directory 的 `reecho-evidence/plan159-20260831-88439a40/`；历史开发失败、干净基线和本次通过证据分别保留，不以新日志覆盖旧结论。

### 架构文档审阅结果（最终）

- `modules/MOD-ReEcho.md`：已更新 AREA-Run 最终余额通知、覆盖/排除边界与 AREA-UI 消费契约。
- `modules/MOD-ReEchoUI.md`：已更新初始化/生命周期绑定、局部刷新、样式保护、GM旧路径清理与专项测试入口。
- `ARCHITECTURE.md`：已审阅、无需修改；此次仍在 ReEcho 主模块内部由 Run 向 UI 发布只读快照，不新增 Runtime Module、反向依赖或状态所有者。
- `README.md`：已审阅、无需修改；MOD-ReEcho、MOD-ReEchoUI、AREA-Run 和 AREA-UI 稳定标识及目录路由未变。
