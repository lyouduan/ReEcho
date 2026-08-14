#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ReEchoAttackControllerComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "GameFramework/Actor.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAttackIdentitySourceLifetimeTest,
	                             "ReEcho.Combat.AttackIdentity.SourceLifetime",
	                             EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAttackIdentitySourceLifetimeTest::RunTest(const FString& Parameters)
{
	FReEchoAttackIdentity EmptyIdentity;
	TestFalse(TEXT("Unassigned identity is invalid"), EmptyIdentity.IsValid());

	AActor* Source = NewObject<AActor>();
	FReEchoAttackIdentity Identity;
	Identity.Source = Source;
	Identity.Sequence = 7;
	const FReEchoAttackIdentity IdentityCopy = Identity;
	TestTrue(TEXT("Live source makes the identity valid"), Identity.IsValid());
	TestTrue(TEXT("Live source is available for optional feedback"), Identity.HasLiveSource());
	TestEqual(TEXT("Weak source resolves while live"), Identity.Source.Get(), Source);

	Source->MarkAsGarbage();
	TestTrue(TEXT("Destroyed source does not erase the assigned attack identity"), Identity.IsValid());
	TestTrue(TEXT("Identity comparison remains stable after source destruction"), Identity == IdentityCopy);
	TestFalse(TEXT("Destroyed source is unavailable for optional feedback"), Identity.HasLiveSource());
	TestNull(TEXT("Destroyed source cannot be dereferenced by delayed hit resolution"), Identity.Source.Get());
	return true;
}

#endif
