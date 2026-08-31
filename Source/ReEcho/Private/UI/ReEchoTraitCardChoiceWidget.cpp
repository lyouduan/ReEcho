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
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "ReEchoAudioEvents.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/Framework/ReEchoUIFlowCoordinatorSubsystem.h"
#include "UI/ReEchoIndexedButton.h"
#include "UI/ReEchoTraitCardEntryWidget.h"
#include "UI/Framework/ReEchoUIInteractionAudit.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "ReEchoGameMode.h"

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

void ApplyRefreshButtonArt(UButton* Button, UTexture2D* Texture)
{
	if (!Button || !Texture)
	{
		return;
	}

	FButtonStyle Style = Button->GetStyle();
	auto ConfigureBrush = [Texture](FSlateBrush& Brush)
	{
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = FVector2D(210.0f, 48.0f);
		Brush.TintColor = FSlateColor(FLinearColor::White);
	};
	ConfigureBrush(Style.Normal);
	ConfigureBrush(Style.Hovered);
	ConfigureBrush(Style.Pressed);
	ConfigureBrush(Style.Disabled);
	Style.NormalPadding = FMargin(0.0f);
	Style.PressedPadding = FMargin(0.0f);
	Button->SetStyle(Style);
	Button->SetBackgroundColor(FLinearColor::White);
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
	static ConstructorHelpers::FClassFinder<UReEchoTraitCardEntryWidget> CardEntryClassFinder(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoTraitCardEntry"));
	CardEntryWidgetClass = CardEntryClassFinder.Class;
	static ConstructorHelpers::FObjectFinder<UTexture2D> RefreshButtonTextureFinder(
	    TEXT("/Game/ReEcho/Textures/UI/InteractionPlaceholder/PauseAndCombat/"
	         "T_UI_Pause_ButtonLight.T_UI_Pause_ButtonLight"));
	RefreshButtonTexture = RefreshButtonTextureFinder.Object;
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
	EnsureConfirmButton();
	EnsureShopCancelButton();
	BuildCardEntries();

	for (UReEchoIndexedButton* CardButton : CardButtons)
	{
		CardButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoTraitCardChoiceWidget::HandleCardClicked);
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
			        GameInstance->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
			{
				UIFlow->BindButtonAudioFeedback(
				    CardButton, FReEchoAudioEvents::UiHover, FReEchoAudioEvents::UiCardSelect);
			}
		}
	}
	for (UReEchoTraitCardEntryWidget* CardEntry : CardEntries)
	{
		CardEntry->OnEntrySelected.AddUniqueDynamic(this, &UReEchoTraitCardChoiceWidget::HandleCardClicked);
	}
	for (UReEchoIndexedButton* RefreshButton : CardRefreshButtons)
	{
		RefreshButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoTraitCardChoiceWidget::HandleCardRefreshClicked);
	}
	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.AddUniqueDynamic(this, &UReEchoTraitCardChoiceWidget::HandleConfirmClicked);
	}
	if (ShopCancelButton)
	{
		ShopCancelButton->OnClicked.AddUniqueDynamic(this, &UReEchoTraitCardChoiceWidget::HandleShopCancelClicked);
	}

	RefreshOffers();
	ResetRevealAnimation();
}

void UReEchoTraitCardChoiceWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AdvanceRevealAnimation(InDeltaTime);
}

void UReEchoTraitCardChoiceWidget::AdvanceRevealAnimation(const float DeltaSeconds)
{
	if (bRevealComplete)
	{
		return;
	}
	RevealElapsed += FMath::Max(0.0f, DeltaSeconds);

	if (RevealingCardIndex == INDEX_NONE && NeedleWidget)
	{
		const float SpinProgress = FMath::Clamp(RevealElapsed / NeedleSpinDuration, 0.0f, 1.0f);
		const float DeceleratedProgress = 1.0f - FMath::Pow(1.0f - SpinProgress, 3.0f);
		NeedleWidget->SetRenderTransformAngle(1440.0f * DeceleratedProgress + 25.0f);
	}

	bool bAllCardsRevealed = false;
	if (RevealingCardIndex != INDEX_NONE && CardPanels.IsValidIndex(RevealingCardIndex))
	{
		const float Progress = FMath::Clamp(RevealElapsed / CardRevealDuration, 0.0f, 1.0f);
		const float EasedProgress = EaseOutBack(Progress);
		CardPanels[RevealingCardIndex]->SetRenderOpacity(Progress);
		CardPanels[RevealingCardIndex]->SetRenderScale(FVector2D(FMath::Lerp(0.72f, 1.0f, EasedProgress)));
		CardPanels[RevealingCardIndex]->SetRenderTranslation(FVector2D(0.0f, FMath::Lerp(95.0f, 0.0f, EasedProgress)));
		bAllCardsRevealed = Progress >= 1.0f;
	}
	else
	{
		bAllCardsRevealed = true;
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
	}

	if (bAllCardsRevealed && !bRevealComplete)
	{
		bRevealComplete = true;
		for (UButton* CardButton : CardButtons)
		{
			const int32 CardIndex = CardButtons.IndexOfByKey(CardButton);
			CardButton->SetIsEnabled(CanSelectOffer(CardIndex));
		}
		for (UReEchoTraitCardEntryWidget* CardEntry : CardEntries)
		{
			const int32 CardIndex = CardEntries.IndexOfByKey(CardEntry);
			CardEntry->SetSelectionEnabled(CanSelectOffer(CardIndex));
		}
		for (int32 CardIndex = 0; CardIndex < CardRefreshButtons.Num(); ++CardIndex)
		{
			const bool bCanRefresh = Offers.IsValidIndex(CardIndex) && Offers[CardIndex].bCanRefresh &&
			                         Offers[CardIndex].RefreshCost <= CurrentTimeShards;
			CardRefreshButtons[CardIndex]->SetIsEnabled(bCanRefresh);
		}
		if (!CardEntries.IsEmpty())
		{
			CardEntries[0]->FocusSelection();
		}
		else if (!CardButtons.IsEmpty())
		{
			CardButtons[0]->SetKeyboardFocus();
		}
	}
}

