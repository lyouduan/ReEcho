# 击杀移速弓弦不可叠加修复

- 用户确认：击杀触发后 5 秒内不可再次触发，不叠层、不刷新剩余时间；到期后下一次击杀可触发。
- 基线：`f62541505fb875269e19b6b9ec84eec59671c72b`。
- 专属工作树：`ReEcho-fix-bow-kill-haste-gate`，分支 `fix/bow-kill-haste-gate`。
- 属于既有行为的边界明确小修复，不新增模块、Schema、公共 API 或存档契约。

## 范围与架构

- 修改 `MOD-ReEcho / AREA-Weapons` 的 `Part.MoveSpeedOnKill` 消费分支及对应测试。
- 复用 Combatant 的临时增益作为唯一门控；按 BehaviorId 归组，避免同一行为的不同档位或攻击上下文叠加。
- 不改加成数值、持续时间、其他叠层符文、策划 XLSX 或生成 CSV。
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
