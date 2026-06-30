// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MarsGreenhouse : ModuleRules
{
	public MarsGreenhouse(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"MarsGreenhouse",
			"MarsGreenhouse/Variant_Strategy",
			"MarsGreenhouse/Variant_Strategy/UI",
			"MarsGreenhouse/Variant_TwinStick",
			"MarsGreenhouse/Variant_TwinStick/AI",
			"MarsGreenhouse/Variant_TwinStick/Gameplay",
			"MarsGreenhouse/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
