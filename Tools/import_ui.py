"""
import_ui.py - headless import of the UI fonts and standalone UI art.

Run with:
  UnrealEditor-Cmd.exe GameEngine.uproject -run=pythonscript -script=Tools/import_ui.py -unattended -nosplash -nullrhi

- ArtSource/Fonts/*.ttf            -> /Game/UI/Fonts   (FontFace assets, named FF_<file name>)
- ArtSource/FigmaUI/backgrounds/*.png and icons/**/*.png -> /Game/UI/Figma
  (the 5 style-guide pages directly in ArtSource/FigmaUI are NOT imported)
Safe to re-run: existing assets are replaced with the same content.
"""
import os
import unreal

PROJECT = unreal.Paths.project_dir()
FONT_DIR = os.path.join(PROJECT, "ArtSource", "Fonts")
FIGMA_DIR = os.path.join(PROJECT, "ArtSource", "FigmaUI")


def make_task(filename, dest_path, dest_name, factory=None):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filename)
    task.set_editor_property("destination_path", dest_path)
    task.set_editor_property("destination_name", dest_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    if factory is not None:
        task.set_editor_property("factory", factory)
    return task


def import_fonts():
    tasks = []
    if not os.path.isdir(FONT_DIR):
        unreal.log_warning("[import_ui] no font folder: " + FONT_DIR)
        return
    for f in sorted(os.listdir(FONT_DIR)):
        if f.lower().endswith((".ttf", ".otf")):
            name = "FF_" + os.path.splitext(f)[0].replace("-", "_")
            tasks.append(make_task(os.path.join(FONT_DIR, f), "/Game/UI/Fonts", name,
                                   unreal.FontFileImportFactory()))
    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
        for t in tasks:
            unreal.log("[import_ui] font imported: " + str(list(t.get_editor_property("imported_object_paths"))))


def import_textures():
    tasks = []
    for sub in ("backgrounds", "icons"):
        root = os.path.join(FIGMA_DIR, sub)
        if not os.path.isdir(root):
            continue
        for dirpath, _, files in os.walk(root):
            for f in sorted(files):
                if f.lower().endswith(".png"):
                    name = "T_" + os.path.splitext(f)[0].replace("-", "_").replace(" ", "_")
                    tasks.append(make_task(os.path.join(dirpath, f), "/Game/UI/Figma", name))
    if not tasks:
        unreal.log("[import_ui] no standalone PNGs under backgrounds/ or icons/; nothing to import")
        return
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for t in tasks:
        for path in t.get_editor_property("imported_object_paths"):
            tex = unreal.EditorAssetLibrary.load_asset(str(path).split(".")[0])
            if isinstance(tex, unreal.Texture2D):
                # crisp UI: no mips, no compression artifacts, UI texture group
                tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
                tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
                tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
                tex.set_editor_property("never_stream", True)
                unreal.EditorAssetLibrary.save_loaded_asset(tex)
                unreal.log("[import_ui] texture imported: " + str(path))


import_fonts()
import_textures()
unreal.log("[import_ui] done")
