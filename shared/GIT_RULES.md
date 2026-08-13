# ReEcho Git 规则

本文件是提交身份和发布完整性规则的唯一权威。角色和职责文件只路由至此，不重复定义标签或程序发布构建门禁。

## AI 提交身份

AI 创建的每个提交，其标题必须以且仅以一个标签开头；标签需匹配创建该提交所使用的专业路线或明确指派的仓库职责：

| 提交创建者 | 标题必需前缀 |
|---|---|
| 程序路线 | `[PROGRAMMER]` |
| 策划路线 | `[DESIGNER]` |
| 美术路线 | `[ARTIST]` |
| 明确指派的项目秘书职责 | `[SECRETARY]` |

- 任何角色不得使用其他角色的标签。项目秘书仅对 `SECRETARY_RULES.md` 范围内的协调/控制面提交使用其标签，不得用它标记专业实现。
- 程序 Planner 的集成或合并提交使用 `[PROGRAMMER]`，因为 Planner/Executor 是程序路线内的仓库职责，不是独立专业身份。
- 混合范围不是组合标签的理由；应先按专业边界拆分提交。
- 本规则仅适用于本文件生效后创建的提交；不得仅为添加标签而重写历史提交。

## 程序发布构建门禁

本门禁适用于普通程序路线。任何 Planner-Executor 模式选择都不能豁免；`shared/PROGRAMMER_RULES.md` 中不可跳过的远端协作安全检查在所有模式下均适用。

普通程序路线每次推送 `origin/main` 前，必须使用项目标准 UE 5.8 安装版/发行版完整构建最终集成候选：

```powershell
scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild
```

命令成功后会刷新精选 Win64 Editor 预构建包及其源码指纹。同一候选必须包含全部已批准源码/内容/配置变更，以及 `Binaries/Win64/ReEchoEditor.prebuilt.json` 声明的匹配跟踪文件。

- 发布门禁要求 `-FullRebuild`；实现期间可使用报告目标已是最新的增量构建，但它不能满足最终推送门禁。
- 禁止发布仅含源码的程序候选，也不得复用最终集成/rebase/冲突解决之前的构建证据。
- 构建失败、缺少二进制、二进制哈希不匹配、源码指纹过期、Engine Build ID 错误或预构建刷新后工作区不干净，都会阻止推送。
- 构建后运行 `python scripts/validate_project.py`，并在最终 fetch/push 门禁前提交刷新后的允许列表产物。
- 可发布的生成 Editor 产物仅限 manifest、`ReEchoEditor.target`、`UnrealEditor.modules` 及 manifest 指定的准确模块 DLL。其他 `.target`、`Intermediate`、PDB/导入库、Live Coding patch、生成的解决方案、缓存、日志和机器本地路径均保留在本地。
- 该包只保证使用 manifest 记录的准确 UE 5.8 安装版/发行版 Build ID 在 Win64 上拉取即开。其他引擎构建必须本地编译，或建立各自经过评审的交付契约。

策划、美术和秘书提交不重建或修改程序拥有的预构建包。其本地提交保留各自角色标签并由程序 Planner 集成；之后的程序发布构建覆盖最终组合候选。
