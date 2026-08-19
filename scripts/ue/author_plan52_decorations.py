"""Bake or clear deterministic, profile-driven Arena decoration cards in Level00.

Examples:
  -ExecutePythonScript=.../author_plan52_decorations.py -Plan52Operation=Bake
  -ExecutePythonScript=.../author_plan52_decorations.py -Plan52Operation=Clear

Clear only removes actors carrying the ReEchoGeneratedDecoration_Plan52 tag.
"""

import random
import re

import unreal


LEVEL_PATH = "/Game/Level00"
GENERATED_TAG = "ReEchoGeneratedDecoration_Plan52"
GENERATED_FOLDER = "Arena/GeneratedDecorations"
PLANE_PATH = "/Engine/BasicShapes/Plane.Plane"


def fail(message):
    raise RuntimeError(f"[Plan52] {message}")


def command_value(name, default):
    match = re.search(rf"-{name}=([^\s]+)", unreal.SystemLibrary.get_command_line(), re.IGNORECASE)
    return match.group(1) if match else default


def arena_actors(actor_subsystem):
    arena_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaSceneActor")
    return unreal.EditorFilterLibrary.by_class(actor_subsystem.get_all_level_actors(), arena_class)


def generated_actors(actor_subsystem):
    return [
        actor
        for actor in actor_subsystem.get_all_level_actors()
        if GENERATED_TAG in [str(tag) for tag in actor.tags]
    ]


def destroy_generated(actor_subsystem):
    actors = generated_actors(actor_subsystem)
    for actor in actors:
        if not actor_subsystem.destroy_actor(actor):
            fail(f"Could not remove generated decoration: {actor.get_actor_label()}")
    return len(actors)


def spawn_card(actor_subsystem, static_mesh, material, location, rotation, height, priority, index):
    actor = actor_subsystem.spawn_actor_from_class(unreal.StaticMeshActor, location, rotation)
    if actor is None:
        fail(f"Could not spawn decoration card {index}")
    actor.tags = [GENERATED_TAG]
    actor.set_actor_label(f"Plan52Decoration_{index:02d}_{material.get_name()}")
    actor.set_folder_path(GENERATED_FOLDER)
    component = actor.static_mesh_component
    component.set_static_mesh(static_mesh)
    component.set_material(0, material)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    component.set_cast_shadow(False)
    component.set_translucent_sort_priority(priority)
    width_ratio = 0.62
    actor.set_actor_scale3d(unreal.Vector(height / 100.0, height * width_ratio / 100.0, 1.0))
    return actor


def footpoint_priority(arena, x, y, center):
    axis = arena.get_editor_property("depth_sort_axis")
    length = max((axis.x * axis.x + axis.y * axis.y) ** 0.5, 0.0001)
    axis_x = axis.x / length
    axis_y = axis.y / length
    units = max(arena.get_editor_property("depth_sort_world_units_per_step"), 1.0)
    offset = round(((x - center.x) * axis_x + (y - center.y) * axis_y) / units)
    priority_range = arena.get_editor_property("depth_sort_priority_range")
    offset = max(min(offset, max(priority_range.x, priority_range.y)), min(priority_range.x, priority_range.y))
    return arena.get_editor_property("depth_sort_base_priority") + offset


operation = command_value("Plan52Operation", "Bake").lower()
level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level_subsystem.load_level(LEVEL_PATH):
    fail(f"Could not load {LEVEL_PATH}")
arenas = arena_actors(actor_subsystem)
if len(arenas) != 1:
    fail(f"Expected exactly one Arena Scene actor, found {len(arenas)}")
arena = arenas[0]
profile = arena.get_editor_property("scene_profile")
if profile is None:
    fail("Arena Scene has no SceneProfile")

if operation == "clear":
    removed = destroy_generated(actor_subsystem)
    if not level_subsystem.save_current_level():
        fail("Could not save Level00 after clearing generated decorations")
    unreal.log(f"[Plan52] Cleared {removed} tagged generated decorations")
elif operation == "bake":
    static_mesh = unreal.EditorAssetLibrary.load_asset(PLANE_PATH)
    if not isinstance(static_mesh, unreal.StaticMesh):
        fail(f"Missing plane mesh: {PLANE_PATH}")
    camera_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaCameraActor")
    camera_actors = unreal.EditorFilterLibrary.by_class(actor_subsystem.get_all_level_actors(), camera_class)
    if not camera_actors:
        fail("Level00 has no CameraActor for decoration orientation")
    camera = camera_actors[0]
    facing = unreal.MathLibrary.multiply_vector_float(camera.get_actor_forward_vector(), -1.0)
    rotation = unreal.MathLibrary.make_rot_from_zx(facing, unreal.Vector(0.0, 0.0, 1.0))
    center = arena.get_actor_location()
    half_extents = arena.get_editor_property("backdrop_half_extents")
    safe_ratio = profile.get_editor_property("center_safe_zone_ratio")
    safe_x = half_extents.x * safe_ratio.x
    safe_y = half_extents.y * safe_ratio.y
    density = profile.get_editor_property("decoration_density")
    seed = profile.get_editor_property("decoration_seed")
    rng = random.Random(seed)
    ground_palette = list(profile.get_editor_property("ground_detail_palette"))
    mid_palette = list(profile.get_editor_property("mid_decoration_palette"))
    foreground_palette = list(profile.get_editor_property("foreground_palette"))
    if not ground_palette and not mid_palette and not foreground_palette:
        fail("SceneProfile has no decoration materials")
    removed = destroy_generated(actor_subsystem)
    count = max(6, round(8 + density * 28))
    spawned = []
    for index in range(count):
        for _ in range(100):
            x = rng.uniform(-half_extents.x * 0.92, half_extents.x * 0.92)
            y = rng.uniform(-half_extents.y * 0.92, half_extents.y * 0.92)
            if abs(x) >= safe_x or abs(y) >= safe_y:
                break
        palette = ground_palette if index < count // 3 and ground_palette else mid_palette
        if not palette:
            palette = ground_palette or foreground_palette
        material = rng.choice(palette)
        height = rng.choice((120.0, 160.0, 210.0)) if palette is ground_palette else rng.choice((190.0, 250.0, 320.0))
        world = unreal.Vector(center.x + x, center.y + y, arena.get_editor_property("gameplay_plane_z") + height * 0.45)
        priority = footpoint_priority(arena, world.x, world.y, center)
        spawned.append(spawn_card(actor_subsystem, static_mesh, material, world, rotation, height, priority, index))
    for foreground_index, material in enumerate(foreground_palette[:2]):
        x_sign = -1.0 if foreground_index == 0 else 1.0
        height = 420.0
        world = unreal.Vector(
            center.x + x_sign * half_extents.x * 0.88,
            center.y - half_extents.y * 0.82,
            arena.get_editor_property("gameplay_plane_z") + height * 0.45,
        )
        spawned.append(
            spawn_card(actor_subsystem, static_mesh, material, world, rotation, height, 80 + foreground_index, count + foreground_index)
        )
    if not level_subsystem.save_current_level():
        fail("Could not save Level00 after baking generated decorations")
    unreal.log(
        f"[Plan52] Baked {len(spawned)} deterministic decorations for {profile.get_editor_property('scene_id')} "
        f"after replacing {removed} prior tagged actors"
    )
else:
    fail(f"Unsupported Plan52Operation: {operation}")
