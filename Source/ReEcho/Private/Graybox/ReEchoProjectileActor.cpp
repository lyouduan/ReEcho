#include "Graybox/ReEchoProjectileActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Materials/MaterialInstanceDynamic.h"

AReEchoProjectileActor::AReEchoProjectileActor()
{
	PrimaryActorTick.bCanEverTick = true;
	InitialLifeSpan = 2.f;
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(13.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileShape"));
	Shape->SetupAttachment(RootComponent);
	Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	Shape->SetRelativeScale3D(FVector(0.26f));
}

void AReEchoProjectileActor::InitializeProjectile(const FVector& Direction,
                                                  float InDamage,
                                                  const FVector& InDamageSource,
                                                  const FLinearColor& Color)
{
	Velocity = Direction.GetSafeNormal() * Speed;
	Damage = FMath::Max(0.f, InDamage);
	DamageSource = InDamageSource;
	if (UMaterialInterface* Base =
	        LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, this);
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Shape->SetMaterial(0, Material);
	}
}

void AReEchoProjectileActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FVector PreviousLocation = GetActorLocation();
	const FVector NewLocation = PreviousLocation + Velocity * DeltaSeconds;
	SetActorLocation(NewLocation);
	for (TActorIterator<AReEchoEnemyActor> It(GetWorld()); It; ++It)
	{
		if (!It->IsAlive())
		{
			continue;
		}
		if (It->IntersectsProjectilePath(PreviousLocation, NewLocation, Collision->GetScaledSphereRadius()))
		{
			It->ReceiveGrayboxDamage(Damage, DamageSource);
			Destroy();
			return;
		}
	}
}
