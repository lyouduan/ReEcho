# ReEcho UI Icon 替换指南（策划）

本文用于替换**已经存在的**卡牌、武器配件、商店属性、元素或角色 icon。只替换画面、不增加新 ID、不修改玩法数据时，可以按本文自行完成。

如果要新增卡牌/配件、改变 ID、改变启用状态、修改运行时绑定或让一种新 icon 类型进入界面，请交给程序处理。不要为了换图直接编辑 `Content/Data/*.csv`；这些 CSV 是从策划工作簿生成的运行时数据，不是图片替换入口。

## 最重要的原则

1. 真正的源图位于 `Content/SourceArt/`，Unreal 运行时纹理位于 `Content/ReEcho/Textures/`。
2. 替换现有 icon 时，**文件名、稳定 ID 和运行时资产路径都不要改**，只替换 PNG 内容。
3. 不要在资源管理器中直接覆盖 `.uasset`，也不要把新图拖进 Content Browser 生成 `名称_1`。应替换源 PNG，再对原 Texture2D 执行重新导入。
4. `策划数据源/icon/` 是本地整批交付与校对区，不是 Unreal 的实际导入目录。只改这里，游戏不会变化。
5. PNG 应保留透明通道，建议沿用旧图的画布尺寸、比例和留白。不要通过拉伸图片来适配界面。

## 各类 icon 的稳定路径

### 卡牌 icon

先在 `Content/Data/cards.csv` 中按 `DisplayName` 找到对应的 `Id`，只读取 ID，不要手改该 CSV。

| 项目 | 路径规则 |
|---|---|
| 源 PNG | `Content/SourceArt/UI/Cards/Icon/T_UI_CardIcon_<CardId>.png` |
| Unreal 纹理 | `/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_<CardId>` |

例如 `G_1_01「速度强化」`：

- 源图：`Content/SourceArt/UI/Cards/Icon/T_UI_CardIcon_G_1_01.png`
- 运行时纹理：`/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_G_1_01`

### 武器配件 icon

先在 `Content/Data/parts.csv` 中按 `DisplayName` 找到对应的 `PartId`。不要用中文名作为运行时文件名。

| 项目 | 路径规则 |
|---|---|
| 源 PNG | `Content/SourceArt/UI/WeaponParts/Icons/T_UI_Part_<PartId>.png` |
| Unreal 纹理 | `/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_<PartId>` |
| 中文名与 ID 对照 | `Content/SourceArt/UI/WeaponParts/Icons/icon_manifest.csv` |

例如 `连射移速枪机`：

- `PartId`：`P_GUN_MOVESTACK_GUNACTION`
- 源图：`Content/SourceArt/UI/WeaponParts/Icons/T_UI_Part_P_GUN_MOVESTACK_GUNACTION.png`
- 运行时纹理：`/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_GUN_MOVESTACK_GUNACTION`

仅替换现有图片时，不需要修改 `icon_manifest.csv`。只有中文名、`PartId` 或交付映射发生变化时才需要程序更新清单。

### 商店属性 icon

| 中文含义 | 源文件名 | Unreal 纹理名 |
|---|---|---|
| 生命 | `Health.png` | `T_UI_Shop110_Health` |
| 物理攻击 | `PhysicalAttack.png` | `T_UI_Shop110_PhysicalAttack` |
| 元素攻击 | `ElementalAttack.png` | `T_UI_Shop110_ElementalAttack` |
| 移动速度 | `MovementSpeed.png` | `T_UI_Shop110_MovementSpeed` |
| 暴击率 | `CriticalChance.png` | `T_UI_Shop110_CriticalChance` |
| 暴击效果 | `CriticalEffect.png` | `T_UI_Shop110_CriticalEffect` |
| 回响效率 | `EchoEfficiency.png` | `T_UI_Shop110_EchoEfficiency` |
| 反应效率 | `ReactionEfficiency.png` | `T_UI_Shop110_ReactionEfficiency` |

- 源目录：`Content/SourceArt/UI/InventoryShop/Plan110/Elements/`
- Unreal 目录：`/Game/ReEcho/Textures/UI/InventoryShop/Plan110/`

### 元素 icon

| 元素 | 源 PNG / Unreal 纹理名 |
|---|---|
| 火 | `T_UI_Element_Flame` |
| 雷 | `T_UI_Element_Lightning` |
| 草 | `T_UI_Element_Grass` |
| 水 | `T_UI_Element_Water` |

- 源目录：`Content/SourceArt/UI/IconCatalog/Elements/`
- Unreal 目录：`/Game/ReEcho/Textures/UI/IconCatalog/Elements/`

### 角色 icon