#if WITH_DEV_AUTOMATION_TESTS
void UReEchoTraitCardChoiceWidget::AdvanceRevealAnimationForTesting(const float DeltaSeconds)
{
	AdvanceRevealAnimation(DeltaSeconds);
}
#endif

void UReEchoTraitCardChoiceWidget::InitializeOffers(const TArray<FReEchoTraitCardOffer>& InOffers,
                                                    const int32 InTimeShards,
                                                    const int32 RefreshedSlotIndex)
{
	bShopMode = false;
	ShopTier = 0;
	ShopOffers.Reset();
	Offers = InOffers;
	CurrentTimeShards = InTimeShards;
	if (ShopCancelButton)
	{
		ShopCancelButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	RefreshOffers();
	if (RefreshedSlotIndex == INDEX_NONE)
	{
		ResetRevealAnimation();
	}
	else
	{
		ResetRevealAnimationForSlot(RefreshedSlotIndex);
	}
}

void UReEchoTraitCardChoiceWidget::InitializeShopOffers(const TArray<FReEchoShopCardChoiceOffer>& InOffers,
                                                        const int32 InTimeShards,
                                                        const int32 Tier,
                                                        const int32 RefreshedSlotIndex)
{
	bShopMode = true;
	ShopTier = Tier;
	ShopOffers = InOffers;
	CurrentTimeShards = InTimeShards;
	Offers.Reset();
	for (const FReEchoShopCardChoiceOffer& Choice : ShopOffers)
	{
		FReEchoTraitCardOffer Offer;
		Offer.CardId = Choice.CardId;
		Offer.Tier = Choice.Tier;
		Offer.PresentationTier = Choice.PresentationTier;
		Offer.DisplayName = Choice.DisplayName;
		Offer.Description = Choice.EffectText;
		Offer.Tags = Choice.Tags;
		Offer.SlotIndex = Choice.SlotIndex;
		Offer.RemainingRefreshes = Choice.RemainingRefreshes;
		Offer.RefreshCost = Choice.RefreshCost;
		Offer.bCanRefresh = Choice.bCanRefresh;
		Offers.Add(MoveTemp(Offer));
	}
	EnsureShopCancelButton();
	if (ShopCancelButton)
	{
		ShopCancelButton->SetVisibility(ESlateVisibility::Visible);
	}
	RefreshOffers();
	if (RefreshedSlotIndex == INDEX_NONE)
	{
		ResetRevealAnimation();
	}
	else
	{
		ResetRevealAnimationForSlot(RefreshedSlotIndex);
	}
}

void UReEchoTraitCardChoiceWidget::RestoreChoiceFailure(const int32 InTimeShards)
{
	CurrentTimeShards = InTimeShards;
	SelectedOfferIndex = INDEX_NONE;
	SelectedOfferIndices.Reset();
	bRevealComplete = true;
	RefreshOffers();
}

void UReEchoTraitCardChoiceWidget::BuildWidgetTree()
{
	// An authored WBP root is the presentation authority even when optional fallback-only
	// widgets are deliberately absent. Never replace designer layout just to recreate them.
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas =
	    WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TraitDrawRoot"));
	bNativeFallbackLayout = true;
	WidgetTree->RootWidget = RootCanvas;

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

void UReEchoTraitCardChoiceWidget::EnsureConfirmButton()
{
	if (!WidgetTree || ConfirmButton)
	{
		return;
	}
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}
	ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TraitCardChoiceConfirmButton"));
	ConfirmButton->SetBackgroundColor(FLinearColor(0.20f, 0.14f, 0.05f, 0.96f));
	UTextBlock* Label =
	    CreateCenteredText(WidgetTree, TEXT("TraitCardChoiceConfirmText"), 24, FLinearColor(0.96f, 0.87f, 0.62f));
	Label->SetText(NSLOCTEXT("ReEcho", "TraitCardChoiceConfirm", "确认选择"));
	ConfirmButton->SetContent(Label);
	UCanvasPanelSlot* ConfirmSlot = RootCanvas->AddChildToCanvas(ConfirmButton);
	ConfirmSlot->SetAnchors(FAnchors(0.5f, 0.92f));
	ConfirmSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	ConfirmSlot->SetSize(FVector2D(220.0f, 64.0f));
	ConfirmSlot->SetZOrder(20);
	ConfirmButton->OnClicked.AddUniqueDynamic(this, &UReEchoTraitCardChoiceWidget::HandleConfirmClicked);
}

void UReEchoTraitCardChoiceWidget::EnsureShopCancelButton()
{
	if (!WidgetTree || ShopCancelButton)
	{
		return;
	}
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}
	ShopCancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ShopCardChoiceCancelButton"));
	ShopCancelButton->SetBackgroundColor(FLinearColor(0.08f, 0.09f, 0.11f, 0.94f));
	UTextBlock* Label =
	    CreateCenteredText(WidgetTree, TEXT("ShopCardChoiceCancelText"), 20, FLinearColor(0.92f, 0.82f, 0.62f));
	Label->SetText(NSLOCTEXT("ReEcho", "ShopCardChoiceCancel", "返回商店"));
	ShopCancelButton->SetContent(Label);
	ShopCancelButton->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* CancelSlot = RootCanvas->AddChildToCanvas(ShopCancelButton);
	CancelSlot->SetAnchors(FAnchors(0.82f, 0.075f));
	CancelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CancelSlot->SetSize(FVector2D(180.0f, 54.0f));
	CancelSlot->SetZOrder(20);
	ShopCancelButton->OnClicked.AddUniqueDynamic(this, &UReEchoTraitCardChoiceWidget::HandleShopCancelClicked);
}

