# 击杀移速弓弦不可叠加修复

- 用户确认：击杀触发后 5 秒内不可再次触发，不叠层、不刷新剩余时间；到期后下一次击杀可触发。
- 基线：`f62541505fb875269e19b6b9ec84eec59671c72b`。
- 专属工作树：`ReEcho-fix-bow-kill-haste-gate`，分支 `fix/bow-kill-haste-gate`。
- 属于既有行为的边界明确小修复，不新增模块、Schema、公共 API 或存档契约。

## 范围与架构

- 修改 `MOD-ReEcho / AREA-Weapons` 的 `Part.MoveSpeedOnKill` 消费分支及对应测试。
- 复用 Combatant 的临时增益作为唯一门控；按 BehaviorId 归组，避免同一行为的不同档位或攻击上下文叠加。
- 不改加成数值、持续时间、其他叠层符文、策划 XLSX 或生成 CSV。
- 用户在知晓校验登记阻塞后要求推送并合并主线；补齐 `scripts/validate_project.py` 对已在 C++ 注册的 `Card.EasterShardThreshold` 的静态登记，不改卡牌玩法或配表，不跳过检查。
- `MOD-ReEcho.md`：更新该行为的非重入契约。
- `MOD-ReEchoCombat.md`、`MOD-ReEchoWeapons.md`：已审阅，无需修改；现有临时层/到期接口和武器逻辑模块不变。
- `ARCHITECTURE.md`、`README.md`：拓扑、依赖和检索路由不变，无需修改。

## 验证

- 更新旧“击杀移速可叠到两层”断言。
- 聚焦回归 `ReEcho.Weapons.Runes.KillHasteNonRefreshingWindow`：真实生产 I/II/III 数据，GAS 与非 GAS，非击杀、同帧多击杀、4.9 秒新击杀、不刷新原 5 秒到期、到期后再次触发及第二次到期。
- 已通过：`python scripts/setup_lfs.py --check`、两份修改 C++ 的 clang-format、`Build-Editor.cmd -Configuration Development`（98 个构建动作）、`python scripts/ue/prebuilt_editor.py check`（7 个模块，Build ID 55116800）和 `git diff --check`。
- 构建仅有既有 `ReEchoGameMode.cpp` 的 `FImageUtils::CompressImageArray` 弃用警告，无编译错误。
- 聚焦回归已通过（2026-08-31 21:31）：用户确认 Editor 已关闭后，重新检查进程与 LFS，原子取得 Git-common-dir Unreal 锁，再执行 `Run-Automation.cmd -Filter ReEcho.Weapons.Runes.KillHasteNonRefreshingWindow`；命令退出码 0，日志明确记录该测试 `Result={Success}`。覆盖 I/II/III × GAS/非 GAS 六组场景；结束后释放本任务取得的 Unreal 锁。
- 证据：本工作树 `Saved/Logs/ReEcho.log` 第 2185 行记录测试成功，第 2194 行记录 `TEST COMPLETE. EXIT CODE: 0`。测试事件中仅有 GameplayCue 路径与敌人 Animation2D fallback 警告。启动阶段另有 `Condition failed` 日志，发生在专项测试开始前，不属于该测试结果；未将整份启动日志描述为零错误。
- `python scripts/validate_project.py` 未通过：`Content/Data/card_effects.csv:78` 的 `Card.EasterShardThreshold` 不在校验器的注册清单。运行时 `ReEchoCardCatalog.cpp` / `ReEchoCsvDataRegistry.cpp` 已注册，相关文件本任务没有修改。基线历史 `e3e63cbc` 已记录同一缺项；其中对 Plan159 的一次性暂缓授权不适用于本任务，未复用或绕过。
- 再次 fetch 确认 `origin/main` 仍为上述基线；未集成其他候选、未获取 main 发布锁、未推送。
- 当前状态：本地实现、编译和聚焦回归完成；全项目静态校验仍因既有卡牌登记缺项失败，尚未满足完整交付/发布门禁。未执行最终发布 FullRebuild，独立工作树及匹配预构建包保留；不占用远端发布锁等待范围决定。

## 主线发布复核（2026-08-31）

