#!/usr/bin/env python3
"""Plan 69 WBP 编辑脚本（经由 UnrealEditor -ExecutePythonScript 运行，风险较高）。

尝试给 WBP_ReEchoTraitCardEntry 的 widget tree 添加两个 Image 控件，
命名为 ArtImage / IconImage，以匹配 C++ 中 BindWidgetOptional 字段并自动绑定。

若脚本失败（UE 版本 API 差异），改为手动：编辑器打开 WBP_ReEchoTraitCardEntry，
从面板拖两个 Image 到画布，分别改名 ArtImage、IconImage，编译保存。
"""
import unreal

WBP_PATH = "/Game/ReEcho/UI/WBP_ReEchoTraitCardEntry"


def main():
    bp = unreal.load_asset(WBP_PATH)
    if not bp:
        unreal.log_error(f"[Plan69] WBP not found: {WBP_PATH}")
        return

    widget_tree = bp.get_editor_property("widget_tree")
    root = widget_tree.get_editor_property("root_widget")

    for name in ("ArtImage", "IconImage"):
        try:
            existing = widget_tree.find_widget(name)
        except Exception:
            existing = None
        if existing:
            unreal.log(f"[Plan69] {name} already present")
            continue
        img = widget_tree.construct_widget(unreal.Image)
        try:
            img.set_editor_property("name", name)
        except Exception:
            img.set_name(name)
        if root:
            root.add_child(img)
        unreal.log(f"[Plan69] added {name}")

    unreal.EditorAssetLibrary.save_asset(WBP_PATH)
    unreal.log("[Plan69] WBP saved")


if __name__ == "__main__":
    main()
