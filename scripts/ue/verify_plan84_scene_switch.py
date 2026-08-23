"""Verify Plan84 Arena assets, with an optional isolated author-preservation probe."""

import importlib.util
from pathlib import Path
import unreal


PREFAB_ROOT = "/Game/ReEcho/Scene/Prefabs"
ART_ROOT = "/Game/ReEcho/Art/Scene/SC01/EdgeInserts"
MAP_ROOT = "/Game/ReEcho/Art/Scene/Map"
PROFILE_ROOT = "/Game/ReEcho/Scene/Profiles"
SCENES = ("SC01", "SC02", "SC03", "SC04")
COMPONENTS = (
    "Edge_TopLeaves_Mid", "Edge_BottomFrame_Foreground", "Edge_BottomGrassLeft_Foreground",
    "Edge_BottomGrassRight_Foreground", "Edge_MidRight_Mid", "Edge_MidLeftInsert_Mid",
    "Edge_MidLeftLargeLeaves_Mid",
)
MID_PRIORITIES = {
    "Edge_TopLeaves_Mid": 55,
    "Edge_MidLeftInsert_Mid": 57,
    "Edge_MidRight_Mid": 58,
    "Edge_MidLeftLargeLeaves_Mid": 59,
}
FOREGROUND_PRIORITIES = {
    "Edge_BottomFrame_Foreground": 90,
    "Edge_BottomGrassLeft_Foreground": 92,
    "Edge_BottomGrassRight_Foreground": 93,
}


def fail(message):
    raise RuntimeError(f"[Plan84] {message}")


def component_entries(blueprint):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    result = {}
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(data)
        if obj is not None:
            name = str(obj.get_name()).removesuffix("_GEN_VARIABLE")
            if name in result and result[name].get_path_name() != obj.get_path_name():
                fail(f"Duplicate component {name} in {blueprint.get_name()}")
            result[name] = (handle, obj)
    return subsystem, result


def component_names(blueprint):
    _, items = component_entries(blueprint)
    return {name: item[1] for name, item in items.items()}


