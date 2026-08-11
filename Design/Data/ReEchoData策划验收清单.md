# ReEcho 配表策划验收清单

本清单用于策划首次拉取项目后，验证 `Design/Data/ReEchoData.xlsx` 是否便于编辑、能否稳定生成 CSV，以及改值能否在游戏的新一局中生效。日常字段说明仍以 [`ReEchoData使用说明.md`](ReEchoData使用说明.md) 为准。

## 1. 三层验收与环境要求

| 层级 | 验收内容 | 必需环境 |
|---|---|---|
| A - 表格可用性 | 布局、可编辑区域、下拉、非法输入、增删 Table 行 | Excel 或支持 Excel Table/数据验证的 WPS |
| B - 生成链路 | XLSX 校验、生成 16 个 CSV、报错定位、生成结果检查 | 本地 Git 仓库、Python 3.10+ |
| C - 游戏效果 | 角色、卡牌、反应、武器或配件改值在新一局中生效 | A+B，加 Unreal Engine 5.8 和可用的 ReEcho Editor 构建 |

只拿到单独的 XLSX 可以完成 A，但不能证明 XLSX→CSV 和游戏读取链路可用。正式验收至少应完成 A+B；有 Unreal 环境时再完成 C。

## 2. 获取指定验收版本

不要默认测试远端 `main`。验收负责人必须同时提供：

- 仓库地址：`https://github.com/lyouduan/ReEcho.git`
- 验收远端分支，例如 `origin/plan/25-xlsx-authoring`
- 验收提交号；该提交必须至少包含 `c9e819e` 的严格下拉修订

建议使用一个全新的测试目录，避免覆盖策划或程序已有工作。首次拉取示例：

```powershell
git clone https://github.com/lyouduan/ReEcho.git ReEcho-Plan25-QA
cd ReEcho-Plan25-QA
git fetch origin
git switch --create qa/plan25-designer origin/plan/25-xlsx-authoring
git status --short --branch
git log -1 --oneline
```

开始验收前确认：

- 当前分支是专用本地 QA 分支，不是 `main`。
- `git status --short` 没有未知修改。
- 当前提交与验收负责人给出的提交一致或是其后继提交。
- 没有其他人同时编辑本分支中的 `ReEchoData.xlsx`。

如果分支或提交在远端不存在，停止验收并联系负责人；不要自行改用 `main` 或猜测另一个分支。

## 3. 准备 Python 与基线检查

在仓库根目录打开 PowerShell：

```powershell
python --version
python -m pip install -r scripts\data\requirements.txt
python scripts\data\sync_xlsx_to_csv.py --check
python scripts\validate_project.py
```

基线必须同时满足：

- Python 为 3.10 或更高版本。
- `--check` 输出 `[PASS] XLSX export package validates and production CSV bytes match.`。
- `validate_project.py` 最终通过。
- `git status --short` 仍然为空。

基线不通过时不要开始改表，把完整命令、退出码和错误文本交给负责人。

## 4. A 层：表格可用性验收

打开 `Design/Data/ReEchoData.xlsx`，由策划本人检查；AI 可以记录结果，但不要代替策划进行视觉判断。

至少检查以下项目：

1. 六个生产 Sheet 中，左侧参考区与右侧深色表头生产 Table 容易区分。
2. 生产 Table 数据行可以编辑；表头、参考区和系统 Sheet 不可随意修改。
3. `Enabled`、`RoleId`、`BehaviorId`、`FormulaId`、`AttackPatternId`、父表 ID 等受限字段能看到单元格下拉。
4. 在一个受限字段中键入 `INVALID_TEST_VALUE`，Excel/WPS 应立即以“停止”错误拒绝；随后取消输入，不要保存非法值。
5. 修改一个普通数值并保存成功；建议记录“Sheet、Table、稳定 ID、字段、原值、测试值”。
6. 在 Table 内新增一行和删除该测试行，确认新行仍属于同一个 Table，表头和参考区没有被移动或解锁。
7. 不修改 Sheet 名、Table 名、表头、系统 Sheet 或左侧 ReferenceOnly 内容。

如果 WPS 没有显示下拉或没有阻止非法值，需要记录 WPS 版本、字段位置和现象；这属于兼容性问题，不能直接判定通过。

## 5. B 层：XLSX→CSV 验收

保存并关闭 Excel/WPS，然后在仓库根目录运行：

```powershell
python scripts\data\sync_xlsx_to_csv.py
python scripts\data\sync_xlsx_to_csv.py --check
python scripts\validate_project.py
git status --short
git diff -- Content\Data
```

通过条件：

- 全量同步成功，不留下部分发布结果。
- `--check` 和 `validate_project.py` 均通过。
- `Content/Data` 只出现与本次测试字段对应的 CSV 变化。
- XLSX 与生成 CSV 同时发生变化；没有直接手改 CSV。
- 故意制造的重复 ID、未知外键或非法逻辑 ID 能给出 `Sheet:Table:row:column` 定位。负例测试结束后必须恢复正确值并重新通过全量同步。

## 6. C 层：游戏效果验收

### 6.1 新拉仓库的首次构建

源码仓库不提交本机生成的 Editor 二进制。第一次运行前需要 Unreal Engine 5.8 和 Windows C++ 编译环境，并在 Unreal Editor 关闭时执行：

```powershell
scripts\ue\Find-UnrealEngine.cmd
scripts\ue\Build-Editor.cmd
```

如果找不到 UE 5.8，或缺少 Visual Studio/MSVC/Windows SDK，不要让 AI 自动安装大型工具；记录错误并让程序提供环境或已构建的测试版本。A+B 仍可独立验收。

### 6.2 改值后的正确启动顺序

