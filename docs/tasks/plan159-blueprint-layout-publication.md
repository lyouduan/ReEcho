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
