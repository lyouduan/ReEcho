#include "Graybox/ReEchoSwordArcActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"

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
	SlashSprite->SetRelativeRotation(
	    FRotationMatrix::MakeFromZX(FVector(-0.573576f, 0.0f, 0.819152f), FVector::RightVector).Rotator());
	if (UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
	{
		SlashSprite->SetStaticMesh(PlaneMesh);
	}
	UMaterialInterface* SpriteMaterial = LoadObject<UMaterialInterface>(
	    nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	UTexture2D* SlashTexture = LoadObject<UTexture2D>(nullptr, *ResolveWeaponTexturePath(TEXT("CrescentBlade")));
	if (SpriteMaterial && SlashTexture)
	{
		UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(SpriteMaterial, this);
		MaterialInstance->SetTextureParameterValue(TEXT("SpriteTexture"), SlashTexture);
		SlashSprite->SetMaterial(0, MaterialInstance);
		constexpr float SlashWorldHeight = 360.0f;
		const float AspectRatio =
		    static_cast<float>(SlashTexture->GetSizeX()) / FMath::Max(1, SlashTexture->GetSizeY());
		SlashBaseScale = FVector(SlashWorldHeight * AspectRatio / 100.0f, SlashWorldHeight / 100.0f, 1.0f);
		SlashSprite->SetRelativeScale3D(SlashBaseScale);
	}
	SetActorEnableCollision(false);
}

FString AReEchoSwordArcActor::ResolveWeaponTexturePath(const FName WeaponVisualKey)
{
	return FReEchoWeaponVisualCatalog::ResolveAttackTexturePath(WeaponVisualKey);
}

void AReEchoSwordArcActor::ConfigureWeaponVisual(const FName WeaponVisualKey)
{
	UMaterialInterface* SpriteMaterial = LoadObject<UMaterialInterface>(
	    nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ResolveWeaponTexturePath(WeaponVisualKey));
	// Missing specialist art falls back explicitly to the known-safe slash, never to a hand-held static texture.
	if (!Texture)
	{
		Texture = LoadObject<UTexture2D>(nullptr,
		                                 FReEchoWeaponVisualCatalog::ResolveAttackTexturePath(TEXT("CrescentBlade")));
	}
	if (!SpriteMaterial || !Texture)
	{
		SlashSprite->SetVisibility(false);
		return;
	}
	UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(SpriteMaterial, this);
	MaterialInstance->SetTextureParameterValue(TEXT("SpriteTexture"), Texture);
	SlashSprite->SetMaterial(0, MaterialInstance);
	const float WorldHeight = WeaponVisualKey == TEXT("Whip") ? 260.0f : 360.0f;
	const float AspectRatio = static_cast<float>(Texture->GetSizeX()) / FMath::Max(1, Texture->GetSizeY());
	SlashBaseScale = FVector(WorldHeight * AspectRatio / 100.0f, WorldHeight / 100.0f, 1.0f);
}

void AReEchoSwordArcActor::InitializeArc(const float SwingDirection, const FName WeaponVisualKey)
{
	if (!SlashSprite)
	{
		return;
	}
	ConfigureWeaponVisual(WeaponVisualKey);
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
