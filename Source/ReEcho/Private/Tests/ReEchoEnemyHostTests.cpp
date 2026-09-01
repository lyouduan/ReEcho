#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Core/ReEchoRabbitProjectilePattern.h"
#include "Data/ReEchoBossPhase3Config.h"
#include "Encounter/ReEchoEncounterDirector.h"
#include "ReEchoGameMode.h"
#include "Data/ReEchoEnemyDefinitionCompiler.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "Enemies/ReEchoEnemyLogicComponent.h"
#include "Enemies/ReEchoEnemyRosterComponent.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Camera/CameraComponent.h"
#include "Presentation/Scene/ReEchoArenaCameraActor.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "PaperFlipbook.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DPresentationCatalog.h"
#include "Presentation/Enemy/ReEchoEnemyPresentationComponent.h"
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
	UPrimitiveComponent* RootCollision = Cast<UPrimitiveComponent>(Source->GetRootComponent());
	TestNotNull(TEXT("Enemy host has one root gameplay collision"), RootCollision);
	if (!RootCollision)
	{
		return false;
	}
	Source->SetCanBeDamaged(false);
	Source->SetActorEnableCollision(false);
	RootCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Source->Configure(EReEchoEnemyKind::Bomber, 12);
	TestTrue(TEXT("Commit configuration restores damage acceptance"), Source->CanBeDamaged());
	TestTrue(TEXT("Commit configuration restores actor collision"), Source->GetActorEnableCollision());
	TestTrue(TEXT("Commit configuration restores root query collision"), RootCollision->IsCollisionEnabled());
	const float HealthBeforeImmediateHit = Source->GetCombatantComponent()->CurrentHealth;
	TestEqual(TEXT("A committed enemy immediately accepts damage"),
	          Source->GetCombatantComponent()->ApplyFinalDamageForTests(1.0f),
	          1.0f);
	TestEqual(TEXT("Immediate post-commit damage changes health"),
	          Source->GetCombatantComponent()->CurrentHealth,
	          HealthBeforeImmediateHit - 1.0f);
	FReEchoEnemyLogicSnapshot PresentationTransform = Source->GetEnemyLogicComponent()->GetSnapshot();
	PresentationTransform.Phase = EReEchoEnemyBehaviorPhase::Transforming;
	Source->GetEnemyLogicComponent()->RestoreSnapshot(PresentationTransform);
	FReEchoHitIntent TransformingHit;
	TransformingHit.RawDamage = 2.0f;
	TestEqual(TEXT("Ordinary presentation-only transformation accepts incoming damage"),
	          Source->ModifyIncomingRawDamage(TransformingHit),
	          2.0f);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHostStunRetargetTest,
                                 "ReEcho.Enemies.Host.StunRetarget",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHostStunRetargetTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	if (!TestTrue(TEXT("Production enemy CSV loads for stun retarget"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}

	FReEchoEnemyDefinition FoxDefinition;
	FString CompileError;
	if (!TestTrue(
	        TEXT("Fox definition compiles for stun retarget"),
	        ReEchoEnemyDefinitionCompiler::Compile(*LoadResult.Snapshot, TEXT("M_FOX"), FoxDefinition, CompileError)))
	{
		AddError(CompileError);
		return false;
	}

	FReEchoEnemyHostWorldFixture Fixture;
	AReEchoEnemyActor* Fox = Fixture.World->SpawnActor<AReEchoEnemyActor>();
	AReEchoPlayerPawn* OldTarget =
	    Fixture.World->SpawnActor<AReEchoPlayerPawn>(FVector(500.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	AReEchoPlayerPawn* CurrentTarget =
	    Fixture.World->SpawnActor<AReEchoPlayerPawn>(FVector(0.0f, 500.0f, 0.0f), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Fox host spawns for stun retarget"), Fox) ||
	    !TestNotNull(TEXT("Old target spawns for stun retarget"), OldTarget) ||
	    !TestNotNull(TEXT("Current target spawns for stun retarget"), CurrentTarget))
	{
		return false;
	}
	if (!Fox->HasActorBegunPlay())
	{
		Fox->DispatchBeginPlay();
	}
	Fox->SetEnemyId(TEXT("M_FOX"));
	if (!TestTrue(TEXT("Fox accepts the production definition for stun retarget"),
	              Fox->ConfigureFromDefinition(FoxDefinition, 30)))
	{
		return false;
	}

	auto MakeSense = [Fox](AActor* Target)
	{
		FReEchoEnemySenseSnapshot Sense;
		Sense.Target = Target;
		Sense.SelfLocation = Fox->GetActorLocation();
		Sense.TargetLocation = Target->GetActorLocation();
		Sense.bTargetExists = true;
		Sense.bTargetAlive = true;
		Sense.bSpecialActionPermitted = true;
		return Sense;
	};

	Fox->AdvanceBehaviorForTests(MakeSense(OldTarget), 0.01f);
	FReEchoEnemyLogicSnapshot BeforeStun = Fox->GetEnemyLogicComponent()->GetSnapshot();
	TestEqual(TEXT("Fox begins windup against the old target"),
	          BeforeStun.SpecialActionPhase,
	          EReEchoEnemySpecialActionPhase::Windup);
	TestTrue(TEXT("Windup stores the old target location"),
	         BeforeStun.SpecialLockedTargetLocation.Equals(OldTarget->GetActorLocation(), KINDA_SMALL_NUMBER));
	BeforeStun.AttackCooldownRemainingSeconds = 0.75f;
	Fox->GetEnemyLogicComponent()->RestoreSnapshot(BeforeStun);
	Fox->GetEnemyEventsComponent()->ClearPublishedSpecialActionEventsForTests();

	Fox->UpdateStunStateForTests(true);
	const FReEchoEnemyLogicSnapshot DuringStun = Fox->GetEnemyLogicComponent()->GetSnapshot();
	TestEqual(TEXT("Entering stun cancels the target-locked action"),
	          DuringStun.SpecialActionPhase,
	          EReEchoEnemySpecialActionPhase::None);
	TestTrue(TEXT("Entering stun clears the old target location"),
	         DuringStun.SpecialLockedTargetLocation.IsNearlyZero());
	TestEqual(TEXT("Entering stun preserves the existing cooldown"), DuringStun.AttackCooldownRemainingSeconds, 0.75f);
	const TArray<FReEchoEnemySpecialActionEvent>& CancellationEvents =
	    Fox->GetEnemyEventsComponent()->GetPublishedSpecialActionEventsForTests();
	TestEqual(TEXT("Entering stun publishes one cancellation"), CancellationEvents.Num(), 1);
	if (CancellationEvents.Num() == 1)
	{
		TestEqual(TEXT("The terminal event is an explicit cancellation"),
		          CancellationEvents[0].Type,
		          EReEchoEnemySpecialActionEventType::ActionCancelled);
	}
	Fox->UpdateStunStateForTests(true);
	TestEqual(TEXT("Extending the same stun does not cancel twice"),
	          Fox->GetEnemyEventsComponent()->GetPublishedSpecialActionEventsForTests().Num(),
	          1);

	Fox->UpdateStunStateForTests(false);
	const FReEchoEnemyActionIntent FirstRecoveredStep = Fox->AdvanceBehaviorForTests(MakeSense(CurrentTarget), 0.01f);
	TestTrue(TEXT("The first recovered step faces the current target"),
	         FirstRecoveredStep.bHasFacing &&
	             FirstRecoveredStep.FacingDirection.Equals(FVector::RightVector, KINDA_SMALL_NUMBER));
	Fox->AdvanceBehaviorForTests(MakeSense(CurrentTarget), 0.75f);
	const FReEchoEnemyLogicSnapshot Retargeted = Fox->GetEnemyLogicComponent()->GetSnapshot();
	TestEqual(TEXT("Fox can begin a fresh action after its preserved cooldown"),
	          Retargeted.SpecialActionPhase,
	          EReEchoEnemySpecialActionPhase::Windup);
	TestTrue(TEXT("The fresh action locks the current target instead of the old one"),
	         Retargeted.SpecialLockedTargetLocation.Equals(CurrentTarget->GetActorLocation(), KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHostSheepStunImmunityTest,
                                 "ReEcho.Enemies.Host.SheepStunImmunity",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHostSheepStunImmunityTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	if (!TestTrue(TEXT("Production enemy CSV loads for sheep stun immunity"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}

	FReEchoEnemyDefinition SheepDefinition;
	FString CompileError;
	if (!TestTrue(TEXT("Sheep definition compiles for stun immunity"),
	              ReEchoEnemyDefinitionCompiler::Compile(
	                  *LoadResult.Snapshot, TEXT("M_SHEEP"), SheepDefinition, CompileError)))
	{
		AddError(CompileError);
		return false;
	}

	FReEchoEnemyHostWorldFixture Fixture;
	AReEchoEnemyActor* Sheep = Fixture.World->SpawnActor<AReEchoEnemyActor>();
	if (!TestNotNull(TEXT("Sheep host spawns for stun immunity"), Sheep))
	{
		return false;
	}
	Sheep->SetEnemyId(TEXT("M_SHEEP"));
	if (!TestTrue(TEXT("Sheep accepts production definition for stun immunity"),
	              Sheep->ConfigureFromDefinition(SheepDefinition, 31)))
	{
		return false;
	}

	UReEchoCombatantComponent* Combatant = Sheep->GetCombatantComponent();
	if (!TestNotNull(TEXT("Sheep has combat authority"), Combatant))
	{
		return false;
	}
	TestTrue(TEXT("M_SHEEP configures combat stun immunity"), Combatant->IsStunImmune());

	FReEchoTimedStatusCommand WeaponStun;
	WeaponStun.StatusId = TEXT("Z_Vertigo");
	WeaponStun.CurrentTimeSeconds = Fixture.World->GetTimeSeconds();
	WeaponStun.DurationSeconds = 10.0f;
	TestFalse(TEXT("M_SHEEP rejects weapon/status Vertigo"), Combatant->ApplyTimedStatus(WeaponStun));
	Sheep->ApplyCardStun(10.0f);
	TestFalse(TEXT("M_SHEEP remains action-enabled after card stun"),
	          Combatant->IsActionDisabled(Fixture.World->GetTimeSeconds() + 1.0f));
	TestFalse(TEXT("M_SHEEP never records Vertigo in element state"),
	          Combatant->GetElementState().ActiveStatusUntilSeconds.Contains(TEXT("Z_Vertigo")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHostBossEchoTauntImmunityTest,
                                 "ReEcho.Enemies.Host.BossEchoTauntImmunity",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHostBossEchoTauntImmunityTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	if (!TestTrue(TEXT("Production enemy CSV loads for Boss taunt immunity"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}

	FReEchoEnemyDefinition SheepDefinition;
	FString CompileError;
	if (!TestTrue(TEXT("Sheep definition compiles for Boss taunt immunity"),
	              ReEchoEnemyDefinitionCompiler::Compile(
	                  *LoadResult.Snapshot, TEXT("M_SHEEP"), SheepDefinition, CompileError)))
	{
		AddError(CompileError);
		return false;
	}

	FReEchoEnemyHostWorldFixture Fixture;
	AActor* PlayerTarget = Fixture.World->SpawnActor<AActor>(FVector(600.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	AReEchoEchoActor* Echo =
	    Fixture.World->SpawnActor<AReEchoEchoActor>(FVector(100.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	AReEchoEnemyActor* Grunt = Fixture.Spawn(EReEchoEnemyKind::Grunt, 32);
	AReEchoEnemyActor* Sheep = Fixture.World->SpawnActor<AReEchoEnemyActor>();
	if (!TestNotNull(TEXT("Default player target spawns"), PlayerTarget) ||
	    !TestNotNull(TEXT("Echo target spawns"), Echo) || !TestNotNull(TEXT("Ordinary enemy host spawns"), Grunt) ||
	    !TestNotNull(TEXT("Boss host spawns"), Sheep))
	{
		return false;
	}

	FReEchoStatBlock EchoStats;
	EchoStats.HpMax = 100.0f;
	EchoStats.HpPoint = 100.0f;
	Echo->GetCombatTargetCombatant()->InitializeFromStats(EchoStats, true);
	Sheep->SetEnemyId(TEXT("M_SHEEP"));
	if (!TestTrue(TEXT("Sheep accepts production definition for Boss taunt immunity"),
	              Sheep->ConfigureFromDefinition(SheepDefinition, 33)))
	{
		return false;
	}

	TestEqual(TEXT("Ordinary enemy selects a living Echo when taunt is active"),
	          Grunt->ResolveAggroTargetForTests(PlayerTarget, true),
	          static_cast<AActor*>(Echo));
	TestEqual(TEXT("Boss keeps the player target when Echo taunt is active"),
	          Sheep->ResolveAggroTargetForTests(PlayerTarget, true),
	          PlayerTarget);
	TestEqual(TEXT("Ordinary enemy keeps the player target when Echo taunt is inactive"),
	          Grunt->ResolveAggroTargetForTests(PlayerTarget, false),
	          PlayerTarget);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHostFoxDashCollisionTest,
                                 "ReEcho.Enemies.Host.FoxDashCollision",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHostFoxDashCollisionTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	if (!TestTrue(TEXT("Production enemy CSV loads for Fox dash"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}
	FReEchoEnemyDefinition FoxDefinition;
	FString CompileError;
	if (!TestTrue(
	        TEXT("Fox definition compiles"),
	        ReEchoEnemyDefinitionCompiler::Compile(*LoadResult.Snapshot, TEXT("M_FOX"), FoxDefinition, CompileError)))
	{
		AddError(CompileError);
		return false;
	}
	const FReEchoEnemyAbilityDefinition* Dash = FoxDefinition.Abilities.FindByPredicate(
	    [](const FReEchoEnemyAbilityDefinition& Ability)
	    {
		    return Ability.Id == TEXT("M_FOX_Dash");
	    });
	if (!TestNotNull(TEXT("Production Fox keeps its dash ability"), Dash))
	{
		return false;
	}

	auto InitializePlayer = [](AReEchoPlayerPawn* Player)
	{
		if (!Player->HasActorBegunPlay())
		{
			Player->DispatchBeginPlay();
		}
		Player->SetAutoAttackMode(false);
		FReEchoStatBlock PlayerStats;
		PlayerStats.HpMax = 100.0f;
		PlayerStats.HpPoint = 100.0f;
		Player->Combatant->InitializeFromStats(PlayerStats, true);
	};
	auto MakeSense = [](AReEchoEnemyActor* Fox, AReEchoPlayerPawn* Player, const bool bInvulnerable)
	{
		FReEchoEnemySenseSnapshot Sense;
		Sense.Target = Player;
		Sense.SelfLocation = Fox->GetActorLocation();
		Sense.TargetLocation = Player->GetActorLocation();
		Sense.bTargetExists = true;
		Sense.bTargetAlive = Player->IsCombatTargetAlive();
		Sense.bTargetInvulnerable = bInvulnerable;
		Sense.bSpecialActionPermitted = true;
		return Sense;
	};

	{
		FReEchoEnemyHostWorldFixture Fixture;
		AReEchoPlayerPawn* Player =
		    Fixture.World->SpawnActor<AReEchoPlayerPawn>(FVector(300.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
		AReEchoEnemyActor* Fox =
		    Fixture.World->SpawnActor<AReEchoEnemyActor>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (!TestNotNull(TEXT("Fox dash target spawns"), Player) || !TestNotNull(TEXT("Fox host spawns"), Fox))
		{
			return false;
		}
		InitializePlayer(Player);
		Fox->SetEnemyId(TEXT("M_FOX"));
		TestTrue(TEXT("Fox accepts the production definition"), Fox->ConfigureFromDefinition(FoxDefinition, 20));
		Fox->SetActorLocation(FVector::ZeroVector);
		Player->SetActorLocation(FVector(300.0f, 0.0f, 0.0f));
		Fox->AdvanceBehaviorForTests(MakeSense(Fox, Player, false), 0.01f);
		const FReEchoEnemyActionIntent Commit =
		    Fox->AdvanceBehaviorForTests(MakeSense(Fox, Player, false), Dash->WindupSeconds + 0.01f);
		TestTrue(TEXT("Fox Host receives the dash commit"), Commit.bAttackCommitted);
		TestFalse(TEXT("Dash commit does not teleport the Fox"), Commit.bHasMovement);

		int32 ActiveMovementSteps = 0;
		float LargestStepCm = 0.0f;
		for (int32 StepIndex = 0; StepIndex < 20; ++StepIndex)
		{
			const FReEchoEnemyActionIntent Step =
			    Fox->AdvanceBehaviorForTests(MakeSense(Fox, Player, false), 1.0f / 60.0f);
			if (Step.bSpecialDashMovement && Step.bHasMovement)
			{
				++ActiveMovementSteps;
				LargestStepCm = FMath::Max(LargestStepCm, Step.MovementDelta.Size2D());
			}
			if (Fox->GetEnemyLogicComponent()->GetSnapshot().SpecialActionPhase ==
			    EReEchoEnemySpecialActionPhase::Recovery)
			{
				break;
			}
		}
		TestTrue(TEXT("Fox visibly advances across more than one Active step"), ActiveMovementSteps > 1);
		TestTrue(TEXT("No Active step contains the complete dash distance"), LargestStepCm < Dash->LengthCm);
		TestEqual(TEXT("First swept contact applies production Fox damage exactly once"),
		          Player->Combatant->CurrentHealth,
		          100.0f - Dash->Damage);
		TestTrue(TEXT("First swept contact consumes the saved one-shot gate"),
		         Fox->GetEnemyLogicComponent()->GetSnapshot().bSpecialDamageConsumed);
		Fox->AdvanceBehaviorForTests(MakeSense(Fox, Player, false), Dash->RecoverySeconds * 0.5f);
		TestEqual(
		    TEXT("Recovery cannot repeat Fox dash damage"), Player->Combatant->CurrentHealth, 100.0f - Dash->Damage);
	}

	{
		FReEchoEnemyHostWorldFixture Fixture;
		AReEchoPlayerPawn* Player =
		    Fixture.World->SpawnActor<AReEchoPlayerPawn>(FVector(300.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
		AReEchoEnemyActor* Fox =
		    Fixture.World->SpawnActor<AReEchoEnemyActor>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (!Player || !Fox)
		{
			return false;
		}
		InitializePlayer(Player);
		Fox->SetEnemyId(TEXT("M_FOX"));
		Fox->ConfigureFromDefinition(FoxDefinition, 21);
		Fox->SetActorLocation(FVector::ZeroVector);
		Player->SetActorLocation(FVector(300.0f, 0.0f, 0.0f));
		Fox->AdvanceBehaviorForTests(MakeSense(Fox, Player, false), 0.01f);
		Fox->AdvanceBehaviorForTests(MakeSense(Fox, Player, false), Dash->WindupSeconds + 0.01f);
		Player->SetActorLocation(FVector(300.0f, 400.0f, 0.0f));
		float AttemptedDistanceCm = 0.0f;
		for (int32 StepIndex = 0; StepIndex < 20; ++StepIndex)
		{
			const FReEchoEnemyActionIntent Step =
			    Fox->AdvanceBehaviorForTests(MakeSense(Fox, Player, false), 1.0f / 60.0f);
			AttemptedDistanceCm += Step.MovementDelta.Size2D();
			if (Fox->GetEnemyLogicComponent()->GetSnapshot().SpecialActionPhase ==
			    EReEchoEnemySpecialActionPhase::Recovery)
			{
				break;
			}
		}
		TestTrue(TEXT("A dodged target takes no path damage"),
		         FMath::IsNearlyEqual(Player->Combatant->CurrentHealth, 100.0f));
		TestTrue(TEXT("An unobstructed dash attempts the exact authored distance"),
		         FMath::IsNearlyEqual(AttemptedDistanceCm, Dash->LengthCm, 0.01f));
	}

	{
		FReEchoEnemyHostWorldFixture Fixture;
		AReEchoPlayerPawn* Player =
		    Fixture.World->SpawnActor<AReEchoPlayerPawn>(FVector(300.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
		AReEchoEnemyActor* Fox =
		    Fixture.World->SpawnActor<AReEchoEnemyActor>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (!Player || !Fox)
		{
			return false;
		}
		InitializePlayer(Player);
		Fox->SetEnemyId(TEXT("M_FOX"));
		Fox->ConfigureFromDefinition(FoxDefinition, 22);
		Fox->SetActorLocation(FVector::ZeroVector);
		Player->SetActorLocation(FVector(300.0f, 0.0f, 0.0f));
		Fox->AdvanceBehaviorForTests(MakeSense(Fox, Player, false), 0.01f);
		Fox->AdvanceBehaviorForTests(MakeSense(Fox, Player, false), Dash->WindupSeconds + 0.01f);
		for (int32 StepIndex = 0; StepIndex < 20; ++StepIndex)
		{
			Fox->AdvanceBehaviorForTests(MakeSense(Fox, Player, true), 1.0f / 60.0f);
			if (Fox->GetEnemyLogicComponent()->GetSnapshot().SpecialActionPhase ==
			    EReEchoEnemySpecialActionPhase::Recovery)
			{
				break;
			}
		}
		TestTrue(TEXT("Invulnerable target takes no Fox dash damage"),
		         FMath::IsNearlyEqual(Player->Combatant->CurrentHealth, 100.0f));
		TestTrue(TEXT("Invulnerable contact still consumes the one-shot gate"),
		         Fox->GetEnemyLogicComponent()->GetSnapshot().bSpecialDamageConsumed);
	}

	{
		FReEchoEnemyHostWorldFixture Fixture;
		AReEchoPlayerPawn* Player =
		    Fixture.World->SpawnActor<AReEchoPlayerPawn>(FVector(600.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
		AReEchoEnemyActor* Fox =
		    Fixture.World->SpawnActor<AReEchoEnemyActor>(FVector::ZeroVector, FRotator::ZeroRotator);
		AActor* Wall = Fixture.World->SpawnActor<AActor>(FVector(180.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
		if (!Player || !Fox || !Wall)
		{
			return false;
		}
		InitializePlayer(Player);
		UBoxComponent* WallCollision = NewObject<UBoxComponent>(Wall, TEXT("FoxDashWall"));
		Wall->SetRootComponent(WallCollision);
		WallCollision->SetBoxExtent(FVector(20.0f, 200.0f, 200.0f));
		WallCollision->SetCollisionProfileName(TEXT("BlockAll"));
		WallCollision->RegisterComponent();
		Wall->SetActorLocation(FVector(180.0f, 0.0f, 0.0f));
		Fox->SetEnemyId(TEXT("M_FOX"));
		Fox->ConfigureFromDefinition(FoxDefinition, 23);
		Fox->SetActorLocation(FVector::ZeroVector);
		Player->SetActorLocation(FVector(600.0f, 0.0f, 0.0f));
		Fox->AdvanceBehaviorForTests(MakeSense(Fox, Player, false), 0.01f);
		Fox->AdvanceBehaviorForTests(MakeSense(Fox, Player, false), Dash->WindupSeconds + 0.01f);
		for (int32 StepIndex = 0; StepIndex < 20; ++StepIndex)
		{
			Fox->AdvanceBehaviorForTests(MakeSense(Fox, Player, false), 1.0f / 60.0f);
			if (Fox->GetEnemyLogicComponent()->GetSnapshot().SpecialActionPhase ==
			    EReEchoEnemySpecialActionPhase::Recovery)
			{
				break;
			}
		}
		TestEqual(TEXT("World blocker ends the dash in Recovery"),
		          Fox->GetEnemyLogicComponent()->GetSnapshot().SpecialActionPhase,
		          EReEchoEnemySpecialActionPhase::Recovery);
		TestTrue(TEXT("World blocker before the target prevents damage"),
		         FMath::IsNearlyEqual(Player->Combatant->CurrentHealth, 100.0f));
		TestTrue(TEXT("Blocked dash does not compensate by teleporting to the authored endpoint"),
		         Fox->GetActorLocation().X < Dash->LengthCm);
	}
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
	const FReEchoEnemyAbilityDefinition* MovingVolley = RabbitDefinition.Abilities.FindByPredicate(
	    [](const FReEchoEnemyAbilityDefinition& Ability)
	    {
		    return Ability.Id == TEXT("M_RABBIT_MovingVolley");
	    });
	if (!TestNotNull(TEXT("Production rabbit keeps the moving-volley ability"), MovingVolley))
	{
		return false;
	}
	const FReEchoEnemyAbilityDefinition* StationaryVolley = RabbitDefinition.Abilities.FindByPredicate(
	    [](const FReEchoEnemyAbilityDefinition& Ability)
	    {
		    return Ability.Id == TEXT("M_RABBIT_RangedBurst");
	    });
	if (!TestNotNull(TEXT("Production rabbit keeps the stationary-volley ability"), StationaryVolley))
	{
		return false;
	}
	const int32 MovingVolleyCount = FMath::Max(1, MovingVolley->ProjectileCount);
	const float FixedRabbitBallRadius =
	    ReEchoRabbitProjectilePattern::ResolveBallCollisionRadius(MovingVolley->RadiusCm);
	const float StationaryShotInterval =
	    StationaryVolley->ActiveSeconds / static_cast<float>(FMath::Max(1, StationaryVolley->ProjectileCount - 1));
	const float MovingProjectileSpeed = MovingVolley->ProjectileSpeedCmPerSecond > 0.0f
	                                        ? MovingVolley->ProjectileSpeedCmPerSecond
	                                        : MovingVolley->MaxRangeCm / MovingVolley->CooldownSeconds;

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
	for (int32 BallIndex = 0; BallIndex < SpawnedState.BossProjectiles.Num(); ++BallIndex)
	{
		const FReEchoEnemyProjectileRuntimeState& Ball = SpawnedState.BossProjectiles[BallIndex];
		TestEqual(TEXT("Rabbit ball keeps the authored damage"), Ball.Damage, 1.0f);
		TestEqual(TEXT("Rabbit volley radius is divided into one collider per ball"),
		          Ball.CollisionRadiusCm,
		          FixedRabbitBallRadius);
		TestEqual(TEXT("Rabbit ball stores its stable volley index"), Ball.VolleyBallIndex, BallIndex);
		TestEqual(TEXT("Every rabbit ball uses the same resolved data-driven speed"),
		          Ball.Definition.SpeedCmPerSecond,
		          MovingProjectileSpeed);
		TestTrue(TEXT("Resolved rabbit ball speed is positive"), Ball.Definition.SpeedCmPerSecond > 0.0f);
		TestTrue(TEXT("Rabbit ball starts active"), Ball.Snapshot.bActive);
		TestFalse(TEXT("Rabbit ball starts without a consumed collision"), Ball.bCollisionConsumed);
		TestTrue(TEXT("Rabbit ball uses the authoritative fan direction"),
		         Ball.Definition.Direction.Equals(
		             ReEchoRabbitProjectilePattern::ResolveVolleyDirection(
		                 FVector::ForwardVector, MovingVolley->SpreadAngleDegrees, BallIndex, MovingVolleyCount),
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
		          FixedRabbitBallRadius);
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

	const int32 SwordCutCount =
	    Rabbit->DestroyRabbitProjectilesInMeleeArc(FVector::ZeroVector, FVector::ForwardVector, 700.0f, 180.0f);
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

	FReEchoEnemyLogicSnapshot StationaryWindup = Rabbit->GetEnemyLogicComponent()->GetSnapshot();
	StationaryWindup.Phase = EReEchoEnemyBehaviorPhase::Attacking;
	StationaryWindup.AttackCooldownRemainingSeconds = 0.0f;
	StationaryWindup.SpecialActionPhase = EReEchoEnemySpecialActionPhase::Windup;
	StationaryWindup.SpecialAbilityId = TEXT("M_RABBIT_RangedBurst");
	StationaryWindup.SpecialActionRemainingSeconds = 0.0f;
	StationaryWindup.SpecialLockedTargetLocation = FVector(600.0f, 0.0f, ProjectileGameplayZ);
	StationaryWindup.SpecialLockedDirection = FVector::ForwardVector;
	Rabbit->GetEnemyLogicComponent()->RestoreSnapshot(StationaryWindup);
	Sense.TargetLocation = FVector(600.0f, 0.0f, ProjectileGameplayZ);
	Player->SetActorLocation(FVector(0.0f, 1000.0f, ProjectileGameplayZ));
	Rabbit->GetEnemyEventsComponent()->ClearPublishedProjectileEventsForTests();
	const FReEchoEnemyActionIntent StationaryCommit = Rabbit->AdvanceBehaviorForTests(Sense, 0.0f);
	TestTrue(TEXT("Stationary rabbit burst commits from its restored active ability"),
	         StationaryCommit.bAttackCommitted);

	const FReEchoEnemyRuntimeState StationaryStart = Rabbit->CaptureRuntimeState();
	if (!TestEqual(
	        TEXT("Stationary burst queues all four authoritative balls"), StationaryStart.BossProjectiles.Num(), 4))
	{
		return false;
	}
	int32 PublishedAtCommit = 0;
	for (const FReEchoEnemyProjectileRuntimeState& Ball : StationaryStart.BossProjectiles)
	{
		PublishedAtCommit += Ball.bSpawnEventPublished ? 1 : 0;
		TestEqual(TEXT("Stationary and moving rabbit shots keep the same fixed collision radius"),
		          Ball.CollisionRadiusCm,
		          FixedRabbitBallRadius);
		TestTrue(TEXT("Stationary burst balls share one locked direction"),
		         Ball.Definition.Direction.Equals(FVector::ForwardVector, KINDA_SMALL_NUMBER));
	}
	TestEqual(TEXT("Only the first stationary shot enters the world at commit"), PublishedAtCommit, 1);

	Rabbit->AdvanceEnemyProjectilesForTests(StationaryShotInterval + 0.001f);
	const FReEchoEnemyRuntimeState MidBurst = Rabbit->CaptureRuntimeState();
	int32 PublishedAfterFirstInterval = 0;
	for (const FReEchoEnemyProjectileRuntimeState& Ball : MidBurst.BossProjectiles)
	{
		PublishedAfterFirstInterval += Ball.bSpawnEventPublished ? 1 : 0;
	}
	TestEqual(TEXT("One interval later exactly two stationary shots have spawned"), PublishedAfterFirstInterval, 2);
	Rabbit->GetEnemyEventsComponent()->ClearPublishedProjectileEventsForTests();
	Rabbit->RestoreRuntimeState(MidBurst);
	int32 RestoredSpawnEvents = 0;
	for (const FReEchoEnemyProjectileEvent& Event :
	     Rabbit->GetEnemyEventsComponent()->GetPublishedProjectileEventsForTests())
	{
		RestoredSpawnEvents += Event.Type == EReEchoEnemyProjectileEventType::Spawned ? 1 : 0;
	}
	TestEqual(TEXT("Restore republishes only shots that had already entered the world"), RestoredSpawnEvents, 2);

	Rabbit->GetEnemyEventsComponent()->ClearPublishedProjectileEventsForTests();
	Rabbit->AdvanceEnemyProjectilesForTests(StationaryShotInterval);
	int32 ThirdShotEvents = 0;
	for (const FReEchoEnemyProjectileEvent& Event :
	     Rabbit->GetEnemyEventsComponent()->GetPublishedProjectileEventsForTests())
	{
		ThirdShotEvents += Event.Type == EReEchoEnemyProjectileEventType::Spawned ? 1 : 0;
	}
	TestEqual(TEXT("The third stationary shot spawns after its remaining delay"), ThirdShotEvents, 1);
	Rabbit->GetEnemyEventsComponent()->ClearPublishedProjectileEventsForTests();
	Rabbit->AdvanceEnemyProjectilesForTests(StationaryShotInterval);
	int32 FourthShotEvents = 0;
	for (const FReEchoEnemyProjectileEvent& Event :
	     Rabbit->GetEnemyEventsComponent()->GetPublishedProjectileEventsForTests())
	{
		FourthShotEvents += Event.Type == EReEchoEnemyProjectileEventType::Spawned ? 1 : 0;
	}
	TestEqual(TEXT("The fourth stationary shot spawns last instead of overlapping at commit"), FourthShotEvents, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBossDamageGeometryTest,
                                 "ReEcho.Enemies.Host.BossDamageGeometry",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBossDamageGeometryTest::RunTest(const FString& Parameters)
{
	FReEchoBossIntent BlinkSlam;
	BlinkSlam.AttackShape = EReEchoBossAttackShape::Circle;
	BlinkSlam.Origin = FVector(600.0f, -200.0f, 50.0f);
	BlinkSlam.RadiusCm = 180.0f;
	TestTrue(TEXT("Blink Slam damages inside its locked warning circle"),
	         AReEchoEnemyActor::IntersectsBossDamageShape(BlinkSlam, FVector(779.0f, -200.0f, 0.0f)));
	TestFalse(TEXT("Blink Slam does not damage outside its locked warning circle"),
	          AReEchoEnemyActor::IntersectsBossDamageShape(BlinkSlam, FVector(781.0f, -200.0f, 0.0f)));

	FReEchoBossIntent Projectile;
	Projectile.AttackShape = EReEchoBossAttackShape::Projectile;
	Projectile.Origin = BlinkSlam.Origin;
	Projectile.RadiusCm = 100.0f;
	TestFalse(TEXT("Skill02 never resolves immediate AOE damage from its ability radius"),
	          AReEchoEnemyActor::IntersectsBossDamageShape(Projectile, Projectile.Origin));

	FReEchoBossIntent Beam;
	Beam.AbilityId = TEXT("M_SHEEP_PrayerBeam");
	Beam.AttackShape = EReEchoBossAttackShape::Beam;
	Beam.Origin = FVector(100.0f, 200.0f, 0.0f);
	Beam.LockedTargetLocation = FVector(500.0f, 300.0f, 0.0f);
	Beam.LockedDirection = FVector::ForwardVector;
	Beam.LengthCm = 1200.0f;
	Beam.WidthCm = 160.0f;
	TestTrue(TEXT("Prayer Beam damages upward from its locked warning center"),
	         AReEchoEnemyActor::IntersectsBossDamageShape(Beam, FVector(1400.0f, 379.0f, 0.0f)));
	TestFalse(TEXT("Prayer Beam rejects targets below its locked warning center"),
	          AReEchoEnemyActor::IntersectsBossDamageShape(Beam, FVector(499.0f, 300.0f, 0.0f)));
	TestFalse(TEXT("Prayer Beam rejects targets outside its locked width"),
	          AReEchoEnemyActor::IntersectsBossDamageShape(Beam, FVector(1400.0f, 381.0f, 0.0f)));

	FReEchoBossIntent Melee;
	Melee.AbilityId = TEXT("M_SHEEP_MeleeSweep");
	Melee.AttackShape = EReEchoBossAttackShape::Rectangle;
	// Host replaces the gameplay Intent origin with BossWeaponRoot before evaluating this shared geometry.
	Melee.Origin = FVector(100.0f, 200.0f, 0.0f);
	Melee.LockedDirection = FVector::ForwardVector;
	Melee.LengthCm = 260.0f;
	TestTrue(TEXT("Melee Sweep damages within the staff-pivoted front 180-degree semicircle"),
	         AReEchoEnemyActor::IntersectsBossDamageShape(Melee, FVector(100.0f, 450.0f, 0.0f)));
	TestFalse(TEXT("Melee Sweep rejects targets behind the Boss"),
	          AReEchoEnemyActor::IntersectsBossDamageShape(Melee, FVector(99.0f, 200.0f, 0.0f)));
	TestFalse(TEXT("Melee Sweep rejects targets beyond its configured reach"),
	          AReEchoEnemyActor::IntersectsBossDamageShape(Melee, FVector(361.0f, 200.0f, 0.0f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHostSheepProjectileTest,
                                 "ReEcho.Enemies.Host.SheepProjectilePipeline",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHostSheepProjectileTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyHostWorldFixture Fixture;
	const FReEchoCsvLoadResult LoadResult =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	if (!TestTrue(TEXT("Production enemy CSV loads"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}
	FReEchoEnemyDefinition SheepDefinition;
	FString CompileError;
	if (!TestTrue(TEXT("Sheep definition compiles"),
	              ReEchoEnemyDefinitionCompiler::Compile(
	                  *LoadResult.Snapshot, TEXT("M_SHEEP"), SheepDefinition, CompileError)))
	{
		AddError(CompileError);
		return false;
	}
	const FReEchoEnemyAbilityDefinition* StationaryVolley = SheepDefinition.Abilities.FindByPredicate(
	    [](const FReEchoEnemyAbilityDefinition& Ability)
	    {
		    return Ability.Id == TEXT("M_SHEEP_StationaryVolley");
	    });
	if (!TestNotNull(TEXT("Production sheep keeps the stationary-volley ability"), StationaryVolley))
	{
		return false;
	}
	const FReEchoEnemyAbilityDefinition* MovingSpread = SheepDefinition.Abilities.FindByPredicate(
	    [](const FReEchoEnemyAbilityDefinition& Ability)
	    {
		    return Ability.Id == TEXT("M_SHEEP_MovingSpread");
	    });
	if (!TestNotNull(TEXT("Production sheep keeps the moving-spread ability"), MovingSpread))
	{
		return false;
	}
	const FReEchoEnemyAbilityDefinition* PrayerBeam = SheepDefinition.Abilities.FindByPredicate(
	    [](const FReEchoEnemyAbilityDefinition& Ability)
	    {
		    return Ability.Id == TEXT("M_SHEEP_PrayerBeam");
	    });
	if (!TestNotNull(TEXT("Production sheep keeps the prayer-beam ability"), PrayerBeam))
	{
		return false;
	}
	TestEqual(TEXT("Prayer beam uses a three-second visual and damage window"), PrayerBeam->ActiveSeconds, 3.0f);

	AReEchoPlayerPawn* Player =
	    Fixture.World->SpawnActor<AReEchoPlayerPawn>(FVector(600.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	AReEchoEnemyActor* Sheep = Fixture.World->SpawnActor<AReEchoEnemyActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Sheep host spawns"), Sheep) || !TestNotNull(TEXT("Target player spawns"), Player))
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
	APlayerController* PlayerController = Fixture.World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("Player controller spawns"), PlayerController))
	{
		return false;
	}
	PlayerController->Possess(Player);
	Sheep->SetEnemyId(TEXT("M_SHEEP"));
	if (!TestTrue(TEXT("Sheep accepts production definition"), Sheep->ConfigureFromDefinition(SheepDefinition, 15)))
	{
		return false;
	}
	if (!Sheep->HasActorBegunPlay())
	{
		Sheep->DispatchBeginPlay();
	}
	TestTrue(TEXT("Stationary volley can be queued"),
	         Sheep->GetEnemyLogicComponent()->DebugQueueBossAbility(TEXT("M_SHEEP_StationaryVolley")));

	FReEchoEnemySenseSnapshot Sense;
	Sense.Target = Player;
	Sense.SelfLocation = Sheep->GetActorLocation();
	Sense.TargetLocation = Player->GetActorLocation();
	Sense.bTargetExists = true;
	Sense.bTargetAlive = true;
	Sheep->AdvanceBehaviorForTests(Sense, 0.01f);
	Sheep->AdvanceBehaviorForTests(Sense, StationaryVolley->WindupSeconds + 0.1f);

	const FReEchoEnemyRuntimeState SpawnedState = Sheep->CaptureRuntimeState();
	if (!TestEqual(TEXT("Stationary volley creates four authoritative sheep projectiles"),
	               SpawnedState.BossProjectiles.Num(),
	               4))
	{
		return false;
	}
	for (const FReEchoEnemyProjectileRuntimeState& Ball : SpawnedState.BossProjectiles)
	{
		TestEqual(
		    TEXT("Sheep projectile uses ability MaxRangeCm"), Ball.Definition.MaxRangeCm, StationaryVolley->MaxRangeCm);
		TestEqual(TEXT("Sheep volley divides the configured radius per ball"),
		          Ball.CollisionRadiusCm,
		          ReEchoRabbitProjectilePattern::ResolveBallCollisionRadius(StationaryVolley->RadiusCm,
		                                                                    StationaryVolley->ProjectileCount));
	}
	TestTrue(TEXT("Sheep projectile presentation height differs from the shorter player collision center"),
	         !FMath::IsNearlyEqual(SpawnedState.BossProjectiles[0].Snapshot.Location.Z,
	                               Player->GetCombatTargetLocation().Z));
	const TArray<FReEchoEnemyProjectileEvent>& Events =
	    Sheep->GetEnemyEventsComponent()->GetPublishedProjectileEventsForTests();
	const int32 SpawnCount = Events
	                             .FilterByPredicate(
	                                 [](const FReEchoEnemyProjectileEvent& Event)
	                                 {
		                                 return Event.Type == EReEchoEnemyProjectileEventType::Spawned &&
		                                        Event.AbilityId == TEXT("M_SHEEP_Projectile");
	                                 })
	                             .Num();
	TestEqual(TEXT("Stationary volley initially publishes only its first projectile"), SpawnCount, 1);
	if (UReEchoCombatVfxComponent* Vfx = Sheep->FindComponentByClass<UReEchoCombatVfxComponent>();
	    Vfx && FApp::CanEverRender())
	{
		TestEqual(TEXT("Only the first sequential Goat Skill02 Bullet is visible immediately"),
		          Vfx->GetBossProjectileEffectCountForTests(),
		          1);
	}
	Sheep->AdvanceEnemyProjectilesForTests(1.0f);
	Sheep->AdvanceEnemyProjectilesForTests(0.5f);
	const int32 TotalSpawnCount = Events
	                                  .FilterByPredicate(
	                                      [](const FReEchoEnemyProjectileEvent& Event)
	                                      {
		                                      return Event.Type == EReEchoEnemyProjectileEventType::Spawned &&
		                                             Event.AbilityId == TEXT("M_SHEEP_Projectile");
	                                      })
	                                  .Num();
	TestEqual(TEXT("Stationary volley publishes all four projectiles over its firing window"), TotalSpawnCount, 4);
	TestEqual(TEXT("Stationary volley applies the configured damage for every projectile"),
	          Player->Combatant->CurrentHealth,
	          100.0f - StationaryVolley->Damage * StationaryVolley->ProjectileCount);

	Player->Combatant->RestoreCurrentHealth(100.0f);
	TestTrue(TEXT("Moving spread can be queued while the previous ability recovers"),
	         Sheep->GetEnemyLogicComponent()->DebugQueueBossAbility(TEXT("M_SHEEP_MovingSpread")));
	Sheep->AdvanceBehaviorForTests(Sense, StationaryVolley->RecoverySeconds + MovingSpread->WindupSeconds + 0.2f);
	const FReEchoEnemyRuntimeState MovingSpreadState = Sheep->CaptureRuntimeState();
	TestEqual(
	    TEXT("Moving spread creates three authoritative projectiles"), MovingSpreadState.BossProjectiles.Num(), 3);
	const int32 SpawnCountBeforeMoving = TotalSpawnCount;
	const int32 SpawnCountAfterMoving = Events
	                                        .FilterByPredicate(
	                                            [](const FReEchoEnemyProjectileEvent& Event)
	                                            {
		                                            return Event.Type == EReEchoEnemyProjectileEventType::Spawned &&
		                                                   Event.AbilityId == TEXT("M_SHEEP_Projectile");
	                                            })
	                                        .Num();
	TestEqual(TEXT("Moving spread publishes all three directions immediately"),
	          SpawnCountAfterMoving - SpawnCountBeforeMoving,
	          3);
	Sheep->AdvanceEnemyProjectilesForTests(1.0f);
	Sheep->AdvanceEnemyProjectilesForTests(0.5f);
	TestEqual(TEXT("Moving spread center projectile applies its configured damage"),
	          Player->Combatant->CurrentHealth,
	          100.0f - MovingSpread->Damage);

	Player->Combatant->RestoreCurrentHealth(100.0f);
	FReEchoBossIntent BlinkSlam;
	BlinkSlam.Type = EReEchoBossIntentType::AttackWindowStarted;
	BlinkSlam.AbilityKind = EReEchoBossAbilityKind::BlinkSlam;
	BlinkSlam.AbilityId = TEXT("M_SHEEP_BlinkSlam");
	BlinkSlam.Attack.Sequence = 9001;
	BlinkSlam.Attack.Source = Sheep;
	BlinkSlam.Target = Player;
	BlinkSlam.AttackShape = EReEchoBossAttackShape::Circle;
	BlinkSlam.Origin = Player->GetActorLocation();
	BlinkSlam.LockedTargetLocation = Player->GetActorLocation();
	BlinkSlam.TeleportDestination = Player->GetActorLocation();
	BlinkSlam.RawDamage = 12.0f;
	BlinkSlam.RadiusCm = 180.0f;
	BlinkSlam.bCanDamageTarget = true;
	BlinkSlam.bRequestTeleport = true;
	UReEchoCombatVfxComponent* BlinkSlamVfx = Sheep->FindComponentByClass<UReEchoCombatVfxComponent>();
	FVector TelegraphWorldLocation = FVector::ZeroVector;
	if (BlinkSlamVfx && FApp::CanEverRender())
	{
		FReEchoBossIntent Telegraph = BlinkSlam;
		Telegraph.Type = EReEchoBossIntentType::TelegraphStarted;
		Sheep->GetEnemyEventsComponent()->PublishBossIntent(Telegraph);
		if (UNiagaraComponent* TelegraphEffect = BlinkSlamVfx->GetBossTelegraphEffectForTests())
		{
			TelegraphWorldLocation = TelegraphEffect->GetComponentLocation();
		}
	}
	Sheep->ApplyBossIntentForTests(BlinkSlam);
	TestEqual(TEXT("Blink slam does not damage at the start of its descent"), Player->Combatant->CurrentHealth, 100.0f);
	Sheep->AdvancePendingBossBlinkSlamForTests(0.49f);
	TestEqual(TEXT("Blink slam remains non-damaging before the 0.5 second landing"),
	          Player->Combatant->CurrentHealth,
	          100.0f);
	Sheep->AdvancePendingBossBlinkSlamForTests(0.01f);
	TestEqual(TEXT("Blink slam applies damage once its descent completes"), Player->Combatant->CurrentHealth, 88.0f);
	if (UReEchoCombatVfxComponent* Vfx = BlinkSlamVfx)
	{
		TestEqual(TEXT("Blink slam landing reaches the Skill03 VFX impact handler"),
		          Vfx->GetBossSkill03ImpactIntentCountForTests(),
		          1);
		if (FApp::CanEverRender())
		{
			TestEqual(TEXT("Blink slam landing successfully spawns one Skill03 ground crack"),
			          Vfx->GetBossSkill03ImpactSpawnCountForTests(),
			          1);
			UNiagaraComponent* ImpactEffect = Vfx->GetLastBossSkill03ImpactEffectForTests();
			TestNotNull(TEXT("Blink slam retains the ground-crack Niagara for its explicit lifetime"), ImpactEffect);
			if (ImpactEffect)
			{
				TestEqual(TEXT("Blink slam ground crack uses the authored Skill03 impact system"),
				          ImpactEffect->GetAsset(),
				          LoadObject<UNiagaraSystem>(nullptr,
				                                     TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill03_BeAttacked."
				                                          "NS_Goat_Skill03_BeAttacked")));
				const FVector ImpactOrigin = ImpactEffect->GetComponentLocation();
				TestTrue(TEXT("Blink slam ground crack preserves the telegraph's locked XY center"),
				         FVector2D(ImpactOrigin).Equals(FVector2D(TelegraphWorldLocation), 1.0f));
				const UNiagaraSystem* ImpactSystem = ImpactEffect->GetAsset();
				const FBox ImpactBounds = ImpactSystem ? ImpactSystem->GetFixedBounds() : FBox(EForceInit::ForceInit);
				const double LowestWorldZ =
				    ImpactBounds.IsValid
				        ? ImpactOrigin.Z + ImpactBounds.Min.Z * FMath::Abs(ImpactEffect->GetComponentScale().Z)
				        : ImpactOrigin.Z;
				TestEqual(
				    TEXT("Blink slam ground crack's lowest authored layer aligns with the telegraph ground height"),
				    LowestWorldZ,
				    static_cast<double>(TelegraphWorldLocation.Z),
				    1.0);
			}
		}
	}
	Sheep->AdvancePendingBossBlinkSlamForTests(1.0f);
	TestEqual(TEXT("Blink slam delayed impact is consumed exactly once"), Player->Combatant->CurrentHealth, 88.0f);

	Player->Combatant->RestoreCurrentHealth(100.0f);
	Player->SetActorLocation(FVector(600.0f, 200.0f, 0.0f));
	FReEchoBossIntent PrayerBeamIntent;
	PrayerBeamIntent.Type = EReEchoBossIntentType::AttackWindowStarted;
	PrayerBeamIntent.AbilityId = TEXT("M_SHEEP_PrayerBeam");
	PrayerBeamIntent.Attack.Sequence = 9002;
	PrayerBeamIntent.Attack.Source = Sheep;
	PrayerBeamIntent.Target = Player;
	PrayerBeamIntent.AttackShape = EReEchoBossAttackShape::Beam;
	PrayerBeamIntent.LockedTargetLocation = FVector::ZeroVector;
	PrayerBeamIntent.LockedDirection = FVector::ForwardVector;
	PrayerBeamIntent.RawDamage = PrayerBeam->Damage;
	PrayerBeamIntent.ActiveSeconds = PrayerBeam->ActiveSeconds;
	PrayerBeamIntent.LengthCm = PrayerBeam->LengthCm;
	PrayerBeamIntent.WidthCm = PrayerBeam->WidthCm;
	PrayerBeamIntent.bCanDamageTarget = true;
	Sheep->ApplyBossIntentForTests(PrayerBeamIntent);
	TestEqual(TEXT("Prayer beam does not hit a target outside its locked column at startup"),
	          Player->Combatant->CurrentHealth,
	          100.0f);
	Player->SetActorLocation(FVector(600.0f, 0.0f, 0.0f));
	Sheep->AdvancePendingBossPrayerBeamForTests(1.5f);
	TestEqual(TEXT("Prayer beam hits a target entering during its three-second window"),
	          Player->Combatant->CurrentHealth,
	          100.0f - PrayerBeam->Damage);
	Sheep->AdvancePendingBossPrayerBeamForTests(1.0f);
	TestEqual(TEXT("Prayer beam damages each target at most once per cast"),
	          Player->Combatant->CurrentHealth,
	          100.0f - PrayerBeam->Damage);
	Player->SetActorLocation(FVector(600.0f, 200.0f, 0.0f));
	Sheep->AdvancePendingBossPrayerBeamForTests(0.5f);
	Player->SetActorLocation(FVector(600.0f, 0.0f, 0.0f));
	Sheep->AdvancePendingBossPrayerBeamForTests(1.0f);
	TestEqual(TEXT("Prayer beam cannot hit after its three-second window ends"),
	          Player->Combatant->CurrentHealth,
	          100.0f - PrayerBeam->Damage);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHostPhase2BornPermitContractTest,
                                 "ReEcho.Enemies.Host.Phase2BornPermitContract",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHostPhase2BornPermitContractTest::RunTest(const FString& Parameters)
{
	FReEchoEnemySenseSnapshot Sense;
	TestTrue(TEXT("Missing or failed Born playback leaves the typed Phase2 permit open by default"),
	         Sense.bPhase2TransitionPermitted);
	Sense.bPhase2TransitionPermitted = false;
	TestFalse(TEXT("Host can close only the typed Phase2-start permit while Born is active"),
	          Sense.bPhase2TransitionPermitted);
	TestTrue(TEXT("Missing or failed Born leaves the typed attack permit open by default"), Sense.bAttackPermitted);
	Sense.bAttackPermitted = false;
	TestFalse(TEXT("Host can close the typed attack permit only while successful Born is active"),
	          Sense.bAttackPermitted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHostBossPhase2GMTransitionTest,
                                 "ReEcho.Enemies.Host.BossPhase2GMTransition",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHostBossPhase2GMTransitionTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyHostWorldFixture Fixture;
	AReEchoEnemyActor* Sheep = Fixture.World->SpawnActor<AReEchoEnemyActor>();
	if (!TestNotNull(TEXT("GM Phase2 test spawns a sheep Host"), Sheep))
	{
		return false;
	}

	const FReEchoCsvLoadResult LoadResult =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	if (!TestTrue(TEXT("GM Phase2 test loads production enemy CSV"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}
	FReEchoEnemyDefinition Definition;
	FString CompileError;
	if (!TestTrue(
	        TEXT("GM Phase2 test compiles the production sheep"),
	        ReEchoEnemyDefinitionCompiler::Compile(*LoadResult.Snapshot, TEXT("M_SHEEP"), Definition, CompileError)))
	{
		AddError(CompileError);
		return false;
	}
	Sheep->SetEnemyId(TEXT("M_SHEEP"));
	if (!TestTrue(TEXT("GM Phase2 test configures the sheep Host"), Sheep->ConfigureFromDefinition(Definition, 153)))
	{
		return false;
	}

	TestTrue(TEXT("GMBossPhase 2 starts the normal timed transition"), Sheep->DebugForceBossPhaseForGM(2));
	const FReEchoEnemyLogicSnapshot Started = Sheep->GetEnemyLogicComponent()->GetSnapshot();
	TestEqual(TEXT("GM keeps Phase1 authoritative during the transform"), Started.CurrentPhaseIndex, 1);
	TestEqual(
	    TEXT("GM enters the transforming behavior phase"), Started.Phase, EReEchoEnemyBehaviorPhase::Transforming);
	TestEqual(TEXT("GM uses the authored transform duration"),
	          Started.PhaseTransitionRemainingSeconds,
	          Definition.Phase2.TransformSeconds);

	FReEchoEnemySenseSnapshot Sense;
	Sheep->AdvanceBehaviorForTests(Sense, Definition.Phase2.TransformSeconds);
	const FReEchoEnemyLogicSnapshot Completed = Sheep->GetEnemyLogicComponent()->GetSnapshot();
	TestEqual(TEXT("GM completes into Phase2 after the transform"), Completed.CurrentPhaseIndex, 2);
	TestEqual(TEXT("GM returns to idle after the transform"), Completed.Phase, EReEchoEnemyBehaviorPhase::Idle);
	TestFalse(TEXT("GM rejects replaying Phase2 after it is already active"), Sheep->DebugForceBossPhaseForGM(2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHostGMKillAllBossPhaseBoundaryTest,
                                 "ReEcho.Enemies.Host.GMKillAllBossPhaseBoundary",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHostGMKillAllBossPhaseBoundaryTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyHostWorldFixture Fixture;
	const FReEchoCsvLoadResult LoadResult =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	UReEchoBossPhase3Config* Phase3Config = LoadObject<UReEchoBossPhase3Config>(
	    nullptr, TEXT("/Game/ReEcho/DataAsset/Enemy/DA_SheepBossPhase3.DA_SheepBossPhase3"));
	FReEchoEnemyDefinition Definition;
	FString CompileError;
	if (!TestTrue(TEXT("GMKillAll phase-boundary test loads production enemy CSV"), LoadResult.bSuccess) ||
	    !TestNotNull(TEXT("GMKillAll phase-boundary test loads the Phase3 overlay"), Phase3Config) ||
	    !TestTrue(
	        TEXT("GMKillAll phase-boundary test compiles the production sheep"),
	        ReEchoEnemyDefinitionCompiler::Compile(*LoadResult.Snapshot, TEXT("M_SHEEP"), Definition, CompileError)) ||
	    !TestTrue(TEXT("GMKillAll phase-boundary test applies the Phase3 overlay"),
	              Phase3Config && Phase3Config->ApplyTo(TEXT("M_SHEEP"), Definition)))
	{
		AddError(LoadResult.bSuccess ? CompileError : LoadResult.FormatIssues());
		return false;
	}

	AReEchoEnemyActor* Sheep = Fixture.World->SpawnActor<AReEchoEnemyActor>();
	Sheep->SetEnemyId(TEXT("M_SHEEP"));
	if (!TestTrue(TEXT("GMKillAll phase-boundary test configures the sheep"),
	              Sheep->ConfigureFromDefinition(Definition, 153)) ||
	    !TestTrue(TEXT("GMKillAll phase-boundary test starts Phase2"), Sheep->DebugForceBossPhaseForGM(2)))
	{
		return false;
	}
	FReEchoEnemySenseSnapshot Sense;
	Sense.bAttackPermitted = true;
	Sheep->AdvanceBehaviorForTests(Sense, Definition.Phase2.TransformSeconds);
	TestEqual(TEXT("GMKillAll test reaches Phase2 before the lethal hit"),
	          Sheep->GetEnemyLogicComponent()->GetSnapshot().CurrentPhaseIndex,
	          2);

	const FVector DamageSource = Sheep->GetActorLocation() - Sheep->GetFacingDirection() * 100.0f;
	Sheep->ReceiveGrayboxDamage(TNumericLimits<float>::Max(), DamageSource);
	TestTrue(TEXT("One GMKillAll lethal hit leaves the transitioned Phase3 boss alive"), Sheep->IsAlive());
	TestEqual(TEXT("One GMKillAll lethal hit enters Phase3"),
	          Sheep->GetEnemyLogicComponent()->GetSnapshot().CurrentPhaseIndex,
	          3);

	Sheep->ReceiveGrayboxDamage(TNumericLimits<float>::Max(), DamageSource);
	TestFalse(TEXT("A later GMKillAll invocation can kill the already active Phase3 boss"), Sheep->IsAlive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBossTransformationLifecycleTest,
                                 "ReEcho.Enemies.Host.BossTransformationLifecycle",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBossTransformationLifecycleTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyHostWorldFixture Fixture;
	AReEchoEnemyActor* StunnedEnemy = Fixture.Spawn(EReEchoEnemyKind::Grunt, 152);
	UReEcho2DAnimationComponent* VisibleRenderer = StunnedEnemy->FindComponentByClass<UReEcho2DAnimationComponent>();
	FReEcho2DAnimationClip SacrificeClip;
	SacrificeClip.Flipbook = LoadObject<UPaperFlipbook>(
	    nullptr, TEXT("/Game/ReEcho/Art/Animation2D/Enemies/Slime/Flipbooks/Default.Default"));
	SacrificeClip.bLooping = true;
	if (!VisibleRenderer || !SacrificeClip.Flipbook || !VisibleRenderer->PlayClip(SacrificeClip, true))
	{
		AddError(TEXT("Sacrifice fixture requires a real rendered enemy"));
		return false;
	}
	VisibleRenderer->SetVisibility(true);
	VisibleRenderer->SetHiddenInGame(false);
	StunnedEnemy->ApplyCardStun(3.0f);
	const float StunDeadline = StunnedEnemy->GetCardStunDeadlineForTests();
	const float SuspendedAt = Fixture.World->GetTimeSeconds();
	StunnedEnemy->SetActorTickEnabled(false);
	Fixture.World->Tick(LEVELTICK_All, 0.25f);
	StunnedEnemy->CompensateBossTransformationPause(SuspendedAt);
	TestTrue(TEXT("Card stun preserves its remaining duration across presentation pause"),
	         FMath::IsNearlyEqual(StunnedEnemy->GetCardStunDeadlineForTests() - Fixture.World->GetTimeSeconds(),
	                              StunDeadline - SuspendedAt));
	const FReEchoCsvLoadResult Load =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	FReEchoEnemyDefinition Definition;
	FString Error;
	if (!Load.bSuccess || !ReEchoEnemyDefinitionCompiler::Compile(*Load.Snapshot, TEXT("M_SHEEP"), Definition, Error))
	{
		AddError(TEXT("Could not load sheep definition"));
		return false;
	}
	NewObject<UReEchoBossPhase3Config>()->ApplyTo(TEXT("M_SHEEP"), Definition);
	AReEchoEnemyActor* Boss = Fixture.World->SpawnActor<AReEchoEnemyActor>();
	Boss->SetEnemyId(TEXT("M_SHEEP"));
	Boss->SetPresentationCatalog(LoadObject<UReEcho2DPresentationCatalog>(
	    nullptr,
	    TEXT("/Game/ReEcho/DataAsset/Enemy/Catalogs/DA_EnemyPresentationCatalog.DA_EnemyPresentationCatalog")));
	if (!Boss->ConfigureFromDefinition(Definition, 153))
	{
		return false;
	}
	AReEchoPlayerPawn* Player = Fixture.World->SpawnActor<AReEchoPlayerPawn>();
	FReEchoStatBlock Stats;
	Stats.HpMax = 100.0f;
	Player->Combatant->InitializeFromStats(Stats, true);
	AReEchoEncounterDirector* Director = Fixture.World->SpawnActor<AReEchoEncounterDirector>();
	AReEchoGameMode* Mode = Fixture.World->SpawnActor<AReEchoGameMode>();
	AReEchoArenaCameraActor* Camera = Fixture.World->SpawnActor<AReEchoArenaCameraActor>();
	Camera->Configure(Player, nullptr);
	const float StandardWidth = Camera->ArenaCamera->OrthoWidth;
	Mode->ConfigureBossTransformationForTests(Player, Director, Camera);
	APlayerState* Pauser = Fixture.World->SpawnActor<APlayerState>();
	const bool PreviousBossTick = Boss->IsActorTickEnabled();
	const bool PreviousDirectorTick = Director->IsActorTickEnabled();
	UReEchoEnemyPresentationComponent* SacrificeVisual =
	    StunnedEnemy->FindComponentByClass<UReEchoEnemyPresentationComponent>();
	// This fixture installs a Flipbook directly, bypassing Host Advance. Establish the grounded baseline first.
	TestTrue(TEXT("Fixture is grounded before capturing the sacrifice baseline"),
	         SacrificeVisual->PrepareSacrificeGroundPose());
	const FVector OriginalVisualCenter = SacrificeVisual->GetSacrificeVisualCenter();
	StunnedEnemy->SetActorHiddenInGame(true);
	TestTrue(TEXT("Empty sacrifice without run data still starts transformation"),
	         Mode->BeginBossTransformation(Boss, 2));
	Camera->Tick(0.8f);
	Mode->AdvanceBossTransformationForTests(1.6f);
	TestEqual(TEXT("Missing production spawn context fails open with no participants"),
	          Mode->GetBossSacrificeCountForTests(),
	          0);
	TestTrue(TEXT("Missing supplementation context does not abort cinematic"),
	         Mode->IsBossTransformationActiveForTests());
	Mode->CancelBossTransformationForTests();
	StunnedEnemy->SetActorHiddenInGame(false);
	TestTrue(TEXT("Sacrifice cancellation starts"), Mode->BeginBossTransformation(Boss, 2));
	Camera->Tick(1.3f);
	Mode->AdvanceBossTransformationForTests(1.3f);
	TestFalse(TEXT("Charge does not start with takeoff at 1.3s"), Mode->HasBossSacrificeChargeStartedForTests());
	Mode->AdvanceBossTransformationForTests(0.19f);
	TestFalse(TEXT("Charge remains off before 1.5s"), Mode->HasBossSacrificeChargeStartedForTests());
	Mode->AdvanceBossTransformationForTests(0.011f);
	TestTrue(TEXT("Charge starts on crossing 1.5s"), Mode->HasBossSacrificeChargeStartedForTests());
	Mode->AdvanceBossTransformationForTests(0.599f);
	TestEqual(TEXT("Single visible ordinary enemy selected"), Mode->GetBossSacrificeCountForTests(), 1);
	TestTrue(TEXT("Animation rises independently of gameplay actor"),
	         SacrificeVisual->GetSacrificeVisualCenter().Z > OriginalVisualCenter.Z);
	Mode->CancelBossTransformationForTests();
	TestTrue(TEXT("Cancellation restores original visual center"),
	         SacrificeVisual->GetSacrificeVisualCenter().Equals(OriginalVisualCenter));
	TestTrue(TEXT("Cancellation restores opacity"), FMath::IsNearlyEqual(VisibleRenderer->GetSpriteColor().A, 1.0f));
	for (int32 Phase = 2; Phase <= 3; ++Phase)
	{
		const float Burst = Phase == 2 ? 4.0f : 1.8f;
		const float Duration = Phase == 2 ? 5.2f : 2.9f;
		const FVector OriginalEnemyLocation = StunnedEnemy->GetActorLocation();
		const float OriginalEnemyHealth = StunnedEnemy->GetCombatantComponent()->CurrentHealth;
		const float FightTime = Boss->GetEnemyLogicComponent()->GetSnapshot().BossEncounterElapsedSeconds;
		TestEqual(TEXT("Read-only fatal qualification keeps the current phase"),
		          Boss->GetEnemyLogicComponent()->ResolveFatalBossTransitionPhase(false),
		          Phase);
		// Reproduce the fatal hook's health/logic boundary: health reaches zero before Logic marks death.
		Boss->GetCombatantComponent()->RestoreCurrentHealth(0.0f);
		Fixture.World->GetWorldSettings()->SetPauserPlayerState(Pauser);
		TestTrue(TEXT("Paused cinematic request accepted"), Mode->BeginBossTransformation(Boss, Phase));
		TestFalse(TEXT("Paused request does not start presentation"), Mode->IsBossTransformationActiveForTests());
		Mode->AdvanceBossTransformationForTests(1.0f);
		TestEqual(TEXT("Paused request does not switch phase"),
		          Boss->GetEnemyLogicComponent()->GetSnapshot().CurrentPhaseIndex,
		          Phase - 1);
		Boss->GetCombatantComponent()->RestoreCurrentHealth(1.0f);
		Fixture.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
		Mode->AdvanceBossTransformationForTests(0.0f);
		TestTrue(TEXT("Cinematic starts after resume"), Mode->IsBossTransformationActiveForTests());
		Camera->Tick(0.1f);
		const float MidZoomWidth = Camera->ArenaCamera->OrthoWidth;
		TestTrue(TEXT("Real camera zooms toward boss"), MidZoomWidth < StandardWidth);
		Fixture.World->GetWorldSettings()->SetPauserPlayerState(Pauser);
		Mode->AdvanceBossTransformationForTests(2.0f);
		Camera->Tick(2.0f);
		TestEqual(TEXT("Camera freezes with presentation while paused"), Camera->ArenaCamera->OrthoWidth, MidZoomWidth);
		TestEqual(TEXT("Paused presentation does not commit phase"),
		          Boss->GetEnemyLogicComponent()->GetSnapshot().CurrentPhaseIndex,
		          Phase - 1);
		Fixture.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
		Camera->Tick(0.1f);
		TestFalse(TEXT("Camera no longer finishes the push in 0.2 seconds"), Camera->IsStage01To02CameraMoveComplete());
		Camera->Tick(0.6f);
		TestFalse(TEXT("Camera is still pushing at 0.8 seconds"), Camera->IsStage01To02CameraMoveComplete());
		Camera->Tick(0.5f);
		TestTrue(TEXT("Camera reaches focus after 1.3 seconds"), Camera->IsStage01To02CameraMoveComplete());
		TestFalse(TEXT("Director tick pauses without pausing the world"), Director->IsActorTickEnabled());
		TestFalse(TEXT("Boss gameplay tick pauses"), Boss->IsActorTickEnabled());
		TestTrue(TEXT("Player rejects damage during presentation"), Player->Combatant->IsPresentationSuspended());
		float BeforeBurstTime = 0.0f;
		FVector HoverStart = OriginalVisualCenter;
		if (Phase == 2)
		{
			Mode->AdvanceBossTransformationForTests(2.5f);
			HoverStart = SacrificeVisual->GetSacrificeVisualCenter();
			TestTrue(TEXT("Rise reaches 1.8 body heights"),
			         FMath::IsNearlyEqual(HoverStart.Z - OriginalVisualCenter.Z,
			                              VisibleRenderer->Bounds.BoxExtent.Z * 2.0f * 1.8f,
			                              0.1f));
			Mode->AdvanceBossTransformationForTests(0.75f);
			const FVector HoverShake = SacrificeVisual->GetSacrificeVisualCenter();
			TestTrue(TEXT("Hover holds peak height"), FMath::IsNearlyEqual(HoverStart.Z, HoverShake.Z, 0.1f));
			TestTrue(TEXT("Hover shakes horizontally"), FVector::DistSquared2D(HoverStart, HoverShake) > 0.01f);
			BeforeBurstTime = 3.25f;
		}
		Mode->AdvanceBossTransformationForTests(Burst - 0.01f - BeforeBurstTime);
		TestEqual(TEXT("Old phase remains before burst"),
		          Boss->GetEnemyLogicComponent()->GetSnapshot().CurrentPhaseIndex,
		          Phase - 1);
		Mode->AdvanceBossTransformationForTests(0.02f);
		TestEqual(TEXT("Burst commits the new phase"),
		          Boss->GetEnemyLogicComponent()->GetSnapshot().CurrentPhaseIndex,
		          Phase);
		TestTrue(TEXT("Gameplay stays blocked for the post-burst display"), Mode->IsBossTransformationActiveForTests());
		if (Phase == 3)
		{
			FVector EnlargedCenter;
			TestTrue(TEXT("Phase3 focus resolves the actual enlarged Flipbook"),
			         AReEchoArenaCameraActor::TryResolveStage01To02VisualCenter(Boss, EnlargedCenter));
			Camera->Tick(0.4f);
			TestFalse(TEXT("Phase3 reframing is smooth, not an instant snap"),
			          Camera->IsStage01To02CameraMoveComplete());
			Camera->Tick(0.4f);
			const FVector ToCenter = EnlargedCenter - Camera->ArenaCamera->GetComponentLocation();
			TestTrue(TEXT("Enlarged Phase3 center is vertically centered after reframing"),
			         FMath::Abs(FVector::DotProduct(ToCenter, Camera->ArenaCamera->GetUpVector())) < 0.1f);
			TestTrue(TEXT("Reframing preserves horizontal boss centering"),
			         FMath::Abs(FVector::DotProduct(ToCenter, Camera->ArenaCamera->GetRightVector())) < 0.1f);
		}
		UReEcho2DAnimationComponent* SacrificeRenderer =
		    StunnedEnemy->FindComponentByClass<UReEcho2DAnimationComponent>();
		float AdditionalSacrificeTime = 0.0f;
		if (Phase == 2)
		{
			TestTrue(TEXT("Nearby ordinary enemies are selected for visual sacrifice"),
			         Mode->GetBossSacrificeCountForTests() > 0);
			TestTrue(TEXT("Sacrifice stays visible at black-goat burst"),
			         SacrificeRenderer && FMath::IsNearlyEqual(SacrificeRenderer->GetSpriteColor().A, 1.0f));
			Mode->AdvanceBossTransformationForTests(0.19f);
			const FVector Falling = SacrificeVisual->GetSacrificeVisualCenter();
			TestTrue(TEXT("Fall descends continuously instead of teleporting"),
			         Falling.Z < HoverStart.Z && Falling.Z > OriginalVisualCenter.Z);
			TestTrue(TEXT("No horizontal drift during fall"),
			         FVector::DistSquared2D(Falling, OriginalVisualCenter) < 0.01f);
			AdditionalSacrificeTime = 0.19f;
		}
		else
		{
			TestEqual(TEXT("Phase3 does not sacrifice enemies"), Mode->GetBossSacrificeCountForTests(), 0);
		}
		TestEqual(
		    TEXT("Sacrifice never moves gameplay position"), StunnedEnemy->GetActorLocation(), OriginalEnemyLocation);
		TestEqual(TEXT("Sacrifice never damages or heals enemy"),
		          StunnedEnemy->GetCombatantComponent()->CurrentHealth,
		          OriginalEnemyHealth);
		TestTrue(TEXT("Burst does not trigger the completion shake early"),
		         Camera->ArenaCamera->GetRelativeLocation().IsNearlyZero());
		Mode->AdvanceBossTransformationForTests(Duration - 0.8f - Burst - 0.01f + 0.001f - AdditionalSacrificeTime);
		if (Phase == 2)
		{
			TestTrue(TEXT("Fall reaches exact original position before camera return"),
			         SacrificeVisual->GetSacrificeVisualCenter().Equals(OriginalVisualCenter, 0.1f));
		}
		Camera->Tick(0.4f);
		if (Phase == 2)
		{
			TestFalse(TEXT("Camera pull also lasts longer than 0.4 seconds"),
			          Camera->IsStage01To02CameraMoveComplete());
		}
		else
		{
			TestTrue(TEXT("Phase3 keeps boss zoom instead of pulling back before opening"),
			         Camera->ArenaCamera->OrthoWidth < StandardWidth);
		}
		Mode->AdvanceBossTransformationForTests(0.4f);
		Camera->Tick(0.4f);
		TestTrue(TEXT("Camera pull completes at 0.8 seconds"), Camera->IsStage01To02CameraMoveComplete());
		Mode->AdvanceBossTransformationForTests(0.4f);
		TestFalse(TEXT("Cinematic ends"), Mode->IsBossTransformationActiveForTests());
		TestEqual(TEXT("Only completed phase3 cinematic queues its production opening"),
		          Boss->GetEnemyLogicComponent()->GetSnapshot().bBossOpeningQueued,
		          Phase == 3);
		TestEqual(TEXT("Sacrifice participants cleared on completion"), Mode->GetBossSacrificeCountForTests(), 0);
		TestTrue(TEXT("Enemy opacity restored after rebirth"),
		         SacrificeRenderer && FMath::IsNearlyEqual(SacrificeRenderer->GetSpriteColor().A, 1.0f));
		if (Phase == 2)
		{
			TestEqual(TEXT("Phase2 camera width restored"), Camera->ArenaCamera->OrthoWidth, StandardWidth);
		}
		else
		{
			TestTrue(TEXT("Phase3 camera remains focused while opening is queued"),
			         Camera->ArenaCamera->OrthoWidth < StandardWidth);
		}
		TestFalse(TEXT("Completion triggers an immediate camera shake"),
		          Camera->ArenaCamera->GetRelativeLocation().IsNearlyZero());
		Camera->Tick(0.25f);
		TestTrue(TEXT("Completion shake returns to neutral after 0.25 seconds"),
		         Camera->ArenaCamera->GetRelativeLocation().IsNearlyZero());
		TestEqual(TEXT("Boss tick restored exactly"), Boss->IsActorTickEnabled(), PreviousBossTick);
		TestEqual(TEXT("Director tick restored exactly"), Director->IsActorTickEnabled(), PreviousDirectorTick);
		TestEqual(TEXT("Player protection lasts through phase3 opening"),
		          Player->Combatant->IsPresentationSuspended(),
		          Phase == 3);
		TestEqual(TEXT("Presentation does not consume the fast-kill clock"),
		          Boss->GetEnemyLogicComponent()->GetSnapshot().BossEncounterElapsedSeconds,
		          FightTime);
		if (Phase == 3)
		{
			const float HeldWidth = Camera->ArenaCamera->OrthoWidth;
			TestFalse(TEXT("Player gameplay remains frozen during opening"), Player->IsActorTickEnabled());
			const FVector HeldPosition = Camera->GetActorLocation();
			const FVector OriginalPlayerPosition = Player->GetActorLocation();
			Player->SetActorLocation(OriginalPlayerPosition + FVector(1000.0f, 0.0f, 0.0f));
			Mode->AdvanceBossTransformationForTests(0.1f);
			Camera->Tick(0.1f);
			TestEqual(
			    TEXT("Queued opening camera does not follow moving player"), Camera->GetActorLocation(), HeldPosition);
			Fixture.World->GetWorldSettings()->SetPauserPlayerState(Pauser);
			Mode->AdvanceBossTransformationForTests(10.0f);
			Camera->Tick(10.0f);
			TestEqual(TEXT("Paused opening retains boss zoom"), Camera->ArenaCamera->OrthoWidth, HeldWidth);
			Fixture.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
			FReEchoEnemySenseSnapshot Sense;
			Sense.Target = Player;
			Sense.bTargetExists = true;
			Sense.bTargetAlive = true;
			Sense.bAttackPermitted = true;
			Sense.SelfLocation = Boss->GetActorLocation();
			Sense.TargetLocation = Player->GetActorLocation();
			int32 StrikeCount = 0;
			bool bOpeningEnded = false;
			for (int32 Frame = 0; Frame < 1200 && !bOpeningEnded; ++Frame)
			{
				const FReEchoEnemyActionIntent Action = Boss->GetEnemyLogicComponent()->Advance(Sense, 1.0f / 60.0f);
				for (const FReEchoBossIntent& Event : Action.BossIntents)
				{
					StrikeCount +=
					    Event.bPhaseOpening && Event.Type == EReEchoBossIntentType::AttackWindowStarted ? 1 : 0;
					bOpeningEnded |= Event.bPhaseOpening && Event.Type == EReEchoBossIntentType::AbilityEnded;
				}
				Mode->AdvanceBossTransformationForTests(1.0f / 60.0f);
				if (!bOpeningEnded)
				{
					Camera->Tick(1.0f / 60.0f);
					TestEqual(TEXT("All opening beats retain boss zoom"), Camera->ArenaCamera->OrthoWidth, HeldWidth);
				}
			}
			TestTrue(TEXT("Opening camera observes actual combo completion"), bOpeningEnded);
			TestEqual(TEXT("Camera holds across all three strikes"), StrikeCount, 3);
			Camera->Tick(0.4f);
			TestFalse(TEXT("Opening return takes more than 0.4 seconds"), Camera->IsStage01To02CameraMoveComplete());
			TestTrue(TEXT("Player protected during camera return"), Player->Combatant->IsPresentationSuspended());
			TestTrue(TEXT("Opening return smoothly widens"), Camera->ArenaCamera->OrthoWidth > HeldWidth);
			Camera->Tick(0.4f);
			Mode->AdvanceBossTransformationForTests(0.0f);
			TestEqual(TEXT("Opening return restores standard width"), Camera->ArenaCamera->OrthoWidth, StandardWidth);
			TestFalse(TEXT("Opening camera ownership released after return"),
			          Camera->IsStage01To02CameraMoveComplete());
			Player->SetActorLocation(OriginalPlayerPosition);
			TestFalse(TEXT("Player protection released after return"), Player->Combatant->IsPresentationSuspended());
		}
	}
	TestTrue(TEXT("Cancellation test starts"), Mode->BeginBossTransformation(Boss, 3));
	Mode->CancelBossTransformationForTests();
	TestFalse(TEXT("Cancellation releases damage gate"), Player->Combatant->IsPresentationSuspended());
	TestTrue(TEXT("Cancellation does not play completion shake"),
	         Camera->ArenaCamera->GetRelativeLocation().IsNearlyZero());
	TestEqual(TEXT("Cancellation restores director"), Director->IsActorTickEnabled(), PreviousDirectorTick);
	Fixture.World->GetWorldSettings()->SetPauserPlayerState(Pauser);
	TestTrue(TEXT("Queue cancellation request accepted"), Mode->BeginBossTransformation(Boss, 3));
	Mode->CancelBossTransformationForTests();
	Fixture.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
	Mode->AdvanceBossTransformationForTests(0.0f);
	TestFalse(TEXT("Cancelled queue cannot restart"), Mode->IsBossTransformationActiveForTests());
	Camera->BeginStage01To02CameraSequence();
	Camera->FocusStage01To02Target(Boss, 0.5f, 0.2f);
	Fixture.World->GetWorldSettings()->SetPauserPlayerState(Pauser);
	Camera->Tick(0.2f);
	TestTrue(TEXT("Legacy echo sequence still advances while paused"), Camera->IsStage01To02CameraMoveComplete());
	Fixture.World->GetWorldSettings()->SetPauserPlayerState(nullptr);
	Camera->EndStage01To02CameraSequence();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBossOpeningComboTest,
                                 "ReEcho.Enemies.Host.Phase3OpeningCombo",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBossOpeningComboTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyHostWorldFixture Fixture;
	const FReEchoCsvLoadResult Load =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	FReEchoEnemyDefinition Definition;
	FString Error;
	if (!Load.bSuccess || !ReEchoEnemyDefinitionCompiler::Compile(*Load.Snapshot, TEXT("M_SHEEP"), Definition, Error))
	{
		AddError(TEXT("Production sheep definition unavailable"));
		return false;
	}
	TestTrue(TEXT("DA overlays definition"),
	         GetDefault<UReEchoBossPhase3Config>()->ApplyTo(TEXT("M_SHEEP"), Definition));
	AReEchoEnemyActor* Boss = Fixture.World->SpawnActor<AReEchoEnemyActor>();
	Boss->SetPresentationCatalog(LoadObject<UReEcho2DPresentationCatalog>(
	    nullptr,
	    TEXT("/Game/ReEcho/DataAsset/Enemy/Catalogs/DA_EnemyPresentationCatalog.DA_EnemyPresentationCatalog")));
	Boss->ConfigureFromDefinition(Definition, 153);
	Boss->CompleteBornGameplayGateForTests();
	UReEchoEnemyLogicComponent* Logic = Boss->FindComponentByClass<UReEchoEnemyLogicComponent>();
	FReEchoEnemyActionIntent Transition;
	TestFalse(TEXT("Phase1 cannot queue phase3 opening"), Logic->QueueBossPhaseOpening());
	TestTrue(TEXT("Enter phase3 with production presentation"), Boss->DebugForceBossPhaseForGM(3));
	TestTrue(TEXT("Production opening queues"), Logic->QueueBossPhaseOpening());
	TestFalse(TEXT("Repeated callback cannot queue twice"), Logic->QueueBossPhaseOpening());
	Logic->RestoreSnapshot(Logic->GetSnapshot());
	TestTrue(TEXT("Queued opening survives snapshot"), Logic->GetSnapshot().bBossOpeningQueued);
	TestFalse(TEXT("Restore cannot reset one-shot consumption"), Logic->QueueBossPhaseOpening());
	AReEchoPlayerPawn* Target = Fixture.World->SpawnActor<AReEchoPlayerPawn>();
	Target->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	APlayerController* ShakeController = Fixture.World->SpawnActor<APlayerController>();
	ShakeController->Possess(Target);
	AReEchoArenaCameraActor* ShakeCamera = Fixture.World->SpawnActor<AReEchoArenaCameraActor>();
	ShakeCamera->Configure(Target, nullptr);
	ShakeController->SetViewTarget(ShakeCamera);
	UReEchoEnemyPresentationComponent* Visual = Boss->FindComponentByClass<UReEchoEnemyPresentationComponent>();
	UReEcho2DAnimationComponent* Animation = Boss->FindComponentByClass<UReEcho2DAnimationComponent>();
	UReEchoCombatVfxComponent* Vfx = Boss->FindComponentByClass<UReEchoCombatVfxComponent>();
	UPaperFlipbook* Attack = LoadObject<UPaperFlipbook>(
	    nullptr, TEXT("/Game/ReEcho/Art/Animation2D/Enemies/Goat/Phase3/GroundSlam/GroundSlam.GroundSlam"));
	FReEchoEnemySenseSnapshot Sense;
	Sense.Target = Target;
	Sense.bTargetExists = true;
	Sense.bTargetAlive = true;
	Sense.SelfLocation = Boss->GetActorLocation();
	Sense.TargetLocation = Target->GetActorLocation();
	Sense.TeleportDestination = Sense.TargetLocation;
	Sense.bHasTeleportDestination = false;
	Sense.bAttackPermitted = true;
	int32 OpeningAttacks = 0;
	bool bEnded = false;
	for (int32 Frame = 0; Frame < 1200 && !bEnded; ++Frame)
	{
		ShakeCamera->Tick(1.0f / 60.0f);
		const FReEchoEnemyActionIntent Intent = Boss->AdvanceBehaviorForTests(Sense, 1.0f / 60.0f);
		Visual->AdvanceBossPhase3WindupPresentationForTests(1.0f / 60.0f);
		TestTrue(TEXT("Opening never moves gameplay boss"), Boss->GetActorLocation().Equals(Sense.SelfLocation));
		TestEqual(TEXT("Opening never starts descent presentation"), Visual->GetBossBlinkSlamRemainingForTests(), 0.0f);
		TestTrue(TEXT("Opening never hides the boss"), Animation->IsVisible());
		for (const FReEchoBossIntent& Event : Intent.BossIntents)
		{
			if (Event.bPhaseOpening && Event.Type == EReEchoBossIntentType::TelegraphStarted)
			{
				TestEqual(
				    TEXT("Each opening beat uses Phase3 Attack, not Walk or phase1"), Animation->GetFlipbook(), Attack);
			}
			if (Event.bPhaseOpening && Event.Type == EReEchoBossIntentType::AttackWindowStarted)
			{
				++OpeningAttacks;
				TestFalse(TEXT("Each actual ground impact shakes the camera"),
				          ShakeCamera->ArenaCamera->GetRelativeLocation().IsNearlyZero());
				TestEqual(TEXT("Every opening strike reports total three"), Event.ComboStrikeCount, 3);
				TestEqual(
				    TEXT("Opening uses independent triple"), Event.AbilityId, FName(TEXT("M_SHEEP_GroundTriple")));
				TestTrue(TEXT("Opening exposes stationary skill independently from cinematic state"),
				         Event.bGroundedSlam);
				TestFalse(TEXT("Opening never requests a teleport"), Event.bRequestTeleport);
				TestTrue(TEXT("Opening ground center is boss, not player"),
				         Event.LockedTargetLocation.Equals(Sense.SelfLocation));
			}
			bEnded |= Event.bPhaseOpening && Event.Type == EReEchoBossIntentType::AbilityEnded;
		}
	}
	TestTrue(TEXT("Opening completes"), bEnded);
	TestEqual(TEXT("Exactly three opening attacks"), OpeningAttacks, 3);
	TestEqual(
	    TEXT("Exactly three immediate ground impacts reach VFX"), Vfx->GetBossSkill03ImpactIntentCountForTests(), 3);
	TestFalse(TEXT("Opening cannot be repeated after completion"), Logic->QueueBossPhaseOpening());
	TestTrue(TEXT("GroundTriple is independently selectable in combat"),
	         Logic->DebugQueueBossAbility(TEXT("M_SHEEP_GroundTriple")));
	int32 CombatTripleHits = 0;
	bool bCombatTripleEnded = false;
	for (int32 Frame = 0; Frame < 1200 && !bCombatTripleEnded; ++Frame)
	{
		const FReEchoEnemyActionIntent Action = Logic->Advance(Sense, 1.0f / 60.0f);
		for (const FReEchoBossIntent& Event : Action.BossIntents)
		{
			if (Event.AbilityId != TEXT("M_SHEEP_GroundTriple"))
			{
				continue;
			}
			TestFalse(TEXT("Combat triple does not reactivate cinematic immunity"), Event.bPhaseOpening);
			TestTrue(TEXT("Combat triple retains grounded choreography"), Event.bGroundedSlam);
			if (Event.Type == EReEchoBossIntentType::AttackWindowStarted)
			{
				++CombatTripleHits;
				TestTrue(TEXT("Combat triple carries normal damage"), Event.RawDamage > 0.0f);
				TestFalse(TEXT("Combat triple never teleports"), Event.bRequestTeleport);
			}
			bCombatTripleEnded |= Event.Type == EReEchoBossIntentType::AbilityEnded;
		}
	}
	TestTrue(TEXT("Independent triple completes"), bCombatTripleEnded);
	TestEqual(TEXT("Independent triple has three combat hits"), CombatTripleHits, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBossOpeningRepulseTest,
                                 "ReEcho.Enemies.Host.Phase3OpeningRepulse",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBossOpeningRepulseTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyHostWorldFixture Fixture;
	AReEchoEnemyActor* Enemy = Fixture.Spawn(EReEchoEnemyKind::Grunt, 1);
	Enemy->CompleteBornGameplayGateForTests();
	Enemy->SetActorEnableCollision(false);
	Enemy->SetActorLocation(FVector(100.0f, 0.0f, 80.0f));
	const float Health = Enemy->GetCombatantComponent()->CurrentHealth;
	TestTrue(TEXT("Ordinary enemy accepts non-damaging repulse"),
	         Enemy->BeginBossOpeningRepulse(FVector::ZeroVector, 200.0f, 0.3f));
	Enemy->AdvanceBossOpeningRepulse(0.15f);
	TestTrue(TEXT("Ease-out moves 75 percent by halfway, preserving ground height"),
	         Enemy->GetActorLocation().Equals(FVector(250.0f, 0.0f, 80.0f), 0.01f));
	const FReEchoEnemyRuntimeState Saved = Enemy->CaptureRuntimeState();
	Enemy->RestoreRuntimeState(Saved);
	Enemy->AdvanceBossOpeningRepulse(0.15f);
	TestTrue(TEXT("Resume finishes only remaining distance"),
	         Enemy->GetActorLocation().Equals(FVector(300.0f, 0.0f, 80.0f), 0.01f));
	Enemy->AdvanceBossOpeningRepulse(1.0f);
	TestTrue(TEXT("Finished repulse does not drift"),
	         Enemy->GetActorLocation().Equals(FVector(300.0f, 0.0f, 80.0f), 0.01f));
	TestEqual(TEXT("Repulse never damages target"), Enemy->GetCombatantComponent()->CurrentHealth, Health);
	const FReEchoCsvLoadResult Load =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	FReEchoEnemyDefinition BossDefinition;
	FString Error;
	if (!Load.bSuccess ||
	    !ReEchoEnemyDefinitionCompiler::Compile(*Load.Snapshot, TEXT("M_SHEEP"), BossDefinition, Error))
	{
		AddError(TEXT("Production boss fixture unavailable"));
		return false;
	}
	GetDefault<UReEchoBossPhase3Config>()->ApplyTo(TEXT("M_SHEEP"), BossDefinition);
	AReEchoEnemyActor* Boss = Fixture.World->SpawnActor<AReEchoEnemyActor>();
	TestTrue(TEXT("Production boss configures"), Boss->ConfigureFromDefinition(BossDefinition, 2));
	Boss->CompleteBornGameplayGateForTests();
	TestFalse(TEXT("Boss excluded from repulse"), Boss->BeginBossOpeningRepulse(FVector::ZeroVector, 200.0f, 0.3f));
	Boss->SetActorEnableCollision(false);
	Boss->SetActorLocation(FVector(-1000.0f, 0.0f, 80.0f));
	UReEchoEnemyRosterComponent* Roster = NewObject<UReEchoEnemyRosterComponent>(Boss);
	Boss->SetEnemyRoster(Roster);
	Enemy->SetEnemyRoster(Roster);
	AReEchoPlayerPawn* Target = Fixture.World->SpawnActor<AReEchoPlayerPawn>();
	Target->SetActorEnableCollision(false);
	Target->SetActorLocation(FVector(0.0f, 0.0f, 80.0f));
	FReEchoStatBlock PlayerStats;
	PlayerStats.HpMax = 100.0f;
	Target->Combatant->InitializeFromStats(PlayerStats, true);
	Target->SetActorTickEnabled(false);
	FReEchoBossIntent Slam;
	Slam.Type = EReEchoBossIntentType::AttackWindowStarted;
	Slam.AbilityId = TEXT("M_SHEEP_BlinkSlam");
	Slam.AbilityKind = EReEchoBossAbilityKind::BlinkSlam;
	Slam.AttackShape = EReEchoBossAttackShape::Circle;
	Slam.Target = Target;
	Slam.Attack.Source = Boss;
	Slam.Attack.Sequence = 1;
	Slam.bPhaseOpening = true;
	Slam.RawDamage = 20.0f;
	Slam.bCanDamageTarget = true;
	Slam.LockedTargetLocation = Target->GetActorLocation();
	Slam.RadiusCm = 180.0f;
	Enemy->SetActorLocation(FVector(100.0f, 0.0f, 80.0f));
	Boss->ApplyBossIntentForTests(Slam);
	Target->AdvanceBossRepulse(0.3f);
	TestTrue(TEXT("Frozen player can be pushed without player gameplay tick"),
	         Target->GetActorLocation().Equals(FVector(200.0f, 0.0f, 80.0f), 0.01f));
	TestEqual(TEXT("Opening hit never damages player even without protection flag"),
	          Target->Combatant->CurrentHealth,
	          100.0f);
	Enemy->AdvanceBossOpeningRepulse(0.3f);
	TestTrue(TEXT("Grounded opening immediately repulses nearby enemy without descent delay"),
	         Enemy->GetActorLocation().Equals(FVector(300.0f, 0.0f, 80.0f), 0.01f));
	Boss->AdvancePendingBossBlinkSlamForTests(1.0f);
	Enemy->AdvanceBossOpeningRepulse(0.3f);
	TestTrue(TEXT("Landing pulse is consumed once"),
	         Enemy->GetActorLocation().Equals(FVector(300.0f, 0.0f, 80.0f), 0.01f));
	Slam.bPhaseOpening = false;
	Enemy->SetActorLocation(FVector(100.0f, 0.0f, 80.0f));
	Boss->ApplyBossIntentForTests(Slam);
	Boss->AdvancePendingBossBlinkSlamForTests(0.51f);
	Enemy->AdvanceBossOpeningRepulse(0.3f);
	TestTrue(TEXT("Normal later slam does not use opening repulse"),
	         Enemy->GetActorLocation().Equals(FVector(100.0f, 0.0f, 80.0f), 0.01f));
	Target->SetActorLocation(FVector(0.0f, 0.0f, 80.0f));
	Slam.Attack.Sequence = 2;
	Slam.AbilityId = TEXT("M_SHEEP_GroundTriple");
	Slam.bGroundedSlam = true;
	Boss->ApplyBossIntentForTests(Slam);
	TestTrue(TEXT("Combat GroundTriple damages player normally"), Target->Combatant->CurrentHealth < 100.0f);
	const float AfterCombatHitHealth = Target->Combatant->CurrentHealth;
	Target->AdvanceBossRepulse(0.3f);
	TestTrue(TEXT("Combat GroundTriple also repulses player"),
	         Target->GetActorLocation().Equals(FVector(200.0f, 0.0f, 80.0f), 0.01f));
	TestEqual(
	    TEXT("External repulse itself never adds damage"), Target->Combatant->CurrentHealth, AfterCombatHitHealth);
	AActor* Wall = Fixture.World->SpawnActor<AActor>();
	UBoxComponent* WallBox = NewObject<UBoxComponent>(Wall);
	Wall->SetRootComponent(WallBox);
	WallBox->SetBoxExtent(FVector(10.0f, 200.0f, 200.0f));
	WallBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WallBox->SetCollisionObjectType(ECC_WorldStatic);
	WallBox->SetCollisionResponseToAllChannels(ECR_Block);
	WallBox->RegisterComponent();
	Wall->SetActorLocation(FVector(220.0f, 0.0f, 80.0f));
	Enemy->SetActorEnableCollision(true);
	TestTrue(TEXT("Repulse starts near wall"), Enemy->BeginBossOpeningRepulse(FVector::ZeroVector, 200.0f, 0.3f));
	Enemy->AdvanceBossOpeningRepulse(0.3f);
	TestTrue(TEXT("Swept repulse stops before wall"),
	         Enemy->GetActorLocation().X > 100.0f && Enemy->GetActorLocation().X < 220.0f);
	Enemy->SetActorEnableCollision(false);
	Target->SetActorEnableCollision(true);
	Target->SetActorLocation(FVector(100.0f, 0.0f, 80.0f));
	Target->BeginBossRepulse(FVector::ZeroVector, 200.0f, 0.3f);
	Target->AdvanceBossRepulse(0.3f);
	TestTrue(TEXT("Player swept repulse stops before wall"),
	         Target->GetActorLocation().X > 100.0f && Target->GetActorLocation().X < 220.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHostBossPhase3GMAnimationTest,
                                 "ReEcho.Enemies.Host.BossPhase3GMAnimation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHostBossPhase3GMAnimationTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyHostWorldFixture Fixture;
	UReEcho2DPresentationCatalog* Catalog = LoadObject<UReEcho2DPresentationCatalog>(
	    nullptr, TEXT("/Game/ReEcho/DataAsset/Enemy/Catalogs/DA_EnemyPresentationCatalog.DA_EnemyPresentationCatalog"));
	UReEchoBossPhase3Config* Phase3Config = LoadObject<UReEchoBossPhase3Config>(
	    nullptr, TEXT("/Game/ReEcho/DataAsset/Enemy/DA_SheepBossPhase3.DA_SheepBossPhase3"));
	UPaperFlipbook* Phase3Walk =
	    LoadObject<UPaperFlipbook>(nullptr, TEXT("/Game/ReEcho/Art/Animation2D/Enemies/Goat/Phase3/Walk/Walk.Walk"));
	UPaperFlipbook* Phase3Attack = LoadObject<UPaperFlipbook>(
	    nullptr, TEXT("/Game/ReEcho/Art/Animation2D/Enemies/Goat/Phase3/GroundSlam/GroundSlam.GroundSlam"));
	if (!TestNotNull(TEXT("Phase3 GM animation loads the production catalog"), Catalog) ||
	    !TestNotNull(TEXT("Phase3 GM animation loads the programmer overlay"), Phase3Config) ||
	    !TestNotNull(TEXT("Phase3 GM animation loads the authored walk"), Phase3Walk) ||
	    !TestNotNull(TEXT("Phase3 GM animation loads the authored attack"), Phase3Attack))
	{
		return false;
	}

	const FReEchoCsvLoadResult LoadResult =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	FReEchoEnemyDefinition Definition;
	FString CompileError;
	if (!TestTrue(TEXT("Phase3 GM animation loads production enemy CSV"), LoadResult.bSuccess) ||
	    !TestTrue(
	        TEXT("Phase3 GM animation compiles the production sheep"),
	        ReEchoEnemyDefinitionCompiler::Compile(*LoadResult.Snapshot, TEXT("M_SHEEP"), Definition, CompileError)) ||
	    !TestTrue(TEXT("Phase3 GM animation applies the programmer overlay"),
	              Phase3Config->ApplyTo(TEXT("M_SHEEP"), Definition)))
	{
		AddError(LoadResult.bSuccess ? CompileError : LoadResult.FormatIssues());
		return false;
	}

	AReEchoEnemyActor* Sheep = Fixture.World->SpawnActor<AReEchoEnemyActor>();
	Sheep->SetEnemyId(TEXT("M_SHEEP"));
	Sheep->SetPresentationCatalog(Catalog);
	if (!TestTrue(TEXT("Phase3 GM animation configures the production sheep"),
	              Sheep->ConfigureFromDefinition(Definition, 153)))
	{
		return false;
	}
	UReEcho2DAnimationComponent* Animation = Sheep->FindComponentByClass<UReEcho2DAnimationComponent>();
	if (!TestNotNull(TEXT("Phase3 GM animation finds the sheep renderer"), Animation))
	{
		return false;
	}
	TestTrue(TEXT("Phase3 animation scale resolver applies two-times authored magnitude"),
	         UReEchoEnemyPresentationComponent::ResolveBossPhase3AnimationScale(FVector(-0.75f, 1.0f, 1.25f))
	             .Equals(FVector(1.5f, 2.0f, 2.5f), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Phase3 attack scale resolver preserves the original two-times attack magnitude"),
	         UReEchoEnemyPresentationComponent::ResolveBossPhase3AnimationScale(FVector(-0.75f, 1.0f, 1.25f), true)
	             .Equals(FVector(1.5f, 2.0f, 2.5f), KINDA_SMALL_NUMBER));

	TestTrue(TEXT("GMBossPhase 3 switches the authoritative phase"), Sheep->DebugForceBossPhaseForGM(3));
	const FReEchoBossPhaseDefinition* Phase2Definition = Definition.BossPhases.FindByPredicate(
	    [](const FReEchoBossPhaseDefinition& Phase)
	    {
		    return Phase.bEnabled && Phase.PhaseIndex == 2;
	    });
	TestNotNull(TEXT("Phase3 health contract finds the Phase2 definition"), Phase2Definition);
	if (Phase2Definition)
	{
		// Egao Party: the multiplier is 4/3 so (100000 + 50000) resolves to the authored 200000.
		const float ExpectedPhase3Health =
		    (Definition.MaxHealth + Phase2Definition->PhaseMaxHealth) * (4.0f / 3.0f);
		TestEqual(TEXT("Phase3 maximum health is the authored multiple of Phase1 and Phase2"),
		          Sheep->GetCombatantComponent()->Stats.HpMax,
		          ExpectedPhase3Health);
		TestEqual(TEXT("Phase3 starts refilled to its summed maximum health"),
		          Sheep->GetCombatantComponent()->CurrentHealth,
		          ExpectedPhase3Health);
	}
	TestEqual(TEXT("GMBossPhase 3 resolves the runtime TimeGuard profile's Phase3 walk"),
	          Animation->GetFlipbook(),
	          Phase3Walk);
	TestEqual(TEXT("Phase3 initializes one persistent renderer multiplier for every animation"),
	          Animation->GetDisplayScaleMultiplier(),
	          2.0f);
	TestEqual(TEXT("Phase3 initializes the Walk native height as the shared animation scale reference"),
	          Animation->GetDisplayScaleReferenceHeight(),
	          static_cast<float>(Phase3Walk->GetRenderBounds().BoxExtent.Z * 2.0));
	FVector Phase3WalkScale = Animation->GetRelativeScale3D();
	UReEchoEnemyPresentationComponent* EnemyPresentation =
	    Sheep->FindComponentByClass<UReEchoEnemyPresentationComponent>();
	TestNotNull(TEXT("Phase3 finds the enemy presentation component"), EnemyPresentation);
	if (EnemyPresentation)
	{
		EnemyPresentation->ApplyFacingSignForTests(-1.0f);
		TestTrue(TEXT("A camera-facing refresh preserves the fixed Phase3 scale"),
		         Animation->GetRelativeScale3D().GetAbs().Equals(Phase3WalkScale.GetAbs(), KINDA_SMALL_NUMBER));
		Phase3WalkScale = Animation->GetRelativeScale3D();
	}
	UBillboardComponent* BossWeaponSprite =
	    Cast<UBillboardComponent>(Sheep->GetDefaultSubobjectByName(TEXT("BossWeaponSprite")));
	TestTrue(TEXT("Phase3 hides the sheep weapon"),
	         BossWeaponSprite && !BossWeaponSprite->IsVisible() && BossWeaponSprite->bHiddenInGame);

	UReEchoCombatVfxComponent* CombatVfx = Sheep->FindComponentByClass<UReEchoCombatVfxComponent>();
	USceneComponent* BodyChargingRoot = Cast<USceneComponent>(Sheep->GetDefaultSubobjectByName(TEXT("AttackVfxRoot")));
	TestNotNull(TEXT("Phase3 Skill03 finds the Combat VFX component"), CombatVfx);
	TestNotNull(TEXT("Phase3 Skill03 finds the body charging root"), BodyChargingRoot);
	if (CombatVfx)
	{
		TestEqual(
		    TEXT("Phase3 transition reaches the Combat VFX state"), CombatVfx->GetCurrentBossPhaseIndexForTests(), 3);
	}
	FReEchoBossIntent Skill03Telegraph;
	Skill03Telegraph.Type = EReEchoBossIntentType::TelegraphStarted;
	Skill03Telegraph.AbilityKind = EReEchoBossAbilityKind::BlinkSlam;
	Skill03Telegraph.AbilityId = TEXT("M_SHEEP_BlinkSlam");
	Skill03Telegraph.Attack.Source = Sheep;
	Skill03Telegraph.Attack.Sequence = 1;
	Skill03Telegraph.LockedDirection = FVector::ForwardVector;
	Skill03Telegraph.WindupSeconds = 0.5f;
	Sheep->GetEnemyEventsComponent()->PublishBossIntent(Skill03Telegraph);
	TestEqual(TEXT("Phase3 Skill03 charging and ascent keep the Phase3 walk instead of playing attack"),
	          Animation->GetFlipbook(),
	          Phase3Walk);
	TestTrue(TEXT("Phase3 Skill03 charging and ascent preserve the Phase3 walk display scale"),
	         Animation->GetRelativeScale3D().Equals(Phase3WalkScale, KINDA_SMALL_NUMBER));
	UNiagaraComponent* ChargingEffect = CombatVfx ? CombatVfx->GetBossChargingEffectForTests() : nullptr;
	UNiagaraSystem* Phase3ChargingSystem = LoadObject<UNiagaraSystem>(
	    nullptr, TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill03_Charging.NS_Goat_Skill03_Charging"));
	TestNotNull(TEXT("Phase3 Skill03 charging asset is available"), Phase3ChargingSystem);
	TestEqual(TEXT("Phase3 Skill03 selects the sheep body instead of the removed weapon"),
	          UReEchoCombatVfxComponent::ResolveBossChargingAttachmentRoot(
	              true,
	              CombatVfx ? CombatVfx->GetCurrentBossPhaseIndexForTests() : 1,
	              BodyChargingRoot,
	              Cast<USceneComponent>(Sheep->GetDefaultSubobjectByName(TEXT("BossWeaponTipRoot")))),
	          BodyChargingRoot);
	if (ChargingEffect && BodyChargingRoot)
	{
		TestEqual(TEXT("A runtime-created Phase3 Skill03 charge attaches to the sheep body"),
		          ChargingEffect->GetAttachParent(),
		          BodyChargingRoot);
		const FBoxSphereBounds AnimationWorldBounds = Animation->CalcBounds(Animation->GetComponentTransform());
		TestTrue(TEXT("A runtime-created Phase3 Skill03 charge is centered on top of the active animation"),
		         ChargingEffect->GetComponentLocation().Equals(
		             UReEchoCombatVfxComponent::ResolveBossPhase3ChargingTopWorldLocation(AnimationWorldBounds),
		             KINDA_SMALL_NUMBER));
	}
	TestNotNull(TEXT("Phase3 combo finds the enemy presentation component"), EnemyPresentation);
	FReEchoBossIntent FirstImpact = Skill03Telegraph;
	FirstImpact.Type = EReEchoBossIntentType::AttackWindowStarted;
	Sheep->GetEnemyEventsComponent()->PublishBossIntent(FirstImpact);
	TestEqual(TEXT("Phase3 Skill03 spawns its ground crack when descent starts"),
	          CombatVfx ? CombatVfx->GetBossSkill03ImpactIntentCountForTests() : 0,
	          1);
	FReEchoBossIntent FirstLanding = FirstImpact;
	FirstLanding.Type = EReEchoBossIntentType::ImpactResolved;
	Sheep->GetEnemyEventsComponent()->PublishBossIntent(FirstLanding);
	TestEqual(TEXT("Phase3 Skill03 landing does not replay the early ground crack"),
	          CombatVfx ? CombatVfx->GetBossSkill03ImpactIntentCountForTests() : 0,
	          1);
	TestEqual(TEXT("Phase3 Skill03 selects attack only when descent begins"), Animation->GetFlipbook(), Phase3Attack);
	TestTrue(TEXT("The first Phase3 strike starts its descent presentation"),
	         EnemyPresentation && EnemyPresentation->GetBossBlinkSlamRemainingForTests() > 0.0f);
	TestTrue(TEXT("Phase3 Skill03 descent preserves the Walk pixel-to-world scale"),
	         Animation->GetRelativeScale3D().GetAbs().Equals(Phase3WalkScale.GetAbs(), KINDA_SMALL_NUMBER));
	TestEqual(TEXT("Phase3 Skill03 descent keeps GroundSlam alive for the full 0.5 second fall"),
	          Animation->GetFlipbookLength() / FMath::Max(Animation->GetPlayRate(), KINDA_SMALL_NUMBER),
	          0.5f);
	const FVector Phase3AttackScale = Animation->GetRelativeScale3D();
	FReEchoPresentationActionEvent DuplicateCommitted;
	DuplicateCommitted.Phase = EReEchoPresentationActionPhase::Committed;
	DuplicateCommitted.Key.Source = Sheep;
	DuplicateCommitted.Key.Sequence = 1;
	DuplicateCommitted.Key.AbilityId = TEXT("Enemy.Basic");
	if (EnemyPresentation)
	{
		EnemyPresentation->ConsumePresentationActionForTests(DuplicateCommitted);
	}
	TestEqual(TEXT("Generic committed presentation cannot replace a Phase3 Boss attack"),
	          Animation->GetFlipbook(),
	          Phase3Attack);
	TestTrue(TEXT("Generic committed presentation cannot shrink the Phase3 Boss attack"),
	         Animation->GetRelativeScale3D().Equals(Phase3AttackScale, KINDA_SMALL_NUMBER));
	FReEchoBossIntent SecondTelegraph = Skill03Telegraph;
	SecondTelegraph.Attack.Sequence = 2;
	SecondTelegraph.ComboStrikeIndex = 2;
	SecondTelegraph.ComboStrikeCount = 3;
	Sheep->GetEnemyEventsComponent()->PublishBossIntent(SecondTelegraph);
	TestEqual(TEXT("A new combo windup retires the prior strike's descent presentation"),
	          EnemyPresentation ? EnemyPresentation->GetBossBlinkSlamRemainingForTests() : -1.0f,
	          0.0f);
	TestEqual(TEXT("A combo landing briefly holds GroundSlam instead of shrinking on the impact frame"),
	          Animation->GetFlipbook(),
	          Phase3Attack);
	TestTrue(TEXT("A combo landing starts the short presentation-only hold"),
	         EnemyPresentation && EnemyPresentation->GetBossPhase3LandingHoldRemainingForTests() > 0.0f);
	if (EnemyPresentation)
	{
		EnemyPresentation->AdvanceBossPhase3WindupPresentationForTests(0.1f);
	}
	TestEqual(TEXT("After the landing hold, combo charging/ascent returns to the Phase3 walk"),
	          Animation->GetFlipbook(),
	          Phase3Walk);
	TestTrue(TEXT("After the landing hold, the next windup restores the Phase3 walk scale"),
	         Animation->GetRelativeScale3D().Equals(Phase3WalkScale, KINDA_SMALL_NUMBER));
	const FVector SecondWindupScale = Animation->GetRelativeScale3D();
	FReEchoDamageEvent Phase3Hurt;
	Phase3Hurt.Target = Sheep;
	Phase3Hurt.AppliedDamage = 1.0f;
	Phase3Hurt.WorldLocation = Sheep->GetActorLocation();
	Phase3Hurt.SourceWorldLocation = Sheep->GetActorLocation() - FVector::ForwardVector * 100.0f;
	Sheep->GetCombatEventsComponent()->PublishHurt(Phase3Hurt);
	TestEqual(
	    TEXT("A Phase3 hurt during charging cannot replace the Phase3 walk"), Animation->GetFlipbook(), Phase3Walk);
	TestTrue(TEXT("A Phase3 hurt cannot reset the fixed Phase3 scale"),
	         Animation->GetRelativeScale3D().Equals(SecondWindupScale, KINDA_SMALL_NUMBER));
	FReEchoBossIntent SecondImpact = SecondTelegraph;
	SecondImpact.Type = EReEchoBossIntentType::AttackWindowStarted;
	Sheep->GetEnemyEventsComponent()->PublishBossIntent(SecondImpact);
	TestEqual(TEXT("A later combo strike independently spawns its early ground crack"),
	          CombatVfx ? CombatVfx->GetBossSkill03ImpactIntentCountForTests() : 0,
	          2);
	FReEchoBossIntent SecondLanding = SecondImpact;
	SecondLanding.Type = EReEchoBossIntentType::ImpactResolved;
	Sheep->GetEnemyEventsComponent()->PublishBossIntent(SecondLanding);
	TestEqual(TEXT("A later combo landing also avoids replaying its ground crack"),
	          CombatVfx ? CombatVfx->GetBossSkill03ImpactIntentCountForTests() : 0,
	          2);
	float DeathDuration = 0.0f;
	UReEchoEnemyPresentationComponent* DeathPresentation =
	    Sheep->FindComponentByClass<UReEchoEnemyPresentationComponent>();
	TestTrue(TEXT("Phase3 terminal death starts"),
	         DeathPresentation &&
	             DeathPresentation->BeginTerminalDeath(FVector::ZeroVector, FSimpleDelegate(), DeathDuration));
	TestTrue(TEXT("Phase3 explicitly reuses black-goat phase2 death"),
	         Animation->GetFlipbook() && Animation->GetFlipbook()->GetPathName().Contains(TEXT("BadGoat")) &&
	             Animation->GetFlipbook()->GetPathName().Contains(TEXT("Death")));
	TestTrue(TEXT("Reused phase2 death is a finite one-shot"), DeathDuration > 0.0f && !Animation->IsLooping());
	TestEqual(TEXT("Death does not retain Phase3 attack enlargement"), Animation->GetDisplayScaleMultiplier(), 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyHostBornGameplayGateTest,
                                 "ReEcho.Enemies.Host.BornGameplayGate",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyHostBornGameplayGateTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyHostWorldFixture Fixture;
	UReEcho2DPresentationCatalog* Catalog = LoadObject<UReEcho2DPresentationCatalog>(
	    nullptr, TEXT("/Game/ReEcho/DataAsset/Enemy/Catalogs/DA_EnemyPresentationCatalog.DA_EnemyPresentationCatalog"));
	if (!TestNotNull(TEXT("Born gameplay gate loads the production enemy catalog"), Catalog))
	{
		return false;
	}
	AReEchoEnemyActor* Rabbit = Fixture.World->SpawnActor<AReEchoEnemyActor>();
	if (!TestNotNull(TEXT("Born gameplay gate spawns an enemy Host"), Rabbit))
	{
		return false;
	}
	Rabbit->SetPresentationCatalog(Catalog);
	FReEchoEnemyDefinition Definition = ReEchoEnemyDefinitions::MakeLegacyEquivalent(EReEchoEnemyArchetype::Grunt);
	Definition.PresentationId = TEXT("Enemy.Rabbit");
	TestTrue(TEXT("Authored Born configures successfully"), Rabbit->ConfigureFromDefinition(Definition, 14));
	TestTrue(TEXT("Successful Born activates the Host gameplay gate"), Rabbit->IsBornGameplayGateActiveForTests());
	Rabbit->FindComponentByClass<UReEchoEnemyPresentationComponent>()->NormalizeSacrificeBornDuration();
	TestFalse(TEXT("Sacrifice cannot capture a ground pose while Born is playing"),
	          Rabbit->FindComponentByClass<UReEchoEnemyPresentationComponent>()->PrepareSacrificeGroundPose());
	TestFalse(TEXT("Sacrifice cannot begin before Born completes"),
	          Rabbit->FindComponentByClass<UReEchoEnemyPresentationComponent>()->BeginSacrificeVisual());
	const UReEcho2DAnimationComponent* BornRenderer = Rabbit->FindComponentByClass<UReEcho2DAnimationComponent>();
	TestTrue(TEXT("Cinematic summon plays its full Born in 0.5 seconds"),
	         BornRenderer &&
	             FMath::IsNearlyEqual(BornRenderer->GetFlipbookLength() / BornRenderer->GetPlayRate(), 0.5f));
	TestFalse(TEXT("Born gameplay gate disables incoming damage"), Rabbit->CanBeDamaged());
	FReEchoHitIntent Hit;
	Hit.RawDamage = 10.0f;
	TestEqual(TEXT("Born gameplay gate rejects raw damage before Combat"), Rabbit->ModifyIncomingRawDamage(Hit), 0.0f);
	const FVector GatedLocation = Rabbit->GetActorLocation();
	FReEchoBossIntent BlinkIntent;
	BlinkIntent.Type = EReEchoBossIntentType::AttackWindowStarted;
	BlinkIntent.AbilityId = TEXT("M_SHEEP_BlinkSlam");
	BlinkIntent.Target = Rabbit;
	BlinkIntent.bRequestTeleport = true;
	BlinkIntent.TeleportDestination = FVector(700.0f, 300.0f, 0.0f);
	Rabbit->ApplyBossIntentForTests(BlinkIntent);
	TestTrue(TEXT("Host Born gate defensively suppresses Boss teleport/attack windows"),
	         Rabbit->GetActorLocation().Equals(GatedLocation));
	Fixture.World->Tick(LEVELTICK_All, 0.1f);
	TestTrue(TEXT("Born gameplay gate keeps the Host world position fixed"),
	         Rabbit->GetActorLocation().Equals(GatedLocation));
	const FReEchoEnemyRuntimeState SavedRabbit = Rabbit->CaptureRuntimeState();
	AReEchoEnemyActor* RestoredRabbit = Fixture.World->SpawnActor<AReEchoEnemyActor>();
	RestoredRabbit->SetPresentationCatalog(Catalog);
	TestTrue(TEXT("Restore regression configures the saved enemy definition"),
	         RestoredRabbit->ConfigureFromDefinition(Definition, 14));
	TestTrue(TEXT("Restore regression setup starts Born before snapshot restore"),
	         RestoredRabbit->IsBornGameplayGateActiveForTests());
	RestoredRabbit->RestoreRuntimeState(SavedRabbit);
	TestFalse(TEXT("Runtime restore cancels the configuration-started Born gate"),
	          RestoredRabbit->IsBornGameplayGateActiveForTests());
	TestFalse(TEXT("Runtime restore returns presentation to the normal base state"),
	          RestoredRabbit->IsBornPresentationActiveForTests());
	TestTrue(TEXT("Runtime restore leaves a living saved enemy damageable"), RestoredRabbit->CanBeDamaged());
	TestEqual(TEXT("Runtime restore does not leave false Born invulnerability"),
	          RestoredRabbit->ModifyIncomingRawDamage(Hit),
	          10.0f);
	Rabbit->CompleteBornGameplayGateForTests();
	TestTrue(TEXT("Completed Born can settle on ground before sacrifice"),
	         Rabbit->FindComponentByClass<UReEchoEnemyPresentationComponent>()->PrepareSacrificeGroundPose());
	UReEchoEnemyPresentationComponent* ReadyVisual = Rabbit->FindComponentByClass<UReEchoEnemyPresentationComponent>();
	TestTrue(TEXT("Grounded rabbit starts sacrifice"), ReadyVisual->BeginSacrificeVisual());
	ReadyVisual->PlaySacrificeTransformAnimation();
	TestFalse(TEXT("Playing monster transformation blocks landing"), ReadyVisual->IsSacrificeTransformComplete());
	// Cinematic freezes controller completion; only the renderer finishes before landing.
	Rabbit->FindComponentByClass<UReEcho2DAnimationComponent>()->Stop();
	TestTrue(TEXT("Finished monster transformation permits landing"), ReadyVisual->IsSacrificeTransformComplete());
	ReadyVisual->EndSacrificeVisual();
	TestFalse(TEXT("Natural Born completion releases the Host gameplay gate"),
	          Rabbit->IsBornGameplayGateActiveForTests());
	TestTrue(TEXT("Natural Born completion restores the prior damage state"), Rabbit->CanBeDamaged());
	Rabbit->ApplyBossIntentForTests(BlinkIntent);
	TestTrue(TEXT("Host accepts the next eligible Boss attack window after Born completes"),
	         Rabbit->GetActorLocation().Equals(BlinkIntent.TeleportDestination));

	AReEchoEnemyActor* MissingBorn = Fixture.World->SpawnActor<AReEchoEnemyActor>();
	MissingBorn->SetPresentationCatalog(Catalog);
	Definition.PresentationId = TEXT("Enemy.UnknownBornProfile");
	TestTrue(TEXT("Missing Born profile does not block gameplay configuration"),
	         MissingBorn->ConfigureFromDefinition(Definition, 15));
	TestFalse(TEXT("Missing Born never activates the Host gameplay gate"),
	          MissingBorn->IsBornGameplayGateActiveForTests());
	TestTrue(TEXT("Missing Born remains damageable"), MissingBorn->CanBeDamaged());
	return true;
}

#endif
