"""Switch Level00's unique Arena instance to SC02 while preserving authored settings."""

import unreal


LEVEL_PATH = "/Game/Level00"
TARGET_CLASS_PATH = "/Game/ReEcho/Scene/Prefabs/BP_ArenaScene_SC02.BP_ArenaScene_SC02_C"
PROFILE_PATH = "/Game/ReEcho/Scene/Profiles/DA_ArenaScene_SC02"
ACTOR_PROPERTIES = (
    "backdrop_half_extents",
    "camera_clamp_half_extents",
    "player_half_extents",
    "enemy_spawn_half_extents",
    "gameplay_plane_z",
    "auto_layout_backdrop",
    "auto_layout_collision",
    "depth_sort_axis",
    "depth_sort_world_units_per_step",
    "depth_sort_base_priority",
    "depth_sort_priority_range",
    "default_contact_shadow_texture",
    "default_contact_shadow_size",
    "default_contact_shadow_opacity",
    "enable_parallax",
    "mid_decoration_parallax_factor",
    "foreground_parallax_factor",
    "atmosphere_parallax_factor",
    "maximum_parallax_offset",
)
COMPONENT_PROPERTIES = (
    "map_root",
    "backdrop",
    "floor",
    "wall_north",
    "wall_south",
    "wall_east",
    "wall_west",
)


def fail(message):
    raise RuntimeError(f"[SC02Switch] {message}")


def copy_relative_transform(source, target):
    target.set_editor_property("relative_location", source.get_editor_property("relative_location"))
    target.set_editor_property("relative_rotation", source.get_editor_property("relative_rotation"))
    target.set_editor_property("relative_scale3d", source.get_editor_property("relative_scale3d"))


level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level_subsystem.load_level(LEVEL_PATH):
    fail(f"Could not load {LEVEL_PATH}")

arena_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaSceneActor")
target_class = unreal.load_class(None, TARGET_CLASS_PATH)
target_profile = unreal.EditorAssetLibrary.load_asset(PROFILE_PATH)
if arena_class is None or target_class is None or target_profile is None:
    fail("Required native class, SC02 Blueprint class, or SC02 profile is unavailable")

arenas = unreal.EditorFilterLibrary.by_class(actor_subsystem.get_all_level_actors(), arena_class)
if len(arenas) != 1:
    fail(f"Expected exactly one Arena Scene actor, found {len(arenas)}")

old_actor = arenas[0]
if old_actor.get_class() == target_class:
    old_actor.set_editor_property("scene_profile", target_profile)
    if not level_subsystem.save_current_level():
        fail("Could not save already-switched Level00")
    unreal.log("[SC02Switch] Level00 already uses BP_ArenaScene_SC02; refreshed its profile")
else:
    new_actor = actor_subsystem.spawn_actor_from_class(
        target_class, old_actor.get_actor_location(), old_actor.get_actor_rotation()
    )
    if new_actor is None:
        fail("Could not spawn BP_ArenaScene_SC02")
    new_actor.set_actor_scale3d(old_actor.get_actor_scale3d())
    new_actor.set_actor_label(old_actor.get_actor_label())
    for property_name in ACTOR_PROPERTIES:
        new_actor.set_editor_property(property_name, old_actor.get_editor_property(property_name))
    new_actor.set_editor_property("scene_profile", target_profile)
    for property_name in COMPONENT_PROPERTIES:
        copy_relative_transform(
            old_actor.get_editor_property(property_name), new_actor.get_editor_property(property_name)
        )
    if new_actor.get_editor_property("scene_profile") != target_profile:
        fail("New Arena Scene did not retain the SC02 profile")
    if not actor_subsystem.destroy_actor(old_actor):
        fail("Could not remove the replaced Arena Scene instance")
    remaining = unreal.EditorFilterLibrary.by_class(actor_subsystem.get_all_level_actors(), arena_class)
    if len(remaining) != 1 or remaining[0] != new_actor:
        fail("Level00 Arena Scene uniqueness check failed after replacement")
    if not level_subsystem.save_current_level():
        fail(f"Could not save {LEVEL_PATH}")
    unreal.log("[SC02Switch] Level00 now uses BP_ArenaScene_SC02 with preserved authored settings")
