#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ReEchoCombatantComponent.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Encounter/ReEchoEncounterRuntime.h"
#include "Enemies/ReEchoEnemyLogicComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Misc/AutomationTest.h"

namespace
{
struct FReEchoStageTransitionWorldFixture
{
	UWorld* World = nullptr;

	FReEchoStageTransitionWorldFixture()
	{
		const FName WorldName =
		    MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("ReEchoStageTransitionWorld"));
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		World->AddToRoot();
		Context.SetCurrentWorld(World);
		World->SetShouldTick(true);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
	}

	~FReEchoStageTransitionWorldFixture()
	{
		if (World)
		{
			World->DestroyWorld(true);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoStageTransitionPolicyTest,
                                 "ReEcho.StageTransition.PolicyMatrix",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoStageTransitionPolicyTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	if (!TestTrue(TEXT("Production encounter catalog loads"), LoadResult.bSuccess) || !LoadResult.Snapshot.IsValid())
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}

	struct FExpectedTransition
	{
		int32 CompletedEncounterIndex = 0;
		bool bSameStage = false;
		bool bPreserveRoster = false;
		bool bPreservePlayer = false;
	};

	const TArray<FExpectedTransition> Cases = {
	    {0, false, false, false},
	    {1, true, true, true},
	    {2, false, false, false},
	    {3, true, true, true},
	    {4, true, true, true},
	    {5, false, false, false},
	    {6, true, true, true},
	    {7, false, false, false},
	};
	for (const FExpectedTransition& Expected : Cases)
	{
		FReEchoStageTransitionDecision Decision;
		FString Error;
		const FString Context = FString::Printf(
		    TEXT("Transition %d->%d"), Expected.CompletedEncounterIndex, Expected.CompletedEncounterIndex + 1);
		if (!TestTrue(*FString::Printf(TEXT("%s resolves"), *Context),
		              ReEchoStageTransition::Resolve(
		                  *LoadResult.Snapshot, Expected.CompletedEncounterIndex, Decision, Error)))
		{
			AddError(Error);
			continue;
		}
		TestEqual(*FString::Printf(TEXT("%s same Stage"), *Context), Decision.bSameStage, Expected.bSameStage);
		TestEqual(*FString::Printf(TEXT("%s roster policy"), *Context),
		          Decision.bPreserveEnemyRoster,
		          Expected.bPreserveRoster);
		TestEqual(*FString::Printf(TEXT("%s player policy"), *Context),
		          Decision.bPreservePlayerLocation,
		          Expected.bPreservePlayer);
	}

	FReEchoStageTransitionDecision MissingDecision;
	FString MissingError;
	TestFalse(TEXT("Transition beyond the configured final encounter fails closed"),
	          ReEchoStageTransition::Resolve(*LoadResult.Snapshot, 8, MissingDecision, MissingError));
	TestFalse(TEXT("Missing next encounter reports a diagnostic"), MissingError.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoStageTransitionWorldContinuityTest,
                                 "ReEcho.StageTransition.WorldContinuity",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoStageTransitionWorldContinuityTest::RunTest(const FString& Parameters)
{
	FReEchoStageTransitionWorldFixture Fixture;
	AReEchoEnemyActor* Enemy =
	    Fixture.World->SpawnActor<AReEchoEnemyActor>(FVector(321.0f, -187.0f, 50.0f), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Enemy Host spawns"), Enemy))
	{
		return false;
	}
	Enemy->Configure(EReEchoEnemyKind::Bomber, 42);
	Enemy->SetEnemyId(TEXT("M_PLAN54_STABLE"));
	Enemy->GetCombatantComponent()->RestoreCurrentHealth(17.0f);
	FReEchoEnemyLogicSnapshot Logic = Enemy->GetEnemyLogicComponent()->GetSnapshot();
	Logic.AttackCooldownRemainingSeconds = 0.75f;
	Logic.SpecialActionPhase = EReEchoEnemySpecialActionPhase::Windup;
	Logic.SpecialAbilityId = TEXT("A_TestWindup");
	Logic.SpecialActionRemainingSeconds = 0.5f;
	Logic.SpecialLockedTargetLocation = FVector(900.0f, 200.0f, 0.0f);
	Enemy->GetEnemyLogicComponent()->RestoreSnapshot(Logic);
	Enemy->GetCombatantComponent()->EditElementStateForTests().Attached = EReEchoElement::Water;
	Enemy->GetCombatantComponent()->EditElementStateForTests().bEnhancedNextReaction = true;

	AReEchoEnemyActor* const StableIdentity = Enemy;
	const FVector StableLocation = Enemy->GetActorLocation();
	const float StableHealth = Enemy->GetCombatantComponent()->CurrentHealth;
	Enemy->SetEncounterSimulationSuspended(true);
	Fixture.World->Tick(LEVELTICK_All, 1.0f);

	const FReEchoEnemyLogicSnapshot SuspendedLogic = Enemy->GetEnemyLogicComponent()->GetSnapshot();
	TestTrue(TEXT("Enemy Host enters explicit intermission suspension"), Enemy->IsEncounterSimulationSuspended());
	TestTrue(TEXT("Enemy Host object identity is retained"), Enemy == StableIdentity);
	TestEqual(
	    TEXT("Enemy Host location is retained while the UI world advances"), Enemy->GetActorLocation(), StableLocation);
	TestEqual(TEXT("Enemy current health is retained"), Enemy->GetCombatantComponent()->CurrentHealth, StableHealth);
	TestEqual(TEXT("Enemy stable EnemyId is retained"), Enemy->GetEnemyId(), FName(TEXT("M_PLAN54_STABLE")));
	TestEqual(TEXT("Enemy stable SpawnIndex is retained"), Enemy->GetSpawnIndex(), 42);
	TestEqual(TEXT("Encounter-scoped attachment is cleared during intermission"),
	          Enemy->GetElementState().Attached,
	          EReEchoElement::None);
	TestFalse(TEXT("Encounter-scoped enhancement is cleared during intermission"),
	          Enemy->GetElementState().bEnhancedNextReaction);
	TestEqual(TEXT("Persistent attack cooldown does not consume intermission time"),
	          SuspendedLogic.AttackCooldownRemainingSeconds,
	          0.75f);
	TestEqual(TEXT("Old encounter windup is cancelled"),
	          SuspendedLogic.SpecialActionPhase,
	          EReEchoEnemySpecialActionPhase::None);
	TestEqual(TEXT("Old encounter transient timer is cleared"), SuspendedLogic.SpecialActionRemainingSeconds, 0.0f);

	Enemy->SetEncounterSimulationSuspended(false);
	TestFalse(TEXT("Enemy Host resumes for the next encounter"), Enemy->IsEncounterSimulationSuspended());
	return true;
}

#endif
