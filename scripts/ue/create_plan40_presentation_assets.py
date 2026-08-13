import unreal


ROOT = "/Game/ReEcho/Animation2D"


def load(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError(f"Required Plan40 asset is missing: {path}")
    return asset


def get_or_create_data_asset(name, asset_class):
    path = f"{ROOT}/{name}"
    existing = unreal.EditorAssetLibrary.load_asset(path)
    if existing:
        if not isinstance(existing, asset_class):
            raise RuntimeError(f"Asset exists with wrong class: {path}")
        return existing
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", asset_class)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, ROOT, asset_class, factory)


def tag(name):
    value = unreal.ReEcho2DPresentationCatalog.resolve_semantic_tag(name)
    if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(value):
        raise RuntimeError(f"Plan40 semantic GameplayTag is not registered: {name}")
    return value


def clip(flipbook, looping, restart=False):
    value = unreal.ReEcho2DAnimationClip()
    value.set_editor_property("flipbook", flipbook)
    value.set_editor_property("looping", looping)
    value.set_editor_property("restart_on_request", restart)
    value.set_editor_property("play_rate", 1.0)
    value.set_editor_property("use_native_scale", True)
    value.set_editor_property("world_height", 224.0)
    value.set_editor_property("translucent_sort_priority", 10)
    return value


unreal.EditorAssetLibrary.make_directory(ROOT)

appearance_textures = {
    "J_CAT": "/Game/ReEcho/Textures/Characters/NewCast/Player_Cat.Player_Cat",
    "J_HEART": "/Game/ReEcho/Textures/Characters/NewCast/Player_Heart.Player_Heart",
    "J_SPADE": "/Game/2DAnim/Player/Idel_01.Idel_01",
    "J_CLOVER": "/Game/ReEcho/Textures/Characters/NewCast/Player_Clover.Player_Clover",
    "J_DIAMOND": "/Game/ReEcho/Textures/Characters/NewCast/Player_Diamond.Player_Diamond",
}

profiles = []
for appearance_id, texture_path in appearance_textures.items():
    profile = get_or_create_data_asset(
        f"DA_Character_{appearance_id}", unreal.ReEcho2DCharacterPresentationProfile
    )
    profile.set_editor_property("appearance_id", appearance_id)
    profile.set_editor_property("static_fallback", load(texture_path))
    profile.set_editor_property("animation_sets", [])
    profiles.append(profile)

spade = next(profile for profile in profiles if str(profile.get_editor_property("appearance_id")) == "J_SPADE")
walk = load("/Game/2DAnim/Flipbook/walk.walk")
attack = load("/Game/2DAnim/Flipbook/attack.attack")

base_set = unreal.ReEcho2DCompositeAnimationSet()
base_set.set_editor_property("weapon_visual_set_id", "")
base_set.set_editor_property("clips", {tag("Animation.Move"): clip(walk, True)})

moon_staff_set = unreal.ReEcho2DCompositeAnimationSet()
moon_staff_set.set_editor_property("weapon_visual_set_id", "MoonStaff")
moon_staff_set.set_editor_property("clips", {tag("Animation.Attack.Basic"): clip(attack, False, True)})
spade.set_editor_property("animation_sets", [base_set, moon_staff_set])

catalog = get_or_create_data_asset("DA_PresentationCatalog", unreal.ReEcho2DPresentationCatalog)
catalog.set_editor_property("character_profiles", profiles)

grunt = get_or_create_data_asset("DA_Enemy_Grunt", unreal.ReEcho2DCharacterPresentationProfile)
grunt.set_editor_property("appearance_id", "Enemy.Grunt")
grunt.set_editor_property("static_fallback", None)
grunt_set = unreal.ReEcho2DCompositeAnimationSet()
grunt_set.set_editor_property("weapon_visual_set_id", "")
grunt_set.set_editor_property(
    "clips", {tag("Animation.Idle"): clip(load("/Game/2DAnim/Flipbook/01_2.01_2"), True)}
)
grunt.set_editor_property("animation_sets", [grunt_set])

for asset in profiles + [catalog, grunt]:
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)

unreal.log(f"Plan40 presentation assets saved: {len(profiles)} player profiles + Grunt + catalog")
