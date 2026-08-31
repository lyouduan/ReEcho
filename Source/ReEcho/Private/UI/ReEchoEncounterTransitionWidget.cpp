#include "UI/ReEchoEncounterTransitionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "MediaTexture.h"
#include "ReEchoAudioService.h"
#include "ReEchoAudioTypes.h"
#include "Sound/SoundBase.h"

namespace
{
constexpr float SequenceWidth = 1920.0f;
constexpr float SequenceHeight = 1080.0f;
constexpr float CardChoiceFrameLeft = 0.17f;
constexpr float CardChoiceFrameTop = 87.0f / SequenceHeight;
constexpr float CardChoiceFrameRight = 0.83f;
constexpr float CardChoiceFrameBottom = CardChoiceFrameTop + 0.72f;
constexpr double CardChoiceToShopCollapseStartSeconds = 10.0 / 40.0;
constexpr double CardChoiceToShopCollapseEndSeconds = 62.0 / 40.0;
constexpr double CardChoiceToShopBackgroundBlendEndSeconds = 20.0 / 40.0;
constexpr float CardChoiceToShopFinalScale = 0.575f;
constexpr double OpaqueMediaCompletionGraceSeconds = 5.0;
constexpr float FirstFrameTimeoutSeconds = 5.0f;
constexpr float PlaybackStallTimeoutSeconds = 5.0f;
}

