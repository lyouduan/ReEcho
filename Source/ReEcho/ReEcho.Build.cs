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
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/reecho_data_manifest.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/csv_schema.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/runtime_smoke.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/runtime_smoke_effects.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/characters.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/character_aliases.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/cards.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/card_effects.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/elements.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/statuses.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/reactions.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/weapon_types.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/weapons.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/attack_steps.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/slot_types.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/slot_profiles.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/parts.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/part_effects.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/enemies.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/enemy_abilities.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/boss_phases.csv", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Data/audio_events.csv", StagedFileType.NonUFS);
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
