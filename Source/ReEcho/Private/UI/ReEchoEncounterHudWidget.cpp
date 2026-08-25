#include "UI/ReEchoEncounterHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "ReEcho.h"
#include "UI/ReEchoMinimapCanvasWidget.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UReEchoEncounterHudWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UReEchoEncounterHudWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidgetTree();

	// 从 WBP 控件树中按类型找到小地图画布（在编辑器里放入的 ReEchoMinimapCanvasWidget）。
	MinimapCanvas = nullptr;
	if (WidgetTree)
	{
		TArray<UWidget*> AllWidgets;
		WidgetTree->GetAllWidgets(AllWidgets);
		for (UWidget* Child : AllWidgets)
		{
			if (UReEchoMinimapCanvasWidget* CanvasWidget = Cast<UReEchoMinimapCanvasWidget>(Child))
			{
				MinimapCanvas = CanvasWidget;
				break;
			}
		}
	}

	RefreshText();
}

void UReEchoEncounterHudWidget::SetEncounterStatus(const int32 EncounterIndex,
                                                   const int32 TotalEncounters,
                                                   const float RemainingSeconds,
                                                   const float DurationSeconds)
{
	CurrentEncounterIndex = EncounterIndex;
	EncounterCount = FMath::Max(1, TotalEncounters);
	RemainingTime = FMath::IsFinite(RemainingSeconds) ? FMath::Max(0.0f, RemainingSeconds) : 0.0f;
	EncounterDuration = FMath::IsFinite(DurationSeconds) ? FMath::Max(0.0f, DurationSeconds) : 0.0f;
	RefreshText();
}

void UReEchoEncounterHudWidget::SetMinimapView(const FReEchoMinimapView& View)
{
	if (MinimapCanvas)
	{
		MinimapCanvas->SetView(View);
	}
}

FText UReEchoEncounterHudWidget::FormatEncounterLabel(const int32 EncounterIndex)
{
	return FText::Format(NSLOCTEXT("ReEcho", "EncounterHudStage", "第 {0} 关"),
	                     FText::AsNumber(FMath::Max(0, EncounterIndex)));
}

FText UReEchoEncounterHudWidget::FormatCountdown(const float RemainingSeconds)
{
	const int32 DisplaySeconds = FMath::CeilToInt(FMath::Max(0.0f, RemainingSeconds));
	const int32 Minutes = DisplaySeconds / 60;
	const int32 Seconds = DisplaySeconds % 60;
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds));
}

float UReEchoEncounterHudWidget::CalculateCountdownNeedleAngle(const float RemainingSeconds,
                                                               const float DurationSeconds)
{
	constexpr float RightAngle = -90.0f;
	constexpr float LeftAngle = 90.0f;
	const float SafeRemainingSeconds = FMath::IsFinite(RemainingSeconds) ? RemainingSeconds : 0.0f;
	if (!FMath::IsFinite(DurationSeconds) || DurationSeconds <= UE_SMALL_NUMBER)
	{
		return SafeRemainingSeconds <= 0.0f ? LeftAngle : RightAngle;
	}
	const float ElapsedRatio = 1.0f - FMath::Clamp(SafeRemainingSeconds / DurationSeconds, 0.0f, 1.0f);
	return FMath::Lerp(RightAngle, LeftAngle, ElapsedRatio);
}

void UReEchoEncounterHudWidget::BuildWidgetTree()
{
	if (EncounterText || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas =
	    WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("EncounterHudRoot"));
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EncounterHudBackground"));
	Background->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
	Background->SetPadding(FMargin(18.0f, 12.0f));
	Background->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(1.0f, 0.0f));
	BackgroundSlot->SetAlignment(FVector2D(1.0f, 0.0f));
	BackgroundSlot->SetPosition(FVector2D(-28.0f, 28.0f));
	BackgroundSlot->SetAutoSize(true);

	UVerticalBox* Content =
	    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EncounterHudContent"));
	Background->SetContent(Content);

	EncounterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EncounterText"));
	EncounterText->SetJustification(ETextJustify::Right);
	EncounterText->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.78f, 0.58f, 1.0f)));
	FSlateFontInfo EncounterFont = EncounterText->GetFont();
	EncounterFont.Size = 21;
	EncounterText->SetFont(EncounterFont);
	UVerticalBoxSlot* EncounterSlot = Content->AddChildToVerticalBox(EncounterText);
	EncounterSlot->SetHorizontalAlignment(HAlign_Right);

	CountdownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountdownText"));
	CountdownText->SetJustification(ETextJustify::Right);
	CountdownText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo CountdownFont = CountdownText->GetFont();
	CountdownFont.Size = 30;
	CountdownText->SetFont(CountdownFont);
	UVerticalBoxSlot* CountdownSlot = Content->AddChildToVerticalBox(CountdownText);
	CountdownSlot->SetHorizontalAlignment(HAlign_Right);
	CountdownSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));

	RefreshText();
}

void UReEchoEncounterHudWidget::RefreshText()
{
	if (EncounterText)
	{
		EncounterText->SetText(FormatEncounterLabel(CurrentEncounterIndex));
	}
	if (CountdownText)
	{
		const int32 DisplaySeconds = FMath::CeilToInt(RemainingTime);
		CountdownText->SetText(FormatCountdown(RemainingTime));
		CountdownText->SetColorAndOpacity(
		    FSlateColor(DisplaySeconds <= 5 ? FLinearColor(1.0f, 0.2f, 0.12f, 1.0f) : FLinearColor::White));
	}
	if (ArtClockNeedle)
	{
		ArtClockNeedle->SetRenderTransformAngle(CalculateCountdownNeedleAngle(RemainingTime, EncounterDuration));
	}
}
