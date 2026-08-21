#include "UI/ReEchoPlayerScreenFeedbackWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"

UReEchoPlayerScreenFeedbackWidget::UReEchoPlayerScreenFeedbackWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VignetteMaterialFinder(
	    TEXT("/Game/ReEcho/Materials/UI/M_UI_PlayerHurtVignette.M_UI_PlayerHurtVignette"));
	HurtVignetteMaterial = VignetteMaterialFinder.Object;
}

TSharedRef<SWidget> UReEchoPlayerScreenFeedbackWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildFallbackWidgetTree();
	}
	return Super::RebuildWidget();
}

void UReEchoPlayerScreenFeedbackWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackWidgetTree();
	ConfigureVignetteMaterial();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	RefreshVisual();
}

void UReEchoPlayerScreenFeedbackWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	PulseElapsedSeconds += InDeltaTime;
	LowHealthIntensity =
	    FMath::FInterpTo(LowHealthIntensity, TargetLowHealthIntensity, InDeltaTime, LowHealthInterpolationSpeed);
	if (HurtIntensity > 0.0f)
	{
		HurtElapsedSeconds += InDeltaTime;
		const float Alpha =
		    FMath::Clamp(HurtElapsedSeconds / FMath::Max(HurtPulseDuration, UE_SMALL_NUMBER), 0.0f, 1.0f);
		HurtIntensity = FMath::Lerp(HurtIntensity, 0.0f, Alpha);
		if (Alpha >= 1.0f)
		{
			HurtIntensity = 0.0f;
		}
	}
	RefreshVisual();
}

void UReEchoPlayerScreenFeedbackWidget::SetHealth(const float CurrentHealth, const float MaximumHealth)
{
	const float HealthRatio = CalculateHealthRatio(CurrentHealth, MaximumHealth);
	if (HealthRatio <= 0.0f)
	{
		TargetLowHealthIntensity = 0.0f;
		LowHealthIntensity = 0.0f;
		HurtIntensity = 0.0f;
		RefreshVisual();
		return;
	}

	const float LowHealthAlpha =
	    FMath::Clamp((LowHealthThreshold - HealthRatio) / FMath::Max(LowHealthThreshold, UE_SMALL_NUMBER), 0.0f, 1.0f);
	TargetLowHealthIntensity = FMath::Clamp(
	    FMath::Pow(LowHealthAlpha, LowHealthCurveExponent) * MaximumLowHealthIntensity, 0.0f, MaximumCombinedIntensity);
}

void UReEchoPlayerScreenFeedbackWidget::PlayHurtFeedback(const float AppliedDamage, const float MaximumHealth)
{
	const float DamageRatio = FMath::Clamp(AppliedDamage / FMath::Max(MaximumHealth, 1.0f), 0.0f, 1.0f);
	HurtIntensity =
	    FMath::Lerp(MinimumHurtIntensity, MaximumHurtIntensity, FMath::Clamp(DamageRatio / 0.25f, 0.0f, 1.0f));
	HurtElapsedSeconds = 0.0f;
	RefreshVisual();
}

void UReEchoPlayerScreenFeedbackWidget::ClearFeedback()
{
	TargetLowHealthIntensity = 0.0f;
	LowHealthIntensity = 0.0f;
	HurtIntensity = 0.0f;
	HurtElapsedSeconds = 0.0f;
	RefreshVisual();
}

float UReEchoPlayerScreenFeedbackWidget::CalculateHealthRatio(const float CurrentHealth, const float MaximumHealth)
{
	return FMath::Clamp(CurrentHealth / FMath::Max(MaximumHealth, 1.0f), 0.0f, 1.0f);
}

float UReEchoPlayerScreenFeedbackWidget::CalculateCombinedIntensity(const float InLowHealthIntensity,
                                                                    const float InHurtIntensity,
                                                                    const float MaximumIntensity)
{
	return FMath::Clamp(InLowHealthIntensity + InHurtIntensity * 0.65f, 0.0f, FMath::Max(0.0f, MaximumIntensity));
}

