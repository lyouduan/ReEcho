"""Create/update only the Plan134 SceneId-to-Arena Blueprint catalog.

This script deliberately does not modify Arena Blueprints, Level00, inserts, materials,
component transforms, visibility, or translucency ordering.
"""

import unreal


CATALOG_PATH = "/Game/ReEcho/Scene/DA_ArenaSceneCatalog"
PREFAB_ROOT = "/Game/ReEcho/Scene/Prefabs"
SCENE_IDS = ("SC01", "SC02", "SC03", "SC04")


def fail(message):
    raise RuntimeError(f"[Plan134] {message}")


catalog = unreal.EditorAssetLibrary.load_asset(CATALOG_PATH)
if catalog is None:
    catalog = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "DA_ArenaSceneCatalog",
        "/Game/ReEcho/Scene",
        unreal.ReEchoArenaSceneCatalog,
        unreal.DataAssetFactory(),
    )
if not isinstance(catalog, unreal.ReEchoArenaSceneCatalog):
    fail(f"Unexpected asset type at {CATALOG_PATH}")

registrations = []
for scene_id in SCENE_IDS:
    scene_class = unreal.load_class(
        None,
        f"{PREFAB_ROOT}/BP_ArenaScene_{scene_id}.BP_ArenaScene_{scene_id}_C",
    )
    if scene_class is None:
        fail(f"Missing Arena Blueprint class for {scene_id}")
    registrations.append(
        unreal.ReEchoArenaSceneRegistration(
            scene_id=unreal.Name(scene_id), arena_class=scene_class
        )
    )

catalog.set_editor_property("scenes", registrations)
if not unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False):
    fail("Could not save Arena Scene Catalog")
unreal.log("[Plan134] Authored single Arena Scene Catalog; no Blueprint or Level asset was modified")
