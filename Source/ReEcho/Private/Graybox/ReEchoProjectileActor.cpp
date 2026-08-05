#include "Graybox/ReEchoProjectileActor.h"

#include "Combat/ReEchoElementReaction.h"
#include "Camera/PlayerCameraManager.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
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
                                                  const float InReactionEfficiency)
{
	Velocity = Direction.GetSafeNormal() * Speed;
	Damage = FMath::Max(0.f, InDamage);
	DamageSource = InDamageSource;
	Element = InElement;
	ReactionEfficiency = FMath::Max(0.0f, InReactionEfficiency);
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
}

void AReEchoProjectileActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FVector PreviousLocation = GetActorLocation();
	const FVector NewLocation = PreviousLocation + Velocity * DeltaSeconds;
	SetActorLocation(NewLocation);
	if (ElementLabel && ElementLabel->IsVisible())
	{
		if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			ElementLabel->SetWorldRotation((-Camera->GetCameraRotation().Vector()).Rotation());
		}
	}
	for (TActorIterator<AReEchoEnemyActor> It(GetWorld()); It; ++It)
	{
		if (!It->IsAlive())
		{
			continue;
		}
		if (It->IntersectsProjectilePath(PreviousLocation, NewLocation, Collision->GetScaledSphereRadius()))
		{
			if (Element == EReEchoElement::None)
			{
				It->ReceiveGrayboxDamage(Damage, DamageSource, GetOwner());
			}
			else
			{
				It->ReceiveElementalDamage(Damage, Element, DamageSource, GetOwner(), ReactionEfficiency);
			}
			Destroy();
			return;
		}
	}
}
