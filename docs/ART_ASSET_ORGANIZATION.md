# ReEcho 美术资产整理约定

## 目录职责

- `Content/SourceArt/`：保存 PNG 等源图和参考图，不放可运行的 `.uasset`。
- `Content/ReEcho/Textures/`：保存 Unreal 导入后的运行时纹理资产。
- `Content/ReEcho/Art/Animation2D/`：保存运行时 Texture2D、PaperSprite、Flipbook 与动画表现资产。

## 角色资产

- 通用静态回退：`Content/ReEcho/Textures/Characters/`。
- MushroomGirl 原始帧：`Content/SourceArt/Characters/MushroomGirl/`。
- MushroomGirl 运行时纹理：`Content/ReEcho/Textures/Characters/MushroomGirl/`。
- Plan40 序列动画：按角色、敌人和状态放在 `Content/ReEcho/Art/Animation2D/`，不把运行时资产导入 `SourceArt`。

## 导入规则

1. 源 PNG 保留在 `SourceArt`，2D 动画运行时导入目标必须位于 `ReEcho/Art/Animation2D`。
2. `.uasset` 的移动、重命名和删除通过 Unreal Editor 执行，以维护引用和重定向器。
3. 重新导入后先检查引用、像素密度、过滤方式、透明通道、压缩和 Paper2D 碰撞设置，再提交二进制变更。
4. `Player2D`、`SoftGroundShadow` 等 C++ 硬引用资产不得仅凭目录观感清理。
5. 未跟踪且误生成在 `SourceArt` 的 `.uasset` 先隔离，确认无引用后再永久删除。

## 当前审计结论（2026-08-13）

- `Player2D` 与 `SoftGroundShadow` 仍由运行时代码直接加载，必须保留。
- `Echo2D`、`Grunt2D` 仍属于旧静态回退/Cook 兼容资产，暂不删除。
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
