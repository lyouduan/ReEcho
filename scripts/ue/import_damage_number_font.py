"""Import the non-commercial MF YuYue font for world-space damage numbers.

TextRenderComponent requires an offline-cached UFont. The operation is
idempotent; pass ``-ReimportDamageNumberFont`` to rebuild its digit atlas.
"""

import ctypes
from pathlib import Path

import unreal


SOURCE_PATH = (
    Path(unreal.Paths.project_content_dir())
    / "SourceArt"
    / "UI"
    / "CombatHud"
    / "DamageNumbers"
    / "MFYuYue-Noncommercial-Regular-2.otf"
)
DESTINATION_ROOT = "/Game/ReEcho/Fonts/DamageNumbers"
FONT_PATH = f"{DESTINATION_ROOT}/F_DamageNumber_MFYuYue_Font"
LEGACY_FONT_FACE_PATH = f"{DESTINATION_ROOT}/F_DamageNumber_MFYuYue"
FONT_FAMILY_NAME = "MFYuYueNoncommercial"
DAMAGE_CHARACTERS = "0123456789_"
REIMPORT_EXISTING = (
    "-ReimportDamageNumberFont" in unreal.SystemLibrary.get_command_line()
)


def load_font():
    return unreal.load_asset(FONT_PATH)


def is_renderable_offline_font(font):
    return (
        isinstance(font, unreal.Font)
        and font.get_editor_property("font_cache_type")
        == unreal.FontCacheType.OFFLINE
    )


def delete_asset_if_present(asset_path):
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        if not unreal.EditorAssetLibrary.delete_asset(asset_path):
            raise RuntimeError(f"Unable to replace existing font asset: {asset_path}")


def build_offline_font():
    private_font_flag = 0x10  # FR_PRIVATE: visible only to this Editor process.
    added_font_count = ctypes.windll.gdi32.AddFontResourceExW(
        str(SOURCE_PATH), private_font_flag, 0
    )
    if added_font_count <= 0:
        raise RuntimeError(f"Unable to register supplied font temporarily: {SOURCE_PATH}")

    try:
        factory = unreal.TrueTypeFontFactory()
        options = factory.get_editor_property("import_options")
        data = options.get_editor_property("data")
        data.set_editor_property("font_name", FONT_FAMILY_NAME)
        data.set_editor_property("height", 64.0)
        data.set_editor_property("enable_antialiasing", True)
        data.set_editor_property("alpha_only", True)
        data.set_editor_property("chars", DAMAGE_CHARACTERS)
        data.set_editor_property("include_ascii_range", False)
        data.set_editor_property("create_printable_only", True)
        data.set_editor_property("texture_page_width", 256)
        data.set_editor_property("texture_page_max_height", 256)
        options.set_editor_property("data", data)

        font = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "F_DamageNumber_MFYuYue_Font",
            DESTINATION_ROOT,
            unreal.Font,
            factory,
        )
        if not is_renderable_offline_font(font):
            raise RuntimeError(f"Offline damage-number font creation failed: {FONT_PATH}")
        unreal.EditorAssetLibrary.save_loaded_asset(font, only_if_is_dirty=False)
        return font
    finally:
        ctypes.windll.gdi32.RemoveFontResourceExW(
            str(SOURCE_PATH), private_font_flag, 0
        )


if not SOURCE_PATH.is_file():
    raise RuntimeError(f"Missing supplied damage-number font: {SOURCE_PATH}")

font = load_font()
if REIMPORT_EXISTING or not is_renderable_offline_font(font):
    delete_asset_if_present(FONT_PATH)
    # Remove the earlier runtime FontFace form; TextRender cannot render it.
    delete_asset_if_present(LEGACY_FONT_FACE_PATH)
    font = build_offline_font()

if not is_renderable_offline_font(font):
    raise RuntimeError(f"Damage-number font is not TextRender-compatible: {FONT_PATH}")

unreal.log(
    f"[DamageNumberFontImport] ready offline_font={FONT_PATH} "
    f"characters={DAMAGE_CHARACTERS}"
)
