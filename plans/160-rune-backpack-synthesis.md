# Plan 160 - 程序 - 符文背包连锁合成与持久化修复

## 协调

- Planner 负责人：Codex（程序路线，规划执行合并）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`（已合并 main 并完成发布完整重建；商店与存档回归通过，发布暂等 XLSX/CSV 既有漂移的具名处理决定）。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@ac28c32d796fedfa486ff215d4403dc26a29099f`。
- 本地实现方式：独立 worktree `ReEcho-plan160-rune-backpack-synthesis`，分支 `plan/160-rune-backpack-synthesis`。
- 依赖 / 阻塞：用户已确认背包合成后的产物继续与已装备同级合成；六种核心宝石保持单级且购买后入包。构建前检查同克隆 Editor/锁状态。
- Writes:
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`
  - `Source/ReEcho/Public/Run/ReEchoRunSaveGame.h`
  - `Source/ReEcho/Private/Run/ReEchoRuneInventory.*`（按需新增无副作用合成规划器）
  - `Source/ReEcho/Public/Run/ReEchoShopCatalog.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/UI/ReEchoInventoryShopWidget.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoRuneInventoryTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoSaveGameTests.cpp`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `plans/160-rune-backpack-synthesis.md`
  - `Binaries/Win64/` 中 `GIT_RULES.md` 允许的精选 Editor 构建包。
- Stable Reads：`Weapons/ReEchoWeaponRuntime.*`、`Data/ReEchoCsvDataSnapshot.h`、生产 `parts.csv/part_effects.csv/rune_upgrades.csv`、商店购买审计及 Blueprint 作者化 UI。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：继续使用现有 31 族 I/II/III 与 62 条 2 合 1 配方，不改数值或 CSV/XLSX；核心宝石不分级、不重复购买。已装备符文不因无关购买被挤出，商店未购报价不重新抽取；付费、赊账、免费购物及其他卡牌行为继续沿原权威入口。存档版本增加并保守迁移旧版本。
- 明确排除：不修改核心宝石等级、符文效果、价格、刷新预算、普通卡牌、商店美术或自动发布实现到 main。

## 锁定目标

1. 新购买符文若能与当前已装备的同 ID、同等级符文直接合成，优先合成并把高一级结果放在该原槽位；不挤占其他槽。
2. 否则购买结果只进入背包，包括空槽、不同符文、异武器专属符文和六种单级核心宝石，均不自动装备。
3. 每次物品进入符文背包（购买、手动替换/卸下、换武器导致卸下）后统一结算：先完成背包内合成，再结算背包与已装备符文的合成，直到不再变化。背包内部产物未与装备合成时继续留包；与装备合成时保持原槽位置。用户明确例子：装备 II、背包 I、再买 I，最终装备 III。
4. 所有合成按配方消费真实份数；所有权集合是份数的投影，装备占用包含在总份数中但不能重复计数。购买、合成、装配候选在校验成功后原子提交；失败不扣钱、不消耗材料、不保留半套装备。
5. 新局清空符文份数和本页购买记录。存档保存/恢复份数与当前页已购记录；旧档没有份数时按可靠拥有/装备恢复最小份数，不凭历史虚构额外副本。
6. III 持有排除同时应用于新生成报价、缓存报价投影和购买入口；恢复旧商店页不能绕过上限。已购标记缺失的旧档只按可证明的已有记录保守迁移，不重摇其他报价、不重置刷新次数。
7. 装配变化和背包候选通过统一 Run 投影刷新 UI；新增日志仅扩充既有购买审计/合成摘要，不另建玩法状态来源。

## 架构影响与设计决策

- 受影响标识：`MOD-ReEcho`、`AREA-Run`、`AREA-Weapons`、`AREA-Recording`、`AREA-UI`，UI 路由文档 `MOD-ReEchoUI`。
- 对应模块文档：上述两份 `MOD-ReEcho.md`、`MOD-ReEchoUI.md` 已加入 Writes。无新增 Runtime Module，不改变依赖方向。
- 设计意图：Run 继续拥有份数、背包/装配、报价和存档事务；可测试的纯合成规划器只生成候选份数/有序装备 ID，不发布事件。最终装备仍经 `ReEchoWeaponRuntime::TryEquipParts` 编译全部效果后提交，UI 不做合成判定。
- 权威状态：`RuneAcquisitionCounts` 表达总份数（含装备），`OwnedPartIds` 是去重所有权投影，`BuildSnapshot.EquippedParts` 表达有序装配；背包份数由总份数减装备占用推导，避免新增可分叉背包副本。
- 决策：按稳定配方顺序结算；直接装备合成优先于普通入包路径，背包产物再与装备连锁。替换原槽而不是旧的“挤出最早符文”。所有算法只消费配方且必须严格减少物品总数，拒绝非法/循环配方。
- 文档同步范围：维护 `MOD-ReEcho.md` 的购买即装旧叙述与存档规则、`MOD-ReEchoUI.md` 的只读背包/原槽升级规则；关闭前审阅 `ARCHITECTURE.md` / `README.md` 是否需要变化，无变化记录具名结论。

## 锁定验收

- [x] 买首个 I / 核心宝石仅入包，空槽不自动装备；异武器符文仍入包（自动化）。
- [x] 装备 I + 买 I -> 装备 II，其他槽不变；背包 I + 买 I -> 背包 II（自动化）。
- [x] 装备 II + 背包 I + 买 I -> 装备 III，材料正确扣除；背包可连续 I->II->III（自动化）。
- [x] 手动替换、卸下与换武器导致的入包也调用同一合成结算；双槽升级保持对应槽位（自动化）。
- [x] 新局份数清零；同一实例/跨实例存读档恢复已购页，不能重买已购报价或 III 对应 I；旧档兼容迁移覆盖（自动化）。
- [x] 原子失败测试证明坏配方/无效候选不扣钱、不消耗材料；重复重取投影不改变玩法或报价（自动化）。
- [ ] 聚焦自动化、构建、静态检查通过；人工验证背包更新和原槽升级。
- [x] 本任务改动仅包含计划源文件/文档及允许的七模块精选包；未修改生产表或主目录已有 WBP 未提交内容，实现与最新 main 集成已提交本地。

## Step 0 门禁

- 基线：`origin/main@ac28c32d`；远端最高编号 159，分配 160，Plan-only 按规则独立发布。
- 已审计：Run 现有购买先装备再合成；计数缺少 StartRun 重置；已购集合未进入 SaveGame；缓存只检查 Enabled/ShopEnabled。最新 main Plan159 货币事件同步已在基线内，本任务保留其最终余额通知。
- LFS：新工作树 `setup_lfs.py --check` 通过。源码读取无新资产需求。
- 现有测试：尚未执行本次基线 UE 自动化；新增测试使用生产快照加局部覆盖，旧“购买即装备”断言只按此次批准语义更新。
- 停止条件：真实产品歧义、其他任务改写同一存档/合成契约需要取舍、Editor 未关闭或锁被占用时暂停相应步骤。

## 实现提纲

1. 固化符文份数及新局/存档约束，抽出候选合成规则和针对性测试。
2. 接入购买入包、直接原槽升级、背包与装备连锁；覆盖卸下/替换/换武器入口和失败回滚。
3. 修复已购页存档和 III 缓存/购买资格；更新背包只读投影与日志。
4. 编译、测试、核对文档并提供人工验收清单；不在本轮自行发布实现。

## 验证矩阵

| 层级 | 检查 | 证据 |
|---|---|---|
| C++ | 仓库 `.clang-format`、`Build-Editor.cmd -Configuration Development` | UHT/UBT、匹配精选包 |
| 规则/事务 | `Run-Automation.cmd -Filter ReEcho.Shop.Rune` | 购买入包、合成、失效报价、原子失败 |
| 存档 | `Run-Automation.cmd -Filter ReEcho.Run.SaveSnapshot` 及本计划新局/恢复测试 | 份数/已购页/旧档迁移 |
| 静态 | `validate_project.py`、`git diff --check`、LFS 检查 | 只记录实际通过项，基线失败单独说明 |
| 人工 | 商店购买、背包操作、双槽升级、存读档 | 用户验收背包更新与槽位表现 |

## 执行记录

### 变化

- 新增无副作用 `ReEchoRuneInventory::TrySettle`，按稳定配方顺序结算，校验同族相邻等级/2 合 1，并对非法份数、循环/跨族配方、溢出拒绝输出候选。
- Run 的 `TryResolveRuneInventory` 统一购买、显式装配和换武器回包路径；最终候选经 WeaponRuntime 重编译效果后才提交。新购匹配装备直接升级原槽，其余只入包；背包级联完成后继续与装备级联，不自动挤占无关装备。
- 新局与清空全部符文的卡牌效果同步清理份数；SaveVersion 26 保存份数和当前页已购集合，恢复时重置旧实例残留，旧档从合法拥有/装备恢复最小份数并过滤孤立计数。
- III 持有过滤覆盖新报价、缓存投影、购买入口；读档不重摇其他槽。删除 GameMode 购买按钮的免费重装捷径，背包装备仍走独立命令。
- 背包投影新增真实 `BackpackCount`，列表与购买审计减去装备占用再显示份数。总份数含装备，不重复计算。
- 新增六项 `ReEcho.Shop.Rune.*` 自动化；同步现有购买入包断言与已购页/未购页稳定性断言。相关弓/镰刀/剑符文旧测试使用退役无等级 ID，更新为相同符文族的 I 级，未削弱兼容性或装备断言。

### 证据

- 2026-08-31：用户确认两项边界；已完成只读代码/配方审计，独立工作树创建，LFS 检出通过。
- 2026-08-31：准确基线 `ac28c32d` 的 `validate_project.py` 报 `card_effects.csv:78 Card.EasterShardThreshold` 未注册，但 C++ 目录已有该行为注册。本次只有新增 Plan，用户明确批准仅对 Plan160 纯文档发布豁免此项既有校验失败；记录为未通过，不豁免后续实现的构建/针对性测试。`git diff --check` 通过。
- 2026-08-31：Plan-only 已发布 `a7eb82a5`；实现停留本地工作树，未推送实现。
- 2026-08-31 21:20：Development Editor 构建成功（最后已验证源码指纹 `1dffa0505044`），七模块精选包 `prebuilt_editor.py check` 通过；仅现有 `CompressImageArray` API 弃用警告。六项新增符文测试全部 `Success`，覆盖 31 个活跃族、双槽原位升级、跨武器回包、存档内存序列化和失败不扣费。`ReEcho.Shop` 扩展运行 23 项、19 成功、4 失败。
- 2026-08-31 21:22：用主目录未修改源代码的预编译包（`source=f040da08d1c2`，HEAD `ca635500`）复核旧商店回归，17 项中 6 项失败；与本任务基线相比 Run/ShopTests/相关卡牌数据无差异。复现卡牌折扣/购买奖励旧数值、旧卡组投放/价格/单购假设、退役符文 ID 和已购页断言问题。未改卡牌玩法或表格来迎合旧测试；本任务只同步相关符文夹具与批准语义。
- 2026-08-31：最后两项退役符文夹具 ID 更新后，验证曾被 `fix/bow-kill-haste-gate` 的锁和 Plan158 编辑器占用阻挡，未删除他人锁或关闭其进程；用户明确关闭后才继续最终验证。
- 2026-08-31 21:30：用户关闭 Editor；确认无 Editor/Cmd 进程与锁后以 CreateNew 获取同克隆锁，完成最终 Development Editor 构建，`source=7164884ae23f`，七模块精选包 `prebuilt_editor.py check` 通过。完成后仅释放本任务持有的锁。远端规则与 Plan-only 基线相比无变更。
- 最终 `ReEcho.Shop`：23 项 / 21 Success / 2 Fail；六项新增 `ReEcho.Shop.Rune.*`、当前武器背包过滤、武器切换、连续购买页稳定及购买后手动配装均通过。剩余 `CardRulesAreAtomicAndPersistent`、`PageUsesFixedPartSlotsAndConfiguredCardTiers` 使用旧卡牌折扣/奖励/投放/卡组购买次数假设（含未排除彩蛋候选的旧数量断言），已在主分支复核其失败，不在本任务改卡牌规则或降低断言。证据：`Saved/Logs/Plan160-Shop.log`，基线：`Saved/Logs/Plan160-BaselineShop.log`。
- 最终 `ReEcho.Run.SaveSnapshot`：1 项 / 1 Success，证据 `Saved/Logs/Plan160-SaveSnapshot.log`。LFS 和 `git diff --check` 通过；`validate_project.py` 仍报告原有 `Card.EasterShardThreshold` 注册扫描失败，保持未通过。最终所有实现及配套夹具已进入上述构建，未推送。

### 剩余风险

- 旧档没有保存的历史购买事实不能完整重建，采用最小可靠拥有集合与当前页过滤，不伪造额外副本。
- 发布集成后，原 `Card.EasterShardThreshold` 注册缺项和两项旧商店断言已由传入 main 修复；当前完整静态校验在后续 XLSX/CSV 同步检查失败：`card_effects.csv`、`cards.csv`。本任务的表格、CSV、导出/校验工具与 `origin/main@2d6d6da2` 完全相同，未自行重导数据或扩大原豁免范围。以后重新导表可能改变现有卡牌配置，需程序/策划核对或本次准确授权暂缓。

### 本次发布审计

- 用户在收到验收清单后明确要求推送并合入 main，并批准仅豁免已告知的既有 `Card.EasterShardThreshold` 静态检查失败；完整重建、相关回归与发布锁不豁免，不把失败记为通过。用户未另行要求清理本工作树。
- 2026-08-31 发布前 fetch：`origin/main@2d6d6da2`。相对实现基线，传入 Plan159 完整余额事件/Designer-owned 刷新文字、用户商店/HUD 资产布局，以及 Plan158 Boss 血环与选卡 UI。它们不改符文等级、份数或合成规则；本地不覆盖主目录未提交内容，不回退传入资产。Plan160 编号已单独发布，无编号冲突。
- 重叠：Run/ShopWidget/GameMode 的余额通知与整页投影、旧 ShopTests 夹具修正、两份模块文档及精选二进制。保留远端事件生命周期与作者布局，组合本地购买入包/合成候选；相同已购页测试合并保留报价缓存稳定与消费后不可购买两种断言。生成包不作二进制内容拼接，最终 FullRebuild 统一刷新。
- 最新 Plan158 已补齐该行为 ID 与 `CritNegateAmplification` 校验注册。获锁合并后重跑完整校验；如果已通过则本次不实际使用上述豁免。其他新失败不会套用该授权。
- 发布锁已按空 expected lease 原子取得，初始候选 `bc869908500386ac40d19e8ffa8fb37674a2696e`；获锁后 fetch 并 merge `origin/main@2d6d6da2`，集成提交 `9bbb45bb`，未 rebase 或改写锁历史。源码冲突仅 ShopTests 的新购买入包语义与页稳定断言，按已批准契约组合；远端所有资产保持原样。
- 2026-08-31 22:29：同克隆无 Editor 进程后持有 Unreal 锁执行 `Build-Editor.ps1 -Configuration Development -FullRebuild`，96 actions 完成，结果 `Succeeded`。最终七模块预构建包 `source=f8fad78722ec`，BuildId `55116800`，指纹/文件哈希验证通过；只出现既有 `CompressImageArray` 弃用警告。
- 发布集成回归：`ReEcho.Shop` **25/25 Success**，包含全部六项符文专项、余额事务和旧卡牌回归；`ReEcho.Run.SaveSnapshot` **1/1 Success**。证据分别为 `Saved/Logs/Plan160-PublishShop.log`、`Saved/Logs/Plan160-PublishSaveSnapshot.log`。新增登记校验单测 **3/3** 通过，原静态注册缺项豁免未实际使用。
- 额外 UI 集成回归 `ReEcho.UI.Shop` **7/7 Success**：作者化布局/刷新文字、余额订阅、卡组选卡呈现、逻辑分块、符文背包过滤及 tooltip 蓝图全部通过，证据 `Saved/Logs/Plan160-PublishShopUI.log`。测试后预构建指纹、LFS hydration/fsck、`git diff --check` 再检通过；仅释放本任务取得的 Unreal 锁，main 发布锁保留。
- 完整 `validate_project.py` 在 XLSX 导出同步处失败；单独 `sync_xlsx_to_csv.py --check` 复现相同两文件漂移。`git diff --exit-code origin/main -- Design/Data Content/Data scripts/data scripts/validate_project.py` 返回 0，证明与当前远端相同输入；后续独立 `validate_workflow()` 通过。没有临时屏蔽校验或修改数据让检查变绿。已向用户报告具体风险并询问是否另外豁免，未取得此项答复前不推 main，保留本任务发布锁。

### 人工验收结果/请求

- `PendingBeforeClose`：打开 `ReEcho-plan160-rune-backpack-synthesis/ReEcho.uproject`（不要打开主目录的旧实现）。验收：首个 I/核心只入包；装备 I + 买 I -> 原槽 II；装备 II + 背包 I + 买 I -> 原槽 III；不匹配装备不被挤掉；替换/卸下/换武器回包也合成；III 阻断同族 I；存读档本页不重复购买；新局首购无幽灵副本。真实 UI 位置/操作手感尚未由 AI 执行 PIE 验收。

### 架构文档审阅结果

- 已更新 `MOD-ReEcho.md` 与 `MOD-ReEchoUI.md`：统一候选事务、购买/背包装配分离、总份数/背包投影及 v26 存档契约。
- 已审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md`：无 Runtime Module 或依赖方向变化，Run 权威不变，无需修改总架构图。
- 已审阅 `README.md`：无启动/构建入口变化，无需修改。主目录两个既有 WBP 未提交修改保持不动；未修改 XLSX/CSV/美术资产。
