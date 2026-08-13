#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ReEchoAttackControllerComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Misc/AutomationTest.h"

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
