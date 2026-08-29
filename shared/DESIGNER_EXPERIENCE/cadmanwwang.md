# cadmanwwang 的 ReEcho 策划经验

## 2026-08-29 07:55 - v1.1 商城、卡牌、角色与Boss更新

- 结果：成功
- 事项与分支：v1.1 更新候选；`merge/cadmanwwang/shop-cardpack-quantity`；`6c41ee16`（运行时）与 `e00ae185`（数据/资产）
- 目标：调整商城投放和价格，完成卡牌/角色体验修复，并更新最终Boss生命值。
- 生效映射：商城武器符文候选在 `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp` 配置中槽和右槽为 90%当前武器专属符文、5%其他武器、5%其他武器专属符文；最终Boss生命值的真源在 `Design/Data/ReEchoEnemyData.xlsx` 的 `Enemies` 与 `BossPhases` Sheet，运行时同步到 `Content/Data/enemies.csv`、`Content/Data/boss_phases.csv`。
- 修改：包含商店卡牌组购买次数与价格区间、武器与符文投放、卡牌配置及运行时、【样样都通】、【龙魂，集齐！】、【智者】双选确认、【勇者】损血持续攻防状态和最终Boss两阶段生命值（3000/2000）。
- 刷新方式：关闭并重新打开 `ReEcho.uproject`；源码改动通过 `ReEchoEditor Win64 Development` 编译后生效。
- 验证入口：策划在 UE Editor/PIE 完成 v1.1 效果验证，并于 2026-08-29 明确确认“已验证都无误”。已执行增量编译及相关自动化测试命令。
- 策划确认：cadmanwwang 已明确同意推送远端协作分支。
- 限制与踩坑：修改 CSV 时必须保持行字段数与表头一致；禁用卡需同时设 `Enabled=false`、`Offerable=false` 并填写 `DisabledReason`，否则启动数据校验会失败。
- 报告交接：`issues/cadmanwwang/requests/v1.1-商城角色与Boss更新.md`
