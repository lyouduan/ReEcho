#include "UI/ReEchoShopTooltipWidget.h"

#include "Components/TextBlock.h"

UReEchoShopTooltipWidget::UReEchoShopTooltipWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	DesignSizeMode = EDesignPreviewSizeMode::Desired;
#endif
}

void UReEchoShopTooltipWidget::ApplyDesignerPreviewSettings()
{
#if WITH_EDITORONLY_DATA
	DesignSizeMode = EDesignPreviewSizeMode::Desired;
#endif
}

void UReEchoShopTooltipWidget::Configure(const FText& Title, const FText& Description, const FText& Outcome)
{
	PendingTitle = Title;
	PendingDescription = Description;
	PendingOutcome = Outcome;
	ApplyContent();
}

void UReEchoShopTooltipWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (!IsDesignTime())
	{
		ApplyContent();
	}
}

void UReEchoShopTooltipWidget::ApplyContent()
{
	if (TitleText)
	{
		TitleText->SetText(PendingTitle);
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(PendingDescription);
	}
	if (OutcomeText)
	{
		OutcomeText->SetText(PendingOutcome);
	}
	if (OutcomePanel)
	{
		OutcomePanel->SetVisibility(PendingOutcome.IsEmptyOrWhitespace() ? ESlateVisibility::Collapsed
		                                                                 : ESlateVisibility::SelfHitTestInvisible);
	}
}
