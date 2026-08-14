#include "Data/ReEchoEnemyDefinitionCompiler.h"

#include "Data/ReEchoCsvDataRegistry.h"
#include "Enemies/ReEchoEnemyLogicComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyDefinitionCompilerTest,
                                 "ReEcho.Data.Enemies.Compiler",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyDefinitionCompilerTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult = FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(
	    FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	if (!TestTrue(TEXT("Production CSV snapshot loads"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}

	FReEchoEnemyDefinition Boss;
	FString Error;
	if (!TestTrue(TEXT("Stable boss id compiles"),
	              ReEchoEnemyDefinitionCompiler::Compile(*LoadResult.Snapshot, TEXT("M_TimeGuard"), Boss, Error)))
	{
		AddError(Error);
		return false;
	}
	TestEqual(TEXT("Boss archetype compiles"), Boss.Archetype, EReEchoEnemyArchetype::Boss);
	TestEqual(TEXT("Boss health compiles without legacy fallback"), Boss.MaxHealth, 650.0f);
	TestEqual(TEXT("Boss has five configured behaviors"), Boss.BossAbilities.Num(), 5);
	TestEqual(TEXT("Cleanse remains first only because passive order is zero"),
	          Boss.BossAbilities[0].BehaviorId,
	          FName(TEXT("Boss.ElementCleanse")));
	TestEqual(TEXT("Active deterministic rotation starts with melee sweep"),
	          Boss.BossAbilities[1].BehaviorId,
	          FName(TEXT("Boss.MeleeSweep")));
	TestEqual(TEXT("Boss thirty-second phase compiles"), Boss.BossPhases.Num(), 1);
	TestEqual(TEXT("Phase retires encounter echoes"),
	          Boss.BossPhases[0].EchoPolicy,
	          EReEchoBossEchoPolicy::RetireEncounterEchoes);
	TestEqual(TEXT("Phase does not refill health"),
	          Boss.BossPhases[0].RefillHealthPolicy,
	          EReEchoBossRefillHealthPolicy::None);
	UReEchoEnemyLogicComponent* BossLogic = NewObject<UReEchoEnemyLogicComponent>();
	TestTrue(TEXT("Compiled production Boss definition initializes runtime policy"), BossLogic->Initialize(Boss, 1));

	FReEchoEnemyDefinition Missing;
	TestFalse(TEXT("Unknown EnemyId never falls back"),
	          ReEchoEnemyDefinitionCompiler::Compile(*LoadResult.Snapshot, TEXT("M_Unknown"), Missing, Error));
	TestTrue(TEXT("Unknown EnemyId reports explicit error"), Error.Contains(TEXT("unknown or disabled")));
	return true;
}

#endif
