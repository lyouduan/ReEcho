# Plan 91 - 程序 - 商店分级固定卡牌槽

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner 与 Executor）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`（程序实现与自动化已完成，等待 PIE 人工验收）。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main` @ `f8e8a40bf8ce7974d24b7b296c51bdc4cb35344b`。
- 最终集成基线：`origin/main` @ `8c805aeccff45e7a7ffb5a2982382cb4e36a4a5e`（包含 Plan89 VFX 前置修复；已组合完整重建）。
- 本地实现方式（可选，仅作交接说明）：独立 worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan91-tiered-shop-card-slots`，分支 `plan/91-tiered-shop-card-slots`。
- 依赖 / 阻塞：基于 Plan88 已发布的逐关 `ShopTiers`、稳定商店页和统一卡牌资格；不改变策划 XLSX/CSV 字段或逐关配置。
- Writes:
  - `plans/91-tiered-shop-card-slots.md`
  - `Source/ReEcho/Public/Run/ReEchoShopCatalog.h`
  - `Source/ReEcho/Public/Run/ReEchoRunSaveGame.h`
  - `Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Source/ReEcho/Public/ReEchoGameMode.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Public/UI/ReEchoInventoryShopWidget.h`
  - `Source/ReEcho/Private/UI/ReEchoInventoryShopWidget.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoShopLogicBlockTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoSaveGameTests.cpp`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardTypes.h`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `Binaries/Win64/ReEchoEditor.prebuilt.json`
  - `Binaries/Win64/UnrealEditor-ReEcho.dll`
  - `Binaries/Win64/UnrealEditor-ReEchoAudio.dll`
  - `Binaries/Win64/UnrealEditor-ReEchoCards.dll`
  - `Binaries/Win64/UnrealEditor-ReEchoCombat.dll`
  - `Binaries/Win64/UnrealEditor-ReEchoEnemies.dll`
  - `Binaries/Win64/UnrealEditor-ReEchoPresentation.dll`
  - `Binaries/Win64/UnrealEditor-ReEchoWeapons.dll`