| 角色 | CharacterId | 源 PNG / Unreal 纹理名 |
|---|---|---|
| 猎手 | `J_DIAMOND` | `T_UI_CharacterIcon_J_DIAMOND` |
| 诗人 | `J_CLOVER` | `T_UI_CharacterIcon_J_CLOVER` |
| 勇者 | `J_HEART` | `T_UI_CharacterIcon_J_HEART` |
| 智者 | `J_SPADE` | `T_UI_CharacterIcon_J_SPADE` |

- 源目录：`Content/SourceArt/UI/IconCatalog/Characters/`
- Unreal 目录：`/Game/ReEcho/Textures/UI/IconCatalog/Characters/`

## 单张 icon 替换（推荐）

1. 确认 ID 和上表中的稳定源文件。
2. 备份旧 PNG，然后用新 PNG **覆盖同名源文件**。不要改名。
3. 打开 `ReEcho.uproject`，在 Content Browser 找到上表对应的原 Texture2D。
4. 右键该 Texture2D，选择“重新导入”；如果源路径不正确，选择“使用新文件重新导入”，并指定刚才覆盖的 `Content/SourceArt` PNG。
5. 打开 Texture2D 检查：

   - 透明背景正常，没有白底或黑底；
   - 图形没有被压扁或拉长；
   - 边缘没有被画布裁切；
   - `Texture Group` 为 `UI`，`Mip Gen Settings` 为 `NoMipmaps`；
   - `Compression Settings` 为项目当前导入脚本使用的 `Editor Icon` UI 压缩设置。

6. 保存 Texture2D，然后在实际界面中检查正常、悬停、选中、已装备等状态。
7. 提交时至少包含两项：被替换的 `Content/SourceArt` PNG，以及重新导入后同路径的运行时 `.uasset`。

## 整批替换

整批替换前先保存并关闭 Unreal Editor，在项目根目录打开 PowerShell。

先检查 Git LFS：

```powershell
python scripts/setup_lfs.py --check
```

根据替换类型运行相应导入脚本：

```powershell
# 全部 64 张当前启用卡牌
& scripts\ue\Run-EditorPythonLocked.ps1 -ScriptPath scripts\ue\import_plan128_complete_card_icons.py

# 全部 48 张已交付武器配件（其中 45 张当前在商店启用）
& scripts\ue\Run-EditorPythonLocked.ps1 -ScriptPath scripts\ue\import_weapon_part_icons.py

# 商店 Plan110 全套切图；不仅会重导 8 张属性 icon
& scripts\ue\Run-EditorPythonLocked.ps1 -ScriptPath scripts\ue\import_plan110_formal_shop_ui.py

# 4 张元素 icon + 4 张角色 icon
& scripts\ue\Run-EditorPythonLocked.ps1 -ScriptPath scripts\ue\import_plan147_shared_icon_catalog.py
```

如果只换一张商店属性 icon，优先使用上一节的单张重新导入，避免无意义地改写 Plan110 其他纹理资产。

## 自动检查

关闭 Unreal Editor后运行：

```powershell
& scripts\ue\Run-EditorPythonLocked.ps1 -ScriptPath scripts\ue\audit_plan147_complete_shop_icons.py
```

打开 `Saved/Logs/ReEcho.log`，搜索以下 PASS：

```text
[Plan147ShopIconAudit] PASS cards=64 active_shop_parts=45 delivered_parts=48 shop_stats=8 shared=8
```

不要只看 PowerShell 最后的退出码；应以日志中的 `PASS` 为准，并确认其后没有 `LogPython: Error`。

如果本地仍保留 `策划数据源/icon/`，审计还会检查该交付目录中的每张 PNG 是否已经复制到规范化 SourceArt。这个目录不是正式运行时来源，通常不随代码提交；若更新整批交付，应同步更新规范化 SourceArt。

## 界面人工检查清单

- 商店商品卡没有空方框或默认占位图。
- 中文名称与图形语义一致，特别检查同武器、同槽位的近似名称。
- icon 没有拉伸、裁边、黑底或白边。
- 透明区域不会遮住价格、名称、已装备状态或鼠标交互。
- 卡牌三选一、商店列表、装备区等所有会复用该纹理的位置都已检查。
- 至少重新启动一次 Editor 或重新进入界面，确认不是旧缓存画面。

## 遇到以下情况请找程序

- 新增卡牌、配件、角色或元素，而不是替换已有 ID 的图片。
- 找不到对应 `CardId` / `PartId`，或中文名对应多个候选 ID。
- 需要修改 `cards.csv`、`parts.csv`、`icon_manifest.csv` 或导入脚本。
- 重新导入成功但游戏仍显示占位图，说明可能缺少运行时绑定。
- 审计数量不再是 64 张活动卡牌、45 个启用商店配件或 48 个已交付配件。
- `.uasset` 出现合并冲突，或同一运行时纹理已被其他分支修改。
