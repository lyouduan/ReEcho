#include "UI/ReEchoTraitCardChoiceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/ReEchoIndexedButton.h"

namespace
{
constexpr float NeedleSpinDuration = 1.35f;
constexpr float FirstCardRevealTime = 1.15f;
constexpr float CardRevealInterval = 0.22f;
constexpr float CardRevealDuration = 0.34f;

UTextBlock*
CreateCenteredText(UWidgetTree* WidgetTree, const FName Name, const int32 FontSize, const FLinearColor& Color)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	Text->SetJustification(ETextJustify::Center);
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetAutoWrapText(true);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = FontSize;
	Text->SetFont(Font);
	return Text;
}

float EaseOutBack(const float Progress)
{
	const float ClampedProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
	constexpr float Overshoot = 1.70158f;
	const float Shifted = ClampedProgress - 1.0f;
	return 1.0f + (Overshoot + 1.0f) * FMath::Pow(Shifted, 3.0f) + Overshoot * FMath::Square(Shifted);
}
}

UReEchoTraitCardChoiceWidget::UReEchoTraitCardChoiceWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> DrawBackgroundFinder(
	    TEXT("/Game/ReEcho/Textures/UI/ShopBackground.ShopBackground"));
	DrawBackgroundTexture = DrawBackgroundFinder.Object;
}

TSharedRef<SWidget> UReEchoTraitCardChoiceWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UReEchoTraitCardChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
	BuildWidgetTree();
	BuildCardEntries();

	for (UReEchoIndexedButton* CardButton : CardButtons)
	{
		CardButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoTraitCardChoiceWidget::HandleCardClicked);
	}

	RefreshOffers();
	ResetRevealAnimation();
}

void UReEchoTraitCardChoiceWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RevealElapsed += InDeltaTime;

	if (NeedleWidget)
	{
		const float SpinProgress = FMath::Clamp(RevealElapsed / NeedleSpinDuration, 0.0f, 1.0f);
		const float DeceleratedProgress = 1.0f - FMath::Pow(1.0f - SpinProgress, 3.0f);
		NeedleWidget->SetRenderTransformAngle(1440.0f * DeceleratedProgress + 25.0f);
	}

	bool bAllCardsRevealed = true;
	for (int32 CardIndex = 0; CardIndex < CardPanels.Num(); ++CardIndex)
	{
		const float RevealStart = FirstCardRevealTime + CardIndex * CardRevealInterval;
		const float Progress = FMath::Clamp((RevealElapsed - RevealStart) / CardRevealDuration, 0.0f, 1.0f);
		const float EasedProgress = EaseOutBack(Progress);
		CardPanels[CardIndex]->SetRenderOpacity(Progress);
		CardPanels[CardIndex]->SetRenderScale(FVector2D(FMath::Lerp(0.72f, 1.0f, EasedProgress)));
		CardPanels[CardIndex]->SetRenderTranslation(FVector2D(0.0f, FMath::Lerp(95.0f, 0.0f, EasedProgress)));
		bAllCardsRevealed &= Progress >= 1.0f;
	}

	if (bAllCardsRevealed && !bRevealComplete)
	{
		bRevealComplete = true;
		for (UButton* CardButton : CardButtons)
		{
			CardButton->SetIsEnabled(true);
		}
		if (!CardButtons.IsEmpty())
		{
			CardButtons[0]->SetKeyboardFocus();
		}
	}
}

void UReEchoTraitCardChoiceWidget::InitializeOffers(const TArray<FReEchoTraitCardOffer>& InOffers,
                                                    const int32 InTimeShards,
                                                    const bool bInForgeChoice)
{
	Offers = InOffers;
	CurrentTimeShards = InTimeShards;
	bForgeChoice = bInForgeChoice;
	RefreshOffers();
}

