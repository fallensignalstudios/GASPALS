using UnrealBuildTool;

public class SignalForgeEditor : ModuleRules
{
    public SignalForgeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "SignalForgeRPG"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
{
    "UnrealEd",
    "Slate",
    "SlateCore",
    "EditorStyle",
    "ToolMenus",
    "AssetTools",
    "ContentBrowser",
    "EditorFramework",
    "PropertyEditor",
    "Projects",
    "InputCore",
    "GraphEditor",
    "Kismet",
    "AnimGraphRuntime",
    "LevelEditor",
    "ApplicationCore"
});
    }
}