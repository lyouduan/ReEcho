import unreal


PROFILE_ROOT = "/Game/ReEcho/DataAsset/Weapon/Profiles"
VISUAL_KEYS = ("CrescentBlade", "Scythe", "Whip", "Bow", "Gun", "Staff")
CANONICAL_CHARACTER_HEIGHT_CM = 224.0


def load_profile(visual_key):
    path = f"{PROFILE_ROOT}/DA_WeaponPresentation_{visual_key}"
    profile = unreal.EditorAssetLibrary.load_asset(path)
    if not profile:
        raise RuntimeError(f"Missing weapon presentation profile: {path}")
    return profile


def preserve_current_length_as_absolute(profile):
    if profile.get_editor_property("override_held_length"):
        length_cm = max(float(profile.get_editor_property("held_length_override_cm")), 1.0)
    else:
        ratio = max(float(profile.get_editor_property("held_length_ratio")), 0.01)
        length_cm = CANONICAL_CHARACTER_HEIGHT_CM * ratio
    profile.set_editor_property("held_length_override_cm", length_cm)
    profile.set_editor_property("override_held_length", True)


def configure_sword_slash(profile):
    slot = profile.get_editor_property("attack_committed")
    offset = slot.get_editor_property("offset")
    translation = offset.get_editor_property("translation")
    rotation = offset.get_editor_property("rotation").rotator()
    scale = offset.get_editor_property("scale3d")
    translation.z = 60.0
    rotation.roll = -45.0
    slot.set_editor_property(
        "offset", unreal.Transform(location=translation, rotation=rotation, scale=scale)
    )
    slot.set_editor_property("preserve_world_size", True)
    profile.set_editor_property("attack_committed", slot)


for key in VISUAL_KEYS:
    weapon_profile = load_profile(key)
    preserve_current_length_as_absolute(weapon_profile)
    if key == "CrescentBlade":
        configure_sword_slash(weapon_profile)
    if not unreal.EditorAssetLibrary.save_loaded_asset(weapon_profile, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save weapon profile for {key}")

unreal.log("PLAN100_WEAPON_PRESENTATION_RESULT profiles=6 sword_z=60 sword_roll=-45")