UReEchoEncounterTransitionWidget::UReEchoEncounterTransitionWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UMediaSource> SourceFinder(
	    TEXT("/Game/ReEcho/UI/EncounterTransition/FMS_EncounterEndToCardChoiceV2.FMS_EncounterEndToCardChoiceV2"));
	static ConstructorHelpers::FObjectFinder<UMediaSource> CardChoiceToShopSourceFinder(
	    TEXT("/Game/ReEcho/UI/EncounterTransition/FMS_CardChoiceToShop.FMS_CardChoiceToShop"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
	    TEXT("/Game/ReEcho/UI/EncounterTransition/M_UI_EncounterTransition.M_UI_EncounterTransition"));
	static ConstructorHelpers::FObjectFinder<UMediaSource> Stage01To02SourceFinder(
	    TEXT("/Game/ReEcho/UI/EncounterTransition/FMS_Stage01To02.FMS_Stage01To02"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Stage01To02SoundFinder(
	    TEXT("/Game/ReEcho/UI/EncounterTransition/S_Stage01To02.S_Stage01To02"));
	MediaSource = SourceFinder.Object;
	CardChoiceToShopMediaSource = CardChoiceToShopSourceFinder.Object;
	Stage01To02MediaSource = Stage01To02SourceFinder.Object;
	Stage01To02Sound = Stage01To02SoundFinder.Object;
	MediaMaterial = MaterialFinder.Object;
}

FVector2D UReEchoEncounterTransitionWidget::CalculateFillSize(const FVector2D& ViewSize)
{
	if (ViewSize.X <= 0.0f || ViewSize.Y <= 0.0f)
	{
		return FVector2D::ZeroVector;
	}
	const float FillScale = FMath::Max(ViewSize.X / SequenceWidth, ViewSize.Y / SequenceHeight);
	return FVector2D(SequenceWidth * FillScale, SequenceHeight * FillScale);
}

void UReEchoEncounterTransitionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetAnchorsInViewport(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	SetAlignmentInViewport(FVector2D::ZeroVector);
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ResetPresentation();
}

TSharedRef<SWidget> UReEchoEncounterTransitionWidget::RebuildWidget()
{
	// A native UUserWidget must populate WidgetTree before Super::RebuildWidget() snapshots it
	// into the Slate hierarchy. Building it in NativeConstruct is too late: the UObject children
	// tick and report geometry, but they are not painted.
	BuildFallbackTree();
	return Super::RebuildWidget();
}

void UReEchoEncounterTransitionWidget::NativeDestruct()
{
	if (MediaPlayer)
	{
		MediaPlayer->OnMediaOpened.RemoveAll(this);
		MediaPlayer->OnMediaOpenFailed.RemoveAll(this);
		MediaPlayer->OnEndReached.RemoveAll(this);
		MediaPlayer->Close();
	}
	if (Stage01To02AudioComponent)
	{
		Stage01To02AudioComponent->Stop();
		Stage01To02AudioComponent = nullptr;
	}
	Super::NativeDestruct();
}

void UReEchoEncounterTransitionWidget::BuildFallbackTree()
{
	if (WidgetTree->RootWidget)
	{
		RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
		return;
	}

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TransitionRoot"));
	RootCanvas->SetClipping(EWidgetClipping::ClipToBounds);
	WidgetTree->RootWidget = RootCanvas;

	SequenceImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SequenceImage"));
	SequenceImage->SetVisibility(ESlateVisibility::Collapsed);
	RootCanvas->AddChildToCanvas(SequenceImage);

	StageCgSkipButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StageCgSkipButton"));
	StageCgSkipButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.5f));
	StageCgSkipButton->SetVisibility(ESlateVisibility::Collapsed);
	StageCgSkipButton->OnClicked.AddDynamic(this, &UReEchoEncounterTransitionWidget::HandleStageCgSkipClicked);
	StageCgSkipLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StageCgSkipLabel"));
	StageCgSkipLabel->SetText(FText::FromString(TEXT("跳过")));
	StageCgSkipLabel->SetJustification(ETextJustify::Center);
	StageCgSkipButton->AddChild(StageCgSkipLabel);
	if (UCanvasPanelSlot* SkipSlot = RootCanvas->AddChildToCanvas(StageCgSkipButton))
	{
		SkipSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		SkipSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		SkipSlot->SetPosition(FVector2D(-48.0f, 36.0f));
		SkipSlot->SetSize(FVector2D(140.0f, 54.0f));
	}

	if (!MediaSource || !MediaMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("Encounter transition authored asset chain is incomplete."));
		return;
	}
	MediaPlayer = NewObject<UMediaPlayer>(this, TEXT("EncounterTransitionRuntimePlayer"));
	MediaTexture = NewObject<UMediaTexture>(this, TEXT("EncounterTransitionRuntimeTexture"));
	MediaMaterialInstance =
	    UMaterialInstanceDynamic::Create(MediaMaterial, this, TEXT("EncounterTransitionRuntimeMaterial"));
	if (!MediaPlayer || !MediaTexture || !MediaMaterialInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("Encounter transition runtime media chain could not be created."));
		return;
	}
	MediaPlayer->OnMediaOpened.RemoveAll(this);
	MediaPlayer->OnMediaOpenFailed.RemoveAll(this);
	MediaPlayer->OnEndReached.RemoveAll(this);
	MediaPlayer->OnMediaOpened.AddDynamic(this, &UReEchoEncounterTransitionWidget::HandleMediaOpened);
	MediaPlayer->OnMediaOpenFailed.AddDynamic(this, &UReEchoEncounterTransitionWidget::HandleMediaOpenFailed);
	MediaPlayer->OnEndReached.AddDynamic(this, &UReEchoEncounterTransitionWidget::HandleMediaEndReached);
	// UMediaPlayer defaults PlayOnOpen to true and invokes it after OnMediaOpened. This widget
	// explicitly seeks to zero and starts playback in that callback, so leaving the default enabled
	// issues two back-to-back SetRate(1) requests and prevents Electra from presenting its first sample.
	MediaPlayer->PlayOnOpen = false;
	MediaPlayer->SetLooping(false);
	MediaTexture->AutoClear = true;
	MediaTexture->ClearColor = FLinearColor::Black;
	MediaTexture->NewStyleOutput = true;
	MediaTexture->SetMediaPlayer(MediaPlayer);
	MediaTexture->UpdateResource();
	MediaMaterialInstance->SetTextureParameterValue(TEXT("MediaTexture"), MediaTexture);
	FSlateBrush MediaBrush;
	MediaBrush.SetResourceObject(MediaMaterialInstance);
	MediaBrush.ImageSize = FVector2D(SequenceWidth, SequenceHeight);
	MediaBrush.DrawAs = ESlateBrushDrawType::Image;
	MediaBrush.TintColor = FSlateColor(FLinearColor::White);
	SequenceImage->SetBrush(MediaBrush);
	SequenceImage->SetBrushResourceObject(MediaMaterialInstance);
	SequenceImage->SetColorAndOpacity(FLinearColor::White);
	UE_LOG(
	    LogTemp, Display, TEXT("Encounter transition UI material bound to runtime MediaTexture (NewStyleOutput=1)."));
}

