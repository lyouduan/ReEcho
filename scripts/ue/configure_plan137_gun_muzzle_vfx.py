import unreal


PROFILE_PATH = "/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_Gun"
SPARK_PATH = "/Game/VFX/People/Bullet/Particle/NS_People_Bullet_spark"


profile = unreal.EditorAssetLibrary.load_asset(PROFILE_PATH)
spark = unreal.EditorAssetLibrary.load_asset(SPARK_PATH)
if not isinstance(profile, unreal.ReEchoWeaponPresentationProfile):
    raise RuntimeError(f"Missing Gun weapon presentation profile: {PROFILE_PATH}")
if not isinstance(spark, unreal.NiagaraSystem):
    raise RuntimeError(f"Missing gun muzzle Niagara: {SPARK_PATH}")

original_travel = profile.get_editor_property("travel")
source_slot = profile.get_editor_property("damage_applied")
if not source_slot.get_editor_property("enabled"):
    source_slot = profile.get_editor_property("attack_committed")

source_system = source_slot.get_editor_property("system")
if not source_system or source_system.get_path_name() != spark.get_path_name():
    raise RuntimeError(
        f"Gun spark migration source mismatch: expected={spark.get_path_name()} actual={source_system}"
    )

source_slot.set_editor_property("enabled", True)
source_slot.set_editor_property("system", spark)
source_slot.set_editor_property("spawn_mode", unreal.ReEchoWeaponVfxSpawnMode.ATTACH_TO_ATTACK_ROOT)
profile.set_editor_property("attack_committed", source_slot)
profile.set_editor_property("damage_applied", unreal.ReEchoWeaponVfxSlot())
profile.set_editor_property("override_attack_vfx_anchor", True)
profile.set_editor_property("attack_vfx_anchor_ratio", unreal.Vector2D(0.5, 0.0))

if not unreal.ReEchoCombatVfxComponent.set_niagara_system_emitters_local_space(spark):
    raise RuntimeError("Gun muzzle Niagara has no enabled emitter to configure as Local Space")
if not unreal.ReEchoCombatVfxComponent.bind_niagara_sprite_rotation_to_direction_parameter(spark):
    raise RuntimeError("Gun muzzle Niagara has no enabled Sprite renderer to bind to the direction parameter")

if str(profile.get_editor_property("travel")) != str(original_travel):
    raise RuntimeError("Gun muzzle migration changed the projectile Travel slot")
if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
    raise RuntimeError(f"Could not save Gun weapon presentation profile: {PROFILE_PATH}")
if not unreal.EditorAssetLibrary.save_loaded_asset(spark, only_if_is_dirty=False):
    raise RuntimeError(f"Could not save gun muzzle Niagara: {SPARK_PATH}")

unreal.log(
    "PLAN137_GUN_MUZZLE_RESULT "
    f"profile={profile.get_path_name()} system={spark.get_path_name()} "
    f"anchor_override={profile.get_editor_property('override_attack_vfx_anchor')} "
    f"anchor={profile.get_editor_property('attack_vfx_anchor_ratio')} "
    "local_space=true sprite_rotation=User.DirectionSpriteRotationDegrees"
)
