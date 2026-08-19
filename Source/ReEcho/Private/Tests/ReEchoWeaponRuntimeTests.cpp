#if WITH_DEV_AUTOMATION_TESTS

#include "Weapons/ReEchoWeaponRuntime.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Components/SceneComponent.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Graybox/ReEchoProjectileActor.h"
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
	Fixture.Advance(0.29f);
	Weapon->Tick(0.29f);
	TestTrue(TEXT("Second ordered step ignores the longer presentation duration"),
	         Weapon->ExecuteBasicAttack(Combatant));
	TestTrue(TEXT("Front target is killed by second step"), !Front->IsAlive());
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponProjectileRuntimeTest,
                                 "ReEcho.Weapons.ProjectilesUseSingleShotSpreadCountAndExplosion",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponProjectileRuntimeTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();

	FReEchoWeaponWorldFixture SingleFixture;
	UReEchoCombatantComponent* SingleCombatant = nullptr;
	AActor* SingleOwner = SingleFixture.SpawnWeaponOwner(FVector::ZeroVector, SingleCombatant);
	FReEchoBuildSnapshot SingleBuild = MakeBuild(*Snapshot, TEXT("W_J_02"));
	SetBuildStats(SingleBuild, SingleCombatant->Stats);
	AReEchoWeaponActor* SingleWeapon = SingleFixture.World->SpawnActor<AReEchoWeaponActor>();
	SingleWeapon->SetOwner(SingleOwner);
	SingleWeapon->InitializeWeapon(&SingleBuild, Snapshot);
	TestTrue(TEXT("Single projectile attack executes"), SingleWeapon->ExecuteBasicAttack(SingleCombatant));
	TestEqual(TEXT("Single projectile pattern spawns one projectile"), CountProjectiles(SingleFixture.World), 1);

	FReEchoWeaponWorldFixture SpreadFixture;
	UReEchoCombatantComponent* Combatant = nullptr;
	AActor* Owner = SpreadFixture.SpawnWeaponOwner(FVector::ZeroVector, Combatant);
	FReEchoBuildSnapshot Build = MakeBuild(*Snapshot, TEXT("W_J_02"));
	SetBuildStats(Build, Combatant->Stats);
	AReEchoWeaponActor* Weapon = SpreadFixture.World->SpawnActor<AReEchoWeaponActor>();
	Weapon->SetOwner(Owner);
	Weapon->InitializeWeapon(&Build, Snapshot);
	AReEchoEnemyActor* Primary = SpreadFixture.SpawnEnemy(FVector(1000.0f, 0.0f, 0.0f), 4, 100.0f);
	AReEchoEnemyActor* Splash = SpreadFixture.SpawnEnemy(FVector(1000.0f, 120.0f, 0.0f), 5, 100.0f);
	TestTrue(TEXT("Spread projectile attack executes"), Weapon->ExecuteBasicAttack(Combatant));
	TestEqual(TEXT("Projectile count comes from weapon definition"), CountProjectiles(SpreadFixture.World), 3);
	float FirstYaw = 0.0f;
	float LastYaw = 0.0f;
	int32 Seen = 0;
	for (TActorIterator<AReEchoProjectileActor> It(SpreadFixture.World); It; ++It)
	{
		TestEqual(TEXT("Projectile carries explosion radius"), It->GetExplosionRadiusCm(), 200.0f);
		if (Seen == 0)
		{
			FirstYaw = It->GetVelocity().Rotation().Yaw;
		}
		LastYaw = It->GetVelocity().Rotation().Yaw;
		++Seen;
	}
	TestTrue(TEXT("Spread produces distinct projectile directions"), !FMath::IsNearlyEqual(FirstYaw, LastYaw, 0.1f));
	TickProjectiles(SpreadFixture.World, 1.10f);
	TestTrue(TEXT("Primary target takes projectile damage"), WeaponEnemyHealth(Primary) < 100.0f);
	TestTrue(TEXT("Explosion radius damages nearby target"), WeaponEnemyHealth(Splash) < 100.0f);

	FReEchoWeaponWorldFixture LifetimeFixture;
	UReEchoCombatantComponent* LifetimeCombatant = nullptr;
	AActor* LifetimeOwner = LifetimeFixture.SpawnWeaponOwner(FVector::ZeroVector, LifetimeCombatant);
	FReEchoBuildSnapshot LifetimeBuild = MakeBuild(*Snapshot, TEXT("W_J_02"));
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
	FReEchoBuildSnapshot Build = MakeBuild(*Snapshot, TEXT("W_J_02"));
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
	AReEchoEchoActor* ProjectileEcho = InitializeEcho(ProjectileFixture, TEXT("W_J_02"));
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
	TestEqual(TEXT("Unequipped attack consumes physical damage in combat"), WeaponEnemyHealth(PhysicalTarget), 70.0f);

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
	TestEqual(TEXT("Core channel consumes elemental damage in combat"), WeaponEnemyHealth(ElementTarget), 40.0f);

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
	TestTrue(TEXT("Run equips compatible core"),
	         Run->TryEquipParts({TEXT("P_CORE_FLAME")}, Error));
	TestEqual(TEXT("Equipped run has recomputed core attack speed"), Run->CurrentBuild.Stats.AttackSpeed, 1.0f);

	UReEchoRunSaveGame* Save = Run->CreateSaveSnapshot();
	Save->SaveVersion = 9;
	Save->OwnedPartIds.Reset();
	UGameInstance* RestoreGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RestoreRun = NewObject<UReEchoRunSubsystem>(RestoreGameInstance);
	TestTrue(TEXT("Equipment snapshot restores with authoritative base"), RestoreRun->RestoreSaveSnapshot(*Save));
	TestTrue(TEXT("Restored build retains authoritative equipment base"), RestoreRun->CurrentBuild.bHasEquipmentBase);
	TestEqual(
	    TEXT("Restored build recomputes core attack speed"), RestoreRun->CurrentBuild.Stats.AttackSpeed, 1.0f);
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
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_02"));
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

	const FString ModifiedDir =
	    AssembleModifiedCsvDirectory(TEXT("weapons.csv"),
	                                 TEXT("W_J_01,LongSword,Crescent Blade,"
	                                      "CrescentBlade,2,true,2,Pattern.LongSwordCombo,0.28,1.20,0,180,100,"
	                                      "3,0,0,1,true,RuntimeCompatibility,2,"),
	                                 TEXT("W_J_01,LongSword,Crescent Blade,"
	                                      "CrescentBlade,2,true,2,Pattern.LongSwordCombo,0.28,1.20,0,180,100,"
	                                      "5,0,0,1,true,RuntimeCompatibility,2,"));
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
	    TEXT("PE_CORE_FLAME_CHANNEL,P_CORE_FLAME,1,OnEquip,WeaponDamageChannel,DamageChannel,Override,9,Part.CoreDamageChannel,None,None,Flame,0,0,0,Unique,true,,武器插槽C,5"),
	    TEXT("PE_CORE_FLAME_CHANNEL,P_CORE_FLAME,1,OnEquip,WeaponDamageChannel,DamageChannel,Override,8,Part.CoreDamageChannel,None,None,Flame,0,0,0,Unique,true,,武器插槽C,5"));
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

#endif