bool UReEchoEncounterTransitionWidget::StartSequence()
{
	return StartSequenceWithSource(MediaSource, false, TEXT("EncounterEndToCardChoice"));
}

bool UReEchoEncounterTransitionWidget::StartCardChoiceToShopSequence()
{
	ResetPresentation();
	return StartSequenceWithSource(CardChoiceToShopMediaSource, false, TEXT("CardChoiceToShop"));
}

bool UReEchoEncounterTransitionWidget::StartStage01To02Sequence()
{
	return StartSequenceWithSource(Stage01To02MediaSource, true, TEXT("Stage01To02"));
}

void UReEchoEncounterTransitionWidget::SetStage01To02SkipAvailable(const bool bAvailable)
{
	BuildFallbackTree();
	if (StageCgSkipButton)
	{
		StageCgSkipButton->SetIsEnabled(bAvailable);
		StageCgSkipButton->SetVisibility(bAvailable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

bool UReEchoEncounterTransitionWidget::IsStage01To02SkipAvailable() const
{
	return StageCgSkipButton && StageCgSkipButton->GetVisibility() == ESlateVisibility::Visible &&
	       StageCgSkipButton->GetIsEnabled();
}

void UReEchoEncounterTransitionWidget::HandleStageCgSkipClicked()
{
	if (!IsStage01To02SkipAvailable() || ActiveSequencePurpose != TEXT("Stage01To02"))
	{
		return;
	}
	StageCgSkipButton->SetIsEnabled(false);
	OnStageCgSkipRequested.Broadcast();
}

UMediaPlayer* UReEchoEncounterTransitionWidget::GetMediaPlayer() const
{
	return MediaPlayer;
}

FVector2D UReEchoEncounterTransitionWidget::CalculateCardChoiceFrameSize(const FVector2D& ViewSize)
{
	if (ViewSize.X <= 0.0f || ViewSize.Y <= 0.0f)
	{
		return FVector2D::ZeroVector;
	}
	const FVector2D FrameSize(ViewSize.X * (CardChoiceFrameRight - CardChoiceFrameLeft),
	                          ViewSize.Y * (CardChoiceFrameBottom - CardChoiceFrameTop));
	const float FitScale = FMath::Min(FrameSize.X / SequenceWidth, FrameSize.Y / SequenceHeight);
	return FVector2D(SequenceWidth * FitScale, SequenceHeight * FitScale);
}

FVector2D UReEchoEncounterTransitionWidget::CalculateCardChoiceFramePosition(const FVector2D& ViewSize)
{
	const FVector2D MediaSize = CalculateCardChoiceFrameSize(ViewSize);
	const float FrameWidth = ViewSize.X * (CardChoiceFrameRight - CardChoiceFrameLeft);
	return FVector2D(ViewSize.X * CardChoiceFrameLeft + (FrameWidth - MediaSize.X) * 0.5f,
	                 ViewSize.Y * CardChoiceFrameTop);
}

float UReEchoEncounterTransitionWidget::CalculateCardChoiceToShopCollapseAlpha(const double MediaTimeSeconds)
{
	const float LinearAlpha =
	    static_cast<float>((MediaTimeSeconds - CardChoiceToShopCollapseStartSeconds) /
	                       (CardChoiceToShopCollapseEndSeconds - CardChoiceToShopCollapseStartSeconds));
	return FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(LinearAlpha, 0.0f, 1.0f));
}

float UReEchoEncounterTransitionWidget::CalculateCardChoiceToShopBackgroundBlendAlpha(const double MediaTimeSeconds)
{
	return FMath::SmoothStep(
	    0.0f,
	    1.0f,
	    FMath::Clamp(static_cast<float>(MediaTimeSeconds / CardChoiceToShopBackgroundBlendEndSeconds), 0.0f, 1.0f));
}

FVector2D UReEchoEncounterTransitionWidget::CalculateCardChoiceToShopCollapseSize(const FVector2D& ViewSize,
                                                                                  const float CollapseAlpha)
{
	const float Scale = FMath::Lerp(1.0f, CardChoiceToShopFinalScale, FMath::Clamp(CollapseAlpha, 0.0f, 1.0f));
	return CalculateCardChoiceFrameSize(ViewSize) * Scale;
}

float UReEchoEncounterTransitionWidget::CalculateCardChoiceToShopCollapseOpacity(const float CollapseAlpha)
{
	return 1.0f - FMath::Clamp(CollapseAlpha, 0.0f, 1.0f);
}

FVector2D UReEchoEncounterTransitionWidget::CalculateCardChoiceToShopCollapsePosition(const FVector2D& ViewSize,
                                                                                      const float CollapseAlpha,
                                                                                      const FVector2D& TargetCenter)
{
	const float ClampedAlpha = FMath::Clamp(CollapseAlpha, 0.0f, 1.0f);
	const FVector2D InitialSize = CalculateCardChoiceFrameSize(ViewSize);
	const FVector2D CurrentSize = CalculateCardChoiceToShopCollapseSize(ViewSize, ClampedAlpha);
	const FVector2D InitialCenter = CalculateCardChoiceFramePosition(ViewSize) + InitialSize * 0.5f;
	const FVector2D CurrentCenter = FMath::Lerp(InitialCenter, TargetCenter, ClampedAlpha);
	return CurrentCenter - CurrentSize * 0.5f;
}

bool UReEchoEncounterTransitionWidget::StartSequenceWithSource(UMediaSource* Source,
                                                               const bool bOpaqueMedia,
                                                               const FName SequencePurpose)
{
	BuildFallbackTree();
	if (bSequenceStarted)
	{
		return !bSequenceFailed;
	}
	bSequenceStarted = true;
	bSequenceFinished = false;
	bSequenceFailed = false;
	bOpaqueSequence = bOpaqueMedia;
	ActiveSequencePurpose = SequencePurpose;
	bMediaPlaybackStarted = false;
	OpaquePlaybackElapsedSeconds = 0.0f;
	FirstFrameWaitElapsedSeconds = 0.0f;
	PlaybackStallElapsedSeconds = 0.0f;
	LastObservedMediaTime = FTimespan::MinValue();
	MediaState = EReEchoTransitionMediaState::Opening;
	bFadingOut = false;
	FadeElapsedSeconds = 0.0f;
	ApplySequenceBrush(bOpaqueMedia);
	SequenceImage->SetRenderOpacity(1.0f);
	SequenceImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (MediaPlayer)
	{
		MediaPlayer->Close();
		MediaPlayer->SetDesiredPlayerName(FName(TEXT("WmfMedia")));
	}
	UE_LOG(LogTemp,
	       Display,
	       TEXT("Encounter transition purpose=%s desired media player: %s"),
	       *ActiveSequencePurpose.ToString(),
	       bOpaqueMedia ? TEXT("WmfMedia/HAP opaque") : TEXT("WmfMedia/HAP alpha"));
	return MediaPlayer && Source && MediaPlayer->OpenSource(Source);
}

void UReEchoEncounterTransitionWidget::ApplySequenceBrush(const bool bOpaqueMedia)
{
	if (!SequenceImage)
	{
		return;
	}
	// The HAP transition needs its authored alpha material. The ordinary H.264 CG is already opaque
	// and must display the live MediaTexture directly; routing it through the HAP material can retain
	// the preroll sample instead of repainting the subsequent Electra frames.
	SequenceImage->SetBrushResourceObject(bOpaqueMedia ? static_cast<UObject*>(MediaTexture)
	                                                   : static_cast<UObject*>(MediaMaterialInstance));
}

void UReEchoEncounterTransitionWidget::BeginSequenceFadeOut(const float DurationSeconds)
{
	if (bFadingOut)
	{
		return;
	}
	bFadingOut = true;
	FadeDurationSeconds = FMath::Max(DurationSeconds, KINDA_SMALL_NUMBER);
	FadeElapsedSeconds = 0.0f;
	FadeStartOpacity = SequenceImage ? SequenceImage->GetRenderOpacity() : 1.0f;
}

bool UReEchoEncounterTransitionWidget::IsSequenceFinished() const
{
	return bSequenceFinished;
}

bool UReEchoEncounterTransitionWidget::HasSequenceFailed() const
{
	return bSequenceFailed;
}

float UReEchoEncounterTransitionWidget::GetCardChoiceToShopBackgroundBlendAlpha() const
{
	return ActiveSequencePurpose == TEXT("CardChoiceToShop") && MediaPlayer
	           ? CalculateCardChoiceToShopBackgroundBlendAlpha(MediaPlayer->GetTime().GetTotalSeconds())
	           : 0.0f;
}

bool UReEchoEncounterTransitionWidget::HasCardChoiceToShopCollapseStarted() const
{
	return ActiveSequencePurpose == TEXT("CardChoiceToShop") && MediaPlayer &&
	       MediaPlayer->GetTime().GetTotalSeconds() >= CardChoiceToShopCollapseStartSeconds;
}

void UReEchoEncounterTransitionWidget::SetCardChoiceToShopCollapseTargetAbsolute(const FVector2D& AbsoluteCenter)
{
	CardChoiceToShopCollapseTargetAbsolute = AbsoluteCenter;
}

bool UReEchoEncounterTransitionWidget::IsFadeOutFinished() const
{
	return bFadingOut && FadeElapsedSeconds >= FadeDurationSeconds;
}

void UReEchoEncounterTransitionWidget::ResetPresentation()
{
	bSequenceStarted = false;
	bSequenceFinished = false;
	bSequenceFailed = false;
	bOpaqueSequence = false;
	bMediaPlaybackStarted = false;
	OpaquePlaybackElapsedSeconds = 0.0f;
	FirstFrameWaitElapsedSeconds = 0.0f;
	PlaybackStallElapsedSeconds = 0.0f;
	LastObservedMediaTime = FTimespan::MinValue();
	MediaState = EReEchoTransitionMediaState::Closed;
	ActiveSequencePurpose = NAME_None;
	CardChoiceToShopCollapseTargetAbsolute.Reset();
	bFadingOut = false;
	FadeElapsedSeconds = 0.0f;
	FadeStartOpacity = 1.0f;
	if (MediaPlayer)
	{
		MediaPlayer->Close();
	}
	if (Stage01To02AudioComponent)
	{
		Stage01To02AudioComponent->Stop();
		Stage01To02AudioComponent = nullptr;
	}
	if (SequenceImage)
	{
		SequenceImage->SetRenderOpacity(1.0f);
		SequenceImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetStage01To02SkipAvailable(false);
}

void UReEchoEncounterTransitionWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bLoggedViewportGeometry)
	{
		bLoggedViewportGeometry = true;
		UE_LOG(LogTemp,
		       Display,
		       TEXT("Encounter transition viewport geometry: %.0fx%.0f"),
		       MyGeometry.GetLocalSize().X,
		       MyGeometry.GetLocalSize().Y);
	}
	if (MediaPlayer && MediaTexture)
	{
		const FIntPoint MediaSurface(FMath::RoundToInt(MediaTexture->GetSurfaceWidth()),
		                             FMath::RoundToInt(MediaTexture->GetSurfaceHeight()));
		if (MediaSurface.X > 0 && MediaSurface.Y > 0 && MediaSurface != LastLoggedMediaSurface)
		{
			LastLoggedMediaSurface = MediaSurface;
			UE_LOG(LogTemp,
			       Display,
			       TEXT("Encounter transition media surface changed: %dx%d opacity=opaque"),
			       MediaSurface.X,
			       MediaSurface.Y);
		}
		if (bOpaqueSequence && MediaState == EReEchoTransitionMediaState::WaitingForFirstFrame)
		{
			FirstFrameWaitElapsedSeconds += InDeltaTime;
			if (MediaSurface.X > 2 && MediaSurface.Y > 2)
			{
				MediaState = EReEchoTransitionMediaState::Playing;
				LastObservedMediaTime = MediaPlayer->GetTime();
				StartStageCgAudio();
				UE_LOG(LogTemp,
				       Display,
				       TEXT("Encounter transition first video frame ready: player=%s surface=%dx%d time=%.3f."),
				       *MediaPlayer->GetPlayerName().ToString(),
				       MediaSurface.X,
				       MediaSurface.Y,
				       LastObservedMediaTime.GetTotalSeconds());
			}
			else if (FirstFrameWaitElapsedSeconds >= FirstFrameTimeoutSeconds)
			{
				FailSequence(TEXT("no decoded video frame reached MediaTexture within 5 seconds"));
			}
		}
		if (bOpaqueSequence && MediaState == EReEchoTransitionMediaState::Playing)
		{
			const FTimespan CurrentMediaTime = MediaPlayer->GetTime();
			if (CurrentMediaTime > LastObservedMediaTime)
			{
				PlaybackStallElapsedSeconds = 0.0f;
				LastObservedMediaTime = CurrentMediaTime;
			}
			else if ((PlaybackStallElapsedSeconds += InDeltaTime) >= PlaybackStallTimeoutSeconds)
			{
				FailSequence(TEXT("media clock stopped advancing for 5 seconds"));
			}
		}
		if (bSequenceStarted && bOpaqueSequence && bMediaPlaybackStarted && !bSequenceFinished && !bSequenceFailed &&
		    MediaPlayer->IsPlaying())
		{
			OpaquePlaybackElapsedSeconds += InDeltaTime;
			const double DurationSeconds = MediaPlayer->GetDuration().GetTotalSeconds();
			if (DurationSeconds > 0.0 &&
			    OpaquePlaybackElapsedSeconds >= DurationSeconds + OpaqueMediaCompletionGraceSeconds)
			{
				FailSequence(TEXT("media exceeded its duration without an end event"));
			}
		}
	}
	UpdateFillLayout(MyGeometry);
	if (bFadingOut && SequenceImage)
	{
		FadeElapsedSeconds = FMath::Min(FadeElapsedSeconds + InDeltaTime, FadeDurationSeconds);
		const float FadeAlpha = 1.0f - FadeElapsedSeconds / FadeDurationSeconds;
		SequenceImage->SetRenderOpacity(FadeStartOpacity * FadeAlpha);
	}
}

void UReEchoEncounterTransitionWidget::UpdateFillLayout(const FGeometry& Geometry)
{
	const FVector2D ViewSize = Geometry.GetLocalSize();
	if (!SequenceImage || ViewSize.X <= 0.0f || ViewSize.Y <= 0.0f)
	{
		return;
	}
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SequenceImage->Slot);
	if (!CanvasSlot)
	{
		return;
	}
	const bool bCardChoiceComposition =
	    ActiveSequencePurpose == TEXT("EncounterEndToCardChoice") || ActiveSequencePurpose == TEXT("CardChoiceToShop");
	const bool bCollapseToShop = ActiveSequencePurpose == TEXT("CardChoiceToShop");
	const float CollapseAlpha = bCollapseToShop && MediaPlayer
	                                ? CalculateCardChoiceToShopCollapseAlpha(MediaPlayer->GetTime().GetTotalSeconds())
	                                : 0.0f;
	if (bCollapseToShop && !bFadingOut)
	{
		SequenceImage->SetRenderOpacity(CalculateCardChoiceToShopCollapseOpacity(CollapseAlpha));
	}
	const FVector2D FillSize = bCollapseToShop ? CalculateCardChoiceToShopCollapseSize(ViewSize, CollapseAlpha)
	                                           : (bCardChoiceComposition ? CalculateCardChoiceFrameSize(ViewSize)
	                                                                     : CalculateFillSize(ViewSize));
	CanvasSlot->SetAnchors(FAnchors(0.0f));
	const FVector2D CollapseTargetCenter =
	    bCollapseToShop && CardChoiceToShopCollapseTargetAbsolute.IsSet()
	        ? Geometry.AbsoluteToLocal(CardChoiceToShopCollapseTargetAbsolute.GetValue())
	        : ViewSize * 0.5f;
	CanvasSlot->SetPosition(
	    bCollapseToShop
	        ? CalculateCardChoiceToShopCollapsePosition(ViewSize, CollapseAlpha, CollapseTargetCenter)
	        : (bCardChoiceComposition ? CalculateCardChoiceFramePosition(ViewSize) : (ViewSize - FillSize) * 0.5f));
	CanvasSlot->SetSize(FillSize);
}

