#include "UI/ReEchoInventoryShopWidget.h"

#include "Core/ReEchoTypes.h"
#include "Run/ReEchoRunSubsystem.h"
#include "Run/ReEchoShopCatalog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/ReEchoIndexedButton.h"

namespace
{
UTextBlock* CreateText(UWidgetTree* WidgetTree, const FName Name, const int32 Size, const FLinearColor Color)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetAutoWrapText(true);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = Size;
	Text->SetFont(Font);
	return Text;
}

int32 GetEffectiveShopPrice(const int32 BasePrice, const float Discount)
{
	return FMath::Max(0, FMath::CeilToInt(BasePrice * (1.0f - FMath::Clamp(Discount, 0.0f, 1.0f))));
}

int32 CountDraftPartsForSlot(const TArray<FName>& DraftPartIds,
                             const TArray<FReEchoShopOffer>& OwnedParts,
                             const FName SlotTypeId)
{
	int32 Count = 0;
	for (const FName DraftId : DraftPartIds)
	{
		const FReEchoShopOffer* DraftPart = OwnedParts.FindByPredicate(
		    [&](const FReEchoShopOffer& Owned)
		    {
			    return Owned.ContentId == DraftId;
		    });
		if (DraftPart && DraftPart->SlotTypeId == SlotTypeId)
		{
			++Count;
		}
	}
	return Count;
}
}

UReEchoInventoryShopWidget::UReEchoInventoryShopWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> InventoryBackgroundFinder(
	    TEXT("/Game/ReEcho/Textures/UI/InventoryBackground.InventoryBackground"));
	InventoryBackgroundTexture = InventoryBackgroundFinder.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> ShopBackgroundFinder(
	    TEXT("/Game/ReEcho/Textures/UI/ShopBackground.ShopBackground"));
	ShopBackgroundTexture = ShopBackgroundFinder.Object;
}

TSharedRef<SWidget> UReEchoInventoryShopWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UReEchoInventoryShopWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidgetTree();
	BuildOfferEntries();
	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleCloseClicked);
	}
	for (UReEchoIndexedButton* OfferButton : OfferButtons)
	{
		OfferButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleOfferClicked);
	}
	Refresh();
}

void UReEchoInventoryShopWidget::ShowInventory(const int32 TimeShards, const TArray<FName>& OwnedItems)
{
	Mode = EReEchoInventoryShopMode::Inventory;
	bShowingShop = false;
	CurrentTimeShards = TimeShards;
	CurrentOwnedItems = OwnedItems;
	if (EchoPanel)
	{
		EchoPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CloseConfirmWidget)
	{
		CloseConfirmWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	Refresh();
}

void UReEchoInventoryShopWidget::SetWeaponPartShopView(const FReEchoWeaponPartShopView& PartShopView,
                                                       const bool bResetDraft)
{
	CurrentPartShopView = PartShopView;
	if (bResetDraft || DraftWeaponId != PartShopView.WeaponId)
	{
		DraftWeaponId = PartShopView.WeaponId;
		DraftPartIds.Reset();
		for (const FReEchoEquippedPartSnapshot& Equipped : PartShopView.EquippedParts)
		{
			DraftPartIds.Add(Equipped.PartId);
		}
	}
	BuildOfferEntries();
	BuildLoadoutEntries();
	Refresh();
}

void UReEchoInventoryShopWidget::ShowShop(const int32 TimeShards,
                                          const TArray<FName>& OwnedItems,
                                          const float ShopDiscount,
                                          const int32 FreeRefreshes,
                                          const bool bRefreshAllowed,
                                          const bool bExtraCardPurchaseAllowed,
                                          const int32 RefreshSequence)
{
	Mode = EReEchoInventoryShopMode::ManualShop;
	bShowingShop = true;
	CurrentTimeShards = TimeShards;
	CurrentShopDiscount = FMath::Clamp(ShopDiscount, 0.0f, 1.0f);
	CurrentFreeShopRefreshes = FMath::Max(0, FreeRefreshes);
	bCurrentShopRefreshAllowed = bRefreshAllowed;
	bCurrentExtraCardPurchaseAllowed = bExtraCardPurchaseAllowed;
	CurrentShopRefreshSequence = FMath::Max(0, RefreshSequence);
	CurrentOwnedItems = OwnedItems;
	if (EchoPanel)
	{
		EchoPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CloseConfirmWidget)
	{
		CloseConfirmWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	Refresh();
}

void UReEchoInventoryShopWidget::BuildWidgetTree()
{
	if (!WidgetTree || (WidgetTree->RootWidget && BackgroundImage && InventoryPanel && ShopPanel && CurrencyText &&
	                    InventoryText && CloseButton && OfferContainer))
	{
		return;
	}

	UCanvasPanel* RootCanvas =
	    WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("InventoryShopRoot"));
	WidgetTree->RootWidget = RootCanvas;

	BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MenuBackground"));
	UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(BackgroundImage);
	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0.0f));

	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
	CloseButton->SetBackgroundColor(FLinearColor(0.12f, 0.09f, 0.06f, 0.88f));
	UCanvasPanelSlot* CloseSlot = RootCanvas->AddChildToCanvas(CloseButton);
	CloseSlot->SetAnchors(FAnchors(0.03f, 0.88f));
	CloseSlot->SetPosition(FVector2D::ZeroVector);
	CloseSlot->SetSize(FVector2D(112.0f, 64.0f));
	UTextBlock* CloseText = CreateText(WidgetTree, TEXT("CloseText"), 26, FLinearColor(0.85f, 0.77f, 0.62f));
	CloseText->SetText(NSLOCTEXT("ReEcho", "InventoryShopClose", "返回"));
	CloseText->SetJustification(ETextJustify::Center);
	CloseButton->SetContent(CloseText);

	InventoryPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InventoryOverlay"));
	UCanvasPanelSlot* InventorySlot = RootCanvas->AddChildToCanvas(InventoryPanel);
	InventorySlot->SetAnchors(FAnchors(0.56f, 0.18f, 0.91f, 0.82f));
	InventorySlot->SetOffsets(FMargin(0.0f));

	UTextBlock* InventoryTitle =
	    CreateText(WidgetTree, TEXT("InventoryOverlayTitle"), 31, FLinearColor(0.16f, 0.12f, 0.08f));
	InventoryTitle->SetText(NSLOCTEXT("ReEcho", "OwnedItemsTitle", "本轮持有"));
	InventoryTitle->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* InventoryTitleSlot = InventoryPanel->AddChildToVerticalBox(InventoryTitle);
	InventoryTitleSlot->SetPadding(FMargin(12.0f, 8.0f, 12.0f, 24.0f));

	InventoryText = CreateText(WidgetTree, TEXT("InventoryItems"), 23, FLinearColor(0.13f, 0.1f, 0.07f));
	UVerticalBoxSlot* InventoryTextSlot = InventoryPanel->AddChildToVerticalBox(InventoryText);
	InventoryTextSlot->SetPadding(FMargin(30.0f));

	ShopPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ShopOverlay"));
	UCanvasPanelSlot* ShopSlot = RootCanvas->AddChildToCanvas(ShopPanel);
	ShopSlot->SetAnchors(FAnchors(0.62f, 0.15f, 0.92f, 0.84f));
	ShopSlot->SetOffsets(FMargin(0.0f));

	CurrencyText = CreateText(WidgetTree, TEXT("ShopCurrency"), 29, FLinearColor(0.87f, 0.73f, 0.48f));
	CurrencyText->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* CurrencySlot = ShopPanel->AddChildToVerticalBox(CurrencyText);
	CurrencySlot->SetPadding(FMargin(10.0f, 0.0f, 10.0f, 20.0f));
	OfferContainer = ShopPanel;
}

