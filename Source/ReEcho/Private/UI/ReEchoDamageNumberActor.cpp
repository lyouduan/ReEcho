#include "UI/ReEchoDamageNumberActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
/** 暴击跳字基准世界字号，与普通伤害跳字一致。 */
constexpr float DamageNumberBaseWorldSize = 52.0f;
/** 下划线略微加宽，确保在不同数字组合下仍能清楚覆盖数字底部。 */
constexpr float CriticalUnderlineXScale = 1.18f;
/** TextRender 的缩放原点会使加宽后的下划线向右偏，向左补偿。 */
constexpr float CriticalUnderlineHorizontalOffset = -0.10f;
/** 与数字底部拉开距离，避免下划线贴住字形。 */
constexpr float CriticalUnderlineVerticalOffset = -0.58f;
}

const TCHAR* AReEchoDamageNumberActor::GetDamageNumberFontPath()
{
	return TEXT("/Game/ReEcho/Fonts/DamageNumbers/F_DamageNumber_MFYuYue_Font.F_DamageNumber_MFYuYue_Font");
}

const TCHAR* AReEchoDamageNumberActor::GetDamageNumberMaterialPath()
{
	return TEXT("/Game/ReEcho/Fonts/DamageNumbers/M_DamageNumberTextOpacity.M_DamageNumberTextOpacity");
}

const TCHAR* AReEchoDamageNumberActor::GetDamageNumberBlueprintClassPath()
{
	return TEXT("/Game/ReEcho/UI/CombatHud/BP_ReEchoDamageNumber.BP_ReEchoDamageNumber_C");
}

AReEchoDamageNumberActor::AReEchoDamageNumberActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DamageText"));
	SetRootComponent(Text);
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetWorldSize(DamageNumberBaseWorldSize);
	Text->SetXScale(1.0f);
	Text->SetYScale(1.0f);
	Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Text->SetCastShadow(false);
	Text->SetTranslucentSortPriority(20);
	static ConstructorHelpers::FObjectFinder<UFont> DamageNumberFont(GetDamageNumberFontPath());
	if (DamageNumberFont.Succeeded())
	{
		Text->SetFont(DamageNumberFont.Object);
	}
	if (UMaterialInterface* TranslucentTextMaterial =
	        LoadObject<UMaterialInterface>(nullptr, GetDamageNumberMaterialPath()))
	{
		Text->SetTextMaterial(TranslucentTextMaterial);
	}

	Underline = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DamageUnderline"));
	Underline->SetupAttachment(Text);
	Underline->SetVisibility(false);
	Underline->SetHorizontalAlignment(EHTA_Center);
	Underline->SetVerticalAlignment(EVRTA_TextCenter);
	Underline->SetWorldSize(DamageNumberBaseWorldSize);
	Underline->SetXScale(1.0f);
	Underline->SetYScale(1.0f);
	Underline->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Underline->SetCastShadow(false);
	Underline->SetTranslucentSortPriority(21);
	if (DamageNumberFont.Succeeded())
	{
		Underline->SetFont(DamageNumberFont.Object);
	}
	if (UMaterialInterface* TranslucentTextMaterial =
	        LoadObject<UMaterialInterface>(nullptr, GetDamageNumberMaterialPath()))
	{
		Underline->SetTextMaterial(TranslucentTextMaterial);
	}
}

void AReEchoDamageNumberActor::SpawnDamageNumber(UWorld* World,
                                                 const FVector& WorldLocation,
                                                 const float Damage,
                                                 const FLinearColor& Color,
                                                 const float SizeScale,
                                                 const bool bUnderline)
{
	if (!World || Damage <= 0.0f)
	{
		return;
	}

	UClass* DamageNumberClass = LoadClass<AReEchoDamageNumberActor>(nullptr, GetDamageNumberBlueprintClassPath());
	if (!DamageNumberClass)
	{
		DamageNumberClass = StaticClass();
	}

	AReEchoDamageNumberActor* DamageNumber = World->SpawnActor<AReEchoDamageNumberActor>(
	    DamageNumberClass, WorldLocation + FVector(0.0f, 0.0f, 95.0f), FRotator::ZeroRotator);
	if (DamageNumber)
	{
		DamageNumber->InitializeDamage(Damage, Color, SizeScale, bUnderline);
	}
}

void AReEchoDamageNumberActor::InitializeDamage(const float Damage, const FLinearColor& Color, const float SizeScale, const bool bUnderline)
{
	InitialColor = Color;
	const float EffectiveSizeScale = bUnderline ? FMath::Max(1.35f, SizeScale) : SizeScale;
	const int32 DisplayDamage = FMath::Max(1, FMath::RoundToInt(Damage));
	const FString DamageText = FString::Printf(TEXT("%d"), DisplayDamage);
	Text->SetText(FText::FromString(DamageText));
	Text->SetWorldSize(DamageNumberBaseWorldSize * EffectiveSizeScale);
	Text->SetTextRenderColor(InitialColor.ToFColor(false));

	const bool bShowUnderline = bUnderline && Underline != nullptr;
	Underline->SetVisibility(bShowUnderline);
	Underline->SetXScale(bShowUnderline ? CriticalUnderlineXScale : 1.0f);
	if (bShowUnderline)
	{
		// 每三位数字对应一条下划线，按位数向上取整，避免长数字下划线过密。
		const int32 UnderlineCount = FMath::Max(1, (DamageText.Len() + 2) / 3);
		Underline->SetText(FText::FromString(FString::ChrN(UnderlineCount, '_')));
		Underline->SetWorldSize(DamageNumberBaseWorldSize * EffectiveSizeScale);
		// TextRenderComponent 文字向上为 +Y；下划线左移并置于字底下方约 0.58 字高处。
		Underline->SetRelativeLocation(
		    FVector(
		        DamageNumberBaseWorldSize * EffectiveSizeScale * CriticalUnderlineHorizontalOffset,
		        DamageNumberBaseWorldSize * EffectiveSizeScale * CriticalUnderlineVerticalOffset,
		        0.0f));
		Underline->SetTextRenderColor(InitialColor.ToFColor(false));
	}
}

void AReEchoDamageNumberActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedTime += DeltaSeconds;
	AddActorWorldOffset(FVector(0.0f, 0.0f, FloatSpeed * DeltaSeconds));

	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		// 正交相机的所有视线互相平行；统一使用相机前向，避免屏幕边缘文字产生透视式倾斜。
		SetActorRotation((-Camera->GetCameraRotation().Vector()).Rotation());
	}

	const float LifeProgress = DisplayDuration > 0.0f ? FMath::Clamp(ElapsedTime / DisplayDuration, 0.0f, 1.0f) : 1.0f;
	SetActorScale3D(FVector(FMath::Lerp(StartScale, EndScale, LifeProgress)));

	if (ElapsedTime >= DisplayDuration)
	{
		Destroy();
	}
}