def load_author_module():
    path = Path(unreal.Paths.project_dir()) / "scripts" / "ue" / "author_plan84_scene_switch.py"
    spec = importlib.util.spec_from_file_location("plan84_author_probe", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def run_preservation_probe():
    temp_root = "/Game/ReEcho/Tests/Plan84Temp"
    temp_path = f"{temp_root}/BP_ArenaScene_SC01_AuthorProbe"
    unreal.EditorAssetLibrary.make_directory(temp_root)
    if unreal.EditorAssetLibrary.does_asset_exist(temp_path):
        unreal.EditorAssetLibrary.delete_asset(temp_path)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.ReEchoArenaSceneActor)
    probe = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "BP_ArenaScene_SC01_AuthorProbe", temp_root, unreal.Blueprint, factory
    )
    if not isinstance(probe, unreal.Blueprint):
        fail("Could not create isolated author-preservation probe Blueprint")
    author = load_author_module()
    material_map = {
        spec[3]: unreal.EditorAssetLibrary.load_asset(f"{author.MATERIAL_ROOT}/{spec[2]}")
        for spec in author.INSERTS
    }
    preserve_name = "Edge_TopLeaves_Mid"
    missing_name = "Edge_MidLeftInsert_Mid"
    original = None
    try:
        author.author_direct_components(probe, material_map)
        unreal.BlueprintEditorLibrary.compile_blueprint(probe)
        _, initially_authored = component_entries(probe)
        missing_after_first_author = set(COMPONENTS).difference(initially_authored)
        if missing_after_first_author:
            fail(f"Author did not create missing insert components: {sorted(missing_after_first_author)}")
        initial_components = {name: item[1] for name, item in initially_authored.items()}
        initial_mid_priorities = [
            initial_components[name].get_editor_property("translucency_sort_priority")
            for name in MID_PRIORITIES
        ]
        initial_foreground_priorities = [
            initial_components[name].get_editor_property("translucency_sort_priority")
            for name in FOREGROUND_PRIORITIES
        ]
        if min(initial_mid_priorities) <= 50:
            fail("Newly authored Mid defaults do not sort above the character ceiling")
        if min(initial_foreground_priorities) <= max(initial_mid_priorities):
            fail("Newly authored Foreground defaults do not sort above all Mid defaults")

        _, layout_entries = component_entries(probe)
        moved_wall_location = unreal.Vector(111.0, 222.0, 333.0)
        layout_entries["WallNorth"][1].set_editor_property("relative_location", moved_wall_location)
        probe_profile = unreal.EditorAssetLibrary.load_asset(f"{PROFILE_ROOT}/DA_ArenaScene_SC01")
        probe_defaults = unreal.get_default_object(probe.generated_class())
        layout_bounds = {
            name: probe_defaults.get_editor_property(name)
            for name in (
                "camera_clamp_half_extents",
                "player_half_extents",
                "enemy_spawn_half_extents",
            )
        }
        author.configure_arena_editor_defaults(probe, probe_profile)
        _, layout_after = component_entries(probe)
        if layout_after["WallNorth"][1].get_editor_property("relative_location") != moved_wall_location:
            fail("Construction/author rerun snapped the artist-moved WallNorth back")
        probe_defaults = unreal.get_default_object(probe.generated_class())
        for name, expected in layout_bounds.items():
            if probe_defaults.get_editor_property(name) != expected:
                fail(f"Wall authoring implicitly changed {name}")

        probe_defaults = unreal.get_default_object(probe.generated_class())
        independent_bounds = {
            name: probe_defaults.get_editor_property(name)
            for name in (
                "camera_clamp_half_extents",
                "player_half_extents",
                "enemy_spawn_half_extents",
            )
        }
        probe_defaults.set_editor_property("backdrop_half_extents", unreal.Vector2D(1777.0, 2888.0))
        for name, expected in independent_bounds.items():
            if probe_defaults.get_editor_property(name) != expected:
                fail(f"Editing BackdropHalfExtents implicitly changed {name}")
        if not probe_defaults.get_editor_property("auto_layout_backdrop"):
            fail("SC01 Backdrop size editing is not connected to automatic layout")

        subsystem, items = component_entries(probe)
        preserve = items[preserve_name][1]
        original = {
            "location": preserve.get_editor_property("relative_location"),
            "rotation": preserve.get_editor_property("relative_rotation"),
            "scale": preserve.get_editor_property("relative_scale3d"),
            "visible": preserve.get_editor_property("visible"),
            "material": preserve.get_material(0),
            "priority": preserve.get_editor_property("translucency_sort_priority"),
        }
        custom_location = unreal.Vector(123.0, -456.0, 78.0)
        custom_rotation = unreal.Rotator(11.0, 22.0, 33.0)
        custom_scale = unreal.Vector(1.25, 2.5, 0.75)
        custom_material = material_map["Edge_MidRight_Mid"]
        preserve.set_editor_property("relative_location", custom_location)
        preserve.set_editor_property("relative_rotation", custom_rotation)
        preserve.set_editor_property("relative_scale3d", custom_scale)
        preserve.set_editor_property("visible", False)
        preserve.set_material(0, custom_material)
        preserve.set_translucent_sort_priority(777)
        author.author_direct_components(probe, material_map)
        unreal.BlueprintEditorLibrary.compile_blueprint(probe)
        _, after = component_entries(probe)
        preserved = after[preserve_name][1]
        if preserved.get_editor_property("relative_location") != custom_location:
            fail("Author rerun overwrote artist location")
        if preserved.get_editor_property("relative_rotation") != custom_rotation:
            fail("Author rerun overwrote artist rotation")
        if preserved.get_editor_property("relative_scale3d") != custom_scale:
            fail("Author rerun overwrote artist scale")
        if preserved.get_editor_property("visible"):
            fail("Author rerun overwrote artist visibility")
        if preserved.get_material(0) != custom_material:
            fail("Author rerun overwrote artist material")
        if preserved.get_editor_property("translucency_sort_priority") != 777:
            fail("Author rerun overwrote artist translucency sort priority")
        if missing_name not in after:
            fail("Author rerun did not restore a missing insert component")
    finally:
        unreal.EditorAssetLibrary.delete_asset(temp_path)
        unreal.EditorAssetLibrary.delete_directory(temp_root)


