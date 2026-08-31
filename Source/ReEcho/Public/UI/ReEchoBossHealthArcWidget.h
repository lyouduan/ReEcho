#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Styling/SlateBrush.h"
#include "ReEchoBossHealthArcWidget.generated.h"

class SImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** Read-only health projection. Authoring properties stay in the WBP; the transient brush never replaces them. */
UCLASS(meta = (DisplayName = "ReEcho Boss Health Arc"))

class REECHO_API UReEchoBossHealthArcWidget : public UWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss Health Arc")
	TObjectPtr<UMaterialInterface> ArcMaterial;

	/** Remaining-health color; the source clock's ink ticks remain visible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Health Arc")
	FLinearColor FillColor = FLinearColor(0.68f, 0.025f, 0.18f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Health Arc")
	FLinearColor EmptyColor = FLinearColor(0.008f, 0.008f, 0.01f, 1.0f);

	/** 0 = continuous fill, 1 = original black ticks; affects presentation only. */
	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Boss Health Arc",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TickContrast = 0.25f;

	/** Dark, branching detail on remaining health only. 0 disables vein coloration. */
	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Boss Health Arc|Surface",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VeinStrength = 0.4f;

	/** Vein radius in reference-image pixels; does not change the health ring width. */
	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Boss Health Arc|Surface",
	          meta = (ClampMin = "0.5", ClampMax = "4.0"))
	float VeinWidth = 1.3f;

	/** Distance between branches in reference pixels. Larger values mean fewer branches. */
	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Boss Health Arc|Surface",
	          meta = (ClampMin = "20.0", ClampMax = "100.0"))
	float VeinSpacing = 42.0f;

	/** Rounds the entire health strip with upper-left light. Veins stay flat; no opacity/geometry changes. */
	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Boss Health Arc|Surface",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReliefStrength = 0.3f;

	/** Material coordinates in the original clock image, independent of Canvas/DPI scaling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Health Arc|Shape")
	FVector2D ReferenceSize = FVector2D(1159.0, 216.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Health Arc|Shape")
	FVector2D ArcCenter = FVector2D(580.0, 54.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Health Arc|Shape", meta = (ClampMin = "0.0"))
	float InnerRadius = 104.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Health Arc|Shape", meta = (ClampMin = "1.0"))
	float OuterRadius = 126.0f;

	UFUNCTION(BlueprintCallable, Category = "Boss Health Arc")
	void SetHealthRatio(float InRatio);

	UFUNCTION(BlueprintPure, Category = "Boss Health Arc")

	float GetHealthRatio() const
	{
		return HealthRatio;
	}

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void UpdateMaterial();

	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<UMaterialInterface> MaterialSource;

	FSlateBrush DisplayBrush;
	TSharedPtr<SImage> ArcImage;
	float HealthRatio = 1.0f;
};
