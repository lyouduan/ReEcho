# 商店浮窗调整指南（Plan157）

## 三个入口

内容浏览器打开 `/Game/ReEcho/UI/`：

| 蓝图 | 调整范围 |
|---|---|
| `WBP_ReEchoShopTooltip` | 商店角色被动、武器、配件、卡牌、卡组的名称/说明，以及已获得卡牌下方的“实际效果”附加面板 |
| `WBP_ReEchoAttributeTooltip` | 悬停商店角色/时钟区域时显示的完整属性浮窗：总宽、边框、底色、内边距、属性行间距 |
| `WBP_ReEchoAttributeRow` | 所有属性行共用的字体、字号、名称/数值对齐、图标尺寸与间距 |

开场选角色/武器仍用 `WBP_ReEchoLoadoutTooltip`，和商店独立，不受以上蓝图修改影响。商店大蓝图不需要重建，也不用在事件图里设置字体。

三块完整浮窗（主说明、实际效果、属性）都保留背板：外框使用开场选择浮窗同族的 `T_UI_Loadout_DescriptionPanel`，以 Box 九宫格缩放；内部为白色纹理着深色的实心底板。不要把 Border 的 Background 资源清空；只有 Brush Color 并不能保证空 Image Brush 正常绘制。调整背板不需要修改已调好的文字或尺寸。

外框必须给内部实心底板留边：`TooltipFrame`、`OutcomeFrame`、`AttributeTooltipFrame` 的 **Content → Padding** 当前四边均为 3。增大可以加宽留边，不要清零，否则内层底板会盖住边框。子控件的「插槽（边框插槽）→ Padding」与父 Border 的 Content Padding 是同一布局值，不是两层独立间距。正文离底板的距离则在对应 `*Surface` 的 Content Padding 调整；它和外框留边不同。此次修复没有重设已保存的字体、字号、宽度或高度覆盖值，自动高度会随留边自然增长。

## 物品说明与“实际效果”

打开 `WBP_ReEchoShopTooltip` 的 Designer，默认按内容期望大小显示完整两块浮窗，里面的文字是预览示例。

| 控件 | 可调整内容 |
|---|---|
| `TooltipRootSizeBox` | 整个浮窗宽度：Width Override；高度保持自动，不启用固定 Height Override |
| `TooltipFrame` / `TooltipSurface` | 主说明的边框色与边框厚度、底色、正文内边距 |
| `TitleText` | 名称字体、字号、描边、颜色、对齐、与正文间距 |
| `DescriptionText` | 说明正文字体、字号、描边、颜色、行距和自动换行 |
| `OutcomePanel` | 完整附加面板容器；默认预览时可见。运行时有结果才显示，无结果折叠并消除空白 |
| `OutcomeFrame` / `OutcomeSurface` | “实际效果”金边、底色、内边距；`OutcomeFrame` 的 Slot Padding.Top 控制两块面板之间的间距 |
| `OutcomeTitleText` | “实际效果”标题的文案与样式；运行时不会改写这个标题 |
| `OutcomeText` | 已生效情况的正文字体、字号、描边、颜色、换行；默认示例含多项结果 |

选中文本，在「详情 → 外观 → 字体」修改 Font、Size 和 Outline；「颜色和不透明度」调整文字颜色。正文使用自动换行，推荐保持 Wrap Text At 为 0，让排版跟随实际可用宽度。增大字号后也要检查总宽和长文字高度。

运行时只填入真实名称、说明和 `OutcomeText`。普通物品没有结果时不会显示示例中的“物理攻击力 +2”等内容；随机、累计和多项卡牌的结果全部来自原 Run 投影，浮窗不计算效果。预览样例不是玩法数值，不要拿它修改卡牌规则。

## 角色属性

在 `WBP_ReEchoAttributeTooltip` 中展开 `AttributeRows`，能看到完整的 8 个 `WBP_ReEchoAttributeRow` 实例，而不是一块空白占位。

- `AttributeTooltipRootSizeBox`：总宽；默认 320，高度自动。
- `AttributeTooltipFrame` / `AttributeTooltipSurface`：正式边框、近黑底和内边距。
- `AttributeRow0..7` 的 Slot Padding：行间距。运行时复用这些实例，不重建或重置它们的布局。

统一修改每行样式，请打开 `WBP_ReEchoAttributeRow`：

- `AttributeNameText`：属性名称的字体、字号、描边、颜色。
- `AttributeValueText`：数值的字体、字号、描边、颜色；默认右对齐。
- `AttributeIconSize`：图标显示宽高；`AttributeIconScale` 保持 Scale To Fit，避免拉伸。
- `AttributeIcon → 外观 → Brush → Image Size`：图片自身的期望尺寸，当前为 28×28，两个值必须大于 0。它不是外层占位大小；如果清成 0×0，即使 `AttributeIconSize` 有宽高、贴图也绑定正确，居中的图片仍不可见。运行时只替换图标资源，保留你设置的图片尺寸。
- `AttributeRowContent`：图标、名称、数值的横向排列；子控件 Slot Padding 控制间距。
- `AttributeRowSizeBox`：单行最小期望宽度；默认 286，与属性面板扣除边框/内边距后的宽度对应。若将整个面板改得更窄，可一起调小该最小宽度。

单独编辑属性行时有“生命值 20”的示例。完整属性面板中每个行实例的 `Designer Preview` 可改示例名称、数值与图标；这些字段只供 Designer 预览，游戏中按 CSV 目录顺序、实际属性和整数/百分比格式覆盖。若以后数据目录增加行，新增行仍使用同一个属性行蓝图，并沿用首行 Slot 间距。

## 验证微调是否生效

1. 编译并保存修改的蓝图。属性行改完后重新打开/编译属性浮窗以刷新嵌套预览。
2. 在同一个工程中打开商店：检查待购配件、已装备武器/配件、已获得卡牌。
3. 至少检查一张有“实际效果”的卡牌和一张没有结果的卡牌；前者显示两块，后者没有空白附加框。
4. 悬停角色/时钟区域，确认完整属性列表的名字、真实数值和百分比。
5. 检查长说明、多行实际效果，以及屏幕边缘的浮窗避让。

不要重命名绑定控件，不要添加与 C++ 重复的 PreConstruct/Construct 字体设置。字体和布局以 WBP 保存值为准；不同数据造成的自动高度变化是正常的。`author_plan157_shop_tooltips.py` 只创建缺失资产，不会重置已有蓝图的人工微调。

## 开场确认/返回按钮（本轮追加）

在 `WBP_ReEchoLoadoutSelection` 中调 `ConfirmButton` 和 `BackButton` 的 Button Style。两阶段按钮和独立文字始终可见：没有选中角色/武器时确认禁用，使用 Disabled Brush；返回保持可点击，Normal/Disabled 暗、Hovered/Pressed 亮。按钮明暗用各状态 Brush 的 Tint 调整，文字随启用/悬停状态灰显，原字体、字号和位置不被改写。角色页返回主界面、武器页返回角色页的流程不变。
