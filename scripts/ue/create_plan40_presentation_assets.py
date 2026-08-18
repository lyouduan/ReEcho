import unreal


ROOT = "/Game/ReEcho/Animation2D"
GAMEPLAY_PREFAB_ROOT = "/Game/ReEcho/Gameplay/CharacterPrefabs"


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


def get_or_create_gameplay_prefab(name, parent_class_path, box_extent, ground_z, preview_flipbook):
    path = f"{GAMEPLAY_PREFAB_ROOT}/{name}"
    blueprint = (
        unreal.EditorAssetLibrary.load_asset(path)
        if unreal.EditorAssetLibrary.does_asset_exist(path)
        else None
    )
    if not blueprint:
        unreal.EditorAssetLibrary.make_directory(GAMEPLAY_PREFAB_ROOT)
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.load_class(None, parent_class_path))
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, GAMEPLAY_PREFAB_ROOT, unreal.Blueprint, factory
        )
    if not isinstance(blueprint, unreal.Blueprint):
        raise RuntimeError(f"Asset exists with wrong class: {path}")
    generated_class = unreal.load_class(None, f"{path}.{name}_C")
    if not generated_class:
        raise RuntimeError(f"Gameplay Prefab generated class is unavailable: {path}")
    default_object = unreal.get_default_object(generated_class)
    collision = default_object.get_editor_property("collision")
    collision.set_box_extent(unreal.Vector(*box_extent), False)
    collision_extent = collision.get_unscaled_box_extent()
    default_object.get_editor_property("foot_root").set_editor_property(
        "relative_location", unreal.Vector(0.0, 0.0, -collision_extent.z)
    )
    default_object.get_editor_property("ground_root").set_editor_property(
        "relative_location", unreal.Vector(0.0, 0.0, 0.0)
    )
    flipbook_root = default_object.get_editor_property("flipbook_root")
    flipbook_root.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, 0.0))
    flipbook_root.set_editor_property("relative_scale3d", unreal.Vector(1.0, 1.0, 1.0))
    flipbook_renderer = default_object.get_editor_property("sequence_animation")
    flipbook_renderer.set_flipbook(preview_flipbook)
    flipbook_renderer.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, 0.0))
    flipbook_renderer.set_editor_property("relative_rotation", unreal.Rotator(0.0, 0.0, 0.0))
    flipbook_renderer.set_editor_property("relative_scale3d", unreal.Vector(1.0, 1.0, 1.0))
    flipbook_renderer.set_editor_property("visible", True)
    flipbook_renderer.set_editor_property("hidden_in_game", False)
    return blueprint


def get_or_create_single_frame_flipbook(appearance_id, texture):
    generated_root = f"{ROOT}/Generated/Players/{appearance_id}"
    unreal.EditorAssetLibrary.make_directory(generated_root)
    sprite_path = f"{generated_root}/IdleSprite"
    sprite = (
        unreal.EditorAssetLibrary.load_asset(sprite_path)
        if unreal.EditorAssetLibrary.does_asset_exist(sprite_path)
        else None
    )
    if not sprite:
        sprite_factory = unreal.PaperSpriteFactory()
        sprite = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "IdleSprite", generated_root, unreal.PaperSprite, sprite_factory
        )
    width = texture.blueprint_get_size_x()
    height = texture.blueprint_get_size_y()
    sprite.set_editor_property("source_texture", texture)
    sprite.set_editor_property("source_uv", unreal.Vector2D(0.0, 0.0))
    sprite.set_editor_property("source_dimension", unreal.Vector2D(width, height))
    sprite.set_editor_property("pivot_mode", unreal.SpritePivotMode.CENTER_CENTER)
    if not unreal.ReEcho2DAnimationComponent.rebuild_sprite_asset(sprite):
        raise RuntimeError(f"Could not rebuild generated PaperSprite: {sprite_path}")
    flipbook_path = f"{generated_root}/Idle"
    flipbook = (
        unreal.EditorAssetLibrary.load_asset(flipbook_path)
        if unreal.EditorAssetLibrary.does_asset_exist(flipbook_path)
        else None
    )
    if not flipbook:
        flipbook = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "Idle", generated_root, unreal.PaperFlipbook, unreal.PaperFlipbookFactory()
        )
    key_frame = unreal.PaperFlipbookKeyFrame()
    key_frame.set_editor_property("sprite", sprite)
    key_frame.set_editor_property("frame_run", 1)
    flipbook.set_editor_property("key_frames", [key_frame])
    flipbook.set_editor_property("frames_per_second", 1.0)
    return sprite, flipbook


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
    "J_HEART": "/Game/ReEcho/Textures/Characters/NewCast/Player_Heart.Player_Heart",
    "J_SPADE": "/Game/ReEcho/Art/Animation2D/Players/Spade/Walk/Textures/Idel_01.Idel_01",
    "J_CLOVER": "/Game/ReEcho/Textures/Characters/NewCast/Player_Clover.Player_Clover",
    "J_DIAMOND": "/Game/ReEcho/Textures/Characters/NewCast/Player_Diamond.Player_Diamond",
}

