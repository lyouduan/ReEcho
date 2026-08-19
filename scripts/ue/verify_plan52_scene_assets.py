"""Verify the Plan52 scene asset graph and Level00 migration without modifying assets."""

import unreal


SCENE_IDS = ("SC01", "SC02", "SC03", "SC04")
LEGACY_ASSETS = (
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
)


def fail(message):
    raise RuntimeError(f"[Plan52] {message}")


for scene_id in SCENE_IDS:
    texture = unreal.EditorAssetLibrary.load_asset(
        f"/Game/ReEcho/Art/Scene/Map/{scene_id.lower()}"
    )
    material = unreal.EditorAssetLibrary.load_asset(
        f"/Game/ReEcho/Art/Scene/Map/Materials/MI_{scene_id}"
    )
    profile = unreal.EditorAssetLibrary.load_asset(
        f"/Game/ReEcho/Scene/Profiles/DA_ArenaScene_{scene_id}"
    )
    blueprint = unreal.EditorAssetLibrary.load_asset(
        f"/Game/ReEcho/Scene/Prefabs/BP_ArenaScene_{scene_id}"
    )
    if not isinstance(texture, unreal.Texture2D):
        fail(f"Missing Texture2D for {scene_id}")
    if not isinstance(material, unreal.MaterialInstanceConstant):
        fail(f"Missing material instance for {scene_id}")
    assigned_texture = unreal.MaterialEditingLibrary.get_material_instance_texture_parameter_value(
        material, "MapTexture"
    )
    if assigned_texture != texture:
        assigned_path = assigned_texture.get_path_name() if assigned_texture else "None"
        fail(
            f"Material texture mismatch for {scene_id}: expected {texture.get_path_name()}, "
            f"found {assigned_path}"
        )
    if texture.blueprint_get_size_x() != 3840 or texture.blueprint_get_size_y() != 2160:
        fail(
            f"Unexpected source dimensions for {scene_id}: "
            f"{texture.blueprint_get_size_x()}x{texture.blueprint_get_size_y()}"
        )
    if profile is None or profile.get_editor_property("scene_id") != scene_id:
        fail(f"Missing or mismatched profile for {scene_id}")
    if profile.get_editor_property("map_material") != material:
        fail(f"Profile map material mismatch for {scene_id}")
    if blueprint is None:
        fail(f"Missing Arena Blueprint for {scene_id}")
    if unreal.get_default_object(blueprint.generated_class()).get_editor_property("scene_profile") != profile:
        fail(f"Arena Blueprint profile mismatch for {scene_id}")

if unreal.EditorAssetLibrary.load_asset("/Game/ReEcho/Art/Scene/Map/Materials/M_ArenaGround") is None:
    fail("Missing M_ArenaGround")
if unreal.EditorAssetLibrary.load_asset("/Game/ReEcho/Scene/Prefabs/BP_ArenaScene") is None:
    fail("Missing BP_ArenaScene")
for legacy_asset in LEGACY_ASSETS:
    if unreal.EditorAssetLibrary.does_asset_exist(legacy_asset):
        fail(f"Confirmed legacy map still exists: {legacy_asset}")

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level_subsystem.load_level("/Game/Level00"):
    fail("Could not load Level00")
arena_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaSceneActor")
arenas = unreal.EditorFilterLibrary.by_class(actor_subsystem.get_all_level_actors(), arena_class)
if len(arenas) != 1:
    fail(f"Level00 must contain one Arena Scene, found {len(arenas)}")
arena = arenas[0]
if arena.get_class().get_path_name() != "/Game/ReEcho/Scene/Prefabs/BP_ArenaScene_SC01.BP_ArenaScene_SC01_C":
    fail(f"Level00 uses unexpected Arena class: {arena.get_class().get_path_name()}")
if arena.get_editor_property("scene_profile").get_editor_property("scene_id") != "SC01":
    fail("Level00 Arena does not use SC01 profile")
generated = [
    actor
    for actor in actor_subsystem.get_all_level_actors()
    if "ReEchoGeneratedDecoration_Plan52" in [str(tag) for tag in actor.tags]
]
if len(generated) != 18:
    fail(f"Expected 18 Plan52 generated decorations, found {len(generated)}")
unreal.log("[Plan52] Asset verification passed: four scenes, Level00 SC01, 18 decorations, legacy maps absent")
