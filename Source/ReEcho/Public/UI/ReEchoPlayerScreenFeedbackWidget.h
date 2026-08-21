#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoPlayerScreenFeedbackWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class SWidget;

/** 玩家专属的全屏受伤脉冲与低血量边缘反馈。只保存表现状态，不参与生命或伤害结算。 */
UCLASS(Blueprintable)

class REECHO_API UReEchoPlayerScreenFeedbackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoPlayerScreenFeedbackWidget(const FObjectInitializer& ObjectInitializer);

	void SetHealth(float CurrentHealth, float MaximumHealth);
	void PlayHurtFeedback(float AppliedDamage, float MaximumHealth);
	void ClearFeedback();

	static float CalculateHealthRatio(float CurrentHealth, float MaximumHealth);
	static float CalculateCombinedIntensity(float LowHealthIntensity, float HurtIntensity, float MaximumIntensity);
	static bool ShouldPlayHurtFeedback(float AppliedDamage, bool bBlocked);

#if WITH_DEV_AUTOMATION_TESTS
	float GetLowHealthIntensityForTests() const
	{
		return LowHealthIntensity;
	}

	float GetTargetLowHealthIntensityForTests() const
	{
		return TargetLowHealthIntensity;
	}

	float GetHurtIntensityForTests() const
	{
		return HurtIntensity;
	}

	float GetHurtElapsedSecondsForTests() const
	{
		return HurtElapsedSeconds;
	}

	bool HasVignetteVisualForTests() const
	{
		return HurtVignetteImage != nullptr;
	}
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Screen Feedback|Low Health",
	          meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float LowHealthThreshold = 0.5f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Screen Feedback|Low Health",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaximumLowHealthIntensity = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen Feedback|Low Health", meta = (ClampMin = "0.1"))
	float LowHealthCurveExponent = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen Feedback|Low Health", meta = (ClampMin = "0.0"))
	float LowHealthInterpolationSpeed = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen Feedback|Low Health", meta = (ClampMin = "0.0"))
	float LowHealthPulseFrequency = 1.2f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Screen Feedback|Low Health",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthPulseAmplitude = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen Feedback|Hurt", meta = (ClampMin = "0.01"))
	float HurtPulseDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Screen Feedback|Hurt",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinimumHurtIntensity = 0.55f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Screen Feedback|Hurt",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaximumHurtIntensity = 0.8f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Screen Feedback",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaximumCombinedIntensity = 0.85f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen Feedback")
	FLinearColor FeedbackColor = FLinearColor(0.65f, 0.01f, 0.005f, 1.0f);

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Screen Feedback",
	          meta = (ClampMin = "0.01", ClampMax = "0.5"))
	float MaterialEdgeWidth = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen Feedback", meta = (ClampMin = "0.1"))
	float MaterialSoftness = 2.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen Feedback|Fallback", meta = (ClampMin = "0.0"))
	float FallbackEdgeWidth = 260.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen Feedback")
	TObjectPtr<UMaterialInterface> HurtVignetteMaterial;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> HurtVignetteImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> LeftHurtEdge;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RightHurtEdge;

private:
	void BuildFallbackWidgetTree();
	bool ConfigureVignetteMaterial();
	void RefreshVisual();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> VignetteMaterialInstance;

	float TargetLowHealthIntensity = 0.0f;
	float LowHealthIntensity = 0.0f;
	float HurtIntensity = 0.0f;
	float HurtElapsedSeconds = 0.0f;
	float PulseElapsedSeconds = 0.0f;
};
