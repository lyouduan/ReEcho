"""Inspect by default. REECHO_PLAN157_ICON_SIZE=1 repairs only a zero-sized icon.

Run with Run-EditorPythonLocked.ps1 after saving/closing the Editor. Never rebuild
the row or modify authored text, fonts, slots, parent sizing or other assets.
"""
import os
import runpy
from pathlib import Path
import unreal

if os.environ.get("REECHO_PLAN157_APPLY") == "1" or os.environ.get("REECHO_PLAN157_FRAME_INSETS") == "1":
    raise RuntimeError("Do not combine attribute icon repair with other repair modes")
# Reuse the value-based protected snapshot helper; its default execution is read-only.
helpers = runpy.run_path(str(Path(__file__).with_name("repair_plan157_tooltip_backplates.py")))
require = helpers["require"]
bp, widgets = helpers["load"]("WBP_ReEchoAttributeRow")
icon = widgets["AttributeIcon"]
brush = icon.get_editor_property("brush").copy()
size = brush.get_editor_property("image_size").copy()
unreal.log(f"[Plan157IconRepair] BEFORE X={size.get_editor_property('x')} Y={size.get_editor_property('y')}")
if os.environ.get("REECHO_PLAN157_ICON_SIZE") == "1":
    before = helpers["protected_snapshot"](widgets)
    icon_color = icon.get_editor_property("color_and_opacity").export_text()
    if size.get_editor_property("x") <= 0 or size.get_editor_property("y") <= 0:
        size.set_editor_property("x", 28.0)
        size.set_editor_property("y", 28.0)
        brush.set_editor_property("image_size", size)
        expected_brush = brush.export_text()
        icon.set_editor_property("brush", brush)
        require(icon.get_editor_property("color_and_opacity").export_text() == icon_color, "Icon tint changed")
        require(icon.get_editor_property("brush").export_text() == expected_brush, "Unexpected brush change")
        helpers["save_preserving"](bp, widgets, before)
        require(icon.get_editor_property("brush").export_text() == expected_brush, "Compile changed brush")
    else:
        unreal.log("[Plan157IconRepair] Already nonzero; preserve authored icon size, no save")
    unreal.log("[Plan157IconRepair] PASS only invalid icon image size repaired; authored layout preserved")
