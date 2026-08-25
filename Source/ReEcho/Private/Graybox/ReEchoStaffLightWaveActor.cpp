#include "Graybox/ReEchoStaffLightWaveActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"

AReEchoStaffLightWaveActor::AReEchoStaffLightWaveActor()
{
	PrimaryActorTick.bCanEverTick = false;
	ProjectileLogic = CreateDefaultSubobject<UReEchoProjectileLogicComponent>(TEXT("ProjectileLogic"));
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
		    LoadObject<UTexture2D>(nullptr, *FReEchoWeaponVisualCatalog::ResolveAttackTexturePath(TEXT("MoonStaff"))));
		WaveVisual->SetMaterial(0, Material);
	}
}

void AReEchoStaffLightWaveActor::InitializeWave(const FVector& Direction,
                                                const float InDamage,
                                                const FVector& InDamageSource,
                                                const float InRange,
                                                const FReEchoAttackIdentity InAttack,
                                                const EReEchoDamageSource InDamageSourceType)
{
	const FVector TravelDirection = Direction.GetSafeNormal2D();
	FReEchoLogicalProjectileSpec Spec;
	Spec.HitIntent.Attack = InAttack;
	Spec.HitIntent.RawDamage = FMath::Max(0.0f, InDamage);
	Spec.HitIntent.DamageSource = InDamageSourceType;
	Spec.HitIntent.SourceLocation = InDamageSource;
	Spec.Direction = TravelDirection;
	Spec.SpeedCmPerSecond = Speed;
	Spec.CarrierRadiusCm = Collision->GetScaledSphereRadius();
	Spec.MaximumRangeCm = FMath::Max(1.0f, InRange);
	if (!ProjectileLogic->InitializeProjectile(Spec))
	{
		Destroy();
		return;
	}

	const FVector CameraFacingNormal(-0.5736f, 0.0f, 0.8192f);
	FVector ScreenTravel =
	    TravelDirection - CameraFacingNormal * FVector::DotProduct(TravelDirection, CameraFacingNormal);
	ScreenTravel = ScreenTravel.GetSafeNormal();
	if (ScreenTravel.IsNearlyZero())
	{
		ScreenTravel = FVector::RightVector;
	}
	SetActorRotation(FRotationMatrix::MakeFromZX(CameraFacingNormal, ScreenTravel).Rotator());
}
