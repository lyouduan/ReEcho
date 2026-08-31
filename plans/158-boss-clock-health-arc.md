# Plan 158 - 程序 - Boss 钟面弧形血条与血量指针

## 协调

- Planner 负责人：JosephLE910 + Codex。
- Executor 负责人：JosephLE910 + Codex（同一 AI 规划与执行）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`。
- 人工验收：`Passed`（用户请求发布本次已调好的候选；不代表 AI 手工 PIE 验收）。
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
  - 用户已保存并要求一并提交的 `Content/ReEcho/UI/WBP_ReEchoTraitCardChoice.uasset`、`Content/ReEcho/UI/WBP_ReEchoTraitCardEntry.uasset`（原样收录，不由脚本重建）。
  - 必要的 `Content/ReEcho/Materials/UI/M_UI_BossHealthArc.uasset` 与 `MI_UI_BossHealthArc.uasset`
  - `scripts/ue/author_plan158_boss_health_arc.py`、`scripts/ue/audit_plan158_boss_health_arc.py`、`scripts/ue/upgrade_plan158_boss_health_readability.py`
  - `scripts/ue/upgrade_plan158_boss_health_surface.py`
  - 用户后续三选一小型修复：`Source/ReEcho/{Public,Private}/UI/ReEchoTraitCardChoiceWidget.*`、`Source/ReEcho/Private/Tests/ReEchoTraitChoiceAuthoringTests.cpp`、`Source/ReEcho/Private/Tests/ReEchoShopLogicBlockTests.cpp`（旧父级硬编码断言改为保护实际作者层级）、`docs/tasks/trait-choice-authored-presentation.md`。
  - `Design/UI/ReEcho_Boss弧形血条调整指南.md`、`Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - 标准构建刷新、由 `Binaries/Win64/ReEchoEditor.prebuilt.json` 声明的精选 Editor 预构建文件。
- Stable Reads：`ReEchoGameMode.cpp`、`ReEchoBossTransformation.cpp`、`Graybox/ReEchoEnemyActor.cpp`、`Content/Data/boss_phases.csv`、`DA_SheepBossPhase3`、已有钟面/指针纹理、`ReEcho.Build.cs`、UI Framework。
- 影响模式：`SharedContract`，仅扩展 HUD 表现绑定与可编辑预览属性，不改变玩法公共契约。
- 兼容承诺 / 下游操作：普通关倒计时、钟面位置、关卡文字、小地图及用户已调字体/几何均保留；不整页重建 WBP。策划在原 HUD 蓝图中调整弧形样式，运行时只注入真实血量比例与指针角度。
- 明确排除：Boss 数值、阶段规则、伤害/死亡/胜利语义、存档、其他 UI 和美术源图重绘；截图红线是范围标记。按用户后续确认，剩余血量调整为醒目紫红色，空血段近黑灰，刻度弱化。

## 锁定目标

