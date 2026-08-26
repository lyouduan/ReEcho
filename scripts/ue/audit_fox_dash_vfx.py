"""Read-only audit for the Fox dash Direction, Charging, and Trail Niagara roots."""

import unreal


ROOTS = {
    "Direction": "/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_arrow",
    "Charging": "/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_Charging",
    "Trail": "/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_Trail",
}


def read_property(asset, property_name):
    try:
        return asset.get_editor_property(property_name)
    except Exception as error:
        unreal.log_warning(
            f"FOX_DASH_PROPERTY_UNAVAILABLE asset={asset.get_path_name()} "
            f"property={property_name} error={error}"
        )
        return None


def vector_string(vector):
    return f"({float(vector.x):.3f},{float(vector.y):.3f},{float(vector.z):.3f})"


registry = unreal.AssetRegistryHelpers.get_asset_registry()
dependency_options = unreal.AssetRegistryDependencyOptions()
loaded = {}
for semantic, package_name in ROOTS.items():
    system = unreal.EditorAssetLibrary.load_asset(package_name)
    if not isinstance(system, unreal.NiagaraSystem):
        raise RuntimeError(f"Fox {semantic} Niagara system failed to load: {package_name}")
    loaded[semantic] = system
    unreal.log(f"FOX_DASH_ROOT_OK semantic={semantic} package={package_name}")

direction = loaded["Direction"]
fixed_bounds_enabled = read_property(direction, "b_fixed_bounds")
fixed_bounds = read_property(direction, "fixed_bounds")
if fixed_bounds_enabled is None:
    fixed_bounds_enabled = read_property(direction, "fixed_bounds_enabled")
if fixed_bounds_enabled is not None:
    unreal.log(f"FOX_DIRECTION_FIXED_BOUNDS_ENABLED value={bool(fixed_bounds_enabled)}")
if fixed_bounds is not None:
    minimum = fixed_bounds.min
    maximum = fixed_bounds.max
    size = maximum - minimum
    unreal.log(
        "FOX_DIRECTION_FIXED_BOUNDS "
        f"min={vector_string(minimum)} max={vector_string(maximum)} size={vector_string(size)}"
    )
    if min(abs(float(size.x)), abs(float(size.y)), abs(float(size.z))) <= 0.01:
        raise RuntimeError(f"Fox Direction fixed bounds are degenerate: {size}")

dependency_count = 0
for semantic, package_name in ROOTS.items():
    dependencies = sorted(
        str(dependency)
        for dependency in registry.get_dependencies(package_name, dependency_options)
        if str(dependency).startswith("/Game/")
    )
    dependency_count += len(dependencies)
    for dependency in dependencies:
        dependency_asset = unreal.EditorAssetLibrary.load_asset(dependency)
        dependency_type = dependency_asset.get_class().get_name() if dependency_asset else "Missing"
        unreal.log(
            f"FOX_DASH_DEP semantic={semantic} package={dependency} type={dependency_type}"
        )

# Emitter local-space, renderer enablement, and exact component placement/sort are
# asserted by ReEcho.Presentation.VFX.Catalog through Niagara's native C++ API.
unreal.log(
    "FOX_DASH_AUDIT_OK "
    f"roots={len(loaded)} dependencies={dependency_count} "
    "native_contract=ReEcho.Presentation.VFX.Catalog"
)
