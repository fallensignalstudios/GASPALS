using UnrealBuildTool;
using System.IO;

public class SignalForgeRPG : ModuleRules
{
    public SignalForgeRPG(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "GameplayTags",
            "GameplayTasks",
            "GameplayAbilities",
            "AIModule",
            "NavigationSystem",
            "Niagara",
            "UMG",
            "Slate",
            "SlateCore",
            "AnimGraphRuntime",
            "LevelSequence",
            "MovieScene",
            "DeveloperSettings",
            "NetCore"
        });

        PublicIncludePaths.AddRange(new string[]
        {
            ModuleDirectory
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}