- Stable Reads:
  - `Content/Data/shop_drop_levels.csv`
  - `Design/Data/ReEchoData.xlsx`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardRuntime.h`
  - `Source/ReEchoCards/Private/Cards/ReEchoCardRuntime.cpp`
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：继续以 `ShopTiers` 决定本关哪些等级槽投放；继续复用 Cards 的“1级可重复、2/3级持有排除”资格。缓存数组改为固定 `[Tier1, Tier2, Tier3]` 位置并允许 `None` 空位；读取旧的混合池缓存页时按等级形状失配自动重建。扁平 `Offers` 兼容投影保留三个卡牌位置，空位置不可购买。武器拥有集合及武器/符文稳定商店页新增存档字段并提升 SaveVersion；旧存档以当前装备武器作为最小拥有集合迁移并重建商店页。
- 明确排除：不修改战斗结束免费三选一；不修改卡牌效果、价格区间、刷新成本、逐关 XLSX 配置或商店美术资产；空槽不跨级补卡；不引入“每把武器独立保存一套符文装配”。

## 锁定目标

- 商店卡牌区域始终存在三个固定位置：第1槽只允许1级卡，第2槽只允许2级卡，第3槽只允许3级卡。
- `ShopTiers` 仍是逐关投放开关：未配置的等级槽保持空；配置等级但已无合格候选时显示空/售罄。
- 任一等级槽都不得从其他等级回退或补位；刷新只在各槽自己的等级牌池内重抽。
- 保持1级卡可重复投放/叠加，已获得2、3级卡不得再投放；同一稳定页面购买后不重摇其他槽。
- 商店装配室的大武器区域展示 `CurrentBuild.WeaponId` 对应的手持武器图；点击该区域打开武器背包。
- 武器背包列出本轮已获得的全部有效武器并标记当前装备；选择其他已拥有武器时不扣碎片、不重摇商店，只原子切换当前武器并立即刷新武器图、符文槽和装备说明。
- 初始武器与商店购买武器都进入拥有集合并随本轮存档恢复。切换武器复用 `ReEchoWeaponRuntime::TrySelectWeapon`：兼容符文保留，不兼容符文保持拥有但卸下回背包。
- 武器/符文三个商品槽与卡牌槽一致，在同一 `EncounterIndex + ShopRefreshSequence` 下保持 ID 和价格稳定；购买前一个商品不能让后端重算并拒绝仍显示的后续商品。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho` 的 `AREA-Run`、`AREA-UI`，以及 `MOD-ReEchoCards` / `AREA-Cards`。
- 对应模块文档：维护 `MOD-ReEcho.md`、`MOD-ReEchoCards.md`、`MOD-ReEchoUI.md`，均已列入 `Writes`。
- 设计意图：把“投放等级”固化为槽位身份，避免 UI 或调用方把 `ShopTiers` 再解释为共享混合牌池；Run 继续负责编排页面，Cards 继续唯一负责候选资格。
- 权威状态与依赖：不改变权威拥有者和模块依赖方向。Run 持有 `EncounterIndex + ShopRefreshSequence` 页面缓存；Cards 的运行态只保存固定位置 CardId，UI 只消费只读槽投影。
- 决策记录：三个槽始终构造，Tier 固定为1/2/3；未投放或候选耗尽以不可购买的空槽表示。放弃“只返回已填槽”的方案，因为它会让数组位置随关次变化，重新把等级语义泄漏给 UI。放弃跨级补卡，因为用户已明确禁止。
- 扩展决策记录：Run 继续作为武器拥有与当前构筑的唯一权威；UI 只消费包含武器显示名/图标的只读投影，并通过独立“装备武器”请求交给 GameMode。符文兼容裁剪统一调用 WeaponRuntime，拒绝未知、未拥有或禁用武器时不得改变构筑。武器背包复用现有符文背包的按钮/浮层交互形态，但不复用购买请求，避免把免费换装误判为再次购买。
- 相关文档同步范围：更新三个模块文档的商店投放契约；`ARCHITECTURE.md` 已审阅，本 Plan 不新增模块、不改变拓扑或依赖方向；`README.md` 已审阅，稳定标识与阅读路由不变。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：已记录武器拥有集合、SaveVersion 14、稳定武器/符文商店页与免费切换事务。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`：已记录固定三级槽缓存与资格边界，无需因武器背包再修改。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：已记录当前武器图、武器背包与独立装备委托。
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅、无需修改；模块拓扑和依赖方向不变。
  - `shared/CODEBASE_MAP/README.md`：已审阅、无需修改；稳定标识与路由不变。

## 锁定验收

- [x] 每个商店页的 `CardSlotOffers` 精确为3项，索引0/1/2的 `Tier` 精确为1/2/3。
- [x] 第4关 `ShopTiers=2|3` 时1级槽为空，2级槽只出2级卡，3级槽只出3级卡；第1/5/8关三个槽均为空。
- [x] 对应等级合格牌池耗尽时仅该槽为空/售罄，其他槽不受影响且不跨级补位。
- [x] 1级重复、2/3级持有排除、页面刷新、购买稳定性和存读档行为有自动化证据。
- [x] 商店表现层固定构造三个卡牌位置；空槽不可点击、不可广播购买 ID，并能显示“未投放”或“售罄”。
- [x] 当前装备武器图显示于 Designer 武器区域，点击后能看到包含初始武器和已购武器的背包；当前武器明确标记且不可重复装备。
- [x] 选择已拥有的其他武器不扣费、不刷新报价，切换成功后武器图/名称/符文槽同步更新；兼容符文保留，不兼容符文回背包。
- [x] 未拥有、未知或禁用武器切换被拒绝且 `CurrentBuild` 不变；武器拥有集合存读档与旧存档迁移有自动化证据。
- [x] 同一页三个武器/符文商品可按任意顺序依次购买，购买后其余槽的 ID/价格不变，存读档继续保持当前页面。
- [x] C++ 格式化、聚焦自动化、Editor 构建、项目校验和 `git diff --check` 通过。
- [ ] 用户在 PIE 确认第4关三个槽的等级/空槽表现以及第1关全空表现。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@f8e8a40b`，包含 Plan88 最终投放实现；Plan91 为其公开商店槽契约的后续修正。
- 引擎/构建可用性：Plan88 最终候选已完成 UE 5.8 完整构建，Plan91 仍需在自身最终候选重新构建，旧二进制证据不可直接用于发布。
- 现有聚焦测试结果：Plan88 `ReEcho.Shop` 8/8、`ReEcho.UI.Shop` 2/2；其旧断言是“0或3个混合池报价”，需按本 Plan 新契约替换。
- 共享契约 / 难合并资源风险：`ReEchoRunSubsystem.cpp`、卡牌运行态、商店 Widget 和商店测试均为近期高频路径；Plan 发布后、实现发布前都必须 fetch 审计远端变化。
- 基线损坏时的停止条件：远端新增提交；`ShopTiers` 或卡牌资格事实来源变化；旧缓存无法安全自愈；构建或聚焦测试失败。

