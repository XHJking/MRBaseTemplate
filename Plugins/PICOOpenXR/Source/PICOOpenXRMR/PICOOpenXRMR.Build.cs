// Copyright 2023 PICO Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class PICOOpenXRMR : ModuleRules
{
    public PICOOpenXRMR(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "HeadMountedDisplay",
                    "OpenXRHMD",
                    "RHI",
                    "RenderCore",
                    "PICOOpenXRLoader",
                    "PICOOpenXRRuntimeSettings",
                    "ProceduralMeshComponent",
                    "MRMesh"
            }
            );
    }
}