void UReEchoTraitCardChoiceWidget::BuildCardEntries()
{
	if (!WidgetTree || !TraitCardContainer)
	{
		return;
	}

	const TArray<USizeBox*> DesignerCardSlots = {TraitCardSlot0, TraitCardSlot1, TraitCardSlot2};
	const bool bUseDesignerCardSlots = TraitCardSlot0 && TraitCardSlot1 && TraitCardSlot2;
	const TArray<UReEchoIndexedButton*> DesignerRefreshButtons = {
	    ShopCardRefreshButton0, ShopCardRefreshButton1, ShopCardRefreshButton2};
	const TArray<UTextBlock*> DesignerRefreshTexts = {ShopCardRefreshText0, ShopCardRefreshText1, ShopCardRefreshText2};
	const bool bUseDesignerRefreshButtons = ShopCardRefreshButton0 && ShopCardRefreshButton1 &&
	                                        ShopCardRefreshButton2 && ShopCardRefreshText0 && ShopCardRefreshText1 &&
	                                        ShopCardRefreshText2;
	for (USizeBox* CardPanel : CardPanels)
	{
		if (!bUseDesignerCardSlots && CardPanel && CardPanel->GetParent() == TraitCardContainer)
		{
			TraitCardContainer->RemoveChild(CardPanel);
		}
	}
	for (UReEchoIndexedButton* RefreshButton : CardRefreshButtons)
	{
		if (RefreshButton && !DesignerRefreshButtons.Contains(RefreshButton) &&
		    RefreshButton->GetParent() == TraitCardContainer)
		{
			TraitCardContainer->RemoveChild(RefreshButton);
		}
	}
	CardButtons.Reset();
	CardRefreshButtons.Reset();
	CardRefreshTexts.Reset();
	CardEntries.Reset();
	CardPanels.Reset();
	CardNames.Reset();
	CardDescriptions.Reset();

	const TArray<FAnchors> CardAnchors = {FAnchors(0.26f, 0.62f), FAnchors(0.50f, 0.59f), FAnchors(0.74f, 0.62f)};
	const TArray<FText> FallbackCardKickers = {NSLOCTEXT("ReEcho", "TraitCandidateOne", "候选 I"),
	                                           NSLOCTEXT("ReEcho", "TraitCandidateTwo", "候选 II"),
	                                           NSLOCTEXT("ReEcho", "TraitCandidateThree", "候选 III")};
	for (int32 CardIndex = 0; CardIndex < 3; ++CardIndex)
	{
		USizeBox* CardSize = bUseDesignerCardSlots ? DesignerCardSlots[CardIndex] : nullptr;
		if (!CardSize)
		{
			CardSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
			                                                 *FString::Printf(TEXT("TraitCardSize%d"), CardIndex));
			CardSize->SetWidthOverride(310.0f);
			CardSize->SetHeightOverride(390.0f);
			UCanvasPanelSlot* CardSlot = TraitCardContainer->AddChildToCanvas(CardSize);
			CardSlot->SetAnchors(CardAnchors[CardIndex]);
			CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CardSlot->SetSize(FVector2D(310.0f, 390.0f));
			CardSlot->SetZOrder(5 + CardIndex);
		}
		CardSize->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		CardPanels.Add(CardSize);
		if (CardEntryWidgetClass)
		{
			UReEchoTraitCardEntryWidget* CardEntry = WidgetTree->ConstructWidget<UReEchoTraitCardEntryWidget>(
			    CardEntryWidgetClass, *FString::Printf(TEXT("TraitCardEntry%d"), CardIndex));
			CardSize->SetContent(CardEntry);
			CardEntries.Add(CardEntry);
			continue;
		}

		UReEchoIndexedButton* CardButton = WidgetTree->ConstructWidget<UReEchoIndexedButton>(
		    UReEchoIndexedButton::StaticClass(), *FString::Printf(TEXT("TraitCardButton%d"), CardIndex));
		CardButton->SetEntryIndex(CardIndex);
		CardButton->SetBackgroundColor(FLinearColor::Transparent);
		CardButton->SetIsEnabled(false);
		CardSize->SetContent(CardButton);
		CardButtons.Add(CardButton);

		UVerticalBox* CardContent = WidgetTree->ConstructWidget<UVerticalBox>(
		    UVerticalBox::StaticClass(), *FString::Printf(TEXT("TraitCardContent%d"), CardIndex));
		CardButton->SetContent(CardContent);

		UTextBlock* CardKicker = CreateCenteredText(
		    WidgetTree, *FString::Printf(TEXT("TraitCardKicker%d"), CardIndex), 18, FLinearColor(0.48f, 0.90f, 0.88f));
		CardKicker->SetText(FallbackCardKickers[CardIndex]);
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
	for (int32 CardIndex = 0; CardIndex < 3; ++CardIndex)
	{
		UReEchoIndexedButton* RefreshButton = bUseDesignerRefreshButtons ? DesignerRefreshButtons[CardIndex] : nullptr;
		if (!RefreshButton)
		{
			RefreshButton = WidgetTree->ConstructWidget<UReEchoIndexedButton>(
			    UReEchoIndexedButton::StaticClass(), *FString::Printf(TEXT("ShopCardRefreshButton%d"), CardIndex));
			ApplyRefreshButtonArt(RefreshButton, RefreshButtonTexture);
		}
		RefreshButton->SetEntryIndex(CardIndex);
		UTextBlock* RefreshText = bUseDesignerRefreshButtons ? DesignerRefreshTexts[CardIndex] : nullptr;
		if (!RefreshText)
		{
			RefreshText = CreateCenteredText(WidgetTree,
			                                 *FString::Printf(TEXT("ShopCardRefreshText%d"), CardIndex),
			                                 17,
			                                 FLinearColor(0.96f, 0.90f, 0.70f));
			RefreshText->SetText(
			    NSLOCTEXT("ReEcho", "ShopCardSlotRefreshAuthored", "刷新（剩余 {Remaining}） · {Cost}"));
			RefreshButton->SetContent(RefreshText);
		}
		RefreshButton->SetVisibility(ESlateVisibility::Collapsed);
		if (!bUseDesignerRefreshButtons)
		{
			UCanvasPanelSlot* RefreshSlot = TraitCardContainer->AddChildToCanvas(RefreshButton);
			RefreshSlot->SetAnchors(CardAnchors[CardIndex]);
			RefreshSlot->SetAlignment(FVector2D(0.5f, -3.65f));
			RefreshSlot->SetSize(FVector2D(210.0f, 48.0f));
			RefreshSlot->SetZOrder(30 + CardIndex);
		}
		RefreshButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoTraitCardChoiceWidget::HandleCardRefreshClicked);
		CardRefreshButtons.Add(RefreshButton);
		CardRefreshTexts.Add(RefreshText);
	}
	RefreshOffers();
}

