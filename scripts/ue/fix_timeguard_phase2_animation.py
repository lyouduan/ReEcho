"""Audit TimeGuard Phase2 clips; set REECHO_FIX_TIMEGUARD_PHASE2=1 to fill missing bindings only."""
import os
import unreal

path = '/Game/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_TimeGuard'
profile = unreal.EditorAssetLibrary.load_asset(path)
if not isinstance(profile, unreal.ReEcho2DCharacterPresentationProfile):
    raise RuntimeError(f'Missing profile: {path}')
sets = list(profile.get_editor_property('animation_sets'))
matches = [s for s in sets if str(s.get_editor_property('weapon_visual_set_id')) == 'Phase2']
if len(matches) != 1:
    raise RuntimeError('Expected exactly one authored Phase2 set; refusing to replace other sets')
phase2 = matches[0]
clips = phase2.get_editor_property('clips')
mapping = {
    'Animation.Move': ('Walk', True),
    'Animation.Attack.Basic': ('Attack', False),
    'Animation.Attack.Charge': ('Attack', True),
    'Animation.Hit': ('Walk', False),
    'Animation.Transform.Phase2': ('Walk', False),
    'Animation.Death': ('Death', False),
}
changed = False
for name, (asset_name, looping) in mapping.items():
    tag = unreal.ReEcho2DPresentationCatalog.resolve_semantic_tag(name)
    if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(tag):
        raise RuntimeError(f'Invalid semantic: {name}')
    old = clips.get(tag)
    old_asset = old.get_editor_property('flipbook') if old else None
    unreal.log(f'PHASE2_BINDING {name} before={old_asset.get_path_name() if old_asset else "MISSING"}')
    if old_asset:
        if '/BadGoat/' not in old_asset.get_path_name():
            raise RuntimeError(f'Unexpected authored clip, refusing overwrite: {name}')
        continue
    asset = unreal.EditorAssetLibrary.load_asset(f'/Game/ReEcho/Art/Animation2D/Enemies/BadGoat/Flipbooks/{asset_name}')
    if not isinstance(asset, unreal.PaperFlipbook):
        raise RuntimeError(f'Missing black goat flipbook: {asset_name}')
    clip = unreal.ReEcho2DAnimationClip()
    clip.set_editor_property('flipbook', asset)
    clip.set_editor_property('looping', looping)
    clip.set_editor_property('restart_on_request', not looping)
    clip.set_editor_property('play_rate', 1.0)
    clip.set_editor_property('use_native_scale', False)
    clips[tag] = clip
    changed = True
if changed and os.environ.get('REECHO_FIX_TIMEGUARD_PHASE2') == '1':
    phase2.set_editor_property('clips', clips)
    # Preserve every other animation set and all unrelated profile fields.
    profile.set_editor_property('animation_sets', sets)
    if not unreal.EditorAssetLibrary.save_loaded_asset(profile, only_if_is_dirty=False):
        raise RuntimeError('Could not save TimeGuard profile')
    unreal.log('PHASE2_BINDING repaired missing clips only')
elif changed:
    raise RuntimeError('Missing Phase2 mappings; rerun with explicit repair environment flag')
else:
    unreal.log('PHASE2_BINDING verified complete')
