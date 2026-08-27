"""Read-only Plan134 scene authority audit; never saves or mutates assets."""

import unreal


CATALOG_PATH = "/Game/ReEcho/Scene/DA_ArenaSceneCatalog"
PREFAB_ROOT = "/Game/ReEcho/Scene/Prefabs"
SCENE_IDS = ("SC01", "SC02", "SC03", "SC04")
REQUIRED_COMPONENTS = (
    "Backdrop",
    "Ground",
    "GroundDetail",
    "MidDecoration",
    "Foreground",
    "Atmosphere",
    "SceneEffects",
    "CameraClampBounds",
    "PlayerBounds",
    "EnemySpawnBounds",
)


def fail(message):
    raise RuntimeError(f"[Plan134] {message}")


def components(blueprint):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    result = {}
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(data)
        if obj is not None:
            result[str(obj.get_name()).removesuffix("_GEN_VARIABLE")] = obj
    return result


catalog = unreal.EditorAssetLibrary.load_asset(CATALOG_PATH)
if not isinstance(catalog, unreal.ReEchoArenaSceneCatalog):
    fail("Single Arena Scene Catalog is missing")
registrations = catalog.get_editor_property("scenes")
registered_ids = [str(item.get_editor_property("scene_id")) for item in registrations]
if registered_ids != list(SCENE_IDS) or len(set(registered_ids)) != len(SCENE_IDS):
    fail(f"Catalog must map SC01-SC04 exactly once: {registered_ids}")

catalog_only = "-Plan134CatalogOnly" in unreal.SystemLibrary.get_command_line()

for scene_id, registration in zip(SCENE_IDS, registrations):
    blueprint = unreal.EditorAssetLibrary.load_asset(
        f"{PREFAB_ROOT}/BP_ArenaScene_{scene_id}"
    )
    if not isinstance(blueprint, unreal.Blueprint):
        fail(f"Missing Arena Blueprint {scene_id}")
    defaults = unreal.get_default_object(blueprint.generated_class())
    if str(defaults.get_editor_property("scene_id")) != scene_id:
        fail(f"Incorrect SceneId on {scene_id}")
    if registration.get_editor_property("arena_class") != blueprint.generated_class():
        fail(f"Catalog class mismatch for {scene_id}")
    if catalog_only:
        continue
    scene_components = components(blueprint)
    missing = [name for name in REQUIRED_COMPONENTS if name not in scene_components]
    if missing:
        fail(f"{scene_id} is missing required components: {missing}")
    backdrop = scene_components["Backdrop"]
    if backdrop.get_material(0) is None:
        fail(f"{scene_id} Backdrop Material Slot 0 is empty")
    if not defaults.get_editor_property("use_editor_authored_scene_layout"):
        fail(f"{scene_id} has not enabled editor-authored scene layout")
    for name in ("CameraClampBounds", "PlayerBounds", "EnemySpawnBounds"):
        extent = scene_components[name].get_editor_property("box_extent")
        if min(extent.x, extent.y) < 100.0:
            fail(f"{scene_id}.{name} is degenerate: {extent}")

if catalog_only:
    unreal.log("[Plan134] Read-only catalog audit passed: SC01-SC04 resolve exactly once")
else:
    unreal.log("[Plan134] Read-only scene audit passed: catalog, materials, visual roots and gameplay bounds")