FString UReEchoTraitCardChoiceWidget::ResolveCardArtTexturePath(const int32 Tier)
{
	if (Tier < 1)
	{
		return FString();
	}
	return FString::Printf(TEXT("/Game/ReEcho/Textures/UI/Cards/Art/T_UI_CardTier%d.T_UI_CardTier%d"), Tier, Tier);
}

FString UReEchoTraitCardChoiceWidget::ResolveCardIconTexturePath(const FName CardId)
{
	return FString::Printf(TEXT("/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_%s.T_UI_CardIcon_%s"),
	                       *CardId.ToString(),
	                       *CardId.ToString());
}

FText UReEchoTraitCardChoiceWidget::GetAuthoredTextTemplate(UTextBlock* Text, const TArray<FString>& NumberArguments)
{
	if (!Text)
	{
		return FText::GetEmpty();
	}
	const TWeakObjectPtr<UTextBlock> Key(Text);
	if (const FText* Cached = AuthoredTextTemplates.Find(Key))
	{
		return *Cached;
	}
	const FString Source = Text->GetText().ToString();
	FString Pattern;
	// Explicit FText arguments take precedence; numeric literals can then be intentional copy.
	if (Source.Contains(TEXT("{")))
	{
		Pattern = Source;
	}
	else
	{
		int32 ArgumentIndex = 0;
		for (int32 Index = 0; Index < Source.Len();)
		{
			if (ArgumentIndex < NumberArguments.Num() && Source[Index] >= TEXT('0') && Source[Index] <= TEXT('9'))
			{
				Pattern += TEXT("{") + NumberArguments[ArgumentIndex++] + TEXT("}");
				do
				{
					++Index;
				} while (Index < Source.Len() && Source[Index] >= TEXT('0') && Source[Index] <= TEXT('9'));
			}
			else
			{
				Pattern.AppendChar(Source[Index++]);
			}
		}
	}
	const FText Template = FText::FromString(Pattern);
	AuthoredTextTemplates.Add(Key, Template);
	return Template;
}

