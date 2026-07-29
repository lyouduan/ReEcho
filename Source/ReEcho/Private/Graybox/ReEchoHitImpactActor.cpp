#include "Graybox/ReEchoHitImpactActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"

AReEchoHitImpactActor::AReEchoHitImpactActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	SceneRoot->SetAbsolute(false, true, false);

	ImpactSprite = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ImpactSprite"));
	ImpactSprite->SetupAttachment(SceneRoot);
	ImpactSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ImpactSprite->SetCastShadow(false);
	ImpactSprite->SetTranslucentSortPriority(50);
	ImpactSprite->SetRelativeRotation(FRotationMatrix::MakeFromZX(
		FVector(-0.573576f, 0.0f, 0.819152f), FVector::RightVector).Rotator());
	ImpactSprite->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));

	UMaterialInterface* SpriteMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	UTexture2D* ImpactTexture = LoadObject<UTexture2D>(
		nullptr, TEXT("/Game/ReEcho/Textures/Effects/HitStarburst.HitStarburst"));
	if (SpriteMaterial && ImpactTexture)
	{
		UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(SpriteMaterial, this);
		MaterialInstance->SetTextureParameterValue(TEXT("SpriteTexture"), ImpactTexture);
		ImpactSprite->SetMaterial(0, MaterialInstance);
		constexpr float ImpactWorldHeight = 190.0f;
		const float AspectRatio = static_cast<float>(ImpactTexture->GetSizeX())
			/ FMath::Max(1, ImpactTexture->GetSizeY());
		ImpactSprite->SetRelativeScale3D(FVector(
			ImpactWorldHeight * AspectRatio / 100.0f,
			ImpactWorldHeight / 100.0f,
			1.0f));
	}
	SetActorEnableCollision(false);
}

void AReEchoHitImpactActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ElapsedTime += DeltaSeconds;
	const float Progress = FMath::Clamp(ElapsedTime / Lifetime, 0.0f, 1.0f);
	const float Pulse = Progress < 0.28f
		? FMath::Lerp(0.20f, 1.28f, Progress / 0.28f)
		: FMath::Lerp(1.28f, 0.55f, (Progress - 0.28f) / 0.72f);
	SetActorScale3D(FVector(Pulse));
	if (Progress >= 1.0f)
	{
		Destroy();
	}
}