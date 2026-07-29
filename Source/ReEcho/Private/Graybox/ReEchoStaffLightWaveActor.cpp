#include "Graybox/ReEchoStaffLightWaveActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Materials/MaterialInstanceDynamic.h"

AReEchoStaffLightWaveActor::AReEchoStaffLightWaveActor()
{
	PrimaryActorTick.bCanEverTick = true;
	InitialLifeSpan = 2.0f;
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(32.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WaveVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaveVisual"));
	WaveVisual->SetupAttachment(Collision);
	WaveVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaveVisual->SetCastShadow(false);
	WaveVisual->SetTranslucentSortPriority(9);
	WaveVisual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	constexpr float WaveWorldWidth = 150.0f;
	constexpr float WaveWorldHeight = 100.0f;
	WaveVisual->SetRelativeScale3D(FVector(WaveWorldWidth / 100.0f, WaveWorldHeight / 100.0f, 1.0f));

	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(
		    nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial")))
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, this);
		Material->SetTextureParameterValue(
			TEXT("SpriteTexture"),
			LoadObject<UTexture2D>(nullptr, TEXT("/Game/ReEcho/Textures/Effects/StaffLightWave.StaffLightWave")));
		WaveVisual->SetMaterial(0, Material);
	}
}

void AReEchoStaffLightWaveActor::InitializeWave(
	const FVector& Direction,
	const float InDamage,
	const FVector& InDamageSource,
	const float InRange)
{
	const FVector TravelDirection = Direction.GetSafeNormal2D();
	Velocity = (TravelDirection.IsNearlyZero() ? FVector::ForwardVector : TravelDirection) * Speed;
	Damage = FMath::Max(0.0f, InDamage);
	DamageSource = InDamageSource;
	SpawnLocation = GetActorLocation();
	MaximumRange = FMath::Max(1.0f, InRange);

	const FVector CameraFacingNormal(-0.5736f, 0.0f, 0.8192f);
	FVector ScreenTravel = TravelDirection - CameraFacingNormal * FVector::DotProduct(TravelDirection, CameraFacingNormal);
	ScreenTravel = ScreenTravel.GetSafeNormal();
	if (ScreenTravel.IsNearlyZero())
	{
		ScreenTravel = FVector::RightVector;
	}
	SetActorRotation(FRotationMatrix::MakeFromZX(CameraFacingNormal, ScreenTravel).Rotator());
}

void AReEchoStaffLightWaveActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FVector PreviousLocation = GetActorLocation();
	const FVector NewLocation = PreviousLocation + Velocity * DeltaSeconds;
	SetActorLocation(NewLocation);

	for (TActorIterator<AReEchoEnemyActor> It(GetWorld()); It; ++It)
	{
		if (It->IsAlive()
			&& It->IntersectsProjectilePath(PreviousLocation, NewLocation, Collision->GetScaledSphereRadius()))
		{
			It->ReceiveGrayboxDamage(Damage, DamageSource);
			Destroy();
			return;
		}
	}
	if (FVector::Dist2D(SpawnLocation, NewLocation) >= MaximumRange)
	{
		Destroy();
	}
}