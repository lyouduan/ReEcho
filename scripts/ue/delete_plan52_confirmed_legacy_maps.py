"""Delete only the legacy map assets explicitly confirmed for Plan52."""

import unreal


LEGACY_BLUEPRINT = "/Game/ReEcho/Scene/Prefabs/BP_ArenaScene_Map00"
SC01_MATERIAL = "/Game/ReEcho/Art/Scene/Map/Materials/MI_SC01"
SC01_PROFILE = "/Game/ReEcho/Scene/Profiles/DA_ArenaScene_SC01"
DELETE_ASSETS = (
    "/Game/ReEcho/Art/Scene/Map/Materials/MI_Map00",
    "/Game/ReEcho/Art/Scene/Map/Materials/MI_Map01",
    "/Game/ReEcho/Art/Scene/Map/Map00",
    "/Game/ReEcho/Art/Scene/Map/Map01",
    "/Game/ReEcho/Art/Scene/Map/Night",
    "/Game/ReEcho/Art/Scene/map0",
    "/Game/ReEcho/Art/Scene/map00",
    "/Game/ReEcho/Art/Scene/map01",
    "/Game/ReEcho/Art/Scene/map02",
    "/Game/ReEcho/Art/Scene/map03",
    "/Game/ReEcho/Art/Scene/map04",
)


def fail(message):
    raise RuntimeError(f"[Plan52] {message}")


legacy_blueprint = unreal.EditorAssetLibrary.load_asset(LEGACY_BLUEPRINT)
sc01_material = unreal.EditorAssetLibrary.load_asset(SC01_MATERIAL)
sc01_profile = unreal.EditorAssetLibrary.load_asset(SC01_PROFILE)
if legacy_blueprint is None or sc01_material is None or sc01_profile is None:
    fail("Could not load the retained legacy Blueprint or its SC01 replacement assets")
legacy_cdo = unreal.get_default_object(legacy_blueprint.generated_class())
legacy_cdo.set_editor_property("map_material", sc01_material)
legacy_cdo.set_editor_property("scene_profile", sc01_profile)
unreal.BlueprintEditorLibrary.compile_blueprint(legacy_blueprint)
if not unreal.EditorAssetLibrary.save_loaded_asset(legacy_blueprint, only_if_is_dirty=False):
    fail("Could not save BP_ArenaScene_Map00 after redirecting its map reference")

for asset_path in DELETE_ASSETS:
    if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        fail(f"Confirmed delete target is missing: {asset_path}")
    if not unreal.EditorAssetLibrary.delete_asset(asset_path):
        fail(f"Could not delete confirmed legacy asset: {asset_path}")
    unreal.log(f"[Plan52] Deleted confirmed legacy asset {asset_path}")

for asset_path in DELETE_ASSETS:
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        fail(f"Legacy asset still exists after deletion: {asset_path}")
unreal.log("[Plan52] Deleted all confirmed legacy map uassets")