游戏不读取 XLSX，只读取 `Content/Data` 中生成的 CSV；CSV 又在 ReEcho 模块启动时加载一次。因此必须按以下顺序：

```text
编辑并保存 ReEchoData.xlsx
→ 关闭 Excel/WPS
→ 运行全量 sync、--check、validate_project.py
→ 关闭此前已经打开的 ReEcho Unreal Editor
→ 双击 ReEcho.uproject，等待 Level00 打开
→ 点击工具栏 Play
→ 在开始菜单选择“新游戏”，不要使用“继续游戏”
→ 选择与被修改稳定 ID 对应的角色/武器并验证
```

仅修改 XLSX 后直接打开游戏不会生效；必须先生成 CSV。若同步时 Unreal Editor 已经打开，仅停止 PIE 后再点 Play 也不足以重载 CSV，必须重启 Editor。配置改动本身不需要重新编译 C++，但首次拉取或程序代码变化仍需要构建 Editor。

不要使用旧的打包版 EXE 验证仓库内刚生成的 CSV；旧包内的数据不会自动更新。

### 6.3 游戏内通过条件

- 使用“新游戏”，不复用旧存档或“继续游戏”的数据快照。
- 所选角色/武器与测试记录中的稳定 ID 一致。
- 至少一个明显数值变化可以观察到，例如可选角色生命值或初始武器攻击间隔。
- 恢复原值并重新同步后，重启 Editor、新开一局，表现也恢复。

卡牌抽取、特定元素反应和配件可能受随机池或触发条件影响；如果难以稳定复现，B 层的 CSV 结果可以先验收，游戏内覆盖由程序另行提供确定性路线。

## 7. 测试数据与恢复

- 所有试验都在专用 QA 分支/目录进行，不向远端推送测试数值。
- 每次修改前记录原值；不要依赖记忆恢复。
- 测试结束后先保存验收记录，再让 AI 列出将要恢复的文件并等待策划确认。
- 只恢复本次测试修改的 `Design/Data/ReEchoData.xlsx` 和对应生成 CSV；不要清理或覆盖不属于本次测试的改动。
- 恢复后必须再次运行 `--check`、`validate_project.py` 和 `git status --short`。

## 8. 验收反馈模板

```text
验收分支/提交：
Excel 或 WPS 版本：
Python 版本：
UE 版本（未测 C 层则写“未测”）：

A - 表格可用性：通过 / 不通过
- 下拉与非法输入：
- 可编辑区/锁定区：
- 增删 Table 行：
- 视觉或兼容性问题：

B - XLSX→CSV：通过 / 不通过
- 修改的 Sheet/Table/ID/字段/原值/测试值：
- sync、--check、validate_project 结果：
- 生成 CSV 是否仅包含预期变化：
- 报错定位是否易理解：

C - 游戏效果：通过 / 不通过 / 未测
- 是否重启 Editor并使用新游戏：
- 实际观察结果：

遗留问题及复现步骤：
相关截图/完整错误文本：
```

## 9. 可交给策划 AI 的前置操作 Prompt

验收负责人应把实际验收提交号补进下面 Prompt 后，再交给策划。

```text
你是 ReEcho Plan25 的策划验收助手。目标是只在我的电脑上准备一个隔离的本地验收环境，并协助我执行仓库、Python 和可选 Unreal 前置检查；视觉判断和最终通过/不通过由我本人完成。

仓库：https://github.com/lyouduan/ReEcho.git
目标远端分支：origin/plan/25-xlsx-authoring
最低必须包含的提交：c9e819e
验收负责人指定提交：<在这里填入实际验收提交号>

请遵守：
1. 先检查我指定的目标目录。若已有仓库或未提交修改，不得覆盖、清理、reset、checkout 或删除；改用一个全新的 ReEcho-Plan25-QA 目录，必要时先问我目录位置。
2. clone/fetch 后，从目标远端分支创建本地 qa/plan25-designer 分支。不得改用 main，不得 merge，不得 push。
3. 用 git 证明当前 HEAD 等于验收负责人指定提交，或是包含该提交的明确后继；同时证明它包含最低提交 c9e819e。远端没有分支/提交时立即停止并反馈。
4. 阅读 Design/Data/ReEchoData使用说明.md 和 Design/Data/ReEchoData策划验收清单.md。
5. 检查 Git、Python 3.10+、Excel/WPS 是否可用。执行 python -m pip install -r scripts\data\requirements.txt，然后运行：
   python scripts\data\sync_xlsx_to_csv.py --check
   python scripts\validate_project.py
   git status --short --branch
6. 基线不通过就停止，不要修改 XLSX 或 CSV；把命令、退出码和完整错误告诉我。
7. 可选检查 Unreal：运行 scripts\ue\Find-UnrealEngine.cmd。若找到 UE 5.8，再检查能否运行 scripts\ue\Build-Editor.cmd；构建前确认 ReEcho Editor 已关闭。缺少 UE、Visual Studio、MSVC 或 Windows SDK 时只报告，不要自动安装大型软件。
8. 不要打开或修改 ReEchoData.xlsx，不要替我做视觉验收，不要生成测试数值，不要提交或推送任何内容。准备结束后只向我报告：仓库绝对路径、当前分支和提交、基线命令结果、Excel/WPS/Python/UE 可用性，以及我下一步应该打开的文件绝对路径。

当我完成表格修改并明确让你继续后，你再按验收清单帮助我关闭表格、运行全量 sync、--check、validate_project.py、展示精确 CSV diff，并提醒我重启 Unreal Editor、点击 Play、选择“新游戏”。恢复测试数据前必须先列出精确文件并等待我确认。
```
