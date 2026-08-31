# Plan 158 - 程序 - Boss 钟面弧形血条与血量指针

## 协调

- Planner 负责人：JosephLE910 + Codex。
- Executor 负责人：JosephLE910 + Codex（同一 AI 规划与执行）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main` / `fad6d0813fdd84d05cad0bba06769bf2af5b10b5`。
- 本地实现方式：独立 worktree `ReEcho-plan158-boss-clock-health-arc`，分支 `plan/158-boss-clock-health-arc`。
- 依赖 / 阻塞：修改/构建 UE 资产前须确认编辑器已保存并关闭；沿用当前 Boss 阶段生命重置与 GameMode 的只读 HUD 注入。
- Writes:
  - `plans/158-boss-clock-health-arc.md`
  - `scripts/validate_project.py`、`scripts/data/test_validate_card_registration.py`（用户确认的前置校验登记修复）。
  - `Source/ReEcho/{Public,Private}/UI/ReEchoEncounterHudWidget.*`
  - 必要的新弧形表现控件 `Source/ReEcho/{Public,Private}/UI/ReEchoBossHealthArcWidget.*`
  - `Source/ReEcho/Private/Tests/ReEchoCombatHudTests.cpp`
  - `Content/ReEcho/UI/WBP_ReEchoEncounterHud.uasset`
  - 必要的 `Content/ReEcho/Materials/UI/M_UI_BossHealthArc.uasset` 与 `MI_UI_BossHealthArc.uasset`
  - `scripts/ue/author_plan158_boss_health_arc.py`、`scripts/ue/audit_plan158_boss_health_arc.py`
  - `Design/UI/ReEcho_Boss弧形血条调整指南.md`、`Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - 标准构建刷新、由 `Binaries/Win64/ReEchoEditor.prebuilt.json` 声明的精选 Editor 预构建文件。
- Stable Reads：`ReEchoGameMode.cpp`、`ReEchoBossTransformation.cpp`、`Graybox/ReEchoEnemyActor.cpp`、`Content/Data/boss_phases.csv`、`DA_SheepBossPhase3`、已有钟面/指针纹理、`ReEcho.Build.cs`、UI Framework。
- 影响模式：`SharedContract`，仅扩展 HUD 表现绑定与可编辑预览属性，不改变玩法公共契约。
- 兼容承诺 / 下游操作：普通关倒计时、钟面位置、关卡文字、小地图及用户已调字体/几何均保留；不整页重建 WBP。策划在原 HUD 蓝图中调整弧形样式，运行时只注入真实血量比例与指针角度。
- 明确排除：Boss 数值、阶段规则、伤害/死亡/胜利语义、存档、其他 UI 和美术源图重绘；截图红线是范围标记，本轮沿用暗紫色。

## 锁定目标

1. Boss 战取消旧横向血条，以现有顶部钟面的下半圆刻度环承载血量，保留钟背板、关卡文字与指针，不显示倒计时文字。
2. 满血时指针朝左，半血朝下，空血朝右；扣血时由左经下半圆逆时针到右。弧形剩余段终点与指针指向一致，满/半/空分别显示整圈下半环/半段/无填充。
3. 使用当前阶段真实 `CurrentHealth / MaximumHealth`。阶段恢复血量后按新比例显示，不叠加各阶段总血量，不改 GameMode/Combat 权威。
4. 蓝图可见并可调整血条颜色、尺寸、位置；设计期提供代表性血量预览，不能在编译/运行时把作者几何复写为 C++ 默认值。
5. 普通关保持原倒计时与指针表现。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI`（文档型入口）、`AREA-UI`、`AREA-Tests`。
- 对应模块文档：上述两份 `modules/MOD-*.md` 已列入 Writes；不新增 Runtime Module。
- 设计意图：复用钟面视觉语汇，用同一生命比例驱动弧形填充和指针，避免两套读数失配。
- 权威状态与依赖：GameMode 仍从 EnemyRoster 首个存活 Boss Combatant 获取生命；Widget 只读显示。Boss 变身暂停期间保留当前状态，恢复更新后显示新阶段血量。不改变模块依赖或存档。
- 决策记录：优先用可预览的专用 UMG 表现控件或 UI 材质表达弧形，而不是缩放横条或导入整屏截图；具体选型以原素材刻度环对齐、Designer 可调与最小可靠依赖为准。原 `ArtClockNeedle` 在 Boss 模式复用，不另造第二根指针。
- 相关文档同步范围：关闭前审阅 `CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`modules/MOD-ReEcho.md`、`modules/MOD-ReEchoUI.md`；仅同步实际变化。新增专用调参指南并修正 UI 指导中的旧横条描述。