void UReEchoInventoryShopWidget::BuildShopLogicHost()
{
	if (!WidgetTree || !OfferContainer)
	{
		return;
	}
	if (!ShopLogicScrollBox)
	{
		ShopLogicScrollBox =
		    WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ShopLogicScrollBox"));
		UVerticalBoxSlot* ScrollSlot = OfferContainer->AddChildToVerticalBox(ShopLogicScrollBox);
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ScrollSlot->SetPadding(FMargin(0.0f, 4.0f));
		if (UVerticalBoxSlot* OfferContainerSlot = Cast<UVerticalBoxSlot>(OfferContainer->Slot))
		{
			OfferContainerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}
	if (!ShopLogicPanel)
	{
		ShopLogicPanel =
		    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ShopLogicPanel"));
		ShopLogicScrollBox->AddChild(ShopLogicPanel);
	}
}

void UReEchoInventoryShopWidget::BuildOfferEntries()
{
	if (!WidgetTree || !OfferContainer)
	{
		return;
	}
	BuildShopLogicHost();
	if (!ShopLogicPanel)
	{
		return;
	}

	VisibleRunItemOffers.Reset();
	const TArray<FReEchoShopOffer>& SourceOffers =
	    CurrentPartShopView.Offers.IsEmpty() ? GetReEchoShopCatalog() : CurrentPartShopView.Offers;
	for (const FReEchoShopOffer& Offer : SourceOffers)
	{
		if (Offer.Type == EReEchoShopOfferType::RunItem)
		{
			VisibleRunItemOffers.Add(Offer);
		}
	}
	if (VisibleRunItemOffers.IsEmpty())
	{
		VisibleRunItemOffers = GetReEchoShopCatalog();
	}
	if (!RunItemOfferPanel)
	{
		RunItemOfferPanel =
		    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RunItemOfferPanel"));
		ShopLogicPanel->AddChildToVerticalBox(RunItemOfferPanel);
		UTextBlock* Title = CreateText(WidgetTree, TEXT("RunItemOfferTitle"), 20, FLinearColor(0.9f, 0.82f, 0.66f));
		Title->SetText(NSLOCTEXT("ReEcho", "RunItemOfferTitle", "商品"));
		RunItemOfferPanel->AddChildToVerticalBox(Title);
	}

	if (OfferButtons.Num() != VisibleRunItemOffers.Num())
	{
		for (UReEchoIndexedButton* Button : OfferButtons)
		{
			if (Button)
			{
				Button->RemoveFromParent();
			}
		}
		OfferButtons.Reset();
		OfferTexts.Reset();
	}
	for (int32 OfferIndex = 0; OfferIndex < VisibleRunItemOffers.Num(); ++OfferIndex)
	{
		if (OfferButtons.IsValidIndex(OfferIndex))
		{
			continue;
		}
		UReEchoIndexedButton* OfferButton = WidgetTree->ConstructWidget<UReEchoIndexedButton>(
		    UReEchoIndexedButton::StaticClass(), *FString::Printf(TEXT("ShopOffer%d"), OfferIndex));
		OfferButton->SetEntryIndex(OfferIndex);
		OfferButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleOfferClicked);
		OfferButton->SetBackgroundColor(FLinearColor(0.15f, 0.11f, 0.07f, 0.88f));
		UVerticalBoxSlot* OfferSlot = RunItemOfferPanel->AddChildToVerticalBox(OfferButton);
		OfferSlot->SetPadding(FMargin(8.0f, 6.0f));

		UTextBlock* OfferText = CreateText(
		    WidgetTree, *FString::Printf(TEXT("ShopOfferText%d"), OfferIndex), 20, FLinearColor(0.9f, 0.82f, 0.66f));
		OfferText->SetJustification(ETextJustify::Center);
		OfferButton->SetContent(OfferText);
		OfferButtons.Add(OfferButton);
		OfferTexts.Add(OfferText);
	}

	if (!ShopControlPanel)
	{
		ShopControlPanel =
		    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ShopControlPanel"));
		ShopLogicPanel->AddChildToVerticalBox(ShopControlPanel);
		UTextBlock* Title = CreateText(WidgetTree, TEXT("ShopControlTitle"), 20, FLinearColor(0.9f, 0.82f, 0.66f));
		Title->SetText(NSLOCTEXT("ReEcho", "ShopControlTitle", "商店规则"));
		ShopControlPanel->AddChildToVerticalBox(Title);
	}
	const bool bCreatedRefreshButton = !ShopRefreshButton;
	if (bCreatedRefreshButton)
	{
		ShopRefreshButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ShopRefreshButton"));
		ShopRefreshButton->SetBackgroundColor(FLinearColor(0.18f, 0.3f, 0.42f, 0.9f));
		UVerticalBoxSlot* RefreshSlot = ShopControlPanel->AddChildToVerticalBox(ShopRefreshButton);
		RefreshSlot->SetPadding(FMargin(8.0f, 12.0f, 8.0f, 4.0f));
	}
	if (!ShopRefreshText)
	{
		ShopRefreshText = CreateText(WidgetTree, TEXT("ShopRefreshText"), 20, FLinearColor(0.9f, 0.82f, 0.66f));
		ShopRefreshText->SetJustification(ETextJustify::Center);
		ShopRefreshButton->SetContent(ShopRefreshText);
	}
	else if (bCreatedRefreshButton)
	{
		ShopRefreshButton->SetContent(ShopRefreshText);
	}
	ShopRefreshButton->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleRefreshClicked);
	if (!ShopRuleText)
	{
		ShopRuleText = CreateText(WidgetTree, TEXT("ShopRuleText"), 17, FLinearColor(0.75f, 0.68f, 0.56f));
		ShopRuleText->SetJustification(ETextJustify::Center);
		UVerticalBoxSlot* RuleSlot = ShopControlPanel->AddChildToVerticalBox(ShopRuleText);
		RuleSlot->SetPadding(FMargin(8.0f, 2.0f));
	}

	// ---- Inline echo management panel (origin/main left-side layout) ----
	// Built once (BuildWidgetTree is guarded). Visibility and content are driven by BuildEchoPanel().
	if (EchoPanel)
	{
		return;
	}
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}
	EchoPanelScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("EchoPanelScale"));
	EchoPanelScale->SetStretch(EStretch::ScaleToFit);
	EchoPanelScale->SetStretchDirection(EStretchDirection::DownOnly);
	EchoPanelScale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UCanvasPanelSlot* EchoSlotCanvas = RootCanvas->AddChildToCanvas(EchoPanelScale);
	EchoSlotCanvas->SetAnchors(FAnchors(0.06f, 0.70f, 0.66f, 0.95f));
	EchoSlotCanvas->SetOffsets(FMargin(0.0f));
	EchoSlotCanvas->SetZOrder(20);

	EchoPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EchoPanel"));
	EchoPanel->SetVisibility(ESlateVisibility::Collapsed);
	EchoPanelScale->SetContent(EchoPanel);

	EchoCapacityText = CreateText(WidgetTree, TEXT("EchoCapacity"), 24, FLinearColor(0.9f, 0.8f, 0.55f));
	EchoCapacityText->SetJustification(ETextJustify::Center);
	EchoPanel->AddChildToVerticalBox(EchoCapacityText);

	EchoReplayModeText = CreateText(WidgetTree, TEXT("EchoReplayMode"), 20, FLinearColor(0.85f, 0.75f, 0.6f));
	EchoReplayModeText->SetAutoWrapText(true);
	EchoPanel->AddChildToVerticalBox(EchoReplayModeText);

	EchoPendingInfoText = CreateText(WidgetTree, TEXT("EchoPendingInfo"), 22, FLinearColor(0.95f, 0.8f, 0.5f));
	EchoPendingInfoText->SetAutoWrapText(true);
	EchoPanel->AddChildToVerticalBox(EchoPendingInfoText);

	EchoStoreButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EchoStoreButton"));
	EchoStoreButton->SetBackgroundColor(FLinearColor(0.2f, 0.5f, 0.2f, 0.9f));
	{
		UTextBlock* T = CreateText(WidgetTree, TEXT("EchoStoreLabel"), 22, FLinearColor(1.0f, 1.0f, 1.0f));
		T->SetText(FText::FromString(TEXT("Store pending echo")));
		T->SetJustification(ETextJustify::Center);
		EchoStoreButton->SetContent(T);
	}
	EchoStoreButton->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleStoreClicked);
	EchoPanel->AddChildToVerticalBox(EchoStoreButton);

	EchoSkipButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EchoSkipButton"));
	EchoSkipButton->SetBackgroundColor(FLinearColor(0.5f, 0.2f, 0.2f, 0.9f));
	{
		UTextBlock* T = CreateText(WidgetTree, TEXT("EchoSkipLabel"), 22, FLinearColor(1.0f, 1.0f, 1.0f));
		T->SetText(FText::FromString(TEXT("Skip pending echo")));
		T->SetJustification(ETextJustify::Center);
		EchoSkipButton->SetContent(T);
	}
	EchoSkipButton->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleSkipClicked);
	EchoPanel->AddChildToVerticalBox(EchoSkipButton);

	EchoReplaceInstructionText =
	    CreateText(WidgetTree, TEXT("EchoReplaceInstruction"), 20, FLinearColor(0.95f, 0.8f, 0.5f));
	EchoReplaceInstructionText->SetAutoWrapText(true);
	EchoPanel->AddChildToVerticalBox(EchoReplaceInstructionText);

	EchoCancelReplaceButton =
	    WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EchoCancelReplaceButton"));
	EchoCancelReplaceButton->SetBackgroundColor(FLinearColor(0.4f, 0.4f, 0.4f, 0.9f));
	{
		UTextBlock* T = CreateText(WidgetTree, TEXT("EchoCancelReplaceLabel"), 22, FLinearColor(1.0f, 1.0f, 1.0f));
		T->SetText(FText::FromString(TEXT("Cancel replace")));
		T->SetJustification(ETextJustify::Center);
		EchoCancelReplaceButton->SetContent(T);
	}
	EchoCancelReplaceButton->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleCancelReplaceClicked);
	EchoPanel->AddChildToVerticalBox(EchoCancelReplaceButton);

	const int32 SlotCount = ReEchoEchoStorage::MaxStorageCapacity;
	EchoSlotTexts.SetNum(SlotCount);
	EchoReplaceButtons.SetNum(SlotCount);
	EchoSelectButtons.SetNum(SlotCount);
	EchoSelectLabels.SetNum(SlotCount);
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		EchoSlotTexts[SlotIndex] = CreateText(
		    WidgetTree, *FString::Printf(TEXT("EchoSlotText%d"), SlotIndex), 18, FLinearColor(0.85f, 0.78f, 0.62f));
		EchoSlotTexts[SlotIndex]->SetAutoWrapText(true);
		EchoPanel->AddChildToVerticalBox(EchoSlotTexts[SlotIndex]);

		EchoReplaceButtons[SlotIndex] = WidgetTree->ConstructWidget<UButton>(
		    UButton::StaticClass(), *FString::Printf(TEXT("EchoReplace%d"), SlotIndex));
		EchoReplaceButtons[SlotIndex]->SetBackgroundColor(FLinearColor(0.4f, 0.3f, 0.15f, 0.9f));
		UTextBlock* RLabel = CreateText(
		    WidgetTree, *FString::Printf(TEXT("EchoReplaceLabel%d"), SlotIndex), 18, FLinearColor(1.0f, 1.0f, 1.0f));
		RLabel->SetText(FText::FromString(TEXT("Replace this echo")));
		RLabel->SetJustification(ETextJustify::Center);
		EchoReplaceButtons[SlotIndex]->SetContent(RLabel);
		if (SlotIndex == 0)
		{
			EchoReplaceButtons[SlotIndex]->OnClicked.AddUniqueDynamic(
			    this, &UReEchoInventoryShopWidget::HandleReplaceSlot0Clicked);
		}
		else if (SlotIndex == 1)
		{
			EchoReplaceButtons[SlotIndex]->OnClicked.AddUniqueDynamic(
			    this, &UReEchoInventoryShopWidget::HandleReplaceSlot1Clicked);
		}
		else
		{
			EchoReplaceButtons[SlotIndex]->OnClicked.AddUniqueDynamic(
			    this, &UReEchoInventoryShopWidget::HandleReplaceSlot2Clicked);
		}
		EchoPanel->AddChildToVerticalBox(EchoReplaceButtons[SlotIndex]);

		EchoSelectButtons[SlotIndex] = WidgetTree->ConstructWidget<UButton>(
		    UButton::StaticClass(), *FString::Printf(TEXT("EchoSelect%d"), SlotIndex));
		EchoSelectButtons[SlotIndex]->SetBackgroundColor(FLinearColor(0.2f, 0.35f, 0.5f, 0.9f));
		EchoSelectLabels[SlotIndex] = CreateText(
		    WidgetTree, *FString::Printf(TEXT("EchoSelectLabel%d"), SlotIndex), 18, FLinearColor(1.0f, 1.0f, 1.0f));
		EchoSelectLabels[SlotIndex]->SetText(FText::FromString(TEXT("Select for replay")));
		EchoSelectLabels[SlotIndex]->SetJustification(ETextJustify::Center);
		EchoSelectButtons[SlotIndex]->SetContent(EchoSelectLabels[SlotIndex]);
		if (SlotIndex == 0)
		{
			EchoSelectButtons[SlotIndex]->OnClicked.AddUniqueDynamic(
			    this, &UReEchoInventoryShopWidget::HandleSelectSlot0Clicked);
		}
		else if (SlotIndex == 1)
		{
			EchoSelectButtons[SlotIndex]->OnClicked.AddUniqueDynamic(
			    this, &UReEchoInventoryShopWidget::HandleSelectSlot1Clicked);
		}
		else
		{
			EchoSelectButtons[SlotIndex]->OnClicked.AddUniqueDynamic(
			    this, &UReEchoInventoryShopWidget::HandleSelectSlot2Clicked);
		}
		EchoPanel->AddChildToVerticalBox(EchoSelectButtons[SlotIndex]);
	}

	EchoSelectionText = CreateText(WidgetTree, TEXT("EchoSelection"), 20, FLinearColor(0.8f, 0.9f, 0.7f));
	EchoSelectionText->SetAutoWrapText(true);
	EchoPanel->AddChildToVerticalBox(EchoSelectionText);

	// Close confirmation overlay (hidden until an undecided pending echo blocks closing).
	CloseConfirmWidget =
	    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EchoCloseConfirm"));
	UCanvasPanelSlot* ConfirmSlot = RootCanvas->AddChildToCanvas(CloseConfirmWidget);
	ConfirmSlot->SetAnchors(FAnchors(0.35f, 0.4f, 0.65f, 0.62f));
	ConfirmSlot->SetOffsets(FMargin(0.0f));
	ConfirmSlot->SetZOrder(30);
	CloseConfirmWidget->SetVisibility(ESlateVisibility::Collapsed);
	{
		UTextBlock* CText = CreateText(WidgetTree, TEXT("EchoConfirmText"), 22, FLinearColor(0.95f, 0.85f, 0.6f));
		CText->SetText(FText::FromString(TEXT("Pending echo not decided. Store or Skip before leaving?")));
		CText->SetAutoWrapText(true);
		CloseConfirmWidget->AddChildToVerticalBox(CText);

		UButton* SkipContinue =
		    WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EchoConfirmSkipContinue"));
		SkipContinue->SetBackgroundColor(FLinearColor(0.5f, 0.2f, 0.2f, 0.9f));
		UTextBlock* SC = CreateText(WidgetTree, TEXT("EchoConfirmSkipLabel"), 20, FLinearColor(1.0f, 1.0f, 1.0f));
		SC->SetText(FText::FromString(TEXT("Skip and continue")));
		SC->SetJustification(ETextJustify::Center);
		SkipContinue->SetContent(SC);
		SkipContinue->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleConfirmSkipContinueClicked);
		CloseConfirmWidget->AddChildToVerticalBox(SkipContinue);

		UButton* ReturnSel = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EchoConfirmReturn"));
		ReturnSel->SetBackgroundColor(FLinearColor(0.3f, 0.4f, 0.5f, 0.9f));
		UTextBlock* RS = CreateText(WidgetTree, TEXT("EchoConfirmReturnLabel"), 20, FLinearColor(1.0f, 1.0f, 1.0f));
		RS->SetText(FText::FromString(TEXT("Return to selection")));
		RS->SetJustification(ETextJustify::Center);
		ReturnSel->SetContent(RS);
		ReturnSel->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleConfirmReturnClicked);
		CloseConfirmWidget->AddChildToVerticalBox(ReturnSel);
	}
}

