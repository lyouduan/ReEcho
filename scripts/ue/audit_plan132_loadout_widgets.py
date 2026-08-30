"""Audit the authored Plan132 Loadout WBP hierarchy and binding contract."""

import unreal


ASSET_PATHS = (
    "/Game/ReEcho/UI/WBP_ReEchoLoadoutSelection",
    "/Game/ReEcho/UI/WBP_ReEchoLoadoutEntry",
    "/Game/ReEcho/UI/WBP_ReEchoLoadoutTooltip",
)

FONT_PATHS = (
    "/Game/SourceArt/UI/InteractionPlaceholder/Fonts/Titles/汉仪瑞意宋_80W_Font",
    "/Game/SourceArt/UI/InteractionPlaceholder/Fonts/Titles/汉仪瑞意宋_80W",
)

FORMAL_BUTTON_RESOURCES = {
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/PauseAndCombat/"
    "T_UI_Pause_ButtonLight.T_UI_Pause_ButtonLight",
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/PauseAndCombat/"
    "T_UI_Pause_ButtonDark.T_UI_Pause_ButtonDark",
}


def object_path(value):
    return value.get_path_name() if value else "<none>"


def main():
    toolset = unreal.UMGToolSet.get_default_object()
    for font_path in FONT_PATHS:
        font_asset = unreal.load_asset(font_path)
        unreal.log(
            f"[Plan132LoadoutAudit] font={font_path} "
            f"type={font_asset.get_class().get_name() if font_asset else '<missing>'}"
        )
    for asset_path in ASSET_PATHS:
        blueprint = unreal.load_asset(asset_path)
        if not isinstance(blueprint, unreal.WidgetBlueprint):
            raise RuntimeError(f"Missing Plan132 WidgetBlueprint: {asset_path}")
        infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
        widgets = [info.widget for info in infos if info.widget]
        widget_by_name = {widget.get_name(): widget for widget in widgets}
        roots = [widget for widget in widgets if widget.get_parent() is None]
        unreal.log(
            f"[Plan132LoadoutAudit] asset={asset_path} widgets={len(widgets)} "
            f"roots={[root.get_name() for root in roots]}"
        )
        if asset_path.endswith("WBP_ReEchoLoadoutEntry"):
            root_size = widget_by_name.get("EntryRootSizeBox")
            portrait_size = widget_by_name.get("PortraitSize")
            visual_overlay = widget_by_name.get("EntryVisualOverlay")
            select_button = widget_by_name.get("SelectButton")
            selection_arrow = widget_by_name.get("EntrySelectionArrow")
            portrait_scale = widget_by_name.get("PortraitScale")
            if (
                not isinstance(visual_overlay, unreal.Overlay)
                or select_button.get_parent() is not visual_overlay
                or not isinstance(selection_arrow, unreal.Image)
                or selection_arrow.get_parent() is not visual_overlay
            ):
                raise RuntimeError(
                    "Plan132 entry arrow is not inside the button hover visual root"
                )
            if (
                not isinstance(portrait_scale, unreal.ScaleBox)
                or portrait_scale.get_editor_property("stretch")
                != unreal.Stretch.SCALE_TO_FIT
            ):
                raise RuntimeError("Plan132 entry portrait is not strict aspect-fit")
            portrait_image = widget_by_name.get("PortraitImage")
            portrait_slot = (
                portrait_image.get_editor_property("slot")
                if isinstance(portrait_image, unreal.Image)
                else None
            )
            if (
                not isinstance(portrait_slot, unreal.ScaleBoxSlot)
                or portrait_slot.get_editor_property("horizontal_alignment")
                != unreal.HorizontalAlignment.H_ALIGN_CENTER
                or portrait_slot.get_editor_property("vertical_alignment")
                != unreal.VerticalAlignment.V_ALIGN_CENTER
            ):
                raise RuntimeError(
                    "Plan132 PortraitImage ScaleBox slot still stretches its desired size"
                )
            if (
                selection_arrow.get_editor_property("visibility")
                != unreal.SlateVisibility.HIT_TEST_INVISIBLE
            ):
                raise RuntimeError(
                    "Plan132 standalone Entry preview does not author the arrow visible"
                )
            unreal.log(
                "[Plan132LoadoutAudit] entry_visual_root=EntryVisualOverlay "
                "children=['SelectButton', 'EntrySelectionArrow'] "
                "portrait_stretch=ScaleToFit portrait_align=Center"
            )
            generated_class = blueprint.generated_class()
            default_object = unreal.get_default_object(generated_class)
            has_desired_preview = default_object.call_method(
                "HasDesiredDesignerPreview"
            )
            if not has_desired_preview:
                raise RuntimeError(
                    "Plan132 standalone Entry Designer preview is not desired-size"
                )
            unreal.log(
                "[Plan132LoadoutAudit] entry_desired_preview=True "
                "entry_arrow_default=HitTestInvisible"
            )
            if not isinstance(root_size, unreal.SizeBox) or not isinstance(
                portrait_size, unreal.SizeBox
            ):
                raise RuntimeError("Plan132 Entry authored SizeBox hierarchy is missing")
            unreal.log(
                "[Plan132LoadoutAudit] blueprint_owned_heights "
                f"entry={root_size.get_editor_property('height_override')} "
                f"portrait={portrait_size.get_editor_property('height_override')}"
            )
            tooltip_class = default_object.get_editor_property("tooltip_widget_class")
            expected_tooltip_path = (
                "/Game/ReEcho/UI/WBP_ReEchoLoadoutTooltip."
                "WBP_ReEchoLoadoutTooltip_C"
            )
            actual_tooltip_path = (
                tooltip_class.get_path_name() if tooltip_class else "<missing>"
            )
            unreal.log(
                f"[Plan132LoadoutAudit] entry_tooltip_class={actual_tooltip_path}"
            )
            if actual_tooltip_path != expected_tooltip_path:
                raise RuntimeError(
                    "Plan132 entry does not reference the authored tooltip class"
                )
        if asset_path.endswith("WBP_ReEchoLoadoutSelection"):
            design_canvas = widget_by_name.get("LoadoutDesignCanvas")
            stage_switcher = widget_by_name.get("StageSwitcher")
            character_stage = widget_by_name.get("CharacterStagePanel")
            weapon_stage = widget_by_name.get("WeaponStagePanel")
            if (
                not isinstance(stage_switcher, unreal.WidgetSwitcher)
                or stage_switcher.get_parent() is not design_canvas
                or stage_switcher.get_children_count() != 2
                or stage_switcher.get_child_at(0) is not character_stage
                or stage_switcher.get_child_at(1) is not weapon_stage
            ):
                raise RuntimeError(
                    "Plan132 selection stages are not owned by the Designer WidgetSwitcher"
                )
            unreal.log(
                "[Plan132LoadoutAudit] "
                f"stage_switcher_active={stage_switcher.get_active_widget_index()} "
                "pages=['CharacterStagePanel', 'WeaponStagePanel']"
            )
            character_row = widget_by_name.get("CharacterRow")
            weapon_row = widget_by_name.get("WeaponRow")
            if not isinstance(character_row, unreal.HorizontalBox) or not isinstance(
                weapon_row, unreal.HorizontalBox
            ):
                raise RuntimeError("Plan132 selection preview rows are missing")
            for domain, row in (
                ("Character", character_row),
                ("Weapon", weapon_row),
            ):
                children = [
                    widget
                    for widget in widgets
                    if widget.get_parent() is row
                ]
                unreal.log(
                    f"[Plan132LoadoutAudit] {domain.lower()}_preview_entries="
                    f"{[child.get_name() for child in children]}"
                )
                expected_names = [f"{domain}Entry{index}" for index in range(4)]
                if [child.get_name() for child in children] != expected_names:
                    raise RuntimeError(
                        f"Plan132 {domain} Designer preview is not four authored entries"
                    )
                if any(
                    child.get_class().get_name() != "WBP_ReEchoLoadoutEntry_C"
                    for child in children
                ):
                    raise RuntimeError(
                        f"Plan132 {domain} preview does not use the runtime Entry WBP"
                    )
            legacy_arrow_names = (
                "CharacterSelectionArrow0",
                "CharacterSelectionArrow1",
                "CharacterSelectionArrow2",
                "SelectionArrow",
                "WeaponSelectionArrow0",
                "WeaponSelectionArrow1",
                "WeaponSelectionArrow2",
                "WeaponSelectionArrow3",
            )
            remaining_legacy_arrows = [
                name for name in legacy_arrow_names if widget_by_name.get(name) is not None
            ]
            if remaining_legacy_arrows:
                raise RuntimeError(
                    "Plan132 selection still owns detached legacy arrows: "
                    f"{remaining_legacy_arrows}"
                )
            unreal.log(
                "[Plan132LoadoutAudit] detached_selection_arrows=[]"
            )
            confirm_button = widget_by_name.get("ConfirmButton")
            confirm_label = widget_by_name.get("ConfirmButtonLabel")
            back_button = widget_by_name.get("BackButton")
            back_label = widget_by_name.get("BackButtonLabel")
            if not isinstance(confirm_button, unreal.Button) or not isinstance(
                back_button, unreal.Button
            ):
                raise RuntimeError("Plan154 Loadout Back/Confirm buttons are missing")
            if not isinstance(confirm_label, unreal.TextBlock) or not isinstance(
                back_label, unreal.TextBlock
            ):
                raise RuntimeError("Plan154 Loadout Back/Confirm labels are missing")
            if (
                confirm_button.get_parent() is not design_canvas
                or back_button.get_parent() is not design_canvas
                or confirm_label.get_parent() is not confirm_button
                or back_label.get_parent() is not back_button
            ):
                raise RuntimeError(
                    "Plan154 Back/Confirm controls do not share the Designer-owned button hierarchy"
                )
            confirm_style = confirm_button.get_editor_property("widget_style")
            back_style = back_button.get_editor_property("widget_style")
            for state in ("normal", "hovered", "pressed", "disabled"):
                confirm_brush = confirm_style.get_editor_property(state)
                back_brush = back_style.get_editor_property(state)
                confirm_resource = confirm_brush.get_editor_property("resource_object")
                back_resource = back_brush.get_editor_property("resource_object")
                unreal.log(
                    f"[Plan154LoadoutAudit] state={state} "
                    f"confirm={object_path(confirm_resource)} "
                    f"back={object_path(back_resource)}"
                )
                if (
                    object_path(confirm_resource) not in FORMAL_BUTTON_RESOURCES
                    or object_path(back_resource) not in FORMAL_BUTTON_RESOURCES
                    or confirm_brush.get_editor_property("draw_as")
                    != back_brush.get_editor_property("draw_as")
                ):
                    raise RuntimeError(
                        f"Plan154 Back/Confirm {state} Brushes leave the formal family: "
                        f"confirm={object_path(confirm_resource)} "
                        f"back={object_path(back_resource)}"
                    )
            confirm_slot = confirm_button.get_editor_property("slot")
            back_slot = back_button.get_editor_property("slot")
            confirm_offsets = confirm_slot.get_editor_property(
                "layout_data"
            ).get_editor_property("offsets")
            back_offsets = back_slot.get_editor_property("layout_data").get_editor_property(
                "offsets"
            )
            unreal.log(
                "[Plan154LoadoutAudit] "
                f"back=({back_offsets.left},{back_offsets.top},"
                f"{back_offsets.right},{back_offsets.bottom}) "
                f"confirm=({confirm_offsets.left},{confirm_offsets.top},"
                f"{confirm_offsets.right},{confirm_offsets.bottom})"
            )
            if back_offsets.right <= 0 or back_offsets.bottom <= 0:
                raise RuntimeError("Plan154 Back button has no authored size")
            if (
                back_offsets.left < 0
                or back_offsets.top < 0
                or back_offsets.left + back_offsets.right > 1920
                or back_offsets.top + back_offsets.bottom > 1080
            ):
                raise RuntimeError("Plan154 Back button leaves the 1920x1080 authored surface")
            if (
                back_offsets.left < confirm_offsets.left + confirm_offsets.right
                and back_offsets.left + back_offsets.right > confirm_offsets.left
                and back_offsets.top < confirm_offsets.top + confirm_offsets.bottom
                and back_offsets.top + back_offsets.bottom > confirm_offsets.top
            ):
                raise RuntimeError("Plan154 Back button overlaps Confirm")
            if str(back_label.get_editor_property("text")) != "返回":
                raise RuntimeError("Plan154 Back label does not use the formal return text")
            back_font = back_label.get_editor_property("font")
            confirm_font = confirm_label.get_editor_property("font")
            back_color = back_label.get_editor_property("color_and_opacity")
            confirm_color = confirm_label.get_editor_property("color_and_opacity")
            unreal.log(
                "[Plan154LoadoutAudit] typography "
                f"back_font={object_path(back_font.get_editor_property('font_object'))} "
                f"back_size={back_font.get_editor_property('size')} "
                f"back_color={back_color.get_editor_property('specified_color')} "
                f"confirm_font={object_path(confirm_font.get_editor_property('font_object'))} "
                f"confirm_size={confirm_font.get_editor_property('size')} "
                f"confirm_color={confirm_color.get_editor_property('specified_color')}"
            )
            if (
                object_path(back_font.get_editor_property("font_object"))
                != object_path(confirm_font.get_editor_property("font_object"))
                or back_font.get_editor_property("size") <= 0
                or back_color.get_editor_property("specified_color")
                != confirm_color.get_editor_property("specified_color")
            ):
                raise RuntimeError("Plan154 Back label leaves the accepted formal typography")
            unreal.log(
                "[Plan154LoadoutAudit] "
                f"back=({back_offsets.left},{back_offsets.top},"
                f"{back_offsets.right},{back_offsets.bottom}) "
                f"confirm=({confirm_offsets.left},{confirm_offsets.top},"
                f"{confirm_offsets.right},{confirm_offsets.bottom}) "
                "style=formal-family designer_owned=True"
            )
        for info in infos:
            widget = info.widget
            if widget is None:
                continue
            parent = widget.get_parent()
            slot = widget.get_editor_property("slot")
            unreal.log(
                f"[Plan132LoadoutAudit] widget={widget.get_name()} "
                f"type={widget.get_class().get_name()} "
                f"parent={parent.get_name() if parent else '<none>'} "
                f"slot={slot.get_class().get_name() if slot else '<none>'} "
                f"variable={bool(info.get_editor_property('is_variable'))}"
            )
            if isinstance(widget, unreal.TextBlock):
                font = widget.get_editor_property("font")
                font_object = font.get_editor_property("font_object")
                outline = font.get_editor_property("outline_settings")
                unreal.log(
                    f"[Plan132LoadoutAudit] text={widget.get_name()} "
                    f"font={font_object.get_path_name() if font_object else '<default>'} "
                    f"size={font.get_editor_property('size')} "
                    f"outline={outline.get_editor_property('outline_size')}"
                )
        if asset_path.endswith("WBP_ReEchoLoadoutTooltip"):
            widget_by_name = {widget.get_name(): widget for widget in widgets}
            root = widget_by_name.get("TooltipRootSizeBox")
            frame = widget_by_name.get("TooltipFrame")
            title = widget_by_name.get("TitleText")
            description = widget_by_name.get("DescriptionText")
            if not isinstance(root, unreal.SizeBox):
                raise RuntimeError("Plan132 tooltip root SizeBox is missing")
            if not isinstance(title, unreal.TextBlock):
                raise RuntimeError("Plan132 tooltip TitleText is missing")
            if not isinstance(description, unreal.TextBlock):
                raise RuntimeError("Plan132 tooltip DescriptionText is missing")
            if not isinstance(frame, unreal.Border):
                raise RuntimeError("Plan132 tooltip frame Border is missing")
            frame_brush = frame.get_editor_property("background")
            frame_resource = frame_brush.get_editor_property("resource_object")
            expected_frame_path = (
                "/Game/ReEcho/Textures/UI/LoadoutSelection/"
                "T_UI_Loadout_DescriptionPanel.T_UI_Loadout_DescriptionPanel"
            )
            if (
                not isinstance(frame_resource, unreal.Texture2D)
                or frame_resource.get_path_name() != expected_frame_path
                or frame_brush.get_editor_property("draw_as")
                != unreal.SlateBrushDrawType.BOX
            ):
                raise RuntimeError(
                    "Plan132 tooltip frame does not use the scalable delivery texture"
                )
            if root.get_editor_property("width_override") != 380.0:
                raise RuntimeError("Plan132 tooltip authored width is not 380")
            if title.get_editor_property("font").size != 22:
                raise RuntimeError("Plan132 tooltip authored title font is not 22")
            if description.get_editor_property("font").size != 20:
                raise RuntimeError("Plan132 tooltip authored body font is not 20")
            default_object = unreal.get_default_object(blueprint.generated_class())
            has_desired_preview = default_object.call_method(
                "HasDesiredDesignerPreview"
            )
            unreal.log(
                "[Plan132LoadoutAudit] "
                f"tooltip_desired_preview={bool(has_desired_preview)} "
                f"tooltip_frame={frame_resource.get_path_name()} draw=Box"
            )
            if not has_desired_preview:
                raise RuntimeError(
                    "Plan132 tooltip Designer preview is not desired-size WYSIWYG"
                )
            preview_description = str(description.get_editor_property("text"))
            if len(preview_description) < 30:
                raise RuntimeError(
                    "Plan132 tooltip Designer preview description is not representative"
                )


if __name__ == "__main__":
    try:
        main()
    except Exception:
        unreal.SystemLibrary.request_exit_with_status(True, 1)
        raise
