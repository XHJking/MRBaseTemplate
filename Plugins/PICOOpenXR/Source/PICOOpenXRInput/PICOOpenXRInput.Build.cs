// Copyright 2023 PICO Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class PICOOpenXRInput : ModuleRules
{
    public PICOOpenXRInput(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "HeadMountedDisplay",
                    "XRBase",
                    "InputCore",
                    "OpenXRHMD",
                    "Slate",
                    "SlateCore",
                    "PICOOpenXRLoader",
                    "PICOOpenXRRuntimeSettings"
            }
            );
    }
}