// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class EpicGameJamGame : ModuleRules
{
	public EpicGameJamGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

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
			"Slate",
			"Paper2D",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"EpicGameJamGame",
			"EpicGameJamGame/DirectorsCut",
			"EpicGameJamGame/Variant_Horror",
			"EpicGameJamGame/Variant_Horror/UI",
			"EpicGameJamGame/Variant_Shooter",
			"EpicGameJamGame/Variant_Shooter/AI",
			"EpicGameJamGame/Variant_Shooter/UI",
			"EpicGameJamGame/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
