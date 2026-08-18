import unreal


PREFABS = (
    "/Game/ReEcho/Gameplay/CharacterPrefabs/BP_PlayerGameplay",
    "/Game/ReEcho/Gameplay/CharacterPrefabs/BP_EnemyGameplay_Grunt",
    "/Game/ReEcho/Gameplay/CharacterPrefabs/BP_EnemyGameplay_Rabbit",
    "/Game/ReEcho/Gameplay/CharacterPrefabs/BP_EnemyGameplay_Goat",
    "/Game/ReEcho/Gameplay/CharacterPrefabs/BP_EnemyGameplay_Fox",
)


for prefab_path in PREFABS:
    blueprint = unreal.EditorAssetLibrary.load_asset(prefab_path)
    if not isinstance(blueprint, unreal.Blueprint):
        raise RuntimeError(f"Missing Gameplay CharacterPrefab Blueprint: {prefab_path}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save compiled Gameplay CharacterPrefab: {prefab_path}")

unreal.log(f"Compiled and saved {len(PREFABS)} Gameplay CharacterPrefab Blueprints")
