# Boss 弧形血条调整指南

## 调整入口

打开 `/Game/ReEcho/UI/WBP_ReEchoEncounterHud` 的 Designer。

- 选择根 Widget 的 **类默认值 → Boss Health Preview**，勾选 `Preview Boss Encounter`，即可预览 Boss 钟面；取消则预览普通关。
- `Preview Boss Health Ratio` 是设计期血量：`1` 满血、`0.5` 半血、`0` 空血。它同时驱动弧形边界、指针与中央百分比，**不修改游戏里 Boss 的生命值**。
- 选择控件树中的 `BossHealthArc`，在 **Boss Health Arc** 分类调整颜色，在 Canvas Slot 调整位置和尺寸。Compile/Save 后这些作者值仍保留，运行时只更新比例和显隐。
- 选择 `BossHealthPercentText` 调整百分比字体、字号、描边、颜色和位置。它初次创建时复制原倒计时的作者样式和几何，之后独立编辑；运行时只填文字与显隐，不覆盖这些样式。普通关仍使用原 `CountdownText`。

## 常用参数

| 参数 | 用途 |
|---|---|
| `Fill Color` | 剩余血量颜色，默认醒目的紫红色；A 控制透明度 |
| `Empty Color` | 已损失血量的底色，默认近黑灰，避免误读为剩余血量 |
| `Tick Contrast` | 刻度强度，默认 `0.25`；`0` 为连续填充，`1` 为原黑色刻度强度 |
| `Vein Strength` | **Surface** 分类：血管纹理深浅，默认 `0.4`；`0` 关闭血管着色，`1` 最明显 |
| `Vein Width` | 血管粗细，默认 `1.3`；范围 `0.5–4`，单位为钟面参考像素，不改变血条本身宽度 |
| `Vein Spacing` | 分叉间距，默认 `42`；范围 `20–100`，越小越密，越大越疏 |
| `Relief Strength` | **整条血条**的圆润立体感，默认 `0.3`；`0` 为平面，越大截面高光和边缘暗部越明显，不是真实几何凸起 |
| `Inner Radius` / `Outer Radius` | 环的内外半径，差值决定线宽；以原钟面图像素为单位 |
| `Arc Center` | 弧心在钟面源图中的位置；默认 `(580,54)`，与现有指针轴心对齐 |
| `Reference Size` | 钟面源图参考尺寸 `(1159,216)`；普通移动/缩放时不要改它 |
| `Arc Material` | `/Game/ReEcho/Materials/UI/M_UI_BossHealthArc`，通常不需要更换 |

填充默认只保留弱化的手绘刻度，使血量的连续长度成为主体。颜色还会受到刻度强度、素材明暗和父级透明度影响。

### 血管与凸起

选 `BossHealthArc → Boss Health Arc → Surface`。先调 `Relief Strength` 看整条血条圆润隆起的截面、高光和边缘暗部，再调 `Vein Strength` 看平面纹理深浅，`Vein Width` / `Vein Spacing` 调粗细和疏密。**血管不参与凸起**：`Vein Strength=0` 时仍保留整条血条的立体感；血管粗细、间距不影响立体截面。固定左上方光照只改变原配色的明暗，不额外染成红色；血管沿弧形分布，不随血量变化滚动。

纹理只作用于剩余血量，空槽和透明度不受影响。**同时将 `Vein Strength`、`Relief Strength` 设为 `0`，即可还原无纹理的平面效果**；关闭其中一个仍会保留另一个。血条整体透明度仍在 `Fill Color → A` 调整。Designer 与运行时使用同一个材质，正常 Compile/Save 不会重置这些参数。

## 移动与缩放

`BossHealthArc` 与 `ArtClockFrame` 初始拥有相同的 Canvas 位置/尺寸/锚点，弧形只绘制钟面的下半环。调整整个钟面时，需同时调整这两个控件及 `ArtClockNeedle`，避免血条与刻度、指针脱离。

- `ArtClockFrame` 与 `BossHealthArc` 初始 ZOrder 都是 `5`，后加入的弧形在钟面上方。
- 指针 `ArtClockNeedle` 保持原 ZOrder `6`、Pivot `(0.5,0.12)`；不要把弧形放到指针之上。
- 指针的角度是运行时数据，位置、尺寸及 Pivot 是蓝图作者值。除非更换指针图，不建议修改 Pivot。
- `BossHealthPercentText` 初始沿用原倒计时的 ZOrder `9`，位于指针上方，避免指针盖住读数；编辑后也需保持读数可见。
- 不要更改 `BossHealthArc`、`BossHealthPercentText`、`ArtClockFrame`、`ArtClockNeedle`、`CountdownText` 的绑定名称。

## 游戏内行为

- 普通关：显示倒计时与原计时指针，不显示血条。
- Boss 关：隐藏倒计时文字，保留钟背板与指针，以当前阶段的 `CurrentHealth / MaximumHealth` 显示弧形及中央百分比。
- 百分比显示整数，如 `50%`。只在真正空血/满血时显示 `0%`/`100%`；尚有血量或已经受伤时限制在 `1%`–`99%`，不以显示四舍五入暗示死亡或满血。此规则不改真实生命值、弧长或指针角度。
- 满血指针朝左；扣血时经下半圆逆时针移动；半血朝下、空血朝右。剩余填充从右端延伸到指针，空血只留下暗色底环。
- Boss 阶段回血后恢复对应比例，不把各阶段生命加总；变身流程暂停 HUD 更新期间沿用已有冻结行为。
- 旧横向 `BossHealthPanel` / `BossHealthFill` / `BossHealthFrame` 已从正式 WBP 删除。

## 验证与注意事项

编译保存后，分别预览 `1 / 0.5 / 0`，再进普通关与 Boss 关检查。Designer 预览不会扣血，真实生命值仍来自战斗系统。

`scripts/ue/author_plan158_boss_health_arc.py` 是初次迁移工具；`upgrade_plan158_boss_health_readability.py` 是旧配色/独立百分比升级，**微调后不要重跑**。本次 `upgrade_plan158_boss_health_surface.py` 只增加材质表面参数，保护当前颜色、Alpha、字体、几何和预览，不重设旧配色。三者都不是日常刷新工具。只读核对可使用 `audit_plan158_boss_health_arc.py`；它优先对比本轮表面升级前快照，不要求人工调色符合旧的紫红默认值。主动微调后快照差异需人工审阅，不应为了让旧快照审计通过撤回人工调整。
