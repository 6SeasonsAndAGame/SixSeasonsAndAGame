// Copyright(c) 2017 Venugopalan Sreedharan. All rights reserved.

namespace UnrealBuildTool.Rules
{
	public class DonMeshPainting : ModuleRules
	{
		public DonMeshPainting(ReadOnlyTargetRules Target) : base(Target)
		{
            bEnableShadowVariableWarnings = false;

            PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

            PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					// ... add other private include paths required here ...
                }
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					// ... add other public dependencies that you statically link with here ...                    
                    "AIModule",
                    "GameplayTasks",
                    "Engine",
                    "RHI", "RenderCore", "Landscape",
                    "SlateCore", // text rendering
                    "ImageWrapper" // png operations
                }
				);

            PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add private dependencies that you statically link with here ...
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);
		}
	}
}