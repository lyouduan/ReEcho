# Plan 125 - 程序 - 删除旧角色晋升与跨关换角逻辑

## 协调

- Planner 负责人：Gavyn / Codex。
- Executor 负责人：Gavyn / Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@ac9bb58dafdc8129e4bcaf5e65fb1d87caa9e422`。
- 本地实现方式（可选，仅作交接说明）：独立 worktree `ReEcho-plan125-remove-promotion`，分支 `plan/125-remove-legacy-character-promotion`。
- 依赖 / 阻塞：Plan122 构筑路径日志已进入主线；以策划源表 `策划数据源/【开普勒】回响数值与构筑体系.xlsx` 为产品权威。实现发布前仍需按最新 `origin/main` 审计传入变化。
- Writes:
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`、`Source/ReEcho/Private/Run/ReEchoCharacterPromotion.cpp`、`Source/ReEcho/Public/Run/ReEchoCharacterPromotion.h`；
  - `Source/ReEcho/Public/Data/ReEchoCsvDataRegistry.h`、`Source/ReEcho/Private/Data/ReEchoCharacterBuildCsvReader.cpp`、`Source/ReEcho/Private/Data/ReEchoCsvDataRegistry.cpp`；
  - `Source/ReEcho/Private/UI/ReEchoLoadoutSelectionWidget.cpp`；
  - `Source/ReEchoCards/Public/Cards/ReEchoCardTypes.h`；
  - `Source/ReEcho/Private/Tests/ReEchoCharacterPromotionTests.cpp`、受影响 Save/Run/Data/Card/UI 聚焦测试；
  - `Design/Data/ReEchoData.xlsx`、`Content/Data/cards.csv`、`Content/Data/characters.csv`、`Content/Data/csv_schema.csv`、受影响 CSV fixture；
  - `scripts/data/sync_xlsx_to_csv.py`、`scripts/validate_project.py` 及受影响数据同步测试；
  - `plans/125-remove-legacy-character-promotion.md`；
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`、`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`，必要时 `shared/LESSONS.md`。
- Stable Reads:
  - `策划数据源/【开普勒】回响数值与构筑体系.xlsx` 的 `角色体系J!A1:E5`、`构筑体系G!A1:H70` 与 `投放系统!A1:D48`；
  - `Source/ReEcho/Private/Run/CharacterAbilities/ReEchoCharacterAbilityRuntime.*`；
  - `Source/ReEcho/Public/Run/ReEchoRunSaveGame.h`、当前 BuildSnapshot/SaveVersion 迁移路径；
  - `plans/15-character-promotion.md`、`plans/22-data-character-build-csv.md`、`plans/87-forge-cleanup-and-character-abilities.md` 仅作历史来源，不作为当前产品权威。
- 影响模式：`SharedContract`。
- 兼容承诺 / 下游操作：四个稳定角色 ID、角色选择 UI 顺序、各角色固定能力、普通卡牌效果/投放、武器与存档继续可用；新局和当前版本存档中，玩家选择的角色身份在整局内保持不变。旧存档的晋升标志只作一次迁移输入，不再触发第二次换角。
- 明确排除：不修改四名角色的策划数值或能力效果；不改卡牌内容、分类标签、抽取概率、武器、敌人、Echo、音频或角色美术；不把普通卡牌类别重新解释为职业计分；不修改策划源工作簿中已经确认的可见内容。

## 锁定目标

1. 删除“拥有至少 4 张普通卡牌后，按卡牌职业桶自动晋升并替换 `CharacterId`”的旧运行时机制；免费选择、付费卡组、调试授卡、读档和跨 Encounter/Stage 均不得改变玩家已选角色。
2. 角色的 `RoleId` 只表示已选角色的固定能力类别。智者、猎手、诗人、勇者能力继续由角色表和 `ReEchoCharacterAbilityRuntime` 生效，普通构筑卡不再参与角色归属判断。
3. 清除只服务于晋升计分的生产数据和公共字段：卡牌 `PromotionRoleId` 删除；角色 `PromotionPriority` 不再作为晋升优先级。若装载选择仍需稳定顺序，将该字段收敛为语义准确的 `SelectionOrder` 并保持当前可见顺序。
4. 新建 Run 不再写入 `BaseCharacterId`、`Promoted`、`Role` 等旧晋升 RuleFlags。旧存档加载时确定性移除这些标志：存在有效 `BaseCharacterId` 时恢复该原始角色并逆向扣除旧晋升角色差额；缺失可靠原始 ID 时保留当前 `CharacterId`，按该角色修正 `RoleId`，不得猜测或再次换角。
5. 保存迁移必须同时处理当前构筑与装备基线快照，保持卡牌/符文增益、已损失生命和角色固定能力不重复、不丢失；迁移后再次保存/加载结果稳定。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoCards`、`MOD-ReEchoUI`；没有新增 Runtime Module，也不改变现有模块依赖拓扑。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`（Run 身份与存档迁移）、`MOD-ReEchoCards.md`（卡牌不承担角色晋升计分）、`MOD-ReEchoUI.md`（装载顺序字段从晋升语义解耦），均已加入 `Writes`。
- 设计意图：角色选择已经是整局身份和固定能力的唯一权威；卡牌只修改构筑状态，不能暗中替换角色、基础外观和角色能力。删除旧晋升领域后，角色身份变化只有明确开始新 Run 或旧存档迁移两个入口。
- 权威状态与依赖：`UReEchoRunSubsystem::CurrentBuild.CharacterId` 继续拥有整局角色身份；Cards 只返回卡牌授予结果，不携带职业计分；UI 只按只读 `SelectionOrder` 展示角色，不拥有晋升规则。`RoleId` 仍由角色数据初始化，供角色能力和现有战斗查询使用。
- 决策记录：不采用“仅把触发阈值改大/关闭调用”的软禁用，因为会留下可重新触发的第二套产品规则与无来源 Schema；采用删除运行时类型、移除数据列并迁移旧存档。角色展示顺序与晋升优先级语义拆开，避免为了 UI 排序保留废弃机制。
- 相关文档同步范围：必审 `ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoCards.md`、`MOD-ReEchoUI.md`；预计拓扑和 AREA 路由不变，前三份模块文档按实际实现更新，前两份总览若事实未变则记录无需修改。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅模块拓扑与角色身份跨模块不变量；
  - `README.md`：待审阅 Run/Cards/UI 阅读路由；
  - `MOD-ReEcho.md`：待记录角色身份唯一权威与旧存档迁移；
  - `MOD-ReEchoCards.md`：待删除晋升计分职责并明确普通卡牌不改变角色身份；
  - `MOD-ReEchoUI.md`：待记录装载选择只消费稳定展示顺序。

## 锁定验收

- [ ] 新 Run 选择任一角色后，连续获得至少 8 张不同职业分类卡牌并跨越 Stage，`CharacterId`、外观和固定角色能力始终不变。
- [ ] 正常授卡、付费卡组领取和 `DebugGrantCard` 都不调用或间接重建晋升规则；代码库不再存在 `TryPromote`/`EvaluateRole` 生产入口。
- [ ] `cards.csv`/Card 类型不再含 `PromotionRoleId`；角色排序使用 `SelectionOrder` 或等价的非晋升字段；XLSX、CSV、Schema、生成器、fixture 与静态校验一致。
- [ ] 旧晋升存档迁移恢复可靠的原始角色，移除遗留 RuleFlags，正确修正当前/装备基线属性；无可靠原始 ID 时采用锁定回退且往返稳定。
- [ ] 角色固定能力、卡牌授予、装载选择、保存恢复聚焦自动化通过；Development Editor FullRebuild、项目校验、预构建检查和 `git diff --check` 通过。
- [ ] 用户 PIE 验收：复现原路径，在第 4 张及后续卡牌、跨第 5→6 场时角色不再突然变化。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@ac9bb58dafdc8129e4bcaf5e65fb1d87caa9e422`；Plan122 已提供构筑快照日志。
- 引擎/构建可用性：UE 5.8 Win64；基线已在 Plan122 最终候选完成 94/94 FullRebuild、静态校验和预构建核验。
- 现有聚焦测试结果：`ReEchoCharacterPromotionTests.cpp` 明确把“四卡后换角色”当作成功条件，属于与当前策划源表冲突的旧测试；同一文件中的智者额外选卡等角色能力测试仍为有效回归，必须迁移保留。
- 共享契约 / 难合并资源风险：修改 Run/Save、Card 公共字段、生产 XLSX/CSV、装载 UI 排序和共享预构建包；与并行的数据表、卡牌、商店、角色能力或 UI 工作有耦合，发布前需逐路径审计。
- 基线损坏时的停止条件：若删除晋升后角色固定能力、普通卡牌或当前主线存档测试失败，先区分旧晋升断言与真实回归；不得通过保留隐式换角或修改策划源表来让旧测试变绿。

