#if WITH_DEV_AUTOMATION_TESTS

#include "Weapons/ReEchoWeaponRuntime.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Components/SceneComponent.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Enemies/ReEchoEnemyLogicComponent.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Graybox/ReEchoProjectileActor.h"
#include "Graybox/ReEchoTimeShardPickupActor.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoRunSubsystem.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Recording/ReEchoRecorderComponent.h"
#include "Weapons/ReEchoWeaponActor.h"
#include "Weapons/ReEchoWeaponLogic.h"

namespace
{
struct FReEchoWeaponWorldFixture
{
	UWorld* World = nullptr;

	FReEchoWeaponWorldFixture()
	{
		const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("ReEchoWeaponTestWorld"));
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		World->AddToRoot();
		WorldContext.SetCurrentWorld(World);
		World->SetShouldTick(true);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
	}

	~FReEchoWeaponWorldFixture()
	{
		if (World)
		{
			World->DestroyWorld(true);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}
	}

	void Advance(const float Seconds)
	{
		const double ExpectedTimeSeconds = World->GetTimeSeconds() + Seconds;
		World->Tick(ELevelTick::LEVELTICK_All, Seconds);
		if (!FMath::IsNearlyEqual(World->GetTimeSeconds(), ExpectedTimeSeconds, 0.001))
		{
			World->TimeSeconds = ExpectedTimeSeconds;
		}
	}

	AReEchoEnemyActor* SpawnEnemy(const FVector& Location, const int32 SpawnIndex, const float HpMax = 100.0f)
	{
		AReEchoEnemyActor* Enemy = World->SpawnActor<AReEchoEnemyActor>(Location, FRotator::ZeroRotator);
		Enemy->Configure(EReEchoEnemyKind::Grunt, SpawnIndex);
		FReEchoStatBlock Stats;
		Stats.HpMax = HpMax;
		Stats.HpPoint = HpMax;
		Stats.Block = 0;
		Enemy->GetCombatantComponent()->BindToAbilitySystem(Enemy->GetAbilitySystemComponent());
		Enemy->GetCombatantComponent()->InitializeFromStats(Stats, true);
		return Enemy;
	}

	AActor* SpawnWeaponOwner(const FVector& Location, UReEchoCombatantComponent*& OutCombatant)
	{
		AActor* Owner = World->SpawnActor<AActor>(Location, FRotator::ZeroRotator);
		USceneComponent* Root = NewObject<USceneComponent>(Owner, TEXT("WeaponOwnerRoot"));
		Owner->SetRootComponent(Root);
		Owner->AddInstanceComponent(Root);
		Root->RegisterComponent();
		Owner->SetActorLocation(Location);
		OutCombatant = NewObject<UReEchoCombatantComponent>(Owner, TEXT("WeaponOwnerCombatant"));
		Owner->AddInstanceComponent(OutCombatant);
		OutCombatant->RegisterComponent();
		FReEchoStatBlock Stats;
		Stats.HpMax = 100.0f;
		Stats.HpPoint = 100.0f;
		Stats.PhysicalAttack = 50.0f;
		Stats.ElementalAttack = 100.0f;
		Stats.AttackSpeed = 1.0f;
		Stats.ReactionEfficiency = 1.0f;
		OutCombatant->InitializeFromStats(Stats, true);
		UReEchoCombatEventsComponent* Events = NewObject<UReEchoCombatEventsComponent>(Owner, TEXT("WeaponOwnerEvents"));
		Owner->AddInstanceComponent(Events);
		Events->RegisterComponent();
		return Owner;
	}
};

float WeaponEnemyHealth(const AReEchoEnemyActor* Enemy)
{
	return Enemy->GetCombatantComponent()->CurrentHealth;
}

void SetBuildStats(FReEchoBuildSnapshot& Build, const FReEchoStatBlock& Stats)
{
	Build.Stats = Stats;
	Build.EquipmentBaseStats = Stats;
	Build.bHasEquipmentBase = true;
}

FReEchoBuildSnapshot MakeBuild(const FReEchoCsvDataSnapshot& Snapshot, const FName WeaponId)
{
	FReEchoStartRunResolveResult Result =
	    ReEchoRunData::ResolveStartingBuildFromSnapshot(&Snapshot, TEXT("J_SPADE"), WeaponId);
	check(Result.bSuccess);
	return Result.Build;
}

int32 CountProjectiles(UWorld* World)
{
	int32 Count = 0;
	for (TActorIterator<AReEchoProjectileActor> It(World); It; ++It)
	{
		if (!It->IsActorBeingDestroyed())
		{
			++Count;
		}
	}
	return Count;
}

void TickProjectiles(UWorld* World, const float Seconds)
{
	TArray<AReEchoProjectileActor*> Projectiles;
	for (TActorIterator<AReEchoProjectileActor> It(World); It; ++It)
	{
		if (!It->IsActorBeingDestroyed())
		{
			Projectiles.Add(*It);
		}
	}
	for (AReEchoProjectileActor* Projectile : Projectiles)
	{
		if (IsValid(Projectile) && !Projectile->IsActorBeingDestroyed())
		{
			if (UReEchoProjectileLogicComponent* Logic =
			        Projectile->FindComponentByClass<UReEchoProjectileLogicComponent>())
			{
				Logic->AdvanceSimulation(Seconds);
			}
		}
	}
}

