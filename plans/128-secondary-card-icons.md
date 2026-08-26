# Plan 128 - 程序 - 新增二级卡牌独立图标接入

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner 与 Executor）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Review`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@7b63217ccb32a04f022d2870e65c51e6fb547ea8`。
- 本地实现方式（可选，仅作交接说明）：独立 worktree `C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan128-secondary-card-icons`，分支 `plan/128-secondary-card-icons`。
- 依赖 / 阻塞：依赖 Plan69 的 `T_UI_CardIcon_{CardId}` 路径契约、Plan111 的新增卡牌稳定 ID，以及 `/Game/ReEcho/Textures/UI/Cards` 已纳入 AlwaysCook。执行 UE 导入/验证前需用户保存并关闭 Editor，并遵守同克隆 Unreal 锁。
- Writes:
  - `plans/128-secondary-card-icons.md`
  - `Content/SourceArt/UI/Cards/Icon/T_UI_CardIcon_G_2_{18..36}.png`
  - `Content/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_G_2_{18..36}.uasset`
  - `Content/SourceArt/UI/Cards/README.md`
  - `scripts/ue/import_plan128_secondary_card_icons.py`
  - `Design/UI/ReEcho_UI修改指导.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - 精选 Win64 Editor 预构建包及 manifest（仅最终发布门禁刷新）。
- Stable Reads:
  - `Design/Data/ReEchoData.xlsx` 与 `Content/Data/cards.csv`（只读卡牌 ID、名称、Tier、Enabled/Offerable 映射；不修改数据）。
  - `Source/ReEcho/{Public,Private}/UI/ReEchoTraitCardChoiceWidget.*`
  - `Source/ReEcho/Private/Run/ReEchoRunSubsystem.cpp`
  - `Config/DefaultGame.ini`
  - 外部交付目录 `C:\Users\gavynqiu\Downloads\二级卡牌新增\二级卡牌新增\`。
- 影响模式：`Isolated`。
- 兼容承诺 / 下游操作：不改变卡牌稳定 ID、名称、Tier、启用/投放资格、抽取/刷新事务、存档或 UI 布局；只补充现有动态路径会解析到的 Texture2D。二级卡仍按 CardId 加载独立 icon，未覆盖的其他 Tier 继续使用既有通用 fallback。
- 明确排除：不修改任何卡牌启用/投放状态、效果、数值或文案，不为三级卡制作或生成新图，不使用 AI 重绘用户交付图，不修改 WBP 布局。

## 锁定目标

将用户交付目录中的 19 张 `512×512` 二级卡牌图标按卡牌名称唯一映射到 `G_2_18`—`G_2_36`，以 `T_UI_CardIcon_{CardId}` 规范归档 SourceArt 并导入运行时 Texture2D。现有可投放二级卡中 18 张通用占位图全部替换为对应独立图；`G_2_30 连接，连接！` 使用本次交付覆盖既有版本。导入后所有 32 张可投放二级卡都具有独立 icon。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoUI`、`AREA-UI`；不修改 Runtime Module 源码或公共 API。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`，已加入 `Writes`。
- 设计意图：延续 Plan69 的稳定 CardId 路径解析，让美术资产替换不进入玩法数据或 Widget 分支逻辑；SourceArt 作为可追溯输入，UAsset 作为 Cook 后运行时表现。
- 权威状态与依赖：卡牌定义继续由权威 XLSX/生成 CSV 与 Cards/Run 负责；UI 只按既有 `CardId` 路径加载 Texture2D。`Config/DefaultGame.ini` 已将整个卡牌纹理目录加入 AlwaysCook，无需新增硬编码资源表或依赖方向。
- 决策记录：
  - 19 个交付文件按前缀顺序和中文名称均能唯一映射：`01/02 → G_2_18/G_2_19`，`03..19 → G_2_20..G_2_36`；`喂，打劫`、`就要那个`、`连接，连接` 与数据名只差末尾 `！`，卡序与其余名称共同消除歧义。
  - 所有文件均为 `512×512` PNG；18 张为 32-bit ARGB，`连接，连接` 为 24-bit RGB。保持用户原始像素与 Alpha，不做重采样、补透明或颜色处理。
  - 权威 CSV 的多行描述必须用标准 CSV 解析器读取；`G_2_18 数字挑战` 与 `G_2_19 诅咒银行` 实际均已启用且可投放。早期 PowerShell 逐行管道审计把多行字段拆坏，UE 导入脚本的标准 CSV 解析及时阻止了错误假设进入最终证据。
  - 运行时已有 `G_2_30` 资产，本次明确 `replace_existing=True`；其余 18 张新增。导入脚本必须校验准确对象路径和 Texture2D 可加载，避免静默继续 fallback。
- 相关文档同步范围：
  - `shared/CODEBASE_MAP/ARCHITECTURE.md`：关闭前审阅；预期模块拓扑和依赖不变。
  - `shared/CODEBASE_MAP/README.md`：关闭前审阅；预期标识和阅读路线不变。
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：更新二级卡图标覆盖和 fallback 边界。
  - `Design/UI/ReEcho_UI修改指导.md`：更新卡牌图标来源、命名、导入和替换说明。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅。
  - `README.md`：待审阅。
  - `MOD-ReEchoUI.md`：待更新。
  - `Design/UI/ReEcho_UI修改指导.md`：待更新。

## 锁定验收

- [x] 19 个交付 PNG 均以准确 CardId 命名归档，SHA-256 可追溯，尺寸保持 `512×512`，没有图像重绘或重采样。
- [x] `G_2_18`—`G_2_36` 共 19 个运行时 Texture2D 均可由 `/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_{CardId}` 加载；`G_2_30` 确认已被本批交付覆盖。
- [x] 当前 18 张可投放、原本缺独立 icon 的二级新增卡不再进入 `T_UI_Shop_CardIcon` fallback；所有 32 张可投放二级卡均有独立 icon。
- [x] 卡牌 CSV、稳定 ID、启用/投放状态、效果、商店/免费三选一与逐槽刷新行为没有变化。
- [ ] 纹理导入/加载检查、`ReEcho.UI.TraitCard` 回归、项目静态校验及最终 `-FullRebuild` 发布门禁通过。
- [ ] 用户在三选一界面确认新二级卡图标对应正确、清晰度和裁切可接受。
- [x] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`plan/128-secondary-card-icons@7b63217ccb32a04f022d2870e65c51e6fb547ea8`，准确基于当前 `origin/main`。
- 引擎/构建可用性：项目标准 UE 5.8 安装版与精选预构建包在主线可用；本任务导入和最终候选重新验证。
- 现有聚焦测试结果：`ReEcho.UI.TraitCard` 是既有卡牌 WBP/资源契约回归；不得把历史结果冒充本任务最终证据。
- 共享契约 / 难合并资源风险：主要二进制热点为卡牌 Icon UAsset，其中仅 `G_2_30` 是覆盖；Plan127 不写卡牌纹理或导入脚本，当前无 Writes 重叠。精选预构建包仅最终门禁刷新。
- 基线损坏时的停止条件：交付名称不能唯一映射、PNG 无法解码、导入后对象路径不符合既有解析契约、或必须修改卡牌数据/运行时加载逻辑才能显示时，停止扩张并报告用户。

## 实现提纲

1. 固化 19 项文件名—CardId—显示名映射及源哈希，把 PNG 机械复制为规范 SourceArt 名称。
2. 新增基于 `unreal.Paths.project_dir()` 的幂等导入脚本，覆盖 `G_2_30` 并导入其余 Texture2D，逐项验证对象路径和可加载性。
3. 运行卡牌资产覆盖审计与 `ReEcho.UI.TraitCard`，确认全部可投放二级卡命中独立 icon，卡牌启用/投放状态及玩法数据不变。
4. 更新 UI 资产文档、Plan 执行记录，完成最终构建、静态检查和人工验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 源图 | 文件名映射、SHA-256、PNG 尺寸/像素格式检查 | 19/19 唯一映射且为 512×512；字节未被处理 |
| 导入/加载 | UE 5.8 执行 `scripts/ue/import_plan128_secondary_card_icons.py` | 19/19 对象路径准确、Texture2D 可加载，`G_2_30` 已覆盖 |
| 资产覆盖 | UE 内运行卡牌资产审计 | 全部 Enabled+Offerable Tier2 不再 fallback；Tier3 现有缺图不被误报为本任务失败 |
| UI 回归 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.UI.TraitCard` | WBP 绑定、CardId 图标路径与三选一表现契约通过 |
| 静态 | `python scripts/validate_project.py`、`git diff --check` | 项目、AlwaysCook、UTF-8 与生成物边界通过 |
| 最终发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终组合候选成功并刷新匹配的精选 Editor 包 |
| 人工 | 在免费或商店二级三选一中查看本批卡牌 | 图标名称对应、清晰、无错误裁切或通用占位 |

