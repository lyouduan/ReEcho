"""Read-only audit for Plan153 Sheep Boss Phase3 Paper2D assets."""

from pathlib import Path
import os

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve()
ASSET_ROOT = "/Game/ReEcho/Art/Animation2D/Enemies/Goat/Phase3"
PROFILE_PATH = "/Game/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_TimeGuard"
LEGACY_PROFILE_PATH = "/Game/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_GoatPriest"
CATALOG_PATH = "/Game/ReEcho/DataAsset/Enemy/Catalogs/DA_EnemyPresentationCatalog"
CONFIG_PATH = "/Game/ReEcho/DataAsset/Enemy/DA_SheepBossPhase3"
SPECS = (("Walk", 8.0), ("GroundSlam", 16.0))
issues = []


def normalized(path):
    return os.path.normcase(os.path.normpath(str(Path(path).resolve()))) if path else ""


def semantic(name):
    value = unreal.ReEcho2DPresentationCatalog.resolve_semantic_tag(name)
    if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(value):
        issues.append(f"missing semantic GameplayTag: {name}")
    return value


def set_id(animation_set):
    value = str(animation_set.get_editor_property("weapon_visual_set_id"))
    return "" if value in ("None", "") else value


flipbooks = {}
for animation_name, expected_fps in SPECS:
    expected_sprites = []
    for index in range(1, 9):
        texture_path = f"{ASSET_ROOT}/{animation_name}/Textures/{animation_name}_{index:02d}"
        texture = unreal.EditorAssetLibrary.load_asset(texture_path)
        if not isinstance(texture, unreal.Texture2D):
            issues.append(f"missing Texture2D: {texture_path}")
            expected_sprites.append(None)
            continue
        expected_source = (
            PROJECT_ROOT
            / "Content"
            / "ReEcho"
            / "Art"
            / "Animation2D"
            / "Enemies"
            / "Goat"
            / "Phase3"
            / animation_name
            / "Textures"
            / f"{animation_name}_{index:02d}.png"
        )
        import_data = texture.get_editor_property("asset_import_data")
        actual_source = import_data.get_first_filename() if import_data else ""
        if normalized(actual_source) != normalized(expected_source):
            issues.append(f"wrong import source: {texture_path} -> {actual_source}")
        expected_settings = (
            ("srgb", True),
            ("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),
            ("address_x", unreal.TextureAddress.TA_CLAMP),
            ("address_y", unreal.TextureAddress.TA_CLAMP),
            ("filter", unreal.TextureFilter.TF_BILINEAR),
            ("compression_settings", unreal.TextureCompressionSettings.TC_BC7),
            ("compression_no_alpha", False),
            ("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI),
        )
        for property_name, expected_value in expected_settings:
            actual_value = texture.get_editor_property(property_name)
            if actual_value != expected_value:
                issues.append(
                    f"{texture_path} {property_name}={actual_value}, expected {expected_value}"
                )
        sprite_path = f"{ASSET_ROOT}/{animation_name}/Sprites/{animation_name}_{index:02d}_Sprite"
        sprite = unreal.EditorAssetLibrary.load_asset(sprite_path)
        if not isinstance(sprite, unreal.PaperSprite):
            issues.append(f"missing PaperSprite: {sprite_path}")
        else:
            if sprite.get_editor_property("source_texture") != texture:
                issues.append(f"Sprite source mismatch: {sprite_path}")
            if sprite.get_editor_property("pivot_mode") != unreal.SpritePivotMode.CENTER_CENTER:
                issues.append(f"unexpected pivot: {sprite_path}")
            if abs(float(sprite.get_editor_property("pixels_per_unreal_unit")) - 1.0) > 0.0001:
                issues.append(f"unexpected pixels_per_unreal_unit: {sprite_path}")
        expected_sprites.append(sprite)

    flipbook_path = f"{ASSET_ROOT}/{animation_name}/{animation_name}"
    flipbook = unreal.EditorAssetLibrary.load_asset(flipbook_path)
    if not isinstance(flipbook, unreal.PaperFlipbook):
        issues.append(f"missing PaperFlipbook: {flipbook_path}")
        continue
    flipbooks[animation_name] = flipbook
    if abs(float(flipbook.get_editor_property("frames_per_second")) - expected_fps) > 0.0001:
        issues.append(f"unexpected FPS: {flipbook_path}")
    key_frames = list(flipbook.get_editor_property("key_frames"))
    if len(key_frames) != 8 or flipbook.get_num_frames() != 8:
        issues.append(f"unexpected frame count: {flipbook_path}")
    actual_sprites = [key_frame.get_editor_property("sprite") for key_frame in key_frames]
    if actual_sprites != expected_sprites:
        issues.append(f"Sprite order mismatch: {flipbook_path}")
    if any(int(key_frame.get_editor_property("frame_run")) != 1 for key_frame in key_frames):
        issues.append(f"unexpected frame_run: {flipbook_path}")


catalog = unreal.EditorAssetLibrary.load_asset(CATALOG_PATH)
profile = unreal.EditorAssetLibrary.load_asset(PROFILE_PATH)
if not isinstance(catalog, unreal.ReEcho2DPresentationCatalog):
    issues.append(f"missing enemy presentation catalog: {CATALOG_PATH}")
else:
    time_guard_profiles = [
        entry.get_editor_property("profile")
        for entry in catalog.get_editor_property("presentation_entries")
        if str(entry.get_editor_property("presentation_id")) == "Enemy.TimeGuard"
    ]
    if time_guard_profiles != [profile]:
        issues.append("Enemy.TimeGuard does not resolve to the audited TimeGuard profile")

if not isinstance(profile, unreal.ReEcho2DCharacterPresentationProfile):
    issues.append(f"missing TimeGuard Profile: {PROFILE_PATH}")
else:
    phase3_sets = [
        animation_set
        for animation_set in profile.get_editor_property("animation_sets")
        if set_id(animation_set) == "Phase3"
    ]
    if len(phase3_sets) != 1:
        issues.append(f"expected one Phase3 AnimationSet: count={len(phase3_sets)}")
    else:
        clips = phase3_sets[0].get_editor_property("clips")
        expected_clips = (
            ("Animation.Move", "Walk", True, False),
            ("Animation.Attack.Charge", "GroundSlam", False, True),
            ("Animation.Attack.Basic", "GroundSlam", False, True),
        )
        if len(clips) != len(expected_clips):
            issues.append(f"Phase3 must contain exactly 3 clips: actual={len(clips)}")
        for semantic_name, flipbook_name, looping, restart in expected_clips:
            clip = clips.get(semantic(semantic_name))
            if not clip:
                issues.append(f"Phase3 missing clip: {semantic_name}")
                continue
            if clip.get_editor_property("flipbook") != flipbooks.get(flipbook_name):
                issues.append(f"Phase3 wrong Flipbook: {semantic_name}")
            if bool(clip.get_editor_property("looping")) != looping:
                issues.append(f"Phase3 wrong looping: {semantic_name}")
            if bool(clip.get_editor_property("restart_on_request")) != restart:
                issues.append(f"Phase3 wrong restart_on_request: {semantic_name}")

legacy_profile = unreal.EditorAssetLibrary.load_asset(LEGACY_PROFILE_PATH)
if not isinstance(legacy_profile, unreal.ReEcho2DCharacterPresentationProfile):
    issues.append(f"missing legacy GoatPriest Profile: {LEGACY_PROFILE_PATH}")
elif any(
    set_id(animation_set) == "Phase3"
    for animation_set in legacy_profile.get_editor_property("animation_sets")
):
    issues.append("stale Phase3 AnimationSet remains on unused GoatPriest profile")


config = unreal.EditorAssetLibrary.load_asset(CONFIG_PATH)
if not isinstance(config, unreal.ReEchoBossPhase3Config):
    issues.append(f"missing Phase3 programmer config: {CONFIG_PATH}")
else:
    if str(config.get_editor_property("boss_enemy_id")) != "M_SHEEP":
        issues.append("Phase3 config wrong BossEnemyId")
    if int(config.get_editor_property("existing_ability_max_phase_index")) != 2:
        issues.append("Phase3 config wrong existing ability maximum phase")
    if abs(float(config.get_editor_property("trigger_seconds")) - 15.0) > 0.0001:
        issues.append("Phase3 config wrong trigger window")
    if abs(float(config.get_editor_property("phase_max_health")) - 500.0) > 0.0001:
        issues.append("Phase3 config wrong maximum health")
    if str(config.get_editor_property("ability_id")) != "M_SHEEP_BlinkSlam":
        issues.append("Phase3 config wrong ability id")
    if int(config.get_editor_property("combo_min")) != 1 or int(
        config.get_editor_property("combo_max")
    ) != 3:
        issues.append("Phase3 config wrong combo range")


if issues:
    for issue in issues:
        unreal.log_error(f"PLAN153_PHASE3_AUDIT_ISSUE {issue}")
    raise RuntimeError(f"Plan153 Phase3 animation audit failed with {len(issues)} issue(s)")

unreal.log(
    "PLAN153_PHASE3_AUDIT_OK textures=16 sprites=16 flipbooks=2 "
    "walk_frames=8 walk_fps=8 walk_looping=true "
    "ground_slam_frames=8 ground_slam_fps=16 ground_slam_looping=false "
    "phase3_clips=3 profile=1 config=1"
)
