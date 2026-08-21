"""Verify the SC02 asset chain and Level00 binding without modifying assets."""

import unreal


def fail(message):
    raise RuntimeError(f"[SC02Verify] {message}")


texture = unreal.EditorAssetLibrary.load_asset("/Game/ReEcho/Art/Scene/Map/sc02")
material = unreal.EditorAssetLibrary.load_asset("/Game/ReEcho/Art/Scene/Map/Materials/MI_SC02")
profile = unreal.EditorAssetLibrary.load_asset("/Game/ReEcho/Scene/Profiles/DA_ArenaScene_SC02")
blueprint = unreal.EditorAssetLibrary.load_asset("/Game/ReEcho/Scene/Prefabs/BP_ArenaScene_SC02")
if not isinstance(texture, unreal.Texture2D):
    fail("Missing SC02 Texture2D")
if not isinstance(material, unreal.MaterialInstanceConstant):
    fail("Missing SC02 material instance")
assigned_texture = unreal.MaterialEditingLibrary.get_material_instance_texture_parameter_value(
    material, "MapTexture"
)
if assigned_texture != texture:
    fail("MI_SC02 does not reference the SC02 texture")
if profile is None or profile.get_editor_property("scene_id") != "SC02":
    fail("Missing or mismatched SC02 profile")
if profile.get_editor_property("map_material") != material:
    fail("SC02 profile does not reference MI_SC02")
if blueprint is None:
    fail("Missing BP_ArenaScene_SC02")
if unreal.get_default_object(blueprint.generated_class()).get_editor_property("scene_profile") != profile:
    fail("BP_ArenaScene_SC02 does not reference the SC02 profile")

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level_subsystem.load_level("/Game/Level00"):
    fail("Could not load Level00")
arena_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaSceneActor")
arenas = unreal.EditorFilterLibrary.by_class(actor_subsystem.get_all_level_actors(), arena_class)
if len(arenas) != 1:
    fail(f"Level00 must contain one Arena Scene, found {len(arenas)}")
arena = arenas[0]
if arena.get_class().get_path_name() != "/Game/ReEcho/Scene/Prefabs/BP_ArenaScene_SC02.BP_ArenaScene_SC02_C":
    fail(f"Level00 uses unexpected Arena class: {arena.get_class().get_path_name()}")
if arena.get_editor_property("scene_profile") != profile:
    fail("Level00 Arena does not reference the SC02 profile")

unreal.log("[SC02Verify] Passed: texture, material, profile, Blueprint, and Level00 references are SC02")
