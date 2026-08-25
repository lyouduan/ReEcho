import unreal


PROFILE_PATH = (
    "/Game/ReEcho/DataAsset/Weapon/Profiles/"
    "DA_WeaponPresentation_CrescentBlade.DA_WeaponPresentation_CrescentBlade"
)

profile = unreal.load_object(None, PROFILE_PATH)
if not profile:
    raise RuntimeError(f"Missing sword weapon profile: {PROFILE_PATH}")

slot = profile.get_editor_property("attack_committed")
system = slot.get_editor_property("system")
if not system:
    raise RuntimeError("Sword AttackCommitted Niagara is not configured")

if not unreal.ReEchoCombatVfxComponent.set_niagara_system_emitters_local_space(system):
    raise RuntimeError(f"Could not enable Local Space for {system.get_path_name()}")
if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
    raise RuntimeError(f"Could not save {system.get_path_name()}")

unreal.log(f"PLAN100_SWORD_DIRECTION_RESULT system={system.get_path_name()} local_space=true")