void UReEchoTraitCardChoiceWidget::BuildWidgetTree()
{
	if (!CardButtons.IsEmpty() || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas =
	    WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TraitDrawRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UImage* BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TraitDrawBackground"));
	if (DrawBackgroundTexture)
	{
		BackgroundImage->SetBrushFromTexture(DrawBackgroundTexture, true);
	}
	BackgroundImage->SetColorAndOpacity(FLinearColor(0.48f, 0.48f, 0.48f, 1.0f));
	UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(BackgroundImage);
	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0.0f));

	UBorder* Vignette = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TraitDrawVignette"));
	Vignette->SetBrushColor(FLinearColor(0.0f, 0.015f, 0.025f, 0.48f));
	Vignette->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* VignetteSlot = RootCanvas->AddChildToCanvas(Vignette);
	VignetteSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	VignetteSlot->SetOffsets(FMargin(0.0f));

	UBorder* HeaderPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TraitDrawHeader"));
	HeaderPanel->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.05f, 0.90f));
	HeaderPanel->SetPadding(FMargin(34.0f, 18.0f));
	UCanvasPanelSlot* HeaderSlot = RootCanvas->AddChildToCanvas(HeaderPanel);
	HeaderSlot->SetAnchors(FAnchors(0.23f, 0.045f, 0.77f, 0.225f));
	HeaderSlot->SetOffsets(FMargin(0.0f));
	HeaderSlot->SetZOrder(4);

	UVerticalBox* HeaderContent = WidgetTree->ConstructWidget<UVerticalBox>();
	HeaderPanel->SetContent(HeaderContent);
	TitleText = CreateCenteredText(WidgetTree, TEXT("TraitDrawTitle"), 40, FLinearColor(0.92f, 0.80f, 0.50f));
	UVerticalBoxSlot* TitleSlot = HeaderContent->AddChildToVerticalBox(TitleText);
	TitleSlot->SetHorizontalAlignment(HAlign_Fill);
	SubtitleText = CreateCenteredText(WidgetTree, TEXT("TraitDrawSubtitle"), 20, FLinearColor(0.82f, 0.86f, 0.91f));
	UVerticalBoxSlot* SubtitleSlot = HeaderContent->AddChildToVerticalBox(SubtitleText);
	SubtitleSlot->SetHorizontalAlignment(HAlign_Fill);
	SubtitleSlot->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 8.0f));
	CurrencyText = CreateCenteredText(WidgetTree, TEXT("TraitDrawCurrency"), 22, FLinearColor(0.48f, 0.90f, 0.88f));
	UVerticalBoxSlot* CurrencySlot = HeaderContent->AddChildToVerticalBox(CurrencyText);
	CurrencySlot->SetHorizontalAlignment(HAlign_Fill);

	UBorder* Needle = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TraitDrawNeedle"));
	Needle->SetBrushColor(FLinearColor(0.70f, 0.53f, 0.22f, 0.88f));
	Needle->SetRenderTransformPivot(FVector2D(0.5f, 0.88f));
	Needle->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* NeedleSlot = RootCanvas->AddChildToCanvas(Needle);
	NeedleSlot->SetAnchors(FAnchors(0.5f, 0.285f));
	NeedleSlot->SetAlignment(FVector2D(0.5f, 0.88f));
	NeedleSlot->SetSize(FVector2D(8.0f, 82.0f));
	NeedleSlot->SetZOrder(2);
	NeedleWidget = Needle;

	TraitCardContainer = RootCanvas;
	RefreshOffers();
}

void UReEchoTraitCardChoiceWidget::BuildCardEntries()
{
	if (!WidgetTree || !TraitCardContainer)
	{
		return;
	}

	for (USizeBox* CardPanel : CardPanels)
	{
		if (CardPanel && CardPanel->GetParent() == TraitCardContainer)
		{
			TraitCardContainer->RemoveChild(CardPanel);
		}
	}
	CardButtons.Reset();
	CardPanels.Reset();
	CardNames.Reset();
	CardDescriptions.Reset();

	const TArray<FAnchors> CardAnchors = {FAnchors(0.26f, 0.62f), FAnchors(0.50f, 0.59f), FAnchors(0.74f, 0.62f)};
	const TArray<FLinearColor> CardColors = {FLinearColor(0.30f, 0.20f, 0.11f, 0.98f),
	                                         FLinearColor(0.13f, 0.27f, 0.26f, 0.98f),
	                                         FLinearColor(0.22f, 0.16f, 0.31f, 0.98f)};
	const TArray<FText> CardKickers = {NSLOCTEXT("ReEcho", "TraitCandidateOne", "候选 I"),
	                                   NSLOCTEXT("ReEcho", "TraitCandidateTwo", "候选 II"),
	                                   NSLOCTEXT("ReEcho", "TraitCandidateThree", "候选 III")};
	for (int32 CardIndex = 0; CardIndex < 3; ++CardIndex)
	{
		USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(
		    USizeBox::StaticClass(), *FString::Printf(TEXT("TraitCardSize%d"), CardIndex));
		CardSize->SetWidthOverride(310.0f);
		CardSize->SetHeightOverride(390.0f);
		CardSize->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		UCanvasPanelSlot* CardSlot = TraitCardContainer->AddChildToCanvas(CardSize);
		CardSlot->SetAnchors(CardAnchors[CardIndex]);
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetSize(FVector2D(310.0f, 390.0f));
		CardSlot->SetZOrder(5 + CardIndex);
		CardPanels.Add(CardSize);

		UReEchoIndexedButton* CardButton = WidgetTree->ConstructWidget<UReEchoIndexedButton>(
		    UReEchoIndexedButton::StaticClass(), *FString::Printf(TEXT("TraitCardButton%d"), CardIndex));
		CardButton->SetEntryIndex(CardIndex);
		CardButton->SetBackgroundColor(CardColors[CardIndex]);
		CardButton->SetIsEnabled(false);
		CardSize->SetContent(CardButton);
		CardButtons.Add(CardButton);

		UVerticalBox* CardContent = WidgetTree->ConstructWidget<UVerticalBox>(
		    UVerticalBox::StaticClass(), *FString::Printf(TEXT("TraitCardContent%d"), CardIndex));
		CardButton->SetContent(CardContent);

		UTextBlock* CardKicker = CreateCenteredText(
		    WidgetTree, *FString::Printf(TEXT("TraitCardKicker%d"), CardIndex), 18, FLinearColor(0.48f, 0.90f, 0.88f));
		CardKicker->SetText(CardKickers[CardIndex]);
		UVerticalBoxSlot* KickerSlot = CardContent->AddChildToVerticalBox(CardKicker);
		KickerSlot->SetHorizontalAlignment(HAlign_Fill);
		KickerSlot->SetPadding(FMargin(18.0f, 26.0f, 18.0f, 18.0f));

		UTextBlock* CardName = CreateCenteredText(
		    WidgetTree, *FString::Printf(TEXT("TraitCardName%d"), CardIndex), 29, FLinearColor(0.96f, 0.88f, 0.68f));
		UVerticalBoxSlot* NameSlot = CardContent->AddChildToVerticalBox(CardName);
		NameSlot->SetHorizontalAlignment(HAlign_Fill);
		NameSlot->SetPadding(FMargin(18.0f, 8.0f, 18.0f, 28.0f));
		CardNames.Add(CardName);

		UTextBlock* CardDescription = CreateCenteredText(WidgetTree,
		                                                 *FString::Printf(TEXT("TraitCardDescription%d"), CardIndex),
		                                                 20,
		                                                 FLinearColor(0.88f, 0.90f, 0.93f));
		UVerticalBoxSlot* DescriptionSlot = CardContent->AddChildToVerticalBox(CardDescription);
		DescriptionSlot->SetHorizontalAlignment(HAlign_Fill);
		DescriptionSlot->SetPadding(FMargin(26.0f, 12.0f, 26.0f, 28.0f));
		CardDescriptions.Add(CardDescription);

		UTextBlock* SelectHint = CreateCenteredText(WidgetTree,
		                                            *FString::Printf(TEXT("TraitCardSelectHint%d"), CardIndex),
		                                            16,
		                                            FLinearColor(0.70f, 0.76f, 0.82f));
		SelectHint->SetText(NSLOCTEXT("ReEcho", "TraitCardSelectHint", "点击选择 · 确认后不可撤回"));
		UVerticalBoxSlot* SelectHintSlot = CardContent->AddChildToVerticalBox(SelectHint);
		SelectHintSlot->SetHorizontalAlignment(HAlign_Fill);
		SelectHintSlot->SetPadding(FMargin(18.0f, 16.0f, 18.0f, 20.0f));
	}
	RefreshOffers();
}

