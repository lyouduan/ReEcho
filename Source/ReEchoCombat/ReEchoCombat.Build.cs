using UnrealBuildTool;

public class ReEchoCombat : ModuleRules
{
	public ReEchoCombat(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivatePCHHeaderFile = "Private/ReEchoCombatPrivatePCH.h";
		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core", "CoreUObject", "Engine", "GameplayAbilities", "GameplayTags", "GameplayTasks"
		});
	}
}
