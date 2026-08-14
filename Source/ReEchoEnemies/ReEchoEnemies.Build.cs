using UnrealBuildTool;

public class ReEchoEnemies : ModuleRules
{
	public ReEchoEnemies(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivatePCHHeaderFile = "Private/ReEchoEnemiesPrivatePCH.h";
		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core", "CoreUObject", "Engine", "ReEchoCombat"
		});
	}
}