profiles = []
generated_player_assets = []
for appearance_id, texture_path in appearance_textures.items():
    profile = get_or_create_data_asset(
        f"DA_Character_{appearance_id}", unreal.ReEcho2DCharacterPresentationProfile
    )
    profile.set_editor_property("appearance_id", appearance_id)
    profile.set_editor_property("world_height", 100.0)
    texture = load(texture_path)
    sprite, idle_flipbook = get_or_create_single_frame_flipbook(appearance_id, texture)
    generated_player_assets.extend([sprite, idle_flipbook])
    default_set = unreal.ReEcho2DCompositeAnimationSet()
    default_set.set_editor_property("weapon_visual_set_id", "")
    default_set.set_editor_property(
        "clips",
        {
            tag("Animation.Idle"): clip(idle_flipbook, True),
            tag("Animation.Move"): clip(idle_flipbook, True),
            tag("Animation.Attack.Basic"): clip(idle_flipbook, False, True),
            tag("Animation.Hit"): clip(idle_flipbook, False, True),
        },
    )
    profile.set_editor_property("animation_sets", [default_set])
    profiles.append(profile)

spade = next(profile for profile in profiles if str(profile.get_editor_property("appearance_id")) == "J_SPADE")
walk = load("/Game/ReEcho/Art/Animation2D/Players/Spade/Flipbooks/Walk.Walk")
attack = load("/Game/ReEcho/Art/Animation2D/Players/Spade/Flipbooks/Attack.Attack")

base_set = unreal.ReEcho2DCompositeAnimationSet()
base_set.set_editor_property("weapon_visual_set_id", "")
base_set.set_editor_property(
    "clips",
    {
        tag("Animation.Idle"): clip(walk, True),
        tag("Animation.Move"): clip(walk, True),
        tag("Animation.Hit"): clip(walk, False, True),
    },
)

moon_staff_set = unreal.ReEcho2DCompositeAnimationSet()
moon_staff_set.set_editor_property("weapon_visual_set_id", "MoonStaff")
moon_staff_set.set_editor_property("clips", {tag("Animation.Attack.Basic"): clip(attack, False, True)})
spade.set_editor_property("animation_sets", [base_set, moon_staff_set])

catalog = get_or_create_data_asset("DA_PresentationCatalog", unreal.ReEcho2DPresentationCatalog)
catalog.set_editor_property("character_profiles", profiles)

state_machine = get_or_create_data_asset("SM2D_DefaultCharacter", unreal.ReEcho2DAnimationStateMachineAsset)


def state(state_tag, semantic_tag, priority, lock=False, terminal=False):
    definition = unreal.ReEcho2DAnimationStateDefinition()
    definition.set_editor_property("state_tag", tag(state_tag))
    definition.set_editor_property("semantic_key", tag(semantic_tag))
    definition.set_editor_property("interrupt_priority", priority)
    definition.set_editor_property("lock_until_playback_complete", lock)
    definition.set_editor_property("terminal", terminal)
    return definition


state_machine.set_editor_property("initial_state_tag", tag("Animation.Idle"))
state_machine.set_editor_property(
    "states",
    [
        state("Animation.Idle", "Animation.Idle", 0),
        state("Animation.Move", "Animation.Move", 10),
        state("Animation.Attack.Basic", "Animation.Attack.Basic", 40, lock=True),
        state("Animation.Hit", "Animation.Hit", 60, lock=True),
    ],
)
for profile in profiles:
    profile.set_editor_property("state_machine", state_machine)

