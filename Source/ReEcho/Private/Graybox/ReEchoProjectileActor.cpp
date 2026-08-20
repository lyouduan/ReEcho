#include "Graybox/ReEchoProjectileActor.h"

#include "Combat/ReEchoElementReaction.h"
#include "Camera/PlayerCameraManager.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"

AReEchoProjectileActor::AReEchoProjectileActor()
{
	PrimaryActorTick.bCanEverTick = true;
	ProjectileLogic = CreateDefaultSubobject<UReEchoProjectileLogicComponent>(TEXT("ProjectileLogic"));
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(13.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileShape"));
	Shape->SetupAttachment(RootComponent);
	Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	Shape->SetRelativeScale3D(FVector(0.26f));

	ElementLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ElementLabel"));
	ElementLabel->SetupAttachment(Collision);
	ElementLabel->SetHorizontalAlignment(EHTA_Center);
	ElementLabel->SetVerticalAlignment(EVRTA_TextCenter);
	ElementLabel->SetWorldSize(42.0f);
	ElementLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 28.0f));
	ElementLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ElementLabel->SetCastShadow(false);
	ElementLabel->SetTranslucentSortPriority(24);
	ElementLabel->SetVisibility(false);
	if (UMaterialInterface* UnlitTextMaterial =
	        LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/UnlitText.UnlitText")))
	{
		ElementLabel->SetTextMaterial(UnlitTextMaterial);
	}
}

void AReEchoProjectileActor::InitializeProjectile(const FVector& Direction,
                                                  float InDamage,
                                                  const FVector& InDamageSource,
                                                  const FLinearColor& Color,
                                                  const EReEchoElement InElement,
                                                  const float InReactionEfficiency,
                                                   const float InExplosionRadiusCm,
                                                   const float InMaxRangeCm,
                                                   const FReEchoAttackIdentity InAttack,
                                                   const EReEchoDamageSource InDamageSourceType,
                                                   const FName InWeaponVisualKey)
{
	Damage = FMath::Max(0.f, InDamage);
	Element = InElement;
	ExplosionRadiusCm = FMath::Max(0.0f, InExplosionRadiusCm);
	FReEchoLogicalProjectileSpec Spec;
	Spec.HitIntent.Attack = InAttack;
	Spec.HitIntent.RawDamage = Damage;
	Spec.HitIntent.DamageSource = InDamageSourceType;
	Spec.HitIntent.Element = Element;
	Spec.HitIntent.ReactionEfficiency = FMath::Max(0.0f, InReactionEfficiency);
	Spec.HitIntent.SourceLocation = InDamageSource;
	Spec.Direction = Direction;
	Spec.SpeedCmPerSecond = Speed;
	Spec.CarrierRadiusCm = Collision->GetScaledSphereRadius();
	Spec.ExplosionRadiusCm = ExplosionRadiusCm;
	Spec.MaximumRangeCm = FMath::Max(1.0f, InMaxRangeCm);
	if (!ProjectileLogic->InitializeProjectile(Spec))
	{
		Destroy();
		return;
	}
	const bool bHasElement = ReEchoElementReaction::IsCombatElement(Element);
	Shape->SetVisibility(!bHasElement);
	ElementLabel->SetVisibility(bHasElement);
	if (bHasElement)
	{
		const FString Label = ReEchoElementReaction::GetElementLabel(Element).Left(1);
		ElementLabel->SetText(FText::FromString(Label));
		ElementLabel->SetTextRenderColor(Color.ToFColor(false));
	}
	if (UMaterialInterface* Base =
	        LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, this);
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Shape->SetMaterial(0, Material);
	}
	ConfigureWeaponVisual(InWeaponVisualKey, Color);
}

FString AReEchoProjectileActor::ResolveWeaponTexturePath(const FName WeaponVisualKey)
{
	if (WeaponVisualKey == TEXT("Bow"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/BowProjectile.BowProjectile");
	}
	if (WeaponVisualKey == TEXT("Gun"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/GunProjectile.GunProjectile");
	}
	if (WeaponVisualKey == TEXT("Staff") || WeaponVisualKey == TEXT("MoonStaff"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/StaffLightWave.StaffLightWave");
	}
	return FString();
}

void AReEchoProjectileActor::ConfigureWeaponVisual(const FName WeaponVisualKey, const FLinearColor& Color)
{
	const FString TexturePath = ResolveWeaponTexturePath(WeaponVisualKey);
	UTexture2D* Texture = TexturePath.IsEmpty() ? nullptr : LoadObject<UTexture2D>(nullptr, *TexturePath);
	if (Texture)
	{
		UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
		UMaterialInterface* SpriteMaterial = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
		if (Plane && SpriteMaterial)
		{
			Shape->SetStaticMesh(Plane);
			// The texture is a camera card. Gameplay direction remains owned by ProjectileLogic.
			Shape->SetAbsolute(false, true, false);
			Shape->SetWorldRotation(FRotationMatrix::MakeFromZX(
				FVector(-0.573576f, 0.0f, 0.819152f), FVector::RightVector).Rotator());
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SpriteMaterial, this);
			Material->SetTextureParameterValue(TEXT("SpriteTexture"), Texture);
			Shape->SetMaterial(0, Material);
			const float Height = WeaponVisualKey == TEXT("Bow") ? 34.0f : 56.0f;
			const float Aspect = static_cast<float>(Texture->GetSizeX()) / FMath::Max(1, Texture->GetSizeY());
			Shape->SetRelativeScale3D(FVector(Height * Aspect / 100.0f, Height / 100.0f, 1.0f));
			Shape->SetVisibility(true);
			return;
		}
	}

	// Distinct procedural fallbacks keep missing art playable and visually diagnosable.
	if (WeaponVisualKey == TEXT("Bow"))
	{
		Shape->SetRelativeScale3D(FVector(0.12f, 0.48f, 0.12f));
	}
	else if (WeaponVisualKey == TEXT("Gun"))
	{
		Shape->SetRelativeScale3D(FVector(0.14f));
	}
	else if (WeaponVisualKey == TEXT("Staff") || WeaponVisualKey == TEXT("MoonStaff"))
	{
		Shape->SetRelativeScale3D(FVector(0.34f, 0.18f, 0.34f));
	}
}

void AReEchoProjectileActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (ElementLabel && ElementLabel->IsVisible())
	{
		if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			ElementLabel->SetWorldRotation((-Camera->GetCameraRotation().Vector()).Rotation());
		}
	}
}
