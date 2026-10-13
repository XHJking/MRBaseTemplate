using UnrealBuildTool;
using System.IO;

public class MRBasePlugin : ModuleRules
{
	public MRBasePlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"HeadMountedDisplay",
			"UMG",
			"PICOXRInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"PICOXRHMD",
			"PICOXRMR",
			"PXRPlugin",
			"XRBase",
			"ProceduralMeshComponent"
		});
	}
}