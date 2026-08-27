# 关卡切换时主角缺少短暂无敌保护

- 报告人/提交身份：`[DESIGNER] baopeijia29-del + CodeBuddy`
- 分支：`issue/baopeijia29/stage-transition-invincibility`（基于 `origin/main`）
- 状态：Proposed（待程序/秘书审计）
- 类型：战斗机制 Bug（关卡流程）

## 现象
每次进入下一关（关卡切换/场景转换）时，主角**没有无敌保护时间**，导致在新关卡开始瞬间即被周围敌人围攻致死。

## 预期
每次进入下一关时，应给主角 **0.5 秒无敌时间**，让玩家有反应空间，避免被围攻秒杀。

## 根因（推测）
关卡切换逻辑位于 `Source/ReEcho/Private/ReEchoGameMode.cpp` 的 `BeginNextEncounter()` 附近。当前实现在新 Encounter 开始时可能：
- 未设置临时无敌状态（Invincibility Frame）；
- 或无敌时长为 0 / 未触发。

需程序确认 `BeginNextEncounter()` 或关卡过渡动画结束后是否有调用无敌接口。

## 复现
1. 开始运行，通关前一关。
2. 进入下一关瞬间观察主角状态。
3. 实际：主角立即受到伤害，容易被密集敌群围攻致死。

## 影响
关卡切换体验差，尤其在高难度关卡或多敌群场景中，玩家因无法规避开局伤害而产生挫败感。

## 建议修复方向（需确认）
在 `BeginPlay()` 或关卡过渡结束回调中，给主角添加约 **0.5 秒无敌帧（IFrame）**：
- 方案 A：在 `ReEchoGameMode::BeginNextEncounter()` 中调用 `Player->Combatant->SetInvincible(0.5f)`；
- 方案 B：在关卡过渡 UI 动画结束时触发无敌，确保无敌覆盖"画面切回游戏可操作"后的窗口期。

具体实现方式需程序评估现有 Combatant 组件是否已支持临时无敌接口。

## 提交约束
本报告仅作 bug 登记（只读审计 + 记录），不含代码修改。修复须另立 Plan。
