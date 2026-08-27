# ReEcho 美术资产整理约定

## 目录职责

- `Content/SourceArt/`：保存 PNG 等源图和参考图，不放可运行的 `.uasset`。
- `Content/ReEcho/Textures/`：保存 Unreal 导入后的运行时纹理资产。
- `Content/SourceArt/Pickups/`：保存世界拾取物的审核源图；运行时纹理导入到 `Content/ReEcho/Textures/Pickups/`。
- 时间碎片的可编辑运行时 Prefab 为 `Content/ReEcho/Gameplay/Pickups/BP_TimeShardPickup`，透明主材质/实例为 `Content/ReEcho/Materials/Pickups/M_TimeShardPickup` 与 `MI_TimeShardPickup`。美术在 Blueprint Class Defaults 调整图标高度、地面排序、阴影开关，以及 `Animation|Landing` 下的弹跳高度/时长/次数和 `Animation|Collection` 下的上升高度/时长；在继承组件树调整 `VisualRoot/GroundRoot/GroundShadow` 的 Transform、缩放和阴影材质。运行时只移动图标根，地面阴影不参与弹跳并在拾取时隐藏；不得回到 C++ 修改视觉常量。
- `Content/ReEcho/Art/Animation2D/`：保存运行时 Texture2D、PaperSprite、Flipbook 与动画表现资产。
- Plan45 UI 交付源包：`Content/SourceArt/UI/InteractionPlaceholder/`，其中 `Elements` 是候选切图、`References` 是效果参考、`Fonts/PendingLicense` 是未获授权的隔离字体。
- Plan45 已消费的运行时 UI 纹理：`Content/ReEcho/Textures/UI/InteractionPlaceholder/`；只导入现有 WBP 实际引用的切图。目前按 `StartMenu`、`Settings`、`PauseAndCombat`、`ResultsAndRestart`、`TraitChoice`、`InventoryShop` 分页，未被 WBP 消费的按钮、参考图和字体仍只保留在 `SourceArt`。
- `scripts/ue/import_ui_interaction_placeholders.py` 默认跳过已存在的运行时纹理，避免重复执行改写既有资产；只有明确传入 `-Plan45ReimportExisting` 才重导同名纹理。
- Plan93 战斗 HUD 交付源包：`Content/SourceArt/UI/CombatHud/Plan93/`；`References/1-战斗场景.png` 只用于 1920×1080 构图对照。9 张 `Elements` 原图继续归档，其中废案 `技能栏.png` 从未导入；Plan102 又废弃倒计时黑底 `时间显示.png` 及其运行时纹理，当前其余 7 张按 `_SourceManifest.csv` 的稳定 ASCII 名保留在 `Content/ReEcho/Textures/UI/CombatHud/`。
- `scripts/ue/import_plan93_combat_hud.py` 默认保留已存在纹理；仅显式传入 `-Plan93ReimportExisting` 时重导。参考图始终不进入运行时资产。
- Plan102 小地图头像源包：`Content/SourceArt/UI/CombatHud/Plan102/`。`_SourceManifest.csv` 把 8 张 `512×512` 透明 PNG 映射到四个稳定 `CharacterId` 的 Player/Echo 两套外观；运行时 Texture2D 位于 `Content/ReEcho/Textures/UI/CombatHud/Minimap/`，由对应 Character/Echo Presentation Profile 的 `MinimapIcon` 硬引用。`scripts/ue/import_plan102_minimap_icons.py` 负责幂等导入与绑定，运行时代码不得再按原始编号猜测身份。
- Plan110 正式商店源包：`Content/SourceArt/UI/InventoryShop/Plan110/`。`Elements/` 使用 ASCII 稳定名保存 24 张已审核切图，`References/` 的三张整屏目标只用于 1920×1080 构图、悬停说明和属性面板比对；运行时 Texture2D 统一位于 `Content/ReEcho/Textures/UI/InventoryShop/Plan110/`，整屏参考图不得导入。`scripts/ue/import_plan110_formal_shop_ui.py` 负责切图导入，`author_plan110_formal_shop_ui.py` 只建立 WBP 初始设计面，人工微调后使用 `audit_plan110_formal_shop_ui.py` 做只读验证。
- Plan110 新增二级卡牌图标：19 张 `512×512 RGBA` 源图按不与现有存档身份冲突的 Game Card ID `G_2_18`～`G_2_36` 归档到 `Content/SourceArt/UI/Cards/Icon/`，策划源 ID `G_2_14`～`G_2_32` 的对应关系记录在 `_Plan110NewTier2IconMap.csv`。运行时纹理位于 `Content/ReEcho/Textures/UI/Cards/Icon/`；卡牌数据尚未建立对应 Game Card ID 时这些纹理保持未消费，不允许按策划源 ID 覆盖现有 `G_2_14`～`G_2_17`。

## 角色资产