void UReEchoEncounterTransitionWidget::HandleMediaOpened(FString OpenedUrl)
{
	UE_LOG(LogTemp,
	       Display,
	       TEXT("Encounter transition purpose=%s media opened: %s"),
	       *ActiveSequencePurpose.ToString(),
	       *OpenedUrl);
	if (!MediaPlayer || !MediaPlayer->Seek(FTimespan::Zero()))
	{
		UE_LOG(LogTemp, Error, TEXT("Encounter transition media could not seek to frame zero."));
		bSequenceFailed = true;
		return;
	}
	if (!MediaPlayer->Play())
	{
		UE_LOG(LogTemp, Error, TEXT("Encounter transition media could not play after seeking to frame zero."));
		bSequenceFailed = true;
		return;
	}
	UE_LOG(LogTemp,
	       Display,
	       TEXT("Encounter transition media playback started at %.3f seconds."),
	       MediaPlayer->GetTime().GetTotalSeconds());
	bMediaPlaybackStarted = true;
	MediaState =
	    bOpaqueSequence ? EReEchoTransitionMediaState::WaitingForFirstFrame : EReEchoTransitionMediaState::Playing;
}

void UReEchoEncounterTransitionWidget::HandleMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogTemp,
	       Error,
	       TEXT("Encounter transition purpose=%s media failed to open: %s"),
	       *ActiveSequencePurpose.ToString(),
	       *FailedUrl);
	bSequenceFailed = true;
	MediaState = EReEchoTransitionMediaState::Failed;
}

