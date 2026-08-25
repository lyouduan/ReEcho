# Plan 97 - 程序 - 商店分级卡组三选一

## 协调

- Planner / Executor：Codex（按用户要求合并）。
- 任务状态：`Closed`。
- 人工验收：`AcceptedByUser`。
- 基线：`origin/main@c3342804`。
- 工作分支：`plan/97-tiered-card-pack-choice`。
- 工作树：`C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan97-tiered-card-pack-choice`。
- 依赖：Plan 88/91 的卡牌资格、稳定商店页、统一购买事务；`ShopTiers` 继续是每关投放等级的唯一数据真源。
- Writes：
  - `plans/97-tiered-shop-card-pack-choice.md`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardTypes.h`
  - `Source/ReEcho/Public/Run/ReEchoShopCatalog.h`
  - `Source/ReEcho/{Public,Private}/Run/ReEchoRun{SaveGame,Subsystem}.*`
  - `Source/ReEcho/{Public,Private}/UI/ReEcho{InventoryShop,TraitCardChoice,TraitCardEntry}Widget.*`
  - `Source/ReEcho/{Public,Private}/ReEchoGameMode.*`
  - `Source/ReEcho/Private/Tests/ReEcho{Shop,ShopLogicBlock,SaveGame,TraitCardChoice}Tests.cpp` 中实际存在且受影响的文件
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
- 影响模式：`SharedContract`。商店卡牌从“固定等级槽直接卖一张卡”改成“固定等级卡组入口，进入该组的付费三选一”。
- 兼容承诺：武器、符文、免费过关选卡、折扣、统一购买审计和 `ShopTiers` 关卡配置保持不变；旧保存的单卡缓存不再作为当前权威，加载后确定性重建新卡组页。
- 明确排除：本 Plan 不接入正式卡组图片，不修改 XLSX 投放表，不改变免费投放卡牌的选择规则，不允许跨等级补位。

## 锁定目标

1. 商店卡牌区永远显示一级、二级、三级三个固定入口；入口只显示等级和状态，不显示具体卡牌图标，为后续专用卡组图预留接口。
2. `ShopTiers` 决定本关哪些入口可用。未配置显示“未投放”；配置但无候选显示“售罄”；成功购买后显示“已购”。
3. 点击可用入口打开与过关免费投放一致的选择层，最多展示三张同等级剩余卡牌，每张使用自己的价格；不足三张时展示实际数量，零张不打开。
4. 玩家一次只能购买其中一张。成功后该等级入口本页不可再购；失败或取消保持同一批候选与未购状态。
5. 一级卡已拥有仍可重复投放；二、三级卡排除已拥有卡；所有候选继续通过 Cards 统一资格判断。
6. 商店刷新执行整页刷新：三个卡组候选全部按新刷新序号确定性重建，并清除本页各卡组“已购”状态。武器/符文刷新语义保持不变。
7. 当前页三个卡组的候选、价格来源键与购买状态随存档恢复；恢复、打开/关闭选择层和购买失败均不得暗中重摇。

## 架构影响与设计决策

- Cards 的 `FReEchoCardRuntimeState` 保存固定三个等级卡组的候选 CardId 数组和购买状态；Run 根据关卡、刷新序号及 Cards 资格生成并验证缓存。
- Run 对 UI 投影固定三个 `FReEchoShopCardPackOffer`；候选拥有独立 ItemId、价格和卡牌说明。购买仍只经 `PurchaseShopItemDetailed`，事务成功后才提交卡组已购状态。
- InventoryShop 只负责卡组入口和状态；TraitCardChoice 复用展示布局并增加“商店付费模式”、返回商店和购买失败恢复交互。
- GameMode 负责在商店与选择层之间切换焦点；取消/失败不关闭商店或解除暂停，成功关闭选择层并刷新商店投影。
- 保存版本提升一版；旧单卡缓存显式丢弃并按当前关卡/刷新序号重建，不把旧的一张卡伪装成新卡组三选一。

## 锁定验收

- [x] 商店固定显示三个等级入口，无具体卡牌 icon；各入口严格对应一级/二级/三级。
- [x] 第1至第8关入口启用情况严格匹配 XLSX `ShopTiers`；未投放、售罄、已购状态准确。
- [x] 点击入口最多出现三张同级候选；不足三张不补位，零张售罄；每张显示自己的有效价格。
- [x] 一级允许重复，二三级排除已拥有；同一批内无重复卡；页面生成后经其他途径新获得的二三级候选也会从投影中剔除，但不会补抽。
- [x] 成功购买只获得所选卡并把对应入口置为已购；同包第二次购买被拒绝；其他入口不受影响。
- [x] 取消、余额不足或其他购买失败保持原候选与未购状态；再次打开不重摇。
- [x] 全刷新同时重建所有卡组并清除已购状态；新页仍满足等级和资格规则。
- [x] 保存/加载恢复同一候选和已购状态；旧保存安全重建。
- [x] 聚焦自动化、Development 构建、项目校验与 `git diff --check` 通过。

## 实现步骤

1. 将单 CardId 页缓存迁移为三个持久化卡组状态；实现严格验证、旧存档迁移和确定性候选/价格投影。
2. 改造统一购买事务，使卡组状态在卡牌 grant 成功后原子置为已购；刷新重建整页。
3. 将商店卡牌区改为三个无卡牌 icon 的等级入口，并发布打开卡组事件。
4. 扩展卡牌选择层的商店模式、独立价格、取消和失败恢复；GameMode 接通购买结果与焦点/暂停生命周期。
5. 更新自动化测试和模块文档，执行格式化、构建与静态检查，提供 PIE 手测路径。

## 执行记录

- 2026-08-25：用户确认三个卡组各自独立；每次成功购买一张后该卡组显示已购；刷新为全刷新；不足三张显示实际剩余；价格规则保持不变；失败不消耗；保存当前候选及已购状态。
- 2026-08-25：从最新 `origin/main@c3342804` 创建本 Plan 专属工作树并开始实现。
- 2026-08-25：实现 SaveVersion 16 卡组页状态、统一购买事务、固定三级入口、复用三选一卡牌层及旧保存迁移；补充页面生成后所有权变化的动态过滤，保持缓存不重摇、不补位。
- 2026-08-25：Development Editor 构建、`ReEcho.Shop`、`ReEcho.UI.Shop`、`ReEcho.Run.SaveV15ShopCardPageMigratesToTierPacks`、`validate_project.py`、prebuilt 校验和 `git diff --check` 通过。完整 `ReEcho.*` 回归触发与本 Plan 无关的既有数据/资产断言（敌人双形态、卡牌总数、武器符文、VFX local-space 等），并最终在旧武器测试 fixture 断言处终止；未扩大本 Plan 修复范围。
- 2026-08-25：PIE 发现商店 `Screen`(95) 遮挡卡组 `BuildChoice`(90)。将 `BuildChoice` 调整为 98，保持低于 Pause(100)，并新增层级回归断言。新增 Shipping 可用的 `UIInteractionAudit.log`，统一记录所有框架按钮点击与页面创建/复用/关闭/聚焦；动态 IndexedButton 自审计，卡组链路额外记录按钮状态、GameMode 接收、拒绝原因、候选及打开成功。
- 2026-08-25：层级与日志修复通过 Development Editor 构建、`ReEcho.UIManagerSubsystem`、`ReEcho.UI.Shop`、`ReEcho.UI.ButtonVisualFeedback`、项目静态校验、prebuilt 校验及 `git diff --check`；按钮测试同时验证审计文件可读且包含 screen/widget/button 上下文。
- 2026-08-25：用户 PIE 确认表现正常，同意推送并合入远端主分支，Plan 关闭。