## 实现提纲

1. 将卡牌商店视图固定为三个带 Tier 身份的槽，并为未投放/售罄增加明确不可购买状态。
2. 将稳定页面缓存改为三个位置的 CardId；每个配置 Tier 独立建立资格池、独立确定性抽取一张，空池写入 `None`。
3. 读取旧缓存时校验位置等级与本关配置；不匹配则在当前页面键下重建，购买后继续保留页面。
4. 扁平兼容投影和 Widget 固定渲染三个位置；空槽禁用购买并显示状态，不加载不存在的卡图标。
5. 更新逐关矩阵、资格耗尽、刷新、购买和存档自动化，并同步模块文档。
6. 将初始/购买武器纳入可持久化拥有集合，为 Run 增加窄的已拥有武器切换事务，并让购买武器也复用同一 WeaponRuntime 切换规则。
7. 在只读商店视图中投影当前武器图和已拥有武器详情；在 Designer 武器区域叠加可点击武器图，复用符文背包形态构造武器背包并通过独立装备委托刷新整个商店投影。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 数据静态 | `python scripts/data/sync_xlsx_to_csv.py --check` | XLSX 与生产 CSV 无漂移 |
| C++ 格式 | 仓库 `.clang-format` 仅格式化本 Plan 修改的 `.h/.cpp` | diff 无无关格式噪声 |
| 商店逻辑 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Shop` | 固定等级槽、资格、刷新、购买、存档通过 |
| 商店 UI | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.Shop` | 三槽及空槽不可购买通过 |
| 武器背包 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Shop`、`-Filter ReEcho.UI.Shop` | 武器拥有/切换事务、持久化、当前武器图与背包请求通过 |
| Run/卡牌回归 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Run`、`-Filter ReEcho.Cards`、`-Filter ReEcho.Traits` | 页面缓存与统一资格无回归 |
| C++ 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 成功并刷新精选预构建包 |
| 发布构建 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终集成候选完整构建并刷新指纹 |
| 静态 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 项目与精选包一致 |
| 人工 | PIE 检查第1、4关商店 | 固定三个槽、等级和空槽状态符合锁定目标 |

## 执行记录

### 变化

- `FReEchoCardSlotOffer` 增加显式可用状态；Run 始终返回1/2/3级三个槽，逐级解析 `ShopTiers`，每个启用等级独立调用 Cards 资格池并确定性抽取一张。
- 商店稳定页缓存固定为 `[Tier1, Tier2, Tier3]` 三个 CardId，`None` 表示未投放或耗尽；旧混合池缓存的数量/等级形状不匹配时自动重建，不提升 SaveVersion。
- 扁平兼容报价与目标商店表现都保留三个位置；空槽隐藏卡图和价格、显示“未投放”或“售罄”、禁用按钮，并在请求边界拒绝 `None` ID。刷新不再旋转卡牌位置。
- 商店矩阵自动化改为核对槽位等级身份、逐关启用、Tier2 耗尽不影响 Tier3、页面稳定购买和存读档。顺带修正既有随机测试：不再无条件选择可能清空全部货币的 `G_2_15` 后断言货币仍存在。
- 发布门禁发现远端新增 `513ec51b` 的 Plan92 怪物时间碎片掉落计划。经用户确认组合适配后，Plan91 干净变基到该文档提交之上；传入提交没有产品源码、配置或二进制变化。
- 2026-08-24 用户要求继续复用 Plan91 工作分支，新增商店当前武器图与武器背包切换闭环；Plan 状态重新打开为 `InProgress`。清理门禁同时确认 Plan88 工作树干净且 HEAD 已进入 `origin/main`，仅移除其本地工作树目录，保留 Git 分支。
- Run 新增 `OwnedWeaponIds` 权威集合和 `TryEquipOwnedWeapon` 原子事务；初始武器、商店购入武器均进入集合。SaveVersion 提升至13，旧存档以当前武器迁移出最小拥有集合。
- 商店只读视图新增当前武器图与已拥有武器报价投影；Designer 大武器区域叠加透明点击按钮和当前武器图，点击后打开可滚动武器背包。当前武器标记“已装备”并禁用，选择其他武器通过独立装备委托刷新完整稳定商店视图，不扣费、不重摇报价。
- 武器购买和背包切换统一复用 `ReEchoWeaponRuntime::TrySelectWeapon`，因此通用兼容符文保留、武器专属不兼容符文卸下但仍留在符文背包；未知、禁用、未拥有武器在提交前拒绝。
- 最终变基引入 `origin/main@8c805aec` 的 Plan89 VFX 源码/资产与精选二进制；无商店/Run 源码冲突。二进制冲突先采用远端后，在组合源码上完整重建生成最终精选包。
- 修复 Designer Blueprint 与 C++ 同名 `DesignerWeaponPanel` 属性导致的编译冲突；C++ 内部成员改名但保留蓝图控件名。加固既有随机商店测试，使货币断言避开 `G_2_15` 的购买即清零效果并按实际折后价验证。
- 用户 PIE 复现同页第三个枪符文无法购买：界面保留旧报价，但 Run 每次购买都按已缩小资格池重算武器/符文页，导致显示 ID 与购买校验分叉。现将三个武器/符文 ContentId 与价格按关卡/刷新序号稳定缓存，单页不重复，SaveVersion 提升至14持久化页面；购买仅改变已购/装备状态。
- 大武器图由默认 32px Brush 改为源纹理尺寸，透明按钮 Content Slot 强制填充 authored 武器面板，再由 ScaleBox 等比缩放。

