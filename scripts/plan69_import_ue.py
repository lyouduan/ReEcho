#!/usr/bin/env python3
"""Plan 69 UE 编辑器导入脚本（经由 UnrealEditor -ExecutePythonScript 运行）。

将工作树 Content/SourceArt/UI/Cards/{Art,Icon} 下的 PNG 导入为 UTexture2D：
  /Game/ReEcho/Textures/UI/Cards/Art/T_UI_Card_{id}
  /Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_{id}
对应 ReEchoTraitCardChoiceWidget::ResolveCardArtTexturePath / ResolveCardIconTexturePath。
"""
import os
import unreal

WORKTREE = r"c:/Users/gavynqiu/Documents/miniGame/ReEcho-plan69-trait-card-art"
SOURCE_ART = os.path.join(WORKTREE, "Content", "SourceArt", "UI", "Cards")


def import_folder(src_sub: str, dest_game_path: str):
    src = os.path.join(SOURCE_ART, src_sub)
    if not os.path.isdir(src):
        unreal.log_warning(f"[Plan69] no source dir {src}")
        return
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    files = [f for f in os.listdir(src) if f.lower().endswith(".png")]
    tasks = []
    for png in files:
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", os.path.join(src, png))
        task.set_editor_property("destination_path", dest_game_path)
        task.set_editor_property("destination_name", os.path.splitext(png)[0])
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        tasks.append(task)
    if tasks:
        asset_tools.import_asset_tasks(tasks)
        unreal.log(f"[Plan69] imported {len(tasks)} textures -> {dest_game_path}")
    else:
        unreal.log_warning(f"[Plan69] no PNGs found in {src}")


if __name__ == "__main__":
    import_folder("Art", "/Game/ReEcho/Textures/UI/Cards/Art")
    import_folder("Icon", "/Game/ReEcho/Textures/UI/Cards/Icon")
    unreal.log("[Plan69] done")
