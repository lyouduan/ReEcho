"""Audit the authored scythe slash scale against its Niagara system bounds."""

import unreal


PROFILE_PATH = "/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_Scythe"
profile = unreal.EditorAssetLibrary.load_asset(PROFILE_PATH)
if not isinstance(profile, unreal.ReEchoWeaponPresentationProfile):
    raise RuntimeError(f"Missing scythe profile: {PROFILE_PATH}")

slot = profile.get_editor_property("attack_committed")
system = slot.get_editor_property("system")
if not isinstance(system, unreal.NiagaraSystem):
    system = system.load_synchronous()
if not isinstance(system, unreal.NiagaraSystem):
    raise RuntimeError("Missing scythe AttackCommitted Niagara")

offset = slot.get_editor_property("offset")
bounds = system.get_editor_property("fixed_bounds")
fixed_bounds_enabled = None
for property_name in ("b_fixed_bounds", "fixed_bounds_enabled"):
    try:
        fixed_bounds_enabled = system.get_editor_property(property_name)
        break
    except Exception:
        pass
unreal.log(
    "PLAN148_SCYTHE_RANGE_AUDIT "
    f"profile={PROFILE_PATH} system={system.get_path_name()} "
    f"offset_location={offset.get_editor_property('translation')} "
    f"offset_rotation={offset.get_editor_property('rotation').rotator()} "
    f"offset_scale={offset.get_editor_property('scale3d')} fixed_bounds={bounds} "
    f"fixed_bounds_enabled={fixed_bounds_enabled}"
)
