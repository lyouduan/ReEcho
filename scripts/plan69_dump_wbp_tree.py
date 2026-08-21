"""Plan 69: dump WBP_ReEchoTraitCardEntry widget tree (names + class + bind targets).

Run this inside the editor: Window > Developer Tools > Output Log, command mode = Python,
paste this file's content or `py "path/to/plan69_dump_wbp_tree.py"`, then copy the printed
tree back to the planner. It reads the live WidgetBlueprint via reflection, so names must
match exactly what BindWidgetOptional expects (ArtImage / IconImage).
"""
import unreal

WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoTraitCardEntry"


def _walk(slot_obj, depth):
    # Slot wrappers expose the actual child via get_content() on PanelWidget slots,
    # but for UMG WidgetTree the nodes are UWidget; iterate children generically.
    pad = "  " * depth
    try:
        name = slot_obj.get_name()
    except Exception:
        name = "?"
    try:
        cls = slot_obj.get_class().get_name()
    except Exception:
        cls = "?"
    print(f"{pad}- {name} : {cls}")


def main():
    asset = unreal.load_asset(WIDGET_PATH)
    if not asset:
        print(f"[Plan69] NOT FOUND: {WIDGET_PATH}")
        return
    tree = getattr(asset, "widget_tree", None)
    if not tree:
        # Fallback: some builds expose it differently.
        print(f"[Plan69] loaded {type(asset).__name__} but no widget_tree attr")
        return
    root = tree.get_editor_property("root_widget") if hasattr(tree, "get_editor_property") else None
    print(f"[Plan69] root widget: {root}")

    def recurse(w, depth):
        if w is None:
            return
        try:
            name = w.get_name()
            cls = w.get_class().get_name()
        except Exception:
            name, cls = "?", "?"
        print("  " * depth + f"- {name} : {cls}")
        # PanelWidget children
        try:
            if isinstance(w, unreal.PanelWidget):
                n = w.get_child_count()
                for i in range(n):
                    recurse(w.get_child_at(i), depth + 1)
        except Exception as e:
            print("  " * (depth + 1) + f"(walk error: {e})")

    recurse(root, 0)
    print("[Plan69] done")


main()
