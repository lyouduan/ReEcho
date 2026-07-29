using UnrealBuildTool;

public class ReEcho : ModuleRules
{
    public ReEcho(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "DeveloperSettings",
            "Json",
            "GameplayAbilities",
            "GameplayTags",
            "GameplayTasks",
            "UMG",
            "Slate",
            "SlateCore"
        });
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/cards.json");
    }
}