## 锁定验收

- [ ] Boss 满/半/空血时弧形与指针一致，空血不残留填充，异常比例被安全钳制。
- [ ] 普通关倒计时与 Boss 显隐切换正确，阶段回血可恢复满环。
- [ ] 正式 WBP 中不再展示横条，钟面、刻度与指针保留且层级正确。
- [ ] Designer 中调整颜色/几何后 Compile/Save 与运行注入不覆盖作者值。
- [ ] 用户已调字体、位置、小地图及其他 HUD 控件无无关差异。
- [ ] 适用静态、构建、资产加载/编译及聚焦自动化通过；精选预构建包与源码一致。
- [ ] 用户验收 Boss 阶段切换、伤害更新及实际屏幕可读性。

## Step 0 门禁

- 基线：`fad6d0813fdd84d05cad0bba06769bf2af5b10b5`，远端最大 Plan 为 157。
- 外部提交审计：相对已完成 Plan157，远端包含彩蛋卡牌、Boss 三阶段/变身和其他 UI 迭代；本任务从准确最新 main 新建，不合入旧工作树 WIP。HUD 配对源码与 `WBP_ReEchoEncounterHud` 未被这些提交修改；玩法变更保留为 Stable Reads，不存在本地物理冲突或需要产品取舍的覆盖。
- 引擎：UE 5.8 安装版；构建前检查进程与同克隆 Unreal 锁，并执行 LFS 检出检查。
- 现有聚焦测试：`ReEchoCombatHudTests.cpp` 已覆盖倒计时角度、Boss 比例和正式资产绑定；本轮尚未运行。
- 难合并资源：单一 HUD 二进制 WBP，迁移仅改 Boss 命名节点；修改前记录其他节点属性供审计。
- 停止条件：缺失正式资产、LFS 未还原、编辑器未关闭、出现同一 WBP 未合并人工修改或无法复核的基线损坏。

## 实现提纲

