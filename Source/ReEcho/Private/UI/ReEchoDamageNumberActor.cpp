#include "UI/ReEchoDamageNumberActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

AReEchoDamageNumberActor::AReEchoDamageNumberActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DamageText"));
	SetRootComponent(Text);
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetWorldSize(52.0f);
	Text->SetXScale(1.0f);
	Text->SetYScale(1.0f);
	Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Text->SetCastShadow(false);
	Text->SetTranslucentSortPriority(20);
	if (UMaterialInterface* UnlitTextMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Engine/EngineMaterials/UnlitText.UnlitText")))
	{
		Text->SetTextMaterial(UnlitTextMaterial);
	}
}

void AReEchoDamageNumberActor::SpawnDamageNumber(
	UWorld* World,
	const FVector& WorldLocation,
	const float Damage,
	const FLinearColor& Color)
{
	if (!World || Damage <= 0.0f)
	{
		return;
	}

	AReEchoDamageNumberActor* DamageNumber =
		World->SpawnActor<AReEchoDamageNumberActor>(
			WorldLocation + FVector(0.0f, 0.0f, 95.0f),
			FRotator::ZeroRotator);
	if (DamageNumber)
	{
		DamageNumber->InitializeDamage(Damage, Color);
	}
}

void AReEchoDamageNumberActor::InitializeDamage(
	const float Damage,
	const FLinearColor& Color)
{
	InitialColor = Color;
	const int32 DisplayDamage = FMath::Max(1, FMath::RoundToInt(Damage));
	Text->SetText(FText::FromString(FString::Printf(TEXT("%d"), DisplayDamage)));
	Text->SetTextRenderColor(InitialColor.ToFColor(false));
}

void AReEchoDamageNumberActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedTime += DeltaSeconds;
	AddActorWorldOffset(FVector(0.0f, 0.0f, 70.0f * DeltaSeconds));

	if (APlayerCameraManager* Camera =
			UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		// 正交相机的所有视线互相平行；统一使用相机前向，避免屏幕边缘文字产生透视式倾斜。
		SetActorRotation((-Camera->GetCameraRotation().Vector()).Rotation());
	}

	const float Alpha = 1.0f - FMath::Clamp(ElapsedTime / DisplayDuration, 0.0f, 1.0f);
	FLinearColor FadedColor = InitialColor;
	FadedColor.A = Alpha;
	Text->SetTextRenderColor(FadedColor.ToFColor(false));
	SetActorScale3D(FVector(FMath::Lerp(1.15f, 0.85f, 1.0f - Alpha)));

	if (ElapsedTime >= DisplayDuration)
	{
		Destroy();
	}
}