void UReEchoTraitCardChoiceWidget::RefreshOffers()
{
	if (TitleText)
	{
		const FText PickCount = FText::AsNumber(SelectableCount);
		const FText AuthoredTitle =
		    bNativeFallbackLayout ? FText::GetEmpty() : GetAuthoredTextTemplate(TitleText, {TEXT("Count")});
		if (!bNativeFallbackLayout)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Count"), PickCount);
			Arguments.Add(TEXT("Tier"),
			              FText::FromString(ShopTier == 1   ? TEXT("一级")
			                                : ShopTier == 2 ? TEXT("二级")
			                                                : TEXT("三级")));
			TitleText->SetText(FText::Format(AuthoredTitle, Arguments));
		}
		else
		{
			TitleText->SetText(
			    SelectableCount > 1
			        ? FText::Format(NSLOCTEXT("ReEcho",
			                                  "CardChoiceCadenceTitle",
			                                  "从破碎的刻度中，你读出了更多的可能...本次可以选择{0}张！"),
			                        PickCount)
			        : (bShopMode
			               ? FText::Format(NSLOCTEXT("ReEcho", "ShopCardChoiceTitle", "选择{0}张{1}构筑卡牌"),
			                               PickCount,
			                               FText::FromString(ShopTier == 1   ? TEXT("一级")
			                                                 : ShopTier == 2 ? TEXT("二级")
			                                                                 : TEXT("三级")))
			               : FText::Format(NSLOCTEXT("ReEcho", "TraitChoiceTitle", "选择{0}张构筑卡牌"), PickCount)));
		}
	}
	if (SubtitleText)
	{
		// The cadence flavour line now lives in the title, so keep the subtitle a short actionable hint
		// instead of rendering the same sentence twice.
		SubtitleText->SetText(
		    SelectableCount > 1
		        ? FText::Format(NSLOCTEXT("ReEcho", "CardChoiceCadenceHint", "请选满 {0} 张后点击确认"),
		                        FText::AsNumber(SelectableCount))
		        : (bShopMode ? NSLOCTEXT("ReEcho", "ShopCardChoiceSubtitle", "卡牌组已付款；选择1张卡牌完成领取")
		                     : NSLOCTEXT("ReEcho",
		                                 "TraitChoiceSubtitle",
		                                 "完成本次构筑选择后，将进入时光商城使用碎片购买道具")));
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
		CardButtons[CardIndex]->SetIsEnabled(bHasOffer && bRevealComplete && CanSelectOffer(CardIndex));
		if (bHasOffer)
		{
			CardNames[CardIndex]->SetText(Offers[CardIndex].DisplayName);
			CardDescriptions[CardIndex]->SetText(Offers[CardIndex].Description);
		}
	}
	for (int32 CardIndex = 0; CardIndex < CardEntries.Num(); ++CardIndex)
	{
		const bool bHasOffer = Offers.IsValidIndex(CardIndex);
		CardPanels[CardIndex]->SetVisibility(bHasOffer ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		CardEntries[CardIndex]->SetSelectionEnabled(bHasOffer && bRevealComplete && CanSelectOffer(CardIndex));
		if (bHasOffer)
		{
			FReEchoTraitCardOffer& Offer = Offers[CardIndex];
			if (!Offer.CardArt)
			{
				Offer.CardArt = LoadObject<UTexture2D>(nullptr, *ResolveCardArtTexturePath(Offer.PresentationTier));
			}
			if (!Offer.CardIcon)
			{
				Offer.CardIcon = LoadObject<UTexture2D>(nullptr, *ResolveCardIconTexturePath(Offer.CardId));
				if (!Offer.CardIcon)
				{
					Offer.CardIcon =
					    LoadObject<UTexture2D>(nullptr,
					                           TEXT("/Game/ReEcho/Textures/UI/InteractionPlaceholder/"
					                                "InventoryShop/T_UI_Shop_CardIcon.T_UI_Shop_CardIcon"));
				}
			}
			CardEntries[CardIndex]->Configure(
			    CardIndex, Offer.DisplayName, Offer.Description, Offer.CardArt, Offer.CardIcon);
		}
	}
	for (int32 CardIndex = 0; CardIndex < CardRefreshButtons.Num(); ++CardIndex)
	{
		const bool bHasRefreshableOffer = Offers.IsValidIndex(CardIndex) && Offers[CardIndex].SlotIndex != INDEX_NONE;
		CardRefreshButtons[CardIndex]->SetVisibility(bHasRefreshableOffer ? ESlateVisibility::Visible
		                                                                  : ESlateVisibility::Collapsed);
		if (!bHasRefreshableOffer)
		{
			continue;
		}
		const FReEchoTraitCardOffer& Offer = Offers[CardIndex];
		FText Template = GetAuthoredTextTemplate(CardRefreshTexts[CardIndex], {TEXT("Remaining"), TEXT("Cost")});
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Remaining"), FText::AsNumber(Offer.RemainingRefreshes));
		Arguments.Add(TEXT("Cost"), FText::AsNumber(Offer.RefreshCost));
		CardRefreshTexts[CardIndex]->SetText(FText::Format(Template, Arguments));
		CardRefreshButtons[CardIndex]->SetIsEnabled(bRevealComplete && Offer.bCanRefresh &&
		                                            Offer.RefreshCost <= CurrentTimeShards);
	}
	RefreshSelectionVisuals();
	if (bNativeFallbackLayout)
	{
		ApplyFallbackTitleLayout();
	}
}

