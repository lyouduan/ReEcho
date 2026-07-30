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

namespace
{
constexpr float NeedleSpinDuration = 1.35f;
constexpr float FirstCardRevealTime = 1.15f;
constexpr float CardRevealInterval = 0.22f;
constexpr float CardRevealDuration = 0.34f;

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
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UReEchoTraitCardChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidgetTree();

	if (CardButtons.Num() == 3)
	{
		CardButtons[0]->OnClicked.AddUniqueDynamic(this, &UReEchoTraitCardChoiceWidget::HandleFirstCardClicked);
		CardButtons[1]->OnClicked.AddUniqueDynamic(this, &UReEchoTraitCardChoiceWidget::HandleSecondCardClicked);
		CardButtons[2]->OnClicked.AddUniqueDynamic(this, &UReEchoTraitCardChoiceWidget::HandleThirdCardClicked);
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

void UReEchoTraitCardChoiceWidget::InitializeOffers(const TArray<FReEchoTraitCardOffer>& InOffers)
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

	UCanvasPanel* RootCanvas =
	    WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TraitDrawRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UImage* BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TraitDrawBackground"));
	if (DrawBackgroundTexture)
	{
		BackgroundImage->SetBrushFromTexture(DrawBackgroundTexture, true);
	}
	BackgroundImage->SetColorAndOpacity(FLinearColor(0.72f, 0.72f, 0.72f, 1.0f));
	UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(BackgroundImage);
	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0.0f));

	UBorder* Vignette = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TraitDrawVignette"));
	Vignette->SetBrushColor(FLinearColor(0.0f, 0.015f, 0.025f, 0.25f));
	Vignette->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* VignetteSlot = RootCanvas->AddChildToCanvas(Vignette);
	VignetteSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	VignetteSlot->SetOffsets(FMargin(0.0f));

	UBorder* Needle = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TraitDrawNeedle"));
	Needle->SetBrushColor(FLinearColor(0.12f, 0.075f, 0.045f, 0.92f));
	Needle->SetRenderTransformPivot(FVector2D(0.5f, 0.88f));
	Needle->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* NeedleSlot = RootCanvas->AddChildToCanvas(Needle);
	NeedleSlot->SetAnchors(FAnchors(0.65f, 0.58f));
	NeedleSlot->SetAlignment(FVector2D(0.5f, 0.88f));
	NeedleSlot->SetSize(FVector2D(14.0f, 205.0f));
	NeedleSlot->SetZOrder(2);
	NeedleWidget = Needle;

	const TArray<FAnchors> CardAnchors = {FAnchors(0.43f, 0.55f), FAnchors(0.62f, 0.51f), FAnchors(0.81f, 0.55f)};
	for (int32 CardIndex = 0; CardIndex < 3; ++CardIndex)
	{
		USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(
		    USizeBox::StaticClass(), *FString::Printf(TEXT("TraitCardSize%d"), CardIndex));
		CardSize->SetWidthOverride(280.0f);
		CardSize->SetHeightOverride(350.0f);
		CardSize->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		UCanvasPanelSlot* CardSlot = RootCanvas->AddChildToCanvas(CardSize);
		CardSlot->SetAnchors(CardAnchors[CardIndex]);
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetSize(FVector2D(280.0f, 350.0f));
		CardSlot->SetZOrder(5 + CardIndex);
		CardPanels.Add(CardSize);

		UButton* CardButton = WidgetTree->ConstructWidget<UButton>(
		    UButton::StaticClass(), *FString::Printf(TEXT("TraitCardButton%d"), CardIndex));
		CardButton->SetBackgroundColor(FLinearColor(0.62f, 0.52f, 0.39f, 1.0f));
		CardButton->SetIsEnabled(false);
		CardSize->SetContent(CardButton);
		CardButtons.Add(CardButton);

		UVerticalBox* CardContent = WidgetTree->ConstructWidget<UVerticalBox>(
		    UVerticalBox::StaticClass(), *FString::Printf(TEXT("TraitCardContent%d"), CardIndex));
		CardButton->SetContent(CardContent);

		UTextBlock* CardName = WidgetTree->ConstructWidget<UTextBlock>(
		    UTextBlock::StaticClass(), *FString::Printf(TEXT("TraitCardName%d"), CardIndex));
		CardName->SetJustification(ETextJustify::Center);
		CardName->SetColorAndOpacity(FSlateColor(FLinearColor(0.15f, 0.10f, 0.065f, 1.0f)));
		FSlateFontInfo NameFont = CardName->GetFont();
		NameFont.Size = 27;
		CardName->SetFont(NameFont);
		UVerticalBoxSlot* NameSlot = CardContent->AddChildToVerticalBox(CardName);
		NameSlot->SetHorizontalAlignment(HAlign_Fill);
		NameSlot->SetPadding(FMargin(16.0f, 92.0f, 16.0f, 24.0f));
		CardNames.Add(CardName);

		UTextBlock* CardDescription = WidgetTree->ConstructWidget<UTextBlock>(
		    UTextBlock::StaticClass(), *FString::Printf(TEXT("TraitCardDescription%d"), CardIndex));
		CardDescription->SetJustification(ETextJustify::Center);
		CardDescription->SetAutoWrapText(true);
		CardDescription->SetColorAndOpacity(FSlateColor(FLinearColor(0.20f, 0.14f, 0.09f, 1.0f)));
		FSlateFontInfo DescriptionFont = CardDescription->GetFont();
		DescriptionFont.Size = 18;
		CardDescription->SetFont(DescriptionFont);
		UVerticalBoxSlot* DescriptionSlot = CardContent->AddChildToVerticalBox(CardDescription);
		DescriptionSlot->SetHorizontalAlignment(HAlign_Fill);
		DescriptionSlot->SetPadding(FMargin(22.0f));
		CardDescriptions.Add(CardDescription);
	}
}

void UReEchoTraitCardChoiceWidget::RefreshOffers()
{
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