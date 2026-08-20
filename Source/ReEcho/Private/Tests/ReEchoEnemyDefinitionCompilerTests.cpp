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
	const FReEchoCsvLoadResult LoadResult =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
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
	TestEqual(TEXT("Boss has five configured behaviors"), Boss.Abilities.Num(), 5);
	TestEqual(TEXT("Cleanse remains first only because passive order is zero"),
	          Boss.Abilities[0].BehaviorId,
	          FName(TEXT("Boss.ElementCleanse")));
	TestEqual(TEXT("Active deterministic rotation starts with melee sweep"),
	          Boss.Abilities[1].BehaviorId,
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
	FReEchoEnemyDefinition Rabbit;
	TestTrue(TEXT("Rabbit definition compiles"),
	         ReEchoEnemyDefinitionCompiler::Compile(*LoadResult.Snapshot, TEXT("M_RABBIT"), Rabbit, Error));
	TestTrue(TEXT("Rabbit temporarily enables phase two"), Rabbit.Phase2.bEnabled);
	TestEqual(TEXT("Rabbit phase two animation set"), Rabbit.Phase2.AnimationSetId, FName(TEXT("Phase2")));
	TestEqual(TEXT("Rabbit phase two attack threshold"), Rabbit.Phase2.RequiredAttackCount, 2);
	TestEqual(TEXT("Rabbit phase two aggro range"), Rabbit.Phase2.TriggerRangeCm, 300.0f);
	TestTrue(TEXT("Boss also exposes the shared temporary phase two"), Boss.Phase2.bEnabled);
	TestEqual(TEXT("Every transformable enemy uses two health reductions"), Boss.Phase2.RequiredAttackCount, 2);
	TestEqual(
	    TEXT("Every transformable enemy selects Phase2 animations"), Boss.Phase2.AnimationSetId, FName(TEXT("Phase2")));
	return true;
}

#endif
