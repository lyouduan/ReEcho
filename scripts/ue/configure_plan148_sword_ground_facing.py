"""Make every production longsword slash renderer follow one ground-plane basis."""

import unreal


PROFILE_PATH = "/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_CrescentBlade"
ELEMENT_SYSTEM_PATHS = (
    "/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_Fire",
    "/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_Thunder",
    "/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_Grass",
    "/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_Water",
)

profile = unreal.EditorAssetLibrary.load_asset(PROFILE_PATH)
if not isinstance(profile, unreal.ReEchoWeaponPresentationProfile):
    raise RuntimeError(f"Missing longsword presentation profile: {PROFILE_PATH}")

slot = profile.get_editor_property("attack_committed")
default_system = slot.get_editor_property("system")
if not isinstance(default_system, unreal.NiagaraSystem):
    default_system = default_system.load_synchronous()
if not isinstance(default_system, unreal.NiagaraSystem):
    raise RuntimeError("Longsword AttackCommitted Niagara is not configured")

default_system_path = default_system.get_path_name().split(".", 1)[0]
systems = [(default_system_path, default_system)]
for asset_path in ELEMENT_SYSTEM_PATHS:
    element_system = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(element_system, unreal.NiagaraSystem):
        raise RuntimeError(f"Missing element longsword Niagara: {asset_path}")
    systems.append((asset_path, element_system))

for system_path, sword_system in systems:
    if not unreal.ReEchoCombatVfxComponent.configure_melee_niagara_component_facing(
        sword_system
    ):
        raise RuntimeError(
            f"Longsword Niagara requires enabled emitters and a mesh renderer: {system_path}"
        )
    if not unreal.ReEchoCombatVfxComponent.set_niagara_system_sprite_facing_owner_up(
        sword_system
    ):
        raise RuntimeError(
            f"Longsword Niagara has no configurable sprite renderer: {system_path}"
        )
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        sword_system, only_if_is_dirty=False
    ):
        raise RuntimeError(f"Could not save longsword Niagara: {system_path}")
    unreal.log(
        "PLAN148_SWORD_GROUND_FACING "
        f"system={system_path} local_space=true mesh_facing=Default "
        "sprite_facing=CustomFacingVector normal=User.GroundNormal "
        "tangent=User.GroundTangent"
    )
