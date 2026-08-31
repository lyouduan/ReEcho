# Plan159 两个蓝图微调发布

## 范围与来源

- 用户请求：发布本地159中已保存的商店刷新键与时间碎片位置微调，不继续不规则文本框工作。
- 批准基线：`f62541505fb875269e19b6b9ec84eec59671c72b`；发布前 fetch 后远端 main 仍为该提交，无传入规则、源码、配表或资产变动。
- 用户资产快照：`08ab1e71`，仅修改 `Content/ReEcho/UI/WBP_ReEchoInventoryShopScreen.uasset` 与 `Content/ReEcho/UI/WBP_ReEchoPlayerHud.uasset`。
- 在同一本地159工作树中建立独立发布分支 `publish/plan159-blueprint-layout`，从批准基线精选上述两个资产，得到 `cdd3da4f`；逐路径比对与用户快照完全一致。
- 原 `fix/shop-balance-events` 分支保留用户快照及 `0c3c8017` 校验修复；本次不合入该修复，不修改生产 CSV、XLSX、运行时代码或其他 Blueprint。除两个用户资产外，只提交发布记录与规则要求的全量构建精选产物。

## 准确静态例外

- 用户获知“主线既有静态校验问题；配表和校验修复不夹带；完整构建和发布锁照常；XLSX/CSV 不一致仍保留，后续导表可能覆盖配置”，并在建议先问程序后明确回复“是的”。
- 例外仅针对上述基线与两个资产快照：静态清单缺少已实现的 `Card.EasterShardThreshold` / `CritNegateAmplification`，以及 `cards.csv`、`card_effects.csv` 的 XLSX 同步漂移，记录为经人工确认暂缓、未通过。不扩展为其他错误或新基线的豁免。
- 不重新导表，不修正/绕过校验器。已知静态失败不被冒充通过；完整构建、预构建匹配、相关资产加载、LFS、diff和发布锁门禁保留。

## 架构与验证

- 影响面：`MOD-ReEcho` 的 `AREA-UI`，商店与玩家HUD资产布局；仍以WBP为几何权威，运行时绑定和余额通知契约不改。
- `MOD-ReEchoUI.md` 已审阅，现有作者化布局/只读数值投影说明仍适用，无需修改；没有源码、模块拓扑、公共契约或稳定标识改变，`ARCHITECTURE.md`、`README.md` 和 `MOD-ReEcho.md` 不需要更新。
- 用户已完成微调并要求发布，视觉决定以该已保存快照为准；AI仅验证加载和已有自动化契约，不替代人工视觉验收。
- 发布状态：待获取锁后 merge 最新 main，执行 Development FullRebuild 和相关资产/测试检查，再提交精选产物并普通推送 main。

## 最终集成与证据（2026-08-31）

- 正式候选 `30198fa4` 通过空 expected lease 原子创建 `main-publish-lock`，随后核验锁值、重新 fetch 并 merge 最新 `origin/main@f6254150`（Already up to date）。原校验修复 `0c3c8017` 不是本次候选祖先，源码、scripts、配表与批准基线无差异。
- Git-common Unreal 锁下执行 `Build-Editor.cmd -Configuration Development -FullRebuild`：96/96 actions 成功，41.79秒；UE5.8 Build ID `55116800`，7模块精选包刷新与 `prebuilt_editor.py check` 均通过，源码指纹保持 `8ebca95f113e3d8fb3c8bf3683f634f5f0e44bf975b59bdab856863bb1606afc`。
- `ReEcho.Shop+ReEcho.UI.Shop+ReEcho.Traits+ReEcho.Run.Save` 原45项全部通过，包含真实商店WBP加载、作者化刷新文字样式保留、余额绑定、存档和卡牌行为测试。
- 本次额外增加可选 `ReEcho.UI.CombatHud.Formatting` 检查，整体46项中45通过、1失败，进程退出255；不称全套通过。唯一失败为原生HUD对象 `SetTimeShards(-5)` 后旧断言期待0，现有实现直接保存-5。此断言发生于加载用户WBP之前，对应原生函数和测试与 `f6254150` 逐文件diff为0；实现来自既有 `e99a8431`。后续真实玩家HUD的加载、初始化、27/22数值绑定断言未报错。按PROJECT_RULES验证矩阵，额外功能自动化为补充证据，不作为新发布门禁；记录旧失败，不借静态例外豁免它，不修改测试或玩法。
- 完整静态检查退出1，复现 `Card.EasterShardThreshold` 登记漏项；独立 `sync_xlsx_to_csv.py --check` 退出1，复现 `cards.csv` / `card_effects.csv` 漂移。两者依本次具名确认暂缓，无重新导表或资产重写。
- LFS还原、fsck、diff检查通过；构建/测试后两个资产与用户 `08ab1e71` 快照逐路径diff仍为0。测试进程已结束并释放持有的Unreal锁，没有关闭用户编辑器。
- 证据保存在 `Saved/Plan159LayoutPublication/`，同时归档至Git common directory的 `reecho-evidence/plan159-layout-20260831-30198fa4/`。最终提交只追加本记录和规则允许的7模块DLL/manifest；普通push前再次核验main祖先和准确锁值，发布成功后精确核验main并以准确lease释放锁。
- 本地 `fix/shop-balance-events` 保留未发布的校验修复与原用户快照，不删除未合并工作；本次不清理其他任务目录。
