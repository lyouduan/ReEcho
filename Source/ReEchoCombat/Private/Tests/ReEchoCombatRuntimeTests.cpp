#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ReEchoAttackControllerComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCombatSingleCadenceTest,
                                 "ReEcho.Combat.SingleCadence",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCombatSingleCadenceTest::RunTest(const FString& Parameters)
{
	FReEchoWeaponCadence Cadence;
	FReEchoAttackCommitId FirstCommit;
	TestTrue(TEXT("Fresh cadence accepts the first attack"), Cadence.TryCommit(0.28f, FirstCommit));
	TestTrue(TEXT("First commit id is valid"), FirstCommit.IsValid());
	TestEqual(TEXT("Weapon interval is the only readiness timer"), Cadence.GetRemaining(), 0.28f);

	FReEchoAttackCommitId RejectedCommit;
	TestFalse(TEXT("A second request is rejected while cadence is busy"), Cadence.TryCommit(0.80f, RejectedCommit));
	TestFalse(TEXT("Rejected requests do not receive a commit id"), RejectedCommit.IsValid());

	// A step may visually last 0.80 seconds, but only the 0.28 weapon interval advances readiness.
	Cadence.Tick(0.27f);
	TestFalse(TEXT("Cadence remains busy before the configured interval"), Cadence.IsReady());
	Cadence.Tick(0.01f);
	TestTrue(TEXT("Cadence becomes ready at the configured interval, independent of step duration"), Cadence.IsReady());

	FReEchoAttackCommitId SecondCommit;
	TestTrue(TEXT("Held input can commit again after readiness"), Cadence.TryCommit(0.28f, SecondCommit));
	TestTrue(TEXT("Every successful commit gets a unique monotonic id"), SecondCommit.Value > FirstCommit.Value);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCombatAttackModeCommandTest,
                                 "ReEcho.Combat.AttackModeCommand",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCombatAttackModeCommandTest::RunTest(const FString& Parameters)
{
	UReEchoAttackControllerComponent* Controller = NewObject<UReEchoAttackControllerComponent>();
	TestEqual(TEXT("Automatic attack is the default"), Controller->GetAttackMode(), EReEchoAttackMode::Automatic);
	Controller->SetAttackMode(EReEchoAttackMode::Manual);
	TestEqual(TEXT("The typed command changes mode"), Controller->GetAttackMode(), EReEchoAttackMode::Manual);
	TestFalse(TEXT("Changing mode releases manual input"), Controller->IsManualHeld());
	TestFalse(TEXT("Changing mode releases automatic input"), Controller->IsAutomaticHeld());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCombatantSnapshotTest,
                                 "ReEcho.Combat.CombatantSnapshot",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCombatantSnapshotTest::RunTest(const FString& Parameters)
{
	UReEchoCombatantComponent* Combatant = NewObject<UReEchoCombatantComponent>();
	FReEchoStatBlock Stats;
	Stats.HpMax = 125.0f;
	Stats.PhysicalAttack = 17.0f;
	Stats.AttackSpeed = 1.25f;
	Combatant->InitializeFromStats(Stats, true);

	const FReEchoCombatantSnapshot Snapshot = Combatant->GetSnapshot();
	TestEqual(TEXT("Snapshot identifies its combatant"), Snapshot.Combatant.Get(), Combatant);
	TestEqual(TEXT("Snapshot captures current health"), Snapshot.CurrentHealth, 125.0f);
	TestEqual(TEXT("Snapshot captures maximum health"), Snapshot.MaximumHealth, 125.0f);
	TestEqual(TEXT("Snapshot captures combat stats"), Snapshot.Stats.PhysicalAttack, 17.0f);
	TestTrue(TEXT("Snapshot captures alive state"), Snapshot.bAlive);
	return true;
}

#endif
