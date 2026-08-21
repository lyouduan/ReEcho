#include "Enemies/ReEchoEnemyLogicComponent.h"
#include "Enemies/ReEchoEnemyRosterComponent.h"

#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
FReEchoEnemyAbilityDefinition MakeBossAbility(
    const TCHAR* Id, const TCHAR* BehaviorId, const int32 SequenceOrder, const float MinRangeCm, const float MaxRangeCm)
{
	FReEchoEnemyAbilityDefinition Ability;
	Ability.Id = FName(Id);
	Ability.BehaviorId = FName(BehaviorId);
	Ability.SequenceOrder = SequenceOrder;
	Ability.Damage = 10.0f + SequenceOrder;
	Ability.WindupSeconds = 0.05f;
	Ability.ActiveSeconds = 0.05f;
	Ability.RecoverySeconds = 0.05f;
	Ability.CooldownSeconds = 0.5f;
	Ability.MinRangeCm = MinRangeCm;
	Ability.MaxRangeCm = MaxRangeCm;
	Ability.RadiusCm = 50.0f;
	Ability.WidthCm = 80.0f;
	Ability.LengthCm = 600.0f;
	Ability.LockTiming = EReEchoBossLockTiming::WindupStarted;
	Ability.bEnabled = true;
	return Ability;
}

FReEchoEnemyDefinition MakeBossTestDefinition()
{
	FReEchoEnemyDefinition Definition;
	Definition.Archetype = EReEchoEnemyArchetype::Boss;
	Definition.MaxHealth = 100.0f;
	Definition.MoveSpeedCmPerSecond = 60.0f;
	Definition.MovementStopDistanceCm = 50.0f;

	Definition.Abilities.Add(MakeBossAbility(TEXT("A_Melee"), TEXT("Boss.MeleeSweep"), 0, 0.0f, 250.0f));
	FReEchoEnemyAbilityDefinition Projectile =
	    MakeBossAbility(TEXT("A_Projectile"), TEXT("Boss.Projectile"), 1, 200.0f, 1200.0f);
	Projectile.ProjectileSpeedCmPerSecond = 700.0f;
	Projectile.LockTiming = EReEchoBossLockTiming::WindupEnded;
	Definition.Abilities.Add(Projectile);

	FReEchoEnemyAbilityDefinition Blink = MakeBossAbility(TEXT("A_Blink"), TEXT("Boss.BlinkSlam"), 2, 0.0f, 1000.0f);
	Blink.TeleportOffsetCm = 180.0f;
	Definition.Abilities.Add(Blink);
	Definition.Abilities.Add(MakeBossAbility(TEXT("A_Beam"), TEXT("Boss.PrayerBeam"), 3, 0.0f, 1200.0f));

	FReEchoEnemyAbilityDefinition Cleanse;
	Cleanse.Id = TEXT("A_Cleanse");
	Cleanse.BehaviorId = TEXT("Boss.ElementCleanse");
	Cleanse.CleanseIntervalSeconds = 0.15f;
	Cleanse.ImmunitySeconds = 0.1f;
	Cleanse.bEnabled = true;
	Definition.Abilities.Add(Cleanse);

	FReEchoBossPhaseDefinition Phase;
	Phase.Id = TEXT("P_Enrage");
	Phase.PhaseIndex = 1;
	Phase.TriggerSeconds = 0.2f;
	Phase.EchoPolicy = EReEchoBossEchoPolicy::RetireEncounterEchoes;
	Phase.PhysicalAttackMultiplier = 2.0f;
	Phase.ElementalAttackMultiplier = 2.0f;
	Phase.AttackSpeedMultiplier = 1.5f;
	Phase.MovementSpeedMultiplier = 1.25f;
	Phase.bEnabled = true;
	Definition.BossPhases.Add(Phase);
	return Definition;
}

const FReEchoBossIntent* FindBossIntent(const FReEchoEnemyActionIntent& Intent,
                                        const EReEchoBossIntentType Type,
                                        const FName AbilityId = NAME_None)
{
	return Intent.BossIntents.FindByPredicate(
	    [Type, AbilityId](const FReEchoBossIntent& BossIntent)
	    {
		    return BossIntent.Type == Type && (AbilityId.IsNone() || BossIntent.AbilityId == AbilityId);
	    });
}

