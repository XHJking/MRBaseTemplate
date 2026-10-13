"""
MRBasePlugin - Create Lv_MR level
Run inside the Unreal Editor (Window > Developer Tools > Output Log, set command line to Python):
    `py "Plugins/MRBasePlugin/Content/Python/create_mr_level.py"`

This creates an empty level '/MRBasePlugin/Maps/Lv_MR' and saves it.
The level inherits the global game mode (MRGameMode) already configured in DefaultEngine.ini,
so the passthrough (video see-through) background and the white-box sphere/cube will be
spawned automatically at runtime - no manual actor placement required.
"""

import unreal
import os

unreal.log("MRBasePlugin: creating Lv_MR level...")

LEVEL_PATH = "/MRBasePlugin/Maps/Lv_MR"

try:
    editor_level_subsystem = unreal.get_editor_subsystem(unreal.EditorLevelSubsystem)
except Exception as e:
    unreal.log_error("MRBasePlugin: EditorLevelSubsystem not available: %s" % e)
    editor_level_subsystem = None

created = False

if editor_level_subsystem is not None:
    # UE 5.x: new_level creates an empty level and loads it as current.
    try:
        unreal.EditorLevelUtils.new_level(LEVEL_PATH)
        created = True
    except Exception:
        try:
            editor_level_subsystem.new_level(LEVEL_PATH)
            created = True
        except Exception as e:
            unreal.log_error("MRBasePlugin: new_level failed: %s" % e)

    if created:
        try:
            unreal.EditorLevelUtils.save_current_level()
        except Exception:
            try:
                editor_level_subsystem.save_current_level()
            except Exception as e:
                unreal.log_error("MRBasePlugin: save_current_level failed: %s" % e)

if not created:
    unreal.log_warning(
        "MRBasePlugin: Automatic level creation failed. Please create the level manually:\n"
        "  1) File > New Level...\n"
        "  2) Choose 'Empty Level'\n"
        "  3) Save as: Plugins/MRBasePlugin/Content/Maps/Lv_MR"
    )
else:
    unreal.log("MRBasePlugin: Lv_MR created and saved at %s" % LEVEL_PATH)