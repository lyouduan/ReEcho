"""One-time minimal Boss HUD migration; never rebuild the rest of the HUD."""

import json
from pathlib import Path
import unreal

HUD_PATH = '/Game/ReEcho/UI/WBP_ReEchoEncounterHud'
MATERIAL_PATH = '/Game/ReEcho/Materials/UI/M_UI_BossHealthArc'
FRAME_PATH = '/Game/ReEcho/Textures/UI/CombatHud/T_UI_CombatHud_ClockFrame'
REMOVED = {'BossHealthPanel', 'BossHealthFill', 'BossHealthFrame'}
SURFACE_DEFAULTS = {'VeinStrength': 0.4, 'VeinWidth': 1.3, 'VeinSpacing': 42.0, 'ReliefStrength': 0.3}

# Angle is zero at the right, 1 at the left. Remaining fill extends from the
# right to the needle: as health drops, its boundary travels left-bottom-right.
SHADER = r'''
// Static reference-pixel height field: never tied to time or current health.
struct VeinSurface
{
    float trunk(float x)
    {
        return 1.5*sin(x*0.047) + 0.65*sin(x*0.113 + 0.7);
    }
    float segment(float2 q, float2 a, float2 b)
    {
        float2 ab = b-a;
        float t = saturate(dot(q-a, ab)/max(dot(ab, ab), 0.01));
        return length(q-a-ab*t);
    }
    float height(float2 q, float width, float spacing, float halfWidth)
    {
        float d = abs(q.y-trunk(q.x));
        float cell = floor(q.x/spacing);
        [unroll] for (int offset=-1; offset<=1; ++offset)
        {
            float id = cell+offset;
            float seed = frac(sin(id*127.1+5.3)*43758.5453);
            float x = (id+0.25+seed*0.3)*spacing;
            float side = fmod(abs(id), 2.0) < 1.0 ? 1.0 : -1.0;
            float2 a = float2(x, trunk(x));
            float2 b = a+float2(spacing*(0.22+seed*0.12), side*halfWidth*0.75);
            float2 c = lerp(a, b, 0.55);
            float2 e = c+float2(-spacing*0.12, side*halfWidth*0.3);
            d = min(d, segment(q, a, b)/0.62);
            d = min(d, segment(q, c, e)/0.38);
        }
        float h = exp2(-1.5*d*d/max(width*width, 0.25));
        // Blend into the ring before its silhouette, leaving opacity untouched.
        return h*(1.0-smoothstep(halfWidth*0.65, halfWidth, abs(q.y)));
    }
};
float2 p = UV * ReferenceSize.xy - ArcCenter.xy;
float radius = length(p);
float aa = max(fwidth(radius), 0.5);
float ring = smoothstep(InnerRadius-aa, InnerRadius+aa, radius);
ring *= 1.0-smoothstep(OuterRadius-aa, OuterRadius+aa, radius);
ring *= smoothstep(-aa, aa, p.y);
float angle = atan2(max(p.y, 0.0), p.x) / 3.14159265359;
float angularAA = max(fwidth(angle), 0.001);
float filled = 1.0-smoothstep(HealthRatio-angularAA, HealthRatio+angularAA, angle);
filled = HealthRatio <= 0.0 ? 0.0 : (HealthRatio >= 1.0 ? 1.0 : filled);
float4 tint = lerp(EmptyColor, FillColor, filled);
// Keep adjustable, subtle ink detail so the continuous health fill reads first.
float ink = saturate(max(FrameRGB.r, max(FrameRGB.g, FrameRGB.b)) * 4.0);
float midRadius = (InnerRadius+OuterRadius)*0.5;
float halfWidth = max((OuterRadius-InnerRadius)*0.5, 0.5);
float2 q = float2(angle*3.14159265359*midRadius, radius-midRadius);
VeinSurface surface;
float width = clamp(VeinWidth, 0.5, 4.0);
float spacing = clamp(VeinSpacing, 20.0, 100.0);
float height = surface.height(q, width, spacing, halfWidth);
// The whole strip has a rounded, raised cross-section. Veins are flat albedo
// only: their width/spacing/strength never influence these normals or lighting.
float crossSection = clamp(q.y/halfWidth, -1.0, 1.0);
float2 radial = p/max(radius, 1.0);
float3 normal = float3(radial*crossSection, sqrt(saturate(1.0-crossSection*crossSection)));
float3 light = normalize(float3(-0.45,-0.6,1.0));
float3 halfVector = normalize(light+float3(0.0,0.0,1.0));
float diffuse = saturate(dot(normal, light));
float highlight = pow(saturate(dot(normal, halfVector)), 16.0);
float lit = 0.22 + 1.05*diffuse + 0.45*highlight;
float relief = 1.0-pow(1.0-saturate(ReliefStrength), 2.0);
float shading = lerp(1.0, lit, relief);
float surfaceTint = shading*(1.0-saturate(VeinStrength)*height*0.65);
float3 rgb = tint.rgb*lerp(1.0, ink, saturate(TickContrast))*lerp(1.0, surfaceTint, filled);
return float4(rgb, tint.a * ring * FrameAlpha);
'''


