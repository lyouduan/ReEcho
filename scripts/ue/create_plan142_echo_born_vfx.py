"""Create and audit the one-shot Echo birth circle from the existing Goat ground-circle system."""

import unreal


SOURCE_PATH = "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill03_Alarming"
DESTINATION_PATH = "/Game/VFX/Echo/Particle/NS_Echo_Born"
SOURCE_MATERIALS = ("BaseVFX003_Inst15", "BaseVFX003_Inst21", "BaseVFX003_Inst22", "BaseVFX003_Inst23")
# Niagara particle curves own the ice-blue hue. A mild neutral HDR multiplier makes the circle materials readable.
ECHO_CYAN = unreal.LinearColor(1.35, 1.35, 1.35, 1.0)


source = unreal.EditorAssetLibrary.load_asset(SOURCE_PATH)
if not isinstance(source, unreal.NiagaraSystem):
    raise RuntimeError(f"Echo Born source Niagara failed to load: {SOURCE_PATH}")

if unreal.EditorAssetLibrary.does_asset_exist(DESTINATION_PATH):
    system = unreal.EditorAssetLibrary.load_asset(DESTINATION_PATH)
else:
    if not unreal.EditorAssetLibrary.duplicate_asset(SOURCE_PATH, DESTINATION_PATH):
        raise RuntimeError(f"Could not duplicate {SOURCE_PATH} to {DESTINATION_PATH}")
    system = unreal.EditorAssetLibrary.load_asset(DESTINATION_PATH)

if not isinstance(system, unreal.NiagaraSystem):
    raise RuntimeError(f"Echo Born Niagara failed to load: {DESTINATION_PATH}")
for source_name in SOURCE_MATERIALS:
    source_path = f"/Game/VFX/Monster/Goat/MI/{source_name}"
    suffix = source_name.removeprefix("BaseVFX003_")
    destination_path = f"/Game/VFX/Echo/MI/MI_Echo_Born_{suffix}"
    if not unreal.EditorAssetLibrary.does_asset_exist(destination_path):
        if not unreal.EditorAssetLibrary.duplicate_asset(source_path, destination_path):
            raise RuntimeError(f"Could not duplicate Echo Born material {source_path}")
    material = unreal.EditorAssetLibrary.load_asset(destination_path)
    if not isinstance(material, unreal.MaterialInstanceConstant):
        raise RuntimeError(f"Echo Born material failed to load: {destination_path}")
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(material, "基础颜色", ECHO_CYAN)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(material, "溶解亮边", ECHO_CYAN)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(material, "FresnelColor", ECHO_CYAN)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save Echo Born material: {destination_path}")

if not unreal.ReEchoCombatVfxComponent.set_echo_born_renderer_materials(system):
    raise RuntimeError(f"Failed to bind independent Echo Born materials: {DESTINATION_PATH}")
if not unreal.ReEchoCombatVfxComponent.set_echo_born_particle_colors(system):
    raise RuntimeError(f"Echo Born Niagara exposes no editable particle colors: {DESTINATION_PATH}")
if not unreal.ReEchoCombatVfxComponent.set_echo_born_mesh_height_scale(system):
    raise RuntimeError(f"Echo Born Niagara exposes no editable mesh height scale: {DESTINATION_PATH}")
if not unreal.ReEchoCombatVfxComponent.set_niagara_system_sprite_facing_owner_up(system):
    raise RuntimeError(f"Could not bind Echo Born sprites to the ground normal: {DESTINATION_PATH}")
if not unreal.ReEchoCombatVfxComponent.set_niagara_system_emitters_local_space(system):
    raise RuntimeError(f"Could not make Echo Born emitters follow the Echo ground anchor: {DESTINATION_PATH}")
if not unreal.ReEchoCombatVfxComponent.compile_niagara_system_and_wait(system):
    raise RuntimeError(f"Echo Born Niagara compilation did not finish: {DESTINATION_PATH}")
if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Echo Born Niagara: {DESTINATION_PATH}")

registry = unreal.AssetRegistryHelpers.get_asset_registry()
options = unreal.AssetRegistryDependencyOptions(
    include_soft_package_references=True,
    include_hard_package_references=True,
    include_searchable_names=True,
    include_soft_management_references=True,
    include_hard_management_references=True,
)
dependencies = [str(dependency) for dependency in registry.get_dependencies(DESTINATION_PATH, options) or []]
for dependency in dependencies:
    unreal.log(f"PLAN142_ECHO_BORN dependency={dependency}")

for source_name in SOURCE_MATERIALS:
    source_material = f"/Game/VFX/Monster/Goat/MI/{source_name}"
    echo_material = f"/Game/VFX/Echo/MI/MI_Echo_Born_{source_name.removeprefix('BaseVFX003_')}"
    if source_material in dependencies:
        raise RuntimeError(f"Echo Born still references shared Goat material: {source_material}")
    if echo_material not in dependencies:
        raise RuntimeError(f"Echo Born does not reference its independent material: {echo_material}")

unreal.log(
    f"PLAN142_ECHO_BORN_COMPLETE source={source.get_path_name()} destination={system.get_path_name()}"
)
