"""Make the isolated Echo birth circle face world up without rebuilding its other authored tuning."""

import unreal


SYSTEM_PATH = "/Game/VFX/Echo/Particle/NS_Echo_Born"

system = unreal.EditorAssetLibrary.load_asset(SYSTEM_PATH)
if not isinstance(system, unreal.NiagaraSystem):
    raise RuntimeError(f"Echo Born Niagara failed to load: {SYSTEM_PATH}")
if not unreal.ReEchoCombatVfxComponent.set_niagara_system_sprite_facing_owner_up(system):
    raise RuntimeError(f"Echo Born exposes no ground-facing Sprite Renderer: {SYSTEM_PATH}")
if not unreal.ReEchoCombatVfxComponent.compile_niagara_system_and_wait(system):
    raise RuntimeError(f"Echo Born Niagara compilation did not finish: {SYSTEM_PATH}")
if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Echo Born Niagara: {SYSTEM_PATH}")

unreal.log(f"PLAN142_ECHO_BORN_GROUND_FACING_COMPLETE system={system.get_path_name()}")