void UReEchoInventoryShopWidget::BuildLoadoutEntries()
{
	if (!WidgetTree || !ShopPanel)
	{
		return;
	}
	BuildShopLogicHost();
	if (!ShopLogicPanel)
	{
		return;
	}
	VisibleWeaponPartOffers.Reset();
	for (const FReEchoShopOffer& Offer : CurrentPartShopView.Offers)
	{
		if (Offer.Type == EReEchoShopOfferType::WeaponPart)
		{
			VisibleWeaponPartOffers.Add(Offer);
		}
	}
	if (!WeaponPartOfferPanel)
	{
		WeaponPartOfferPanel =
		    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WeaponPartOfferPanel"));
		ShopLogicPanel->AddChildToVerticalBox(WeaponPartOfferPanel);
		UTextBlock* Title =
		    CreateText(WidgetTree, TEXT("WeaponPartOfferTitle"), 20, FLinearColor(0.9f, 0.82f, 0.66f));
		Title->SetText(NSLOCTEXT("ReEcho", "WeaponPartOfferTitle", "武器配件"));
		WeaponPartOfferPanel->AddChildToVerticalBox(Title);
	}
	if (!WeaponLoadoutPanel)
	{
		WeaponLoadoutPanel =
		    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WeaponLoadoutPanel"));
		ShopLogicPanel->AddChildToVerticalBox(WeaponLoadoutPanel);
		UTextBlock* Title = CreateText(WidgetTree, TEXT("WeaponLoadoutTitle"), 20, FLinearColor(0.9f, 0.82f, 0.66f));
		Title->SetText(NSLOCTEXT("ReEcho", "WeaponLoadoutTitle", "装备槽位"));
		WeaponLoadoutPanel->AddChildToVerticalBox(Title);
	}
	if (!WeaponLoadoutText)
	{
		WeaponLoadoutText =
		    CreateText(WidgetTree, TEXT("WeaponLoadoutText"), 18, FLinearColor(0.9f, 0.82f, 0.66f));
		UVerticalBoxSlot* LoadoutTextSlot = WeaponLoadoutPanel->AddChildToVerticalBox(WeaponLoadoutText);
		LoadoutTextSlot->SetPadding(FMargin(18.0f, 16.0f, 18.0f, 8.0f));
	}
	if (!SaveLoadoutButton)
	{
		SaveLoadoutButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SaveLoadoutButton"));
		SaveLoadoutButton->SetBackgroundColor(FLinearColor(0.2f, 0.55f, 0.25f, 0.9f));
		UTextBlock* SaveText = CreateText(WidgetTree, TEXT("SaveLoadoutText"), 22, FLinearColor::White);
		SaveText->SetText(NSLOCTEXT("ReEcho", "SaveWeaponLoadout", "保存配置"));
		SaveText->SetJustification(ETextJustify::Center);
		SaveLoadoutButton->SetContent(SaveText);
		UVerticalBoxSlot* SaveSlot = WeaponLoadoutPanel->AddChildToVerticalBox(SaveLoadoutButton);
		SaveSlot->SetPadding(FMargin(18.0f, 8.0f, 18.0f, 12.0f));
	}
	SaveLoadoutButton->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleSaveLoadoutClicked);

	if (WeaponPartOfferButtons.Num() != VisibleWeaponPartOffers.Num())
	{
		for (UReEchoIndexedButton* Button : WeaponPartOfferButtons)
		{
			if (Button)
			{
				Button->RemoveFromParent();
			}
		}
		WeaponPartOfferButtons.Reset();
		WeaponPartOfferTexts.Reset();
		for (int32 Index = 0; Index < VisibleWeaponPartOffers.Num(); ++Index)
		{
			UReEchoIndexedButton* Button = WidgetTree->ConstructWidget<UReEchoIndexedButton>(
			    UReEchoIndexedButton::StaticClass(), *FString::Printf(TEXT("WeaponPartOffer%d"), Index));
			Button->SetEntryIndex(Index);
			Button->OnIndexedClicked.AddUniqueDynamic(this,
			                                          &UReEchoInventoryShopWidget::HandleWeaponPartOfferClicked);
			UTextBlock* Text = CreateText(WidgetTree,
			                              *FString::Printf(TEXT("WeaponPartOfferText%d"), Index),
			                              17,
			                              FLinearColor(0.9f, 0.82f, 0.66f));
			Text->SetJustification(ETextJustify::Center);
			Button->SetContent(Text);
			WeaponPartOfferPanel->AddChildToVerticalBox(Button);
			WeaponPartOfferButtons.Add(Button);
			WeaponPartOfferTexts.Add(Text);
		}
	}
}

