#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Core/ReEchoRabbitProjectilePattern.h"
#include "Data/ReEchoEnemyDefinitionCompiler.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "Enemies/ReEchoEnemyLogicComponent.h"
#include "Enemies/ReEchoEnemyRosterComponent.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Misc/AutomationTest.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Presentation/VFX/ReEchoCombatVfxComponent.h"

namespace
{
struct FReEchoEnemyHostWorldFixture
{
	UWorld* World = nullptr;

	FReEchoEnemyHostWorldFixture()
	{
		const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("ReEchoEnemyHostTestWorld"));
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		World->AddToRoot();
		Context.SetCurrentWorld(World);
		World->SetShouldTick(true);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
	}

	~FReEchoEnemyHostWorldFixture()
	{
		if (World)
		{
			World->DestroyWorld(true);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}
	}

	AReEchoEnemyActor* Spawn(const EReEchoEnemyKind Kind, const int32 SpawnIndex)
	{
		AReEchoEnemyActor* Enemy = World->SpawnActor<AReEchoEnemyActor>();
		if (Enemy && !Enemy->HasActorBegunPlay())
		{
			Enemy->DispatchBeginPlay();
		}
		if (Enemy)
		{
			Enemy->Configure(Kind, SpawnIndex);
		}
		return Enemy;
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHostCompositionTest,
                                 "ReEcho.Enemies.Host.CompositionAndSave",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHostCompositionTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyHostWorldFixture Fixture;
	UReEchoEnemyRosterComponent* Roster = NewObject<UReEchoEnemyRosterComponent>();
	AReEchoEnemyActor* Source = Fixture.Spawn(EReEchoEnemyKind::Bomber, 12);
	TestNotNull(TEXT("Enemy host spawns"), Source);
	if (!Source)
	{
		return false;
	}
	Source->SetEnemyRoster(Roster);
	TestEqual(TEXT("Host registers into the single roster"), Roster->GetLivingEnemyCount(), 1);
	TestEqual(TEXT("Legacy host kind is projected from EnemyLogic"), Source->GetKind(), EReEchoEnemyKind::Bomber);
	TestEqual(TEXT("Stable spawn index is projected from EnemyLogic"), Source->GetSpawnIndex(), 12);

	FReEchoEnemyLogicSnapshot LogicState = Source->GetEnemyLogicComponent()->GetSnapshot();
	LogicState.Phase = EReEchoEnemyBehaviorPhase::HitReaction;
	LogicState.AttackCooldownRemainingSeconds = 0.75f;
	LogicState.FuseRemainingSeconds = 0.5f;
	LogicState.HitReactionRemainingSeconds = 0.1f;
	LogicState.KnockbackVelocity = FVector(30.0f, -20.0f, 0.0f);
	LogicState.AttackSequence = 8;
	LogicState.FacingDirection = FVector(0.0f, -1.0f, 0.0f);
	LogicState.bFuseActive = true;
	Source->GetEnemyLogicComponent()->RestoreSnapshot(LogicState);
	Source->SetActorLocation(FVector(10.0f, 20.0f, 30.0f));
	Source->SetActorRotation(FRotator(0.0f, 73.0f, 0.0f));
	const FReEchoEnemyRuntimeState Saved = Source->CaptureRuntimeState();
	TestEqual(TEXT("Host save composes logic cooldown"), Saved.AttackCooldown, 0.75f);
	TestEqual(TEXT("Host save composes fuse"), Saved.FuseRemaining, 0.5f);
	TestEqual(TEXT("Host save composes hit reaction"), Saved.HitReactionRemaining, 0.1f);
	TestEqual(TEXT("Host save composes attack identity"), Saved.AttackSequence, int64(8));
	TestTrue(TEXT("Enemy save transform no longer persists visual facing rotation"),
	         Saved.Transform.GetRotation().Equals(FQuat::Identity));

	AReEchoEnemyActor* Restored = Fixture.Spawn(EReEchoEnemyKind::Grunt, 99);
	TestNotNull(TEXT("Restore host spawns"), Restored);
	if (!Restored)
	{
		return false;
	}
	Restored->RestoreRuntimeState(Saved);
	const FReEchoEnemyLogicSnapshot RestoredLogic = Restored->GetEnemyLogicComponent()->GetSnapshot();
	TestEqual(TEXT("Restore returns archetype authority to EnemyLogic"),
	          RestoredLogic.Archetype,
	          EReEchoEnemyArchetype::Bomber);
	TestEqual(
	    TEXT("Restore returns cooldown authority to EnemyLogic"), RestoredLogic.AttackCooldownRemainingSeconds, 0.75f);
	TestEqual(TEXT("Restore returns fuse authority to EnemyLogic"), RestoredLogic.FuseRemainingSeconds, 0.5f);
	TestEqual(TEXT("Restore returns attack identity authority to EnemyLogic"), RestoredLogic.AttackSequence, int64(8));
	TestEqual(TEXT("Restore returns world transform authority to Host"),
	          Restored->GetActorLocation(),
	          FVector(10.0f, 20.0f, 30.0f));
	TestTrue(TEXT("Restore keeps Enemy Actor rotation identity"), Restored->GetActorQuat().Equals(FQuat::Identity));
	TestTrue(TEXT("Restore preserves explicit logical facing"),
	         RestoredLogic.FacingDirection.Equals(FVector(0.0f, -1.0f, 0.0f)));

	UReEchoCombatEventsComponent* SourceCombatEvents = Source->FindComponentByClass<UReEchoCombatEventsComponent>();
	Source->ReceiveGrayboxDamage(TNumericLimits<float>::Max(), Source->GetActorLocation());
	TestFalse(TEXT("Combat death immediately closes EnemyLogic"),
	          Source->GetEnemyLogicComponent()->GetSnapshot().bAlive);
	TestFalse(TEXT("Combat death immediately disables host collision"), Source->GetActorEnableCollision());
	TestEqual(TEXT("Roster reads death from EnemyLogic without a copied life flag"), Roster->GetLivingEnemyCount(), 0);
	TestTrue(TEXT("Enemy without a Death clip schedules immediate safe destruction"), Source->GetLifeSpan() > 0.0f);
	TestTrue(TEXT("Lethal Hurt is explicitly marked fatal for presentation suppression"),
	         SourceCombatEvents && SourceCombatEvents->GetHurtPublishCountForTests() == 1 &&
	             SourceCombatEvents->GetLastHurtEventForTests().bFatal);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHostCrowdCollisionTest,
                                 "ReEcho.Enemies.Host.CrowdCollision",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHostCrowdCollisionTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyHostWorldFixture Fixture;
	UReEchoEnemyRosterComponent* Roster = NewObject<UReEchoEnemyRosterComponent>();
	AReEchoEnemyActor* First = Fixture.Spawn(EReEchoEnemyKind::Grunt, 1);
	AReEchoEnemyActor* Second = Fixture.Spawn(EReEchoEnemyKind::Shield, 2);
	if (!TestNotNull(TEXT("First crowd enemy spawns"), First) ||
	    !TestNotNull(TEXT("Second crowd enemy spawns"), Second))
	{
		return false;
	}
	First->SetEnemyRoster(Roster);
	Second->SetEnemyRoster(Roster);
	TestTrue(TEXT("Ordinary enemies mutually ignore swept movement"),
	         First->IsIgnoringEnemyMovementForTests(Second) && Second->IsIgnoringEnemyMovementForTests(First));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHostAttackPipelineTest,
                                 "ReEcho.Enemies.Host.AttackPipeline",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHostAttackPipelineTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyHostWorldFixture Fixture;
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	AReEchoPlayerPawn* Player =
	    Fixture.World->SpawnActor<AReEchoPlayerPawn>(FVector::ZeroVector, FRotator::ZeroRotator);
	TestNotNull(TEXT("Player host spawns"), Player);
	if (!Player)
	{
		return false;
	}
	if (!Player->HasActorBegunPlay())
	{
		Player->DispatchBeginPlay();
	}
	Player->SetAutoAttackMode(false);
	FReEchoStatBlock PlayerStats;
	PlayerStats.HpMax = 100.0f;
	PlayerStats.HpPoint = 100.0f;
	Player->Combatant->InitializeFromStats(PlayerStats, true);

	AReEchoEnemyActor* Enemy =
	    Fixture.World->SpawnActor<AReEchoEnemyActor>(FVector(50.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	TestNotNull(TEXT("Enemy host spawns for attack pipeline"), Enemy);
	if (!Enemy)
	{
		return false;
	}
	if (!Enemy->HasActorBegunPlay())
	{
		Enemy->DispatchBeginPlay();
	}
	Enemy->Configure(EReEchoEnemyKind::Grunt, 1);
	FReEchoEnemySenseSnapshot Sense;
	Sense.Target = Player;
	Sense.SelfLocation = Enemy->GetActorLocation();
	Sense.TargetLocation = Player->GetActorLocation();
	Sense.bTargetExists = true;
	Sense.bTargetAlive = true;

	Enemy->AdvanceBehaviorForTests(Sense, 0.01f);
	TestTrue(TEXT("Enemy logic facing does not rotate the Actor"), Enemy->GetActorQuat().Equals(FQuat::Identity));
	TestEqual(TEXT("Enemy ActionIntent reaches Combat exactly once"), Player->Combatant->CurrentHealth, 91.0f);
	TestEqual(TEXT("Committed host attack uses EnemyLogic identity"),
	          Enemy->GetEnemyLogicComponent()->GetSnapshot().AttackSequence,
	          int64(1));
	Enemy->AdvanceBehaviorForTests(Sense, 0.5f);
	TestEqual(TEXT("Enemy cooldown prevents an early second hit"), Player->Combatant->CurrentHealth, 91.0f);
	TestEqual(TEXT("Enemy remains in normal contact behavior"),
	          Enemy->GetEnemyLogicComponent()->GetSnapshot().Phase,
	          EReEchoEnemyBehaviorPhase::Idle);
	TestEqual(TEXT("Enemy cooldown advances while the target remains valid"),
	          Enemy->GetEnemyLogicComponent()->GetSnapshot().AttackCooldownRemainingSeconds,
	          0.8f);
	Enemy->AdvanceBehaviorForTests(Sense, 0.81f);
	TestTrue(TEXT("Target remains inside contact range"),
	         FVector::Dist2D(Enemy->GetActorLocation(), Player->GetActorLocation()) <= 85.0f);
	TestEqual(
	    TEXT("Enemy cooldown permits the second hit at the legacy cadence"), Player->Combatant->CurrentHealth, 82.0f);
	TestEqual(TEXT("Second attack restarts the legacy cooldown"),
	          Enemy->GetEnemyLogicComponent()->GetSnapshot().AttackCooldownRemainingSeconds,
	          1.3f);
	TestEqual(TEXT("Second host attack increments the same identity sequence"),
	          Enemy->GetEnemyLogicComponent()->GetSnapshot().AttackSequence,
	          int64(2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHostRabbitProjectileTest,
                                 "ReEcho.Enemies.Host.RabbitProjectilePipeline",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHostRabbitProjectileTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyHostWorldFixture Fixture;
	const FReEchoCsvLoadResult LoadResult =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	if (!TestTrue(TEXT("Production enemy CSV loads"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}
	FReEchoEnemyDefinition RabbitDefinition;
	FString CompileError;
	if (!TestTrue(TEXT("Rabbit definition compiles"),
	              ReEchoEnemyDefinitionCompiler::Compile(
	                  *LoadResult.Snapshot, TEXT("M_RABBIT"), RabbitDefinition, CompileError)))
	{
		AddError(CompileError);
		return false;
	}

	AReEchoPlayerPawn* Player =
	    Fixture.World->SpawnActor<AReEchoPlayerPawn>(FVector(600.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	APlayerController* PlayerController = Fixture.World->SpawnActor<APlayerController>();
	AReEchoEnemyActor* Rabbit =
	    Fixture.World->SpawnActor<AReEchoEnemyActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Rabbit host spawns"), Rabbit) || !TestNotNull(TEXT("Target player spawns"), Player) ||
	    !TestNotNull(TEXT("Player controller spawns"), PlayerController))
	{
		return false;
	}
	if (!Player->HasActorBegunPlay())
	{
		Player->DispatchBeginPlay();
	}
	FReEchoStatBlock PlayerStats;
	PlayerStats.HpMax = 100.0f;
	PlayerStats.HpPoint = 100.0f;
	Player->Combatant->InitializeFromStats(PlayerStats, true);
	PlayerController->Possess(Player);
	Rabbit->SetEnemyId(TEXT("M_RABBIT"));
	if (!TestTrue(TEXT("Rabbit accepts production definition"), Rabbit->ConfigureFromDefinition(RabbitDefinition, 7)))
	{
		return false;
	}
	if (!Rabbit->HasActorBegunPlay())
	{
		Rabbit->DispatchBeginPlay();
	}

	FReEchoEnemySenseSnapshot Sense;
	Sense.Target = Player;
	Sense.SelfLocation = Rabbit->GetActorLocation();
	Sense.TargetLocation = Player->GetActorLocation();
	Sense.bTargetExists = true;
	Sense.bTargetAlive = true;
	Sense.bSpecialActionPermitted = true;
	Rabbit->AdvanceBehaviorForTests(Sense, 0.01f);
	const FReEchoEnemyActionIntent Commit = Rabbit->AdvanceBehaviorForTests(Sense, 0.55f);
	TestTrue(TEXT("Rabbit commits after configured windup"), Commit.bAttackCommitted);

	const FReEchoEnemyRuntimeState SpawnedState = Rabbit->CaptureRuntimeState();
	if (!TestEqual(TEXT("Commit creates three authoritative rabbit balls"),
	               SpawnedState.BossProjectiles.Num(),
	               ReEchoRabbitProjectilePattern::BallCount))
	{
		return false;
	}
	const float AuthoritativeBallRadius = SpawnedState.BossProjectiles[0].CollisionRadiusCm;
	const float AuthoritativeBallSpeed = SpawnedState.BossProjectiles[0].Definition.SpeedCmPerSecond;
	for (int32 BallIndex = 0; BallIndex < SpawnedState.BossProjectiles.Num(); ++BallIndex)
	{
		const FReEchoEnemyProjectileRuntimeState& Ball = SpawnedState.BossProjectiles[BallIndex];
		TestEqual(TEXT("Rabbit ball keeps the authored damage"), Ball.Damage, 1.0f);
		TestEqual(TEXT("Every rabbit ball uses the same data-driven collider radius"),
		          Ball.CollisionRadiusCm,
		          AuthoritativeBallRadius);
		TestEqual(TEXT("Rabbit ball stores its stable volley index"), Ball.VolleyBallIndex, BallIndex);
		TestEqual(TEXT("Every rabbit ball uses the same resolved data-driven speed"),
		          Ball.Definition.SpeedCmPerSecond,
		          AuthoritativeBallSpeed);
		TestTrue(TEXT("Resolved rabbit ball speed is positive"), Ball.Definition.SpeedCmPerSecond > 0.0f);
		TestTrue(TEXT("Rabbit ball starts active"), Ball.Snapshot.bActive);
		TestFalse(TEXT("Rabbit ball starts without a consumed collision"), Ball.bCollisionConsumed);
		TestTrue(TEXT("Rabbit ball uses the authoritative fan direction"),
		         Ball.Definition.Direction.Equals(
		             ReEchoRabbitProjectilePattern::ResolveVolleyDirection(FVector::ForwardVector, 40.0f, BallIndex, 3),
		             0.001f));
	}
	const TArray<FReEchoEnemyProjectileEvent>& SpawnEvents =
	    Rabbit->GetEnemyEventsComponent()->GetPublishedProjectileEventsForTests();
	bool bSawSpawnForBall[ReEchoRabbitProjectilePattern::BallCount] = {false, false, false};
	int32 SpawnEventCount = 0;
	for (const FReEchoEnemyProjectileEvent& Event : SpawnEvents)
	{
		if (Event.Type != EReEchoEnemyProjectileEventType::Spawned)
		{
			continue;
		}
		++SpawnEventCount;
		if (Event.VolleyBallIndex >= 0 && Event.VolleyBallIndex < ReEchoRabbitProjectilePattern::BallCount)
		{
			bSawSpawnForBall[Event.VolleyBallIndex] = true;
		}
		TestEqual(TEXT("Each presentation event carries the authoritative collider radius"),
		          Event.CollisionRadiusCm,
		          AuthoritativeBallRadius);
	}
	TestEqual(TEXT("Host publishes one presentation spawn per authoritative ball"),
	          SpawnEventCount,
	          ReEchoRabbitProjectilePattern::BallCount);
	for (int32 BallIndex = 0; BallIndex < ReEchoRabbitProjectilePattern::BallCount; ++BallIndex)
	{
		TestTrue(FString::Printf(TEXT("Presentation spawn includes ball %d"), BallIndex), bSawSpawnForBall[BallIndex]);
	}
	UReEchoCombatVfxComponent* RabbitVfx = Rabbit->FindComponentByClass<UReEchoCombatVfxComponent>();
	if (TestNotNull(TEXT("Rabbit host composes the combat VFX adapter"), RabbitVfx))
	{
		TestEqual(TEXT("One visual proxy exists for every authoritative ball"),
		          RabbitVfx->GetProjectileVisualCountForTests(),
		          ReEchoRabbitProjectilePattern::BallCount);
		for (const FReEchoEnemyProjectileRuntimeState& Ball : SpawnedState.BossProjectiles)
		{
			FVector VisualLocation = FVector::ZeroVector;
			TestTrue(TEXT("Spawned ball has a keyed visual proxy"),
			         RabbitVfx->TryGetProjectileVisualLocationForTests(
			             Ball.Attack.Sequence, Ball.VolleyBallIndex, VisualLocation));
			TestTrue(TEXT("Spawned visual proxy is exactly at its logical ball"),
			         VisualLocation.Equals(Ball.Snapshot.Location, KINDA_SMALL_NUMBER));
		}
	}
	const float ProjectileGameplayZ = SpawnedState.BossProjectiles[0].Snapshot.Location.Z;

	Rabbit->GetEnemyEventsComponent()->ClearPublishedProjectileEventsForTests();
	Rabbit->AdvanceEnemyProjectilesForTests(0.1f);
	const FReEchoEnemyRuntimeState AdvancedState = Rabbit->CaptureRuntimeState();
	TestEqual(TEXT("All balls remain in flight before reaching target"),
	          AdvancedState.BossProjectiles.Num(),
	          ReEchoRabbitProjectilePattern::BallCount);
	if (AdvancedState.BossProjectiles.Num() == ReEchoRabbitProjectilePattern::BallCount)
	{
		for (const FReEchoEnemyProjectileRuntimeState& Ball : AdvancedState.BossProjectiles)
		{
			TestTrue(TEXT("Host advances each rabbit ball along its fan direction"),
			         Ball.Snapshot.DistanceTravelledCm > 0.0f);
			if (RabbitVfx)
			{
				FVector VisualLocation = FVector::ZeroVector;
				TestTrue(TEXT("Moved ball keeps its keyed visual proxy"),
				         RabbitVfx->TryGetProjectileVisualLocationForTests(
				             Ball.Attack.Sequence, Ball.VolleyBallIndex, VisualLocation));
				TestTrue(TEXT("Moved visual proxy remains exactly at its logical ball"),
				         VisualLocation.Equals(Ball.Snapshot.Location, KINDA_SMALL_NUMBER));
			}
		}
	}
	int32 MoveEventCount = 0;
	for (const FReEchoEnemyProjectileEvent& Event :
	     Rabbit->GetEnemyEventsComponent()->GetPublishedProjectileEventsForTests())
	{
		MoveEventCount += Event.Type == EReEchoEnemyProjectileEventType::Moved ? 1 : 0;
	}
	TestEqual(TEXT("Every authoritative ball publishes its own moved event"),
	          MoveEventCount,
	          ReEchoRabbitProjectilePattern::BallCount);

	Player->SetActorLocation(FVector(0.0f, 1000.0f, ProjectileGameplayZ));
	Rabbit->AdvanceEnemyProjectilesForTests(0.5f);
	TestEqual(
	    TEXT("Player outside the swept projectile collider takes no damage"), Player->Combatant->CurrentHealth, 100.0f);
	TestEqual(TEXT("A missed volley keeps all balls in flight"),
	          Rabbit->CaptureRuntimeState().BossProjectiles.Num(),
	          ReEchoRabbitProjectilePattern::BallCount);

	const FReEchoEnemyRuntimeState BeforeCenterHit = Rabbit->CaptureRuntimeState();
	const FReEchoEnemyProjectileRuntimeState* CenterBeforeHit = BeforeCenterHit.BossProjectiles.FindByPredicate(
	    [](const FReEchoEnemyProjectileRuntimeState& Ball)
	    {
		    return Ball.VolleyBallIndex == ReEchoRabbitProjectilePattern::CenterBallIndex;
	    });
	if (!TestNotNull(TEXT("Center ball remains available before its collision sample"), CenterBeforeHit))
	{
		return false;
	}
	const FVector CenterNextLocation =
	    CenterBeforeHit->Snapshot.Location +
	    CenterBeforeHit->Snapshot.Direction * CenterBeforeHit->Definition.SpeedCmPerSecond * 0.2f;
	// Place the player at the midpoint of the next authoritative swept segment. This keeps the
	// collision assertion stable when production projectile speed changes.
	Player->SetActorLocation(FMath::Lerp(CenterBeforeHit->Snapshot.Location, CenterNextLocation, 0.5f));
	TestTrue(TEXT("Center ball's next swept segment intersects the player collider"),
	         Player->IntersectsCombatPath(
	             CenterBeforeHit->Snapshot.Location, CenterNextLocation, CenterBeforeHit->CollisionRadiusCm));
	Rabbit->AdvanceEnemyProjectilesForTests(0.2f);
	TestEqual(TEXT("Center ball applies exactly one damage"), Player->Combatant->CurrentHealth, 99.0f);
	TestEqual(TEXT("A rabbit ball is removed immediately after hitting the player"),
	          Rabbit->CaptureRuntimeState().BossProjectiles.Num(),
	          ReEchoRabbitProjectilePattern::BallCount - 1);
	if (RabbitVfx)
	{
		TestEqual(TEXT("The hit rabbit ball's visual proxy ends with its logic"),
		          RabbitVfx->GetProjectileVisualCountForTests(),
		          ReEchoRabbitProjectilePattern::BallCount - 1);
	}

	const int32 SwordCutCount = Rabbit->DestroyRabbitProjectilesInMeleeArc(
	    FVector::ZeroVector, FVector::ForwardVector, 700.0f, 180.0f);
	TestEqual(TEXT("Longsword's forward 180-degree sector removes the two remaining rabbit balls"),
	          SwordCutCount,
	          ReEchoRabbitProjectilePattern::BallCount - 1);
	TestEqual(TEXT("Sword-cut rabbit balls leave no authoritative projectile state"),
	          Rabbit->CaptureRuntimeState().BossProjectiles.Num(),
	          0);
	if (RabbitVfx)
	{
		TestEqual(TEXT("Sword-cut rabbit balls publish Ended and remove their visual proxies"),
		          RabbitVfx->GetProjectileVisualCountForTests(),
		          0);
	}

	Rabbit->AdvanceEnemyProjectilesForTests(0.1f);
	TestEqual(TEXT("Removed balls cannot damage the player twice"), Player->Combatant->CurrentHealth, 99.0f);
	Rabbit->AdvanceEnemyProjectilesForTests(1.0f);
	TestEqual(TEXT("All balls end after their authored maximum range"),
	          Rabbit->CaptureRuntimeState().BossProjectiles.Num(),
	          0);
	TestEqual(TEXT("Ended volley cannot add damage"), Player->Combatant->CurrentHealth, 99.0f);
	int32 EndEventCount = 0;
	for (const FReEchoEnemyProjectileEvent& Event :
	     Rabbit->GetEnemyEventsComponent()->GetPublishedProjectileEventsForTests())
	{
		EndEventCount += Event.Type == EReEchoEnemyProjectileEventType::Ended ? 1 : 0;
	}
	TestEqual(TEXT("Every authoritative ball publishes its own ended event"),
	          EndEventCount,
	          ReEchoRabbitProjectilePattern::BallCount);
	if (RabbitVfx)
	{
		TestEqual(
		    TEXT("Ended volley leaves no projectile visual proxies"), RabbitVfx->GetProjectileVisualCountForTests(), 0);
	}
	return true;
}

#endif
