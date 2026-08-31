#include "UI/ReEchoBossHealthArcWidget.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/Images/SImage.h"

TSharedRef<SWidget> UReEchoBossHealthArcWidget::RebuildWidget()
{
	ArcImage = SNew(SImage).Image(&DisplayBrush);
	return ArcImage.ToSharedRef();
}

void UReEchoBossHealthArcWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	UpdateMaterial();
}

void UReEchoBossHealthArcWidget::ReleaseSlateResources(const bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	ArcImage.Reset();
}

void UReEchoBossHealthArcWidget::SetHealthRatio(const float InRatio)
{
	HealthRatio = FMath::IsFinite(InRatio) ? FMath::Clamp(InRatio, 0.0f, 1.0f) : 0.0f;
	UpdateMaterial();
}

void UReEchoBossHealthArcWidget::UpdateMaterial()
{
	if (MaterialSource != ArcMaterial || (!DynamicMaterial && ArcMaterial))
	{
		MaterialSource = ArcMaterial;
		DynamicMaterial = ArcMaterial ? UMaterialInstanceDynamic::Create(ArcMaterial, this) : nullptr;
	}
	// The native brush is not duplicated with a Blueprint widget tree. Always
	// reconnect it, even if this instance already has a dynamic material.
	DisplayBrush.SetResourceObject(DynamicMaterial);

	DisplayBrush.ImageSize = FVector2f(ReferenceSize);
	DisplayBrush.DrawAs = DynamicMaterial ? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(TEXT("HealthRatio"), HealthRatio);
		DynamicMaterial->SetVectorParameterValue(TEXT("FillColor"), FillColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("EmptyColor"), EmptyColor);
		DynamicMaterial->SetScalarParameterValue(TEXT("TickContrast"), FMath::Clamp(TickContrast, 0.0f, 1.0f));
		DynamicMaterial->SetScalarParameterValue(TEXT("VeinStrength"), FMath::Clamp(VeinStrength, 0.0f, 1.0f));
		DynamicMaterial->SetScalarParameterValue(TEXT("VeinWidth"), FMath::Clamp(VeinWidth, 0.5f, 4.0f));
		DynamicMaterial->SetScalarParameterValue(TEXT("VeinSpacing"), FMath::Clamp(VeinSpacing, 20.0f, 100.0f));
		DynamicMaterial->SetScalarParameterValue(TEXT("ReliefStrength"), FMath::Clamp(ReliefStrength, 0.0f, 1.0f));
		DynamicMaterial->SetVectorParameterValue(TEXT("ReferenceSize"),
		                                         FLinearColor(ReferenceSize.X, ReferenceSize.Y, 0.0f, 0.0f));
		DynamicMaterial->SetVectorParameterValue(TEXT("ArcCenter"), FLinearColor(ArcCenter.X, ArcCenter.Y, 0.0f, 0.0f));
		DynamicMaterial->SetScalarParameterValue(TEXT("InnerRadius"), FMath::Max(0.0f, InnerRadius));
		DynamicMaterial->SetScalarParameterValue(TEXT("OuterRadius"), FMath::Max(InnerRadius + 1.0f, OuterRadius));
	}
	if (ArcImage)
	{
		ArcImage->SetImage(&DisplayBrush);
		ArcImage->InvalidateImage();
	}
}

#if WITH_EDITOR
const FText UReEchoBossHealthArcWidget::GetPaletteCategory()
{
	return NSLOCTEXT("ReEcho", "BossHealthArcPaletteCategory", "ReEcho");
}
#endif
