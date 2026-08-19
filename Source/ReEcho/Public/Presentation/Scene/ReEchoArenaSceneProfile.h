#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ReEchoArenaSceneProfile.generated.h"

class UMaterialInterface;

/** Editor-authored visual defaults for one arena theme. Never owns gameplay bounds or encounter state. */
UCLASS(BlueprintType)

class REECHO_API UReEchoArenaSceneProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Profile")
	FName SceneId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Profile|Ground")
	TObjectPtr<UMaterialInterface> MapMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Profile|Ground")
	FLinearColor GroundTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Profile|Ground", meta = (ClampMin = "0.0"))
	float GroundBrightness = 1.0f;

	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Arena Profile|Ground",
	          meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float GroundSaturation = 1.0f;

	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Arena Profile|Ground",
	          meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float GroundContrast = 1.0f;

	/** Materials allowed for low, non-occluding ground detail cards. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Profile|Decoration")
	TArray<TObjectPtr<UMaterialInterface>> GroundDetailPalette;

	/** Materials allowed to share footpoint sorting with characters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Profile|Decoration")
	TArray<TObjectPtr<UMaterialInterface>> MidDecorationPalette;

	/** Materials reserved for the camera-edge foreground frame. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Profile|Decoration")
	TArray<TObjectPtr<UMaterialInterface>> ForegroundPalette;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Profile|Decoration", meta = (ClampMin = "0"))
	int32 DecorationSeed = 52001;

	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Arena Profile|Decoration",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DecorationDensity = 0.35f;

	/** Normalized half-extent of the center area where generated tall cards are forbidden. */
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Arena Profile|Decoration",
	          meta = (ClampMin = "0.0", ClampMax = "0.9"))
	FVector2D CenterSafeZoneRatio = FVector2D(0.6f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Profile|Decoration", meta = (ClampMin = "0"))
	int32 MaximumLandmarks = 1;
};
