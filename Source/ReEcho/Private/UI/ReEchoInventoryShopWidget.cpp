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
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UObject/ConstructorHelpers.h"

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
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UReEchoInventoryShopWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidgetTree();
	Refresh();
}

void UReEchoInventoryShopWidget::ShowInventory(const int32 TimeShards, const TArray<FName>& OwnedItems)
{
	bShowingShop = false;
	CurrentTimeShards = TimeShards;
	CurrentOwnedItems = OwnedItems;
	Refresh();
}

void UReEchoInventoryShopWidget::ShowShop(const int32 TimeShards, const TArray<FName>& OwnedItems)
{
	bShowingShop = true;
	CurrentTimeShards = TimeShards;
	CurrentOwnedItems = OwnedItems;
	Refresh();
}

void UReEchoInventoryShopWidget::BuildWidgetTree()
{
	if (BackgroundImage || !WidgetTree)
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

	UButton* CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
	CloseButton->SetBackgroundColor(FLinearColor(0.12f, 0.09f, 0.06f, 0.88f));
	UCanvasPanelSlot* CloseSlot = RootCanvas->AddChildToCanvas(CloseButton);
	CloseSlot->SetAnchors(FAnchors(0.03f, 0.88f));
	CloseSlot->SetPosition(FVector2D::ZeroVector);
	CloseSlot->SetSize(FVector2D(112.0f, 64.0f));
	UTextBlock* CloseText = CreateText(WidgetTree, TEXT("CloseText"), 26, FLinearColor(0.85f, 0.77f, 0.62f));
	CloseText->SetText(NSLOCTEXT("ReEcho", "InventoryShopClose", "返回"));
	CloseText->SetJustification(ETextJustify::Center);
	CloseButton->SetContent(CloseText);
	CloseButton->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleCloseClicked);

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

	const TArray<FReEchoShopOffer>& Offers = GetReEchoShopCatalog();
	for (int32 OfferIndex = 0; OfferIndex < Offers.Num(); ++OfferIndex)
	{
		UButton* OfferButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),
		                                                            *FString::Printf(TEXT("ShopOffer%d"), OfferIndex));
		OfferButton->SetBackgroundColor(FLinearColor(0.15f, 0.11f, 0.07f, 0.88f));
		UVerticalBoxSlot* OfferSlot = ShopPanel->AddChildToVerticalBox(OfferButton);
		OfferSlot->SetPadding(FMargin(8.0f, 6.0f));

		UTextBlock* OfferText = CreateText(
		    WidgetTree, *FString::Printf(TEXT("ShopOfferText%d"), OfferIndex), 20, FLinearColor(0.9f, 0.82f, 0.66f));
		OfferText->SetJustification(ETextJustify::Center);
		OfferButton->SetContent(OfferText);
		OfferButtons.Add(OfferButton);
		OfferTexts.Add(OfferText);
	}

	OfferButtons[0]->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleFirstOfferClicked);
	OfferButtons[1]->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleSecondOfferClicked);
	OfferButtons[2]->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleThirdOfferClicked);
	OfferButtons[3]->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleFourthOfferClicked);

	// ---- Plan31 echo management panel ----
	// Built once (BuildWidgetTree is guarded). Visibility and content are driven by BuildEchoPanel().
	EchoPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EchoPanel"));
	UCanvasPanelSlot* EchoSlotCanvas = RootCanvas->AddChildToCanvas(EchoPanel);
	EchoSlotCanvas->SetAnchors(FAnchors(0.04f, 0.16f, 0.50f, 0.86f));
	EchoSlotCanvas->SetOffsets(FMargin(0.0f));

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

	EchoReplaceInstructionText = CreateText(WidgetTree, TEXT("EchoReplaceInstruction"), 20, FLinearColor(0.95f, 0.8f, 0.5f));
	EchoReplaceInstructionText->SetAutoWrapText(true);
	EchoPanel->AddChildToVerticalBox(EchoReplaceInstructionText);

	EchoCancelReplaceButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EchoCancelReplaceButton"));
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
		EchoSlotTexts[SlotIndex] = CreateText(WidgetTree, *FString::Printf(TEXT("EchoSlotText%d"), SlotIndex), 18, FLinearColor(0.85f, 0.78f, 0.62f));
		EchoSlotTexts[SlotIndex]->SetAutoWrapText(true);
		EchoPanel->AddChildToVerticalBox(EchoSlotTexts[SlotIndex]);

		EchoReplaceButtons[SlotIndex] = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *FString::Printf(TEXT("EchoReplace%d"), SlotIndex));
		EchoReplaceButtons[SlotIndex]->SetBackgroundColor(FLinearColor(0.4f, 0.3f, 0.15f, 0.9f));
		UTextBlock* RLabel = CreateText(WidgetTree, *FString::Printf(TEXT("EchoReplaceLabel%d"), SlotIndex), 18, FLinearColor(1.0f, 1.0f, 1.0f));
		RLabel->SetText(FText::FromString(TEXT("Replace this echo")));
		RLabel->SetJustification(ETextJustify::Center);
		EchoReplaceButtons[SlotIndex]->SetContent(RLabel);
		if (SlotIndex == 0)
			EchoReplaceButtons[SlotIndex]->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleReplaceSlot0Clicked);
		else if (SlotIndex == 1)
			EchoReplaceButtons[SlotIndex]->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleReplaceSlot1Clicked);
		else
			EchoReplaceButtons[SlotIndex]->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleReplaceSlot2Clicked);
		EchoPanel->AddChildToVerticalBox(EchoReplaceButtons[SlotIndex]);

		EchoSelectButtons[SlotIndex] = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *FString::Printf(TEXT("EchoSelect%d"), SlotIndex));
		EchoSelectButtons[SlotIndex]->SetBackgroundColor(FLinearColor(0.2f, 0.35f, 0.5f, 0.9f));
		EchoSelectLabels[SlotIndex] = CreateText(WidgetTree, *FString::Printf(TEXT("EchoSelectLabel%d"), SlotIndex), 18, FLinearColor(1.0f, 1.0f, 1.0f));
		EchoSelectLabels[SlotIndex]->SetText(FText::FromString(TEXT("Select for replay")));
		EchoSelectLabels[SlotIndex]->SetJustification(ETextJustify::Center);
		EchoSelectButtons[SlotIndex]->SetContent(EchoSelectLabels[SlotIndex]);
		if (SlotIndex == 0)
			EchoSelectButtons[SlotIndex]->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleSelectSlot0Clicked);
		else if (SlotIndex == 1)
			EchoSelectButtons[SlotIndex]->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleSelectSlot1Clicked);
		else
			EchoSelectButtons[SlotIndex]->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleSelectSlot2Clicked);
		EchoPanel->AddChildToVerticalBox(EchoSelectButtons[SlotIndex]);
	}

	EchoSelectionText = CreateText(WidgetTree, TEXT("EchoSelection"), 20, FLinearColor(0.8f, 0.9f, 0.7f));
	EchoSelectionText->SetAutoWrapText(true);
	EchoPanel->AddChildToVerticalBox(EchoSelectionText);

	// Close confirmation overlay (hidden until an undecided pending echo blocks closing).
	CloseConfirmWidget = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EchoCloseConfirm"));
	UCanvasPanelSlot* ConfirmSlot = RootCanvas->AddChildToCanvas(CloseConfirmWidget);
	ConfirmSlot->SetAnchors(FAnchors(0.35f, 0.4f, 0.65f, 0.62f));
	ConfirmSlot->SetOffsets(FMargin(0.0f));
	CloseConfirmWidget->SetVisibility(ESlateVisibility::Collapsed);
	{
		UTextBlock* CText = CreateText(WidgetTree, TEXT("EchoConfirmText"), 22, FLinearColor(0.95f, 0.85f, 0.6f));
		CText->SetText(FText::FromString(TEXT("Pending echo not decided. Store or Skip before leaving?")));
		CText->SetAutoWrapText(true);
		CloseConfirmWidget->AddChildToVerticalBox(CText);

		UButton* SkipContinue = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EchoConfirmSkipContinue"));
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

void UReEchoInventoryShopWidget::Refresh()
{
	if (!BackgroundImage || !InventoryPanel || !ShopPanel)
	{
		return;
	}

	BackgroundImage->SetBrushFromTexture(bShowingShop ? ShopBackgroundTexture : InventoryBackgroundTexture, true);
	InventoryPanel->SetVisibility(bShowingShop ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	ShopPanel->SetVisibility(bShowingShop ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

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
	const TArray<FReEchoShopOffer>& Offers = GetReEchoShopCatalog();
	for (int32 OfferIndex = 0; OfferIndex < Offers.Num(); ++OfferIndex)
	{
		const bool bOwned = CurrentOwnedItems.Contains(Offers[OfferIndex].ItemId);
		const bool bAffordable = CurrentTimeShards >= Offers[OfferIndex].Price;
		OfferButtons[OfferIndex]->SetIsEnabled(!bOwned && bAffordable);
		OfferTexts[OfferIndex]->SetText(FText::Format(NSLOCTEXT("ReEcho", "ShopOfferFormat", "{0}\n{1}\n{2}"),
		                                              Offers[OfferIndex].DisplayName,
		                                              Offers[OfferIndex].EffectText,
		                                              bOwned
		                                                  ? NSLOCTEXT("ReEcho", "ShopOwned", "已拥有")
		                                                  : FText::Format(NSLOCTEXT("ReEcho", "ShopPrice", "{0} 碎片"),
		                                                                  FText::AsNumber(Offers[OfferIndex].Price))));
	}
}

void UReEchoInventoryShopWidget::RequestPurchase(const int32 OfferIndex)
{
	const TArray<FReEchoShopOffer>& Offers = GetReEchoShopCatalog();
	if (Offers.IsValidIndex(OfferIndex))
	{
		OnPurchaseRequested.Broadcast(Offers[OfferIndex].ItemId);
	}
}

void UReEchoInventoryShopWidget::HandleCloseClicked()
{
	RequestClose();
}

void UReEchoInventoryShopWidget::HandleFirstOfferClicked()
{
	RequestPurchase(0);
}

void UReEchoInventoryShopWidget::HandleSecondOfferClicked()
{
	RequestPurchase(1);
}

void UReEchoInventoryShopWidget::HandleThirdOfferClicked()
{
	RequestPurchase(2);
}

void UReEchoInventoryShopWidget::HandleFourthOfferClicked()
{
	RequestPurchase(3);
}

// ============================ Plan31 echo management ============================

void UReEchoInventoryShopWidget::RefreshEchoState(UReEchoRunSubsystem* RunSubsystem)
{
	if (!RunSubsystem)
	{
		return;
	}
	CachedRunSubsystem = RunSubsystem;
	EchoSummary = RunSubsystem->GetEchoStorageSummary();
	// Reconcile the local selection mirror with authoritative backend state.
	EchoSelection = EchoSummary.SelectedReplayIds;
	if (!EchoSummary.bHasPendingRecording && EchoPendingDecision == EReEchoShopEchoPendingDecision::Replacing)
	{
		// No pending recording remains; replacement mode is no longer meaningful.
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
		// Full storage never auto-evicts; require an explicit replacement target.
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
		// Specific replay is unavailable; selection is not offered.
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
		// Single-select: choosing a new echo replaces the previous selection.
		Next.Reset();
		Next.Add(Id);
	}
	else if (Next.Num() < Limit)
	{
		Next.Add(Id);
	}
	else
	{
		// At capacity and not toggling an existing selection: ignore.
		return;
	}
	// Commit through the backend; the widget never writes the selection itself.
	if (RunSubsystem->SetSelectedReplayIds(Next) == EReEchoEchoStorageResult::Success)
	{
		EchoSelection = Next;
	}
	RefreshEchoState(RunSubsystem);
}

void UReEchoInventoryShopWidget::RequestClose()
{
	if (EchoSummary.bHasPendingRecording && CloseConfirmWidget)
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

	const FReEchoEchoStorageSummary& S = EchoSummary;
	EchoCapacityText->SetText(FText::FromString(
	    FString::Printf(TEXT("回响存储  %d / %d"), S.StoredEchoes.Num(), S.StorageCapacity)));

	if (S.SpecificReplayLimit <= 0)
	{
		EchoReplayModeText->SetText(FText::FromString(
		    TEXT("下一场自动回放上一场（SpecificReplayLimit = 0，不提供指定回放选择）。")));
	}
	else
	{
		EchoReplayModeText->SetText(FText::FromString(FString::Printf(
		    TEXT("可选下场回放：上限 %d（SpecificReplayLimit = %d）。"), S.SpecificReplayLimit, S.SpecificReplayLimit)));
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
		EchoPendingInfoText->SetText(FText::FromString(FString::Printf(
		    TEXT("本场回响：遭遇 #%d | 角色 %s | 武器 %s"),
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
		SlotText->SetText(FText::FromString(FString::Printf(
		    TEXT("槽 %d：遭遇 #%d | 角色 %s | 武器 %s%s"),
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
		EchoSelectionText->SetText(FText::FromString(FString::Printf(
		    TEXT("已选下场回放（%d / %d）：%s"), EchoSelection.Num(), S.SpecificReplayLimit, *Sel)));
	}
}

void UReEchoInventoryShopWidget::HandleStoreClicked()
{
	RequestStoreEcho(CachedRunSubsystem);
}

void UReEchoInventoryShopWidget::HandleSkipClicked()
{
	RequestSkipEcho(CachedRunSubsystem);
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
	if (CachedRunSubsystem && EchoSlotGuids.IsValidIndex(SlotIndex))
	{
		RequestReplaceEcho(CachedRunSubsystem, EchoSlotGuids[SlotIndex]);
	}
}

void UReEchoInventoryShopWidget::HandleSelectSlotClicked(int32 SlotIndex)
{
	if (CachedRunSubsystem && EchoSlotGuids.IsValidIndex(SlotIndex))
	{
		ToggleReplaySelection(CachedRunSubsystem, EchoSlotGuids[SlotIndex]);
	}
}

void UReEchoInventoryShopWidget::HandleConfirmSkipContinueClicked()
{
	if (CloseConfirmWidget)
	{
		CloseConfirmWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (EchoSummary.bHasPendingRecording && CachedRunSubsystem)
	{
		RequestSkipEcho(CachedRunSubsystem);
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
