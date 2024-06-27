// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GRL_SimonKeller : ModuleRules
{
	public GRL_SimonKeller(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
