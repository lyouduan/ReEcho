"""Add a procedural vein/relief shader without resetting any authored HUD values."""
import json
from pathlib import Path
import re
import runpy
import unreal

helpers = runpy.run_path(str(Path(__file__).with_name('author_plan158_boss_health_arc.py')))


def protected_snapshot(bp):
    result = helpers['snapshot'](bp)
    for name, widget in helpers['widgets'](bp).items():
        for prop in ('arc_material', 'fill_color', 'empty_color', 'tick_contrast', 'reference_size',
                     'arc_center', 'inner_radius', 'outer_radius', 'vein_strength', 'vein_width',
                     'vein_spacing', 'relief_strength', 'justification', 'shadow_color_and_opacity',
                     'shadow_offset', 'auto_wrap_text', 'wrap_text_at'):
            try:
                value = widget.get_editor_property(prop)
                if isinstance(value, unreal.Object):
                    result[name][prop] = value.get_path_name()
                else:
                    result[name][prop] = value.export_text() if hasattr(value, 'export_text') else str(value)
            except Exception:
                pass
    defaults = unreal.get_default_object(bp.generated_class())
    result['__preview__'] = {prop: str(defaults.get_editor_property(prop))
                            for prop in ('preview_boss_encounter', 'preview_boss_health_ratio')}
    return result


def read_snapshot(path):
    result = json.loads(path.read_text(encoding='utf-8'))
    # Early captures used Unreal's repr, which contains a process-local address.
    # Normalize only the reference to its stable object path, not any author value.
    for row in result.values():
        value = row.get('arc_material', '')
        match = re.match(r"<Object '([^']+)' \(0x[0-9A-Fa-f]+\) Class 'Material'>", value)
        if match:
            row['arc_material'] = match.group(1)
    return result


def main():
    bp = unreal.load_asset(helpers['HUD_PATH'])
    if not bp or 'BossHealthArc' not in helpers['widgets'](bp):
        raise RuntimeError('Requires the existing authored Boss health arc')
    before = protected_snapshot(bp)
    saved = Path(unreal.Paths.project_saved_dir())
    path = saved / 'plan158_surface_before.json'
    if path.exists() and read_snapshot(path) != before:
        raise RuntimeError('Author values changed since surface migration: review/new snapshot required')
    if not path.exists():
        path.write_text(json.dumps(before, indent=2, ensure_ascii=False), encoding='utf-8')
    helpers['create_material']()
    if not unreal.UMGToolSet.get_default_object().call_method('CompileWidgetBlueprint', args=(bp,)):
        raise RuntimeError('HUD compile failed')
    if protected_snapshot(bp) != before:
        raise RuntimeError('Author colors/alpha/font/geometry/preview changed; refusing HUD save')
    if not unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False):
        raise RuntimeError('HUD save failed')
    unreal.log('[Plan158Surface] PASS veins/relief added; all existing and new author values preserved')


if __name__ == '__main__':
    main()
