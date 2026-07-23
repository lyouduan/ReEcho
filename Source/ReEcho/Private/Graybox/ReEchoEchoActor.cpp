#include "Graybox/ReEchoEchoActor.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoTrajectoryActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Recording/ReEchoPlaybackComponent.h"
#include "Weapons/ReEchoWeaponActor.h"

AReEchoEchoActor::AReEchoEchoActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Shape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EchoShape"));
	SetRootComponent(Shape);
	Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shape->SetStaticMesh(
		LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	Shape->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.0f));

	if (UMaterialInterface* BaseMaterial =
			LoadObject<UMaterialInterface>(
				nullptr,
				TEXT("/Game/ReEcho/Materials/M_EchoGhost.M_EchoGhost")))
	{
		EchoMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		EchoMaterial->SetVectorParameterValue(
			TEXT("EchoColor"),
			FLinearColor(0.45f, 0.04f, 1.0f));
		EchoMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.34f);
		Shape->SetMaterial(0, EchoMaterial);
	}

	Playback = CreateDefaultSubobject<UReEchoPlaybackComponent>(TEXT("Playback"));
	Combatant = CreateDefaultSubobject<UReEchoCombatantComponent>(TEXT("Combatant"));
}

void AReEchoEchoActor::InitializeEcho(
	const FReEchoRecording& Recording,
	const float Efficiency)
{
	Playback->LoadRecording(Recording);
	Playback->OnReplayWeapon.AddDynamic(
		this,
		&AReEchoEchoActor::HandleReplayedWeapon);

	for (TActorIterator<AReEchoTrajectoryActor> TrajectoryIterator(GetWorld());
		TrajectoryIterator;
		++TrajectoryIterator)
	{
		TrajectoryIterator->Destroy();
	}

	Trajectory = GetWorld()->SpawnActor<AReEchoTrajectoryActor>();
	if (Trajectory)
	{
		Trajectory->SetOwner(this);
		Trajectory->InitializeTrajectory(Recording);
	}

	FReEchoStatBlock EchoStats = Recording.BuildSnapshot.Stats;
	EchoStats.PhysicalAttack =
		FMath::Max(1.0f, EchoStats.PhysicalAttack * Efficiency);
	EchoStats.ElementalAttack =
		FMath::Max(1.0f, EchoStats.ElementalAttack * Efficiency);
	Combatant->InitializeFromStats(EchoStats, true);

	Weapon = GetWorld()->SpawnActor<AReEchoWeaponActor>();
	if (Weapon)
	{
		Weapon->SetOwner(this);
		Weapon->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
		Weapon->SetActorRelativeLocation(FVector(28.0f, 0.0f, 16.0f));
		Weapon->InitializeWeapon();
		FName InitialWeaponId = Recording.BuildSnapshot.WeaponId;
		if (!Recording.WeaponChanges.IsEmpty())
		{
			InitialWeaponId = Recording.WeaponChanges[0].WeaponId;
		}
		Weapon->SelectWeaponById(InitialWeaponId);
	}
}

void AReEchoEchoActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Weapon)
	{
		Weapon->Destroy();
		Weapon = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AReEchoEchoActor::AdvanceEcho(const float EncounterTime)
{
	Playback->AdvancePlayback(EncounterTime);
}

void AReEchoEchoActor::HandleReplayedWeapon(
	const FName WeaponId,
	const float)
{
	if (Weapon)
	{
		Weapon->SelectWeaponById(WeaponId);
	}
}
void AReEchoEchoActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	VisualTime += DeltaSeconds;
	if (EchoMaterial)
	{
		const float PulsingOpacity =
			0.34f + FMath::Sin(VisualTime * 2.5f) * 0.06f;
		EchoMaterial->SetScalarParameterValue(TEXT("Opacity"), PulsingOpacity);
	}

	if (!Weapon)
	{
		return;
	}

	AReEchoEnemyActor* NearestEnemy = nullptr;
	float NearestDistanceSquared = FMath::Square(AutoTargetRange);
	for (TActorIterator<AReEchoEnemyActor> EnemyIterator(GetWorld());
		EnemyIterator;
		++EnemyIterator)
	{
		if (!EnemyIterator->IsAlive())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(
			GetActorLocation(),
			EnemyIterator->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestEnemy = *EnemyIterator;
		}
	}

	if (!NearestEnemy)
	{
		return;
	}

	const FVector AimDirection =
		(NearestEnemy->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!AimDirection.IsNearlyZero())
	{
		SetActorRotation(AimDirection.Rotation());
	}

	Weapon->TryBasicAttack(Combatant);
}