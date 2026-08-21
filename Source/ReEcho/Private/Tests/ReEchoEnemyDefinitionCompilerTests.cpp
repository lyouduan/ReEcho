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
	TestTrue(TEXT("Rabbit phase two enabled by authoritative data"), Rabbit.Phase2.bEnabled);
	TestEqual(TEXT("Rabbit phase two animation set"), Rabbit.Phase2.AnimationSetId, FName(TEXT("Phase2")));
	TestEqual(TEXT("Rabbit phase two attack threshold from data"), Rabbit.Phase2.RequiredAttackCount, 2);
	TestEqual(TEXT("Rabbit phase two aggro range from data"), Rabbit.Phase2.TriggerRangeCm, 1500.0f);
	TestEqual(TEXT("Rabbit phase two transform seconds from data"), Rabbit.Phase2.TransformSeconds, 1.0f);
	// Boss two-stage data is authored in WS4; the authoritative Enemies worksheet currently leaves Boss Phase2 disabled.
	TestFalse(TEXT("Boss phase two not enabled until WS4 authoritative two-stage data"), Boss.Phase2.bEnabled);
	return true;
}

#endif
