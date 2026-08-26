"""Make the Fox windup arrow follow its attack root and retain valid culling bounds."""

import unreal


SYSTEM_PATH = "/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_arrow"

system = unreal.EditorAssetLibrary.load_asset(SYSTEM_PATH)
if not isinstance(system, unreal.NiagaraSystem):
    raise RuntimeError(f"Missing Fox direction Niagara system: {SYSTEM_PATH}")
if not unreal.ReEchoCombatVfxComponent.set_niagara_system_emitters_local_space(system):
    raise RuntimeError(
        f"Failed to set enabled emitters to local space and enable fixed bounds: {SYSTEM_PATH}"
    )
if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Fox direction Niagara system: {SYSTEM_PATH}")
unreal.log(f"FOX_DIRECTION_VISIBILITY_FIX_OK system={SYSTEM_PATH}")