## 执行记录

### 变化

- 已将用户交付的 19 张 PNG 按 `G_2_18`—`G_2_36` 规范名称原字节归档到 SourceArt；`G_2_30` 旧源图由本批新交付覆盖，其余 18 张为新增。
- 已新增项目路径无关的幂等 UE 导入脚本，逐项校验 CSV 名称/Tier/启用投放状态、对象路径、Texture2D 尺寸以及全部可投放二级卡的独立图标覆盖。

### 证据

- Step 0 只读核对：交付目录含 19 张 `512×512` PNG；19 个中文名称均能唯一映射到 `cards.csv` 的 `G_2_18`—`G_2_36`。更正含多行描述的 CSV 解析后，确认当前可投放二级卡缺独立 icon 的 18 张全部位于该批，`G_2_30` 已有旧 icon，本批 19 张均对应启用且可投放卡牌。
- SourceArt 归档后逐项比较外部交付与仓库文件 SHA-256，结果 `SOURCE_BYTE_MATCH=19/19`；18 张保持 `Format32bppArgb`，`G_2_30` 保持交付原始 `Format24bppRgb`，全部为 `512×512`。
- UE 5.8 幂等导入复跑成功，日志为 `[Plan128][CardIconImport] imported=19 active_tier_two_with_icons=32`；脚本逐项验证对象路径、Texture2D 类型和 `512×512` 尺寸，并确认全部 Enabled+Offerable Tier2 卡都命中独立资源。
- `ReEcho.UI.TraitCard.AuthoredPresentation` 为 `Result={Success}`；`python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check` 与 `git diff --check` 通过。最终发布前仍需在准确最终集成候选上执行 `-FullRebuild`。

### 剩余风险

- 自动化可证明对象路径、尺寸与加载成功，不能替代玩家界面中的主观清晰度、构图和裁切判断。

### 人工验收结果/请求

- `PendingBeforeClose`：实现后请用户在二级卡牌三选一中检查对应关系和显示效果。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅、无需修改；只增加现有 UI 纹理资产，不改变 Runtime Module 拓扑、状态所有者或依赖方向。
- `shared/CODEBASE_MAP/README.md`：已审阅、无需修改；稳定标识和阅读路线不变。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：已更新二级卡独立图标覆盖、AlwaysCook 与其他 Tier fallback 边界。
- `Design/UI/ReEcho_UI修改指导.md`：已更新卡牌 icon SourceArt、运行时路径、Plan128 导入方式及不得按中文名称新增分支的约束。
