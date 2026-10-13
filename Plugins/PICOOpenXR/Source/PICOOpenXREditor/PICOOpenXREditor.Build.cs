// Copyright 2023 PICO Inc. All Rights Reserved.

using UnrealBuildTool;

public class PICOOpenXREditor : ModuleRules
{
    public PICOOpenXREditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "EditorFramework",
                "UnrealEd",
                "PICOOpenXRRuntimeSettings",
                "OpenXRHMD"
            }
        );
    }
}