1. Boss 战取消旧横向血条，以现有顶部钟面的下半圆刻度环承载血量，保留钟背板、关卡文字与指针，不显示倒计时文字。
2. 满血时指针朝左，半血朝下，空血朝右；扣血时由左经下半圆逆时针到右。弧形剩余段终点与指针指向一致，满/半/空分别显示整圈下半环/半段/无填充。
3. 使用当前阶段真实 `CurrentHealth / MaximumHealth`。阶段恢复血量后按新比例显示，不叠加各阶段总血量，不改 GameMode/Combat 权威。
4. 蓝图可见并可调整血条颜色、尺寸、位置；设计期提供代表性血量预览，不能在编译/运行时把作者几何复写为 C++ 默认值。
5. 普通关保持原倒计时与指针表现。
6. 后续可读性修订：中央原倒计时区域用独立、Designer 可编辑的 `BossHealthPercentText` 显示当前阶段整数血量百分比；原倒计时控件及作者字体/几何不改。非零非满血显示范围限定在 1%–99%，避免尚存血量显示 0% 或受伤后显示 100%。
7. 剩余血量紫红色与近黑灰空槽形成显著对比；刻度强度 `Tick Contrast` 可在弧形控件蓝图调整，默认弱化为连续血量填充的辅助纹理。
8. 用户在可读性版本上已微调配色、透明度与字体；本轮以新保存资产为准保留这些值。剩余血量加入细分叉血管纹理与明暗模拟的轻微凸起，蓝图可调纹理强度、粗细、间距和凸起强度；静态纹理不随扣血滑动，不改变 Alpha、形状、百分比或空血段。两个强度归零须恢复原平面表现。
9. 用户澄清凸起对象为整条弧形血条，而非血管：`Relief Strength` 改为整个圆润截面的高光/背光模拟；血管仅作为平面颜色纹理，不参与法线或明暗起伏。关闭血管后仍有完整立体血条，改变血管粗细/间距不影响整体凸起。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI`（文档型入口）、`AREA-UI`、`AREA-Tests`。
- 对应模块文档：上述两份 `modules/MOD-*.md` 已列入 Writes；不新增 Runtime Module。
- 设计意图：复用钟面视觉语汇，用同一生命比例驱动弧形填充和指针，避免两套读数失配。
- 权威状态与依赖：GameMode 仍从 EnemyRoster 首个存活 Boss Combatant 获取生命；Widget 只读显示。Boss 变身暂停期间保留当前状态，恢复更新后显示新阶段血量。不改变模块依赖或存档。
- 决策记录：优先用可预览的专用 UMG 表现控件或 UI 材质表达弧形，而不是缩放横条或导入整屏截图；具体选型以原素材刻度环对齐、Designer 可调与最小可靠依赖为准。原 `ArtClockNeedle` 在 Boss 模式复用，不另造第二根指针。
- 相关文档同步范围：关闭前审阅 `CODEBASE_MAP/ARCHITECTURE.md`、`README.md`、`modules/MOD-ReEcho.md`、`modules/MOD-ReEchoUI.md`；仅同步实际变化。新增专用调参指南并修正 UI 指导中的旧横条描述。

## 锁定验收

- [x] Boss 满/半/空血时弧形与指针一致，空血不残留填充，异常比例被安全钳制。
- [x] 普通关倒计时与 Boss 显隐切换正确，阶段回血可恢复满环。
- [x] 正式 WBP 中不再展示横条，钟面、刻度与指针保留且层级正确。
- [x] Designer 中调整颜色/几何后 Compile/Save 与运行注入不覆盖作者值。
- [x] 用户已调字体、位置、小地图及其他 HUD 控件无无关差异。
- [x] HUD 构建、资产加载/编译、聚焦自动化及预构建一致性通过；完整静态的既有 XLSX 漂移未通过，按具名授权单列，不冒充通过。
- [x] 可读性修订：独立中央百分比、紫红剩余/近黑灰空槽及弱刻度已接入正式资产；整数边界、设计期预览、阶段恢复和作者样式保护通过新版回归。
- [x] 血管/凸起修订：保留用户最新作者值，四个 Surface 参数真实驱动材质，Alpha 逐像素不变，空槽不受影响，扣血不滑纹理，双强度为零恢复平面。
- [x] 整体凸起修正：关闭血管后整条血条仍有圆润截面、高光和背光；血管粗细/间距不影响整体光照，GPU 对照通过，原作者值保持。
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

- 正式发布授权：用户要求推送合入主分支，并限定“只碰我们改了的，远端的保持不变”。在准确远端 `8cde5261` / 本地 `6f9a73cf` 上说明风险并建议先问程序后，用户明确确认仅豁免已知 `cards.csv, card_effects.csv` 与 XLSX 同步检查，并确认 `JosephLE910 + Codex` 提交身份。生产 CSV/XLSX 保持远端原值；其余构建、测试、LFS、差异审计照常。该豁免不适用于新失败或范围变化。
- 发布候选 `publish/plan158-ui` 在同一物理工程使用与 `6f9a73cf` 完全相同的 tree 形成正式提交 `8e4fd3df`；原 `plan/158-boss-clock-health-arc` / `6f9a73cf` 保留作为本地恢复点，没有改写原分支。获锁前只读 merge-tree 已确认冲突仅为 manifest 与 7 个 DLL，必须在最新组合源码上完整重建；UI 文档可合并，远端商店/PlayerHud 资产与本地三份作者蓝图不同路径、均须原样保留。尚未提前合入 main 或开始最终构建。
- 本地提交轮次（2026-08-31）：用户确认需收录最新蓝图微调。本次只形成 Plan158 本地 WIP 检查点，不推远端、不合并 main、不关闭 Plan。最新 `origin/main=8cde5261` 仅在上一轮审计后新增商店/玩家 HUD 蓝图发布及匹配预构建；规则、三选一与 Boss 源码/资产未变。物理耦合为精选预构建，逻辑相邻但不改本任务的可选数量契约，正式集成留待获得发布授权与锁后处理。三选一可选数量读取 RunSubsystem 本次免费/付费卡包额度，不按角色名硬编码；双选标题与确认数量测试通过。
- 最新用户资产 SHA256：HUD `2B817213D0A99A38F2309F07721CA5E651AA35C9799A7F8917C8D10D139F97BC`；三选一 `6CCAADD1D8816CC75A0F6DDB89076F7B648AD5771EC5452BAB39C3537FDA2617`；单卡 Entry `71206801A33A700DE7EEFD65F5C146B68F6FB6EC3AAE488C9AABEF16ABF84F1C`。本轮检查前后不变。`Saved/plan158-local-commit-tests-engine.log` 三项聚焦自动化 3/3 Success；精选预构建源码指纹 `59d43ba74ca7` 匹配，无新源码改动。`Saved/plan158-local-commit-static.log` 仍只有已知 XLSX/CSV 漂移失败，不声明完整门禁通过；本地 WIP 不能视为正式发布候选。
- 用户已调好血条，并在同一物理工程微调三选一后要求修复 C++ 覆盖。本轮只做三选一表现小型修复，保留血条和用户两份已保存 WBP，不重排/重建资产。标题保留作者模板与几何；刷新文案保留作者换行/单位，动态替换数量/费用；刷新按钮样式只给原生 fallback 设置。游戏抽取、逐槽揭示、扣费、选择数量和提交委托不变。对应模块文档与本地小型任务记录一并更新。
- 最新远端 `f6254150` 新增 Plan159 商店余额/刷新文本、Plan160 文档。权威规则与三选一配对代码/资产均未改；物理重叠为预构建及模块/UI 文档，逻辑相邻为商店但本轮不改其代码。只读审计后继续准确 Plan158 工程，不在本轮修复中合入远端或覆盖用户工作；正式集成时须适配最新主线。
- 整体凸起修正完成：删除血管高度场导数参与法线的路径，改为整条血条的圆润截面法线、固定左上漫反射与柔和高光；血管保留平面 RGB 花纹。`Relief Strength` 仍为原字段/数值，只改变其正确表现对象，未重设配色、Alpha、字体、位置或任何作者参数。备份 `Saved/Plan158RaisedStripBackup-20260831-203308`；对比上一轮保护快照一致。同步调整指南和两份模块说明，无新增模块依赖/数据契约。
- 整体凸起修正进行中：最新 fetch 仍为 `ac28c32d`，远端规则/源码无新差异，Editor 当前已关闭。按用户澄清把高度/法线与血管纹理解耦，保留作者参数数值。上一轮验证只证明血管版本，不能作为本轮立体截面的通过证据。
- 表面修订完成：新增 `Vein Strength/Width/Spacing` 和 `Relief Strength`，静态分叉高度场与固定左上光照仅调剩余段 RGB。未新增/重绘任何纹理，未改色值、Alpha、百分比字体、几何或预览。用户本轮资产备份为 `Saved/Plan158SurfaceBackup-20260831-201106`，快照为 `Saved/plan158_surface_before.json`，UE 重新加载审计确认全部作者值一致。
- 原像素覆盖测试误把用户新增的同色百分比数字算成血量：测试对照图继续使用真实作者值；统计阶段仅在 transient 实例上使用固定血条/白色文字配色，不写回资产。旧配色与 0–1 Alpha 审计不再用于限制合法的 UE LinearColor 作者值；改为有限数检查与逐字段完整保护快照，仍保留形状/层级/材质和透明度回归。材质引用快照按对象路径比较，排除进程地址而不忽略属性。
- 表面修订进行中：用户要求血管样纹理或凸起感，采用原 UI 材质中的程序化分叉高度场与固定光照，不生成/修改共享美术纹理。先保存本轮完整作者快照及资产备份，不重跑旧配色升级；上一版构建/渲染结果不作为本轮通过证据。远端 `ac28c32d` 相对本地仅新增 Plan159 文档，适用规则不变，无 HUD 冲突。
- 用户确认编辑器已关闭后，完成可读性修订的蓝图/材质升级与编译。新增 `BossHealthPercentText`，原 `CountdownText` 不动；使用用户刚保存的当前样式/几何作为复制源，原节点在本轮前后快照一致。旧资产备份位于 `Saved/Plan158ReadabilityBackup-20260831-194602`，未覆盖上一版备份。
- 可读性反馈修订已获用户确认：中央显示百分比，其他按紫红填充/近黑灰空槽/弱刻度方案实施。最新 fetch 为 `ac28c32d`，新增内容仅 Plan159 商店余额任务文档，与本轮 HUD 无物理/语义冲突；未把另一任务内容或任何数据变化合入本轮工作树。编辑器打开时只改文本源码，资产和构建等待用户保存关闭。
- 本轮文本部分已实现：百分比格式/独立绑定、刻度参数与 shader、最小资产升级脚本、回归断言和调参文档。`git diff --check`、Python 语法与 workflow 校验通过。当前 PID 27928 仍打开本任务工程，已请用户保存关闭；尚未执行本轮材质/蓝图升级、UHT/UBT 或 GPU 回归。以下“最终”构建/渲染记录属于上一版候选，不能作为当前修改后的通过证据；现有预构建仍是上一版。
- 2026-08-31 Plan-only 已按用户具名授权发布：`ac4bbb3e`。本地实现仍在本任务 worktree，未发布实现，等待人工视觉验收。
- 已完成 `UReEchoBossHealthArcWidget` + UI 材质，使用当前阶段生命比例驱动下半环及原指针；正式 WBP 删除旧横条三个节点，新增与钟背板同几何/Z=5 的 `BossHealthArc`。原指针 Z=6、轴心、字体、关卡文字、小地图和其他作者值均保留。
- 新增根 Widget 的 Boss 设计期预览与比例。颜色、内外半径、参考尺寸、弧心直接暴露在弧形控件；每个实例拥有独立 transient MID 和 Slate Brush，不把材质实例/运行时比例存回作者资产。
- 修正原聚焦测试中已过时的时间碎片断言：现有 `SetTimeShards(-5)` 及已发布 UI 文档均明确显示有符号净余额，测试由误期望 `0` 改为 `-5`；未修改时间碎片行为。
- 实际像素回归最初未显示弧线：定位为 UE 5.8 按需材质着色器尚未提交编译，同一自动化帧内绘制被跳过。测试显式提交已加载 Windows shader platform 的编译任务并等待完成；不向游戏代码添加同步编译。材质输出 RGB/Alpha 分离、常量色探针和临时 Z=99/绘制诊断均已回收，正式资产恢复弧形 shader，层级不变。
- 已写策划调整指南并同步模块/UI 文档。运行时 CSV、XLSX、Boss 玩法代码及数值完全未改；不从旧 XLSX 回填当前实际生效数据。

- 初始编号 Plan，仅发布规格，尚未实施。
- 2026-08-31 用户明确同意修复校验登记。已在本地同步 C++ 已支持的 `Card.EasterShardThreshold` 与效果目标 `CritNegateAmplification`，新增接受生产 CSV / 拒绝未知行为 / 拒绝未知目标的聚焦回归；未修改卡牌逻辑、CSV 或 XLSX，弧形 HUD 仍未实施。
- 用户后续明确：以远端实际生效的数据为准，优先完成血条。保留 `fad6d081` 的运行时 CSV 与代码，不向生产目录执行旧 Excel 导出；XLSX 同步作为已知独立差异保留。已询问是否对本次 Plan-only 发布具名放行该基线已有登记/同步失败，未取得准确确认前不发布。
- 用户已在风险提醒后明确回复“允许”：本轮 Plan158 对准确基线 `fad6d081` 已有的卡牌行为/目标漏登记与 XLSX/CSV 同步失败，记录为“经人工确认暂时放行 / 未通过”；不得以此声称完整静态检查通过，也不豁免 HUD 构建、聚焦验证或其他新错误。Plan-only 候选仅包含本文件，两处登记修复与测试仍留本地，后续随实现交付。

### 证据

- **当前整条血条凸起候选（替代下方血管凸起版本证据）**：`Saved/plan158-raised-strip-build.log` UHT/UBT 成功；精选预构建 7 模块 / BuildId `55116800` / 源码 `00eb7be5baac`。`Saved/plan158-raised-strip-author-engine.log` 升级 PASS，`Saved/plan158-raised-strip-audit-engine.log` 重新加载后完整作者快照与结构审计 PASS。
- `Saved/plan158-raised-strip-tests-engine.log`：`BossArcRendering`、`Formatting` 均 Success、进程退出 0。关闭血管但保留凸起时有 `8742` 个 RGB 像素产生立体明暗；在此条件下改变血管粗细/间距的 RGB 差异均为 0，证明法线解耦。所有 Alpha 差异为 0，空槽不变、归零恢复平面；受控血条 100/75/50/25/0% 填充数 `11806/8824/5853/2881/0`，普通关 0。
- 已查看本轮 `Authored_Boss_065_ReliefOnly.png` 与 `Authored_Boss_065_Surface.png`：前者无血管仍有连续圆润截面光照，后者只叠平面花纹，两者均使用真实作者配色/字体/几何。离屏证据不替代用户实战视觉验收。
- 本轮 Python 语法、workflow、`git diff --check`、预构建一致性和 LFS hydrated/fsck 通过；`Saved/plan158-raised-strip-static.log` 完整静态仍仅失败于已有 XLSX/CSV 漂移，生产数据 diff 为零。未提交、未推送实现。
- **当前血管/凸起候选（替代下方可读性版本证据）**：`Saved/plan158-surface-final-build.log` UHT/UBT 成功；`prebuilt_editor.py check` 验证 7 模块 / BuildId `55116800` / 源码 `55013763e5d5`。资产升级 `Saved/plan158-surface-author-engine.log` 为 `[Plan158Surface] PASS`；最终回读 `Saved/plan158-surface-final-audit-engine.log` 为 Surface round protected colors/alpha/font/geometry/preview PASS 与结构审计 PASS。
- 当前自动化 `Saved/plan158-surface-final-tests-engine.log`：`BossArcRendering`、`Formatting` 均 Success，退出码 0。受控配色的 100/75/50/25/0% 填充像素为 `11846/8854/5866/2888/0`，普通关为 0；四项表面参数均改变真实 GPU RGB，全部比较 Alpha 改变像素为 0；双强度关闭与原平面逐像素一致，空槽不变、扣血保留部分纹理不移动。
- 作者配色对照图 `Saved/Automation/Plan158/Authored_Boss_065_Surface.png` 与 `Authored_Boss_065_Flat.png` 已查看：使用用户最新保存的紫色/透明度和百分比字体，后者只在 transient 实例中关闭两个强度。不是 PIE/主观验收，仍请用户实战确认纹理强度。
- 登记测试 3/3、Encounter 契约 6/6、workflow、Python 语法、`git diff --check`、LFS hydrated/fsck、预构建一致性通过。`Saved/plan158-surface-final-static.log` 的完整校验仍仅失败于已知 `card_effects.csv, cards.csv` 与 XLSX 漂移；生产数据 diff 为零。不声称完整静态通过，未提交或推送实现。
- **当前可读性候选（替代下方上一版证据）**：`Saved/plan158-readability-build.log` UHT/UBT 成功，`prebuilt_editor.py check` 验证 7 模块 / BuildId `55116800` / 源码指纹 `ab6567fac03a`。本轮资产升级日志 `Saved/plan158-readability-author-engine.log` 为 `[Plan158Readability] PASS`，只读回读 `Saved/plan158-readability-audit-engine.log` 为 Protected snapshot PASS 和结构/材质/层级/预览 PASS。
- 当前 HUD 自动化：`Saved/plan158-readability-tests-engine.log` 中 `Formatting`、`BossArcRendering` 均 Success，退出码 0；覆盖 0%/1%/50%/99%/100%、普通关恢复、阶段回血、独立字体/颜色/位置保留、Tick Contrast 保留和 Designer 25% 样例。
- 当前 GPU 实际紫红填充像素在 100/75/50/25/0% 下依次为 `8131/6043/3981/1927/0`，普通关为 0。已查看更新后的 `Saved/Automation/Plan158/Boss_100.png`、`Boss_050.png`、`Boss_000.png`、`Normal.png`：读数可见、针与边界同步、空槽与剩余段有明显区分。截图是离屏渲染证据，实战视觉仍待用户验收。
- 本轮登记测试 3/3、Encounter 契约测试 6/6、workflow 校验与 `git diff --check` 通过；完整项目校验仍失败于基线已有的 `card_effects.csv, cards.csv` 与 XLSX 漂移，不声称通过；`Content/Data`、`Design/Data/ReEchoData.xlsx` 保持未修改。仅本地交付新版，未推送实现。

- 最终本地构建：`Saved/plan158-final-build.log`，Development UHT/UBT `Result: Succeeded`；`prebuilt_editor.py check` 验证 7 模块、BuildId `55116800`、源码指纹前缀 `41915956074a`。本轮为本地测试构建，后续实现 main 发布仍需持发布锁后的最终 FullRebuild。
- 最终 HUD 自动化：`Saved/plan158-final-engine.log`，`ReEcho.UI.CombatHud.Formatting` 和 `ReEcho.UI.CombatHud.BossArcRendering` 均 Success。GPU 紫色填充像素（100/75/50/25/0%）为 `4778/3525/2286/1049/0`，普通关为 `0`；针角、阶段回血、显隐及预览/作者参数保护同时通过。
- 实际 GPU 离屏截图：`Saved/Automation/Plan158/Boss_100.png`、`Boss_075.png`、`Boss_050.png`、`Boss_025.png`、`Boss_000.png`、`Normal.png`；已查看满/半/空，刻度与指针保留、边界对应。仅作为离屏渲染证据，不是 PIE/主观验收。
- 最终 UE 只读回读：`Saved/plan158-final-audit-engine.log`，`[Plan158Audit] Protected snapshot PASS` 与结构/材质/层级/预览 PASS；确认正式材质为实际弧形 shader，旧横条节点已移除，其他作者节点快照一致。
- 交付复核：登记测试 3/3、Encounter 契约测试 6/6、独立 workflow 校验、`git diff --check`、LFS hydrated 检查、`git lfs fsck` 通过；`git -c core.quotepath=false lfs status` 已审查。完整 `validate_project.py` 仍仅在已知 `card_effects.csv, cards.csv` XLSX 漂移处失败；`git diff --quiet -- Content/Data Design/Data/ReEchoData.xlsx` 返回 0，未改变实际运行数据或表格。
- 最新远端复核仍为 `ac4bbb3e`，适用规则无新增差异。实现候选只在本地任务工作树，未提交/发布实现，未清理测试目录。

- 已核对现有 `SetEncounterStatus`、`CalculateCountdownNeedleAngle`、`CalculateBossHealthRatio` 和 `RefreshText`；原 Boss 模式隐藏指针并横向缩放 Fill。
- 钟面源图为 `Content/SourceArt/UI/CombatHud/Plan93/Elements/时间底板.png`，可复用下半圆刻度环。
- Plan-only 发布前运行 `python scripts/validate_project.py` 失败：`Content/Data/card_effects.csv:78:BehaviorId: unknown registered C++ behavior id 'Card.EasterShardThreshold'`。除本 Plan 外工作树与 `origin/main` 无差异（`git diff --quiet` 退出 0），确定是当前远端基线已有问题。本 Plan 尚未提交或发布，未开始 UI 实现。
- 只读定位：当前 C++ `ReEchoCsvDataRegistry.cpp` 与 `ReEchoCardCatalog.cpp` 已注册该行为，但 Python 静态校验登记未同步。需要用户批准将该具名校验修复纳入范围，或先由原任务修复远端；不修改卡牌行为/数值，不跳过校验。
- 登记修复后，完整校验推进至 XLSX/CSV 同步检查并失败：`Generated CSV drift detected: card_effects.csv, cards.csv`。通过项目生成器导出至独立临时目录、逐字段比较确认不是单纯换行差异：例如 `G_4_2_CRIT` Excel 为 `0.5`、CSV 为 `1`，`G_4_3_CONTACT_DAMAGE` 为 `15` / `20`，`G_4_5_RADIUS` 为 `400` / `500`；还存在新旧效果行替换和九张彩蛋卡文案差异。未向生产目录运行同步，不以一方静默覆盖另一方。需要用户或数据任务负责人明确该批玩法数据的最终来源后才能修复这项独立基线问题、发布本 Plan。
- 聚焦证据：`python -m unittest discover -s scripts/data -p test_validate_card_registration.py -v` 3/3 通过；`test_validate_encounter_contract.py` 6/6 通过；`git diff --check` 通过。完整项目校验仍失败于上述同步差异，未声称通过，未提交/发布任何文件。
- 只读 UE 资产核对完成（`[Plan158Inspect] PASS`）：钟面 Canvas 位置 `(13.5,51)`、尺寸 `1159x216`、Z=5；指针位置 `(14,87)`、尺寸 `44x150`、Pivot `(0.5,0.12)`、Z=6，因此当前旋转轴心为画布中心偏右 14、Y=105。旧 Boss 横条为 `464x58`、Y=105、Z=8。已保存本地全部 HUD 节点快照，未修改任何 uasset。

### 剩余风险

- 三选一最新本地证据：`Saved/plan158-choice-authoring-build.log` 构建成功，预构建 7 模块 / BuildId `55116800` / 源码指纹 `59d43ba74ca7`；`Saved/plan158-choice-final-tests-engine.log` 三项聚焦自动化全部 Success（作者表现、正式资产、原生/商店抽卡流程）。旧测试的固定 `RootPanel` 父级断言改为保留实际作者父级/槽位，实测为 `ConfirmButton / ButtonSlot_0`。两份 WBP 前后哈希一致，本轮不写资产；完整证据见 `docs/tasks/trait-choice-authored-presentation.md`。workflow、格式、diff、LFS/prebuilt 通过；完整静态仍仅失败于已知表格漂移，未发布实现。
- 用户已反馈血条调好；当前待验收项为同一工程三选一的最新作者文案/几何修复。聚焦自动化不等同人工 PIE 视觉验收。
- 项目完整静态校验仍有当前基线已有的 `cards.csv` / `card_effects.csv` 与 XLSX 漂移，按本轮准确授权记录为未通过；不得作为已通过证据或默默覆盖数据。实现 main 发布时如基线/风险改变须重新按门禁确认。
- 引擎启动还报告已有 `M_ArenaBackground` 缺纹理警告；本轮未触碰该独立资产。

### 人工验收结果/请求

- `Passed`：用户已反馈“血的我调好了”，继续完成三选一/Entry 微调、要求收录并推送合入主分支；本次按该候选的人类验收处理。AI 只声明实际构建/自动化和资产哈希证据，不代替人类主观验收。正式发布仍须完成最新组合候选门禁。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅、无需修改。GameMode → HUD 的只读生命投影不变，无模块拓扑/依赖/存档变化。
- `shared/CODEBASE_MAP/README.md`：已审阅、无需修改。新表现控件仍归 `MOD-ReEcho` / `AREA-UI`，不新增模块或路由。
- `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`：已补充弧形控件、材质、设计期预览和聚焦渲染测试落点。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：已将正式 Boss 横条描述更新为下半钟环，保留原权威数据来源/普通关约束。
- `Design/UI/ReEcho_UI修改指导.md`：已更新 Boss 绑定入口；新增 `Design/UI/ReEcho_Boss弧形血条调整指南.md`，覆盖参数、几何、层级、运行时与初次迁移脚本边界。
