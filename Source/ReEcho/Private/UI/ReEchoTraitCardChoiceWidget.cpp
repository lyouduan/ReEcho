#include "UI/ReEchoTraitCardChoiceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UReEchoTraitCardChoiceWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UReEchoTraitCardChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidgetTree();

	if (CardButtons.Num() == 3)
	{
		CardButtons[0]->OnClicked.AddUniqueDynamic(
			this,
			&UReEchoTraitCardChoiceWidget::HandleFirstCardClicked);
		CardButtons[1]->OnClicked.AddUniqueDynamic(
			this,
			&UReEchoTraitCardChoiceWidget::HandleSecondCardClicked);
		CardButtons[2]->OnClicked.AddUniqueDynamic(
			this,
			&UReEchoTraitCardChoiceWidget::HandleThirdCardClicked);
		CardButtons[0]->SetKeyboardFocus();
	}

	RefreshOffers();
}

void UReEchoTraitCardChoiceWidget::InitializeOffers(
	const TArray<FReEchoTraitCardOffer>& InOffers)
{
	Offers = InOffers;
	RefreshOffers();
}

void UReEchoTraitCardChoiceWidget::BuildWidgetTree()
{
	if (!CardButtons.IsEmpty() || !WidgetTree)
	{
		return;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("CardChoiceBackground"));
	Background->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.04f, 0.94f));
	Background->SetPadding(FMargin(70.0f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Background;

	UVerticalBox* Page = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("CardChoicePage"));
	Background->SetContent(Page);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("CardChoiceTitle"));
	Title->SetText(NSLOCTEXT("ReEcho", "TraitChoiceTitle", "选择一个时间特性"));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.9f, 1.0f)));
	Title->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 38;
	Title->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = Page->AddChildToVerticalBox(Title);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 34.0f));

	UHorizontalBox* CardRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("TraitCardRow"));
	Page->AddChildToVerticalBox(CardRow);

	for (int32 CardIndex = 0; CardIndex < 3; ++CardIndex)
	{
		USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			*FString::Printf(TEXT("TraitCardSize%d"), CardIndex));
		CardSize->SetWidthOverride(280.0f);
		CardSize->SetHeightOverride(260.0f);
		UHorizontalBoxSlot* CardSlot = CardRow->AddChildToHorizontalBox(CardSize);
		CardSlot->SetPadding(FMargin(12.0f));

		UButton* CardButton = WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(),
			*FString::Printf(TEXT("TraitCardButton%d"), CardIndex));
		CardButton->SetBackgroundColor(FLinearColor(0.08f, 0.18f, 0.34f, 1.0f));
		CardSize->SetContent(CardButton);
		CardButtons.Add(CardButton);

		UVerticalBox* CardContent = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			*FString::Printf(TEXT("TraitCardContent%d"), CardIndex));
		CardButton->SetContent(CardContent);

		UTextBlock* CardName = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("TraitCardName%d"), CardIndex));
		CardName->SetJustification(ETextJustify::Center);
		CardName->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.82f, 0.25f)));
		FSlateFontInfo NameFont = CardName->GetFont();
		NameFont.Size = 26;
		CardName->SetFont(NameFont);
		UVerticalBoxSlot* NameSlot = CardContent->AddChildToVerticalBox(CardName);
		NameSlot->SetHorizontalAlignment(HAlign_Fill);
		NameSlot->SetPadding(FMargin(18.0f, 42.0f, 18.0f, 28.0f));
		CardNames.Add(CardName);

		UTextBlock* CardDescription = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("TraitCardDescription%d"), CardIndex));
		CardDescription->SetJustification(ETextJustify::Center);
		CardDescription->SetAutoWrapText(true);
		CardDescription->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		FSlateFontInfo DescriptionFont = CardDescription->GetFont();
		DescriptionFont.Size = 18;
		CardDescription->SetFont(DescriptionFont);
		UVerticalBoxSlot* DescriptionSlot =
			CardContent->AddChildToVerticalBox(CardDescription);
		DescriptionSlot->SetHorizontalAlignment(HAlign_Fill);
		DescriptionSlot->SetPadding(FMargin(20.0f));
		CardDescriptions.Add(CardDescription);
	}
}

void UReEchoTraitCardChoiceWidget::RefreshOffers()
{
	for (int32 CardIndex = 0; CardIndex < CardButtons.Num(); ++CardIndex)
	{
		const bool bHasOffer = Offers.IsValidIndex(CardIndex);
		CardButtons[CardIndex]->SetIsEnabled(bHasOffer);
		CardButtons[CardIndex]->SetVisibility(
			bHasOffer ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		if (bHasOffer)
		{
			CardNames[CardIndex]->SetText(Offers[CardIndex].DisplayName);
			CardDescriptions[CardIndex]->SetText(Offers[CardIndex].Description);
		}
	}
}

void UReEchoTraitCardChoiceWidget::SelectOffer(const int32 OfferIndex)
{
	if (Offers.IsValidIndex(OfferIndex))
	{
		OnCardSelected.Broadcast(Offers[OfferIndex].CardId);
	}
}

void UReEchoTraitCardChoiceWidget::HandleFirstCardClicked()
{
	SelectOffer(0);
}

void UReEchoTraitCardChoiceWidget::HandleSecondCardClicked()
{
	SelectOffer(1);
}

void UReEchoTraitCardChoiceWidget::HandleThirdCardClicked()
{
	SelectOffer(2);
}