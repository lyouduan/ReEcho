# Plan 160 - 程序 - 符文背包连锁合成与持久化修复

## 协调

- Planner 负责人：Codex（程序路线，规划执行合并）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
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

- [ ] 买首个 I / 核心宝石仅入包，空槽不自动装备；异武器符文仍入包。
- [ ] 装备 I + 买 I -> 装备 II，其他槽不变；背包 I + 买 I -> 背包 II。
- [ ] 装备 II + 背包 I + 买 I -> 装备 III，材料正确扣除；背包可连续 I->II->III。
- [ ] 手动替换、卸下与换武器导致的入包也调用同一合成结算；双槽升级保持对应槽位。
- [ ] 新局份数清零；同一实例/跨实例存读档恢复已购页，不能重买已购报价或 III 对应 I；旧档兼容迁移覆盖。
- [ ] 原子失败测试证明坏配方/无效升级装备不扣钱、不消耗材料；重复重取投影不改变玩法或报价。
- [ ] 聚焦自动化、构建、静态检查通过；人工验证背包更新和原槽升级。
- [ ] 不提交允许列表之外的生成物，不修改生产表或主目录已有 WBP 未提交内容。

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

- 待实现。

### 证据

- 2026-08-31：用户确认两项边界；已完成只读代码/配方审计，独立工作树创建，LFS 检出通过。
- 2026-08-31：准确基线 `ac28c32d` 的 `validate_project.py` 报 `card_effects.csv:78 Card.EasterShardThreshold` 未注册，但 C++ 目录已有该行为注册。本次只有新增 Plan，用户明确批准仅对 Plan160 纯文档发布豁免此项既有校验失败；记录为未通过，不豁免后续实现的构建/针对性测试。`git diff --check` 通过。

### 剩余风险

- 旧档没有保存的历史购买事实不能完整重建，采用最小可靠拥有集合与当前页过滤，不伪造额外副本。

### 人工验收结果/请求

- `PendingBeforeClose`：完成构建后交付具名清单。

### 架构文档审阅结果

- 实现完成后填写。
