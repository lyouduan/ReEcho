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

## 2026-08-30 - 彩蛋卡牌与武器符文展示文案更新

- 结果：成功（仅本地，未推送远端）
- 事项与分支：`merge/cadmanwwang/card-rune-text-update`（本地分支，未推送）
- 目标：按策划提供的《文案修改.xlsx》更新 9 张彩蛋卡与 73 个武器符文的 `Description` 展示文案。
- 生效映射：卡牌文案真源是 `Design/Data/ReEchoData.xlsx` 的 `构筑体系G`（tblCards）——表头在第 3 行、数据 4~83 行、**Description 在第 5 列**、Id 在第 1 列；武器符文文案真源是同工作簿 `武器插槽C`（tblParts）——表头在第 45 行、数据 46~191 行、**Description 在第 6 列**。改完跑 `python scripts/data/sync_xlsx_to_csv.py` 生成 `Content/Data/cards.csv` / `parts.csv`，运行时只读 CSV，不读 XLSX。
- 文案换行与富文本：CSV 字段内直接写换行即可（XLSX 单元格内用 Alt+Enter），`UTextBlock` 会真实换行，无需任何转义符；项目无 `RichTextBlock`/Decorator，**不支持斜体/加粗/变色**，写 `<i>` 会原样显示成字面文本。
- 刷新方式：跑同步脚本后关闭并重新打开 `ReEcho.uproject`。
- 验证入口：`python scripts/data/sync_xlsx_to_csv.py --check`（PASS）、`python scripts/validate_project.py`（PASS）、`git diff --check`（通过）。
- 策划确认：cadmanwwang 已确认数值类文案变化「只改文案、实现不动」；未匹配项（已禁用的【旋刃】攻速减范围旋刃III）跳过不导入。
- 限制与踩坑（重要）：
  - 用 openpyxl 保存 XLSX 会把单元格内原有的 `\r\n` 变成 `\n\n`，使未改动的多行描述多出一个空行；本次受影响 8 条（卡牌 `G_2_18`/`G_2_19`/`G_3_05`/`G_3_17`，武器 `LongSword`/`Scythe`/`Bow`/`Gun`），已逐条修复为单个 `\n`，文字内容未变、显示效果一致。
  - 同一原因导致 openpyxl 重写后的 XLSX 用 Excel 打不开（报“已损坏”）：这是仓库工具链已知问题，`sync` 脚本与游戏运行均不受影响，但策划无法用 Excel 编辑真源，需程序修复同步脚本的回写行为。
  - `Description` 只是展示文案，改它不改变任何实际数值/效果（效果由 `card_effects.csv`/`part_effects.csv`/`BehaviorId` 驱动）。本次有 6 条文案含数值变化（减攻速广域剑刃、命中流血枪口、`G_4_1`/`G_4_2`/`G_4_3`/`G_4_9`），已按策划确认只改文案，文案与实机存在已知不一致，后续需另开需求改实现。
- 报告交接：本轮为直接修改，未建 Issue/Request 报告。

## 2026-08-30 - 结算界面新增 5 项局内统计 + 三列布局重排

- 结果：成功（仅本地，未推送远端）
- 目标：失败/通关结算界面除原有 3 项（关卡进度/时间碎片/构筑卡牌）外新增 5 项局内统计——本局回响累计伤害、本局本体累计伤害、元素反应次数、最高单次伤害、击杀怪物总数；8 项整体重排为左中右三列（3/3/2）并统一字号，顺带为通关结算补齐卡牌图标位。失败结算与通关结算必须视觉一致。
- 实现机制（C++，独立不侵入现有体系）：
  - 新增 `Source/ReEcho/Public/Run/ReEchoRunStatsTracker.h` + `Private/Run/ReEchoRunStatsTracker.cpp`：`FReEchoRunCombatStats`（5 字段）+ `UReEchoRunStatsSubsystem`（UGameInstanceSubsystem）。伤害按 `EReEchoDamageSource` 归入本体/回响；元素反应按发起者归属；最高单次伤害取全来源最大值；击杀排除 `Enemy` 来源。
  - `ReEchoGameMode.cpp`：敌人生成时转发 `OnHurt`/`OnDeath`/`OnElementReactionResolved` 给统计器；`BeginSelectedRun`（由 `bBeginSelectedRunStarted` 保护，一局只跑一次）开局清零，避免每关误清零。
  - `ReEchoRestartWidget`：`SetVictoryScreen` 新增带 `OwnedCards` 重载、`SetDeathScreen` 扩展为 6 参；`BindWidgetOptional` 绑定 `Victory/Defeat{EchoDamage,PlayerDamage,ReactionCount,MaxHit,KillCount}Value` 及 `DesignerVictoryCardIcon0~4`；展示时取整。
