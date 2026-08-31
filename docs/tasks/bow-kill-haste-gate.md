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