### 证据

- 修改文件已使用仓库 `.clang-format` 规则格式化；增量 `scripts\ue\Build-Editor.cmd -Configuration Development` 通过。
- 最新增量候选 `ReEcho.Shop` 10/10、`ReEcho.UI.Shop` 2/2、`ReEcho.Run` 14/14，全部通过；此前最终组合候选 `ReEcho.Cards` 5/5、`ReEcho.Traits` 8/8 及聚焦 `ReEcho.Weapons.RunLockAndEquipmentSnapshotParity` 1/1 通过。
- `ReEcho.Shop.OwnedWeaponBackpackSwitchesAtomically` 覆盖武器购入、双武器拥有、免费切换、兼容/不兼容符文处置、未拥有武器原子拒绝和只读视图；`ReEcho.Shop.WeaponPartPageRemainsStableAfterSequentialPurchases` 覆盖同页三个商品依次购买、ID/价格稳定和 SaveVersion 14 页面恢复；`ReEcho.Run.SaveSnapshot` 覆盖武器拥有集合恢复及 Version 12 迁移；`ReEcho.UI.Shop.LogicBlocks` 覆盖当前武器 Brush 源尺寸、填充布局、武器背包、当前项禁用与装备请求。
- 最终 `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` 通过，97/97 actions 成功；精选 Editor 预构建源指纹 `602db01c81bd`。
- `python scripts/data/sync_xlsx_to_csv.py --check`、`python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check` 与 `git diff --check` 通过。
- 变基到 `origin/main@8c805aec` 后重新执行 XLSX/CSV 同步检查、项目静态校验、精选 Editor 预构建校验与 `git diff --check`；全部通过。变基前恢复点为 `backup/plan91-before-plan92-rebase-20260824`。

### 剩余风险

- 空槽、武器图裁切和武器背包的最终美术样式仍需用户在 PIE 判断；本 Plan 保证状态、交互和基础文字正确，未接入新美术资产。
- 全量 `ReEcho.Weapons` 套件仍包含两个既有、非本 Plan 修改路径的失败：投射物攻击时符文快照断言，以及 `DomainRevisionRejectsChangedTablesAndPinsActiveRun` 依赖已不存在的硬编码 CSV 文本后崩溃；本 Plan 相关的装备快照聚焦用例通过。

### 人工验收结果/请求

- `PendingBeforeClose`：除第1关全空槽和第4关1级空、2/3级有卡外，请在拥有至少两把武器时点击商店大武器图，确认背包、切换、当前标记、符文回包和商店报价不刷新。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已更新：除固定1/2/3级位置外，记录武器拥有集合、SaveVersion 14、稳定武器/符文商店页和免费换装事务。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md` 已更新：记录固定三位置缓存及 `None` 语义，并保持 Cards 统一资格权威。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 已更新：记录空槽展示/禁用，以及当前武器图、武器背包和独立装备委托的边界。
- `shared/CODEBASE_MAP/ARCHITECTURE.md` 已审阅、无需修改：Runtime Module 拓扑、状态拥有者和依赖方向未变。
- `shared/CODEBASE_MAP/README.md` 已审阅、无需修改：稳定标识和代码阅读路由未变。
