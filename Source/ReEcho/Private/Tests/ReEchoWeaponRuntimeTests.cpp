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
	    ReEchoRunData::ResolveStartingBuildFromSnapshot(&Snapshot, TEXT("J_CAT"), WeaponId);
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
			Projectile->Tick(Seconds);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponEquipmentAppliesModifiersTest,
                                 "ReEcho.Weapons.EquipmentAppliesModifiersAndFailsAtomically",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponEquipmentAppliesModifiersTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult = FReEchoCsvDataRegistry::LoadAndPublishDefault();
	if (!TestTrue(TEXT("Default CSV data loads"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	FReEchoBuildSnapshot DaggerBuild = MakeBuild(*Snapshot, TEXT("W_J_05"));

	const TMap<FName, FName> ExpectedCoreChannels = {
	    {TEXT("P_CORE_PRIMORDIAL"), TEXT("Physical")},
	    {TEXT("P_CORE_TIDE"), TEXT("Water")},
	    {TEXT("P_CORE_FOREST"), TEXT("Grass")},
	    {TEXT("P_CORE_FLAME"), TEXT("Flame")},
	    {TEXT("P_CORE_THUNDER"), TEXT("Lightning")},
	    {TEXT("P_CORE_PRISM"), TEXT("RandomElement")},
	};
	for (const TPair<FName, FName>& Pair : ExpectedCoreChannels)
	{
		FString Error;
		FReEchoBuildSnapshot Equipped;
		TestTrue(FString::Printf(TEXT("Core %s equips"), *Pair.Key.ToString()),
		         ReEchoWeaponRuntime::TryEquipParts(*Snapshot, DaggerBuild, {Pair.Key}, Equipped, Error));
		FReEchoEffectiveWeaponDefinition Effective;
		TestTrue(TEXT("Effective definition resolves"),
		         ReEchoWeaponRuntime::BuildEffectiveWeaponDefinition(*Snapshot, Equipped, Effective, Error));
		TestEqual(TEXT("Core writes expected damage channel"), Effective.DamageChannelId, Pair.Value);
	}
	TestEqual(TEXT("Physical channel has no combat element"),
	          ReEchoWeaponRuntime::ElementFromDamageChannel(TEXT("Physical"), 3),
	          EReEchoElement::None);
	TestEqual(TEXT("Flame channel resolves stable ElementId"),
	          ReEchoWeaponRuntime::ElementFromDamageChannel(TEXT("Flame"), 3),
	          EReEchoElement::Flame);
	TestEqual(TEXT("Random element channel is deterministic"),
	          ReEchoWeaponRuntime::ElementFromDamageChannel(TEXT("RandomElement"), 7),
	          ReEchoWeaponRuntime::ElementFromDamageChannel(TEXT("RandomElement"), 7));

	FString Error;
	FReEchoBuildSnapshot Equipped;
	TestTrue(TEXT("Strength grip modifier equips"),
	         ReEchoWeaponRuntime::TryEquipParts(
	             *Snapshot, DaggerBuild, {TEXT("P_CORE_FLAME"), TEXT("P_DAGGER_STRENGTH_GRIP")}, Equipped, Error));
	TestEqual(TEXT("Multiply modifier updates attack speed"), Equipped.Stats.AttackSpeed, 1.2f);
	FReEchoBuildSnapshot Reequipped;
	TestTrue(TEXT("Identical strength grip re-equip succeeds"),
	         ReEchoWeaponRuntime::TryEquipParts(
	             *Snapshot, Equipped, {TEXT("P_CORE_FLAME"), TEXT("P_DAGGER_STRENGTH_GRIP")}, Reequipped, Error));
	TestEqual(TEXT("Identical re-equip does not multiply attack speed twice"), Reequipped.Stats.AttackSpeed, 1.2f);

	FReEchoBuildSnapshot Replaced;
	TestTrue(TEXT("Replacing strength grip succeeds"),
	         ReEchoWeaponRuntime::TryEquipParts(
	             *Snapshot, Reequipped, {TEXT("P_CORE_TIDE"), TEXT("P_DAGGER_THRUST_GRIP")}, Replaced, Error));
	TestEqual(TEXT("Replacing strength grip restores base attack speed"), Replaced.Stats.AttackSpeed, 1.0f);
	TestEqual(TEXT("Replacement writes only the new damage channel"),
	          Replaced.RuleFlags.FindRef(TEXT("Weapon.DamageChannel")),
	          FString(TEXT("Water")));

	FReEchoBuildSnapshot Unequipped;
	TestTrue(TEXT("Unequip-all succeeds"),
	         ReEchoWeaponRuntime::TryEquipParts(*Snapshot, Replaced, {}, Unequipped, Error));
	TestEqual(TEXT("Unequip-all restores base attack speed"), Unequipped.Stats.AttackSpeed, 1.0f);
	TestFalse(TEXT("Unequip-all removes old damage channel"),
	          Unequipped.RuleFlags.Contains(TEXT("Weapon.DamageChannel")));
	TestFalse(TEXT("Unequip-all removes old pattern replacement"),
	          Unequipped.RuleFlags.Contains(TEXT("Weapon.AttackPatternId")));
	TestFalse(TEXT("Unequip-all removes old interval override"),
	          Unequipped.RuleFlags.Contains(TEXT("Weapon.AttackIntervalSeconds")));

	TestTrue(TEXT("Thrust and holy modifiers equip"),
	         ReEchoWeaponRuntime::TryEquipParts(
	             *Snapshot,
	             DaggerBuild,
	             {TEXT("P_CORE_FLAME"), TEXT("P_DAGGER_THRUST_GRIP"), TEXT("P_DAGGER_HOLY_BLADE")},
	             Equipped,
	             Error));
	TestEqual(TEXT("Override modifier writes attack interval"),
	          FCString::Atof(*Equipped.RuleFlags.FindRef(TEXT("Weapon.AttackIntervalSeconds"))),
	          0.4f);
	TestEqual(TEXT("Override modifier replaces pattern"),
	          Equipped.RuleFlags.FindRef(TEXT("Weapon.AttackPatternId")),
	          FString(TEXT("Pattern.DaggerDashOnly")));
	TestEqual(TEXT("Add modifier accumulates OnKill healing"),
	          FCString::Atof(*Equipped.RuleFlags.FindRef(TEXT("Weapon.OnKillHealPercent"))),
	          0.05f);

	FReEchoBuildSnapshot Original = DaggerBuild;
	FReEchoBuildSnapshot Failed = DaggerBuild;
	TestFalse(
	    TEXT("Disabled part fails"),
	    ReEchoWeaponRuntime::TryEquipParts(*Snapshot, DaggerBuild, {TEXT("P_DAGGER_NINJA_BLADE")}, Failed, Error));
	TestEqual(TEXT("Disabled failure is atomic"), Failed.EquippedParts.Num(), Original.EquippedParts.Num());
	TestEqual(TEXT("Disabled failure preserves attack speed"), Failed.Stats.AttackSpeed, Original.Stats.AttackSpeed);
	TestFalse(
	    TEXT("Duplicate slot fails atomically"),
	    ReEchoWeaponRuntime::TryEquipParts(
	        *Snapshot, DaggerBuild, {TEXT("P_DAGGER_STRENGTH_GRIP"), TEXT("P_DAGGER_THRUST_GRIP")}, Failed, Error));
	FReEchoBuildSnapshot LongSwordBuild = MakeBuild(*Snapshot, TEXT("W_J_01"));
	TestFalse(
	    TEXT("Dagger-only part cannot equip to LongSword"),
	    ReEchoWeaponRuntime::TryEquipParts(*Snapshot, LongSwordBuild, {TEXT("P_DAGGER_STRENGTH_GRIP")}, Failed, Error));
	return true;
}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponDaggerPartRuntimeTest,
                                 "ReEcho.Weapons.DaggerPartsReplacePatternMoveInvulnerableAndHealOnKill",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponDaggerPartRuntimeTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	FReEchoWeaponWorldFixture Fixture;
	UReEchoCombatantComponent* Combatant = nullptr;
	AActor* Owner = Fixture.SpawnWeaponOwner(FVector::ZeroVector, Combatant);
	Combatant->Stats.PhysicalAttack = 200.0f;
	Combatant->RestoreCurrentHealth(50.0f);
	FReEchoBuildSnapshot Build = MakeBuild(*Snapshot, TEXT("W_J_05"));
	SetBuildStats(Build, Combatant->Stats);
	FString Error;
	TestTrue(TEXT("Dagger parts equip"),
	         ReEchoWeaponRuntime::TryEquipParts(
	             *Snapshot,
	             Build,
	             {TEXT("P_CORE_PRIMORDIAL"), TEXT("P_DAGGER_THRUST_GRIP"), TEXT("P_DAGGER_HOLY_BLADE")},
	             Build,
	             Error));
	AReEchoWeaponActor* Weapon = Fixture.World->SpawnActor<AReEchoWeaponActor>();
	Weapon->SetOwner(Owner);
	Weapon->InitializeWeapon(&Build, Snapshot);
	AReEchoEnemyActor* Target = Fixture.SpawnEnemy(FVector(300.0f, 0.0f, 0.0f), 3, 50.0f);

	TestTrue(TEXT("Replacement dash attack executes"), Weapon->ExecuteBasicAttack(Combatant));
	TestEqual(
	    TEXT("Dash movement is applied deterministically"), static_cast<float>(Owner->GetActorLocation().X), 200.0f);
	TestTrue(TEXT("Invulnerability window starts"), Weapon->IsInvulnerableWindowActive());
	TestTrue(TEXT("Replacement attack kills target"), !Target->IsAlive());
	TestEqual(TEXT("OnKill healing changes actual health"), Combatant->CurrentHealth, 55.0f);
	Fixture.Advance(0.71f);
	Weapon->Tick(0.71f);
	TestFalse(TEXT("Invulnerability window ends deterministically"), Weapon->IsInvulnerableWindowActive());
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
	FReEchoBuildSnapshot SingleBuild = MakeBuild(*Snapshot, TEXT("W_J_03"));
	SetBuildStats(SingleBuild, SingleCombatant->Stats);
	AReEchoWeaponActor* SingleWeapon = SingleFixture.World->SpawnActor<AReEchoWeaponActor>();
	SingleWeapon->SetOwner(SingleOwner);
	SingleWeapon->InitializeWeapon(&SingleBuild, Snapshot);
	TestTrue(TEXT("Single projectile attack executes"), SingleWeapon->ExecuteBasicAttack(SingleCombatant));
	TestEqual(TEXT("Single projectile pattern spawns one projectile"), CountProjectiles(SingleFixture.World), 1);

	FReEchoWeaponWorldFixture SpreadFixture;
	UReEchoCombatantComponent* Combatant = nullptr;
	AActor* Owner = SpreadFixture.SpawnWeaponOwner(FVector::ZeroVector, Combatant);
	FReEchoBuildSnapshot Build = MakeBuild(*Snapshot, TEXT("W_J_06"));
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
	FReEchoBuildSnapshot PhysicalBuild = MakeBuild(*Snapshot, TEXT("W_J_05"));
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
	FReEchoBuildSnapshot ElementBuild = MakeBuild(*Snapshot, TEXT("W_J_05"));
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

	FReEchoWeaponWorldFixture SpeedFixture;
	UReEchoCombatantComponent* SpeedCombatant = nullptr;
	AActor* SpeedOwner = SpeedFixture.SpawnWeaponOwner(FVector::ZeroVector, SpeedCombatant);
	FReEchoBuildSnapshot SpeedBuild = MakeBuild(*Snapshot, TEXT("W_J_05"));
	SetBuildStats(SpeedBuild, SpeedCombatant->Stats);
	TestTrue(
	    TEXT("Strength grip equips for cooldown proof"),
	    ReEchoWeaponRuntime::TryEquipParts(*Snapshot, SpeedBuild, {TEXT("P_DAGGER_STRENGTH_GRIP")}, SpeedBuild, Error));
	SpeedCombatant->InitializeFromStats(SpeedBuild.Stats, true);
	AReEchoWeaponActor* SpeedWeapon = SpeedFixture.World->SpawnActor<AReEchoWeaponActor>();
	SpeedWeapon->SetOwner(SpeedOwner);
	SpeedWeapon->InitializeWeapon(&SpeedBuild, Snapshot);
	SpeedFixture.SpawnEnemy(FVector(100.0f, 0.0f, 0.0f), 12, 100.0f);
	TestTrue(TEXT("Strength-grip attack executes through runtime cooldown path"),
	         SpeedWeapon->TryBasicAttack(SpeedCombatant));
	TestTrue(TEXT("Runtime cooldown consumes derived attack speed"),
	         FMath::IsNearlyEqual(SpeedWeapon->GetAttackCooldownRemaining(), 0.5f / 1.2f, 0.001f));
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
	Run->StartRun(TEXT("J_CAT"), TEXT("W_J_05"));
	FString Error;
	TestTrue(TEXT("Run equips compatible core and Dagger-only grip"),
	         Run->TryEquipParts({TEXT("P_CORE_FLAME"), TEXT("P_DAGGER_STRENGTH_GRIP")}, Error));
	TestEqual(TEXT("Equipped run has derived strength attack speed"), Run->CurrentBuild.Stats.AttackSpeed, 1.2f);

	UReEchoRunSaveGame* Save = Run->CreateSaveSnapshot();
	UGameInstance* RestoreGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RestoreRun = NewObject<UReEchoRunSubsystem>(RestoreGameInstance);
	TestTrue(TEXT("Equipment snapshot restores with authoritative base"), RestoreRun->RestoreSaveSnapshot(*Save));
	TestTrue(TEXT("Restored build retains authoritative equipment base"), RestoreRun->CurrentBuild.bHasEquipmentBase);
	TestEqual(
	    TEXT("Restored build recomputes strength attack speed"), RestoreRun->CurrentBuild.Stats.AttackSpeed, 1.2f);

	UReEchoRecorderComponent* Recorder = NewObject<UReEchoRecorderComponent>();
	Recorder->BeginRecording(1, TEXT("WeaponTest"), 77, Run->CurrentBuild);
	TestEqual(
	    TEXT("Recorder preserves equipped part count"), Recorder->GetRecording().BuildSnapshot.EquippedParts.Num(), 2);
	TestEqual(TEXT("Recorder preserves equipment base attack speed"),
	          Recorder->GetRecording().BuildSnapshot.EquipmentBaseStats.AttackSpeed,
	          1.0f);

	TestEqual(TEXT("Run keeps the pre-run WeaponId"), Run->CurrentBuild.WeaponId, FName(TEXT("W_J_05")));
	TestEqual(TEXT("Run keeps all compatible equipped parts"), Run->CurrentBuild.EquippedParts.Num(), 2);
	TestEqual(TEXT("Locked weapon keeps derived attack speed"), Run->CurrentBuild.Stats.AttackSpeed, 1.2f);

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
	Run->StartRun(TEXT("J_CAT"), TEXT("W_J_06"));
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
	                                 TEXT("W_J_06,Staff,Apprentice "
	                                      "Staff,ElementalOrb,None,false,0,Pattern.StaffProjectile,1.20,0,0.80,1000,0,"
	                                      "3,30,200,1,true,RuntimeCompatibility,6,"),
	                                 TEXT("W_J_06,Staff,Apprentice "
	                                      "Staff,ElementalOrb,None,false,0,Pattern.StaffProjectile,1.20,0,0.80,1000,0,"
	                                      "5,40,200,1,true,RuntimeCompatibility,6,"));
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
	    TEXT("Pinned actor keeps old projectile count after global republish"), CountProjectiles(Fixture.World), 3);

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
	          3);

	UGameInstance* RestoreGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RestoreRun = NewObject<UReEchoRunSubsystem>(RestoreGameInstance);
	TestFalse(TEXT("Old save is rejected after domain revision change"), RestoreRun->RestoreSaveSnapshot(*Save));
	TestEqual(TEXT("Rejected restore does not partially modify run phase"),
	          RestoreRun->Phase,
	          EReEchoRunPhase::CharacterSelect);

	const FString ModifiedEffectsDir = AssembleModifiedCsvDirectory(
	    TEXT("part_effects.csv"),
	    TEXT("PE_DAGGER_STRENGTH_SPEED,P_DAGGER_STRENGTH_GRIP,1,OnEquip,StatModifier,AttackSpeed,Multiply,1.2"),
	    TEXT("PE_DAGGER_STRENGTH_SPEED,P_DAGGER_STRENGTH_GRIP,1,OnEquip,StatModifier,AttackSpeed,Multiply,1.3"));
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
	FReEchoBuildSnapshot SlowBuild = MakeBuild(*Snapshot, TEXT("W_J_05"));
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
	FReEchoBuildSnapshot FastBuild = MakeBuild(*Snapshot, TEXT("W_J_05"));
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
