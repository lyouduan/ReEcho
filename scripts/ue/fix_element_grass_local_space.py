import unreal


SYSTEM_PATH = "/Game/VFX/Element/Grass/Particle/NS_Element_Grass"


system = unreal.EditorAssetLibrary.load_asset(SYSTEM_PATH)
if not isinstance(system, unreal.NiagaraSystem):
    raise RuntimeError(f"Missing Grass reaction Niagara system: {SYSTEM_PATH}")
if not unreal.ReEchoCombatVfxComponent.set_niagara_system_emitters_local_space(system):
    raise RuntimeError(f"Grass reaction Niagara has no enabled emitter: {SYSTEM_PATH}")
if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
    raise RuntimeError(f"Could not save Grass reaction Niagara system: {SYSTEM_PATH}")

unreal.log(f"ELEMENT_GRASS_LOCAL_SPACE_OK system={SYSTEM_PATH}")
