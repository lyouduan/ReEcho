"""One-time approved readability update; preserve all existing HUD geometry/fonts."""
import json
from pathlib import Path
import runpy
import unreal

helpers = runpy.run_path(str(Path(__file__).with_name('author_plan158_boss_health_arc.py')))
bp = unreal.load_asset(helpers['HUD_PATH'])
before = helpers['snapshot'](bp)
saved = Path(unreal.Paths.project_saved_dir())
snapshot_path = saved / 'plan158_readability_before.json'
if not snapshot_path.exists():
    snapshot_path.write_text(json.dumps(before, indent=2), encoding='utf-8')
widgets = helpers['widgets'](bp)
arc = widgets['BossHealthArc']
helpers['create_material']()
arc.set_editor_property('fill_color', unreal.LinearColor(0.68, 0.025, 0.18, 1.0))
arc.set_editor_property('empty_color', unreal.LinearColor(0.008, 0.008, 0.01, 1.0))
arc.set_editor_property('tick_contrast', 0.25)

tool = unreal.UMGToolSet.get_default_object()
percent = widgets.get('BossHealthPercentText')
if percent is None:
    timer = widgets['CountdownText']
    percent = tool.call_method('AddWidget', args=(bp, unreal.TextBlock, 'BossHealthPercentText', timer.get_parent(), -1)).widget
    if percent is None:
        raise RuntimeError('Cannot create authored Boss percentage text')
    tool.call_method('ToggleWidgetAsVariable', args=(bp, percent, True))
    for prop in ('font', 'color_and_opacity', 'justification', 'shadow_color_and_opacity',
                 'shadow_offset', 'auto_wrap_text', 'wrap_text_at', 'render_transform',
                 'render_transform_pivot', 'render_opacity'):
        value = timer.get_editor_property(prop)
        percent.set_editor_property(prop, value.copy() if hasattr(value, 'copy') else value)
    timer_slot = timer.get_editor_property('slot')
    percent_slot = percent.get_editor_property('slot')
    percent_slot.set_editor_property('layout_data', timer_slot.get_editor_property('layout_data').copy())
    percent_slot.set_editor_property('auto_size', timer_slot.get_editor_property('auto_size'))
    percent_slot.set_editor_property('z_order', timer_slot.get_editor_property('z_order'))
    percent.set_editor_property('text', unreal.Text('65%'))
    percent.set_editor_property('visibility', unreal.SlateVisibility.HIT_TEST_INVISIBLE)

if not tool.call_method('CompileWidgetBlueprint', args=(bp,)):
    raise RuntimeError('HUD compilation failed')
after = helpers['snapshot'](bp)
for name, record in before.items():
    if after.get(name) != record:
        raise RuntimeError(f'Protected existing widget changed: {name}')
if not unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False):
    raise RuntimeError('HUD save failed')
unreal.log('[Plan158Readability] PASS purple-red fill, dark empty track, subtle ticks and authored percentage; existing layout/font preserved')
