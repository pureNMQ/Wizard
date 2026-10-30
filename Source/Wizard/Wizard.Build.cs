// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Wizard : ModuleRules
{
	public Wizard(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "NavigationSystem", "AIModule", "Niagara", "EnhancedInput", "GameplayAbilities", "GameplayTags", "GameplayTasks", "UMG", "CommonUI", "CommonInput" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// 启动配置检查仅在编辑器测试中使用，不引入成品包的编辑器依赖。
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "EngineSettings", "UnrealEd", "UMGEditor", "Kismet", "KismetCompiler", "AssetRegistry", "AnimGraph", "BlueprintGraph", "AnimGraphRuntime" });
		}
	}
}
