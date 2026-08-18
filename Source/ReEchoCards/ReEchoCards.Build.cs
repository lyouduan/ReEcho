using UnrealBuildTool;

public class ReEchoCards : ModuleRules
{
    public ReEchoCards(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "ReEchoCombat"
        });
    }
}