def widgets(bp):
    tool = unreal.UMGToolSet.get_default_object()
    return {i.widget.get_name(): i.widget for i in tool.call_method('GetWidgets', args=(bp,)).widgets if i.widget}


def snapshot(bp):
    result = {}
    for name, widget in widgets(bp).items():
        row = {'class': widget.get_class().get_name()}
        for prop in ('visibility', 'render_transform', 'render_transform_pivot', 'render_opacity',
                     'font', 'color_and_opacity', 'brush_color', 'brush', 'text'):
            try:
                value = widget.get_editor_property(prop)
                row[prop] = value.export_text() if hasattr(value, 'export_text') else str(value)
            except Exception:
                pass
        slot = widget.get_editor_property('slot')
        if isinstance(slot, unreal.CanvasPanelSlot):
            row['layout_data'] = slot.get_editor_property('layout_data').export_text()
            row['z_order'] = slot.get_editor_property('z_order')
            row['auto_size'] = slot.get_editor_property('auto_size')
        parent = widget.get_parent()
        row['parent'] = parent.get_name() if parent else None
        result[name] = row
    return result


def connect_surface_parameters(material, custom):
    """Add missing surface controls only; never replace existing author colors/defaults."""
    library = unreal.MaterialEditingLibrary
    expressions = library.get_material_expressions(material)
    inputs = list(custom.get_editor_property('inputs'))
    names = {str(item.get_editor_property('input_name')) for item in inputs}
    for key in SURFACE_DEFAULTS:
        if key not in names:
            item = unreal.CustomInput()
            item.set_editor_property('input_name', key)
            inputs.append(item)
    custom.set_editor_property('inputs', inputs)
    for index, (key, default) in enumerate(SURFACE_DEFAULTS.items()):
        parameter = next((node for node in expressions
                          if isinstance(node, unreal.MaterialExpressionScalarParameter)
                          and str(node.get_editor_property('parameter_name')) == key), None)
        if parameter is None:
            parameter = library.create_material_expression(material, unreal.MaterialExpressionScalarParameter,
                                                           -1000, 1100+index*120)
            parameter.set_editor_property('parameter_name', key)
            parameter.set_editor_property('default_value', default)
        if not library.connect_material_expressions(parameter, '', custom, key):
            raise RuntimeError(f'Cannot connect surface control {key}')


def create_material():
    material = unreal.load_asset(MATERIAL_PATH)
    if material:
        library = unreal.MaterialEditingLibrary
        expressions = library.get_material_expressions(material)
        custom = next(node for node in expressions if isinstance(node, unreal.MaterialExpressionCustom))
        tick = next((node for node in expressions if isinstance(node, unreal.MaterialExpressionScalarParameter)
                     and str(node.get_editor_property('parameter_name')) == 'TickContrast'), None)
        if tick is None:
            tick = library.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -1000, 500)
            tick.set_editor_property('parameter_name', 'TickContrast')
            tick.set_editor_property('default_value', 0.25)
            inputs = list(custom.get_editor_property('inputs'))
            item = unreal.CustomInput()
            item.set_editor_property('input_name', 'TickContrast')
            inputs.append(item)
            custom.set_editor_property('inputs', inputs)
        if not library.connect_material_expressions(tick, '', custom, 'TickContrast'):
            raise RuntimeError('Cannot connect tick contrast')
        connect_surface_parameters(material, custom)
        custom.set_editor_property('code', SHADER)
        for mask in expressions:
            if isinstance(mask, unreal.MaterialExpressionComponentMask):
                if not library.connect_material_expressions(custom, '', mask, ''):
                    raise RuntimeError('Cannot connect arc output channel')
        errors = library.recompile_material(material)
        if errors:
            raise RuntimeError(str(errors))
        unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
        return material
    folder, name = MATERIAL_PATH.rsplit('/', 1)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, folder, unreal.Material, unreal.MaterialFactoryNew())
    if material is None:
        raise RuntimeError('Cannot create Boss arc material')
    material.set_editor_property('material_domain', unreal.MaterialDomain.MD_UI)
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    library = unreal.MaterialEditingLibrary
    def node(cls, x, y):
        return library.create_material_expression(material, cls, x, y)
    uv = node(unreal.MaterialExpressionTextureCoordinate, -1000, -300)
    sample = node(unreal.MaterialExpressionTextureSample, -1000, -100)
    sample.set_editor_property('texture', unreal.load_asset(FRAME_PATH))
    sources = {'UV': (uv, ''), 'FrameRGB': (sample, 'RGB'), 'FrameAlpha': (sample, 'A')}
    for index, (key, value) in enumerate({'HealthRatio': 1.0, 'InnerRadius': 104.0, 'OuterRadius': 126.0, 'TickContrast': 0.25}.items()):
        parameter = node(unreal.MaterialExpressionScalarParameter, -1000, 140 + index*120)
        parameter.set_editor_property('parameter_name', key)
        parameter.set_editor_property('default_value', value)
        sources[key] = (parameter, '')
    for index, (key, value) in enumerate({
        'FillColor': (0.68, 0.025, 0.18, 1.0), 'EmptyColor': (0.008, 0.008, 0.01, 1.0),
        'ReferenceSize': (1159.0, 216.0, 0.0, 0.0), 'ArcCenter': (580.0, 54.0, 0.0, 0.0),
    }.items()):
        parameter = node(unreal.MaterialExpressionVectorParameter, -1000, 550 + index*120)
        parameter.set_editor_property('parameter_name', key)
        parameter.set_editor_property('default_value', unreal.LinearColor(*value))
        sources[key] = (parameter, 'RGBA')
    custom = node(unreal.MaterialExpressionCustom, -450, -100)
    custom.set_editor_property('code', SHADER)
    custom.set_editor_property('description', 'Boss health along original clock lower dial')
    custom.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    inputs = []
    for key in sources:
        item = unreal.CustomInput()
        item.set_editor_property('input_name', key)
        inputs.append(item)
    custom.set_editor_property('inputs', inputs)
    for key, (source, output) in sources.items():
        if not library.connect_material_expressions(source, output, custom, key):
            raise RuntimeError(f'Cannot connect {key}')
    connect_surface_parameters(material, custom)
    for alpha, property_name, y in ((False, unreal.MaterialProperty.MP_EMISSIVE_COLOR, -150),
                                    (True, unreal.MaterialProperty.MP_OPACITY, 100)):
        mask = node(unreal.MaterialExpressionComponentMask, -100, y)
        for channel in ('r', 'g', 'b'):
            mask.set_editor_property(channel, not alpha)
        mask.set_editor_property('a', alpha)
        if not library.connect_material_expressions(custom, '', mask, ''):
            raise RuntimeError('Cannot connect arc output channel')
        if not library.connect_material_property(mask, '', property_name):
            raise RuntimeError('Cannot connect arc material property')
    errors = library.recompile_material(material)
    if errors:
        raise RuntimeError(str(errors))
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError('Cannot save Boss arc material')
    return material


