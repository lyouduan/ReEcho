# Plan 45 - UI - 交互占位美术资产接入

## 协调

- Planner 负责人：Codex。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`InProgress`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：`origin/main@7a89328ebb9afb433199d56b975763e1775c7cef`。
- 本地实现方式：Start Menu 首批已在 `main` 发布；剩余页面在独立 worktree 小批量接入，每个 WBP 串行修改。
- 后续隔离工作区：`C:/Users/gavynqiu/Documents/miniGame/ReEcho-plan45-ui`，分支 `plan/45-ui-interaction-assets`，从 `origin/main@87cde6c8534b484e020f4b16b656eebe831a1836` 继续剩余页面批次。
- 依赖 / 阻塞：沿用 Plan29 UMG/C++ 边界和 Plan34 Settings 绑定契约；占位字体缺少授权/来源证明，本 Plan 不导入运行时字体。
- Writes: `Content/SourceArt/UI/InteractionPlaceholder/**`；`Content/ReEcho/Textures/UI/InteractionPlaceholder/**`；与既有页面对应的 `Content/ReEcho/UI/WBP_ReEcho*.uasset`；`Source/ReEcho/{Public,Private}/UI/**` 中与 Start Menu、Settings、Restart/Pause、Trait Choice、Inventory/Shop、HUD 对应的 Widget；`Source/ReEcho/{Public,Private}/ReEchoGameMode.*`；`Config/DefaultEngine.ini` 的 UI focus rule；必要的可复现导入脚本 `scripts/ue/**`；`docs/ART_ASSET_ORGANIZATION.md`；`Design/UI/ReEcho_UI修改指导.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`；本 Plan。
- Stable Reads: `Source/ReEcho/{Public,Private}/UI/**`；`Source/ReEcho/{Public,Private}/ReEchoGameMode.*`；现有 WBP 绑定和页面流程；交付包 `正式-交互占位.zip`。
- 影响模式：`Exclusive`，源图和新纹理目录为独立新增，每个修改的 WBP 是不可文本合并的二进制资产。
- 兼容承诺 / 下游操作：保留全部原生父类、`BindWidget`/`BindWidgetOptional` 名称和类型、稳定屏幕 ID、页面 Delegate、输入/焦点/暂停和存档语义；只替换或补全表现。
- 明确排除：不新增存档列表/历史时间线或“关于我们”页面；“存档回溯”复用现有 Continue 单存档恢复语义；开始页新增直接退出请求，但不改变暂停页保存退出流程；不导入授权未确认的 TTF；不修改玩法、Schema、平衡、存档格式或数据表；不把 1920×1080 效果图当作正常运行时整屏贴图。

## 锁定目标