- 通用静态回退：`Content/ReEcho/Textures/Characters/`。
- MushroomGirl 原始帧：`Content/SourceArt/Characters/MushroomGirl/`。
- MushroomGirl 运行时纹理：`Content/ReEcho/Textures/Characters/MushroomGirl/`。
- Plan40 序列动画：按角色、敌人和状态放在 `Content/ReEcho/Art/Animation2D/`，不把运行时资产导入 `SourceArt`。
- 生产角色只保留 `J_SPADE/J_DIAMOND/J_CLOVER/J_HEART`；`ReEchoPresentation` 的 Catalog/Profile 是身份到表现资源的唯一绑定入口，运行时代码不得重新硬加载 CAT 或具体 Boss 贴图。
- 敌人统一以 `PresentationId` 绑定资源：`DA_PresentationCatalog` 只绑定 `DA_Enemy_*` Profile，主模块 `DA_EnemyGameplayClassRegistry` 单独绑定 `BP_EnemyGameplay_*`。Boss/TimeGuard 与普通怪使用相同结构，缺少专属动画时在 Profile 内显式复用已批准资源。

## 导入规则

1. 源 PNG 保留在 `SourceArt`，2D 动画运行时导入目标必须位于 `ReEcho/Art/Animation2D`。
2. `.uasset` 的移动、重命名和删除通过 Unreal Editor 执行，以维护引用和重定向器。
3. 重新导入后先检查引用、像素密度、过滤方式、透明通道、压缩和 Paper2D 碰撞设置，再提交二进制变更。
4. `Player2D`、`SoftGroundShadow` 等 C++ 硬引用资产不得仅凭目录观感清理。
5. 未跟踪且误生成在 `SourceArt` 的 `.uasset` 先隔离，确认无引用后再永久删除。
6. UI 交付效果图不直接作为正常运行时整屏纹理；未附来源和授权证明的字体不得导入、在 WBP 中引用或提交分发，待授权 TTF 只保留在本地隔离目录。

## 当前审计结论（2026-08-18）

- `Player2D` 与 `SoftGroundShadow` 仍由运行时代码直接加载，必须保留。
- `Echo2D`、`Grunt2D` 仍属于旧静态回退/Cook 兼容资产，暂不删除。
- `Boss2D`、`DA_Character_J_CAT`、CAT 生成动画、Player/Echo CAT 贴图及其源 PNG 是 Plan50 的定向清理对象；必须先用 Unreal Asset Registry 确认无引用，再通过 Editor 删除并修复 Redirector。
- MushroomGirl 旧单帧资源已按当前工作区清理意图移除。
- `/Game/SourceArt/Characters/PlayerEchoReference` 与 `/Game/SourceArt/Characters/MushroomGirl/MushroomGirl_SpriteSheet` 是源 PNG 的重复 Texture2D 导入，无代码文本引用，应从 Content Browser 的运行时资产集合排除。

## Flipbook 引用审计

运行时依赖必须按以下顺序保持完整：

`Flipbook -> PaperSprite -> Texture2D -> Source PNG`

迁移前七个有效 Flipbook 使用的旧 Sprite 路径如下：

| Flipbook | 当前 Sprite 根目录 |
|---|---|
| `walk` | `/Game/2DAnim/Player` |
| `attack` | `/Game/2DAnim/Attack_row` |
| `Grount` | `/Game/2DAnim/small_Enemy` |
| `Rabbit` | `/Game/SourceArt/Characters/Rabbit` |
| `Goat` | `/Game/SourceArt/Characters/Goat` |
| `Fox_Walk` | `/Game/SourceArt/Characters/Fox`，首尾帧还跨用了 `Fox_Attack/01_Sprite` |
| `Fox_Attack` | `/Game/SourceArt/Characters/Fox_Attack` |

整理后的权威运行时资源位于 `/Game/ReEcho/Art/Animation2D/**`。Texture、Sprite、
Flipbook 三层引用已经闭合，现有 Presentation Profile 也已切换到新 Flipbook。旧目录资产
已完成自动化验证；旧目录资产已清理并暂存到可恢复隔离区，Cook 与 PIE 仍需后续验证。

Fox Walk 原先错误地把第 0 帧指向 Fox Attack 的 `01_Sprite`。整理时已改回
`/Game/ReEcho/Art/Animation2D/Enemies/Fox/Walk/Sprites/01_Sprite`。

推荐迁移批次：

1. 在 `/Game/ReEcho/Art/Animation2D` 恢复或建立 Texture2D。
2. 将对应 PaperSprite 放在同角色、同状态的 `Sprites` 子目录，并确认 Source Texture。
3. 最后修改 Flipbook Key Frames，使每帧引用新的 PaperSprite。
4. 保存 Flipbook 和引用它的 Profile，重新打开工程验证。
5. 通过自动化、Cook 和 PIE 后再 Fix Up Redirectors、清理旧目录。