void UReEchoEncounterTransitionWidget::HandleMediaEndReached()
{
	bSequenceFinished = true;
	MediaState = EReEchoTransitionMediaState::Completed;
	UE_LOG(LogTemp,
	       Display,
	       TEXT("Encounter transition purpose=%s media reached the final frame."),
	       *ActiveSequencePurpose.ToString());
}

void UReEchoEncounterTransitionWidget::StartStageCgAudio()
{
	if (!bOpaqueSequence || !Stage01To02Sound || Stage01To02AudioComponent)
	{
		return;
	}
	UReEchoAudioService* AudioService =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoAudioService>() : nullptr;
	const float EffectiveMusicVolume =
	    AudioService ? CalculateStageCgMusicVolume(AudioService->GetMasterVolume(),
	                                               AudioService->GetBusVolume(EReEchoAudioBus::Music))
	                 : CalculateStageCgMusicVolume(1.0f, 1.0f);
	Stage01To02AudioComponent = UGameplayStatics::CreateSound2D(
	    this, Stage01To02Sound, EffectiveMusicVolume, 1.0f, 0.0f, nullptr, false, false);
	if (Stage01To02AudioComponent)
	{
		Stage01To02AudioComponent->SetUISound(true);
		Stage01To02AudioComponent->Play(0.0f);
	}
	UE_LOG(LogTemp,
	       Display,
	       TEXT("Encounter transition independent audio started: result=%s effectiveMusicVolume=%.3f service=%s."),
	       Stage01To02AudioComponent ? TEXT("success") : TEXT("failed"),
	       EffectiveMusicVolume,
	       AudioService ? TEXT("true") : TEXT("false"));
}

float UReEchoEncounterTransitionWidget::CalculateStageCgMusicVolume(const float MasterVolume,
                                                                    const float MusicBusVolume)
{
	constexpr float StageCgVolumeMultiplier = 1.5f;
	return FMath::Clamp(MasterVolume, 0.0f, 1.0f) * FMath::Clamp(MusicBusVolume, 0.0f, 1.0f) * StageCgVolumeMultiplier;
}

void UReEchoEncounterTransitionWidget::FailSequence(const TCHAR* Reason)
{
	if (MediaState == EReEchoTransitionMediaState::Failed || MediaState == EReEchoTransitionMediaState::Completed)
	{
		return;
	}
	bSequenceFailed = true;
	MediaState = EReEchoTransitionMediaState::Failed;
	if (MediaPlayer)
	{
		MediaPlayer->Close();
	}
	if (Stage01To02AudioComponent)
	{
		Stage01To02AudioComponent->Stop();
		Stage01To02AudioComponent = nullptr;
	}
	UE_LOG(LogTemp,
	       Error,
	       TEXT("Encounter transition purpose=%s media failed: %s."),
	       *ActiveSequencePurpose.ToString(),
	       Reason);
}
