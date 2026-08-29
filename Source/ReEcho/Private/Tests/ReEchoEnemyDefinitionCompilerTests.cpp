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
	// WS4 (Plan 68): the authoritative boss is M_SHEEP (blood-bar-depleted two-form). Validate its compiled shape.
	if (!TestTrue(TEXT("Stable boss id compiles"),
	              ReEchoEnemyDefinitionCompiler::Compile(*LoadResult.Snapshot, TEXT("M_SHEEP"), Boss, Error)))
	{
		AddError(Error);
		return false;
	}
	TestEqual(TEXT("Boss archetype compiles"), Boss.Archetype, EReEchoEnemyArchetype::Boss);
	TestEqual(TEXT("Boss health compiles from the one-phase maximum"), Boss.MaxHealth, 3000.0f);
	TestEqual(
	    TEXT("Boss has five configured behaviors (melee + volley + spread + blink + beam)"), Boss.Abilities.Num(), 5);
	TestEqual(TEXT("Deterministic rotation starts with the melee basic attack"),
	          Boss.Abilities[0].BehaviorId,
	          FName(TEXT("Boss.MeleeSweep")));
	TestEqual(TEXT("Boss ships two phases: one-form and blood-depleted two-form"), Boss.BossPhases.Num(), 2);
	TestEqual(TEXT("Phase one uses no timed echo policy"), Boss.BossPhases[0].EchoPolicy, EReEchoBossEchoPolicy::None);
	TestEqual(TEXT("Phase one does not refill health"),
	          Boss.BossPhases[0].RefillHealthPolicy,
	          EReEchoBossRefillHealthPolicy::None);
	TestEqual(TEXT("Phase two refills to its blood-depleted maximum"),
	          Boss.BossPhases[1].RefillHealthPolicy,
	          EReEchoBossRefillHealthPolicy::RefillToMaximum);
	TestEqual(TEXT("Phase two maximum health is the black-form ceiling"), Boss.BossPhases[1].PhaseMaxHealth, 2000.0f);
	UReEchoEnemyLogicComponent* BossLogic = NewObject<UReEchoEnemyLogicComponent>();
	TestTrue(TEXT("Compiled production Boss definition initializes runtime policy"), BossLogic->Initialize(Boss, 1));

	FReEchoEnemyDefinition Missing;
	TestFalse(TEXT("Unknown EnemyId never falls back"),
	          ReEchoEnemyDefinitionCompiler::Compile(*LoadResult.Snapshot, TEXT("M_Unknown"), Missing, Error));
	TestTrue(TEXT("Unknown EnemyId reports explicit error"), Error.Contains(TEXT("unknown or disabled")));
	FReEchoEnemyDefinition Rabbit;
	TestTrue(TEXT("Rabbit definition compiles"),
	         ReEchoEnemyDefinitionCompiler::Compile(*LoadResult.Snapshot, TEXT("M_RABBIT"), Rabbit, Error));
	TestFalse(TEXT("Rabbit dual forms remain presentation-only in authoritative data"), Rabbit.Phase2.bEnabled);
	TestEqual(TEXT("Rabbit phase two animation set"), Rabbit.Phase2.AnimationSetId, FName(TEXT("Phase2")));
	TestEqual(TEXT("Rabbit phase two attack threshold from data"), Rabbit.Phase2.RequiredAttackCount, 2);
	TestEqual(TEXT("Rabbit phase two aggro range from data"), Rabbit.Phase2.TriggerRangeCm, 1500.0f);
	TestEqual(TEXT("Rabbit phase two transform seconds from data"), Rabbit.Phase2.TransformSeconds, 1.0f);
	TestTrue(TEXT("Boss blood-depleted phase two is enabled by authoritative data"), Boss.Phase2.bEnabled);
	TestEqual(TEXT("Boss phase two uses the health-threshold trigger"),
	          Boss.Phase2.TriggerMode,
	          EReEchoEnemyPhase2TriggerMode::HealthThreshold);
	TestEqual(TEXT("Boss phase two triggers at an empty health bar"), Boss.Phase2.HealthThresholdRatio, 0.0f);
	FReEchoEnemyDefinition Fox;
	TestTrue(TEXT("Fox definition compiles"),
	         ReEchoEnemyDefinitionCompiler::Compile(*LoadResult.Snapshot, TEXT("M_FOX"), Fox, Error));
	TestEqual(TEXT("Fox remains an elite dash enemy"), Fox.Archetype, EReEchoEnemyArchetype::Elite);
	TestFalse(TEXT("Elite fox does not inherit shield-only frontal immunity"), Fox.bUsesDirectionalShield);
	FReEchoEnemyDefinition Shield;
	TestTrue(TEXT("Shield definition compiles"),
	         ReEchoEnemyDefinitionCompiler::Compile(*LoadResult.Snapshot, TEXT("M_Shield"), Shield, Error));
	TestTrue(TEXT("Shield enemy retains directional defense"), Shield.bUsesDirectionalShield);
	return true;
}

#endif
