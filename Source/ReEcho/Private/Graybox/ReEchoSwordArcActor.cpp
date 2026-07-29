#include "Graybox/ReEchoSwordArcActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"

AReEchoSwordArcActor::AReEchoSwordArcActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	// 特效保持固定相机朝向，不继承角色面向旋转。
	SceneRoot->SetAbsolute(false, true, false);

	SlashSprite = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SlashSprite"));
	SlashSprite->SetupAttachment(SceneRoot);
	SlashSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SlashSprite->SetCastShadow(false);
	SlashSprite->SetTranslucentSortPriority(8);
	SlashSprite->SetRelativeRotation(FRotationMatrix::MakeFromZX(
		FVector(-0.573576f, 0.0f, 0.819152f), FVector::RightVector).Rotator());
	if (UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(
		    nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
	{
		SlashSprite->SetStaticMesh(PlaneMesh);
	}
	UMaterialInterface* SpriteMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	UTexture2D* SlashTexture = LoadObject<UTexture2D>(
		nullptr, TEXT("/Game/ReEcho/Textures/Effects/SlashCrescent.SlashCrescent"));
	if (SpriteMaterial && SlashTexture)
	{
		UMaterialInstanceDynamic* MaterialInstance =
			UMaterialInstanceDynamic::Create(SpriteMaterial, this);
		MaterialInstance->SetTextureParameterValue(TEXT("SpriteTexture"), SlashTexture);
		SlashSprite->SetMaterial(0, MaterialInstance);
		constexpr float SlashWorldHeight = 360.0f;
		const float AspectRatio = static_cast<float>(SlashTexture->GetSizeX())
			/ FMath::Max(1, SlashTexture->GetSizeY());
		SlashBaseScale = FVector(
			SlashWorldHeight * AspectRatio / 100.0f,
			SlashWorldHeight / 100.0f,
			1.0f);
		SlashSprite->SetRelativeScale3D(SlashBaseScale);
	}
	SetActorEnableCollision(false);
}

void AReEchoSwordArcActor::InitializeArc(const float SwingDirection)
{
	if (!SlashSprite)
	{
		return;
	}
	FVector DirectedScale = SlashBaseScale;
	DirectedScale.X *= SwingDirection >= 0.0f ? 1.0f : -1.0f;
	SlashSprite->SetRelativeScale3D(DirectedScale);
}

void AReEchoSwordArcActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ElapsedTime += DeltaSeconds;
	const float Progress = FMath::Clamp(ElapsedTime / Lifetime, 0.0f, 1.0f);
	SetActorScale3D(FVector(FMath::Lerp(0.76f, 1.04f, Progress)));
	if (Progress >= 1.0f)
	{
		Destroy();
	}
}