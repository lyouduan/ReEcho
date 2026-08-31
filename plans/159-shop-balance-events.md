# Plan 159 - 程序 - 商店最终余额事件同步

## 协调

- Planner / Executor：JosephLE910 + Codex，同一 AI 规划、实现和自审。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`（现有商店与选卡叠层交互回归）。
- 规划基线：`ac4bbb3e`（origin/main）；实现可组合本对话上一轮的本地提交 `8b36a501`（刷新文字作者化）。该组合不随本次 Plan-only 发布进入远端。
- 本地实现方式：独立 `ReEcho-plan159-shop-balance-events` / `fix/shop-balance-events`。
- 依赖 / 阻塞：既有 `Card.EasterShardThreshold` 静态校验白名单失败；用户于 2026-08-31 明确允许仅本次 Plan 文档发布跳过这一项既有失败。实现构建、专项测试和后续实现发布不继承豁免。
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

- [ ] 购买武器/符文、购买卡包、卡槽刷新、主商店刷新、正常/GM授卡的碎片变化、结算收益与债务、重开/读档初值均有正确通知或初始化。
- [ ] 每笔发生余额变化的完整事务最多一次最终通知；无变化与原子失败不广播；组合状态一致。
- [ ] 买卡包→刷新卡牌→返回商店，现金、债务、显示余额和购买可用性同步。
- [ ] GM改余额、债务变化不重置第二页、不改变商品身份与刷新序列、不关闭已打开子层。
- [ ] 关闭商店不再接收回调，重新打开只有一个订阅并读取最新余额。
- [ ] 保留 WBP 作者文字样式和既有按钮悬停交互；清理旧余额写入路径。
- [ ] 格式、构建、聚焦测试、LFS与 diff 检查有真实证据；既有失败如实记录，不放宽验证。

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

待实施。

### 架构文档审阅结果

待实现后逐项记录。
