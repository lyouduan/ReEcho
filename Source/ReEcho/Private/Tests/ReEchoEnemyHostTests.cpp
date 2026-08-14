#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ReEchoCombatantComponent.h"
#include "Enemies/ReEchoEnemyLogicComponent.h"
#include "Enemies/ReEchoEnemyRosterComponent.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Misc/AutomationTest.h"
#include "Player/ReEchoPlayerPawn.h"

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
	LogicState.bFuseActive = true;
	Source->GetEnemyLogicComponent()->RestoreSnapshot(LogicState);
	Source->SetActorLocation(FVector(10.0f, 20.0f, 30.0f));
	const FReEchoEnemyRuntimeState Saved = Source->CaptureRuntimeState();
	TestEqual(TEXT("Host save composes logic cooldown"), Saved.AttackCooldown, 0.75f);
	TestEqual(TEXT("Host save composes fuse"), Saved.FuseRemaining, 0.5f);
	TestEqual(TEXT("Host save composes hit reaction"), Saved.HitReactionRemaining, 0.1f);
	TestEqual(TEXT("Host save composes attack identity"), Saved.AttackSequence, int64(8));

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
	TestEqual(TEXT("Restore returns cooldown authority to EnemyLogic"),
	          RestoredLogic.AttackCooldownRemainingSeconds,
	          0.75f);
	TestEqual(TEXT("Restore returns fuse authority to EnemyLogic"), RestoredLogic.FuseRemainingSeconds, 0.5f);
	TestEqual(TEXT("Restore returns attack identity authority to EnemyLogic"),
	          RestoredLogic.AttackSequence,
	          int64(8));
	TestEqual(TEXT("Restore returns world transform authority to Host"),
	          Restored->GetActorLocation(),
	          FVector(10.0f, 20.0f, 30.0f));

	Source->ReceiveGrayboxDamage(TNumericLimits<float>::Max(), Source->GetActorLocation());
	TestFalse(TEXT("Combat death immediately closes EnemyLogic"),
	          Source->GetEnemyLogicComponent()->GetSnapshot().bAlive);
	TestFalse(TEXT("Combat death immediately disables host collision"), Source->GetActorEnableCollision());
	TestEqual(TEXT("Roster reads death from EnemyLogic without a copied life flag"), Roster->GetLivingEnemyCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHostAttackPipelineTest,
                                 "ReEcho.Enemies.Host.AttackPipeline",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHostAttackPipelineTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyHostWorldFixture Fixture;
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	AReEchoPlayerPawn* Player = Fixture.World->SpawnActor<AReEchoPlayerPawn>(FVector::ZeroVector,
	                                                                       FRotator::ZeroRotator);
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

	AReEchoEnemyActor* Enemy = Fixture.World->SpawnActor<AReEchoEnemyActor>(FVector(50.0f, 0.0f, 0.0f),
	                                                                       FRotator::ZeroRotator);
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
	TestEqual(TEXT("Enemy ActionIntent reaches Combat exactly once"),
	          Player->Combatant->CurrentHealth,
	          91.0f);
	TestEqual(TEXT("Committed host attack uses EnemyLogic identity"),
	          Enemy->GetEnemyLogicComponent()->GetSnapshot().AttackSequence,
	          int64(1));
	Enemy->AdvanceBehaviorForTests(Sense, 0.5f);
	TestEqual(TEXT("Enemy cooldown prevents an early second hit"),
	          Player->Combatant->CurrentHealth,
	          91.0f);
	TestEqual(TEXT("Enemy remains in normal contact behavior"),
	          Enemy->GetEnemyLogicComponent()->GetSnapshot().Phase,
	          EReEchoEnemyBehaviorPhase::Idle);
	TestEqual(TEXT("Enemy cooldown advances while the target remains valid"),
	          Enemy->GetEnemyLogicComponent()->GetSnapshot().AttackCooldownRemainingSeconds,
	          0.8f);
	Enemy->AdvanceBehaviorForTests(Sense, 0.81f);
	TestTrue(TEXT("Target remains inside contact range"),
	         FVector::Dist2D(Enemy->GetActorLocation(), Player->GetActorLocation()) <= 85.0f);
	TestEqual(TEXT("Enemy cooldown permits the second hit at the legacy cadence"),
	          Player->Combatant->CurrentHealth,
	          82.0f);
	TestEqual(TEXT("Second attack restarts the legacy cooldown"),
	          Enemy->GetEnemyLogicComponent()->GetSnapshot().AttackCooldownRemainingSeconds,
	          1.3f);
	TestEqual(TEXT("Second host attack increments the same identity sequence"),
	          Enemy->GetEnemyLogicComponent()->GetSnapshot().AttackSequence,
	          int64(2));
	return true;
}

#endif
