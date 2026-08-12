#include "UI/ReEchoInventoryShopWidget.h"

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

void UReEchoInventoryShopWidget::BuildOfferEntries()
{
	if (!WidgetTree || !OfferContainer || !OfferButtons.IsEmpty())
	{
		return;
	}

	const TArray<FReEchoShopOffer>& Offers = GetReEchoShopCatalog();
	for (int32 OfferIndex = 0; OfferIndex < Offers.Num(); ++OfferIndex)
	{
		UReEchoIndexedButton* OfferButton = WidgetTree->ConstructWidget<UReEchoIndexedButton>(
		    UReEchoIndexedButton::StaticClass(), *FString::Printf(TEXT("ShopOffer%d"), OfferIndex));
		OfferButton->SetEntryIndex(OfferIndex);
		OfferButton->SetBackgroundColor(FLinearColor(0.15f, 0.11f, 0.07f, 0.88f));
		UVerticalBoxSlot* OfferSlot = OfferContainer->AddChildToVerticalBox(OfferButton);
		OfferSlot->SetPadding(FMargin(8.0f, 6.0f));

		UTextBlock* OfferText = CreateText(
		    WidgetTree, *FString::Printf(TEXT("ShopOfferText%d"), OfferIndex), 20, FLinearColor(0.9f, 0.82f, 0.66f));
		OfferText->SetJustification(ETextJustify::Center);
		OfferButton->SetContent(OfferText);
		OfferButtons.Add(OfferButton);
		OfferTexts.Add(OfferText);
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
	OnClosed.Broadcast();
}

void UReEchoInventoryShopWidget::HandleOfferClicked(const int32 OfferIndex)
{
	RequestPurchase(OfferIndex);
}
