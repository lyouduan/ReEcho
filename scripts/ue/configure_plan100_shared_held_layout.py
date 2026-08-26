"""Persist Plan100's shared held-weapon hand anchors on the weapon presentation catalog."""

import unreal


CATALOG_PATH = "/Game/ReEcho/DataAsset/Weapon/Catalogs/DA_WeaponPresentationCatalog"


catalog = unreal.EditorAssetLibrary.load_asset(CATALOG_PATH)
if catalog is None:
    raise RuntimeError(f"Missing weapon presentation catalog: {CATALOG_PATH}")

catalog.set_editor_property("right_hand_anchor_ratio", unreal.Vector(-0.16, 0.30, 0.06))
catalog.set_editor_property("left_hand_anchor_ratio", unreal.Vector(-0.16, -0.30, 0.06))

if not unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save weapon presentation catalog: {CATALOG_PATH}")

unreal.log(
    "Plan100 shared held layout saved: "
    f"right={catalog.get_editor_property('right_hand_anchor_ratio')} "
    f"left={catalog.get_editor_property('left_hand_anchor_ratio')}"
)