/** Vertical breathing room between the pick hint and the top of the tallest card frame. */
static constexpr float TraitChoiceTitleCardGap = 26.0f;

void UReEchoTraitCardChoiceWidget::ApplyFallbackTitleLayout()
{
	if (!TitleText)
	{
		return;
	}
	// The card frames are the runtime layout authority. Park the pick hint just above their top edge so it
	// can never sit on top of the page chrome the designer laid out further up the canvas.
	float CardTopY = TNumericLimits<float>::Max();
	for (const TObjectPtr<USizeBox>& CardPanel : CardPanels)
	{
		const UCanvasPanelSlot* CardSlot = CardPanel ? Cast<UCanvasPanelSlot>(CardPanel->Slot) : nullptr;
		if (!CardSlot)
		{
			continue;
		}
		CardTopY = FMath::Min(CardTopY, CardSlot->GetPosition().Y - CardSlot->GetSize().Y * CardSlot->GetAlignment().Y);
	}
	if (!FMath::IsFinite(CardTopY))
	{
		return;
	}
	UCanvasPanelSlot* TitleSlot = Cast<UCanvasPanelSlot>(TitleText->Slot);
	if (!TitleSlot)
	{
		// The authored layout may keep the hint inside a header container. Move that container instead so
		// the fallback layout and any designer grouping are both covered.
		UWidget* Ancestor = TitleText->GetParent();
		while (Ancestor && !TitleSlot)
		{
			TitleSlot = Cast<UCanvasPanelSlot>(Ancestor->Slot);
			Ancestor = Ancestor->GetParent();
		}
	}
	if (!TitleSlot)
	{
		return;
	}
	const FVector2D TitleSize = TitleSlot->GetSize();
	// Keep the designer's X placement; only move the hint vertically so its bottom edge clears the cards.
	const float TitleBottomY = CardTopY - TraitChoiceTitleCardGap;
	TitleSlot->SetPosition(
	    FVector2D(TitleSlot->GetPosition().X, TitleBottomY - TitleSize.Y * (1.0f - TitleSlot->GetAlignment().Y)));
}

