# 商店卡组投放图标

## 目标与边界

- 用户于2026-09-01要求：商店卡牌存在投放时，不再显示锁图，改用其提供的透明底三卡叠图。
- 基线：`origin/main@4d2656568956a8953c155a1cba78de6737eb550c`；独立工作树 `ReEcho-shop-card-deployed-icon` / `fix/shop-card-deployed-icon`。
- 只改变左侧三个卡组商品位 `DesignerPackOfferIcon0..2` 的有效投放图标；价格、卡组等级、购买、选择、刷新、锁定与右侧已拥有卡牌槽逻辑不变。

## 设计与影响面

- `MOD-ReEcho / AREA-UI` 继续拥有运行时投影；新增独立纹理 `T_UI_Shop110_CardPackOfferIcon`，有效 `FReEchoShopCardPackOffer` 使用它，缺少投放时仍按现有逻辑隐藏槽位，不复用锁图冒充有效商品。
- Blueprint 的三个作者化样例也使用新纹理，保证 Designer 所见即所得；C++只填真实投放状态，不改几何和尺寸。
- Writes：纹理SourceArt和uasset、`WBP_ReEchoInventoryShopScreen`、窄导入/作者脚本、InventoryShop Widget C++、聚焦测试、Plan110审计、`MOD-ReEchoUI.md`、本记录和匹配Editor预构建包。
- 已审阅 `ARCHITECTURE.md` / `README.md`：无模块、依赖、稳定标识或路由变化，无需修改。没有配表、Schema、存档或公共API变化。

## 验证目标

- 纹理为UI组、透明通道保留；三个Designer卡组样例和运行时有效投放都使用新卡叠图。
- 锁图资产保留，右侧装配树与商店玩法状态不受影响。
- 运行导入/作者与Plan110审计、Development构建、`ReEcho.UI.Shop.AuthoredLayoutHosts`、项目静态/LFS/diff检查。

## 当前交付（2026-09-01）

- 用户将本轮门禁收窄为“完成可测试的必要条件”：导入/保存资产并完成Development构建后直接人工测试；明确不要求本轮先跑审计、自动化或额外静态验证。
- 实际提供的图片位于 `C:/Users/gavynqiu/Pictures/task_3504536_1_精细抠图_透明底.png`（原消息中的分层目录不存在）；已复制为规范SourceArt并导入独立Texture2D，未覆盖原锁图。
- 导入脚本已把三个 `DesignerPackOfferIcon0..2` 样例写为新卡叠图并编译、保存 `WBP_ReEchoInventoryShopScreen`；运行时对有效卡组报价使用同一新纹理。
- `scripts/ue/Build-Editor.cmd -Configuration Development` 成功；UE 5.8 Build ID `55116800`，7模块预构建包已刷新，源码指纹 `2e10bf43e711`。仅有既存 `CompressImageArray` 弃用警告。
- 按用户要求未执行：Plan110审计、自动化测试、项目静态检查和人工PIE结论；这些项目不得声称通过，待用户在本工作树测试。
- 首次人工测试发现固定Tier的 `NotOffered` 条目也被误判成投放，因为数组中存在状态条目不等于有可打开候选。修订为以 `CanOpenChoices()` 为唯一视觉判定：可购买/已付款待选且候选非空显示卡叠图，其余继续显示锁图；Designer样例改为第一槽卡叠图、后两槽锁图。
- 用户于2026-09-01复测确认修订表现“没问题”，并明确授权推送远端合入主分支；人工验收为 `Passed`。进入发布流程后恢复执行默认发布门禁，不沿用此前仅为快速交测试而跳过的验证。
- 发布前补验：Plan110蓝图/纹理审计成功；`ReEcho.UI.Shop.AuthoredLayoutHosts` 1/1通过；`python scripts/validate_project.py`（含XLSX/CSV）、预构建包检查、LFS对象检查与 `git diff --check` 均通过。开发构建指纹 `bad1ff64f075`，最终发布仍须在合入最新main后执行FullRebuild。
- 发布锁候选 `db6128807b938e4dc636d63534fdd64ce12d6688`；获锁后重新fetch，`origin/main` 仍为批准基线 `4d265656`，无传入提交、物理/逻辑冲突、耦合或Plan编号变化，merge结果为already-up-to-date。
- 最终 `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` 成功；UE 5.8 Build ID `55116800`，7模块，源码指纹 `bad1ff64f075`，仅有既存 `CompressImageArray` 弃用警告。最终聚焦自动化1/1、项目静态/XLSX-CSV、LFS与diff检查通过；证据在 `Saved/CardPackOfferIconRelease/`，日志不提交。
