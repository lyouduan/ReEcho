# Minimap classic ink brush source art

This directory archives the complete reviewed Photoshop delivery for the minimap Echo trail.

- `BrushTip_512.png`: production runtime brush-tip Alpha.
- `Grain_900.png`: production runtime grain texture.
- `BrushTip_256.png`: delivered lower-resolution alternative; archived only.
- `ClassicInkBrush.abr`: original Photoshop brush package; archived only because Unreal does not load ABR files at runtime.
- `OriginalParameters.json`: source brush parameters.
- `PhotoshopParameters.txt`: human-readable Photoshop parameter mapping.

Runtime textures are imported by `scripts/ue/import_minimap_ink_brush_assets.py`. The UI material is authored by `scripts/ue/author_minimap_ink_trail_material.py`.
