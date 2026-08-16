// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ACTGame : ModuleRules
{
	public ACTGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		OptimizeCode = CodeOptimization.InShippingBuildsOnly;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
            "EnTT",
            "EventPP",
            "ALS", // ALS-Refactored
            "ALSCamera"
        });

		PrivateDependencyModuleNames.AddRange(new string[] {
            "EngineSettings"
        });

		PublicIncludePaths.AddRange(new string[] {
			"ACTGame",
			"ACTGame/Variant_Platforming",
			"ACTGame/Variant_Combat",
			"ACTGame/Variant_Combat/AI",
			"ACTGame/Variant_SideScrolling",
			"ACTGame/Variant_SideScrolling/Gameplay",
			"ACTGame/Variant_SideScrolling/AI",
            //Path.Combine(ModuleDirectory, "../../Plugins/ALS-Community-main/Source")
        });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