- 用户要求推送远端并合并主线。Fetch 得到 `origin/main@8cde5261`，比初始基线新增 `cdd3da4f`、`30198fa4`、`8cde5261`，作者均为 `JosephLE910 + Codex`；范围为商店/玩家 HUD 两个已保存布局蓝图、对应任务记录和精选预构建包。
- 物理审计：`git merge-tree --write-tree --name-only HEAD origin/main` 仅报告 7 个 DLL 与预构建 manifest 冲突；源码、校验器、任务文档无文本冲突。获锁后才能实际 merge，生成物冲突将通过最终 FullRebuild 重建处理。两个远端蓝图将原样保留。
- 逻辑与耦合：UI 布局不改变击杀移速契约；未新增本任务编号 Plan，无编号冲突。基线集成后重跑构建和聚焦回归，不沿用其他任务的静态例外。
- 已补 `Card.EasterShardThreshold` 静态登记。重跑 `validate_project.py` 后显露下一项主线旧缺项：`card_effects.csv:82` 的 `CritNegateAmplification` 未列入 `CARD_TARGETS`；该目标已经存在于 CSV Schema、C++ Reader 和 Card Runtime。
- 独立只读 `sync_xlsx_to_csv.py --check` 另确认 `cards.csv`、`card_effects.csv` 与 XLSX 漂移。没有重新导表、修改工作簿或运行时数据，也没有改校验器放行漂移。
- 已向用户说明后续导表可能覆盖现行卡牌效果，并询问是否针对上述准确主线保留配表原状、补登记、仅暂缓 XLSX/CSV 一致性检查；尚未得到此具名确认。当前未取得 main 发布锁、未实际 merge、未执行最终发布构建、未推送。

## CSV 回写工作簿决定与实现

- 用户明确要求“应根据csv改xlsx”，因此不使用静态例外：以现行 `cards.csv`、`card_effects.csv` 回写 `Design/Data/ReEchoData.xlsx` 的 `构筑体系G / tblCards、tblCardEffects`，CSV 本身保持逐字节不变。
- 读取前显式排除隐藏行和隐藏列；本次两个表范围内没有隐藏内容。卡牌表仍为 74 行；效果表从 101 行扩为 103 行，包含 CSV 中的 3 个替换 ID 和 2 个新增 ID，表范围从 `Q3:AA104` 扩展到 `Q3:AA106`，对应下拉验证同步扩展。
- 工作簿中原有表外 8 行不属于生产导出，原样下移两行保留，没有静默删除。仅改变工作簿包内 `xl/worksheets/sheet3.xml` 和 `xl/tables/table4.xml`，其他 ZIP 成员逐字节保留，包括其他 Sheet、样式、保护、定义名称、图片及工作簿关系。
- 表格工具完成内容写入、导出和前后预览；因工具导出不保留原保护，最终只将经过值校验的工具生成单元格嵌回原 OOXML 包，保留原样式/保护。没有使用第二个工作簿写入库。同步保留 CSV 文本单元格内的 CRLF，避免仅换行字节导致伪漂移。
- 补齐 `CritNegateAmplification` 静态目标登记；它已存在于 C++ 和 CSV Schema，不新增玩法。增加聚焦测试，覆盖生产包和未知 Behavior/Target 仍拒绝，未放宽校验器。
- `sync_xlsx_to_csv.py --check` 对回写候选及其余三份原工作簿已通过：导出的全套生产 CSV 字节一致。原静态漂移阻塞已消除；本次不请求或采用任何跳过验证例外。
- 全量静态检查继续发现主模块文档把 UI 文档入口写成 Runtime 稳定 ID；依据索引已有“文档型入口，不注册新稳定标识”的契约，仅将该引用改为 `MOD-ReEchoUI.md` 的 Markdown 链接，不修改 UI 职责、模块列表或校验规则。
- 同步器 18 项回归中原有单项测试用已失效的武器平衡元组 `0.7,0.6,250` 制造旧数据；改用当前稳定攻击步骤 ID 制造同样的测试漂移，保留全部整组发布、无关文件字节和时间戳断言，不改生成器或生产武器数值。

## 本地验证完成，按用户要求暂缓发布

- `test_validate_card_registrations.py`：4/4 通过；`test_sync_xlsx_to_csv.py`：18/18 通过；`validate_project.py`、`sync_xlsx_to_csv.py --check`、`git diff --check` 通过，`Content/Data` 相对已验证移速候选没有字节变化。
- 回写文件重新导入并完成关键区域视觉/公式错误扫描；其他表及保护/样式保留，原有窄列/固定行高布局未全表重排。移速 C++、运行时 CSV 与此前专项回归的候选一致，已有 Editor 编译和移速专项结果仍有效；尚未执行最终合并候选的 FullRebuild。
- 用户最新要求：“你稍后发布，因为你改了表”。已暂停所有远端发布操作，仅保存本地候选；未获取 main 发布锁、未合并 `origin/main`、未推送。等用户确认恢复后，再 fetch 审计最新表格与外部变化、按锁流程 merge、FullRebuild 并重跑适用验证。