blueprints = {}
for scene_id in SCENES:
    blueprint = unreal.EditorAssetLibrary.load_asset(f"{PREFAB_ROOT}/BP_ArenaScene_{scene_id}")
    if not isinstance(blueprint, unreal.Blueprint):
        fail(f"Missing Arena Blueprint {scene_id}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    blueprints[scene_id] = blueprint
    defaults = unreal.get_default_object(blueprint.generated_class())
    if str(defaults.get_editor_property("scene_id")) != scene_id:
        fail(f"Incorrect SceneId on {scene_id}")
    registrations = defaults.get_editor_property("scene_registry")
    registered_ids = [str(item.get_editor_property("scene_id")) for item in registrations]
    if registered_ids != list(SCENES) or len(set(registered_ids)) != len(SCENES):
        fail(f"Invalid Scene registry on {scene_id}: {registered_ids}")
    if defaults.get_editor_property("auto_layout_collision"):
        fail(f"{scene_id} still enables collision auto-layout")
    texture = unreal.EditorAssetLibrary.load_asset(f"{MAP_ROOT}/{scene_id.lower()}")
    material = unreal.EditorAssetLibrary.load_asset(f"{MAP_ROOT}/Materials/MI_{scene_id}")
    profile = unreal.EditorAssetLibrary.load_asset(f"{PROFILE_ROOT}/DA_ArenaScene_{scene_id}")
    if not isinstance(texture, unreal.Texture2D):
        fail(f"Missing map Texture2D for {scene_id}")
    if not isinstance(material, unreal.MaterialInstanceConstant):
        fail(f"Missing map material instance for {scene_id}")
    if profile is None or profile.get_editor_property("map_material") != material:
        fail(f"SceneProfile to map material mismatch for {scene_id}")
    assigned_texture = unreal.MaterialEditingLibrary.get_material_instance_texture_parameter_value(
        material, "MapTexture"
    )
    if assigned_texture != texture:
        fail(f"Map material to texture mismatch for {scene_id}")
    scene_components = component_names(blueprint)
    if scene_components["Backdrop"].get_material(0) != material:
        fail(f"Backdrop template material mismatch for {scene_id}")
    if scene_components["Floor"].get_material(0) == material:
        fail(f"Floor incorrectly carries the visual map material for {scene_id}")

sc01_components = component_names(blueprints["SC01"])
for name in COMPONENTS:
    component = sc01_components.get(name)
    if not isinstance(component, unreal.StaticMeshComponent):
        fail(f"SC01 missing direct editable component {name}")
    if component.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION:
        fail(f"Collision enabled on {name}")
defaults = unreal.get_default_object(blueprints["SC01"].generated_class())
character_priority_ceiling = defaults.get_editor_property("depth_sort_base_priority") + max(
    defaults.get_editor_property("depth_sort_priority_range").x,
    defaults.get_editor_property("depth_sort_priority_range").y,
)
backdrop = sc01_components.get("Backdrop")
if not isinstance(backdrop, unreal.StaticMeshComponent):
    fail("SC01 has no editable Backdrop component")
if backdrop.get_editor_property("translucency_sort_priority") >= character_priority_ceiling:
    fail("Backdrop does not sort below the complete character priority range")
if min(MID_PRIORITIES.values()) <= character_priority_ceiling:
    fail("Authored Mid defaults do not sort above the maximum character priority")
if min(FOREGROUND_PRIORITIES.values()) <= max(MID_PRIORITIES.values()):
    fail("Authored Foreground defaults do not sort above all Mid defaults")
if "SC01EdgeInserts" in sc01_components:
    fail("SC01 still uses the rejected Child Actor wrapper")
for scene_id in SCENES[1:]:
    leaked = set(COMPONENTS).intersection(component_names(blueprints[scene_id]))
    if leaked:
        fail(f"SC01 inserts leaked into {scene_id}: {sorted(leaked)}")

if "-Plan84MutationProbe" in unreal.SystemLibrary.get_command_line():
    run_preservation_probe()
    unreal.log("[Plan84] Isolated author preservation and missing-component probe passed")

unreal.log("[Plan84] Verification passed: direct SC01 components and unique SC01-SC04 registrations")
