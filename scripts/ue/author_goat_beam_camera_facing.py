"""Keep the planar Goat Skill04 beam readable from front and side views."""

import unreal


ASSET_PATH = "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill04_Lighting"

system = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
if not isinstance(system, unreal.NiagaraSystem):
    raise RuntimeError(f"Goat beam Niagara failed to load: {ASSET_PATH}")
if not unreal.ReEchoCombatVfxComponent.set_niagara_system_mesh_facing_camera_plane(system):
    raise RuntimeError(f"Goat beam Niagara has no enabled mesh renderer: {ASSET_PATH}")
if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Goat beam Niagara: {ASSET_PATH}")
unreal.log(f"GOAT_BEAM_CAMERA_FACING_OK asset={ASSET_PATH}")
