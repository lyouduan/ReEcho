"""Dump the editable widget tree for the Plan145 start-menu audit."""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoStartMenu"


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing widget blueprint: {ASSET_PATH}")

infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
widgets = {info.widget.get_name(): info.widget for info in infos if info.widget}
for info in infos:
    widget = info.widget
    if widget is None:
        continue
    parent = widget.get_parent()
    parent_name = parent.get_name() if parent is not None else "<root>"
    slot = widget.get_editor_property("slot")
    geometry = ""
    if isinstance(slot, unreal.CanvasPanelSlot):
        layout = slot.get_editor_property("layout_data")
        offsets = layout.offsets
        geometry = (
            f" x={offsets.left:.1f} y={offsets.top:.1f} "
            f"w={offsets.right:.1f} h={offsets.bottom:.1f} "
            f"z={slot.get_editor_property('z_order')}"
        )
    unreal.log(
        f"[Plan145Tree] {widget.get_name()} class={widget.get_class().get_name()} "
        f"parent={parent_name} variable={info.get_editor_property('is_variable')}{geometry}"
    )

required = {
    "StartMenuPanel": unreal.CanvasPanel,
    "SaveRollbackPanel": unreal.CanvasPanel,
    "ArtSaveRollbackFrame": unreal.Image,
    "ArtSaveRollbackSurface": unreal.Image,
    "ArtSaveRollbackTitle": unreal.Image,
    "ArtSaveRollbackClose": unreal.Image,
    "SaveRollbackCloseButton": unreal.ReEchoIndexedButton,
}
for index in range(3):
    required.update(
        {
            f"SaveSlotOccupiedArt{index}": unreal.Image,
            f"SaveSlotEmptyArt{index}": unreal.Image,
            f"SaveSlotPreview{index}": unreal.Image,
            f"DesignerSaveSlotNoteStrip{index}": unreal.Image,
            f"SaveSlotName{index}": unreal.TextBlock,
            f"SaveSlotMetadata{index}": unreal.TextBlock,
            f"SaveSlotTimestamp{index}": unreal.TextBlock,
            f"SaveSlotButton{index}": unreal.ReEchoIndexedButton,
        }
    )

for name, expected_class in required.items():
    widget = widgets.get(name)
    if not isinstance(widget, expected_class):
        actual = widget.get_class().get_name() if widget is not None else "missing"
        raise RuntimeError(f"Plan145 audit failed: {name} expected {expected_class.__name__}, got {actual}")

save_panel = widgets["SaveRollbackPanel"]
if save_panel.get_editor_property("visibility") != unreal.SlateVisibility.COLLAPSED:
    raise RuntimeError("Plan145 audit failed: SaveRollbackPanel must default to Collapsed")
for name in ("ArtTitleLogo", "ContinueGame", "NewGame", "About", "Settings", "Quit"):
    if widgets[name].get_parent() != widgets["StartMenuPanel"]:
        raise RuntimeError(f"Plan145 audit failed: {name} is outside StartMenuPanel")
for index in range(3):
    for name in (
        f"SaveSlotOccupiedArt{index}",
        f"SaveSlotEmptyArt{index}",
        f"SaveSlotPreview{index}",
        f"DesignerSaveSlotNoteStrip{index}",
        f"SaveSlotName{index}",
        f"SaveSlotMetadata{index}",
        f"SaveSlotTimestamp{index}",
        f"SaveSlotButton{index}",
    ):
        if widgets[name].get_parent() != save_panel:
            raise RuntimeError(f"Plan145 audit failed: {name} is outside SaveRollbackPanel")

unreal.log("[Plan145Audit] PASS: formal save-rollback bindings, classes and ownership are valid")