int32 CountBossIntents(const FReEchoEnemyActionIntent& Intent, const EReEchoBossIntentType Type)
{
	int32 Count = 0;
	for (const FReEchoBossIntent& BossIntent : Intent.BossIntents)
	{
		Count += BossIntent.Type == Type ? 1 : 0;
	}
	return Count;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyLegacyDefinitionTest,
                                 "ReEcho.Enemies.Logic.LegacyDefinitions",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyLegacyDefinitionTest::RunTest(const FString& Parameters)
{
	const FReEchoEnemyDefinition Grunt = ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Grunt);
	TestEqual(TEXT("Grunt health"), Grunt.MaxHealth, 28.0f);
	TestEqual(TEXT("Grunt speed"), Grunt.MoveSpeedCmPerSecond, 95.0f);
	TestEqual(TEXT("Grunt damage"), Grunt.ContactDamage, 9.0f);
	TestEqual(TEXT("Grunt interval"), Grunt.AttackIntervalSeconds, 1.3f);

	const FReEchoEnemyDefinition Shield = ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Shield);
	TestEqual(TEXT("Shield health"), Shield.MaxHealth, 55.0f);
	TestTrue(TEXT("Shield retains directional defense policy"), Shield.bUsesDirectionalShield);

	const FReEchoEnemyDefinition Bomber =
	    ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Bomber, 300.0f, 120.0f, 0.8f, 31.0f);
	TestEqual(TEXT("Bomber health"), Bomber.MaxHealth, 16.0f);
	TestEqual(TEXT("Bomber trigger comes from host tuning"), Bomber.BomberTriggerRadiusCm, 300.0f);
	TestEqual(TEXT("Bomber damage radius comes from host tuning"), Bomber.BomberDamageRadiusCm, 120.0f);
	TestEqual(TEXT("Bomber fuse comes from host tuning"), Bomber.BomberFuseDurationSeconds, 0.8f);
	TestEqual(TEXT("Bomber damage comes from host tuning"), Bomber.ContactDamage, 31.0f);

	const FReEchoEnemyDefinition Boss = ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Boss);
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
	TestEqual(
	    TEXT("Committed attack starts legacy cooldown"), Logic->GetSnapshot().AttackCooldownRemainingSeconds, 1.3f);

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
	TestEqual(TEXT("Invulnerable action consumes cooldown"), Logic->GetSnapshot().AttackCooldownRemainingSeconds, 1.3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyBomberFuseTest,
                                 "ReEcho.Enemies.Logic.BomberFuse",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyBomberFuseTest::RunTest(const FString& Parameters)
{
	UReEchoEnemyLogicComponent* Logic = NewObject<UReEchoEnemyLogicComponent>();
	Logic->Initialize(
	    ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Bomber, 260.0f, 180.0f, 1.2f, 22.0f), 2);
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
	TestTrue(TEXT("Self destruct is recorded as a one-shot action"), Logic->GetSnapshot().bSelfDestructCommitted);
	TestFalse(TEXT("Expired fuse cannot publish a second explosion"), Logic->Advance(Sense, 0.1f).bAttackCommitted);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBossInjectedDefinitionTest,
                                 "ReEcho.Enemies.Boss.InjectedDefinitionRequired",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBossInjectedDefinitionTest::RunTest(const FString& Parameters)
{
	UReEchoEnemyLogicComponent* Logic = NewObject<UReEchoEnemyLogicComponent>();
	const FReEchoEnemyDefinition LegacyBoss = ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Boss);
	TestFalse(TEXT("Boss cannot silently run without injected abilities and phase"), Logic->Initialize(LegacyBoss, 1));
	TestTrue(TEXT("Complete injected Boss definition initializes"), Logic->Initialize(MakeBossTestDefinition(), 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBossRotationAndSkipTest,
                                 "ReEcho.Enemies.Boss.RotationAndConditionalSkip",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBossRotationAndSkipTest::RunTest(const FString& Parameters)
{
	UReEchoEnemyLogicComponent* Logic = NewObject<UReEchoEnemyLogicComponent>();
	TestTrue(TEXT("Boss initializes"), Logic->Initialize(MakeBossTestDefinition(), 4));

	FReEchoEnemySenseSnapshot Sense;
	Sense.bTargetExists = true;
	Sense.bTargetAlive = true;
	Sense.TargetLocation = FVector(100.0f, 0.0f, 0.0f);
	const FReEchoEnemyActionIntent First = Logic->Advance(Sense, 1.0f / 60.0f);
	const FReEchoBossIntent* FirstTelegraph = FindBossIntent(First, EReEchoBossIntentType::TelegraphStarted);
	TestNotNull(TEXT("First legal ability starts a telegraph"), FirstTelegraph);
	if (FirstTelegraph)
	{
		TestEqual(TEXT("Fixed rotation begins with melee"), FirstTelegraph->AbilityId, FName(TEXT("A_Melee")));
	}

	const FReEchoEnemyActionIntent AfterMelee = Logic->Advance(Sense, 0.2f);
	const FReEchoBossIntent* SkippedTelegraph =
	    FindBossIntent(AfterMelee, EReEchoBossIntentType::TelegraphStarted, FName(TEXT("A_Beam")));
	TestNotNull(TEXT("Out-of-range projectile and unsafe blink are skipped to beam"), SkippedTelegraph);
	TestEqual(TEXT("Skipped abilities do not consume identities; second telegraph gets sequence two"),
	          Logic->GetSnapshot().AttackSequence,
	          int64(2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBossLockAndIdentityTest,
                                 "ReEcho.Enemies.Boss.LockPointAndAttackIdentity",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBossLockAndIdentityTest::RunTest(const FString& Parameters)
{
	UReEchoEnemyLogicComponent* Logic = NewObject<UReEchoEnemyLogicComponent>();
	Logic->Initialize(MakeBossTestDefinition(), 5);
	FReEchoEnemySenseSnapshot Sense;
	Sense.bTargetExists = true;
	Sense.bTargetAlive = true;
	Sense.TargetLocation = FVector(300.0f, 0.0f, 0.0f);

	const FReEchoEnemyActionIntent Telegraph = Logic->Advance(Sense, 1.0f / 60.0f);
	const FReEchoBossIntent* ProjectileTelegraph = FindBossIntent(Telegraph, EReEchoBossIntentType::TelegraphStarted);
	TestNotNull(TEXT("Projectile telegraph starts after melee range skip"), ProjectileTelegraph);
	if (ProjectileTelegraph)
	{
		TestEqual(TEXT("Projectile is selected"), ProjectileTelegraph->AbilityId, FName(TEXT("A_Projectile")));
		TestEqual(TEXT("Action identity allocated at windup start"), ProjectileTelegraph->Attack.Sequence, int64(1));
	}

	Sense.TargetLocation = FVector(800.0f, 0.0f, 0.0f);
	const FReEchoEnemyActionIntent Committed = Logic->Advance(Sense, 0.05f);
	const FReEchoBossIntent* Attack =
	    FindBossIntent(Committed, EReEchoBossIntentType::AttackWindowStarted, FName(TEXT("A_Projectile")));
	TestNotNull(TEXT("Projectile commits after windup"), Attack);
	if (Attack)
	{
		TestEqual(TEXT("Windup-end lock captures the latest target point"),
		          Attack->LockedTargetLocation,
		          FVector(800.0f, 0.0f, 0.0f));
		TestEqual(TEXT("Commit keeps the telegraph identity"), Attack->Attack.Sequence, int64(1));
	}

	Sense.TargetLocation = FVector(1100.0f, 0.0f, 0.0f);
	Logic->Advance(Sense, 1.0f / 60.0f);
	TestEqual(TEXT("Post-commit target motion cannot change the locked point"),
	          Logic->GetSnapshot().BossLockedTargetLocation,
	          FVector(800.0f, 0.0f, 0.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBossAmbientIntentTest,
                                 "ReEcho.Enemies.Boss.CleansePhaseAndPause",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBossAmbientIntentTest::RunTest(const FString& Parameters)
{
	UReEchoEnemyLogicComponent* Logic = NewObject<UReEchoEnemyLogicComponent>();
	Logic->Initialize(MakeBossTestDefinition(), 6);
	FReEchoEnemySenseSnapshot NoTarget;
	Logic->Advance(NoTarget, 0.1f);
	const FReEchoEnemyLogicSnapshot BeforePause = Logic->GetSnapshot();
	const FReEchoEnemyActionIntent Paused = Logic->Advance(NoTarget, 0.0f);
	TestEqual(TEXT("Pause emits no Boss command"), Paused.BossIntents.Num(), 0);
	TestEqual(TEXT("Pause does not advance encounter time"),
	          Logic->GetSnapshot().BossEncounterElapsedSeconds,
	          BeforePause.BossEncounterElapsedSeconds);

	const FReEchoEnemyActionIntent Cleanse = Logic->Advance(NoTarget, 0.05f);
	const FReEchoBossIntent* CleanseIntent = FindBossIntent(Cleanse, EReEchoBossIntentType::ElementCleanse);
	TestNotNull(TEXT("Injected cleanse interval emits a command"), CleanseIntent);
	if (CleanseIntent)
	{
		TestEqual(TEXT("Cleanse carries injected immunity"), CleanseIntent->ElementImmunitySeconds, 0.1f);
	}

	const FReEchoEnemyActionIntent Phase = Logic->Advance(NoTarget, 0.05f);
	const FReEchoBossIntent* PhaseIntent = FindBossIntent(Phase, EReEchoBossIntentType::EncounterPhase);
	TestNotNull(TEXT("Injected encounter threshold emits one phase command"), PhaseIntent);
	if (PhaseIntent)
	{
		TestEqual(TEXT("Phase intent carries its stable ID"), PhaseIntent->PhaseDefinition.Id, FName(TEXT("P_Enrage")));
	}
	const FReEchoEnemyActionIntent AfterPhase = Logic->Advance(NoTarget, 0.05f);
	TestEqual(TEXT("Phase is one-shot"), CountBossIntents(AfterPhase, EReEchoBossIntentType::EncounterPhase), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBossSnapshotDeterminismTest,
                                 "ReEcho.Enemies.Boss.SnapshotAndFixedStepDeterminism",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBossSnapshotDeterminismTest::RunTest(const FString& Parameters)
{
	const FReEchoEnemyDefinition Definition = MakeBossTestDefinition();
	UReEchoEnemyLogicComponent* Original = NewObject<UReEchoEnemyLogicComponent>();
	UReEchoEnemyLogicComponent* Restored = NewObject<UReEchoEnemyLogicComponent>();
	Original->Initialize(Definition, 7);
	Restored->Initialize(Definition, 7);

	FReEchoEnemySenseSnapshot Sense;
	Sense.bTargetExists = true;
	Sense.bTargetAlive = true;
	Sense.TargetLocation = FVector(300.0f, 0.0f, 0.0f);
	Original->Advance(Sense, 0.025f);
	const FReEchoEnemyLogicSnapshot Saved = Original->GetSnapshot();
	Restored->RestoreSnapshot(Saved);

	Sense.TargetLocation = FVector(650.0f, 0.0f, 0.0f);
	const FReEchoEnemyActionIntent OriginalNext = Original->Advance(Sense, 0.1f);
	const FReEchoEnemyActionIntent RestoredNext = Restored->Advance(Sense, 0.1f);
	const FReEchoBossIntent* OriginalAttack = FindBossIntent(OriginalNext, EReEchoBossIntentType::AttackWindowStarted);
	const FReEchoBossIntent* RestoredAttack = FindBossIntent(RestoredNext, EReEchoBossIntentType::AttackWindowStarted);
	TestNotNull(TEXT("Original commits after saved windup"), OriginalAttack);
	TestNotNull(TEXT("Restored commits after saved windup"), RestoredAttack);
	if (OriginalAttack && RestoredAttack)
	{
		TestEqual(TEXT("Restored attack identity sequence matches"),
		          RestoredAttack->Attack.Sequence,
		          OriginalAttack->Attack.Sequence);
		TestEqual(TEXT("Restored locked point matches"),
		          RestoredAttack->LockedTargetLocation,
		          OriginalAttack->LockedTargetLocation);
	}

	const FReEchoEnemyLogicSnapshot OriginalState = Original->GetSnapshot();
	const FReEchoEnemyLogicSnapshot RestoredState = Restored->GetSnapshot();
	TestEqual(TEXT("Restored action phase matches"), RestoredState.BossActionPhase, OriginalState.BossActionPhase);
	TestEqual(TEXT("Restored current ability matches"),
	          RestoredState.BossCurrentAbilityId,
	          OriginalState.BossCurrentAbilityId);
	TestTrue(TEXT("Restored phase timer matches"),
	         FMath::IsNearlyEqual(RestoredState.BossActionPhaseRemainingSeconds,
	                              OriginalState.BossActionPhaseRemainingSeconds));

	UReEchoEnemyLogicComponent* OneChunk = NewObject<UReEchoEnemyLogicComponent>();
	UReEchoEnemyLogicComponent* Partitioned = NewObject<UReEchoEnemyLogicComponent>();
	OneChunk->Initialize(Definition, 8);
	Partitioned->Initialize(Definition, 8);
	OneChunk->Advance(Sense, 0.5f);
	for (int32 Step = 0; Step < 30; ++Step)
	{
		Partitioned->Advance(Sense, 1.0f / 60.0f);
	}
	const FReEchoEnemyLogicSnapshot ChunkState = OneChunk->GetSnapshot();
	const FReEchoEnemyLogicSnapshot PartitionState = Partitioned->GetSnapshot();
	TestEqual(TEXT("Frame partition keeps attack sequence"), PartitionState.AttackSequence, ChunkState.AttackSequence);
	TestEqual(TEXT("Frame partition keeps current ability"),
	          PartitionState.BossCurrentAbilityId,
	          ChunkState.BossCurrentAbilityId);
	TestTrue(TEXT("Frame partition keeps encounter clock"),
	         FMath::IsNearlyEqual(PartitionState.BossEncounterElapsedSeconds, ChunkState.BossEncounterElapsedSeconds));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemySpecialBehaviorsTest,
                                 "ReEcho.Enemies.Logic.RangedAndEliteBehaviors",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemySpecialBehaviorsTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyDefinition Ranged;
	Ranged.Archetype = EReEchoEnemyArchetype::Ranged;
	Ranged.MaxHealth = 20.0f;
	Ranged.MoveSpeedCmPerSecond = 165.0f;
	Ranged.MovementStopDistanceCm = 650.0f;
	FReEchoEnemyAbilityDefinition Burst;
	Burst.Id = TEXT("RabbitBurst");
	Burst.BehaviorId = TEXT("Enemy.RangedBurst");
	Burst.bEnabled = true;
	Burst.Damage = 10.0f;
	Burst.WindupSeconds = 0.55f;
	Burst.CooldownSeconds = 2.0f;
	Burst.MaxRangeCm = 1000.0f;
	Burst.RadiusCm = 150.0f;
	Ranged.Abilities.Add(Burst);

	UReEchoEnemyLogicComponent* RangedLogic = NewObject<UReEchoEnemyLogicComponent>();
	TestTrue(TEXT("Ranged definition initializes"), RangedLogic->Initialize(Ranged, 1));
	FReEchoEnemySenseSnapshot Sense;
	Sense.bTargetExists = true;
	Sense.bTargetAlive = true;
	Sense.TargetLocation = FVector(500.0f, 0.0f, 0.0f);
	Sense.bSpecialActionPermitted = false;
	RangedLogic->Advance(Sense, 0.01f);
	TestEqual(TEXT("Encounter gate prevents a ranged windup"),
	          RangedLogic->GetSnapshot().SpecialActionPhase,
	          EReEchoEnemySpecialActionPhase::None);
	Sense.bSpecialActionPermitted = true;
	RangedLogic->Advance(Sense, 0.01f);
	TestEqual(TEXT("Ranged attack enters windup"),
	          RangedLogic->GetSnapshot().SpecialActionPhase,
	          EReEchoEnemySpecialActionPhase::Windup);
	Sense.TargetLocation = FVector(800.0f, 0.0f, 0.0f);
	const FReEchoEnemyActionIntent MissedBurst = RangedLogic->Advance(Sense, 0.56f);
	TestTrue(TEXT("Ranged burst commits after table windup"), MissedBurst.bAttackCommitted);
	TestFalse(TEXT("Moving outside the locked radius avoids the burst"), MissedBurst.bCanDamageTarget);
	TestEqual(TEXT("Ranged burst uses table damage"), MissedBurst.RawDamage, 10.0f);

	FReEchoEnemyDefinition Elite;
	Elite.Archetype = EReEchoEnemyArchetype::Elite;
	Elite.MaxHealth = 55.0f;
	Elite.MoveSpeedCmPerSecond = 120.0f;
	Elite.bUsesDirectionalShield = true;
	FReEchoEnemyAbilityDefinition Dash;
	Dash.Id = TEXT("FoxDash");
	Dash.BehaviorId = TEXT("Enemy.EliteDash");
	Dash.bEnabled = true;
	Dash.Damage = 18.0f;
	Dash.WindupSeconds = 0.8f;
	Dash.RecoverySeconds = 0.9f;
	Dash.CooldownSeconds = 4.0f;
	Dash.MaxRangeCm = 450.0f;
	Dash.WidthCm = 140.0f;
	Dash.LengthCm = 450.0f;
	Elite.Abilities.Add(Dash);
	UReEchoEnemyLogicComponent* EliteLogic = NewObject<UReEchoEnemyLogicComponent>();
	TestTrue(TEXT("Elite definition initializes"), EliteLogic->Initialize(Elite, 2));
	Sense.TargetLocation = FVector(300.0f, 0.0f, 0.0f);
	Sense.bSpecialActionPermitted = false;
	EliteLogic->Advance(Sense, 0.01f);
	TestEqual(TEXT("Encounter gate prevents an elite windup"),
	          EliteLogic->GetSnapshot().SpecialActionPhase,
	          EReEchoEnemySpecialActionPhase::None);
	Sense.bSpecialActionPermitted = true;
	EliteLogic->Advance(Sense, 0.01f);
	const FReEchoEnemyActionIntent DashCommit = EliteLogic->Advance(Sense, 0.81f);
	TestTrue(TEXT("Elite dash commits after table warning"), DashCommit.bAttackCommitted);
	TestTrue(TEXT("Elite dash moves along the locked line"), DashCommit.bHasMovement);
	TestEqual(TEXT("Elite dash length comes from table ability"), DashCommit.MovementDelta.Size2D(), 450.0);
	TestEqual(TEXT("Elite dash uses table damage"), DashCommit.RawDamage, 18.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyPhaseRangeAggroPolicyTest,
                                 "ReEcho.Enemies.Logic.Phase2.RangeAggroPolicy",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyPhaseRangeAggroPolicyTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyDefinition Definition = ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Grunt);
	Definition.Phase2.Id = TEXT("Phase2");
	Definition.Phase2.TriggerRangeCm = 200.0f;
	Definition.Phase2.TransformSeconds = 0.5f;
	Definition.Phase2.bEnabled = true;
	UReEchoEnemyLogicComponent* Logic = NewObject<UReEchoEnemyLogicComponent>();
	TestTrue(TEXT("Phase definition initializes"), Logic->Initialize(Definition, 1));
	FReEchoEnemySenseSnapshot Sense;
	Sense.bTargetExists = true;
	Sense.bTargetAlive = true;
	Sense.TargetLocation = FVector(100.0f, 0.0f, 0.0f);
	TestFalse(TEXT("A non-aggro Echo cannot range-trigger"), Logic->Advance(Sense, 0.1f).bPhaseTransitionStarted);
	Sense.bTargetCanAttractAggro = true;
	const FReEchoEnemyActionIntent Started = Logic->Advance(Sense, 0.1f);
	TestTrue(TEXT("Current valid aggro target can range-trigger"), Started.bPhaseTransitionStarted);
	TestEqual(TEXT("Range trigger reason"), Started.PhaseTriggerReason, EReEchoEnemyPhaseTriggerReason::RangeEntered);
	TestFalse(TEXT("Transformation emits no movement"), Started.bHasMovement);
	TestTrue(TEXT("Transformation remains active"),
	         Logic->GetSnapshot().Phase == EReEchoEnemyBehaviorPhase::Transforming);
	TestTrue(TEXT("Logic time completes transformation"), Logic->Advance(Sense, 0.5f).bPhaseTransitionCompleted);
	TestEqual(TEXT("Transformation enters phase two"), Logic->GetSnapshot().CurrentPhaseIndex, 2);
	TestFalse(TEXT("Transition never starts twice"), Logic->Advance(Sense, 1.0f).bPhaseTransitionStarted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyPhaseAttackCountTest,
                                 "ReEcho.Enemies.Logic.Phase2.AttackCountAndSnapshot",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyPhaseAttackCountTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyDefinition Definition = ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Grunt);
	Definition.Phase2.Id = TEXT("Phase2");
	Definition.Phase2.RequiredAttackCount = 3;
	Definition.Phase2.TransformSeconds = 1.0f;
	Definition.Phase2.bEnabled = true;
	UReEchoEnemyLogicComponent* Logic = NewObject<UReEchoEnemyLogicComponent>();
	TestTrue(TEXT("Attack-count definition initializes"), Logic->Initialize(Definition, 2));
	AActor* PlayerSource = NewObject<AActor>();
	AActor* EchoSource = NewObject<AActor>();
	FReEchoDamageEvent Event;
	Event.DamageSource = EReEchoDamageSource::Player;
	Event.Attack.Source = PlayerSource;
	Event.Attack.Sequence = 10;
	Event.RawDamage = 20.0f;
	Event.AppliedDamage = 0.0f;
	Event.bBlocked = true;
	Logic->NotifyReceivedAttack(Event);
	TestEqual(
	    TEXT("Raw damage with zero final applied damage does not count"), Logic->GetSnapshot().ReceivedDamageCount, 0);
	Event.AppliedDamage = 1.0f;
	Event.bBlocked = false;
	Logic->NotifyReceivedAttack(Event);
	Logic->NotifyReceivedAttack(Event);
	TestEqual(TEXT("Each actual health reduction counts even for the same attack identity"),
	          Logic->GetSnapshot().ReceivedDamageCount,
	          2);
	Event.DamageSource = EReEchoDamageSource::Echo;
	Event.Attack.Source = EchoSource;
	Event.Attack.Sequence = 3;
	Logic->NotifyReceivedAttack(Event);
	TestEqual(TEXT("Echo health reduction counts"), Logic->GetSnapshot().ReceivedDamageCount, 3);
	FReEchoEnemySenseSnapshot NoTarget;
	const FReEchoEnemyActionIntent Started = Logic->Advance(NoTarget, 0.0f);
	TestTrue(TEXT("Attack threshold starts transformation"), Started.bPhaseTransitionStarted);
	TestEqual(TEXT("Attack count has deterministic OR precedence"),
	          Started.PhaseTriggerReason,
	          EReEchoEnemyPhaseTriggerReason::AttackCountReached);
	const FReEchoEnemyLogicSnapshot Saved = Logic->GetSnapshot();
	UReEchoEnemyLogicComponent* Restored = NewObject<UReEchoEnemyLogicComponent>();
	Restored->Initialize(Definition, 2);
	Restored->RestoreSnapshot(Saved);
	TestEqual(TEXT("Snapshot restores health-reduction count"), Restored->GetSnapshot().ReceivedDamageCount, 3);
	TestEqual(TEXT("Snapshot restores transition time"), Restored->GetSnapshot().PhaseTransitionRemainingSeconds, 1.0f);
	TestTrue(TEXT("Restored transition completes without retrigger"),
	         Restored->Advance(NoTarget, 1.0f).bPhaseTransitionCompleted);
	return true;
}

#endif