FString AssembleModifiedCsvDirectory(const TCHAR* FileName, const TCHAR* SearchText, const TCHAR* ReplacementText)
{
	const FString SourceDataDirectory = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"));
	const FString AssembledDirectory = FPaths::Combine(FPaths::ProjectSavedDir(),
	                                                   TEXT("Automation"),
	                                                   TEXT("WeaponRuntime"),
	                                                   FGuid::NewGuid().ToString(EGuidFormats::Digits));
	IFileManager& FileManager = IFileManager::Get();
	FileManager.MakeDirectory(*AssembledDirectory, true);

	TArray<FString> ProductionCsvFiles;
	FileManager.FindFiles(ProductionCsvFiles, *FPaths::Combine(SourceDataDirectory, TEXT("*.csv")), true, false);
	for (const FString& CsvFileName : ProductionCsvFiles)
	{
		FileManager.Copy(*FPaths::Combine(AssembledDirectory, CsvFileName),
		                 *FPaths::Combine(SourceDataDirectory, CsvFileName));
	}

	const FString TargetPath = FPaths::Combine(AssembledDirectory, FileName);
	FString Contents;
	check(FFileHelper::LoadFileToString(Contents, *TargetPath));
	check(Contents.Contains(SearchText));
	Contents = Contents.Replace(SearchText, ReplacementText);
	check(FFileHelper::SaveStringToFile(Contents, *TargetPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
	return AssembledDirectory;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponMeleeStepRuntimeTest,
                                 "ReEcho.Weapons.MeleeStepsUseOrderArcAndTiming",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponMeleeStepRuntimeTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	FReEchoWeaponWorldFixture Fixture;
	UReEchoCombatantComponent* Combatant = nullptr;
	AActor* Owner = Fixture.SpawnWeaponOwner(FVector::ZeroVector, Combatant);
	FReEchoBuildSnapshot Build = MakeBuild(*Snapshot, TEXT("W_J_01"));
	SetBuildStats(Build, Combatant->Stats);
	AReEchoWeaponActor* Weapon = Fixture.World->SpawnActor<AReEchoWeaponActor>();
	Weapon->SetOwner(Owner);
	Weapon->InitializeWeapon(&Build, Snapshot);
	AReEchoEnemyActor* Front = Fixture.SpawnEnemy(FVector(100.0f, 0.0f, 0.0f), 1, 100.0f);
	AReEchoEnemyActor* Side = Fixture.SpawnEnemy(FVector(-100.0f, 0.0f, 0.0f), 2, 100.0f);

	TestTrue(TEXT("First melee step executes"), Weapon->ExecuteBasicAttack(Combatant));
	TestEqual(TEXT("Front target takes first ordered step damage"), WeaponEnemyHealth(Front), 50.0f);
	TestEqual(TEXT("Side target outside arc is untouched"), WeaponEnemyHealth(Side), 100.0f);
	TestFalse(TEXT("Second step cannot execute before weapon cadence ends"), Weapon->ExecuteBasicAttack(Combatant));
	Fixture.Advance(1.01f);
	Weapon->Tick(1.01f);
	TestTrue(TEXT("Second ordered step executes after the authored one-second cadence"),
	         Weapon->ExecuteBasicAttack(Combatant));
	TestTrue(TEXT("Front target is killed by second step"), !Front->IsAlive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoScytheOuterRingRuntimeTest,
                                 "ReEcho.Weapons.ScytheBaseOuterRingUsesAuthoredThresholdAndBonus",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoScytheOuterRingRuntimeTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	FReEchoWeaponWorldFixture Fixture;
	UReEchoCombatantComponent* Combatant = nullptr;
	AActor* Owner = Fixture.SpawnWeaponOwner(FVector::ZeroVector, Combatant);
	FReEchoBuildSnapshot Build = MakeBuild(*Snapshot, TEXT("W_J_04"));
	SetBuildStats(Build, Combatant->Stats);
	AReEchoWeaponActor* Weapon = Fixture.World->SpawnActor<AReEchoWeaponActor>();
	Weapon->SetOwner(Owner);
	Weapon->InitializeWeapon(&Build, Snapshot);

	AReEchoEnemyActor* Inner = Fixture.SpawnEnemy(FVector(74.0f, 0.0f, 0.0f), 40, 1000.0f);
	AReEchoEnemyActor* Boundary = Fixture.SpawnEnemy(FVector(75.0f, 0.0f, 0.0f), 41, 1000.0f);
	AReEchoEnemyActor* Outer = Fixture.SpawnEnemy(FVector(140.0f, 0.0f, 0.0f), 42, 1000.0f);
	TestTrue(TEXT("Scythe sweep executes"), Weapon->ExecuteBasicAttack(Combatant));

	const float InnerDamage = 1000.0f - WeaponEnemyHealth(Inner);
	const float BoundaryDamage = 1000.0f - WeaponEnemyHealth(Boundary);
	const float OuterDamage = 1000.0f - WeaponEnemyHealth(Outer);
	TestTrue(TEXT("Scythe deals positive base damage in the inner ring"), InnerDamage > 0.0f);
	TestTrue(TEXT("The authored 50 percent boundary receives the 40 percent bonus"),
	         FMath::IsNearlyEqual(BoundaryDamage, InnerDamage * 1.4f, 0.01f));
	TestTrue(TEXT("Multiple outer-ring targets receive the same authored bonus"),
	         FMath::IsNearlyEqual(OuterDamage, InnerDamage * 1.4f, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponProjectileRuntimeTest,
                                 "ReEcho.Weapons.ProjectilesUseSingleShotSpreadCountAndLifetime",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponProjectileRuntimeTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();

	FReEchoWeaponWorldFixture SingleFixture;
	UReEchoCombatantComponent* SingleCombatant = nullptr;
	AActor* SingleOwner = SingleFixture.SpawnWeaponOwner(FVector::ZeroVector, SingleCombatant);
	FReEchoBuildSnapshot SingleBuild = MakeBuild(*Snapshot, TEXT("W_J_08"));
	SetBuildStats(SingleBuild, SingleCombatant->Stats);
	AReEchoWeaponActor* SingleWeapon = SingleFixture.World->SpawnActor<AReEchoWeaponActor>();
	SingleWeapon->SetOwner(SingleOwner);
	SingleWeapon->InitializeWeapon(&SingleBuild, Snapshot);
	TestTrue(TEXT("Single projectile attack executes"), SingleWeapon->ExecuteBasicAttack(SingleCombatant));
	TestEqual(TEXT("Single projectile pattern spawns one projectile"), CountProjectiles(SingleFixture.World), 1);

	FReEchoWeaponWorldFixture SpreadFixture;
	UReEchoCombatantComponent* Combatant = nullptr;
	AActor* Owner = SpreadFixture.SpawnWeaponOwner(FVector::ZeroVector, Combatant);
	FReEchoBuildSnapshot Build = MakeBuild(*Snapshot, TEXT("W_J_08"));
	SetBuildStats(Build, Combatant->Stats);
	AReEchoWeaponActor* Weapon = SpreadFixture.World->SpawnActor<AReEchoWeaponActor>();
	Weapon->SetOwner(Owner);
	Weapon->InitializeWeapon(&Build, Snapshot);
	AReEchoEnemyActor* Primary = SpreadFixture.SpawnEnemy(FVector(1000.0f, 0.0f, 0.0f), 4, 100.0f);
	AReEchoEnemyActor* Splash = SpreadFixture.SpawnEnemy(FVector(1000.0f, 120.0f, 0.0f), 5, 100.0f);
	FReEchoEnemyLogicSnapshot TransformingState = Primary->GetEnemyLogicComponent()->GetSnapshot();
	TransformingState.Phase = EReEchoEnemyBehaviorPhase::Transforming;
	Primary->GetEnemyLogicComponent()->RestoreSnapshot(TransformingState);
	TestTrue(TEXT("Spread projectile attack executes"), Weapon->ExecuteBasicAttack(Combatant));
	TestEqual(
	    TEXT("Canonical bow projectile count comes from weapon definition"), CountProjectiles(SpreadFixture.World), 1);
	for (TActorIterator<AReEchoProjectileActor> It(SpreadFixture.World); It; ++It)
	{
		TestEqual(TEXT("Canonical bow has no base explosion radius"), It->GetExplosionRadiusCm(), 0.0f);
	}
	const TArray<FVector> SyntheticSpread =
	    ReEchoWeaponRuntime::BuildProjectileDirections(FVector::ForwardVector, 3, 30.0f);
	TestEqual(TEXT("Pure spread helper still produces requested direction count"), SyntheticSpread.Num(), 3);
	TestTrue(TEXT("Pure spread helper produces distinct edge directions"),
	         SyntheticSpread.Num() == 3 &&
	             !FMath::IsNearlyEqual(SyntheticSpread[0].Rotation().Yaw, SyntheticSpread[2].Rotation().Yaw, 0.1f));
	TickProjectiles(SpreadFixture.World, 1.10f);
	TestTrue(TEXT("Projectile damages an ordinary enemy during presentation-only transformation"),
	         WeaponEnemyHealth(Primary) < 100.0f);
	TestEqual(TEXT("Non-explosive bow projectile leaves nearby target untouched"), WeaponEnemyHealth(Splash), 100.0f);

	FReEchoWeaponWorldFixture LifetimeFixture;
	UReEchoCombatantComponent* LifetimeCombatant = nullptr;
	AActor* LifetimeOwner = LifetimeFixture.SpawnWeaponOwner(FVector::ZeroVector, LifetimeCombatant);
	FReEchoBuildSnapshot LifetimeBuild = MakeBuild(*Snapshot, TEXT("W_J_08"));
	SetBuildStats(LifetimeBuild, LifetimeCombatant->Stats);
	AReEchoWeaponActor* LifetimeWeapon = LifetimeFixture.World->SpawnActor<AReEchoWeaponActor>();
	LifetimeWeapon->SetOwner(LifetimeOwner);
	LifetimeWeapon->InitializeWeapon(&LifetimeBuild, Snapshot);
	AReEchoEnemyActor* DelayedTarget = LifetimeFixture.SpawnEnemy(FVector(1000.0f, 0.0f, 0.0f), 6, 100.0f);
	TestTrue(TEXT("Delayed projectile attack executes"), LifetimeWeapon->ExecuteBasicAttack(LifetimeCombatant));
	TestTrue(TEXT("Attack source can leave the world before impact"), LifetimeOwner->Destroy());
	TickProjectiles(LifetimeFixture.World, 1.10f);
	TestTrue(TEXT("Delayed projectile still resolves without dereferencing the destroyed source"),
	         WeaponEnemyHealth(DelayedTarget) < 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponPlayerAimDirectionTest,
                                 "ReEcho.Weapons.PlayerLogicalAimDrivesProjectile",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponPlayerAimDirectionTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	FReEchoWeaponWorldFixture Fixture;
	AReEchoPlayerPawn* Player =
	    Fixture.World->SpawnActor<AReEchoPlayerPawn>(FVector::ZeroVector, FRotator::ZeroRotator);
	AActor* AimTarget = Fixture.World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("Player spawns"), Player) || !TestNotNull(TEXT("Aim target spawns"), AimTarget))
	{
		return false;
	}
	USceneComponent* AimTargetRoot = NewObject<USceneComponent>(AimTarget, TEXT("AimTargetRoot"));
	AimTarget->SetRootComponent(AimTargetRoot);
	AimTarget->AddInstanceComponent(AimTargetRoot);
	AimTargetRoot->RegisterComponent();
	AimTarget->SetActorLocation(FVector(0.0f, 1000.0f, 0.0f));
	Player->FaceAutomaticTarget(*AimTarget);
	TestTrue(TEXT("Logical aim differs from the unchanged actor forward"),
	         FVector::DotProduct(Player->GetAttackAimDirection(), FVector::RightVector) > 0.99f);

	UReEchoCombatantComponent* Combatant = Player->GetCombatTargetCombatant();
	if (!TestNotNull(TEXT("Player combatant exists"), Combatant))
	{
		return false;
	}
	FReEchoBuildSnapshot Build = MakeBuild(*Snapshot, TEXT("W_J_08"));
	SetBuildStats(Build, Combatant->Stats);
	AReEchoWeaponActor* Weapon = Fixture.World->SpawnActor<AReEchoWeaponActor>();
	Weapon->SetOwner(Player);
	Weapon->InitializeWeapon(&Build, Snapshot);
	TestTrue(TEXT("Player projectile attack executes"), Weapon->ExecuteBasicAttack(Combatant));

	AReEchoProjectileActor* Projectile = nullptr;
	for (TActorIterator<AReEchoProjectileActor> It(Fixture.World); It; ++It)
	{
		if (It->GetOwner() == Player)
		{
			Projectile = *It;
			break;
		}
	}
	if (!TestNotNull(TEXT("Player attack spawns a projectile"), Projectile))
	{
		return false;
	}
	const FVector ProjectileDirection = Projectile->GetVelocity().GetSafeNormal2D();
	TestTrue(TEXT("Projectile consumes player logical aim instead of actor forward"),
	         FVector::DotProduct(ProjectileDirection, Player->GetAttackAimDirection()) > 0.99f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponEchoFriendlyFireTest,
                                 "ReEcho.Weapons.EchoAttacksIgnorePlayerSide",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponEchoFriendlyFireTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	auto InitializeEcho = [&](FReEchoWeaponWorldFixture& Fixture, const FName WeaponId)
	{
		FReEchoRecording Recording;
		Recording.Id = FGuid::NewGuid();
		Recording.BuildSnapshot = MakeBuild(*Snapshot, WeaponId);
		Recording.Positions.Add({0.0f, FVector::ZeroVector});
		Recording.Positions.Add({1.0f, FVector::ZeroVector});
		Recording.Duration = 1.0f;
		AReEchoEchoActor* Echo =
		    Fixture.World->SpawnActor<AReEchoEchoActor>(FVector::ZeroVector, FRotator::ZeroRotator);
		return Echo && Echo->InitializeEcho(Recording, 1.0f, Snapshot) ? Echo : nullptr;
	};
	auto InitializePlayer = [](AReEchoPlayerPawn& Player)
	{
		FReEchoStatBlock Stats;
		Stats.HpMax = 100.0f;
		Stats.HpPoint = 100.0f;
		Stats.Block = 0;
		Player.GetCombatTargetCombatant()->InitializeFromStats(Stats, true);
	};

	FReEchoWeaponWorldFixture MeleeFixture;
	AReEchoPlayerPawn* MeleePlayer =
	    MeleeFixture.World->SpawnActor<AReEchoPlayerPawn>(FVector(100.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	AReEchoEnemyActor* MeleeEnemy = MeleeFixture.SpawnEnemy(FVector(180.0f, 0.0f, 0.0f), 30, 100.0f);
	AReEchoEchoActor* MeleeEcho = InitializeEcho(MeleeFixture, TEXT("W_J_01"));
	if (!TestNotNull(TEXT("Melee player spawns"), MeleePlayer) ||
	    !TestNotNull(TEXT("Melee echo initializes"), MeleeEcho))
	{
		return false;
	}
	InitializePlayer(*MeleePlayer);
	const float MeleePlayerHealth = MeleePlayer->GetCombatTargetCombatant()->CurrentHealth;
	MeleeEcho->Tick(0.01f);
	TestEqual(TEXT("Echo melee never damages the player side"),
	          MeleePlayer->GetCombatTargetCombatant()->CurrentHealth,
	          MeleePlayerHealth);
	TestTrue(TEXT("Echo melee still damages an enemy"), WeaponEnemyHealth(MeleeEnemy) < 100.0f);

	FReEchoWeaponWorldFixture ProjectileFixture;
	AReEchoPlayerPawn* ProjectilePlayer =
	    ProjectileFixture.World->SpawnActor<AReEchoPlayerPawn>(FVector(100.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	AReEchoEnemyActor* ProjectileEnemy = ProjectileFixture.SpawnEnemy(FVector(180.0f, 0.0f, 0.0f), 31, 100.0f);
	AReEchoEchoActor* ProjectileEcho = InitializeEcho(ProjectileFixture, TEXT("W_J_08"));
	if (!TestNotNull(TEXT("Projectile player spawns"), ProjectilePlayer) ||
	    !TestNotNull(TEXT("Projectile echo initializes"), ProjectileEcho))
	{
		return false;
	}
	InitializePlayer(*ProjectilePlayer);
	const float ProjectilePlayerHealth = ProjectilePlayer->GetCombatTargetCombatant()->CurrentHealth;
	ProjectileEcho->Tick(0.01f);
	TestEqual(TEXT("Echo projectile spawns"), CountProjectiles(ProjectileFixture.World), 1);
	TickProjectiles(ProjectileFixture.World, 0.25f);
	TestEqual(TEXT("Echo projectile passes the player side without damage"),
	          ProjectilePlayer->GetCombatTargetCombatant()->CurrentHealth,
	          ProjectilePlayerHealth);
	TestTrue(TEXT("Echo projectile still damages an enemy behind the player"),
	         WeaponEnemyHealth(ProjectileEnemy) < 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponEquipmentCombatRuntimeTest,
                                 "ReEcho.Weapons.EquipmentChangesActualCooldownAndDamage",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponEquipmentCombatRuntimeTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();

	FReEchoWeaponWorldFixture PhysicalFixture;
	UReEchoCombatantComponent* PhysicalCombatant = nullptr;
	AActor* PhysicalOwner = PhysicalFixture.SpawnWeaponOwner(FVector::ZeroVector, PhysicalCombatant);
	FReEchoBuildSnapshot PhysicalBuild = MakeBuild(*Snapshot, TEXT("W_J_01"));
	SetBuildStats(PhysicalBuild, PhysicalCombatant->Stats);
	AReEchoWeaponActor* PhysicalWeapon = PhysicalFixture.World->SpawnActor<AReEchoWeaponActor>();
	PhysicalWeapon->SetOwner(PhysicalOwner);
	PhysicalWeapon->InitializeWeapon(&PhysicalBuild, Snapshot);
	AReEchoEnemyActor* PhysicalTarget = PhysicalFixture.SpawnEnemy(FVector(100.0f, 0.0f, 0.0f), 10, 100.0f);
	TestTrue(TEXT("Unequipped physical attack executes"), PhysicalWeapon->ExecuteBasicAttack(PhysicalCombatant));
	TestEqual(TEXT("Unequipped attack consumes current physical damage in combat"),
	          WeaponEnemyHealth(PhysicalTarget),
	          FMath::Max(0.0f, 100.0f - PhysicalCombatant->Stats.PhysicalAttack));

	FReEchoWeaponWorldFixture ElementFixture;
	UReEchoCombatantComponent* ElementCombatant = nullptr;
	AActor* ElementOwner = ElementFixture.SpawnWeaponOwner(FVector::ZeroVector, ElementCombatant);
	FReEchoBuildSnapshot ElementBuild = MakeBuild(*Snapshot, TEXT("W_J_01"));
	SetBuildStats(ElementBuild, ElementCombatant->Stats);
	FString Error;
	TestTrue(TEXT("Flame core equips for combat proof"),
	         ReEchoWeaponRuntime::TryEquipParts(*Snapshot, ElementBuild, {TEXT("P_CORE_FLAME")}, ElementBuild, Error));
	ElementCombatant->InitializeFromStats(ElementBuild.Stats, true);
	AReEchoWeaponActor* ElementWeapon = ElementFixture.World->SpawnActor<AReEchoWeaponActor>();
	ElementWeapon->SetOwner(ElementOwner);
	ElementWeapon->InitializeWeapon(&ElementBuild, Snapshot);
	AReEchoEnemyActor* ElementTarget = ElementFixture.SpawnEnemy(FVector(100.0f, 0.0f, 0.0f), 11, 100.0f);
	TestTrue(TEXT("Core-modified elemental attack executes"), ElementWeapon->ExecuteBasicAttack(ElementCombatant));
	TestEqual(TEXT("Core channel consumes current elemental damage in combat"),
	          WeaponEnemyHealth(ElementTarget),
	          FMath::Max(0.0f, 100.0f - ElementCombatant->Stats.ElementalAttack));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponGemDamageCoefficientMatrixTest,
                                 "ReEcho.Weapons.Gems.DamageCoefficientMatrix",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoPrismMultishotElementTest,
                                 "ReEcho.Weapons.Gems.PrismMultishotUsesPerProjectileElements",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoPrismMultishotElementTest::RunTest(const FString& Parameters)
{
	bool bObservedDifferentElementsWithinOneAttack = false;
	for (int64 AttackSequence = 1; AttackSequence <= 32; ++AttackSequence)
	{
		const EReEchoElement First =
		    ReEchoWeaponRuntime::ResolveProjectileElement(true, EReEchoElement::Water, AttackSequence, 0);
		for (int32 ProjectileIndex = 0; ProjectileIndex < 3; ++ProjectileIndex)
		{
			const EReEchoElement Element = ReEchoWeaponRuntime::ResolveProjectileElement(
			    true, EReEchoElement::Water, AttackSequence, ProjectileIndex);
			TestTrue(TEXT("Prism projectile resolves to one of the four production elements"),
			         Element == EReEchoElement::Water || Element == EReEchoElement::Flame ||
			             Element == EReEchoElement::Lightning || Element == EReEchoElement::Grass);
			TestEqual(TEXT("Prism projectile element is deterministic for replay"),
			          ReEchoWeaponRuntime::ResolveProjectileElement(
			              true, EReEchoElement::None, AttackSequence, ProjectileIndex),
			          Element);
			bObservedDifferentElementsWithinOneAttack |= Element != First;
		}
	}
	TestTrue(TEXT("Multishot projectiles no longer share one attack-level prism element"),
	         bObservedDifferentElementsWithinOneAttack);
	TestEqual(TEXT("Non-prism projectiles keep the attack element"),
	          ReEchoWeaponRuntime::ResolveProjectileElement(false, EReEchoElement::Flame, 1, 2),
	          EReEchoElement::Flame);
	return true;
}

bool FReEchoWeaponGemDamageCoefficientMatrixTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!TestTrue(TEXT("Damage matrix loads the production snapshot"), Snapshot.IsValid()))
	{
		return false;
	}

	struct FWeaponCase
	{
		FName WeaponId;
		float DamageCoefficient;
	};

	const TArray<FWeaponCase> WeaponCases = {
	    {TEXT("W_J_01"), 1.0f}, {TEXT("W_J_04"), 0.6f}, {TEXT("W_J_08"), 1.5f}, {TEXT("W_J_09"), 0.6f}};

	struct FCoreCase
	{
		FName PartId;
		EReEchoElement ExpectedElement;
		bool bUsesElementalAttack;
	};

	const TArray<FCoreCase> CoreCases = {{TEXT("P_CORE_PRIMORDIAL"), EReEchoElement::None, false},
	                                     {TEXT("P_CORE_FLAME"), EReEchoElement::Flame, true},
	                                     {TEXT("P_CORE_PRISM"), EReEchoElement::None, true}};

	FReEchoWeaponWorldFixture Fixture;
	UReEchoCombatantComponent* Combatant = nullptr;
	AActor* Owner = Fixture.SpawnWeaponOwner(FVector::ZeroVector, Combatant);
	for (const FWeaponCase& WeaponCase : WeaponCases)
	{
		for (const FCoreCase& CoreCase : CoreCases)
		{
			FReEchoBuildSnapshot Build = MakeBuild(*Snapshot, WeaponCase.WeaponId);
			SetBuildStats(Build, Combatant->Stats);
			FString Error;
			const FString CaseLabel =
			    FString::Printf(TEXT("%s + %s"), *WeaponCase.WeaponId.ToString(), *CoreCase.PartId.ToString());
			if (!TestTrue(*FString::Printf(TEXT("%s equips"), *CaseLabel),
			              ReEchoWeaponRuntime::TryEquipParts(*Snapshot, Build, {CoreCase.PartId}, Build, Error)))
			{
				AddError(FString::Printf(TEXT("%s: %s"), *CaseLabel, *Error));
				continue;
			}
			FReEchoEffectiveWeaponDefinition Effective;
			if (!TestTrue(*FString::Printf(TEXT("%s compiles"), *CaseLabel),
			              ReEchoWeaponRuntime::BuildEffectiveWeaponDefinition(*Snapshot, Build, Effective, Error)))
			{
				AddError(FString::Printf(TEXT("%s: %s"), *CaseLabel, *Error));
				continue;
			}
			FReEchoWeaponLogic Logic;
			TestTrue(*FString::Printf(TEXT("%s initializes logic"), *CaseLabel),
			         Logic.Initialize(ReEchoWeaponRuntime::CompileLogicDefinition(Effective)));
			FReEchoWeaponAttackCommit Commit;
			if (!TestTrue(*FString::Printf(TEXT("%s commits"), *CaseLabel),
			              Logic.TryCommitBasicAttack(Owner, Combatant->Stats, Commit)))
			{
				continue;
			}

			if (CoreCase.PartId == TEXT("P_CORE_PRISM"))
			{
				TestTrue(*FString::Printf(TEXT("%s resolves a deterministic element"), *CaseLabel),
				         Commit.Element != EReEchoElement::None);
			}
			else
			{
				TestEqual(*FString::Printf(TEXT("%s selects the expected damage type"), *CaseLabel),
				          Commit.Element,
				          CoreCase.ExpectedElement);
			}
			const float Attack =
			    CoreCase.bUsesElementalAttack ? Combatant->Stats.ElementalAttack : Combatant->Stats.PhysicalAttack;
			TestTrue(*FString::Printf(TEXT("%s uses one effective coefficient"), *CaseLabel),
			         FMath::IsNearlyEqual(Commit.RawDamage, Attack * WeaponCase.DamageCoefficient, 0.001f));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponLockAndPersistenceTest,
                                 "ReEcho.Weapons.RunLockAndEquipmentSnapshotParity",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponLockAndPersistenceTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	FString Error;
	TestTrue(TEXT("Run equips compatible core"), Run->TryEquipParts({TEXT("P_CORE_FLAME")}, Error));
	TestEqual(TEXT("Equipped run has recomputed core attack speed"), Run->CurrentBuild.Stats.AttackSpeed, 1.0f);

	UReEchoRunSaveGame* Save = Run->CreateSaveSnapshot();
	Save->SaveVersion = 9;
	Save->OwnedPartIds.Reset();
	UGameInstance* RestoreGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RestoreRun = NewObject<UReEchoRunSubsystem>(RestoreGameInstance);
	TestTrue(TEXT("Equipment snapshot restores with authoritative base"), RestoreRun->RestoreSaveSnapshot(*Save));
	TestTrue(TEXT("Restored build retains authoritative equipment base"), RestoreRun->CurrentBuild.bHasEquipmentBase);
	TestEqual(TEXT("Restored build recomputes core attack speed"), RestoreRun->CurrentBuild.Stats.AttackSpeed, 1.0f);
	TestTrue(TEXT("v9 equipment migrates into v10 part ownership"),
	         RestoreRun->OwnedPartIds.Contains(TEXT("P_CORE_FLAME")));

	UReEchoRecorderComponent* Recorder = NewObject<UReEchoRecorderComponent>();
	Recorder->BeginRecording(1, TEXT("WeaponTest"), 77, Run->CurrentBuild);
	TestEqual(
	    TEXT("Recorder preserves equipped part count"), Recorder->GetRecording().BuildSnapshot.EquippedParts.Num(), 1);
	TestEqual(TEXT("Recorder preserves equipment base attack speed"),
	          Recorder->GetRecording().BuildSnapshot.EquipmentBaseStats.AttackSpeed,
	          1.0f);

	TestEqual(TEXT("Run keeps the pre-run WeaponId"), Run->CurrentBuild.WeaponId, FName(TEXT("W_J_01")));
	TestEqual(TEXT("Run keeps all compatible equipped parts"), Run->CurrentBuild.EquippedParts.Num(), 1);
	TestEqual(TEXT("Locked weapon keeps recomputed attack speed"), Run->CurrentBuild.Stats.AttackSpeed, 1.0f);

	FReEchoWeaponWorldFixture Fixture;
	UReEchoCombatantComponent* Combatant = nullptr;
	AActor* Owner = Fixture.SpawnWeaponOwner(FVector::ZeroVector, Combatant);
	AReEchoWeaponActor* Weapon = Fixture.World->SpawnActor<AReEchoWeaponActor>();
	Weapon->SetOwner(Owner);
	Weapon->InitializeWeapon(&RestoreRun->CurrentBuild, Snapshot);
	TestTrue(TEXT("Echo/player shared actor switch policy succeeds"), Weapon->SelectWeaponById(TEXT("W_J_01")));
	TestEqual(TEXT("Actor switch retains compatible core only"), Weapon->GetBuildSnapshot().EquippedParts.Num(), 1);
	const FReEchoBuildSnapshot ActorBeforeFailure = Weapon->GetBuildSnapshot();
	TestFalse(TEXT("Actor unknown switch fails atomically"), Weapon->SelectWeaponById(TEXT("W_UNKNOWN")));
	TestEqual(
	    TEXT("Actor failure preserves WeaponId"), Weapon->GetBuildSnapshot().WeaponId, ActorBeforeFailure.WeaponId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponDomainRevisionRuntimeTest,
                                 "ReEcho.Weapons.DomainRevisionRejectsChangedTablesAndPinsActiveRun",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponDomainRevisionRuntimeTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> OldSnapshot = FReEchoCsvDataRegistry::GetSnapshot();
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_DIAMOND"), TEXT("W_J_08"));
	const FString OldRevision = Run->CurrentBuild.WeaponDomainRevision;
	UReEchoRunSaveGame* Save = Run->CreateSaveSnapshot();

	FReEchoWeaponWorldFixture Fixture;
	UReEchoCombatantComponent* Combatant = nullptr;
	AActor* Owner = Fixture.SpawnWeaponOwner(FVector::ZeroVector, Combatant);
	FReEchoBuildSnapshot ActorBuild = Run->CurrentBuild;
	SetBuildStats(ActorBuild, Combatant->Stats);
	AReEchoWeaponActor* Weapon = Fixture.World->SpawnActor<AReEchoWeaponActor>();
	Weapon->SetOwner(Owner);
	Weapon->InitializeWeapon(&ActorBuild, Run->GetRunDataSnapshot());

	const FString ModifiedDir = AssembleModifiedCsvDirectory(TEXT("weapons.csv"),
	                                                         TEXT("Pattern.BowShot,1.00,1.00,1000,0,1,0,0,"
	                                                              "1,true,RuntimeCompatibility,8,"),
	                                                         TEXT("Pattern.BowShot,1.01,1.00,1000,0,1,0,0,"
	                                                              "1,true,RuntimeCompatibility,8,"));
	const FReEchoCsvLoadResult PublishResult = FReEchoCsvDataRegistry::LoadAndPublishFromDirectory(ModifiedDir);
	if (!TestTrue(TEXT("Modified CSV publishes"), PublishResult.bSuccess))
	{
		AddError(PublishResult.FormatIssues());
		return false;
	}
	TestNotEqual(TEXT("Weapon domain revision changes after weapon table edit"),
	             FReEchoCsvDataRegistry::GetSnapshot()->WeaponDomainRevision,
	             OldRevision);
	TestEqual(TEXT("Active run keeps pinned weapon domain revision"),
	          Run->GetRunDataSnapshot()->WeaponDomainRevision,
	          OldRevision);
	TestTrue(TEXT("Pinned weapon actor executes with old snapshot"), Weapon->ExecuteBasicAttack(Combatant));
	TestEqual(
	    TEXT("Pinned actor keeps old projectile count after global republish"), CountProjectiles(Fixture.World), 1);

	FReEchoWeaponWorldFixture ConsumerFixture;
	AReEchoPlayerPawn* Player = ConsumerFixture.World->SpawnActor<AReEchoPlayerPawn>();
	TestTrue(TEXT("New player accepts active run pinned snapshot"),
	         Player->InitializeWeaponFromBuild(Run->CurrentBuild, Run->GetRunDataSnapshot()));
	TestEqual(TEXT("New player remains on active run revision after global republish"),
	          Player->GetPinnedWeaponDomainRevision(),
	          OldRevision);

	FReEchoRecording Recording;
	Recording.Id = FGuid::NewGuid();
	Recording.BuildSnapshot = Run->CurrentBuild;
	Recording.Positions.Add({0.0f, FVector::ZeroVector});
	Recording.Positions.Add({1.0f, FVector::ZeroVector});
	Recording.Duration = 1.0f;
	AReEchoEchoActor* Echo = ConsumerFixture.World->SpawnActor<AReEchoEchoActor>();
	TestTrue(TEXT("New echo accepts active run pinned snapshot"),
	         Echo->InitializeEcho(Recording, 1.0f, Run->GetRunDataSnapshot()));
	TestEqual(TEXT("New echo remains on active run revision after global republish"),
	          Echo->GetPinnedWeaponDomainRevision(),
	          OldRevision);
	TestEqual(
	    TEXT("New player and echo share pinned WeaponId"), Echo->GetEquippedWeaponId(), Run->CurrentBuild.WeaponId);
	ConsumerFixture.SpawnEnemy(FVector(900.0f, 0.0f, 0.0f), 20, 1000.0f);
	Echo->Tick(0.01f);
	TestEqual(TEXT("New echo executes old pinned projectile count after global republish"),
	          CountProjectiles(ConsumerFixture.World),
	          1);

	UGameInstance* RestoreGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RestoreRun = NewObject<UReEchoRunSubsystem>(RestoreGameInstance);
	TestFalse(TEXT("Old save is rejected after domain revision change"), RestoreRun->RestoreSaveSnapshot(*Save));
	TestEqual(TEXT("Rejected restore does not partially modify run phase"),
	          RestoreRun->Phase,
	          EReEchoRunPhase::CharacterSelect);

	const FString ModifiedEffectsDir = AssembleModifiedCsvDirectory(
	    TEXT("part_effects.csv"),
	    TEXT("PE_CORE_FLAME_CHANNEL,P_CORE_FLAME,1,OnEquip,WeaponDamageChannel,DamageChannel,Override,0,Part."
	         "CoreDamageChannel,None,None,Flame,0,0,0,Unique,true,,武器插槽C,5"),
	    TEXT("PE_CORE_FLAME_CHANNEL,P_CORE_FLAME,1,OnEquip,WeaponDamageChannel,DamageChannel,Override,1,Part."
	         "CoreDamageChannel,None,None,Flame,0,0,0,Unique,true,,武器插槽C,5"));
	const FReEchoCsvLoadResult EffectsPublishResult =
	    FReEchoCsvDataRegistry::LoadAndPublishFromDirectory(ModifiedEffectsDir);
	TestTrue(TEXT("Modified part effects publish"), EffectsPublishResult.bSuccess);
	TestNotEqual(TEXT("Weapon domain revision changes after part_effects edit"),
	             FReEchoCsvDataRegistry::GetSnapshot()->WeaponDomainRevision,
	             OldRevision);
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponStepLockScalesWithAttackSpeedTest,
                                 "ReEcho.Weapons.AttackStepLockScalesWithAttackSpeed",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponStepLockScalesWithAttackSpeedTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();

	FReEchoWeaponWorldFixture SlowFixture;
	UReEchoCombatantComponent* SlowCombatant = nullptr;
	AActor* SlowOwner = SlowFixture.SpawnWeaponOwner(FVector::ZeroVector, SlowCombatant);
	FReEchoBuildSnapshot SlowBuild = MakeBuild(*Snapshot, TEXT("W_J_01"));
	SetBuildStats(SlowBuild, SlowCombatant->Stats);
	SlowCombatant->Stats.AttackSpeed = 1.0f;
	AReEchoWeaponActor* SlowWeapon = SlowFixture.World->SpawnActor<AReEchoWeaponActor>();
	SlowWeapon->SetOwner(SlowOwner);
	SlowWeapon->InitializeWeapon(&SlowBuild, Snapshot);
	TestTrue(TEXT("slow attack executes"), SlowWeapon->ExecuteBasicAttack(SlowCombatant));
	const float SlowLock = SlowWeapon->GetStepLockRemaining();

	FReEchoWeaponWorldFixture FastFixture;
	UReEchoCombatantComponent* FastCombatant = nullptr;
	AActor* FastOwner = FastFixture.SpawnWeaponOwner(FVector::ZeroVector, FastCombatant);
	FReEchoBuildSnapshot FastBuild = MakeBuild(*Snapshot, TEXT("W_J_01"));
	SetBuildStats(FastBuild, FastCombatant->Stats);
	FastCombatant->Stats.AttackSpeed = 2.5f;
	AReEchoWeaponActor* FastWeapon = FastFixture.World->SpawnActor<AReEchoWeaponActor>();
	FastWeapon->SetOwner(FastOwner);
	FastWeapon->InitializeWeapon(&FastBuild, Snapshot);
	TestTrue(TEXT("fast attack executes"), FastWeapon->ExecuteBasicAttack(FastCombatant));
	const float FastLock = FastWeapon->GetStepLockRemaining();

	TestTrue(TEXT("step lock scales inversely with attack speed"),
	         FMath::IsNearlyEqual(SlowLock, FastLock * 2.5f, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponRuneCatalogCoverageTest,
                                 "ReEcho.Weapons.Runes.CatalogAndHandlerCoverage",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponRuneCatalogCoverageTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!TestTrue(TEXT("Production data snapshot loads"), Snapshot.IsValid()))
	{
		return false;
	}

	const TArray<TPair<FName, FName>> NewRuneWeapons = {
	    {TEXT("P_BOW_SPLIT_ARROWHEAD"), TEXT("W_J_08")},
	    {TEXT("P_BOW_EXPLOSIVE_ARROWHEAD"), TEXT("W_J_08")},
	    {TEXT("P_BOW_PIERCING_ARROWHEAD"), TEXT("W_J_08")},
	    {TEXT("P_BOW_CRITBLEED_ARROWHEAD"), TEXT("W_J_08")},
	    {TEXT("P_BOW_MULTISHOT_ARROWHEAD"), TEXT("W_J_08")},
	    {TEXT("P_BOW_KILLSHARD_ARROWHEAD"), TEXT("W_J_08")},
	    {TEXT("P_BOW_KILLHASTE_BOWSTRING"), TEXT("W_J_08")},
	    {TEXT("P_BOW_COMBOHASTE_BOWSTRING"), TEXT("W_J_08")},
	    {TEXT("P_SCYTHE_GROUPGROWTH_ROTARYBLADE"), TEXT("W_J_04")},
	    {TEXT("P_SCYTHE_LIFESTEAL_ROTARYBLADE"), TEXT("W_J_04")},
	    {TEXT("P_SCYTHE_STUN_ROTARYBLADE"), TEXT("W_J_04")},
	    {TEXT("P_SCYTHE_BLEED_ROTARYBLADE"), TEXT("W_J_04")},
	    {TEXT("P_SCYTHE_OUTERRING_ROTARYBLADE"), TEXT("W_J_04")},
	    {TEXT("P_SCYTHE_MOVESTACK_GRIP"), TEXT("W_J_04")},
	    {TEXT("P_SCYTHE_GROUPINVULN_GRIP"), TEXT("W_J_04")},
	    {TEXT("P_SCYTHE_ATTACKSTACK_GRIP"), TEXT("W_J_04")},
	    {TEXT("P_SCYTHE_THROWRECALL_GRIP"), TEXT("W_J_04")},
	    {TEXT("P_LONGSWORD_GROUPGROWTH_SWORDBLADE"), TEXT("W_J_01")},
	    {TEXT("P_LONGSWORD_HITSHARD_SWORDBLADE"), TEXT("W_J_01")},
	    {TEXT("P_LONGSWORD_CRITBLEED_SWORDBLADE"), TEXT("W_J_01")},
	    {TEXT("P_LONGSWORD_METEOR_SWORDBLADE"), TEXT("W_J_01")},
	    {TEXT("P_LONGSWORD_MOVESTACK_GRIP"), TEXT("W_J_01")},
	    {TEXT("P_LONGSWORD_ATTACKSTACK_GRIP"), TEXT("W_J_01")},
	    {TEXT("P_LONGSWORD_STUN_GRIP"), TEXT("W_J_01")},
	    {TEXT("P_GUN_CHARGED_MUZZLE"), TEXT("W_J_09")},
	    {TEXT("P_GUN_EXPLOSIVE_MUZZLE"), TEXT("W_J_09")},
	    {TEXT("P_GUN_PIERCING_MUZZLE"), TEXT("W_J_09")},
	    {TEXT("P_GUN_BLEED_MUZZLE"), TEXT("W_J_09")},
	    {TEXT("P_GUN_LIFESTEAL_GUNACTION"), TEXT("W_J_09")},
	    {TEXT("P_GUN_HITSHARD_GUNACTION"), TEXT("W_J_09")},
	    {TEXT("P_GUN_ATTACKSTACK_GUNACTION"), TEXT("W_J_09")},
	};
	TestEqual(TEXT("Four-weapon roster contains exactly 31 completed Plan76 runes"), NewRuneWeapons.Num(), 31);

	int32 ActiveVisibleRuneCount = 0;
	for (const TPair<FName, FReEchoCsvPartRow>& Pair : Snapshot->Parts)
	{
		const FReEchoCsvPartRow& Part = Pair.Value;
		if (Part.bEnabled && Part.SourceRow >= 2 && Part.SourceRow <= 56)
		{
			++ActiveVisibleRuneCount;
		}
	}
	TestEqual(TEXT("Four-weapon roster contains 46 visible production runes"), ActiveVisibleRuneCount, 46);

	for (const TPair<FName, FName>& RuneWeapon : NewRuneWeapons)
	{
		const FName PartId = RuneWeapon.Key;
		const FReEchoCsvPartRow* Part = Snapshot->Parts.Find(PartId);
		if (!TestNotNull(*FString::Printf(TEXT("%s exists"), *PartId.ToString()), Part))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("%s is enabled"), *PartId.ToString()), Part->bEnabled);
		TestTrue(*FString::Printf(TEXT("%s is shop-enabled"), *PartId.ToString()), Part->bShopEnabled);
		TestEqual(*FString::Printf(TEXT("%s is implemented"), *PartId.ToString()),
		          Part->ImplementationStatus,
		          FName(TEXT("Implemented")));
		TestTrue(*FString::Printf(TEXT("%s has effect rows"), *PartId.ToString()), !Part->Effects.IsEmpty());

		FReEchoBuildSnapshot Build = MakeBuild(*Snapshot, RuneWeapon.Value);
		FReEchoBuildSnapshot EquippedBuild;
		FString Error;
		if (!TestTrue(*FString::Printf(TEXT("%s equips"), *PartId.ToString()),
		              ReEchoWeaponRuntime::TryEquipParts(*Snapshot, Build, {PartId}, EquippedBuild, Error)))
		{
			AddError(Error);
			continue;
		}
		FReEchoEffectiveWeaponDefinition Definition;
		if (!TestTrue(*FString::Printf(TEXT("%s compiles"), *PartId.ToString()),
		              ReEchoWeaponRuntime::BuildEffectiveWeaponDefinition(*Snapshot, EquippedBuild, Definition, Error)))
		{
			AddError(Error);
			continue;
		}
		for (const FReEchoCsvPartEffectRow& Effect : Part->Effects)
		{
			if (Effect.bEnabled && Effect.EffectKind == TEXT("UniqueBehavior") &&
			    Effect.Target == TEXT("RuntimeBehavior"))
			{
				TestTrue(*FString::Printf(TEXT("%s has a closed runtime handler"), *Effect.BehaviorId.ToString()),
				         ReEchoWeaponRuntime::IsRuntimeRuneBehaviorSupported(Effect.BehaviorId));
				TestTrue(*FString::Printf(TEXT("%s is compiled into the equipped definition"), *Effect.Id.ToString()),
				         Definition.RuneEffects.ContainsByPredicate(
				             [&Effect](const FReEchoWeaponRuneEffectSpec& Spec)
				             {
					             return Spec.PartId == Effect.PartId && Spec.BehaviorId == Effect.BehaviorId;
				             }));
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponRuneStaticStepCompilationTest,
                                 "ReEcho.Weapons.Runes.StaticEffectsReachAttackSteps",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponRuneStaticStepCompilationTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	auto CompilePart = [this, &Snapshot](const FName WeaponId,
	                                     const FName PartId,
	                                     FReEchoBuildSnapshot& OutBuild,
	                                     FReEchoEffectiveWeaponDefinition& OutDefinition)
	{
		const FReEchoBuildSnapshot BaseBuild = MakeBuild(*Snapshot, WeaponId);
		FString Error;
		if (!TestTrue(*FString::Printf(TEXT("%s equips"), *PartId.ToString()),
		              ReEchoWeaponRuntime::TryEquipParts(*Snapshot, BaseBuild, {PartId}, OutBuild, Error)))
		{
			AddError(Error);
			return false;
		}
		if (!TestTrue(*FString::Printf(TEXT("%s compiles"), *PartId.ToString()),
		              ReEchoWeaponRuntime::BuildEffectiveWeaponDefinition(*Snapshot, OutBuild, OutDefinition, Error)))
		{
			AddError(Error);
			return false;
		}
		return true;
	};

	FReEchoBuildSnapshot LongSwordBuild;
	FReEchoEffectiveWeaponDefinition LongSword;
	if (CompilePart(TEXT("W_J_01"), TEXT("P_LONGSWORD_NARROWWIDE_SWORDBLADE"), LongSwordBuild, LongSword))
	{
		TestTrue(TEXT("Longsword rune scales the authored step range"),
		         FMath::IsNearlyEqual(LongSword.AttackSteps[0].RangeCm, 520.0f));
		TestTrue(TEXT("Longsword rune overrides the authored step arc"),
		         FMath::IsNearlyEqual(LongSword.AttackSteps[0].ArcDegrees, 120.0f));
	}

	FReEchoBuildSnapshot BowBuild;
	FReEchoEffectiveWeaponDefinition Bow;
	if (CompilePart(TEXT("W_J_08"), TEXT("P_BOW_MULTISHOT_ARROWHEAD"), BowBuild, Bow))
	{
		TestEqual(TEXT("Multishot reaches the authored projectile step"), Bow.AttackSteps[0].ProjectileCount, 3);
		TestTrue(TEXT("Multishot reaches the authored spread step"),
		         FMath::IsNearlyEqual(Bow.AttackSteps[0].ConcentrationDegrees, 60.0f));
	}

	FReEchoBuildSnapshot ExplosiveBuild;
	FReEchoEffectiveWeaponDefinition Explosive;
	if (CompilePart(TEXT("W_J_09"), TEXT("P_GUN_EXPLOSIVE_MUZZLE"), ExplosiveBuild, Explosive))
	{
		TestTrue(TEXT("Explosive rune reaches the authored projectile step"),
		         FMath::IsNearlyEqual(Explosive.AttackSteps[0].ExplosionRadiusCm, 100.0f));
	}

	FReEchoBuildSnapshot ChargedBuild;
	FReEchoEffectiveWeaponDefinition Charged;
	const FReEchoBuildSnapshot ChargedBase = MakeBuild(*Snapshot, TEXT("W_J_09"));
	if (CompilePart(TEXT("W_J_09"), TEXT("P_GUN_CHARGED_MUZZLE"), ChargedBuild, Charged))
	{
		TestTrue(TEXT("Charged gun implements -500% cadence as speed x 1/6"),
		         FMath::IsNearlyEqual(ChargedBuild.Stats.AttackSpeed, ChargedBase.Stats.AttackSpeed / 6.0f, 0.001f));
		TestTrue(TEXT("Charged gun applies +180% damage to the authored step"),
		         FMath::IsNearlyEqual(Charged.AttackSteps[0].DamageCoefficient, 1.12f, 0.001f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponRuneShardGrantTransactionTest,
                                 "ReEcho.Weapons.Runes.TimeShardGrantTransaction",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponRuneShardGrantTransactionTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->TimeShards = 5;
	TestFalse(TEXT("Non-positive pickup grants are rejected"), Run->GrantTimeShards(0));
	TestEqual(TEXT("Rejected grant is atomic"), Run->TimeShards, 5);
	TestTrue(TEXT("Positive pickup grants currency"), Run->GrantTimeShards(2));
	TestEqual(TEXT("Pickup grant applies exact amount"), Run->TimeShards, 7);
	Run->TimeShards = TNumericLimits<int32>::Max() - 1;
	TestTrue(TEXT("Overflowing pickup grant is accepted and clamped"), Run->GrantTimeShards(10));
	TestEqual(TEXT("Pickup grant never overflows"), Run->TimeShards, TNumericLimits<int32>::Max());
	TestFalse(TEXT("Already-saturated balance rejects an uncollectable pickup"), Run->GrantTimeShards(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponRuneDynamicHitHandlersTest,
                                 "ReEcho.Weapons.Runes.DynamicHitHandlers",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponRuneDynamicHitHandlersTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	FReEchoWeaponWorldFixture Fixture;
	UReEchoCombatantComponent* SourceCombatant = nullptr;
	AActor* Owner = Fixture.SpawnWeaponOwner(FVector::ZeroVector, SourceCombatant);
	const FReEchoStatBlock BaseStats = SourceCombatant->Stats;
	int32 EnemyIndex = 100;

	auto SpawnRuneWeapon = [&](const FName WeaponId, const FName PartId)
	{
		FReEchoBuildSnapshot Build = MakeBuild(*Snapshot, WeaponId);
		SetBuildStats(Build, BaseStats);
		FReEchoBuildSnapshot Equipped;
		FString Error;
		if (!TestTrue(*FString::Printf(TEXT("%s equips for handler proof"), *PartId.ToString()),
		              ReEchoWeaponRuntime::TryEquipParts(*Snapshot, Build, {PartId}, Equipped, Error)))
		{
			AddError(Error);
			return static_cast<AReEchoWeaponActor*>(nullptr);
		}
		SourceCombatant->InitializeFromStats(Equipped.Stats, true);
		AReEchoWeaponActor* Weapon = Fixture.World->SpawnActor<AReEchoWeaponActor>();
		Weapon->SetOwner(Owner);
		Weapon->InitializeWeapon(&Equipped, Snapshot);
		return Weapon;
	};
	auto MakeCommit = [&](const int64 Sequence)
	{
		FReEchoWeaponAttackCommit Commit;
		Commit.Attack.Source = Owner;
		Commit.Attack.Sequence = Sequence;
		Commit.Attack.SourceFaction = EReEchoCombatFaction::PlayerSide;
		Commit.RawDamage = 10.0f;
		Commit.RangeCm = 200.0f;
		return Commit;
	};
	auto MakeHit = [&](AReEchoEnemyActor* Target, const int64 Sequence)
	{
		FReEchoHitResolved Hit;
		Hit.Attack = MakeCommit(Sequence).Attack;
		Hit.Target = Target;
		Hit.RawDamage = 10.0f;
		Hit.AppliedDamage = 10.0f;
		Hit.DamageSource = EReEchoDamageSource::Player;
		Hit.HitLocation = Target->GetActorLocation();
		return Hit;
	};

	AReEchoEnemyActor* HealTarget = Fixture.SpawnEnemy(FVector(50.0f, 0.0f, 0.0f), EnemyIndex++, 1000.0f);
	AReEchoWeaponActor* HealWeapon = SpawnRuneWeapon(TEXT("W_J_04"), TEXT("P_SCYTHE_LIFESTEAL_ROTARYBLADE"));
	SourceCombatant->RestoreCurrentHealth(50.0f);
	auto HealContext = HealWeapon->BuildRuneAttackContextForTests(MakeCommit(1), SourceCombatant);
	HealWeapon->ProcessRuneHitForTests(HealContext, MakeHit(HealTarget, 1));
	TestTrue(TEXT("HealOnHit restores 1% maximum health per effective target"),
	         FMath::IsNearlyEqual(SourceCombatant->CurrentHealth, 51.0f));

	AReEchoEnemyActor* CriticalBleedTarget = Fixture.SpawnEnemy(FVector(60.0f, 0.0f, 0.0f), EnemyIndex++, 1000.0f);
	AReEchoWeaponActor* CriticalBleedWeapon = SpawnRuneWeapon(TEXT("W_J_01"), TEXT("P_LONGSWORD_CRITBLEED_SWORDBLADE"));
	FReEchoHitResolved CriticalHit = MakeHit(CriticalBleedTarget, 2);
	CriticalHit.bCritical = true;
	CriticalBleedWeapon->ProcessRuneHitForTests(
	    CriticalBleedWeapon->BuildRuneAttackContextForTests(MakeCommit(2), SourceCombatant), CriticalHit);
	TestTrue(TEXT("Critical bleed handler applies Z_Bleeding"),
	         CriticalBleedTarget->GetCombatantComponent()->GetElementState().ActiveStatusUntilSeconds.Contains(
	             TEXT("Z_Bleeding")));

	AReEchoEnemyActor* ChanceBleedTarget = Fixture.SpawnEnemy(FVector(70.0f, 0.0f, 0.0f), EnemyIndex++, 1000.0f);
	AReEchoWeaponActor* ChanceBleedWeapon = SpawnRuneWeapon(TEXT("W_J_09"), TEXT("P_GUN_BLEED_MUZZLE"));
	for (int64 Sequence = 1; Sequence <= 100; ++Sequence)
	{
		ChanceBleedWeapon->ProcessRuneHitForTests(
		    ChanceBleedWeapon->BuildRuneAttackContextForTests(MakeCommit(Sequence), SourceCombatant),
		    MakeHit(ChanceBleedTarget, Sequence));
	}
	TestTrue(TEXT("Deterministic 15% hit sampling reaches the bleed handler"),
	         ChanceBleedTarget->GetCombatantComponent()->GetElementState().ActiveStatusUntilSeconds.Contains(
	             TEXT("Z_Bleeding")));

	AReEchoEnemyActor* StunTarget = Fixture.SpawnEnemy(FVector(80.0f, 0.0f, 0.0f), EnemyIndex++, 1000.0f);
	AReEchoWeaponActor* StunWeapon = SpawnRuneWeapon(TEXT("W_J_01"), TEXT("P_LONGSWORD_STUN_GRIP"));
	for (int64 Sequence = 1; Sequence <= 100; ++Sequence)
	{
		StunWeapon->ProcessRuneHitForTests(
		    StunWeapon->BuildRuneAttackContextForTests(MakeCommit(Sequence), SourceCombatant),
		    MakeHit(StunTarget, Sequence));
	}
	TestTrue(TEXT("Deterministic 20% hit sampling reaches the stun handler"),
	         StunTarget->GetCombatantComponent()->IsActionDisabled(Fixture.World->GetTimeSeconds() + 0.5f));

	AReEchoEnemyActor* ScytheBleedTarget = Fixture.SpawnEnemy(FVector(90.0f, 0.0f, 0.0f), EnemyIndex++, 1000.0f);
	AReEchoWeaponActor* ScytheBleedWeapon = SpawnRuneWeapon(TEXT("W_J_04"), TEXT("P_SCYTHE_BLEED_ROTARYBLADE"));
	FReEchoHitResolved NonCriticalScytheHit = MakeHit(ScytheBleedTarget, 3);
	ScytheBleedWeapon->ProcessRuneHitForTests(
	    ScytheBleedWeapon->BuildRuneAttackContextForTests(MakeCommit(3), SourceCombatant), NonCriticalScytheHit);
	TestFalse(TEXT("Scythe bleed does not fire on a non-critical hit"),
	          ScytheBleedTarget->GetCombatantComponent()->GetElementState().ActiveStatusUntilSeconds.Contains(
	              TEXT("Z_Bleeding")));
	FReEchoHitResolved CriticalScytheHit = MakeHit(ScytheBleedTarget, 4);
	CriticalScytheHit.bCritical = true;
	ScytheBleedWeapon->ProcessRuneHitForTests(
	    ScytheBleedWeapon->BuildRuneAttackContextForTests(MakeCommit(4), SourceCombatant), CriticalScytheHit);
	TestTrue(TEXT("Scythe bleed fires on a critical hit"),
	         ScytheBleedTarget->GetCombatantComponent()->GetElementState().ActiveStatusUntilSeconds.Contains(
	             TEXT("Z_Bleeding")));

	AReEchoEnemyActor* StatTarget = Fixture.SpawnEnemy(FVector(100.0f, 0.0f, 0.0f), EnemyIndex++, 1000.0f);
	AReEchoWeaponActor* MoveWeapon = SpawnRuneWeapon(TEXT("W_J_04"), TEXT("P_SCYTHE_MOVESTACK_GRIP"));
	MoveWeapon->ProcessRuneHitForTests(MoveWeapon->BuildRuneAttackContextForTests(MakeCommit(4), SourceCombatant),
	                                   MakeHit(StatTarget, 4));
	TestEqual(TEXT("MoveSpeedPerHit creates one independent timed layer"),
	          SourceCombatant->GetTransientStatStackCount(TEXT("P_SCYTHE_MOVESTACK_GRIP")),
	          1);
	AReEchoWeaponActor* AttackWeapon = SpawnRuneWeapon(TEXT("W_J_01"), TEXT("P_LONGSWORD_ATTACKSTACK_GRIP"));
	AttackWeapon->ProcessRuneHitForTests(AttackWeapon->BuildRuneAttackContextForTests(MakeCommit(5), SourceCombatant),
	                                     MakeHit(StatTarget, 5));
	TestEqual(TEXT("AttackSpeedPerHit creates one independent timed layer"),
	          SourceCombatant->GetTransientStatStackCount(TEXT("P_LONGSWORD_ATTACKSTACK_GRIP")),
	          1);

	AReEchoWeaponActor* ComboWeapon = SpawnRuneWeapon(TEXT("W_J_08"), TEXT("P_BOW_COMBOHASTE_BOWSTRING"));
	auto ComboContext = ComboWeapon->BuildRuneAttackContextForTests(MakeCommit(6), SourceCombatant);
	ComboWeapon->ProcessRuneOnAttackForTests(ComboContext);
	ComboWeapon->ProcessRuneOnAttackForTests(ComboContext);
	TestEqual(TEXT("Bow combo handler stacks attack and move speed"),
	          SourceCombatant->GetTransientStatStackCount(TEXT("P_BOW_COMBOHASTE_BOWSTRING")),
	          2);
	Fixture.Advance(1.01f);
	Fixture.Advance(0.01f);
	ComboWeapon->Tick(0.0f);
	TestEqual(TEXT("Bow combo loses one layer after one idle second"),
	          SourceCombatant->GetTransientStatStackCount(TEXT("P_BOW_COMBOHASTE_BOWSTRING")),
	          1);
	AReEchoWeaponActor* GunAttackWeapon = SpawnRuneWeapon(TEXT("W_J_09"), TEXT("P_GUN_ATTACKSTACK_GUNACTION"));
	GunAttackWeapon->ProcessRuneOnAttackForTests(
	    GunAttackWeapon->BuildRuneAttackContextForTests(MakeCommit(7), SourceCombatant));
	TestEqual(TEXT("Gun on-attack speed handler creates a timed layer"),
	          SourceCombatant->GetTransientStatStackCount(TEXT("P_GUN_ATTACKSTACK_GUNACTION")),
	          1);
	for (int32 StackIndex = 1; StackIndex < 35; ++StackIndex)
	{
		GunAttackWeapon->ProcessRuneOnAttackForTests(
		    GunAttackWeapon->BuildRuneAttackContextForTests(MakeCommit(7 + StackIndex), SourceCombatant));
	}
	TestEqual(TEXT("Gun on-attack speed has no implicit thirty-layer cap"),
	          SourceCombatant->GetTransientStatStackCount(TEXT("P_GUN_ATTACKSTACK_GUNACTION")),
	          35);
	AReEchoWeaponActor* GunMoveWeapon = SpawnRuneWeapon(TEXT("W_J_09"), TEXT("P_GUN_MOVESTACK_GUNACTION"));
	for (int32 StackIndex = 0; StackIndex < 35; ++StackIndex)
	{
		GunMoveWeapon->ProcessRuneOnAttackForTests(
		    GunMoveWeapon->BuildRuneAttackContextForTests(MakeCommit(50 + StackIndex), SourceCombatant));
	}
	TestEqual(TEXT("Gun on-attack move speed is implemented and has no implicit thirty-layer cap"),
	          SourceCombatant->GetTransientStatStackCount(TEXT("P_GUN_MOVESTACK_GUNACTION")),
	          35);

	auto CountShardPickups = [&]()
	{
		int32 Count = 0;
		for (TActorIterator<AReEchoTimeShardPickupActor> It(Fixture.World); It; ++It)
		{
			if (!It->IsActorBeingDestroyed())
			{
				++Count;
			}
		}
		return Count;
	};
	AReEchoWeaponActor* HitShardWeapon = SpawnRuneWeapon(TEXT("W_J_01"), TEXT("P_LONGSWORD_HITSHARD_SWORDBLADE"));
	auto HitShardContext = HitShardWeapon->BuildRuneAttackContextForTests(MakeCommit(8), SourceCombatant);
	for (int32 HitIndex = 0; HitIndex < 5; ++HitIndex)
	{
		HitShardWeapon->ProcessRuneHitForTests(HitShardContext, MakeHit(StatTarget, 8));
	}
	TestEqual(TEXT("Every-five-hits handler spawns one shard pickup"), CountShardPickups(), 1);
	AReEchoWeaponActor* KillShardWeapon = SpawnRuneWeapon(TEXT("W_J_08"), TEXT("P_BOW_KILLSHARD_ARROWHEAD"));
	FReEchoHitResolved KillHit = MakeHit(StatTarget, 9);
	KillHit.bKilled = true;
	KillShardWeapon->ProcessRuneHitForTests(
	    KillShardWeapon->BuildRuneAttackContextForTests(MakeCommit(9), SourceCombatant), KillHit);
	TestEqual(TEXT("Player kill handler spawns one additional shard pickup"), CountShardPickups(), 2);
	KillHit.DamageSource = EReEchoDamageSource::Echo;
	KillShardWeapon->ProcessRuneHitForTests(
	    KillShardWeapon->BuildRuneAttackContextForTests(MakeCommit(10), SourceCombatant), KillHit);
	TestEqual(TEXT("Echo kill handler cannot duplicate economy"), CountShardPickups(), 2);

	AReEchoWeaponActor* KillMoveWeapon = SpawnRuneWeapon(TEXT("W_J_08"), TEXT("P_BOW_KILLHASTE_BOWSTRING"));
	auto KillMoveContext = KillMoveWeapon->BuildRuneAttackContextForTests(MakeCommit(11), SourceCombatant);
	KillHit.DamageSource = EReEchoDamageSource::Player;
	KillMoveWeapon->ProcessRuneHitForTests(KillMoveContext, KillHit);
	KillMoveWeapon->ProcessRuneHitForTests(KillMoveContext, KillHit);
	TestEqual(TEXT("Kill move-speed handler remains stackable with independent five-second layers"),
	          SourceCombatant->GetTransientStatStackCount(TEXT("P_BOW_KILLHASTE_BOWSTRING")),
	          2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponRuneGroupOuterAndScytheTest,
                                 "ReEcho.Weapons.Runes.GroupOuterAndScytheHandlers",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponRuneGroupOuterAndScytheTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	FReEchoWeaponWorldFixture Fixture;
	UReEchoCombatantComponent* SourceCombatant = nullptr;
	AActor* Owner = Fixture.SpawnWeaponOwner(FVector::ZeroVector, SourceCombatant);
	const FReEchoStatBlock BaseStats = SourceCombatant->Stats;
	TArray<AReEchoEnemyActor*> GroupTargets;
	for (int32 Index = 0; Index < 7; ++Index)
	{
		GroupTargets.Add(Fixture.SpawnEnemy(FVector(1200.0f, 1000.0f + 100.0f * Index, 0.0f), 180 + Index, 1000.0f));
	}

	auto SpawnRuneWeapon = [&](const FName WeaponId, const FName PartId)
	{
		FReEchoBuildSnapshot Build = MakeBuild(*Snapshot, WeaponId);
		SetBuildStats(Build, BaseStats);
		FReEchoBuildSnapshot Equipped;
		FString Error;
		if (!TestTrue(*FString::Printf(TEXT("%s equips for group proof"), *PartId.ToString()),
		              ReEchoWeaponRuntime::TryEquipParts(*Snapshot, Build, {PartId}, Equipped, Error)))
		{
			AddError(Error);
			return static_cast<AReEchoWeaponActor*>(nullptr);
		}
		SourceCombatant->InitializeFromStats(Equipped.Stats, true);
		AReEchoWeaponActor* Weapon = Fixture.World->SpawnActor<AReEchoWeaponActor>();
		Weapon->SetOwner(Owner);
		Weapon->InitializeWeapon(&Equipped, Snapshot);
		return Weapon;
	};
	auto MakeCommit = [&](const int64 Sequence)
	{
		FReEchoWeaponAttackCommit Commit;
		Commit.Attack.Source = Owner;
		Commit.Attack.Sequence = Sequence;
		Commit.Attack.SourceFaction = EReEchoCombatFaction::PlayerSide;
		Commit.RawDamage = 10.0f;
		Commit.RangeCm = 200.0f;
		return Commit;
	};
	auto HasEnabledRuntimeEffect = [&](const FName PartId)
	{
		const FReEchoCsvPartRow* Part = Snapshot->Parts.Find(PartId);
		return Part && Part->bEnabled && Part->Effects.ContainsByPredicate(
		                                     [](const FReEchoCsvPartEffectRow& Effect)
		                                     {
			                                     return Effect.bEnabled;
		                                     });
	};

	AReEchoWeaponActor* RangeWeapon = SpawnRuneWeapon(TEXT("W_J_01"), TEXT("P_LONGSWORD_GROUPGROWTH_SWORDBLADE"));
	auto RangeContext = RangeWeapon->BuildRuneAttackContextForTests(MakeCommit(1), SourceCombatant);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		RangeContext->EffectiveHitTargets.Add(GroupTargets[Index]);
	}
	RangeWeapon->ProcessRuneAttackForTests(RangeContext);
	TestTrue(TEXT("Four-target group hit adds one additive +2% range layer"),
	         FMath::IsNearlyEqual(RangeWeapon->GetTimedRangeMultiplierForTests(), 1.02f));

	AReEchoWeaponActor* InvulnerableWeapon = SpawnRuneWeapon(TEXT("W_J_04"), TEXT("P_SCYTHE_GROUPINVULN_GRIP"));
	auto InvulnerableContext = InvulnerableWeapon->BuildRuneAttackContextForTests(MakeCommit(2), SourceCombatant);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		InvulnerableContext->EffectiveHitTargets.Add(GroupTargets[Index]);
	}
	InvulnerableWeapon->ProcessRuneAttackForTests(InvulnerableContext);
	TestTrue(TEXT("Four-target group hit grants the configured half-second invulnerability"),
	         SourceCombatant->IsTimedInvulnerable(Fixture.World->GetTimeSeconds() + 0.49f));

	if (HasEnabledRuntimeEffect(TEXT("P_LONGSWORD_METEOR_SWORDBLADE")))
	{
		AReEchoEnemyActor* MeteorCenter = Fixture.SpawnEnemy(FVector(100.0f, 0.0f, 0.0f), 200, 1000.0f);
		AReEchoEnemyActor* MeteorNeighbor = Fixture.SpawnEnemy(FVector(100.0f, 100.0f, 0.0f), 201, 1000.0f);
		AReEchoWeaponActor* MeteorWeapon =
		    SpawnRuneWeapon(TEXT("W_J_01"), TEXT("P_LONGSWORD_METEOR_SWORDBLADE"));
		auto MeteorContext = MeteorWeapon->BuildRuneAttackContextForTests(MakeCommit(3), SourceCombatant);
		MeteorContext->EffectiveHitTargets.Add(MeteorCenter);
		for (int32 Index = 0; Index < 6; ++Index)
		{
			MeteorContext->EffectiveHitTargets.Add(GroupTargets[Index]);
		}
		const float MeteorCenterBefore = WeaponEnemyHealth(MeteorCenter);
		const float MeteorNeighborBefore = WeaponEnemyHealth(MeteorNeighbor);
		MeteorWeapon->ProcessRuneAttackForTests(MeteorContext);
		TestTrue(TEXT("Seven-target threshold drops a non-recursive meteor on the nearest hit target"),
		         WeaponEnemyHealth(MeteorCenter) < MeteorCenterBefore);
		TestTrue(TEXT("Meteor uses the configured 1.5m explosion radius"),
		         WeaponEnemyHealth(MeteorNeighbor) < MeteorNeighborBefore);
	}

	AReEchoEnemyActor* InnerTarget = Fixture.SpawnEnemy(FVector(40.0f, 0.0f, 0.0f), 202, 1000.0f);
	AReEchoEnemyActor* OuterTarget = Fixture.SpawnEnemy(FVector(160.0f, 0.0f, 0.0f), 203, 1000.0f);
	AReEchoWeaponActor* OuterWeapon = SpawnRuneWeapon(TEXT("W_J_04"), TEXT("P_SCYTHE_OUTERRING_ROTARYBLADE"));
	const FReEchoWeaponAttackCommit OuterCommit = MakeCommit(4);
	auto OuterContext = OuterWeapon->BuildRuneAttackContextForTests(OuterCommit, SourceCombatant);
	const FReEchoHitResolved InnerResult = OuterWeapon->ApplyRuneDamageForTests(
	    *InnerTarget, OuterCommit, FVector::ZeroVector, SourceCombatant, OuterContext);
	const FReEchoHitResolved OuterResult = OuterWeapon->ApplyRuneDamageForTests(
	    *OuterTarget, OuterCommit, FVector::ZeroVector, SourceCombatant, OuterContext);
	TestTrue(TEXT("Inner half keeps base damage"), FMath::IsNearlyEqual(InnerResult.AppliedDamage, 10.0f));
	TestTrue(TEXT("Outer half receives exactly +40% damage"), FMath::IsNearlyEqual(OuterResult.AppliedDamage, 14.0f));

	if (HasEnabledRuntimeEffect(TEXT("P_SCYTHE_THROWRECALL_GRIP")))
	{
		AReEchoEnemyActor* ThrowTarget = Fixture.SpawnEnemy(FVector(350.0f, 0.0f, 0.0f), 204, 1000.0f);
		AReEchoWeaponActor* ThrowWeapon = SpawnRuneWeapon(TEXT("W_J_04"), TEXT("P_SCYTHE_THROWRECALL_GRIP"));
		UReEchoCombatEventsComponent* ThrowEvents =
		    ThrowWeapon->GetOwner()->FindComponentByClass<UReEchoCombatEventsComponent>();
		if (!TestNotNull(TEXT("Active weapon owner has combat presentation events"), ThrowEvents))
		{
			return false;
		}
		const float ThrowBefore = WeaponEnemyHealth(ThrowTarget);
		TestTrue(TEXT("First active input starts the data-authored scythe throw"),
		         ThrowWeapon->TryActiveAttack(SourceCombatant));
		TestEqual(TEXT("Manually triggered active attack publishes one presentation commit"),
		          ThrowEvents->GetAttackCommittedPublishCountForTests(),
		          1);
		TestEqual(TEXT("Published active event keeps the committed scythe attack pattern"),
		          ThrowEvents->GetLastAttackCommittedEventForTests().AttackPatternId,
		          FName(TEXT("Pattern.ScytheSweep")));
		TestTrue(TEXT("Scythe remains in a thrown state"), ThrowWeapon->IsScytheThrownForTests());
		ThrowWeapon->AdvanceScytheThrowForTests(0.6f);
		const float AfterTravel = WeaponEnemyHealth(ThrowTarget);
		TestTrue(TEXT("Travel contact applies the configured 60% attack damage"), AfterTravel < ThrowBefore);
		ThrowWeapon->AdvanceScytheThrowForTests(1.0f);
		TestTrue(TEXT("Stationary scythe ticks at inherited attack speed for configured 20% damage"),
		         WeaponEnemyHealth(ThrowTarget) < AfterTravel);
		TestTrue(TEXT("Second active input recalls without creating another attack"),
		         ThrowWeapon->TryActiveAttack(SourceCombatant));
		TestEqual(TEXT("Recall does not publish a duplicate attack presentation event"),
		          ThrowEvents->GetAttackCommittedPublishCountForTests(),
		          1);
		TestFalse(TEXT("Recall clears the thrown state"), ThrowWeapon->IsScytheThrownForTests());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponRuneProjectileCombinationTest,
                                 "ReEcho.Weapons.Runes.ProjectileSplitPierceExplosion",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponRuneProjectileCombinationTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();

	FReEchoWeaponWorldFixture SplitFixture;
	UReEchoCombatantComponent* SplitCombatant = nullptr;
	AActor* SplitOwner = SplitFixture.SpawnWeaponOwner(FVector::ZeroVector, SplitCombatant);
	FReEchoBuildSnapshot SplitBuild = MakeBuild(*Snapshot, TEXT("W_J_08"));
	SetBuildStats(SplitBuild, SplitCombatant->Stats);
	SplitBuild.CardState.OwnedCardIds.Add(TEXT("G_3_22"));
	FString Error;
	TestTrue(TEXT("Double-slot card allows split and explosive arrowheads in the same slot"),
	         ReEchoWeaponRuntime::TryEquipParts(*Snapshot,
	                                            SplitBuild,
	                                            {TEXT("P_BOW_SPLIT_ARROWHEAD"), TEXT("P_BOW_EXPLOSIVE_ARROWHEAD")},
	                                            SplitBuild,
	                                            Error));
	FReEchoEffectiveWeaponDefinition SplitDefinition;
	TestTrue(TEXT("Same-slot split and explosive arrowheads compile together"),
	         ReEchoWeaponRuntime::BuildEffectiveWeaponDefinition(*Snapshot, SplitBuild, SplitDefinition, Error));
	AReEchoWeaponActor* SplitWeapon = SplitFixture.World->SpawnActor<AReEchoWeaponActor>();
	SplitWeapon->SetOwner(SplitOwner);
	SplitWeapon->InitializeWeapon(&SplitBuild, Snapshot);
	AReEchoEnemyActor* Direct = SplitFixture.SpawnEnemy(FVector(100.0f, 0.0f, 0.0f), 300, 1000.0f);
	SplitFixture.SpawnEnemy(FVector(200.0f, 0.0f, 0.0f), 301, 1000.0f);
	SplitFixture.SpawnEnemy(FVector(250.0f, 50.0f, 0.0f), 302, 1000.0f);
	SplitFixture.SpawnEnemy(FVector(300.0f, -50.0f, 0.0f), 303, 1000.0f);
	SplitFixture.SpawnEnemy(FVector(350.0f, 0.0f, 0.0f), 304, 1000.0f);
	FReEchoWeaponAttackCommit SplitCommit;
	SplitCommit.Attack.Source = SplitOwner;
	SplitCommit.Attack.Sequence = 1;
	SplitCommit.Attack.SourceFaction = EReEchoCombatFaction::PlayerSide;
	SplitCommit.RawDamage = 10.0f;
	SplitCommit.RangeCm = 1000.0f;
	SplitCommit.ExplosionRadiusCm = SplitDefinition.AttackSteps[0].ExplosionRadiusCm;
	auto SplitContext = SplitWeapon->BuildRuneAttackContextForTests(SplitCommit, SplitCombatant);
	FReEchoHitResolved SplitHit;
	SplitHit.Attack = SplitCommit.Attack;
	SplitHit.Target = Direct;
	SplitHit.AppliedDamage = 10.0f;
	SplitHit.DamageSource = EReEchoDamageSource::Player;
	SplitHit.HitLocation = Direct->GetActorLocation();
	FReEchoProjectileSnapshot SplitSnapshot;
	SplitSnapshot.ProjectileId.Value = FGuid::NewGuid();
	SplitWeapon->HandleProjectileResolved(SplitContext, SplitSnapshot, SplitHit, true);
	TestEqual(TEXT("Split chooses at most three other nearest valid targets"), CountProjectiles(SplitFixture.World), 3);
	for (TActorIterator<AReEchoProjectileActor> It(SplitFixture.World); It; ++It)
	{
		TestTrue(TEXT("Split children inherit the same-slot explosive arrowhead"),
		         FMath::IsNearlyEqual(It->GetExplosionRadiusCm(), 100.0f));
	}
	SplitWeapon->HandleProjectileResolved(SplitContext, SplitSnapshot, SplitHit, true);
	TestEqual(TEXT("The same projectile cannot split recursively or twice"), CountProjectiles(SplitFixture.World), 3);

	FReEchoWeaponWorldFixture PierceFixture;
	UReEchoCombatantComponent* PierceCombatant = nullptr;
	AActor* PierceOwner = PierceFixture.SpawnWeaponOwner(FVector::ZeroVector, PierceCombatant);
	FReEchoBuildSnapshot PierceBuild = MakeBuild(*Snapshot, TEXT("W_J_09"));
	SetBuildStats(PierceBuild, PierceCombatant->Stats);
	TestTrue(TEXT("Pierce rune equips"),
	         ReEchoWeaponRuntime::TryEquipParts(
	             *Snapshot, PierceBuild, {TEXT("P_GUN_PIERCING_MUZZLE")}, PierceBuild, Error));
	PierceCombatant->InitializeFromStats(PierceBuild.Stats, true);
	PierceCombatant->Stats.RoleId = TEXT("Hunter");
	PierceCombatant->Stats.CriticalRate = 1.0f;
	AReEchoWeaponActor* PierceWeapon = PierceFixture.World->SpawnActor<AReEchoWeaponActor>();
	PierceWeapon->SetOwner(PierceOwner);
	PierceWeapon->InitializeWeapon(&PierceBuild, Snapshot);
	AReEchoEnemyActor* First = PierceFixture.SpawnEnemy(FVector(200.0f, 0.0f, 0.0f), 310, 1000.0f);
	AReEchoEnemyActor* Second = PierceFixture.SpawnEnemy(FVector(450.0f, 0.0f, 0.0f), 311, 1000.0f);
	TestTrue(TEXT("Critical projectile attack executes"), PierceWeapon->ExecuteBasicAttack(PierceCombatant));
	TickProjectiles(PierceFixture.World, 0.3f);
	TickProjectiles(PierceFixture.World, 0.3f);
	TestTrue(TEXT("Critical projectile damages the first target"), WeaponEnemyHealth(First) < 1000.0f);
	TestTrue(TEXT("Critical projectile continues into the second target"), WeaponEnemyHealth(Second) < 1000.0f);

	FReEchoWeaponWorldFixture CombinationFixture;
	UReEchoCombatantComponent* CombinationCombatant = nullptr;
	AActor* CombinationOwner = CombinationFixture.SpawnWeaponOwner(FVector::ZeroVector, CombinationCombatant);
	AReEchoEnemyActor* ContactOne = CombinationFixture.SpawnEnemy(FVector(200.0f, 0.0f, 0.0f), 320, 1000.0f);
	AReEchoEnemyActor* SplashOne = CombinationFixture.SpawnEnemy(FVector(200.0f, 80.0f, 0.0f), 321, 1000.0f);
	AReEchoEnemyActor* ContactTwo = CombinationFixture.SpawnEnemy(FVector(500.0f, 0.0f, 0.0f), 322, 1000.0f);
	AReEchoEnemyActor* SplashTwo = CombinationFixture.SpawnEnemy(FVector(500.0f, 80.0f, 0.0f), 323, 1000.0f);
	AReEchoProjectileActor* CombinationProjectile =
	    CombinationFixture.World->SpawnActor<AReEchoProjectileActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	CombinationProjectile->SetOwner(CombinationOwner);
	FReEchoAttackIdentity CombinationAttack;
	CombinationAttack.Source = CombinationOwner;
	CombinationAttack.Sequence = 1;
	CombinationAttack.SourceFaction = EReEchoCombatFaction::PlayerSide;
	CombinationProjectile->InitializeProjectile(FVector::ForwardVector,
	                                            10.0f,
	                                            FVector::ZeroVector,
	                                            FLinearColor::White,
	                                            EReEchoElement::None,
	                                            1.0f,
	                                            100.0f,
	                                            1000.0f,
	                                            CombinationAttack,
	                                            true,
	                                            EReEchoDamageSource::Player,
	                                            TEXT("Gun"),
	                                            true);
	TickProjectiles(CombinationFixture.World, 0.3f);
	TickProjectiles(CombinationFixture.World, 0.35f);
	TestTrue(TEXT("Pierce plus explosion damages the first contact"), WeaponEnemyHealth(ContactOne) < 1000.0f);
	TestTrue(TEXT("First contact performs its own explosion"), WeaponEnemyHealth(SplashOne) < 1000.0f);
	TestTrue(TEXT("Piercing reaches the second contact"), WeaponEnemyHealth(ContactTwo) < 1000.0f);
	TestTrue(TEXT("Second contact performs a second explosion"), WeaponEnemyHealth(SplashTwo) < 1000.0f);

	FReEchoWeaponWorldFixture DelayedFixture;
	UReEchoCombatantComponent* DelayedCombatant = nullptr;
	AActor* DelayedOwner = DelayedFixture.SpawnWeaponOwner(FVector::ZeroVector, DelayedCombatant);
	FReEchoBuildSnapshot DelayedBuild = MakeBuild(*Snapshot, TEXT("W_J_08"));
	SetBuildStats(DelayedBuild, DelayedCombatant->Stats);
	TestTrue(TEXT("Delayed critical-pierce rune equips"),
	         ReEchoWeaponRuntime::TryEquipParts(
	             *Snapshot, DelayedBuild, {TEXT("P_BOW_PIERCING_ARROWHEAD")}, DelayedBuild, Error));
	DelayedCombatant->InitializeFromStats(DelayedBuild.Stats, true);
	DelayedCombatant->Stats.RoleId = TEXT("Hunter");
	DelayedCombatant->Stats.CriticalRate = 1.0f;
	AReEchoWeaponActor* DelayedWeapon = DelayedFixture.World->SpawnActor<AReEchoWeaponActor>();
	DelayedWeapon->SetOwner(DelayedOwner);
	DelayedWeapon->InitializeWeapon(&DelayedBuild, Snapshot);
	DelayedFixture.SpawnEnemy(FVector(200.0f, 0.0f, 0.0f), 330, 1000.0f);
	AReEchoEnemyActor* DelayedTarget = DelayedFixture.SpawnEnemy(FVector(450.0f, 0.0f, 0.0f), 331, 1000.0f);
	TestTrue(TEXT("Delayed rune projectile attack executes"), DelayedWeapon->ExecuteBasicAttack(DelayedCombatant));
	FReEchoBuildSnapshot PlainBow = MakeBuild(*Snapshot, TEXT("W_J_08"));
	SetBuildStats(PlainBow, DelayedCombatant->Stats);
	DelayedWeapon->InitializeWeapon(&PlainBow, Snapshot);
	TickProjectiles(DelayedFixture.World, 0.3f);
	TickProjectiles(DelayedFixture.World, 0.3f);
	TestTrue(TEXT("Projectile retains its attack-time rune snapshot after the weapon is rebuilt"),
	         WeaponEnemyHealth(DelayedTarget) < 1000.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoMeleeProjectileCutEligibilityTest,
	                             "ReEcho.Weapons.Runtime.MeleeProjectileCutEligibility",
	                             EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoMeleeProjectileCutEligibilityTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Longsword retains rabbit projectile cutting"),
	         AReEchoWeaponActor::CanCutRabbitProjectilesForTests(TEXT("Pattern.LongSwordCombo")));
	TestTrue(TEXT("Scythe gains rabbit projectile cutting"),
	         AReEchoWeaponActor::CanCutRabbitProjectilesForTests(TEXT("Pattern.ScytheSweep")));
	TestFalse(TEXT("Bow cannot cut rabbit projectiles"),
	          AReEchoWeaponActor::CanCutRabbitProjectilesForTests(TEXT("Pattern.BowShot")));
	TestFalse(TEXT("Gun cannot cut rabbit projectiles"),
	          AReEchoWeaponActor::CanCutRabbitProjectilesForTests(TEXT("Pattern.GunShot")));
	TestFalse(TEXT("Unknown patterns cannot cut rabbit projectiles"),
	          AReEchoWeaponActor::CanCutRabbitProjectilesForTests(NAME_None));
	return true;
}

#endif