bool UReEchoPlayerScreenFeedbackWidget::ShouldPlayHurtFeedback(const float AppliedDamage, const bool bBlocked)
{
	return AppliedDamage > 0.0f && !bBlocked;
}

void UReEchoPlayerScreenFeedbackWidget::BuildFallbackWidgetTree()
{
	if (LeftHurtEdge || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas =
	    WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FeedbackRoot"));
	RootCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = RootCanvas;

	HurtVignetteImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HurtVignetteImage"));
	HurtVignetteImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* VignetteSlot = RootCanvas->AddChildToCanvas(HurtVignetteImage);
	VignetteSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	VignetteSlot->SetOffsets(FMargin(0.0f));

	LeftHurtEdge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LeftHurtEdge"));
	LeftHurtEdge->SetBrushColor(FeedbackColor);
	UCanvasPanelSlot* LeftSlot = RootCanvas->AddChildToCanvas(LeftHurtEdge);
	LeftSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 1.0f));
	LeftSlot->SetOffsets(FMargin(0.0f, 0.0f, FallbackEdgeWidth, 0.0f));
	LeftSlot->SetZOrder(-1);

	RightHurtEdge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightHurtEdge"));
	RightHurtEdge->SetBrushColor(FeedbackColor);
	UCanvasPanelSlot* RightSlot = RootCanvas->AddChildToCanvas(RightHurtEdge);
	RightSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 1.0f));
	RightSlot->SetAlignment(FVector2D(1.0f, 0.0f));
	RightSlot->SetOffsets(FMargin(0.0f, 0.0f, FallbackEdgeWidth, 0.0f));
	RightSlot->SetZOrder(-1);
	ConfigureVignetteMaterial();
}

bool UReEchoPlayerScreenFeedbackWidget::ConfigureVignetteMaterial()
{
	if (!HurtVignetteImage)
	{
		return false;
	}

	UMaterialInterface* Material = HurtVignetteMaterial;
	if (!Material)
	{
		HurtVignetteImage->SetVisibility(ESlateVisibility::Collapsed);
		return false;
	}

	HurtVignetteImage->SetBrushFromMaterial(Material);
	VignetteMaterialInstance = HurtVignetteImage->GetDynamicMaterial();
	if (!VignetteMaterialInstance)
	{
		HurtVignetteImage->SetVisibility(ESlateVisibility::Collapsed);
		return false;
	}

	VignetteMaterialInstance->SetVectorParameterValue(TEXT("TintColor"), FeedbackColor);
	VignetteMaterialInstance->SetScalarParameterValue(TEXT("EdgeWidth"), MaterialEdgeWidth);
	VignetteMaterialInstance->SetScalarParameterValue(TEXT("Softness"), MaterialSoftness);
	HurtVignetteImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (LeftHurtEdge)
	{
		LeftHurtEdge->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (RightHurtEdge)
	{
		RightHurtEdge->SetVisibility(ESlateVisibility::Collapsed);
	}
	return true;
}

void UReEchoPlayerScreenFeedbackWidget::RefreshVisual()
{
	if (!HurtVignetteImage && (!LeftHurtEdge || !RightHurtEdge))
	{
		return;
	}

	const float Pulse =
	    1.0f + FMath::Sin(PulseElapsedSeconds * LowHealthPulseFrequency * 2.0f * UE_PI) * LowHealthPulseAmplitude;
	const float Intensity =
	    CalculateCombinedIntensity(LowHealthIntensity * Pulse, HurtIntensity, MaximumCombinedIntensity);
	if (HurtVignetteImage && VignetteMaterialInstance)
	{
		HurtVignetteImage->SetRenderOpacity(Intensity);
	}
	else
	{
		LeftHurtEdge->SetRenderOpacity(Intensity);
		RightHurtEdge->SetRenderOpacity(Intensity);
	}
}
