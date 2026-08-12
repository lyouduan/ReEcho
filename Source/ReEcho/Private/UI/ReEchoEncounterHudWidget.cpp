#include "UI/ReEchoEncounterHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
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
	RefreshText();
}

void UReEchoEncounterHudWidget::SetEncounterStatus(const int32 EncounterIndex,
                                                   const int32 TotalEncounters,
                                                   const float RemainingSeconds)
{
	CurrentEncounterIndex = EncounterIndex;
	EncounterCount = FMath::Max(1, TotalEncounters);
	RemainingTime = FMath::Max(0.0f, RemainingSeconds);
	RefreshText();
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
		EncounterText->SetText(FText::Format(NSLOCTEXT("ReEcho", "EncounterHudStage", "关卡 {0}/{1}"),
		                                     FText::AsNumber(CurrentEncounterIndex),
		                                     FText::AsNumber(EncounterCount)));
	}
	if (CountdownText)
	{
		const int32 DisplaySeconds = FMath::CeilToInt(RemainingTime);
		CountdownText->SetText(FText::Format(NSLOCTEXT("ReEcho", "EncounterHudCountdown", "剩余 {0} 秒"),
		                                     FText::AsNumber(DisplaySeconds)));
		CountdownText->SetColorAndOpacity(
		    FSlateColor(DisplaySeconds <= 5 ? FLinearColor(1.0f, 0.2f, 0.12f, 1.0f) : FLinearColor::White));
	}
}