将人工提供的“正式-交互占位”包以可追溯、可重复导入的方式纳入 ReEcho，并将可用切图接入已存在的 Start Menu、Settings、Pause/Restart、Trait Choice、Inventory/Shop 和 HUD 表现，不改变玩法、页面流程和权威状态。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoUI`（文档型逻辑模块）、`AREA-UI`；不新增 Runtime Module。
- 对应模块文档：`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md` 已纳入 `Writes`，关闭前核对资产路径、WBP/C++ 边界和验证路线。
- 设计意图：源 PNG/参考图与 Unreal 运行时纹理分层；WBP 持有布局和样式，C++ 继续只管类型化绑定、事件和生命周期。
- 权威状态与依赖：不变更任何运行时状态所有者、公共 API、模块依赖或页面生命周期。
- 决策记录：交付包中 1920×1080 图和“说明”图作为 `References`；透明切图作为 `Elements`；只把实际 WBP 消费的元素导入 `/Game/ReEcho/Textures/UI/InteractionPlaceholder`；字体在授权确认前留在 `PendingLicense`。
- 相关文档同步范围：`shared/CODEBASE_MAP/ARCHITECTURE.md` 关闭前必审，预期因拓扑不变而无需修改；`shared/CODEBASE_MAP/README.md` 关闭前必审，预期路由不变；`MOD-ReEchoUI.md`、`docs/ART_ASSET_ORGANIZATION.md` 和 `Design/UI/ReEcho_UI修改指导.md` 同步资产落点、导入和人工验收。
- 关闭前逐项填写审阅结果。

## 锁定验收

- [x] 交付包安全解压到 `Content/SourceArt/UI/InteractionPlaceholder/`，按 `References`、`Elements`、`Fonts/PendingLicense` 分层，保留来源、尺寸和 SHA-256 清单。
- [x] Start Menu 首批只把现有页面实际需要的切图通过 Unreal Editor/可复现导入脚本导入 `Content/ReEcho/Textures/UI/InteractionPlaceholder/StartMenu/`，使用稳定 ASCII 资产名，不在 `SourceArt` 中生成 `.uasset`。
- [x] `WBP_ReEchoStartMenu` 使用新资产，所有原生父类、绑定控件名/类型、Delegate、屏幕层级、焦点和页面流程保持兼容。
- [x] 占位字体未经授权确认不导入/不用于运行时、不提交分发；“存档回溯/关于我们”效果图只作参考。
- [x] Start Menu WBP 编译且 `CompileAllBlueprints` 无错误/加载失败，导入资产可加载，`python scripts/validate_project.py` 和 `git diff --check` 通过。
- [x] Start Menu 对齐已确认参考：新游戏/存档回溯使用交付按钮视觉；无存档时存档回溯保持显示但置灰禁用；设置为右上角同风格图标；关于我们隐藏；退出位于右下并直接退出。
- [x] Trait Choice 按两张“通关后3选1”效果图组装三卡、未选中压暗、选中强调、标题/确认区；动态卡名、说明和选择事件仍由现有 C++ 数据驱动。
- [ ] Restart/Pause 按胜利、失败、重开确认、普通暂停、退出到主菜单确认、退出游戏确认六种效果组装；复用现有多状态 Widget，不把效果图作为整屏运行时贴图。
- [x] Settings 按画面/声音/键位三张效果图组装页签、表单区、关闭/恢复/应用按钮；已有音频控件保持真实绑定，尚无运行时能力的画面/键位字段仅作明确占位展示。
- [ ] Inventory/Shop 按商店、卡牌说明、属性面板效果图组装左右双区、动态商品卡、武器/配件槽和悬浮信息层；购买、货币、已拥有状态继续由现有 C++ 数据驱动。
- [ ] Player/Encounter HUD 按战斗场景效果图组装血条、时间碎片、时钟/指针、轨迹板和技能栏；不存在数据契约的装饰不伪造玩法状态。
- [ ] 用户在 PIE 验收 Start Menu、Settings、Pause/Restart、Trait Choice、Inventory/Shop 和 HUD 的布局、中文可读性、点击/键盘焦点、返回路径以及 1280×720、1920×1080、2560×1440 和 21:9 DPI 表现。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成产物、原始 ZIP 或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`main` / `7a89328ebb9afb433199d56b975763e1775c7cef`，与 `origin/main` 对齐。
- 引擎/构建可用性：UE 5.8 安装版和仓库 Windows 工具链可用；发布 Plan 前按程序路线执行 `-FullRebuild`。
- 现有聚焦测试结果：复用 `origin/main@7a89328` 已发布的 Editor 包作为静态基线；实现候选不复用旧 WBP/PIE 主观证据。
- 共享契约 / 难合并资源风险：`WBP_ReEchoSettings` 与 Plan34 相关，修改前再次核对其他 worktree 差异；任一 WBP 同时被 Editor 修改时停止该资产批次。
- 基线损坏时的停止条件：远端出现未批准新提交；ZIP 包含路径穿越/重名冲突或文件损坏；目标 WBP 正被另一 Editor/worktree 修改；必须新增页面流程、C++ 绑定或公共契约才能继续；必需字体授权未获确认。

## 实现提纲

1. 验证 ZIP 入口、尺寸、重名和解压后根路径，按参考图/切图/字体分类并生成来源清单。
2. 审计既有 WBP 与切图映射，为实际消费的图片建立稳定 ASCII 名称和确定性导入配置。
3. 通过 Unreal Editor 导入纹理，逐页编辑、Compile 和 Save WBP；先完成 Start Menu 样板批次，再扩展 Settings、Pause/Restart、Trait、Shop 和 HUD。
4. 对每个批次检查父类、绑定、引用、焦点/输入和资产加载；失败时停在当页，不将推测性修复堆叠到下一页。
5. 同步 UI/资产文档和 Plan 执行记录，执行客观门禁后交给用户做 PIE/视觉验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 源包 | ZIP 入口安全、文件数/尺寸/SHA-256 清单与实际文件对齐 | 无路径穿越、绝对路径、重名覆盖或非预期扩展名 |
| 导入 | Unreal Editor 导入/加载检查 | 运行时纹理只在 `/Game/ReEcho/Textures/UI/InteractionPlaceholder/**`，透明通道和 UI 压缩/过滤设置正确 |
| WBP | 逐资产 Compile/Save，然后 `CompileAllBlueprints` | 父类和绑定契约保持，0 compile errors、0 failed loads |
| 静态 | `python scripts/validate_project.py`；`git diff --check` | 项目/源码/资产规约与差异通过 |
| 发布 | `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` | 最终精选 Editor 包与候选指纹匹配 |
| 人工 | 具名 PIE 页面/分辨率/焦点清单 | 用户报告 `Passed` 或具体返工项 |
| 范围 | 显式改动路径审计 | 无原始 ZIP、未授权运行时字体、新页面、玩法逻辑或未精选 UE 产物 |

## 执行记录

### 变化

- Plan 45 已发布并进入执行；完成源包分类/清单，并以 Start Menu 作为首个 WBP 接入样板。
- `正式-交互占位.zip` 的 101 个文件已归档为 77 个可评审切图、21 个参考图和 3 个待授权字体文件；原始 ZIP 未复制入仓库。
- 新增可复现导入脚本，首批导入背景、标题 Logo、设置图标三张 UI Texture2D。
- `WBP_ReEchoStartMenu` 新增命中测试不可见的全屏背景和左侧标题层，将现有按钮组移到右侧；设置图标嵌入既有 `GameSettingsButton`，原有绑定控件和按钮语义未替换。
- 剩余批次导入 13 张实际被现有 WBP 消费的切图；Settings、Restart、Trait Card、Inventory/Shop、Player/Encounter HUD 和 Stats 页面已加入命中测试不可见的美术层。
- `WBP_ReEchoInventoryShopScreen` 的商店/装配室底板与标题留在原 `ShopPanel` / `InventoryPanel` 子树中，继续服从 C++ 的模式显隐；未修改 C++、状态所有权、页面流程或绑定类型。
- Settings 视觉返工将弹窗底板居中置于遮罩之上，画面/声音/键位三个页签改为贴合弹窗上沿的便签条；当前页使用白色素材态，未选中页统一置灰。三页文字、字段框、关闭/恢复/应用素材重新对齐，画面亮度与声音滑条均按 `65%` 展示。
- 导入脚本默认跳过已存在纹理，显式 `-Plan45ReimportExisting` 才执行同名重导，避免重复运行改写已发布的 Start Menu 资产。
- 用户在首轮视觉验收后明确锁定开始页语义：`存档回溯=继续当前存档`；无存档时显示但置灰禁用；关于我们先隐藏；退出直接退出；设置按参考图右上角图标风格呈现。本轮据此返工 Start Menu，而不创建新的存档/关于页面。
- 资产组织、UI 修改指导和 `MOD-ReEchoUI` 路由已同步；未新增 Runtime Module 或依赖拓扑。
- Settings 下拉框选择和滑条调节完成后补发单次 `UI.Confirm`；拖动过程不逐帧刷音效，音频失败仍不影响设置结果。

### 证据

- Plan 编号前 fetch：`main == origin/main == 7a89328ebb9afb433199d56b975763e1775c7cef`，工作区干净。
- Plan-only 发布：`origin/main@131b084ac008bc35ecc8a6e6219c9bde065a6343`；`-FullRebuild`、`python scripts/validate_project.py` 和 `git diff --check` 通过。
- 交付包静态目录：98 张 PNG、2 个 TTF、1 个字体说明 TXT；已识别 1920×1080 效果图与独立切图。
- `_SourceManifest.csv` 与归档目录逐文件复核：101/101 存在，字节数和 SHA-256 全部匹配，0 errors。
- Unreal Editor 导入日志记录 3 张审核后纹理；WBP 资产依赖明确包含 `T_UI_Start_Background`、`T_UI_Start_TitleLogo`、`T_UI_Start_SettingsIcon`。
- UMG ToolSet 对 `WBP_ReEchoStartMenu` 的 Compile/Save 成功，保存后资产 `is_dirty=false`；既有 `StatusText`、`ContinueButton`、`NewGameButton`、`GameSettingsButton` 名称和类型保留。
- `CompileAllBlueprints` 退出码 0；`WBP_ReEchoStartMenu` 编译成功，汇总为 0 errors、0 warnings、0 blueprints failed to load。
- 最终 `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` 成功，刷新精选 5-module Editor bundle（source fingerprint `36ade36d8b29`）；构建后 `python scripts/validate_project.py` 与 `git diff --check` 通过。
- 剩余批次的 13 张纹理通过 Unreal Editor 命令行导入并保存；七个受影响 WBP 均经 UMG ToolSet 编译成功、保存后 `is_dirty=false`，依赖查询逐页命中计划纹理。
- `CompileAllBlueprints` 在独立 worktree 候选上退出码 0，汇总为 0 errors、0 warnings、0 blueprints failed to load。
- Start Menu 返工后 UMG 编译/保存成功；控件树核验为 17 个控件，`ContinueButton`、`NewGameButton`、`GameSettingsButton`、`QuitButton` 均保留为 `ReEchoIndexedButton`，五张 Start Menu 纹理均由表现层引用。
- Start Menu 返工后的 `CompileAllBlueprints` 汇总为 0 errors、0 warnings、0 blueprints failed to load；`ReEcho.UI.IntermissionContexts` 自动化测试通过，项目静态验证和 `git diff --check` 通过。
- 用户确认截图中的外框是 Unreal 默认紫色虚线焦点框；项目 UI focus rule 改为 `Never`，隐藏默认描边但保留键盘/手柄焦点与导航语义。
- 用户进一步明确每个交付目录中的 1920×1080 大图是最终呈现规范，独立 PNG 是组装素材；此前“只加部分装饰层”的候选不足以验收。本轮据此重新进入 InProgress，逐页按效果图构图，同时保留现有真实数据、状态机和按钮语义。
- 导入映射扩展为 72 张审核后切图；1920×1080 效果图仍只作为视觉规范，不进入运行时 Texture2D 映射。
- Trait Choice 改为“先选中、再确定”的两步流程，三张卡按效果稿 344×540 构图，补齐卡图占位和双标签；选中卡强调、其余卡压暗，确认后才广播原有卡牌选择事件。
- Settings 将原有真实音频控件重排到弹窗画布，并新增画面/声音/键位页签美术、右上关闭、恢复默认、应用按钮；画面和键位页用明确标注的静态占位字段呈现，不伪造尚未接入的运行时能力。
- Restart/Result 新增胜利/失败标题、结算底板、已选卡牌区、主体立绘和退出确认底板，由既有 `ScreenMode` / `QuitPromptState` 切换显隐。
- Inventory/Shop 已按左右双区摆放商店与装配室，装配室补齐武器、四个配件槽、属性面板、时钟和隐藏的技能说明浮层；HUD 补齐时钟指针、轨迹地图和技能栏。
- 七个本轮修改的 WBP 逐资产 Compile/Save 均成功；随后 `CompileAllBlueprints` 汇总 0 errors、0 warnings、0 blueprints failed to load。
- `ReEcho.UI.IntermissionContexts` 聚焦自动化测试通过，`python scripts/validate_project.py` 通过；全量 `ReEcho` 自动化仍有两个与本 UI 批次无关的既存失败：`ReEcho.Presentation.Animation2D.AssetProfiles` 与 `ReEcho.Run.EchoReplayResolver.EmptyStale`。
- Settings 返工后在 PIE 中逐页抓取画面、声音、键位运行截图：页签白/灰状态、黑字白底字段、素材版恢复/应用按钮及控件对齐均可见；运行态复核发现并修正画面亮度条从 100% 填充到与标签一致的 65%。
- 最终 `WBP_ReEchoSettings` UMG Compile/Save 成功；`CompileAllBlueprints` 退出码 0，汇总 0 errors、0 warnings、0 blueprints failed to load；`ReEcho.UI.IntermissionContexts` 1/1 通过，`python scripts/validate_project.py` 与 `git diff --check` 通过。
- 合入 `origin/main@86bf9a4` 后，`ReEcho.UI` 2/2 与 `ReEcho.Audio` 13/13 聚焦自动化通过；23 个生成音效、31 行音频目录/常量/生产路由与权威 XLSX/CSV 同步检查通过。最终 Development `-FullRebuild` 73/73 actions 成功，精选 5-module Editor bundle 刷新为 source fingerprint `b1af958a1b5f`，构建后项目静态验证与 `git diff --check` 通过。
- 回响存储底部托盘候选在 `origin/main@39136cd` 基线上完成 Development Editor 增量构建（7/7 actions），精选 5-module Editor bundle 刷新为 source fingerprint `9d40d8818161`；随后 `python scripts/validate_project.py` 与 `git diff --check` 通过，位置、尺寸和按钮可用性等待用户 PIE 验收。
- 用户运行态复测发现音量滑条无法拖动；首轮层级审计修正装饰父级 `*VolumeVisualOverlay` 的整树禁用命中后，二次运行态反馈仍失败。进一步审计 OverlaySlot 定位真实 Slider 被交付 WBP 设为 Left/Top 对齐，只在宽轨道左上角保留极小命中区域。三组 Slider 现强制 Fill/Fill 覆盖完整轨道；装饰父级使用 `SelfHitTestInvisible`，Track/Fill 保持不可命中，并新增父级命中模式、Slider enabled/unlocked 与完整槽位覆盖回归断言。
- 用户确认不为 `Ambience_Rain` 增加偏离目标稿的第四条滑条；交付版三滑条布局中的“音效音量”聚合写入 `Ambience`、`CombatSfx`、`UiSfx`，五滑条 C++ fallback 仍保持各总线独立控制。
- 通关后商店的回响存储面板从左侧大面积锚点迁入底部紧凑缩放托盘，并提升到商店表现层之上；托盘自身不拦截鼠标，已有存储、跳过、替换和回放选择按钮继续参与命中测试。

### 剩余风险

- TTF 仅有占位用途说明，没有授权/来源证明；保留在本地 `PendingLicense` 并由 `.gitignore` 排除，不导入运行时、不提交分发。
- 交付效果图展示了尚无现有产品契约的页面/操作，本 Plan 不根据图片自行创建。
- 所有计划内页面已进入逐页接入批次并通过客观蓝图门禁；主观视觉、按钮点击/键盘焦点、显隐状态和多分辨率 DPI 表现仍需用户 PIE 验收。
- 当前 Plan 已 rebase 到 `origin/main@39136cd`，远端 Plan42 场景/角色表现为基线，Plan45 的 UI 资产、设置交互和音频语义继续保留；rebase 前快照保存在 `backup/plan45-before-origin-rebase-20260818`。
- 新基线上的 Development `-FullRebuild` 66/66 actions 已成功；最终功能回归和预构建包刷新结果记录在后续验证提交中。

### 人工验收结果/请求

- `PendingBeforeClose`：用户对 Start Menu 首批进行了初步测试并报告“初步没问题”；本轮进一步确认通关后商店的回响存储底部托盘“没问题”，批准作为 Plan45 阶段性检查点发布到 `origin/main`。Settings、Pause/Restart、Trait Choice、Inventory/Shop 其余状态、Stats、Player/Encounter HUD 以及完整多分辨率/DPI 验收仍待完成。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅；本批次只新增源图、Texture2D 和 WBP 表现层，不改变模块/权威状态/依赖拓扑，无需修改。
- `shared/CODEBASE_MAP/README.md`：已审阅；现有 UI 路由仍指向 `MOD-ReEchoUI`，索引拓扑不变，无需修改。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：已更新 UI 源图、运行时纹理目录、Plan45 阅读路线和七个剩余页面的 WBP 表现层边界。
- `docs/ART_ASSET_ORGANIZATION.md`、`Design/UI/ReEcho_UI修改指导.md`：已更新源图/参考图/字体隔离边界、分页运行时目录、可重复导入和全部已消费 WBP 的资产契约。