void UReEchoTraitCardChoiceWidget::RefreshSelectionVisuals()
{
	const bool bHasSelection = !SelectedOfferIndices.IsEmpty();
	for (int32 CardIndex = 0; CardIndex < CardEntries.Num(); ++CardIndex)
	{
		CardEntries[CardIndex]->SetSelectedVisual(SelectedOfferIndices.Contains(CardIndex), bHasSelection);
	}
	for (int32 CardIndex = 0; CardIndex < CardButtons.Num(); ++CardIndex)
	{
		CardButtons[CardIndex]->SetRenderOpacity(!bHasSelection || SelectedOfferIndices.Contains(CardIndex) ? 1.0f
		                                                                                                    : 0.38f);
	}
	if (ConfirmButton)
	{
		bool bAllSelectable = true;
		for (const int32 OfferIndex : SelectedOfferIndices)
		{
			if (!CanSelectOffer(OfferIndex))
			{
				bAllSelectable = false;
			}
		}
		// 智者双选必须恰好选满两张后才能确认，避免少选直接结算。
		const bool bCanConfirm = bRevealComplete && SelectedOfferIndices.Num() == SelectableCount && bAllSelectable;
		ConfirmButton->SetIsEnabled(bCanConfirm);
		ConfirmButton->SetRenderOpacity(bCanConfirm ? 1.0f : 0.48f);
	}
}

void UReEchoTraitCardChoiceWidget::ResetRevealAnimation()
{
	RevealElapsed = 0.0f;
	bRevealComplete = false;
	SelectedOfferIndex = INDEX_NONE;
	SelectedOfferIndices.Reset();
	RevealingCardIndex = INDEX_NONE;
	for (int32 CardIndex = 0; CardIndex < CardPanels.Num(); ++CardIndex)
	{
		CardPanels[CardIndex]->SetRenderOpacity(0.0f);
		CardPanels[CardIndex]->SetRenderScale(FVector2D(0.72f));
		CardPanels[CardIndex]->SetRenderTranslation(FVector2D(0.0f, 95.0f));
		if (CardButtons.IsValidIndex(CardIndex))
		{
			CardButtons[CardIndex]->SetIsEnabled(false);
		}
		if (CardEntries.IsValidIndex(CardIndex))
		{
			CardEntries[CardIndex]->SetSelectionEnabled(false);
		}
	}
	RefreshSelectionVisuals();
}

void UReEchoTraitCardChoiceWidget::ResetRevealAnimationForSlot(const int32 SlotIndex)
{
	const int32 CardIndex = Offers.IndexOfByPredicate(
	    [SlotIndex](const FReEchoTraitCardOffer& Offer)
	    {
		    return Offer.SlotIndex == SlotIndex;
	    });
	SelectedOfferIndex = INDEX_NONE;
	if (!CardPanels.IsValidIndex(CardIndex))
	{
		RevealingCardIndex = INDEX_NONE;
		bRevealComplete = true;
		RefreshOffers();
		return;
	}

	RevealElapsed = 0.0f;
	bRevealComplete = false;
	RevealingCardIndex = CardIndex;
	CardPanels[CardIndex]->SetRenderOpacity(0.0f);
	CardPanels[CardIndex]->SetRenderScale(FVector2D(0.72f));
	CardPanels[CardIndex]->SetRenderTranslation(FVector2D(0.0f, 95.0f));
	for (UButton* CardButton : CardButtons)
	{
		CardButton->SetIsEnabled(false);
	}
	for (UReEchoTraitCardEntryWidget* CardEntry : CardEntries)
	{
		CardEntry->SetSelectionEnabled(false);
	}
	for (UReEchoIndexedButton* RefreshButton : CardRefreshButtons)
	{
		RefreshButton->SetIsEnabled(false);
	}
	RefreshSelectionVisuals();
}

void UReEchoTraitCardChoiceWidget::SelectOffer(const int32 OfferIndex)
{
	if (bRevealComplete && Offers.IsValidIndex(OfferIndex) && CanSelectOffer(OfferIndex))
	{
		bRevealComplete = false;
		for (UButton* CardButton : CardButtons)
		{
			CardButton->SetIsEnabled(false);
		}
		for (UReEchoTraitCardEntryWidget* CardEntry : CardEntries)
		{
			CardEntry->SetSelectionEnabled(false);
		}
		if (bShopMode && ShopOffers.IsValidIndex(OfferIndex))
		{
			OnShopCardSelected.Broadcast(ShopOffers[OfferIndex].ItemId);
		}
		else
		{
			OnCardSelected.Broadcast(Offers[OfferIndex].CardId);
		}
	}
}

bool UReEchoTraitCardChoiceWidget::CanSelectOffer(const int32 OfferIndex) const
{
	return Offers.IsValidIndex(OfferIndex) && (!bShopMode || ShopOffers.IsValidIndex(OfferIndex));
}

