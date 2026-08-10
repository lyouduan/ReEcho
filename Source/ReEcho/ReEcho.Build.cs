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
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/reecho_data_manifest.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/csv_schema.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/runtime_smoke.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/runtime_smoke_effects.csv", StagedFileType.NonUFS);
    }
}
