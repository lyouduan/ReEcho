using UnrealBuildTool;

public class ReEchoWeapons : ModuleRules
{
	public ReEchoWeapons(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivatePCHHeaderFile = "Private/ReEchoWeaponsPrivatePCH.h";
		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core", "CoreUObject", "Engine", "ReEchoCombat"
		});
	}
}