void UReEchoInventoryShopWidget::Refresh()
{
	if (!BackgroundImage || !InventoryPanel || !ShopPanel || !InventoryText || !CurrencyText)
	{
		return;
	}

	BuildOfferEntries();
	BuildLoadoutEntries();
	if (OfferButtons.Num() != VisibleRunItemOffers.Num() || OfferTexts.Num() != VisibleRunItemOffers.Num())
	{
		return;
	}
	for (int32 OfferIndex = 0; OfferIndex < VisibleRunItemOffers.Num(); ++OfferIndex)
	{
		if (!OfferButtons[OfferIndex] || !OfferTexts[OfferIndex])
		{
			return;
		}
	}

	BackgroundImage->SetBrushFromTexture(bShowingShop ? ShopBackgroundTexture : InventoryBackgroundTexture, true);
	InventoryPanel->SetVisibility(bShowingShop ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	ShopPanel->SetVisibility(bShowingShop ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (RunItemOfferPanel)
	{
		RunItemOfferPanel->SetVisibility(bShowingShop ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (ShopControlPanel)
	{
		ShopControlPanel->SetVisibility(bShowingShop ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	FString InventoryDescription;
	for (const FReEchoShopOffer& Offer : GetReEchoShopCatalog())
	{
		if (CurrentOwnedItems.Contains(Offer.ItemId))
		{
			InventoryDescription +=
			    FString::Printf(TEXT("◆ %s\n   %s\n\n"), *Offer.DisplayName.ToString(), *Offer.EffectText.ToString());
		}
	}
	if (InventoryDescription.IsEmpty())
	{
		InventoryDescription = TEXT("尚未购买道具。\n前往商城消耗时间碎片获取本轮强化。");
	}
	InventoryText->SetText(FText::FromString(InventoryDescription));

	CurrencyText->SetText(
	    FText::Format(NSLOCTEXT("ReEcho", "ShopCurrency", "时间碎片  {0}"), FText::AsNumber(CurrentTimeShards)));
	for (int32 OfferIndex = 0; OfferIndex < VisibleRunItemOffers.Num(); ++OfferIndex)
	{
		const int32 CatalogIndex =
		    (OfferIndex + CurrentShopRefreshSequence) % VisibleRunItemOffers.Num();
		const FReEchoShopOffer& Offer = VisibleRunItemOffers[CatalogIndex];
		const int32 EffectivePrice = GetEffectiveShopPrice(Offer.Price, CurrentShopDiscount);
		const bool bOwned = CurrentOwnedItems.Contains(Offer.ItemId);
		const bool bAffordable = CurrentTimeShards >= EffectivePrice;
		OfferButtons[OfferIndex]->SetIsEnabled(!bOwned && bAffordable);
		OfferTexts[OfferIndex]->SetText(FText::Format(
		    NSLOCTEXT("ReEcho", "ShopOfferFormat", "{0}\n{1}\n{2}"),
		    Offer.DisplayName,
		    Offer.EffectText,
		    bOwned ? NSLOCTEXT("ReEcho", "ShopOwned", "已拥有")
		           : FText::Format(NSLOCTEXT("ReEcho", "ShopPrice", "{0} 碎片"), FText::AsNumber(EffectivePrice))));
	}
	if (ShopRefreshButton && ShopRefreshText)
	{
		const bool bCanUseFreeRefresh = bCurrentShopRefreshAllowed && CurrentFreeShopRefreshes > 0;
		ShopRefreshButton->SetIsEnabled(bCanUseFreeRefresh);
		ShopRefreshText->SetText(!bCurrentShopRefreshAllowed
		                             ? NSLOCTEXT("ReEcho", "ShopRefreshDisabled", "刷新已被永久代价禁用")
		                             : FText::Format(NSLOCTEXT("ReEcho", "ShopFreeRefresh", "免费刷新（剩余 {0}）"),
		                                             FText::AsNumber(CurrentFreeShopRefreshes)));
	}
	if (ShopRuleText)
	{
		ShopRuleText->SetText(FText::Format(NSLOCTEXT("ReEcho", "ShopCardGroupRule", "商店折扣 {0}%　额外卡牌组：{1}"),
		                                    FText::AsNumber(FMath::RoundToInt(CurrentShopDiscount * 100.0f)),
		                                    bCurrentExtraCardPurchaseAllowed
		                                        ? NSLOCTEXT("ReEcho", "ShopCardGroupAllowed", "可购买")
		                                        : NSLOCTEXT("ReEcho", "ShopCardGroupDisabled", "已被永久代价禁用")));
	}
	if (WeaponLoadoutPanel && WeaponLoadoutText && SaveLoadoutButton)
	{
		const bool bShowWeaponBlocks = bShowingShop && !CurrentPartShopView.WeaponId.IsNone();
		WeaponLoadoutPanel->SetVisibility(bShowWeaponBlocks ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		WeaponPartOfferPanel->SetVisibility(bShowWeaponBlocks && !VisibleWeaponPartOffers.IsEmpty()
		                                        ? ESlateVisibility::Visible
		                                        : ESlateVisibility::Collapsed);
		FString LoadoutDescription =
		    FString::Printf(TEXT("装配室 · %s\n"), *CurrentPartShopView.WeaponDisplayName.ToString());
		for (const FReEchoWeaponSlotShopView& SlotView : CurrentPartShopView.Slots)
		{
			TArray<FString> Names;
			for (const FName DraftId : DraftPartIds)
			{
				const FReEchoShopOffer* DraftPart = CurrentPartShopView.OwnedParts.FindByPredicate(
				    [&](const FReEchoShopOffer& Owned)
				    {
					    return Owned.ContentId == DraftId && Owned.SlotTypeId == SlotView.SlotTypeId;
				    });
				if (DraftPart)
				{
					Names.Add(DraftPart->DisplayName.ToString());
				}
			}
			LoadoutDescription += FString::Printf(TEXT("%s%s [%d/%d]：%s\n"),
			                                      SlotView.bRequired ? TEXT("必需 ") : TEXT(""),
			                                      *SlotView.DisplayName.ToString(),
			                                      Names.Num(),
			                                      SlotView.Capacity,
			                                      Names.IsEmpty() ? TEXT("未装备") : *FString::Join(Names, TEXT("、")));
		}
		LoadoutDescription += TEXT("\n在“武器配件”区购买配件；点击已拥有配件可调整槽位草稿。");
		WeaponLoadoutText->SetText(FText::FromString(LoadoutDescription));
		for (int32 Index = 0; Index < VisibleWeaponPartOffers.Num(); ++Index)
		{
			const FReEchoShopOffer& PartOffer = VisibleWeaponPartOffers[Index];
			const bool bOwned = CurrentPartShopView.OwnedParts.ContainsByPredicate(
			    [&](const FReEchoShopOffer& Owned)
			    {
				    return Owned.ContentId == PartOffer.ContentId;
			    });
			const bool bDrafted = DraftPartIds.Contains(PartOffer.ContentId);
			const int32 EffectivePrice = GetEffectiveShopPrice(PartOffer.Price, CurrentShopDiscount);
			WeaponPartOfferButtons[Index]->SetIsEnabled(bOwned || CurrentTimeShards >= EffectivePrice);
			WeaponPartOfferButtons[Index]->SetBackgroundColor(
			    bDrafted ? FLinearColor(0.2f, 0.55f, 0.25f, 0.95f) : FLinearColor(0.15f, 0.11f, 0.07f, 0.88f));
			WeaponPartOfferTexts[Index]->SetText(FText::Format(
			    NSLOCTEXT("ReEcho", "WeaponPartOfferFormat", "{0} · {1}{2}"),
			    PartOffer.DisplayName,
			    FText::FromName(PartOffer.SlotTypeId),
			    bDrafted
			        ? NSLOCTEXT("ReEcho", "WeaponPartDrafted", "（草稿已装备）")
			        : bOwned ? NSLOCTEXT("ReEcho", "WeaponPartOwned", "（已拥有，点击装配）")
			                 : FText::Format(NSLOCTEXT("ReEcho", "WeaponPartPrice", "（{0} 碎片）"),
			                                 FText::AsNumber(EffectivePrice))));
		}
	}
}

void UReEchoInventoryShopWidget::RequestPurchase(const int32 OfferIndex)
{
	if (VisibleRunItemOffers.IsValidIndex(OfferIndex))
	{
		const int32 CatalogIndex = (OfferIndex + CurrentShopRefreshSequence) % VisibleRunItemOffers.Num();
		OnPurchaseRequested.Broadcast(VisibleRunItemOffers[CatalogIndex].ItemId);
	}
}

void UReEchoInventoryShopWidget::HandleCloseClicked()
{
	RequestClose();
}

void UReEchoInventoryShopWidget::HandleOfferClicked(const int32 OfferIndex)
{
	RequestPurchase(OfferIndex);
}

void UReEchoInventoryShopWidget::HandleRefreshClicked()
{
	OnRefreshRequested.Broadcast();
}

void UReEchoInventoryShopWidget::ToggleDraftPart(const int32 OwnedPartIndex)
{
	if (!CurrentPartShopView.OwnedParts.IsValidIndex(OwnedPartIndex))
	{
		return;
	}
	const FReEchoShopOffer& Part = CurrentPartShopView.OwnedParts[OwnedPartIndex];
	const FReEchoWeaponSlotShopView* SlotView = CurrentPartShopView.Slots.FindByPredicate(
	    [&](const FReEchoWeaponSlotShopView& Candidate)
	    {
		    return Candidate.SlotTypeId == Part.SlotTypeId;
	    });
	if (!SlotView || SlotView->Capacity <= 0)
	{
		return;
	}
	if (DraftPartIds.RemoveSingle(Part.ContentId) > 0)
	{
		const int32 RemainingInSlot =
		    CountDraftPartsForSlot(DraftPartIds, CurrentPartShopView.OwnedParts, Part.SlotTypeId);
		if (SlotView->bRequired && RemainingInSlot == 0)
		{
			DraftPartIds.Add(Part.ContentId);
		}
		Refresh();
		return;
	}

	while (CountDraftPartsForSlot(DraftPartIds, CurrentPartShopView.OwnedParts, Part.SlotTypeId) >= SlotView->Capacity)
	{
		const int32 ReplaceIndex = DraftPartIds.IndexOfByPredicate(
		    [&](const FName DraftId)
		    {
			    const FReEchoShopOffer* DraftPart = CurrentPartShopView.OwnedParts.FindByPredicate(
			        [&](const FReEchoShopOffer& Owned)
			        {
				        return Owned.ContentId == DraftId;
			        });
			    return DraftPart && DraftPart->SlotTypeId == Part.SlotTypeId;
		    });
		if (ReplaceIndex == INDEX_NONE)
		{
			break;
		}
		DraftPartIds.RemoveAt(ReplaceIndex);
	}
	DraftPartIds.Add(Part.ContentId);
	Refresh();
}

void UReEchoInventoryShopWidget::HandleWeaponPartOfferClicked(const int32 PartOfferIndex)
{
	if (!VisibleWeaponPartOffers.IsValidIndex(PartOfferIndex))
	{
		return;
	}
	const FReEchoShopOffer& PartOffer = VisibleWeaponPartOffers[PartOfferIndex];
	const int32 OwnedPartIndex = CurrentPartShopView.OwnedParts.IndexOfByPredicate(
	    [&](const FReEchoShopOffer& Owned)
	    {
		    return Owned.ContentId == PartOffer.ContentId;
	    });
	if (OwnedPartIndex == INDEX_NONE)
	{
		OnPurchaseRequested.Broadcast(PartOffer.ItemId);
		return;
	}
	ToggleDraftPart(OwnedPartIndex);
}

void UReEchoInventoryShopWidget::HandleSaveLoadoutClicked()
{
	OnWeaponLoadoutSaveRequested.Broadcast(DraftPartIds);
}

// ============================ Inline echo management (origin/main) ============================

void UReEchoInventoryShopWidget::RefreshEchoState(UReEchoRunSubsystem* RunSubsystem)
{
	if (!RunSubsystem)
	{
		return;
	}
	CachedRunSubsystem = RunSubsystem;
	EchoSummary = RunSubsystem->GetEchoStorageSummary();
	EchoSelection = EchoSummary.SelectedReplayIds;
	if (!EchoSummary.bHasPendingRecording && EchoPendingDecision == EReEchoShopEchoPendingDecision::Replacing)
	{
		EchoPendingDecision = EReEchoShopEchoPendingDecision::Undecided;
	}
	BuildEchoPanel();
}

void UReEchoInventoryShopWidget::RequestStoreEcho(UReEchoRunSubsystem* RunSubsystem)
{
	if (!RunSubsystem || !EchoSummary.bHasPendingRecording)
	{
		return;
	}
	if (EchoSummary.StoredEchoes.Num() >= FMath::Max(0, EchoSummary.StorageCapacity))
	{
		EchoPendingDecision = EReEchoShopEchoPendingDecision::Replacing;
		BuildEchoPanel();
		return;
	}
	RunSubsystem->StorePendingRecording();
	EchoPendingDecision = EReEchoShopEchoPendingDecision::Stored;
	RefreshEchoState(RunSubsystem);
}

void UReEchoInventoryShopWidget::RequestSkipEcho(UReEchoRunSubsystem* RunSubsystem)
{
	if (!RunSubsystem)
	{
		return;
	}
	RunSubsystem->SkipPendingRecordingStorage();
	EchoPendingDecision = EReEchoShopEchoPendingDecision::Skipped;
	RefreshEchoState(RunSubsystem);
}

void UReEchoInventoryShopWidget::RequestReplaceEcho(UReEchoRunSubsystem* RunSubsystem, FGuid TargetId)
{
	if (!RunSubsystem || !TargetId.IsValid())
	{
		return;
	}
	RunSubsystem->StorePendingRecordingReplacing(TargetId);
	EchoPendingDecision = EReEchoShopEchoPendingDecision::Stored;
	RefreshEchoState(RunSubsystem);
}

void UReEchoInventoryShopWidget::RequestCancelReplaceEcho()
{
	EchoPendingDecision = EReEchoShopEchoPendingDecision::Undecided;
	BuildEchoPanel();
}

void UReEchoInventoryShopWidget::ToggleReplaySelection(UReEchoRunSubsystem* RunSubsystem, FGuid Id)
{
	if (!RunSubsystem || !Id.IsValid())
	{
		return;
	}
	const int32 Limit = EchoSummary.SpecificReplayLimit;
	if (Limit <= 0)
	{
		return;
	}
	TArray<FGuid> Next = EchoSelection;
	const int32 Existing = Next.IndexOfByKey(Id);
	if (Existing != INDEX_NONE)
	{
		Next.RemoveAt(Existing);
	}
	else if (Limit == 1)
	{
		Next.Reset();
		Next.Add(Id);
	}
	else if (Next.Num() < Limit)
	{
		Next.Add(Id);
	}
	else
	{
		return;
	}
	if (RunSubsystem->SetSelectedReplayIds(Next) == EReEchoEchoStorageResult::Success)
	{
		EchoSelection = Next;
	}
	RefreshEchoState(RunSubsystem);
}

void UReEchoInventoryShopWidget::RequestClose()
{
	if (Mode == EReEchoInventoryShopMode::PostTraitIntermission && EchoSummary.bHasPendingRecording &&
	    CloseConfirmWidget)
	{
		CloseConfirmWidget->SetVisibility(ESlateVisibility::Visible);
		return;
	}
	OnClosed.Broadcast();
}

void UReEchoInventoryShopWidget::BuildEchoPanel()
{
	if (!EchoPanel)
	{
		return;
	}
	if (Mode != EReEchoInventoryShopMode::PostTraitIntermission)
	{
		EchoPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	EchoPanel->SetVisibility(ESlateVisibility::Visible);

	const FReEchoEchoStorageSummary& S = EchoSummary;
	EchoCapacityText->SetText(
	    FText::FromString(FString::Printf(TEXT("回响存储  %d / %d"), S.StoredEchoes.Num(), S.StorageCapacity)));

	if (S.SpecificReplayLimit <= 0)
	{
		EchoReplayModeText->SetText(
		    FText::FromString(TEXT("下一场自动回放上一场（SpecificReplayLimit = 0，不提供指定回放选择）。")));
	}
	else
	{
		EchoReplayModeText->SetText(
		    FText::FromString(FString::Printf(TEXT("可选下场回放：上限 %d（SpecificReplayLimit = %d）。"),
		                                      S.SpecificReplayLimit,
		                                      S.SpecificReplayLimit)));
	}

	const bool bHasPending = S.bHasPendingRecording;
	const bool bReplacing = bHasPending && EchoPendingDecision == EReEchoShopEchoPendingDecision::Replacing;

	EchoPendingInfoText->SetVisibility(bHasPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	EchoStoreButton->SetVisibility(bHasPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	EchoSkipButton->SetVisibility(bHasPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	EchoReplaceInstructionText->SetVisibility(bReplacing ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	EchoCancelReplaceButton->SetVisibility(bReplacing ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (bHasPending)
	{
		EchoPendingInfoText->SetText(FText::FromString(FString::Printf(TEXT("本场回响：遭遇 #%d | 角色 %s | 武器 %s"),
		                                                               S.PendingRecording.EncounterIndex,
		                                                               *S.PendingRecording.CharacterId.ToString(),
		                                                               *S.PendingRecording.WeaponId.ToString())));
	}

	for (int32 i = 0; i < EchoReplaceButtons.Num(); ++i)
	{
		EchoReplaceButtons[i]->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (bReplacing)
	{
		EchoReplaceInstructionText->SetText(FText::FromString(TEXT("容量已满：选择要替换的已存储回响。")));
		for (int32 i = 0; i < S.StoredEchoes.Num() && i < EchoSlotGuids.Num(); ++i)
		{
			EchoReplaceButtons[i]->SetVisibility(ESlateVisibility::Visible);
		}
	}

	EchoSlotGuids.Reset();
	for (int32 SlotIndex = 0; SlotIndex < EchoSlotTexts.Num(); ++SlotIndex)
	{
		UTextBlock* SlotText = EchoSlotTexts[SlotIndex];
		UButton* SelectBtn = EchoSelectButtons[SlotIndex];
		UTextBlock* SelectLabel = EchoSelectLabels[SlotIndex];
		if (!S.StoredEchoes.IsValidIndex(SlotIndex))
		{
			SlotText->SetVisibility(ESlateVisibility::Collapsed);
			SelectBtn->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		const FReEchoStoredEchoSummary& Stored = S.StoredEchoes[SlotIndex];
		const bool bSelected = EchoSelection.Contains(Stored.RecordingId);
		SlotText->SetVisibility(ESlateVisibility::Visible);
		SlotText->SetText(FText::FromString(FString::Printf(TEXT("槽 %d：遭遇 #%d | 角色 %s | 武器 %s%s"),
		                                                    SlotIndex,
		                                                    Stored.EncounterIndex,
		                                                    *Stored.CharacterId.ToString(),
		                                                    *Stored.WeaponId.ToString(),
		                                                    bSelected ? TEXT("  [已选为下场回放]") : TEXT(""))));
		EchoSlotGuids.Add(Stored.RecordingId);

		const bool bCanSelect = S.SpecificReplayLimit > 0;
		SelectBtn->SetVisibility(bCanSelect ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		SelectBtn->SetIsEnabled(bCanSelect);
		SelectLabel->SetText(FText::FromString(bSelected ? TEXT("取消回放选择") : TEXT("选为下场回放")));
	}

	if (S.SpecificReplayLimit <= 0)
	{
		EchoSelectionText->SetText(FText::FromString(TEXT("下一场将自动回放上一场。")));
	}
	else
	{
		FString Sel;
		for (const FGuid& Id : EchoSelection)
		{
			const int32 Idx = S.StoredEchoes.IndexOfByPredicate(
			    [&Id](const FReEchoStoredEchoSummary& E)
			    {
				    return E.RecordingId == Id;
			    });
			if (Idx != INDEX_NONE)
			{
				Sel += FString::Printf(TEXT("遭遇 #%d  "), S.StoredEchoes[Idx].EncounterIndex);
			}
		}
		EchoSelectionText->SetText(FText::FromString(
		    FString::Printf(TEXT("已选下场回放（%d / %d）：%s"), EchoSelection.Num(), S.SpecificReplayLimit, *Sel)));
	}
}

void UReEchoInventoryShopWidget::HandleStoreClicked()
{
	OnEchoStoreRequested.Broadcast();
}

void UReEchoInventoryShopWidget::HandleSkipClicked()
{
	OnEchoSkipRequested.Broadcast();
}

void UReEchoInventoryShopWidget::HandleCancelReplaceClicked()
{
	RequestCancelReplaceEcho();
}

void UReEchoInventoryShopWidget::HandleReplaceSlot0Clicked()
{
	HandleReplaceSlotClicked(0);
}

void UReEchoInventoryShopWidget::HandleReplaceSlot1Clicked()
{
	HandleReplaceSlotClicked(1);
}

void UReEchoInventoryShopWidget::HandleReplaceSlot2Clicked()
{
	HandleReplaceSlotClicked(2);
}

void UReEchoInventoryShopWidget::HandleSelectSlot0Clicked()
{
	HandleSelectSlotClicked(0);
}

void UReEchoInventoryShopWidget::HandleSelectSlot1Clicked()
{
	HandleSelectSlotClicked(1);
}

void UReEchoInventoryShopWidget::HandleSelectSlot2Clicked()
{
	HandleSelectSlotClicked(2);
}

void UReEchoInventoryShopWidget::HandleReplaceSlotClicked(int32 SlotIndex)
{
	if (EchoSlotGuids.IsValidIndex(SlotIndex))
	{
		OnEchoReplaceRequested.Broadcast(EchoSlotGuids[SlotIndex]);
	}
}

void UReEchoInventoryShopWidget::HandleSelectSlotClicked(int32 SlotIndex)
{
	if (!EchoSlotGuids.IsValidIndex(SlotIndex))
	{
		return;
	}
	const int32 Limit = EchoSummary.SpecificReplayLimit;
	if (Limit <= 0)
	{
		return;
	}
	TArray<FGuid> Next = EchoSelection;
	const FGuid Id = EchoSlotGuids[SlotIndex];
	const int32 Existing = Next.IndexOfByKey(Id);
	if (Existing != INDEX_NONE)
	{
		Next.RemoveAt(Existing);
	}
	else if (Limit == 1)
	{
		Next.Reset();
		Next.Add(Id);
	}
	else if (Next.Num() < Limit)
	{
		Next.Add(Id);
	}
	else
	{
		return;
	}
	OnEchoSelectionRequested.Broadcast(Next);
}

void UReEchoInventoryShopWidget::HandleConfirmSkipContinueClicked()
{
	if (CloseConfirmWidget)
	{
		CloseConfirmWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (EchoSummary.bHasPendingRecording)
	{
		OnEchoSkipAndCloseRequested.Broadcast();
		return;
	}
	OnClosed.Broadcast();
}

void UReEchoInventoryShopWidget::HandleConfirmReturnClicked()
{
	if (CloseConfirmWidget)
	{
		CloseConfirmWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

// ============================ Public echo API (delegates to GameMode) ============================

void UReEchoInventoryShopWidget::ShowPostTraitIntermission(int32 TimeShards,
                                                           const TArray<FName>& OwnedItems,
                                                           const FReEchoEchoStorageSummary& InEchoSummary,
                                                           float ShopDiscount,
                                                           int32 FreeRefreshes,
                                                           bool bRefreshAllowed,
                                                           bool bExtraCardPurchaseAllowed,
                                                           int32 RefreshSequence)
{
	ShowShop(TimeShards,
	         OwnedItems,
	         ShopDiscount,
	         FreeRefreshes,
	         bRefreshAllowed,
	         bExtraCardPurchaseAllowed,
	         RefreshSequence);
	Mode = EReEchoInventoryShopMode::PostTraitIntermission;
	SetEchoSummary(InEchoSummary);
}

void UReEchoInventoryShopWidget::SetEchoSummary(const FReEchoEchoStorageSummary& InEchoSummary)
{
	EchoSummary = InEchoSummary;
	EchoSelection = EchoSummary.SelectedReplayIds;
	BuildEchoPanel();
}

void UReEchoInventoryShopWidget::ShowEchoStatus(const FText& Status)
{
	if (EchoReplayModeText)
	{
		EchoReplayModeText->SetText(Status);
	}
}

void UReEchoInventoryShopWidget::EnterEchoReplacementMode()
{
	if (EchoSummary.bHasPendingRecording &&
	    EchoSummary.StoredEchoes.Num() >= FMath::Max(0, EchoSummary.StorageCapacity))
	{
		EchoPendingDecision = EReEchoShopEchoPendingDecision::Replacing;
		BuildEchoPanel();
	}
}

void UReEchoInventoryShopWidget::CompletePostTraitClose()
{
	OnClosed.Broadcast();
}

void UReEchoInventoryShopWidget::HandleEchoStoreRequested()
{
	OnEchoStoreRequested.Broadcast();
}

void UReEchoInventoryShopWidget::HandleEchoSkipRequested()
{
	OnEchoSkipRequested.Broadcast();
}

void UReEchoInventoryShopWidget::HandleEchoReplaceRequested(FGuid RecordingId)
{
	OnEchoReplaceRequested.Broadcast(RecordingId);
}

void UReEchoInventoryShopWidget::HandleEchoSelectionRequested(const TArray<FGuid>& RecordingIds)
{
	OnEchoSelectionRequested.Broadcast(RecordingIds);
}

void UReEchoInventoryShopWidget::HandleEchoSkipAndCloseRequested()
{
	OnEchoSkipAndCloseRequested.Broadcast();
}
