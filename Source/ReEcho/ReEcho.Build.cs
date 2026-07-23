using UnrealBuildTool;

public class ReEcho : ModuleRules
{
    public ReEcho(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "DeveloperSettings", "Json", "JsonUtilities", "Niagara", "UMG", "Slate", "SlateCore" });
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/cards.json");
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/weapons.json");
    }
}


