"""Read-only audit for duplicate AnimationSets in presentation profile assets."""

from collections import defaultdict

import unreal


ROOT = "/Game/ReEcho"


def object_path(value):
    if value is None:
        return "None"
    return value.get_path_name()


def tag_name(tag):
    try:
        return str(tag.get_editor_property("tag_name"))
    except Exception:
        return unreal.GameplayTagLibrary.get_tag_name(tag).to_string()


def clip_fingerprint(clip):
    offset = clip.get_editor_property("local_offset")
    return (
        object_path(clip.get_editor_property("flipbook")),
        bool(clip.get_editor_property("looping")),
        bool(clip.get_editor_property("restart_on_request")),
        float(clip.get_editor_property("play_rate")),
        bool(clip.get_editor_property("use_native_scale")),
        bool(clip.get_editor_property("mirror_horizontally")),
        (float(offset.x), float(offset.y), float(offset.z)),
        int(clip.get_editor_property("translucent_sort_priority")),
    )


def set_fingerprint(animation_set):
    clips = animation_set.get_editor_property("clips")
    return tuple(sorted((tag_name(tag), clip_fingerprint(clip)) for tag, clip in clips.items()))


profile_count = 0
set_count = 0
issue_count = 0

for asset_path in unreal.EditorAssetLibrary.list_assets(ROOT, recursive=True, include_folder=False):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(asset, unreal.ReEcho2DCharacterPresentationProfile):
        continue

    profile_count += 1
    animation_sets = list(asset.get_editor_property("animation_sets"))
    set_count += len(animation_sets)
    ids = defaultdict(list)
    fingerprints = defaultdict(list)

    unreal.log(f"ANIMSET_PROFILE {asset_path} count={len(animation_sets)}")
    for index, animation_set in enumerate(animation_sets):
        set_id = str(animation_set.get_editor_property("weapon_visual_set_id"))
        normalized_id = "" if set_id in ("None", "") else set_id
        clips = animation_set.get_editor_property("clips")
        ids[normalized_id].append(index)
        fingerprints[set_fingerprint(animation_set)].append(index)
        semantic_indices = defaultdict(list)
        for clip_index, tag in enumerate(clips.keys()):
            semantic_indices[tag_name(tag)].append(clip_index)
        semantics = ",".join(sorted(semantic_indices.keys()))
        unreal.log(
            f"ANIMSET_ENTRY {asset_path} index={index} id={normalized_id or '<default>'} "
            f"clips={len(clips)} semantics=[{semantics}]"
        )
        for tag, clip in clips.items():
            unreal.log(
                f"ANIMSET_CLIP {asset_path} index={index} semantic={tag_name(tag)} "
                f"flipbook={object_path(clip.get_editor_property('flipbook'))}"
            )
        for semantic, clip_indices in semantic_indices.items():
            if semantic in ("None", ""):
                issue_count += 1
                unreal.log_warning(
                    f"ANIMSET_INVALID_SEMANTIC {asset_path} set_index={index} clip_indices={clip_indices}"
                )
            elif len(clip_indices) > 1:
                issue_count += 1
                unreal.log_warning(
                    f"ANIMSET_DUPLICATE_SEMANTIC {asset_path} set_index={index} "
                    f"semantic={semantic} clip_indices={clip_indices}"
                )

    for set_id, indices in ids.items():
        if len(indices) > 1:
            issue_count += 1
            unreal.log_warning(
                f"ANIMSET_DUPLICATE_ID {asset_path} id={set_id or '<default>'} indices={indices}"
            )

    for indices in fingerprints.values():
        if len(indices) > 1:
            issue_count += 1
            unreal.log_warning(f"ANIMSET_DUPLICATE_CONTENT {asset_path} indices={indices}")

unreal.log(f"ANIMSET_AUDIT_RESULT profiles={profile_count} sets={set_count} issues={issue_count}")