void UReEchoTraitCardChoiceWidget::HandleCardClicked(const int32 OfferIndex)
{
	if (!bRevealComplete || !Offers.IsValidIndex(OfferIndex))
	{
		return;
	}

	// Every supported layout must expose an explicit confirmation control.
	if (!ConfirmButton)
	{
		return;
	}

	// Cadence packs only collect selections here. The player may click a selected card again to cancel it;
	// after exactly the required number is selected, the dedicated confirmation button performs the grant.
	if (SelectableCount > 1)
	{
		if (SelectedOfferIndices.Contains(OfferIndex))
		{
			SelectedOfferIndices.Remove(OfferIndex);
		}
		else if (SelectedOfferIndices.Num() < SelectableCount)
		{
			SelectedOfferIndices.Add(OfferIndex);
		}
		SelectedOfferIndex = SelectedOfferIndices.IsEmpty() ? INDEX_NONE : SelectedOfferIndices[0];
		RefreshSelectionVisuals();
		return;
	}

	SelectedOfferIndices.Reset();
	SelectedOfferIndices.Add(OfferIndex);
	SelectedOfferIndex = OfferIndex;
	RefreshSelectionVisuals();
}

void UReEchoTraitCardChoiceWidget::CommitSelectedChoices()
{
	// Shop packs identify cards by ItemId, the free post-encounter pack by CardId.
	TArray<FName> CardIds;
	for (const int32 OfferIndex : SelectedOfferIndices)
	{
		if (bShopMode)
		{
			if (ShopOffers.IsValidIndex(OfferIndex))
			{
				CardIds.Add(ShopOffers[OfferIndex].ItemId);
			}
		}
		else if (Offers.IsValidIndex(OfferIndex))
		{
			CardIds.Add(Offers[OfferIndex].CardId);
		}
	}
	if (CardIds.Num() != SelectableCount)
	{
		return;
	}
	bRevealComplete = false;
	for (UButton* CardButton : CardButtons)
	{
		CardButton->SetIsEnabled(false);
	}
	for (UReEchoTraitCardEntryWidget* CardEntry : CardEntries)
	{
		CardEntry->SetSelectionEnabled(false);
	}
	if (CardIds.Num() > 1)
	{
		if (bShopMode)
		{
			OnShopCardChoicesSelected.Broadcast(CardIds);
		}
		else
		{
			OnCardChoicesSelected.Broadcast(CardIds);
		}
	}
	else
	{
		if (bShopMode)
		{
			OnShopCardSelected.Broadcast(CardIds[0]);
		}
		else
		{
			OnCardSelected.Broadcast(CardIds[0]);
		}
	}
}

void UReEchoTraitCardChoiceWidget::HandleCardRefreshClicked(const int32 OfferIndex)
{
	if (!bRevealComplete || !Offers.IsValidIndex(OfferIndex))
	{
		return;
	}
	const FReEchoTraitCardOffer& Offer = Offers[OfferIndex];
	ReEchoUIInteractionAudit::Write(
	    bShopMode ? TEXT("SHOP_CARD_SLOT_REFRESH_BUTTON") : TEXT("FREE_CARD_SLOT_REFRESH_BUTTON"),
	    FString::Printf(TEXT("tier=%d visibleIndex=%d slot=%d card=%s remaining=%d cost=%d canRefresh=%d shards=%d"),
	                    Offer.Tier,
	                    OfferIndex,
	                    Offer.SlotIndex,
	                    *Offer.CardId.ToString(),
	                    Offer.RemainingRefreshes,
	                    Offer.RefreshCost,
	                    Offer.bCanRefresh ? 1 : 0,
	                    CurrentTimeShards));
	if (Offer.bCanRefresh && Offer.RefreshCost <= CurrentTimeShards)
	{
		for (UReEchoIndexedButton* RefreshButton : CardRefreshButtons)
		{
			RefreshButton->SetIsEnabled(false);
		}
		OnCardSlotRefreshRequested.Broadcast(Offer.SlotIndex);
	}
}

void UReEchoTraitCardChoiceWidget::HandleConfirmClicked()
{
	if (!bRevealComplete || SelectedOfferIndices.Num() != SelectableCount)
	{
		UE_LOG(LogTemp,
		       Warning,
		       TEXT("[CardChoice] Confirm ignored: reveal=%d picks=%d required=%d"),
		       bRevealComplete ? 1 : 0,
		       SelectedOfferIndices.Num(),
		       SelectableCount);
		return;
	}
	CommitSelectedChoices();
}

void UReEchoTraitCardChoiceWidget::SetSelectableCount(const int32 InSelectableCount)
{
	SelectableCount = FMath::Max(1, InSelectableCount);
	SelectedOfferIndices.Reset();
	SelectedOfferIndex = INDEX_NONE;
}

void UReEchoTraitCardChoiceWidget::HandleShopCancelClicked()
{
	if (bShopMode)
	{
		OnShopChoiceCancelled.Broadcast();
	}
}

FReply UReEchoTraitCardChoiceWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::P)
	{
		if (AReEchoGameMode* GameMode = Cast<AReEchoGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GameMode->TogglePauseMenu();
		}
		return FReply::Handled();
	}
	if (bShopMode && InKeyEvent.GetKey() == EKeys::Escape)
	{
		HandleShopCancelClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