1. 发布本编号 Plan 后，读取 UE 中现有钟面、指针与 Boss 子树真实属性，记录保护快照。
2. 实现弧形可视表现及预览属性，复用统一血量比例驱动指针和填充。
3. 通过 UE 迁移脚本最小修改 WBP，移除被替代的 Boss 横条展示，保留其他作者属性。
4. 编译、聚焦回归、回读资产，维护文档并交付测试工程。实现候选须人工视觉验收后再按规则发布。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 通过 |
| LFS | `python scripts/setup_lfs.py --check` | 真实文件已还原 |
| C++ | `.clang-format`、`scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT 成功，精选预构建已刷新 |
| 聚焦自动化 | `ReEcho.UI.CombatHud` 对应实际过滤器 | 比例、角度、模式、作者属性保护通过 |
| 资产 | UE Compile/Save/回读、`audit_plan158_boss_health_arc.py` | 正式绑定/层级/预览有效，无无关作者属性变化 |
| 人工 | 新工程普通关与 Boss 满/半/空血、阶段回血 | 用户确认视觉与手感 |
| 实现发布时 | 持锁后最新组合候选 `-FullRebuild` 及全部失效证据重验 | 按 GIT_RULES 完成；本次 Plan-only 发布豁免构建且不碰锁 |

## 执行记录

### 变化

- 初始编号 Plan，仅发布规格，尚未实施。
- 2026-08-31 用户明确同意修复校验登记。已在本地同步 C++ 已支持的 `Card.EasterShardThreshold` 与效果目标 `CritNegateAmplification`，新增接受生产 CSV / 拒绝未知行为 / 拒绝未知目标的聚焦回归；未修改卡牌逻辑、CSV 或 XLSX，弧形 HUD 仍未实施。
- 用户后续明确：以远端实际生效的数据为准，优先完成血条。保留 `fad6d081` 的运行时 CSV 与代码，不向生产目录执行旧 Excel 导出；XLSX 同步作为已知独立差异保留。已询问是否对本次 Plan-only 发布具名放行该基线已有登记/同步失败，未取得准确确认前不发布。
- 用户已在风险提醒后明确回复“允许”：本轮 Plan158 对准确基线 `fad6d081` 已有的卡牌行为/目标漏登记与 XLSX/CSV 同步失败，记录为“经人工确认暂时放行 / 未通过”；不得以此声称完整静态检查通过，也不豁免 HUD 构建、聚焦验证或其他新错误。Plan-only 候选仅包含本文件，两处登记修复与测试仍留本地，后续随实现交付。

### 证据

- 已核对现有 `SetEncounterStatus`、`CalculateCountdownNeedleAngle`、`CalculateBossHealthRatio` 和 `RefreshText`；原 Boss 模式隐藏指针并横向缩放 Fill。
- 钟面源图为 `Content/SourceArt/UI/CombatHud/Plan93/Elements/时间底板.png`，可复用下半圆刻度环。
- Plan-only 发布前运行 `python scripts/validate_project.py` 失败：`Content/Data/card_effects.csv:78:BehaviorId: unknown registered C++ behavior id 'Card.EasterShardThreshold'`。除本 Plan 外工作树与 `origin/main` 无差异（`git diff --quiet` 退出 0），确定是当前远端基线已有问题。本 Plan 尚未提交或发布，未开始 UI 实现。
- 只读定位：当前 C++ `ReEchoCsvDataRegistry.cpp` 与 `ReEchoCardCatalog.cpp` 已注册该行为，但 Python 静态校验登记未同步。需要用户批准将该具名校验修复纳入范围，或先由原任务修复远端；不修改卡牌行为/数值，不跳过校验。
- 登记修复后，完整校验推进至 XLSX/CSV 同步检查并失败：`Generated CSV drift detected: card_effects.csv, cards.csv`。通过项目生成器导出至独立临时目录、逐字段比较确认不是单纯换行差异：例如 `G_4_2_CRIT` Excel 为 `0.5`、CSV 为 `1`，`G_4_3_CONTACT_DAMAGE` 为 `15` / `20`，`G_4_5_RADIUS` 为 `400` / `500`；还存在新旧效果行替换和九张彩蛋卡文案差异。未向生产目录运行同步，不以一方静默覆盖另一方。需要用户或数据任务负责人明确该批玩法数据的最终来源后才能修复这项独立基线问题、发布本 Plan。
- 聚焦证据：`python -m unittest discover -s scripts/data -p test_validate_card_registration.py -v` 3/3 通过；`test_validate_encounter_contract.py` 6/6 通过；`git diff --check` 通过。完整项目校验仍失败于上述同步差异，未声称通过，未提交/发布任何文件。
- 只读 UE 资产核对完成（`[Plan158Inspect] PASS`）：钟面 Canvas 位置 `(13.5,51)`、尺寸 `1159x216`、Z=5；指针位置 `(14,87)`、尺寸 `44x150`、Pivot `(0.5,0.12)`、Z=6，因此当前旋转轴心为画布中心偏右 14、Y=105。旧 Boss 横条为 `464x58`、Y=105、Z=8。已保存本地全部 HUD 节点快照，未修改任何 uasset。

### 剩余风险

- 需验证弧形半径与原素材刻度环、现有指针轴心的实际对齐；画布存在作者微调，不凭历史脚本位置覆盖。

### 人工验收结果/请求

- `PendingBeforeClose`：实现后请用户检查暗紫色填充、指针指向、Boss 各阶段与普通关回归。

### 架构文档审阅结果

- 实现后填写具名审阅结论，当前未关闭。
