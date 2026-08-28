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
            "ReEchoCards",
            "ReEchoCombat",
            "ReEchoEnemies",
			"ReEchoPresentation",
            "ReEchoWeapons",
            "Paper2D",
			"MediaAssets",
			"ImgMedia",
            "UMG",
            "Slate",
            "SlateCore"
        });
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/*.csv", StagedFileType.NonUFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/Movies/EncounterTransition/EncounterTransitionAlpha.mov", StagedFileType.NonUFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/Movies/EncounterTransition/EncounterEndToCardChoiceV2.mov", StagedFileType.NonUFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/Movies/EncounterTransition/CardChoiceToShop.mov", StagedFileType.NonUFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/Movies/EncounterTransition/Stage01To02.mov", StagedFileType.NonUFS);

        // One-way dependency: gameplay -> ReEchoAudio audio runtime module.
        // ReEchoAudio must never depend back on ReEcho (see Plan33).
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "ReEchoAudio",
            "Niagara"
        });
    }
}
