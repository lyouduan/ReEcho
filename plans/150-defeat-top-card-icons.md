# Plan 150 - 程序 - 失败结算最高等级卡牌图标

## 协调

- Planner 负责人：Codex（程序路线，规划执行者合一）。
- Executor 负责人：Codex（程序路线，规划执行者合一）。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@9f5fc380aa55a22f189c51ca7873b237a096617c`。
- 本地实现方式（可选，仅作交接说明）：独立分支 `codex/plan150-defeat-top-card-icons` 与独立 worktree `ReEcho-plan150-defeat-top-card-icons`。
- 依赖 / 阻塞：GitHub 人类账号已确认为 `JosephLE910`；最终视觉仍需要在持有少于五张、恰好五张和多于五张卡牌的失败结算状态下人工验收。
- Writes: `plans/150-defeat-top-card-icons.md`；`Source/ReEcho/Public/Run/ReEchoRunSubsystem.h`；`Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`；`Source/ReEcho/Public/UI/ReEchoRestartWidget.h`；`Source/ReEcho/Private/UI/ReEchoRestartWidget.cpp`；`Source/ReEcho/Private/ReEchoGameMode.cpp`；相关 `Source/ReEcho/Private/Tests/*.cpp`；`Content/ReEcho/UI/WBP_ReEchoRestart.uasset`；Plan150 的 WBP 作者ing/审计脚本；`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`；构建门禁允许的精选 Editor 预构建产物。
- Stable Reads: `Source/ReEchoCards/Public/Cards/ReEchoCardTypes.h`；`Source/ReEcho/Private/UI/ReEchoInventoryShopWidget.cpp`；`Source/ReEcho/Public/Run/ReEchoShopCatalog.h`；`Content/ReEcho/UI/WBP_ReEchoRestart.uasset`；`Content/ReEcho/Textures/UI/Cards/Icon/**`；`shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md`；`Design/UI/ReEcho_UI修改指导.md`。
- 影响模式：`SharedContract`（扩展既有 Death 结算只读投影和 `SetDeathScreen` 输入，但不改变 Cards/Run 状态所有权、存档 Schema 或卡牌授予规则）。
- 兼容承诺 / 下游操作：现有五个 `DesignerDefeatCardSlot0..4` 的位置、尺寸和空槽美术保持不变；各槽增加一个正方形 `DesignerDefeatCardIcon0..4` 作者化覆盖层，避免把正方形卡图拉伸成纵向或覆盖槽框；没有可展示卡牌或独立图标缺失时安全隐藏覆盖层/使用商店同款通用卡牌图标；Victory、Pause、重开和返回主菜单流程不变。
- 明确排除：不改变卡牌等级、授予、叠层、抽取、商店排序或存档；不新增结算 WBP；不重排同等级卡牌的获得顺序；不把卡牌说明、价格、Tooltip 或可点击交互加入失败结算。

## 锁定目标

玩家进入失败结算时，页面下方既有五个卡牌槽按等级从高到低展示当前构筑中最多五张卡牌的真实图标。等级取权威卡牌定义的 `Tier`；同等级保持 `OwnedCardIds` 的稳定获得顺序。少于五张时剩余位置继续显示 WBP 已有空槽美术。卡牌图标路径、加载方式和缺图回退与商店已拥有卡牌槽一致；新增正方形作者化 Icon 覆盖层，保持既有槽位底板和整体布局不变。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`MOD-ReEchoUI`、`AREA-Run`、`AREA-UI`。`MOD-ReEchoCards` 仅作为稳定卡牌 `Tier` 和所有权状态来源读取，不修改其契约或实现。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 与 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`，均已加入 `Writes`；审阅 `MOD-ReEchoCards.md`，预计因状态所有权不变而无需修改。
- 设计意图：Run 继续拥有卡牌构筑和目录适配，并提供与商店一致的已拥有卡牌只读投影；GameMode 只把死亡时的真实投影交给 Restart UI；Restart Widget 只排序最多五项并替换现有槽位 Brush，不解释卡牌规则或写回构筑。
- 权威状态与依赖：不改变状态所有者、存档 Schema、模块拓扑或依赖方向。新增/复用的投影只包含稳定 ID、`Tier`、展示文字和图标路径；等级仍来自 Cards 目录，所有权仍来自 `FReEchoCardBuildState::OwnedCardIds`。
- 决策记录：
  1. 复用商店构建 `FReEchoShopOffer` 的已拥有卡牌投影，避免在结算页复制卡牌图标路径规则。
  2. 保留现有五个 `DesignerDefeatCardSlot0..4` 的空槽 Brush，并在 WBP 中增加 `DesignerDefeatCardIcon0..4` 正方形覆盖层；运行时只替换覆盖层 Brush 和显隐，不创建或回写几何。直接替换纵向空槽 Brush 会拉伸正方形 Icon 且丢失槽框，因此不采用。
  3. 使用稳定降序排序：高 `Tier` 优先，同 `Tier` 保留获得顺序；这样满足“最高等级五张”且不会在每次刷新时随机换位。
  4. 结算统计的构筑数量同步读取 `CardState.OwnedCardIds.Num()`，不继续读取仅供旧存档迁移的 `CurrentBuild.Cards`。
- 相关文档同步范围：维护 `MOD-ReEcho.md` 的 Run→结算真实卡牌投影；维护 `MOD-ReEchoUI.md` 的 Death 五槽运行时 Brush 契约；审阅 `CODEBASE_MAP/ARCHITECTURE.md`、索引 `README.md` 和 `MOD-ReEchoCards.md`，无拓扑、路由或 Cards 状态事实变化时只在执行记录说明无需修改。
- 关闭前逐项填写审阅结果：
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 已更新：记录 `GetOwnedBuildCardView()` 的获得顺序、图标契约和失败结算只读消费边界；
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 已更新：记录五个正方形覆盖层、等级排序、商店同款回退和空槽保留；
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoCards.md` 已审阅，无需修改：`Tier` 与 `OwnedCardIds` 的权威和语义未变；
  - `shared/CODEBASE_MAP/ARCHITECTURE.md` 已审阅，无需修改：模块拓扑和依赖方向未变；
  - `shared/CODEBASE_MAP/README.md` 已审阅，无需修改：无稳定标识或阅读路由变化。

## 锁定验收

- [x] 失败结算按 `Tier` 从高到低展示最多五张当前拥有卡牌；同 `Tier` 保持获得顺序。
- [x] 结算图标与商店已拥有卡牌使用同一 `IconTexturePath` 和通用缺图回退，不显示伪造卡面。
- [x] 少于五张时剩余 Icon 覆盖层隐藏、原有空槽美术完整保留；零张时五个空槽均不被通用卡图覆盖。
- [x] 五个 `DesignerDefeatCardIcon0..4` 是 WBP 中可独立调整的正方形 Image，位于对应空槽上方且不拦截输入，不拉伸正方形卡图。
- [x] `DefeatTraitCountValue` 显示 `CardState.OwnedCardIds.Num()` 的真实构筑数量。
- [x] Victory、Pause、失败重开、返回主菜单、按钮悬停和既有 WBP 几何无回归。
- [x] 功能结果有聚焦自动化和运行时/资产可观察证据。
- [x] 必需构建、项目校验、格式与差异检查通过。
- [ ] 失败结算卡图填充、裁切与可读性经人工 PIE 验收。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`codex/plan150-defeat-top-card-icons`，基于 `origin/main@9f5fc380aa55a22f189c51ca7873b237a096617c`。
- 引擎/构建可用性：仓库标准 UE 5.8 脚本存在；独立 worktree 的 `python scripts/setup_lfs.py --check` 已通过。
- 现有聚焦测试结果：Plan129 已记录 `ReEcho.UI.RestartWidgetPresentation`、WBP 编译与正式失败五槽审计通过；本 Plan 实现前未重复运行 UE 自动化。
- 共享契约 / 难合并资源风险：`ReEchoRestartWidget`、`ReEchoGameMode` 与 `WBP_ReEchoRestart.uasset` 是共享 UI/流程入口；WBP 是二进制热点，只通过仓库 UE 5.8 作者ing脚本在本任务 worktree 修改，并在合入最新主线后重做编译/审计。远端基线自最初核对后改过 `GameMode`、`RunSubsystem` 和 UI 文档，已选择直接基于最新远端实现。
- 基线损坏时的停止条件：若五个 `DesignerDefeatCardSlot0..4` 缺失、卡牌目录无法给出 Tier/图标投影、既有 Restart 聚焦自动化在未修改基线上失败，或实现必须改变 Cards/存档 Schema，则停止越界部分并回报。

## 实现提纲

1. 抽取无副作用的已拥有卡牌只读投影，让商店与结算复用同一图标路径和通用回退约定。
2. GameMode 在死亡入口传入真实卡牌投影并改用 `OwnedCardIds` 统计构筑数量。
3. 在 WBP 的五个既有空槽上增加正方形 `DesignerDefeatCardIcon0..4` 覆盖层；Restart Widget 稳定选出高 Tier 的最多五项，只填充覆盖层 Brush 和显隐。
4. 增补投影排序、五槽覆盖层填充、少卡隐藏、缺图回退以及 Victory/Pause 回归的聚焦自动化和 WBP 作者ing审计。
5. 更新模块文档与执行记录，完成格式化、构建、项目校验、差异审计和人工 PIE 验收交接。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| C++ 格式 | 仓库 `.clang-format` 格式化修改的 `.h/.cpp` 并审查 diff | Unreal C++ 风格一致且无无关格式改写 |
| 聚焦自动化 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.Restart`，并运行新增 Run/投影测试的准确过滤器 | 高 Tier 选择、稳定同级顺序、五槽 Brush、空槽和既有结算状态通过 |
| Editor 构建 | `scripts/ue/Build-Editor.cmd -Configuration Development`；发布前 `-FullRebuild` | UHT/UBT 成功，最终候选刷新匹配精选预构建包 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目、源码、文档和文本格式检查通过 |
| LFS/内容 | `python scripts/setup_lfs.py --check`、`git lfs status`、`git lfs fsck` | 既有 WBP 与图标资源完整，未引入指针或缺失对象 |
| WBP 作者ing | Plan150 UE 脚本 + `CompileAllBlueprints` | 五个正方形 Icon Image 位于对应槽位上方、可独立编辑且编译无错误 |
| 人工验收 | PIE 构造 0、1–4、5、6+ 张且混合 Tier 的死亡结算 | 卡图位于五个原槽内且不变形，排序、裁切、留空和页面交互符合预期 |

## 执行记录

### 变化

- 已确认现状：正式失败页存在五个 `DesignerDefeatCardSlot0..4` 静态空槽；C++ 当前只投影构筑数量，不填充真实卡图。
- 已确认商店路径：`UReEchoInventoryShopWidget::RebuildOwnedCardSlots` 消费 `FReEchoShopOffer::IconTexturePath`，缺图回退通用卡牌图标；Run 的 `MakeOwnedBuildCardOffer` 统一生成 `/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_{CardId}`。
- 已确认权威等级与所有权：`FReEchoCardDefinition::Tier` 和 `FReEchoCardBuildState::OwnedCardIds`；旧 `CurrentBuild.Cards` 仅用于旧存档迁移。
- `UReEchoRunSubsystem::GetOwnedBuildCardView()` 已成为商店与失败结算共用的已拥有卡牌只读投影；GameMode 的结算数量改读 `OwnedCardIds.Num()`，仅死亡模式传入卡牌展示数据。
- Restart Widget 以稳定 Tier 降序选取最多五项，只修改 `DesignerDefeatCardIcon0..4` 的 Brush 与显隐；WBP 新增五个 `136×136`、ZOrder 50、默认 Hidden 的作者化 Image，原 `136×164` 空槽与 ZOrder 45 保持不变。
- 聚焦自动化已覆盖六张混合 Tier 的 Top 5、同 Tier 稳定顺序、缺图回退、少卡/零卡隐藏、原空槽 Brush 和 Victory/Pause 状态回归。

### 证据

- `python scripts/setup_lfs.py --check`：通过，当前独立 worktree LFS hydrated。
- `origin/main@9f5fc380` 的 Plan129、Restart 自动化、正式失败作者ing/审计脚本共同证明五个可独立编辑槽位存在。
- Development Editor 增量构建通过并刷新精选预构建包：`modules=7 build_id=55116800 source=78c3ad030f03`；该结果只作为实现期反馈，最终发布仍执行 `-FullRebuild`。
- `ReEcho.UI.RestartWidgetPresentation`：`Result={Success}`；覆盖正式失败 Top 5、空槽与缺图回退，并继续覆盖 Pause/Victory/按钮绑定。
- `ReEcho.Shop`：16 项全部通过，确认抽取共用投影后商店行为无回归。
- `audit_plan150_defeat_card_icons.py`：输出 `[Plan150DefeatCardsAudit] five square card-icon overlays are valid`，并在审计末尾成功编译目标 WBP。
- `python scripts/validate_project.py` 与 `git diff --check`：通过。
- 持有 `main-publish-lock` 的最终组合候选执行 `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild`：`Result: Succeeded`，精选包为 `modules=7 build_id=55116800 source=78c3ad030f03`；仅有既存 `FImageUtils::CompressImageArray` 弃用警告。
- 最终 `CompileAllBlueprints`：`0 errors / 0 warnings / 0 blueprints that failed to load`；命令汇总另列 4 条既有引擎/Legacy 提示。
- 最终 `ReEcho.UI.RestartWidgetPresentation`：1/1 `Result={Success}`；最终 `ReEcho.Shop`：16/16 `Result={Success}`。
- 最终 `python scripts/ue/prebuilt_editor.py check`、`python scripts/validate_project.py`、`python scripts/setup_lfs.py --check`、`git lfs status`、`git lfs fsck` 与 `git diff --check` 均通过。

### 剩余风险

- `WBP_ReEchoRestart.uasset` 是共享二进制热点；作者ing和最终集成必须重新审计远端同路径变化，不能用脚本重建覆盖其他页面状态。
- 实际卡图宽高比和槽位裁切需要 PIE 人工确认，自动化只能验证资源与显隐绑定。

### 人工验收结果/请求

- 技术候选完成后请求用户在 PIE 验证混合 Tier 的失败结算排序、图标裁切和少卡留空；人工结果未记录前保持 `Review`，不关闭 Plan。

### 架构文档审阅结果

- `MOD-ReEcho.md` 与 `MOD-ReEchoUI.md` 已按上方范围更新；`MOD-ReEchoCards.md`、`ARCHITECTURE.md` 和 CODEBASE_MAP 索引均已审阅且无需修改。
