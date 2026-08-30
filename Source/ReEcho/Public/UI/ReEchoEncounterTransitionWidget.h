#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoEncounterTransitionWidget.generated.h"

class UCanvasPanel;
class UAudioComponent;
class UButton;
class UImage;
class UTextBlock;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMediaPlayer;
class UMediaSource;
class UMediaTexture;
class USoundBase;

enum class EReEchoTransitionMediaState : uint8
{
	Closed,
	Opening,
	WaitingForFirstFrame,
	Playing,
	Completed,
	Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoStageCgSkipRequested);

/** Full-screen, non-interactive presentation for the final countdown and post-zero card transition. */
UCLASS()

class REECHO_API UReEchoEncounterTransitionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FReEchoStageCgSkipRequested OnStageCgSkipRequested;

	UReEchoEncounterTransitionWidget(const FObjectInitializer& ObjectInitializer);
	static FVector2D CalculateFillSize(const FVector2D& ViewSize);
	static FVector2D CalculateCardChoiceFrameSize(const FVector2D& ViewSize);
	static FVector2D CalculateCardChoiceFramePosition(const FVector2D& ViewSize);
	static float CalculateCardChoiceToShopBackgroundBlendAlpha(double MediaTimeSeconds);
	static float CalculateCardChoiceToShopCollapseAlpha(double MediaTimeSeconds);
	static FVector2D CalculateCardChoiceToShopCollapseSize(const FVector2D& ViewSize, float CollapseAlpha);
	static float CalculateCardChoiceToShopCollapseOpacity(float CollapseAlpha);
	static float CalculateStageCgMusicVolume(float MasterVolume, float MusicBusVolume);
	static FVector2D CalculateCardChoiceToShopCollapsePosition(const FVector2D& ViewSize,
	                                                           float CollapseAlpha,
	                                                           const FVector2D& TargetCenter);
	bool StartSequence();
	bool StartCardChoiceToShopSequence();
	bool StartStage01To02Sequence();
	void SetStage01To02SkipAvailable(bool bAvailable);
	bool IsStage01To02SkipAvailable() const;
	UMediaPlayer* GetMediaPlayer() const;
	void BeginSequenceFadeOut(float DurationSeconds);
	bool IsSequenceFinished() const;
	bool HasSequenceFailed() const;
	float GetCardChoiceToShopBackgroundBlendAlpha() const;
	bool HasCardChoiceToShopCollapseStarted() const;
	void SetCardChoiceToShopCollapseTargetAbsolute(const FVector2D& AbsoluteCenter);
	bool IsFadeOutFinished() const;
	void ResetPresentation();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildFallbackTree();
	bool StartSequenceWithSource(UMediaSource* Source, bool bOpaqueMedia, FName SequencePurpose);
	void ApplySequenceBrush(bool bOpaqueMedia);
	void UpdateFillLayout(const FGeometry& Geometry);
	void StartStageCgAudio();
	void FailSequence(const TCHAR* Reason);

	UFUNCTION()
	void HandleStageCgSkipClicked();

	UFUNCTION()
	void HandleMediaOpened(FString OpenedUrl);

	UFUNCTION()
	void HandleMediaOpenFailed(FString FailedUrl);

	UFUNCTION()
	void HandleMediaEndReached();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UImage> SequenceImage;

	UPROPERTY(Transient)
	TObjectPtr<UButton> StageCgSkipButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StageCgSkipLabel;

	UPROPERTY(Transient)
	TObjectPtr<UMediaPlayer> MediaPlayer;

	UPROPERTY(Transient)
	TObjectPtr<UMediaTexture> MediaTexture;

	UPROPERTY(Transient)
	TObjectPtr<UMediaSource> MediaSource;

	UPROPERTY(Transient)
	TObjectPtr<UMediaSource> CardChoiceToShopMediaSource;

	UPROPERTY(Transient)
	TObjectPtr<UMediaSource> Stage01To02MediaSource;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> Stage01To02Sound;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Stage01To02AudioComponent;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> MediaMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MediaMaterialInstance;

	bool bSequenceStarted = false;
	bool bSequenceFinished = false;
	bool bSequenceFailed = false;
	bool bOpaqueSequence = false;
	bool bMediaPlaybackStarted = false;
	bool bFadingOut = false;
	bool bLoggedViewportGeometry = false;
	FIntPoint LastLoggedMediaSurface = FIntPoint::ZeroValue;
	float FadeDurationSeconds = 0.4f;
	float FadeElapsedSeconds = 0.0f;
	float FadeStartOpacity = 1.0f;
	float OpaquePlaybackElapsedSeconds = 0.0f;
	float FirstFrameWaitElapsedSeconds = 0.0f;
	float PlaybackStallElapsedSeconds = 0.0f;
	FTimespan LastObservedMediaTime = FTimespan::MinValue();
	EReEchoTransitionMediaState MediaState = EReEchoTransitionMediaState::Closed;
	FName ActiveSequencePurpose = NAME_None;
	TOptional<FVector2D> CardChoiceToShopCollapseTargetAbsolute;
};
