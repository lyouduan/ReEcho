"""Read-only formal-asset and protected authoring snapshot audit."""
import json
import math
from pathlib import Path
import runpy
import unreal

helpers = runpy.run_path(str(Path(__file__).with_name('author_plan158_boss_health_arc.py')))
bp = unreal.load_asset(helpers['HUD_PATH'])
widgets = helpers['widgets'](bp)
arc = widgets.get('BossHealthArc')
assert isinstance(arc, unreal.ReEchoBossHealthArcWidget), 'Missing authored arc'
assert all(name not in widgets for name in helpers['REMOVED']), 'Horizontal bar remains'
material = arc.get_editor_property('arc_material')
assert material is not None, 'Missing material hard reference'
assert material.get_editor_property('material_domain') == unreal.MaterialDomain.MD_UI
assert material.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_TRANSLUCENT
shader = next(node for node in unreal.MaterialEditingLibrary.get_material_expressions(material)
              if isinstance(node, unreal.MaterialExpressionCustom))
assert shader.get_editor_property('code') == helpers['SHADER'], 'Material differs from the approved arc shader'
assert 'TickContrast' in [str(item.get_editor_property('input_name')) for item in shader.get_editor_property('inputs')]
percent = widgets.get('BossHealthPercentText')
assert isinstance(percent, unreal.TextBlock), 'Missing authored Boss percentage'
percent_slot = percent.get_editor_property('slot')
assert percent_slot.get_editor_property('z_order') > widgets['ArtClockNeedle'].get_editor_property('slot').get_editor_property('z_order')
fill = arc.get_editor_property('fill_color')
empty = arc.get_editor_property('empty_color')
# UE LinearColor permits HDR/negative components. Do not replace intentional
# author values with a palette/range assertion; preservation is checked below.
assert all(math.isfinite(value) for color in (fill, empty) for value in (color.r, color.g, color.b, color.a))
assert 0 <= arc.get_editor_property('tick_contrast') <= 1, 'Invalid authored tick strength'
for key in helpers['SURFACE_DEFAULTS']:
    assert key in [str(item.get_editor_property('input_name')) for item in shader.get_editor_property('inputs')], key
arc_slot = arc.get_editor_property('slot')
frame_slot = widgets['ArtClockFrame'].get_editor_property('slot')
needle_slot = widgets['ArtClockNeedle'].get_editor_property('slot')
assert arc_slot.get_editor_property('layout_data').export_text() == frame_slot.get_editor_property('layout_data').export_text()
assert frame_slot.get_editor_property('z_order') <= arc_slot.get_editor_property('z_order') < needle_slot.get_editor_property('z_order')
defaults = unreal.get_default_object(bp.generated_class())
assert defaults.get_editor_property('preview_boss_encounter'), 'Boss Designer preview not enabled'
surface_path = Path(unreal.Paths.project_saved_dir()) / 'plan158_surface_before.json'
path = Path(unreal.Paths.project_saved_dir()) / 'plan158_readability_before.json'
if not path.exists():
    path = Path(unreal.Paths.project_saved_dir()) / 'plan158_hud_protected_before.json'
if surface_path.exists():
    surface_helpers = runpy.run_path(str(Path(__file__).with_name('upgrade_plan158_boss_health_surface.py')))
    before = surface_helpers['read_snapshot'](surface_path)
    after = surface_helpers['protected_snapshot'](bp)
    for name, record in before.items():
        assert after.get(name) == record, f'Protected authoring changed: {name}'
    unreal.log('[Plan158Audit] Surface round protected colors/alpha/font/geometry/preview PASS')
elif path.exists():
    before = json.loads(path.read_text(encoding='utf-8'))
    after = helpers['snapshot'](bp)
    for name, record in before.items():
        if name not in helpers['REMOVED'] and name != 'BossHealthArc':
            assert after.get(name) == record, f'Protected authoring changed: {name}'
    unreal.log('[Plan158Audit] Protected snapshot PASS')
else:
    unreal.log('[Plan158Audit] No local pre-migration snapshot; structural audit only')
unreal.log('[Plan158Audit] PASS arc material, binding, layer, preview and geometry verified')
