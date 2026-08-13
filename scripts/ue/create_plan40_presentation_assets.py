import unreal


ROOT = "/Game/ReEcho/Animation2D"


def load(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError(f"Required Plan40 asset is missing: {path}")
    return asset


def find_flipbook(*keywords):
    matches = []
    for asset_path in unreal.EditorAssetLibrary.list_assets(
        "/Game/ReEcho/Art/Animation2D/Enemies/Fox/Flipbooks", recursive=True
    ):
        lowered = asset_path.lower()
        if all(keyword.lower() in lowered for keyword in keywords):
            asset = unreal.EditorAssetLibrary.load_asset(asset_path)
            if isinstance(asset, unreal.PaperFlipbook):
                matches.append(asset)
    if len(matches) != 1:
        raise RuntimeError(
            f"Expected exactly one Flipbook matching {keywords}, found "
            f"{[asset.get_path_name() for asset in matches]}"
        )
    return matches[0]


def get_or_create_data_asset(name, asset_class):
    path = f"{ROOT}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        existing = unreal.EditorAssetLibrary.load_asset(path)
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
    "J_SPADE": "/Game/ReEcho/Art/Animation2D/Players/Spade/Walk/Textures/Idel_01.Idel_01",
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
walk = load("/Game/ReEcho/Art/Animation2D/Players/Spade/Flipbooks/Walk.Walk")
attack = load("/Game/ReEcho/Art/Animation2D/Players/Spade/Flipbooks/Attack.Attack")

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
    "clips",
    {tag("Animation.Idle"): clip(load("/Game/ReEcho/Art/Animation2D/Enemies/Grunt/Flipbooks/Default.Default"), True)},
)
grunt.set_editor_property("animation_sets", [grunt_set])

enemy_profiles = [grunt]
for asset_name, appearance_id, texture_path, flipbook_path in (
    (
        "DA_Enemy_RabbitDoll",
        "Enemy.Rabbit",
        None,
        "/Game/ReEcho/Art/Animation2D/Enemies/Rabbit/Flipbooks/Default.Default",
    ),
    (
        "DA_Enemy_GoatPriest",
        "Enemy.Goat",
        None,
        "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Default.Default",
    ),
):
    enemy_profile = get_or_create_data_asset(asset_name, unreal.ReEcho2DCharacterPresentationProfile)
    enemy_profile.set_editor_property("appearance_id", appearance_id)
    enemy_profile.set_editor_property("static_fallback", load(texture_path) if texture_path else None)
    enemy_set = unreal.ReEcho2DCompositeAnimationSet()
    enemy_set.set_editor_property("weapon_visual_set_id", "")
    enemy_flipbook = load(flipbook_path)
    enemy_set.set_editor_property(
        "clips",
        {tag("Animation.Idle"): clip(enemy_flipbook, True)},
    )
    enemy_profile.set_editor_property("animation_sets", [enemy_set])
    enemy_profiles.append(enemy_profile)

fox = get_or_create_data_asset("DA_Enemy_Fox", unreal.ReEcho2DCharacterPresentationProfile)
fox.set_editor_property("appearance_id", "Enemy.Fox")
fox.set_editor_property("static_fallback", None)
fox_set = unreal.ReEcho2DCompositeAnimationSet()
fox_set.set_editor_property("weapon_visual_set_id", "")
fox_set.set_editor_property(
    "clips",
    {
        tag("Animation.Idle"): clip(find_flipbook("fox", "walk"), True),
        tag("Animation.Attack.Basic"): clip(find_flipbook("fox", "attack"), False, True),
    },
)
fox.set_editor_property("animation_sets", [fox_set])
enemy_profiles.append(fox)

for asset in profiles + [catalog] + enemy_profiles:
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)

unreal.log(
    f"Plan40 presentation assets saved: {len(profiles)} player profiles + "
    f"{len(enemy_profiles)} enemy profiles + catalog"
)
