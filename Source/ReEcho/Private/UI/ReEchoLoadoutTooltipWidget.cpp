#include "UI/ReEchoLoadoutTooltipWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UReEchoLoadoutTooltipWidget::UReEchoLoadoutTooltipWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	DesignSizeMode = EDesignPreviewSizeMode::Desired;
	DesignTimeSize = FVector2D(380.0f, 240.0f);
#endif
}

void UReEchoLoadoutTooltipWidget::Configure(const FText& InTitle, const FText& InDescription)
{
	PendingTitle = InTitle;
	PendingDescription = InDescription;
	ApplyContent();
}

void UReEchoLoadoutTooltipWidget::ApplyDesignerPreviewSettings()
{
#if WITH_EDITOR
	Modify();
#endif
#if WITH_EDITORONLY_DATA
	DesignSizeMode = EDesignPreviewSizeMode::Desired;
	DesignTimeSize = FVector2D(380.0f, 240.0f);
#endif
#if WITH_EDITOR
	MarkPackageDirty();
#endif
}

bool UReEchoLoadoutTooltipWidget::HasDesiredDesignerPreview() const
{
#if WITH_EDITORONLY_DATA
	return DesignSizeMode == EDesignPreviewSizeMode::Desired;
#else
	return true;
#endif
}

TSharedRef<SWidget> UReEchoLoadoutTooltipWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UReEchoLoadoutTooltipWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (IsDesignTime())
	{
		// Keep the representative CSV text authored into the WBP visible in Designer.
		return;
	}
	ApplyContent();
}

void UReEchoLoadoutTooltipWidget::BuildWidgetTree()
{
	USizeBox* TooltipSize = WidgetTree->ConstructWidget<USizeBox>();
	TooltipSize->SetWidthOverride(380.0f);
	WidgetTree->RootWidget = TooltipSize;

	UBorder* TooltipFrame = WidgetTree->ConstructWidget<UBorder>();
	TooltipFrame->SetBrushColor(FLinearColor(0.95f, 0.88f, 0.72f, 1.0f));
	TooltipFrame->SetPadding(FMargin(3.0f));
	TooltipSize->SetContent(TooltipFrame);

	UBorder* TooltipSurface = WidgetTree->ConstructWidget<UBorder>();
	TooltipSurface->SetBrushColor(FLinearColor(0.015f, 0.015f, 0.015f, 0.97f));
	TooltipSurface->SetPadding(FMargin(18.0f, 14.0f));
	TooltipFrame->SetContent(TooltipSurface);

	UVerticalBox* TooltipContent = WidgetTree->ConstructWidget<UVerticalBox>();
	TooltipSurface->SetContent(TooltipContent);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.96f, 0.88f, 1.0f)));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetAutoWrapText(false);
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 22;
	TitleText->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = TooltipContent->AddChildToVerticalBox(TitleText);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

	DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescriptionText"));
	DescriptionText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	DescriptionText->SetJustification(ETextJustify::Left);
	DescriptionText->SetAutoWrapText(true);
	DescriptionText->SetWrapTextAt(338.0f);
	FSlateFontInfo BodyFont = DescriptionText->GetFont();
	BodyFont.Size = 20;
	DescriptionText->SetFont(BodyFont);
	TooltipContent->AddChildToVerticalBox(DescriptionText);

	ApplyContent();
}

void UReEchoLoadoutTooltipWidget::ApplyContent()
{
	if (TitleText)
	{
		TitleText->SetText(PendingTitle);
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(PendingDescription);
	}
}