grunt = get_or_create_data_asset("DA_Enemy_Grunt", unreal.ReEcho2DCharacterPresentationProfile)
grunt.set_editor_property("appearance_id", "Enemy.Grunt")
grunt.set_editor_property("world_height", 80.0)
grunt.set_editor_property("state_machine", state_machine)
grunt_set = unreal.ReEcho2DCompositeAnimationSet()
grunt_set.set_editor_property("weapon_visual_set_id", "")
grunt_flipbook = load("/Game/ReEcho/Art/Animation2D/Enemies/Grunt/Flipbooks/Default.Default")
grunt_set.set_editor_property(
    "clips",
    {
        tag("Animation.Idle"): clip(grunt_flipbook, True),
        tag("Animation.Move"): clip(grunt_flipbook, True),
        tag("Animation.Attack.Basic"): clip(grunt_flipbook, False, True),
        tag("Animation.Hit"): clip(grunt_flipbook, False, True),
    },
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
    enemy_profile.set_editor_property("world_height", 140.0 if appearance_id == "Enemy.Rabbit" else 220.0)
    enemy_profile.set_editor_property("state_machine", state_machine)
    enemy_set = unreal.ReEcho2DCompositeAnimationSet()
    enemy_set.set_editor_property("weapon_visual_set_id", "")
    enemy_flipbook = load(flipbook_path)
    enemy_set.set_editor_property(
        "clips",
        {
            tag("Animation.Idle"): clip(enemy_flipbook, True),
            tag("Animation.Move"): clip(enemy_flipbook, True),
            tag("Animation.Attack.Basic"): clip(enemy_flipbook, False, True),
            tag("Animation.Hit"): clip(enemy_flipbook, False, True),
        },
    )
    enemy_profile.set_editor_property("animation_sets", [enemy_set])
    enemy_profiles.append(enemy_profile)

fox = get_or_create_data_asset("DA_Enemy_Fox", unreal.ReEcho2DCharacterPresentationProfile)
fox.set_editor_property("appearance_id", "Enemy.Fox")
fox.set_editor_property("world_height", 200.0)
fox.set_editor_property("state_machine", state_machine)
fox_set = unreal.ReEcho2DCompositeAnimationSet()
fox_set.set_editor_property("weapon_visual_set_id", "")
fox_set.set_editor_property(
    "clips",
    {
        tag("Animation.Idle"): clip(find_flipbook("fox", "walk"), True),
        tag("Animation.Move"): clip(find_flipbook("fox", "walk"), True),
        tag("Animation.Attack.Basic"): clip(find_flipbook("fox", "attack"), False, True),
        tag("Animation.Hit"): clip(find_flipbook("fox", "walk"), False, True),
    },
)
fox.set_editor_property("animation_sets", [fox_set])
enemy_profiles.append(fox)

spade_clips = spade.get_editor_property("animation_sets")[0].get_editor_property("clips")
spade_idle_flipbook = spade_clips[tag("Animation.Idle")].get_editor_property("flipbook")

gameplay_prefab_assets = [
    get_or_create_gameplay_prefab(
        "BP_PlayerGameplay",
        "/Script/ReEcho.ReEchoPlayerPawn",
        (24.0, 24.0, 50.0),
        -50.0,
        spade_idle_flipbook,
    ),
    get_or_create_gameplay_prefab(
        "BP_EnemyGameplay_Grunt", "/Script/ReEcho.ReEchoEnemyActor", (22.0, 22.0, 40.0), -40.0, grunt_flipbook
    ),
    get_or_create_gameplay_prefab(
        "BP_EnemyGameplay_Rabbit", "/Script/ReEcho.ReEchoEnemyActor", (28.0, 28.0, 70.0), -70.0, load("/Game/ReEcho/Art/Animation2D/Enemies/Rabbit/Flipbooks/Default.Default")
    ),
    get_or_create_gameplay_prefab(
        "BP_EnemyGameplay_Goat", "/Script/ReEcho.ReEchoEnemyActor", (48.0, 48.0, 110.0), -110.0, load("/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Default.Default")
    ),
    get_or_create_gameplay_prefab(
        "BP_EnemyGameplay_Fox", "/Script/ReEcho.ReEchoEnemyActor", (40.0, 40.0, 100.0), -100.0, find_flipbook("fox", "walk")
    ),
]

for asset in gameplay_prefab_assets + generated_player_assets + [state_machine] + profiles + [catalog] + enemy_profiles:
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)

unreal.log(
    f"Plan40 presentation assets saved: {len(profiles)} player profiles + "
    f"{len(enemy_profiles)} enemy profiles + catalog"
)