void UReEchoTraitCardChoiceWidget::RefreshOffers()
{
	if (TitleText)
	{
		TitleText->SetText(bForgeChoice ? NSLOCTEXT("ReEcho", "ForgeChoiceTitle", "选择本轮锻炼")
		                                : NSLOCTEXT("ReEcho", "TraitChoiceTitle", "选择一项时间特质"));
	}
	if (SubtitleText)
	{
		SubtitleText->SetText(
		    NSLOCTEXT("ReEcho", "TraitChoiceSubtitle", "完成本次构筑选择后，将进入时光商城使用碎片购买道具"));
	}
	if (CurrencyText)
	{
		CurrencyText->SetText(FText::Format(NSLOCTEXT("ReEcho", "TraitChoiceCurrency", "当前时光碎片：{0}"),
		                                    FText::AsNumber(CurrentTimeShards)));
	}
	for (int32 CardIndex = 0; CardIndex < CardButtons.Num(); ++CardIndex)
	{
		const bool bHasOffer = Offers.IsValidIndex(CardIndex);
		CardPanels[CardIndex]->SetVisibility(bHasOffer ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		CardButtons[CardIndex]->SetIsEnabled(bHasOffer && bRevealComplete);
		if (bHasOffer)
		{
			CardNames[CardIndex]->SetText(Offers[CardIndex].DisplayName);
			CardDescriptions[CardIndex]->SetText(Offers[CardIndex].Description);
		}
	}
}

void UReEchoTraitCardChoiceWidget::ResetRevealAnimation()
{
	RevealElapsed = 0.0f;
	bRevealComplete = false;
	for (int32 CardIndex = 0; CardIndex < CardPanels.Num(); ++CardIndex)
	{
		CardPanels[CardIndex]->SetRenderOpacity(0.0f);
		CardPanels[CardIndex]->SetRenderScale(FVector2D(0.72f));
		CardPanels[CardIndex]->SetRenderTranslation(FVector2D(0.0f, 95.0f));
		CardButtons[CardIndex]->SetIsEnabled(false);
	}
}

void UReEchoTraitCardChoiceWidget::SelectOffer(const int32 OfferIndex)
{
	if (bRevealComplete && Offers.IsValidIndex(OfferIndex))
	{
		bRevealComplete = false;
		for (UButton* CardButton : CardButtons)
		{
			CardButton->SetIsEnabled(false);
		}
		OnCardSelected.Broadcast(Offers[OfferIndex].CardId);
	}
}

void UReEchoTraitCardChoiceWidget::HandleCardClicked(const int32 OfferIndex)
{
	SelectOffer(OfferIndex);
}
