#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoEncounterTransitionWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMediaPlayer;
class UMediaSource;
class UMediaTexture;

/** Full-screen, non-interactive presentation for the final countdown and post-zero card transition. */
UCLASS()

class REECHO_API UReEchoEncounterTransitionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoEncounterTransitionWidget(const FObjectInitializer& ObjectInitializer);
	static float CalculateCountdownIntensity(float RemainingTime);
	static FVector2D CalculateFillSize(const FVector2D& ViewSize);
	void SetCountdownIntensity(float Intensity);
	bool StartSequence();
	void BeginSequenceFadeOut(float DurationSeconds);
	bool IsSequenceFinished() const;
	bool HasSequenceFailed() const;
	bool IsFadeOutFinished() const;
	void ResetPresentation();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildFallbackTree();
	void UpdateFillLayout(const FVector2D& ViewSize);

	UFUNCTION()
	void HandleMediaOpened(FString OpenedUrl);

	UFUNCTION()
	void HandleMediaOpenFailed(FString FailedUrl);

	UFUNCTION()
	void HandleMediaEndReached();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> CountdownWash;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> CountdownPulse;

	UPROPERTY(Transient)
	TObjectPtr<UImage> SequenceImage;

	UPROPERTY(Transient)
	TObjectPtr<UMediaPlayer> MediaPlayer;

	UPROPERTY(Transient)
	TObjectPtr<UMediaTexture> MediaTexture;

	UPROPERTY(Transient)
	TObjectPtr<UMediaSource> MediaSource;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> MediaMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MediaMaterialInstance;

	bool bSequenceStarted = false;
	bool bSequenceFinished = false;
	bool bSequenceFailed = false;
	bool bFadingOut = false;
	bool bLoggedViewportGeometry = false;
	FIntPoint LastLoggedMediaSurface = FIntPoint::ZeroValue;
	float FadeDurationSeconds = 0.4f;
	float FadeElapsedSeconds = 0.0f;
};
