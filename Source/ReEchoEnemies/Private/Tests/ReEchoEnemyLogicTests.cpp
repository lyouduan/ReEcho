#include "Enemies/ReEchoEnemyLogicComponent.h"
#include "Enemies/ReEchoEnemyRosterComponent.h"

#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyLegacyDefinitionTest,
                                 "ReEcho.Enemies.Logic.LegacyDefinitions",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyLegacyDefinitionTest::RunTest(const FString& Parameters)
{
	const FReEchoEnemyDefinition Grunt =
	    ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Grunt);
	TestEqual(TEXT("Grunt health"), Grunt.MaxHealth, 28.0f);
	TestEqual(TEXT("Grunt speed"), Grunt.MoveSpeedCmPerSecond, 95.0f);
	TestEqual(TEXT("Grunt damage"), Grunt.ContactDamage, 9.0f);
	TestEqual(TEXT("Grunt interval"), Grunt.AttackIntervalSeconds, 1.3f);

	const FReEchoEnemyDefinition Shield =
	    ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Shield);
	TestEqual(TEXT("Shield health"), Shield.MaxHealth, 55.0f);
	TestTrue(TEXT("Shield retains directional defense policy"), Shield.bUsesDirectionalShield);

	const FReEchoEnemyDefinition Bomber =
	    ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Bomber, 300.0f, 120.0f, 0.8f, 31.0f);
	TestEqual(TEXT("Bomber health"), Bomber.MaxHealth, 16.0f);
	TestEqual(TEXT("Bomber trigger comes from host tuning"), Bomber.BomberTriggerRadiusCm, 300.0f);
	TestEqual(TEXT("Bomber damage radius comes from host tuning"), Bomber.BomberDamageRadiusCm, 120.0f);
	TestEqual(TEXT("Bomber fuse comes from host tuning"), Bomber.BomberFuseDurationSeconds, 0.8f);
	TestEqual(TEXT("Bomber damage comes from host tuning"), Bomber.ContactDamage, 31.0f);

	const FReEchoEnemyDefinition Boss =
	    ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Boss);
	TestEqual(TEXT("Boss health"), Boss.MaxHealth, 650.0f);
	TestEqual(TEXT("Boss knockback speed"), Boss.KnockbackSpeedCmPerSecond, 140.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyContactCadenceTest,
                                 "ReEcho.Enemies.Logic.ContactCadence",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyContactCadenceTest::RunTest(const FString& Parameters)
{
	UReEchoEnemyLogicComponent* Logic = NewObject<UReEchoEnemyLogicComponent>();
	TestTrue(TEXT("Logic initializes"),
	         Logic->Initialize(ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Grunt), 7));

	FReEchoEnemySenseSnapshot Sense;
	Sense.bTargetExists = true;
	Sense.bTargetAlive = true;
	Sense.SelfLocation = FVector::ZeroVector;
	Sense.TargetLocation = FVector(80.0f, 0.0f, 0.0f);

	const FReEchoEnemyActionIntent First = Logic->Advance(Sense, 0.1f);
	TestTrue(TEXT("Ready contact attack commits"), First.bAttackCommitted);
	TestTrue(TEXT("Target in 80cm range still receives movement before contact"), First.bHasMovement);
	TestEqual(TEXT("First attack sequence"), First.Attack.Sequence, int64(1));
	TestEqual(TEXT("Committed attack starts legacy cooldown"),
	          Logic->GetSnapshot().AttackCooldownRemainingSeconds,
	          1.3f);

	const FReEchoEnemyActionIntent Busy = Logic->Advance(Sense, 0.5f);
	TestFalse(TEXT("Cooldown blocks another contact attack"), Busy.bAttackCommitted);
	FReEchoEnemySenseSnapshot NoTarget;
	Logic->Advance(NoTarget, 1.0f);
	TestEqual(TEXT("Missing target preserves the legacy paused cooldown"),
	          Logic->GetSnapshot().AttackCooldownRemainingSeconds,
	          0.8f);
	const FReEchoEnemyActionIntent ReadyAgain = Logic->Advance(Sense, 0.8f);
	TestTrue(TEXT("Exact accumulated interval permits next attack"), ReadyAgain.bAttackCommitted);
	TestEqual(TEXT("Second sequence is monotonic"), ReadyAgain.Attack.Sequence, int64(2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyInvulnerableTargetTest,
                                 "ReEcho.Enemies.Logic.InvulnerableTargetConsumesAttack",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyInvulnerableTargetTest::RunTest(const FString& Parameters)
{
	UReEchoEnemyLogicComponent* Logic = NewObject<UReEchoEnemyLogicComponent>();
	Logic->Initialize(ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Grunt), 1);
	FReEchoEnemySenseSnapshot Sense;
	Sense.bTargetExists = true;
	Sense.bTargetAlive = true;
	Sense.bTargetInvulnerable = true;
	Sense.TargetLocation = FVector(50.0f, 0.0f, 0.0f);

	const FReEchoEnemyActionIntent Intent = Logic->Advance(Sense, 0.0f);
	TestTrue(TEXT("Attack action still commits while target is invulnerable"), Intent.bAttackCommitted);
	TestFalse(TEXT("Invulnerable target is not a damage candidate"), Intent.bCanDamageTarget);
	TestEqual(TEXT("Invulnerable action consumes cooldown"),
	          Logic->GetSnapshot().AttackCooldownRemainingSeconds,
	          1.3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyBomberFuseTest,
                                 "ReEcho.Enemies.Logic.BomberFuse",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyBomberFuseTest::RunTest(const FString& Parameters)
{
	UReEchoEnemyLogicComponent* Logic = NewObject<UReEchoEnemyLogicComponent>();
	Logic->Initialize(
	    ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Bomber, 260.0f, 180.0f, 1.2f, 22.0f),
	    2);
	FReEchoEnemySenseSnapshot Sense;
	Sense.bTargetExists = true;
	Sense.bTargetAlive = true;
	Sense.TargetLocation = FVector(100.0f, 0.0f, 0.0f);

	const FReEchoEnemyActionIntent FuseStart = Logic->Advance(Sense, 0.2f);
	TestFalse(TEXT("Starting fuse does not explode immediately"), FuseStart.bAttackCommitted);
	TestTrue(TEXT("Fuse becomes active"), Logic->GetSnapshot().bFuseActive);
	TestEqual(TEXT("Fuse advances in the trigger frame"), Logic->GetSnapshot().FuseRemainingSeconds, 1.0f);

	Sense.TargetLocation = FVector(400.0f, 0.0f, 0.0f);
	const FReEchoEnemyActionIntent Continued = Logic->Advance(Sense, 0.5f);
	TestFalse(TEXT("Leaving trigger radius does not cancel fuse"), Continued.bAttackCommitted);
	TestTrue(TEXT("Fuse remains active outside trigger radius"), Logic->GetSnapshot().bFuseActive);

	Sense.TargetLocation = FVector(100.0f, 0.0f, 0.0f);
	Sense.bTargetInvulnerable = true;
	const FReEchoEnemyActionIntent Explosion = Logic->Advance(Sense, 0.5f);
	TestTrue(TEXT("Fuse expiry commits explosion"), Explosion.bAttackCommitted);
	TestTrue(TEXT("Explosion always requests self destruction"), Explosion.bSelfDestructAfterAttack);
	TestFalse(TEXT("Invulnerable target takes no explosion damage"), Explosion.bCanDamageTarget);
	TestEqual(TEXT("Bomber attack sequence"), Explosion.Attack.Sequence, int64(1));
	TestTrue(TEXT("Self destruct is recorded as a one-shot action"),
	         Logic->GetSnapshot().bSelfDestructCommitted);
	TestFalse(TEXT("Expired fuse cannot publish a second explosion"),
	          Logic->Advance(Sense, 0.1f).bAttackCommitted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHurtAndSnapshotTest,
                                 "ReEcho.Enemies.Logic.HurtAndSnapshot",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHurtAndSnapshotTest::RunTest(const FString& Parameters)
{
	const FReEchoEnemyDefinition Definition =
	    ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Grunt);
	UReEchoEnemyLogicComponent* Logic = NewObject<UReEchoEnemyLogicComponent>();
	Logic->Initialize(Definition, 3);
	Logic->NotifyHurt(5.0f, FVector(-100.0f, 0.0f, 0.0f), FVector::ZeroVector);

	const FReEchoEnemyLogicSnapshot Hurt = Logic->GetSnapshot();
	TestEqual(TEXT("Hurt starts gameplay reaction"), Hurt.Phase, EReEchoEnemyBehaviorPhase::HitReaction);
	TestEqual(TEXT("Knockback points away from source"), Hurt.KnockbackVelocity, FVector(360.0f, 0.0f, 0.0f));

	FReEchoEnemySenseSnapshot NoTarget;
	const FReEchoEnemyActionIntent Reaction = Logic->Advance(NoTarget, 0.1f);
	TestTrue(TEXT("Reaction produces world movement intent"), Reaction.bHasMovement);
	TestEqual(TEXT("First reaction movement uses authoritative velocity"),
	          Reaction.MovementDelta,
	          FVector(36.0f, 0.0f, 0.0f));

	FReEchoEnemyLogicSnapshot Saved = Logic->GetSnapshot();
	Saved.AttackCooldownRemainingSeconds = 0.75f;
	Saved.AttackSequence = 9;
	UReEchoEnemyLogicComponent* Restored = NewObject<UReEchoEnemyLogicComponent>();
	Restored->Initialize(Definition, 3);
	Restored->RestoreSnapshot(Saved);
	TestEqual(TEXT("Snapshot restores cooldown"), Restored->GetSnapshot().AttackCooldownRemainingSeconds, 0.75f);
	TestEqual(TEXT("Snapshot restores attack identity sequence"), Restored->GetSnapshot().AttackSequence, int64(9));

	Restored->NotifyDeath();
	TestEqual(TEXT("Death stops behavior"), Restored->GetSnapshot().Phase, EReEchoEnemyBehaviorPhase::Dead);
	TestFalse(TEXT("Dead logic emits no action"), Restored->Advance(NoTarget, 1.0f).bAttackCommitted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyRosterTest,
                                 "ReEcho.Enemies.Logic.Roster",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyRosterTest::RunTest(const FString& Parameters)
{
	UReEchoEnemyRosterComponent* Roster = NewObject<UReEchoEnemyRosterComponent>();
	AActor* FirstHost = NewObject<AActor>();
	AActor* SecondHost = NewObject<AActor>();
	UReEchoEnemyLogicComponent* FirstLogic = NewObject<UReEchoEnemyLogicComponent>(FirstHost);
	UReEchoEnemyLogicComponent* SecondLogic = NewObject<UReEchoEnemyLogicComponent>(SecondHost);
	FirstLogic->Initialize(ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Grunt), 2);
	SecondLogic->Initialize(ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Bomber), 1);

	TestTrue(TEXT("First enemy registers"), Roster->RegisterEnemy(FirstHost, FirstLogic));
	TestTrue(TEXT("Second enemy registers"), Roster->RegisterEnemy(SecondHost, SecondLogic));
	TestFalse(TEXT("Host cannot be registered twice"), Roster->RegisterEnemy(FirstHost, FirstLogic));
	TestEqual(TEXT("Both enemies initially count as living"), Roster->GetLivingEnemyCount(), 2);

	const TArray<FReEchoEnemyRosterEntrySnapshot> Entries = Roster->GetEntries();
	TestEqual(TEXT("Roster snapshot is stable by spawn index"), Entries[0].SpawnIndex, 1);
	TestEqual(TEXT("Second stable entry follows"), Entries[1].SpawnIndex, 2);

	SecondLogic->NotifyDeath();
	TestEqual(TEXT("Combat death reflected without a copied roster life flag"), Roster->GetLivingEnemyCount(), 1);
	TestEqual(TEXT("Only the living host is returned"), Roster->GetLivingEnemyActors()[0].Get(), FirstHost);

	Roster->UnregisterEnemy(FirstHost);
	TestFalse(TEXT("No living enemies remain after unregister"), Roster->HasLivingEnemies());
	Roster->ResetRoster();
	TestEqual(TEXT("Reset removes retained dead entries"), Roster->GetEntries().Num(), 0);
	return true;
}

#endif