## 实现提纲

1. 增加“选定角色身份跨授卡/跨关不变”和旧存档归一化测试，先把当前错误行为转为失败证据。
2. 从 Run 正常/调试授卡事务移除晋升调用，删除无消费者的 `ReEchoCharacterPromotion` 类型，并让新 Run 不再写晋升 RuleFlags。
3. 实现一次性旧存档归一化，处理 CharacterId、RoleId、当前属性、装备基线与遗留标志，补充往返测试。
4. 更新角色/卡牌类型化数据链：移除 `PromotionRoleId`，将纯展示顺序从 `PromotionPriority` 重命名为 `SelectionOrder`，同步 XLSX、CSV、Schema、生成器、fixture、校验与 UI。
5. 保留并迁移角色能力测试，更新三个模块文档和执行记录，完成聚焦自动化、最终 FullRebuild 与发布审计。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 数据权威 | `python scripts/data/sync_xlsx_to_csv.py --check`；`python scripts/validate_project.py` | ReEchoData.xlsx、CSV、Schema、字段允许列表与策划源语义一致 |
| 角色/卡牌 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Characters`；受影响 Card/Run 聚焦过滤器 | 角色身份稳定、四种固定能力与普通授卡通过 |
| 存档迁移 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Run.Save` 或准确 Save 过滤器 | 旧晋升标志迁移、属性修正和往返一致 |
| UI | 装载选择聚焦自动化 | 四角色顺序保持，UI 不读取晋升字段 |
| C++/发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`；`python scripts/ue/prebuilt_editor.py check` | UHT/UBT 成功并刷新匹配的 7 模块预构建包 |
| 静态 | `git diff --check`；遗留词与机器路径审计 | 生产代码/Schema 无 `TryPromote`、`PromotionRoleId`、`PromotionPriority` 遗留 |
| 人工 | 新局选择猎手或其他角色，连续选卡并推进至第 6 场 | 角色形象、CharacterId 和固定能力不发生隐式切换 |

## 执行记录

### 变化

- 2026-08-26：从策划源工作簿只读核验 9 个 Sheet；“晋升/转职/四张/4张/角色替换/Promotion/Promoted”全工作簿 0 命中。`角色体系J!A1:E5` 只定义四名角色的固定能力，`构筑体系G` 明确角色卡在选角后直接获得且不进入卡牌组，确认旧四卡晋升不是当前策划规则。
- 2026-08-26：代码审计定位旧机制：`TryPromote` 在普通与调试授卡后触发，达到 4 张卡即按 `PromotionRoleId` 改写 `Build.CharacterId`/`RoleId` 和属性；现有日志证明 `J_DIAMOND` 在 Encounter 5 选卡后于 Encounter 6 被改成 `J_HEART`。

### 证据

- 策划源表只读核验：`角色体系J!A1:E5`、`构筑体系G!A1:H12`、`投放系统!A1:D48`；晋升相关关键词 0 命中。
- Plan122 已发布至 `origin/main@ac9bb58d`，后续实施可用 `[BuildSnapshotTrace]` 比较授卡前后 `CharacterId` 与构筑指纹。

### 剩余风险

- 旧晋升存档可能已经把角色差额混入当前属性，需要基于保存的 `BaseCharacterId` 和当前角色做可逆迁移；缺失原始 ID 的异常旧档只能保守保留当前身份，不能无证据猜测玩家最初选择。

### 人工验收结果/请求

- `PendingBeforeClose`：实现完成后由用户按原构筑路径推进到 Encounter 6，确认角色不再变化。

### 架构文档审阅结果

- 待实现完成后填写。
