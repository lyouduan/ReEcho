"""Import the reviewed Plan 45 UI placeholder slices through Unreal Editor.

Run with UnrealEditor-Cmd and ``-ExecutePythonScript``. Reference screenshots and
pending-license fonts are intentionally absent from this mapping.
"""

from pathlib import Path

import unreal


SOURCE_ROOT = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "InteractionPlaceholder"
    / "Elements"
)
DESTINATION_ROOT = "/Game/ReEcho/Textures/UI/InteractionPlaceholder"
REIMPORT_EXISTING = (
    "-Plan45ReimportExisting" in unreal.SystemLibrary.get_command_line()
)

# Import only slices that have a current WBP consumer. Extend this mapping one
# reviewed page at a time; do not bulk-import delivery references.
IMPORTS = {
    "StartMenu": {
        "bg.png": "T_UI_Start_Background",
        "主体文字.png": "T_UI_Start_TitleLogo",
        "主页按钮.png": "T_UI_Start_PrimaryActions",
        "设置.png": "T_UI_Start_SettingsIcon",
        "退出.png": "T_UI_Start_Quit",
    },
    "Settings": {
        "设置、存档弹窗底板.png": "T_UI_Settings_Panel",
        "叉.png": "T_UI_Settings_Close",
        "打开声音.png": "T_UI_Settings_Unmuted",
        "静音.png": "T_UI_Settings_Muted",
        "滑动条.png": "T_UI_Settings_SliderFill",
        "滑动条底板.png": "T_UI_Settings_SliderTrack",
        "画面激活.png": "T_UI_Settings_GraphicsActive",
        "画面置灰.png": "T_UI_Settings_GraphicsInactive",
        "声音激活.png": "T_UI_Settings_AudioActive",
        "声音置灰.png": "T_UI_Settings_AudioInactive",
        "键位激活.png": "T_UI_Settings_ControlsActive",
        "键位置灰.png": "T_UI_Settings_ControlsInactive",
        "恢复默认按钮.png": "T_UI_Settings_RestoreDefaults",
        "应用按钮.png": "T_UI_Settings_Apply",
        "填空框.png": "T_UI_Settings_KeyField",
        "下拉框.png": "T_UI_Settings_Dropdown",
        "下拉箭头.png": "T_UI_Settings_DropdownArrow",
        "下拉箭头2.png": "T_UI_Settings_DropdownArrowAlt",
    },
    "PauseAndCombat": {
        "生命icon.png": "T_UI_HUD_HealthIcon",
        "生命条.png": "T_UI_HUD_HealthFill",
        "生命条底板.png": "T_UI_HUD_HealthFrame",
        "时间碎片icon.png": "T_UI_HUD_TimeShardIcon",
        "时钟底板.png": "T_UI_HUD_ClockFrame",
        "指针（需要从右到左转动）.png": "T_UI_HUD_ClockNeedle",
        "轨迹地图板.png": "T_UI_HUD_TrajectoryMap",
        "技能栏.png": "T_UI_HUD_SkillBar",
        "继续游戏按钮.png": "T_UI_Pause_Resume",
        "退出至主菜单.png": "T_UI_Pause_ExitToMenu",
        "退出游戏.png": "T_UI_Pause_ExitGame",
        "保存并退出.png": "T_UI_Pause_SaveAndExit",
        "不保存并退出.png": "T_UI_Pause_ExitWithoutSave",
        "返回.png": "T_UI_Pause_Back",
    },
    "ResultsAndRestart": {
        "弹窗底板.png": "T_UI_Restart_DialogPanel",
        "主体立绘.png": "T_UI_Restart_Character",
        "本轮胜利花字.png": "T_UI_Result_VictoryTitle",
        "本轮失败花字.png": "T_UI_Result_DefeatTitle",
        "结算底板.png": "T_UI_Result_SummaryPanel",
        "已选卡牌底板.png": "T_UI_Result_SelectedCardsPanel",
        "继续按钮.png": "T_UI_Result_Continue",
        "返回按钮.png": "T_UI_Result_Back",
        "重开按钮.png": "T_UI_Result_Restart",
        "弹窗按钮-取消.png": "T_UI_Restart_Cancel",
        "弹窗按钮-重开.png": "T_UI_Restart_Confirm",
    },
    "TraitChoice": {
        "卡牌底.png": "T_UI_Trait_CardFrame",
        "卡牌图片占位.png": "T_UI_Trait_ImagePlaceholder",
        "标签底板1.png": "T_UI_Trait_TagPrimary",
        "标签底板2.png": "T_UI_Trait_TagSecondary",
    },
    "InventoryShop": {
        "时间商店底板.png": "T_UI_Shop_OfferPanel",
        "时间商店标题.png": "T_UI_Shop_Title",
        "装配室底板.png": "T_UI_Shop_LoadoutPanel",
        "装配室.png": "T_UI_Shop_LoadoutTitle",
        "面具人立绘.png": "T_UI_Shop_Keeper",
        "当前属性面板底板.png": "T_UI_Stats_Panel",
        "时间碎片底框.png": "T_UI_Shop_CurrencyFrame",
        "商店按钮-刷新.png": "T_UI_Shop_Refresh",
        "商店按钮-购买.png": "T_UI_Shop_Buy",
        "卡片底板.png": "T_UI_Shop_ItemCard",
        "卡牌icon.png": "T_UI_Shop_CardIcon",
        "配件icon.png": "T_UI_Shop_AttachmentIcon",
        "武器图.png": "T_UI_Shop_Weapon",
        "武器配件槽.png": "T_UI_Shop_AttachmentSlot",
        "悬浮卡牌槽.png": "T_UI_Shop_HoverCardSlot",
        "技能文字介绍浮框.png": "T_UI_Shop_SkillTooltip",
        "属性icon.png": "T_UI_Shop_StatIcon",
        "时钟.png": "T_UI_Shop_Clock",
        "保存配置按钮.png": "T_UI_Shop_SaveLoadout",
        "离开商店.png": "T_UI_Shop_Leave",
    },
}


def configure_ui_texture(texture: unreal.Texture2D) -> None:
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
    texture.set_editor_property(
        "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)


def import_texture(category: str, source_name: str, asset_name: str) -> str:
    source = SOURCE_ROOT / category / source_name
    if not source.is_file():
        raise RuntimeError(f"Missing reviewed UI source: {source}")

    destination_path = f"{DESTINATION_ROOT}/{category}"
    expected_path = f"{destination_path}/{asset_name}"
    existing_texture = unreal.load_asset(expected_path)
    if existing_texture is not None and not REIMPORT_EXISTING:
        if not isinstance(existing_texture, unreal.Texture2D):
            raise RuntimeError(f"Existing asset is not Texture2D: {expected_path}")
        return expected_path

    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = destination_path
    task.destination_name = asset_name
    task.filename = str(source)
    task.replace_existing = REIMPORT_EXISTING
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.load_asset(expected_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Imported asset is not Texture2D: {expected_path}")

    configure_ui_texture(texture)
    return expected_path


imported = []
for page_category, page_imports in IMPORTS.items():
    for filename, runtime_name in page_imports.items():
        imported.append(import_texture(page_category, filename, runtime_name))

unreal.log(f"Plan45 resolved {len(imported)} reviewed UI textures: {imported}")
