"""Restore Echo Born mesh size in its authored local YZ plane and flatten only local X."""

import unreal


SYSTEM_PATH = "/Game/VFX/Echo/Particle/NS_Echo_Born"

system = unreal.EditorAssetLibrary.load_asset(SYSTEM_PATH)
if not isinstance(system, unreal.NiagaraSystem):
    raise RuntimeError(f"Echo Born Niagara failed to load: {SYSTEM_PATH}")
if not unreal.ReEchoCombatVfxComponent.set_echo_born_mesh_height_scale(system):
    raise RuntimeError(f"Echo Born exposes no editable mesh-scale parameter: {SYSTEM_PATH}")
if not unreal.ReEchoCombatVfxComponent.compile_niagara_system_and_wait(system):
    raise RuntimeError(f"Echo Born Niagara compilation did not finish: {SYSTEM_PATH}")
if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Echo Born Niagara: {SYSTEM_PATH}")

unreal.log(
    "PLAN142_ECHO_BORN_MESH_PLANE_SCALE_COMPLETE "
    f"system={system.get_path_name()} authored_x=25 authored_y=25 authored_z=40"
)
