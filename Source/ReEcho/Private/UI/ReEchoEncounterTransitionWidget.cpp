#include "UI/ReEchoEncounterTransitionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "MediaTexture.h"

namespace
{
constexpr float SequenceWidth = 1920.0f;
constexpr float SequenceHeight = 1080.0f;
}

UReEchoEncounterTransitionWidget::UReEchoEncounterTransitionWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UMediaSource> SourceFinder(
	    TEXT("/Game/ReEcho/UI/EncounterTransition/FMS_EncounterTransition.FMS_EncounterTransition"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
	    TEXT("/Game/ReEcho/UI/EncounterTransition/M_UI_EncounterTransition.M_UI_EncounterTransition"));
	MediaSource = SourceFinder.Object;
	MediaMaterial = MaterialFinder.Object;
}

float UReEchoEncounterTransitionWidget::CalculateCountdownIntensity(const float RemainingTime)
{
	if (RemainingTime <= 0.0f || RemainingTime > 3.0f)
	{
		return 0.0f;
	}
	return RemainingTime > 1.0f ? (3.0f - RemainingTime) * 0.5f : 1.0f;
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
	SetVisibility(ESlateVisibility::HitTestInvisible);
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

	CountdownWash = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CountdownWash"));
	CountdownWash->SetBrushColor(FLinearColor(0.025f, 0.12f, 0.16f, 0.0f));
	if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(CountdownWash))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
	}

	CountdownPulse = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CountdownPulse"));
	CountdownPulse->SetBrushColor(FLinearColor(0.42f, 0.04f, 0.08f, 0.0f));
	if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(CountdownPulse))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
	}

	SequenceImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SequenceImage"));
	SequenceImage->SetVisibility(ESlateVisibility::Collapsed);
	RootCanvas->AddChildToCanvas(SequenceImage);

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

void UReEchoEncounterTransitionWidget::SetCountdownIntensity(const float Intensity)
{
	BuildFallbackTree();
	const float ClampedIntensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
	CountdownWash->SetBrushColor(FLinearColor(0.025f, 0.12f, 0.16f, 0.28f * ClampedIntensity));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(GetWorld() ? GetWorld()->GetTimeSeconds() * 9.0f : 0.0f);
	CountdownPulse->SetBrushColor(FLinearColor(0.42f, 0.04f, 0.08f, 0.16f * ClampedIntensity * Pulse));
}

bool UReEchoEncounterTransitionWidget::StartSequence()
{
	BuildFallbackTree();
	if (bSequenceStarted)
	{
		return !bSequenceFailed;
	}
	bSequenceStarted = true;
	bSequenceFinished = false;
	bSequenceFailed = false;
	bFadingOut = false;
	FadeElapsedSeconds = 0.0f;
	SequenceImage->SetRenderOpacity(1.0f);
	SequenceImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (MediaPlayer)
	{
		MediaPlayer->Close();
	}
	return MediaPlayer && MediaSource && MediaPlayer->OpenSource(MediaSource);
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
}

bool UReEchoEncounterTransitionWidget::IsSequenceFinished() const
{
	return bSequenceFinished;
}

bool UReEchoEncounterTransitionWidget::HasSequenceFailed() const
{
	return bSequenceFailed;
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
	bFadingOut = false;
	FadeElapsedSeconds = 0.0f;
	if (MediaPlayer)
	{
		MediaPlayer->Close();
	}
	if (SequenceImage)
	{
		SequenceImage->SetRenderOpacity(1.0f);
		SequenceImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetCountdownIntensity(0.0f);
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
	if (MediaPlayer && MediaPlayer->IsPlaying() && MediaTexture)
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
	}
	UpdateFillLayout(MyGeometry.GetLocalSize());
	if (bFadingOut && SequenceImage)
	{
		FadeElapsedSeconds = FMath::Min(FadeElapsedSeconds + InDeltaTime, FadeDurationSeconds);
		SequenceImage->SetRenderOpacity(1.0f - FadeElapsedSeconds / FadeDurationSeconds);
	}
}

void UReEchoEncounterTransitionWidget::UpdateFillLayout(const FVector2D& ViewSize)
{
	if (!SequenceImage || ViewSize.X <= 0.0f || ViewSize.Y <= 0.0f)
	{
		return;
	}
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SequenceImage->Slot);
	if (!CanvasSlot)
	{
		return;
	}
	const FVector2D FillSize = CalculateFillSize(ViewSize);
	CanvasSlot->SetAnchors(FAnchors(0.0f));
	CanvasSlot->SetPosition((ViewSize - FillSize) * 0.5f);
	CanvasSlot->SetSize(FillSize);
}

void UReEchoEncounterTransitionWidget::HandleMediaOpened(FString OpenedUrl)
{
	UE_LOG(LogTemp, Display, TEXT("Encounter transition media opened: %s"), *OpenedUrl);
	if (!MediaPlayer || !MediaPlayer->Seek(FTimespan::Zero()) || !MediaPlayer->Play())
	{
		UE_LOG(LogTemp, Error, TEXT("Encounter transition media could not seek to frame zero and play."));
		bSequenceFailed = true;
		return;
	}
	UE_LOG(LogTemp,
	       Display,
	       TEXT("Encounter transition media playback started at %.3f seconds."),
	       MediaPlayer->GetTime().GetTotalSeconds());
}

void UReEchoEncounterTransitionWidget::HandleMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogTemp, Error, TEXT("Encounter transition media failed to open: %s"), *FailedUrl);
	bSequenceFailed = true;
}

void UReEchoEncounterTransitionWidget::HandleMediaEndReached()
{
	bSequenceFinished = true;
	UE_LOG(LogTemp, Display, TEXT("Encounter transition media reached the final frame."));
}
