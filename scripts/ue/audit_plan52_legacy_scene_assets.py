"""Report referencers for Plan52 legacy map and Arena Blueprint candidates. Never deletes assets."""

import unreal


LEGACY_CANDIDATES = (
    "/Game/ReEcho/Art/Scene/Map/Map00",
    "/Game/ReEcho/Art/Scene/Map/Map01",
    "/Game/ReEcho/Art/Scene/Map/Night",
    "/Game/ReEcho/Art/Scene/Map/Materials/MI_Map00",
    "/Game/ReEcho/Art/Scene/Map/Materials/MI_Map01",
    "/Game/ReEcho/Art/Scene/map0",
    "/Game/ReEcho/Art/Scene/map00",
    "/Game/ReEcho/Art/Scene/map01",
    "/Game/ReEcho/Art/Scene/map02",
    "/Game/ReEcho/Art/Scene/map03",
    "/Game/ReEcho/Art/Scene/map04",
    "/Game/ReEcho/Scene/Prefabs/BP_ArenaScene_Map00",
)


for asset_path in LEGACY_CANDIDATES:
    if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log(f"PLAN52_AUDIT missing={asset_path}")
        continue
    referencers = unreal.EditorAssetLibrary.find_package_referencers_for_asset(
        asset_path, load_assets_to_confirm=True
    )
    unreal.log(f"PLAN52_AUDIT asset={asset_path} referencers={referencers}")