def main():
    bp = unreal.load_asset(HUD_PATH)
    if not bp:
        raise RuntimeError('Encounter HUD is missing')
    tool = unreal.UMGToolSet.get_default_object()
    before = snapshot(bp)
    saved = Path(unreal.Paths.project_saved_dir())
    before_path = saved / 'plan158_hud_protected_before.json'
    if not before_path.exists():
        before_path.write_text(json.dumps(before, indent=2), encoding='utf-8')
    original = widgets(bp)
    material = create_material()
    arc = original.get('BossHealthArc')
    if arc is None:
        frame = original['ArtClockFrame']
        arc = tool.call_method('AddWidget', args=(bp, unreal.ReEchoBossHealthArcWidget,
                              'BossHealthArc', frame.get_parent(), -1)).widget
        if arc is None:
            raise RuntimeError('Cannot add health arc')
        tool.call_method('ToggleWidgetAsVariable', args=(bp, arc, True))
        arc.set_editor_property('arc_material', material)
        arc.set_editor_property('visibility', unreal.SlateVisibility.HIT_TEST_INVISIBLE)
        frame_slot = frame.get_editor_property('slot')
        arc_slot = arc.get_editor_property('slot')
        arc_slot.set_editor_property('layout_data', frame_slot.get_editor_property('layout_data').copy())
        arc_slot.set_editor_property('auto_size', False)
        # Later sibling at the same Z: above the dial, below the untouched Z=6 needle.
        arc_slot.set_editor_property('z_order', frame_slot.get_editor_property('z_order'))
        arc.set_editor_property('render_transform', frame.get_editor_property('render_transform').copy())
        arc.set_editor_property('render_transform_pivot', frame.get_editor_property('render_transform_pivot').copy())
        for name in ('BossHealthFill', 'BossHealthFrame', 'BossHealthPanel'):
            if name in original and not tool.call_method('RemoveWidget', args=(bp, original[name])):
                raise RuntimeError(f'Cannot remove replaced widget: {name}')
    if not tool.call_method('CompileWidgetBlueprint', args=(bp,)):
        raise RuntimeError('HUD compilation failed')
    defaults = unreal.get_default_object(bp.generated_class())
    defaults.set_editor_property('preview_boss_encounter', True)
    defaults.set_editor_property('preview_boss_health_ratio', 0.65)
    if not unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False):
        raise RuntimeError('HUD save failed')
    after = snapshot(bp)
    for name, record in before.items():
        if name not in REMOVED and name != 'BossHealthArc' and after.get(name) != record:
            raise RuntimeError(f'Protected authored widget changed: {name}')
    unreal.log('[Plan158Author] PASS arc added; clock/needle/fonts/minimap preserved; horizontal Boss bar removed')


if __name__ == '__main__':
    main()
