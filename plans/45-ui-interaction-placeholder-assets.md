# Plan 45 - UI - 交互占位美术资产接入

## 协调

- Planner 负责人：Codex。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`InProgress`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`（`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`）。
- 本地规划 / 实现基线：`origin/main@7a89328ebb9afb433199d56b975763e1775c7cef`。
- 本地实现方式：在当前 `main` 上小批量接入，每个 WBP 串行修改。
- 依赖 / 阻塞：沿用 Plan29 UMG/C++ 边界和 Plan34 Settings 绑定契约；占位字体缺少授权/来源证明，本 Plan 不导入运行时字体。
- Writes: `Content/SourceArt/UI/InteractionPlaceholder/**`；`Content/ReEcho/Textures/UI/InteractionPlaceholder/**`；与既有页面对应的 `Content/ReEcho/UI/WBP_ReEcho*.uasset`；必要的可复现导入脚本 `scripts/ue/**`；`docs/ART_ASSET_ORGANIZATION.md`；`Design/UI/ReEcho_UI修改指导.md`；`shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`；本 Plan。
- Stable Reads: `Source/ReEcho/{Public,Private}/UI/**`；`Source/ReEcho/{Public,Private}/ReEchoGameMode.*`；现有 WBP 绑定和页面流程；交付包 `正式-交互占位.zip`。
- 影响模式：`Exclusive`，源图和新纹理目录为独立新增，每个修改的 WBP 是不可文本合并的二进制资产。
- 兼容承诺 / 下游操作：保留全部原生父类、`BindWidget`/`BindWidgetOptional` 名称和类型、稳定屏幕 ID、页面 Delegate、输入/焦点/暂停和存档语义；只替换或补全表现。
- 明确排除：不新增“存档回溯”或“关于我们”页面/流程；不实现交付效果图中尚无产品契约的按钮功能；不导入授权未确认的 TTF；不修改 C++ 玩法、Schema、平衡、存档或数据表；不把 1920×1080 效果图当作正常运行时整屏贴图。

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
- 资产组织、UI 修改指导和 `MOD-ReEchoUI` 路由已同步；未新增 Runtime Module 或依赖拓扑。

### 证据

- Plan 编号前 fetch：`main == origin/main == 7a89328ebb9afb433199d56b975763e1775c7cef`，工作区干净。
- Plan-only 发布：`origin/main@131b084ac008bc35ecc8a6e6219c9bde065a6343`；`-FullRebuild`、`python scripts/validate_project.py` 和 `git diff --check` 通过。
- 交付包静态目录：98 张 PNG、2 个 TTF、1 个字体说明 TXT；已识别 1920×1080 效果图与独立切图。
- `_SourceManifest.csv` 与归档目录逐文件复核：101/101 存在，字节数和 SHA-256 全部匹配，0 errors。
- Unreal Editor 导入日志记录 3 张审核后纹理；WBP 资产依赖明确包含 `T_UI_Start_Background`、`T_UI_Start_TitleLogo`、`T_UI_Start_SettingsIcon`。
- UMG ToolSet 对 `WBP_ReEchoStartMenu` 的 Compile/Save 成功，保存后资产 `is_dirty=false`；既有 `StatusText`、`ContinueButton`、`NewGameButton`、`GameSettingsButton` 名称和类型保留。
- `CompileAllBlueprints` 退出码 0；`WBP_ReEchoStartMenu` 编译成功，汇总为 0 errors、0 warnings、0 blueprints failed to load。
- 最终 `scripts/ue/Build-Editor.cmd -Configuration Development -FullRebuild` 成功，刷新精选 5-module Editor bundle（source fingerprint `36ade36d8b29`）；构建后 `python scripts/validate_project.py` 与 `git diff --check` 通过。

### 剩余风险

- TTF 仅有占位用途说明，没有授权/来源证明；保留在本地 `PendingLicense` 并由 `.gitignore` 排除，不导入运行时、不提交分发。
- 交付效果图展示了尚无现有产品契约的页面/操作，本 Plan 不根据图片自行创建。
- Settings、Pause/Restart、Trait Choice、Inventory/Shop 和 HUD 尚未进入逐页接入批次；继续执行时仍需按现有 WBP 契约逐页审计。
- Start Menu 的主观视觉、按钮点击/键盘焦点和多分辨率 DPI 表现仍需用户 PIE 验收。

### 人工验收结果/请求

- `PendingBeforeClose`：导入和 WBP 批次完成后请用户在 PIE 执行具名视觉/交互/DPI 检查。

### 架构文档审阅结果

- `shared/CODEBASE_MAP/ARCHITECTURE.md`：已审阅；本批次只新增源图、Texture2D 和 WBP 表现层，不改变模块/权威状态/依赖拓扑，无需修改。
- `shared/CODEBASE_MAP/README.md`：已审阅；现有 UI 路由仍指向 `MOD-ReEchoUI`，索引拓扑不变，无需修改。
- `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`：已更新 UI 源图、运行时纹理目录和 Plan45 阅读路线。
- `docs/ART_ASSET_ORGANIZATION.md`、`Design/UI/ReEcho_UI修改指导.md`：已更新源图/参考图/字体隔离边界和 Start Menu 首批资产契约。
