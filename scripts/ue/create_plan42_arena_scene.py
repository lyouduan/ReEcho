import unreal


LEVEL_PATH = "/Game/Level00"
MAP_TEXTURE_PATH = "/Game/ReEcho/Art/Scene/map01.map01"
ACTOR_LABEL = "ArenaScene_Map01"


def fail(message):
    raise RuntimeError(f"[Plan42] {message}")


level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level_subsystem.load_level(LEVEL_PATH):
    fail(f"Could not load {LEVEL_PATH}")

arena_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaSceneActor")
if not arena_class:
    fail("Native ReEchoArenaSceneActor class is unavailable; build the Editor target first")

arenas = [actor for actor in actor_subsystem.get_all_level_actors() if actor.get_class() == arena_class]
if len(arenas) > 1:
    fail(f"Expected at most one Arena Scene actor before authoring, found {len(arenas)}")

arena = arenas[0] if arenas else actor_subsystem.spawn_actor_from_class(
    arena_class, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0)
)
if not arena:
    fail("Could not spawn Arena Scene actor")

map_texture = unreal.load_asset(MAP_TEXTURE_PATH)
if not map_texture:
    fail(f"Could not load {MAP_TEXTURE_PATH}")

arena.set_actor_label(ACTOR_LABEL)
arena.set_editor_property("map_texture", map_texture)
if not level_subsystem.save_current_level():
    fail(f"Could not save {LEVEL_PATH}")

unreal.log(f"[Plan42] Authored {ACTOR_LABEL} in {LEVEL_PATH} with {MAP_TEXTURE_PATH}")
