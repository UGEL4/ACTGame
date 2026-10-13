using UnrealBuildTool;

public class AlsSkeletonsEditor : ModuleRules {
	public AlsSkeletonsEditor(ReadOnlyTargetRules Target) : base(Target) {
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;

		// CppCompileWarningSettings.UnsafeTypeCastWarningLevel = WarningLevel.Warning;
		CppCompileWarningSettings.NonInlinedGenCppWarningLevel = WarningLevel.Warning;

		PublicDependencyModuleNames.AddRange([
			"Core", "CoreUObject", "Engine", "AnimGraphRuntime", "AnimationModifiers", "AnimationBlueprintLibrary", "ALS",
			"AssetTools", "UnrealEd"
		]);

		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange([
				"AnimGraph"
			]);

			PrivateDependencyModuleNames.AddRange([
				"BlueprintGraph"
			]);
		}
	}
}
