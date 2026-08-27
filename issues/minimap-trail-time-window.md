# 小地图墨水轨迹应仅展示最近 5 秒路径

- 报告人/提交身份：`[DESIGNER] baopeijia29-del + CodeBuddy`
- 分支：`designer/stage-health-reset-bug`（基于 `origin/main`）
- 分支哈希：`a62a4ca2`
- 状态：Proposed（待程序/秘书审计）
- 类型：UI 表现调整（小地图系统）

## 现象
右上角小地图的墨水笔刷轨迹（回响轨迹）**展示了过长的历史移动路径**，包括很久之前的移动和过于靠后的路径段，造成视觉杂乱。

## 预期
小地图墨水笔刷轨迹应**仅展示回响（玩家）最近 5 秒内的行进路径**；5 秒前的路径应自动消隐/裁剪，不再显示。

## 相关资产
| 文件 | 用途 |
|---|---|
| `Content/ReEcho/Materials/UI/M_UI_MinimapInkTrail.uasset` | 墨水轨迹材质 |
| `Content/ReEcho/Textures/UI/CombatHud/MinimapInkBrush/T_UI_MinimapInkBrushTip.uasset` | 笔刷尖端纹理 |
| `Content/ReEcho/Textures/UI/CombatHud/MinimapInkBrush/T_UI_MinimapInkGrain.uasset` | 墨水颗粒纹理 |
| `Content/ReEcho/UI/WBP/WBP_ReEchoPlayerHud.uasset` | PlayerHud Widget Blueprint（使用该材质的 UI） |

## 复现
1. 进入战斗关卡。
2. 移动角色并观察右上角小地图墨水轨迹。
3. 实际：轨迹持续累积，长时间移动后地图上布满旧路径。

## 影响
小地图信息密度过高，旧路径干扰当前位置判断，影响玩家导航体验。

## 建议修复方向（需确认）
轨迹的时间窗口控制可能在以下位置之一：
- **材质参数层**：`M_UI_MinimapInkTrail` 材质中是否有 Time / Fade / Lifetime 参数，限制采样窗口；
- **蓝图逻辑层**：`WBP_ReEchoPlayerHud` 中维护轨迹点列表的逻辑，是否按时间裁剪旧点；
- **C++ 数据层**：若有专门的小地图轨迹数据结构（如 `TArray<FVector>` + 时间戳），需增加过期点清理逻辑。

建议程序优先检查蓝图侧的轨迹点管理逻辑，确认是否存在时间窗口限制；若无，则添加"仅保留最近 5 秒轨迹点"的清理机制。

## 提交约束
本报告仅作 bug 登记（只读审计 + 记录），不含代码 / 资产修改。修复须另立 Plan。