- UI 资产（WBP_ReEchoRestart.uasset，纯数据驱动、无逻辑）：用 **UE 5.8 Python `unreal.UMGToolSet` API** 在 Editor 内添加控件并布局（踩坑见下）。最终布局（1920×1080 设计分辨率）：左列 X≈582（关卡进度/时间碎片/构筑卡牌）、中列 X≈990（回响伤害/本体伤害/元素反应）、右列 X≈1398（最高单次伤害/击杀总数）；标签+数值并排，标签宽 210、数值宽 150、行高 58、字号标签 24/数值 32；数据区 Y 起点 414、底 588，与卡牌图标 Y=609 不重叠。通关 5 张图标直接复制失败结算图标几何（X 465 起、间距 162、136×136）。
- 刷新方式：C++ 改动走 `scripts/ue/Build-Editor.cmd -FullRebuild`（31 核按规则必须全量重建）→ 自动刷新 prebuilt bundle；UI 资产改动在 Editor commandlet 内已 Compile+Save。
- 验证入口：`python scripts/validate_project.py`（PASS）、Build-Editor 全量重建（Succeeded、prebuilt refreshed）、UE Python 只读复核确认两面板各 8 项控件全部存在且坐标/字号正确、控件总数 75→100。
- 限制与踩坑（**重要，下次改 UMG 直接照抄，省 2 小时**）：
  - ❌ 不要 `blueprint.get_editor_property('widget_tree')` —— UE5.8 Python 抛 "Failed to find property 'widget_tree'"。
  - ✅ 正确入口：`toolset = unreal.UMGToolSet.get_default_object()`；列出控件 `toolset.call_method("GetWidgets", args=(bp,)).widgets`；加控件 `toolset.call_method("AddWidget", args=(bp, widget_class, name, parent, -1))`；**加完必须 `toolset.call_method("ToggleWidgetAsVariable", args=(bp, widget, True))`**，否则 C++ `BindWidgetOptional` 按名绑定不到（静默不显示，PIE 里是空白）。
  - 布局写值：`slot.set_editor_property("layout_data", unreal.AnchorData(offsets=unreal.Margin(x,y,w,h), anchors=unreal.Anchors(minimum=Vector2D(0,0), maximum=Vector2D(0,0)), alignment=Vector2D(0,0)))`；再 `slot.set_editor_property("auto_size", False)`。
  - 执行脚本用项目自带 `scripts/ue/Run-EditorPythonLocked.ps1 -ScriptPath <py>`（自带 reecho-locks 互斥锁、自动定位 UE_5.8、用 `-ExecutePythonScript=` + `-NullRHI`）。**不要**自己拼 `UnrealEditor-Cmd` 命令行，也**不要**用 `-run=pythonscript`（参数名错误，正确是 `-ExecutePythonScript=`）。
  - 脚本先 `DRY_RUN=True` 试算（打印坐标与"缺失/复用"判断、写 `_local_backup/layout_plan.txt），确认无越界与误判标签后再设 `False` 应用；应用后务必再跑一次只读复核确认控件真的建出来（防 `BindWidgetOptional` 静默失败）。
- 报告交接：本轮为直接修改，未建 Issue/Request 报告。
