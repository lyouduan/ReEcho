using UnrealBuildTool;

public class ReEchoPresentation : ModuleRules
{
    public ReEchoPresentation(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivatePCHHeaderFile = "Private/ReEchoPresentationPrivatePCH.h";
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "GameplayTags", "Paper2D"
        });
    }
}
