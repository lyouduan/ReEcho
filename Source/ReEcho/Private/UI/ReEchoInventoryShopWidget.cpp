#include "UI/ReEchoInventoryShopWidget.h"

#include "ReEcho.h"
#include "Core/ReEchoTypes.h"
#include "Run/ReEchoRunSubsystem.h"
#include "Run/ReEchoShopCatalog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/ReEchoIndexedButton.h"
#include "UI/Framework/ReEchoUIInteractionAudit.h"
#include "UI/Framework/ReEchoButtonVisualFeedback.h"
#include "Data/ReEchoCsvDataRegistry.h"

namespace
{
constexpr float ShopDesignWidth = 1920.0f;
constexpr float ShopDesignHeight = 1080.0f;
constexpr int32 BackpackPopupLayerZOrder = 100;
const FVector2D WeaponBackpackPopupSize(340.0f, 430.0f);
const FVector2D RuneBackpackPopupSize(320.0f, 390.0f);

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

float GetAttributeRawValue(const FName& Id, const FReEchoStatBlock& Stats)
{
	if (Id == FName(TEXT("S_HP")))
	{
		return Stats.HpMax;
	}
	if (Id == FName(TEXT("S_P_Attack_Power")))
	{
		return Stats.PhysicalAttack;
	}
	if (Id == FName(TEXT("S_E_Attack_Power")))
	{
		return Stats.ElementalAttack;
	}
	if (Id == FName(TEXT("S_Movement_Speed")))
	{
		return Stats.MovementSpeed;
	}
	if (Id == FName(TEXT("S_Critical_Hit_Rate")))
	{
		return Stats.CriticalRate;
	}
	if (Id == FName(TEXT("S_Critical_Hit_Effect")))
	{
		return Stats.CriticalEffect;
	}
	if (Id == FName(TEXT("S_Echo_Efficiency")))
	{
		return Stats.EchoEfficiency;
	}
	if (Id == FName(TEXT("S_Elemental_Reaction_Efficiency")))
	{
		return Stats.ReactionEfficiency;
	}
	return 0.0f;
}

void ApplyPersistentSlotFrame(UButton* Button, UTexture2D* SlotTexture)
{
	if (!Button || !SlotTexture)
	{
		return;
	}

	FButtonStyle Style = Button->GetStyle();
	auto ConfigureBrush = [SlotTexture](FSlateBrush& Brush)
	{
		Brush.SetResourceObject(SlotTexture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = FVector2D(89.0f, 89.0f);
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
}

UReEchoInventoryShopWidget::UReEchoInventoryShopWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> ItemCardFinder(
	    TEXT("/Game/ReEcho/Textures/UI/InventoryShop/Plan110/T_UI_Shop110_OfferCard.T_UI_Shop110_OfferCard"));
	ShopItemCardTexture = ItemCardFinder.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> AttachmentSlotFinder(
	    TEXT("/Game/ReEcho/Textures/UI/InventoryShop/Plan110/"
	         "T_UI_Shop110_WeaponLoadoutSlot.T_UI_Shop110_WeaponLoadoutSlot"));
	ShopAttachmentSlotTexture = AttachmentSlotFinder.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> CorePrimordialIconFinder(
	    TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_CORE_PRIMORDIAL.T_UI_Part_P_CORE_PRIMORDIAL"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CoreTideIconFinder(
	    TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_CORE_TIDE.T_UI_Part_P_CORE_TIDE"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CoreForestIconFinder(
	    TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_CORE_FOREST.T_UI_Part_P_CORE_FOREST"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CoreFlameIconFinder(
	    TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_CORE_FLAME.T_UI_Part_P_CORE_FLAME"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CoreThunderIconFinder(
	    TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_CORE_THUNDER.T_UI_Part_P_CORE_THUNDER"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CorePrismIconFinder(
	    TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_CORE_PRISM.T_UI_Part_P_CORE_PRISM"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> BowSplitArrowheadIconFinder(TEXT(
	    "/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_BOW_SPLIT_ARROWHEAD.T_UI_Part_P_BOW_SPLIT_ARROWHEAD"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> BowExplosiveArrowheadIconFinder(
	    TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/"
	         "T_UI_Part_P_BOW_EXPLOSIVE_ARROWHEAD.T_UI_Part_P_BOW_EXPLOSIVE_ARROWHEAD"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> BowPiercingArrowheadIconFinder(
	    TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/"
	         "T_UI_Part_P_BOW_PIERCING_ARROWHEAD.T_UI_Part_P_BOW_PIERCING_ARROWHEAD"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> BowMultishotArrowheadIconFinder(
	    TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/"
	         "T_UI_Part_P_BOW_MULTISHOT_ARROWHEAD.T_UI_Part_P_BOW_MULTISHOT_ARROWHEAD"));
	WeaponPartIconTextures.Add(TEXT("P_CORE_PRIMORDIAL"), CorePrimordialIconFinder.Object);
	WeaponPartIconTextures.Add(TEXT("P_CORE_TIDE"), CoreTideIconFinder.Object);
	WeaponPartIconTextures.Add(TEXT("P_CORE_FOREST"), CoreForestIconFinder.Object);
	WeaponPartIconTextures.Add(TEXT("P_CORE_FLAME"), CoreFlameIconFinder.Object);
	WeaponPartIconTextures.Add(TEXT("P_CORE_THUNDER"), CoreThunderIconFinder.Object);
	WeaponPartIconTextures.Add(TEXT("P_CORE_PRISM"), CorePrismIconFinder.Object);
	WeaponPartIconTextures.Add(TEXT("P_BOW_SPLIT_ARROWHEAD"), BowSplitArrowheadIconFinder.Object);
	WeaponPartIconTextures.Add(TEXT("P_BOW_EXPLOSIVE_ARROWHEAD"), BowExplosiveArrowheadIconFinder.Object);
	WeaponPartIconTextures.Add(TEXT("P_BOW_PIERCING_ARROWHEAD"), BowPiercingArrowheadIconFinder.Object);
	WeaponPartIconTextures.Add(TEXT("P_BOW_MULTISHOT_ARROWHEAD"), BowMultishotArrowheadIconFinder.Object);
	static ConstructorHelpers::FObjectFinder<UTexture2D> CardIconFinder(
	    TEXT("/Game/ReEcho/Textures/UI/InteractionPlaceholder/InventoryShop/T_UI_Shop_CardIcon.T_UI_Shop_CardIcon"));
	ShopCardIconTexture = CardIconFinder.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> BuyFinder(
	    TEXT("/Game/ReEcho/Textures/UI/InventoryShop/Plan110/T_UI_Shop110_BuyButton.T_UI_Shop110_BuyButton"));
	ShopBuyTexture = BuyFinder.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> RefreshFinder(
	    TEXT("/Game/ReEcho/Textures/UI/InventoryShop/Plan110/T_UI_Shop110_RefreshButton.T_UI_Shop110_RefreshButton"));
	ShopRefreshTexture = RefreshFinder.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> CardSlotFinder(
	    TEXT("/Game/ReEcho/Textures/UI/InventoryShop/Plan110/"
	         "T_UI_Shop110_LoadoutCardSlot.T_UI_Shop110_LoadoutCardSlot"));
	ShopCardSlotTexture = CardSlotFinder.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> EmptyCardSlotIconFinder(
	    TEXT("/Game/ReEcho/Textures/UI/InventoryShop/Plan110/"
	         "T_UI_Shop110_EmptyCardSlotIcon.T_UI_Shop110_EmptyCardSlotIcon"));
	ShopEmptyCardSlotIconTexture = EmptyCardSlotIconFinder.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> ShopTitleFinder(
	    TEXT("/Game/ReEcho/Textures/UI/InteractionPlaceholder/InventoryShop/T_UI_Shop_Title.T_UI_Shop_Title"));
	ShopTitleTexture = ShopTitleFinder.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> CurrencyFrameFinder(
	    TEXT("/Game/ReEcho/Textures/UI/InventoryShop/Plan110/"
	         "T_UI_Shop110_CurrencyFrame.T_UI_Shop110_CurrencyFrame"));
	ShopCurrencyFrameTexture = CurrencyFrameFinder.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> WhiteFinder(
	    TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	WhiteTexture = WhiteFinder.Object;
}

bool UReEchoInventoryShopWidget::GetCardChoiceToShopCollapseTargetAbsolute(FVector2D& OutAbsoluteCenter) const
{
	const UWidget* LoadoutTree = GetWidgetFromName(TEXT("ArtFormalLoadoutTree"));
	if (!LoadoutTree)
	{
		return false;
	}

	const FGeometry& TreeGeometry = LoadoutTree->GetCachedGeometry();
	const FVector2D TreeSize = TreeGeometry.GetLocalSize();
	if (TreeSize.X <= 0.0f || TreeSize.Y <= 0.0f)
	{
		return false;
	}

	// The masked shopkeeper is baked into LoadoutTreePanel.png at this normalized anchor.
	const FVector2D MaskedShopkeeperAnchor(0.215f, 0.675f);
	OutAbsoluteCenter = TreeGeometry.LocalToAbsolute(TreeSize * MaskedShopkeeperAnchor);
	return true;
}

TSharedRef<SWidget> UReEchoInventoryShopWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	EnsureResponsiveLayout();
	return Super::RebuildWidget();
}

void UReEchoInventoryShopWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidgetTree();
	BuildOfferEntries();
	BuildTargetShopPresentation();
	BindSaveAndLeaveVisualFeedback();
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
	PurchasedItemIds.Reset();
	bEchoStoragePopupOpen = false;
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

void UReEchoInventoryShopWidget::SetWeaponPartShopView(const FReEchoWeaponPartShopView& PartShopView)
{
	CurrentPartShopView = PartShopView;
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
	PurchasedItemIds.Reset();
	bEchoStoragePopupOpen = false;
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
	if (!WidgetTree || (WidgetTree->RootWidget && InventoryPanel && ShopPanel && CurrencyText && InventoryText &&
	                    CloseButton && OfferContainer))
	{
		return;
	}

	UCanvasPanel* RootCanvas =
	    WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("InventoryShopRoot"));
	WidgetTree->RootWidget = RootCanvas;

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

void UReEchoInventoryShopWidget::EnsureResponsiveLayout()
{
	if (!WidgetTree || ResponsiveContentCanvas)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	struct FRootChildLayout
	{
		UWidget* Widget = nullptr;
		FAnchorData Layout;
		int32 ZOrder = 0;
		bool bAutoSize = false;
	};

	UWidget* Background = GetWidgetFromName(TEXT("BackgroundImage"));
	TArray<FRootChildLayout> ContentChildren;
	for (int32 ChildIndex = 0; ChildIndex < RootCanvas->GetChildrenCount(); ++ChildIndex)
	{
		UWidget* Child = RootCanvas->GetChildAt(ChildIndex);
		if (!Child || Child == Background)
		{
			continue;
		}
		const UCanvasPanelSlot* ChildSlot = Cast<UCanvasPanelSlot>(Child->Slot);
		if (!ChildSlot)
		{
			continue;
		}
		ContentChildren.Add({Child, ChildSlot->GetLayout(), ChildSlot->GetZOrder(), ChildSlot->GetAutoSize()});
	}

	ResponsiveContentScale =
	    WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("ResponsiveContentScale"));
	ResponsiveContentScale->SetStretch(EStretch::ScaleToFit);
	ResponsiveContentScale->SetStretchDirection(EStretchDirection::Both);
	ResponsiveContentScale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	USizeBox* ResponsiveContentSize =
	    WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ResponsiveContentSize"));
	ResponsiveContentSize->SetWidthOverride(ShopDesignWidth);
	ResponsiveContentSize->SetHeightOverride(ShopDesignHeight);
	ResponsiveContentScale->SetContent(ResponsiveContentSize);

	ResponsiveContentCanvas =
	    WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ResponsiveContentCanvas"));
	ResponsiveContentCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ResponsiveContentSize->SetContent(ResponsiveContentCanvas);

	for (const FRootChildLayout& ChildLayout : ContentChildren)
	{
		RootCanvas->RemoveChild(ChildLayout.Widget);
		UCanvasPanelSlot* NewSlot = ResponsiveContentCanvas->AddChildToCanvas(ChildLayout.Widget);
		NewSlot->SetLayout(ChildLayout.Layout);
		NewSlot->SetZOrder(ChildLayout.ZOrder);
		NewSlot->SetAutoSize(ChildLayout.bAutoSize);
	}

	UCanvasPanelSlot* ScaleSlot = RootCanvas->AddChildToCanvas(ResponsiveContentScale);
	ScaleSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	ScaleSlot->SetOffsets(FMargin(0.0f));
	ScaleSlot->SetZOrder(1);
}

UCanvasPanel* UReEchoInventoryShopWidget::GetLayoutCanvas() const
{
	if (ResponsiveContentCanvas)
	{
		return ResponsiveContentCanvas.Get();
	}
	return WidgetTree ? Cast<UCanvasPanel>(WidgetTree->RootWidget) : nullptr;
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
		ShopLogicScrollBox->SetScrollBarVisibility(ESlateVisibility::Visible);

		// The authored OfferContainer is sized by its contents, so nesting a ScrollBox under it lets the list
		// grow off-screen. Give the authored shop an independently bounded canvas viewport instead.
		if (OfferContainer != ShopPanel)
		{
			if (UCanvasPanel* RootCanvas = GetLayoutCanvas())
			{
				UCanvasPanelSlot* ScrollCanvasSlot = RootCanvas->AddChildToCanvas(ShopLogicScrollBox);
				ScrollCanvasSlot->SetOffsets(FMargin(0.0f));
				ScrollCanvasSlot->SetZOrder(10);
				UpdateShopLogicViewportBounds();
			}
		}
		if (!ShopLogicScrollBox->GetParent())
		{
			UVerticalBoxSlot* ScrollSlot = OfferContainer->AddChildToVerticalBox(ShopLogicScrollBox);
			ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ScrollSlot->SetPadding(FMargin(0.0f, 4.0f));
			if (UVerticalBoxSlot* OfferContainerSlot = Cast<UVerticalBoxSlot>(OfferContainer->Slot))
			{
				OfferContainerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}
	}
	if (!ShopLogicPanel)
	{
		ShopLogicPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ShopLogicPanel"));
		ShopLogicScrollBox->AddChild(ShopLogicPanel);
	}
}

void UReEchoInventoryShopWidget::OrderShopLogicBlocks()
{
	if (!ShopLogicPanel)
	{
		return;
	}

	// Purchasable weapon parts belong at the top of the shop, before ordinary run items.
	const TArray<UWidget*> OrderedBlocks = {
	    WeaponPartOfferPanel, RunItemOfferPanel, ShopControlPanel, WeaponLoadoutPanel};
	for (UWidget* Block : OrderedBlocks)
	{
		if (Block && Block->GetParent() == ShopLogicPanel)
		{
			Block->RemoveFromParent();
		}
	}
	for (UWidget* Block : OrderedBlocks)
	{
		if (Block)
		{
			ShopLogicPanel->AddChildToVerticalBox(Block);
		}
	}
}

void UReEchoInventoryShopWidget::UpdateShopLogicViewportBounds()
{
	if (UCanvasPanelSlot* ScrollCanvasSlot =
	        ShopLogicScrollBox ? Cast<UCanvasPanelSlot>(ShopLogicScrollBox->Slot) : nullptr)
	{
		const float Bottom = Mode == EReEchoInventoryShopMode::PostTraitIntermission ? 0.69f : 0.88f;
		ScrollCanvasSlot->SetAnchors(FAnchors(0.035f, 0.23f, 0.405f, Bottom));
		ScrollCanvasSlot->SetOffsets(FMargin(0.0f));
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
	    CurrentPartShopView.RunItemOffers.IsEmpty() ? GetReEchoShopCatalog() : CurrentPartShopView.RunItemOffers;
	for (const FReEchoShopOffer& Offer : SourceOffers)
	{
		if (Offer.Type == EReEchoShopOfferType::BuildCard)
		{
			VisibleRunItemOffers.Add(Offer);
		}
	}
	if (VisibleRunItemOffers.IsEmpty() && CurrentPartShopView.RunItemOffers.IsEmpty() &&
	    CurrentPartShopView.CardPackOffers.IsEmpty())
	{
		VisibleRunItemOffers = GetReEchoShopCatalog();
	}
	if (!RunItemOfferPanel)
	{
		RunItemOfferPanel =
		    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RunItemOfferPanel"));
		ShopLogicPanel->AddChildToVerticalBox(RunItemOfferPanel);
		UTextBlock* Title = CreateText(WidgetTree, TEXT("RunItemOfferTitle"), 20, FLinearColor(0.9f, 0.82f, 0.66f));
		Title->SetText(NSLOCTEXT("ReEcho", "RunItemOfferTitle", "卡牌组"));
		RunItemOfferPanel->AddChildToVerticalBox(Title);
	}
	if (CardPackButtons.Num() != CurrentPartShopView.CardPackOffers.Num())
	{
		for (UReEchoIndexedButton* Button : CardPackButtons)
		{
			if (Button)
			{
				Button->RemoveFromParent();
			}
		}
		CardPackButtons.Reset();
		CardPackTexts.Reset();
	}
	for (int32 PackIndex = 0; PackIndex < CurrentPartShopView.CardPackOffers.Num(); ++PackIndex)
	{
		if (CardPackButtons.IsValidIndex(PackIndex))
		{
			continue;
		}
		UReEchoIndexedButton* PackButton = WidgetTree->ConstructWidget<UReEchoIndexedButton>(
		    UReEchoIndexedButton::StaticClass(), *FString::Printf(TEXT("ShopCardPack%d"), PackIndex));
		PackButton->SetEntryIndex(PackIndex);
		PackButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleCardPackClicked);
		PackButton->SetBackgroundColor(FLinearColor(0.15f, 0.11f, 0.07f, 0.88f));
		UVerticalBoxSlot* PackSlot = RunItemOfferPanel->AddChildToVerticalBox(PackButton);
		PackSlot->SetPadding(FMargin(8.0f, 6.0f));
		UTextBlock* PackText = CreateText(
		    WidgetTree, *FString::Printf(TEXT("ShopCardPackText%d"), PackIndex), 20, FLinearColor(0.9f, 0.82f, 0.66f));
		PackText->SetJustification(ETextJustify::Center);
		PackButton->SetContent(PackText);
		CardPackButtons.Add(PackButton);
		CardPackTexts.Add(PackText);
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
	OrderShopLogicBlocks();

	// ---- Echo storage popup (opened only from the owned G_3_02 card slot) ----
	// Built once (BuildWidgetTree is guarded). Visibility and content are driven by BuildEchoPanel().
	if (EchoPanel)
	{
		return;
	}
	UCanvasPanel* RootCanvas = GetLayoutCanvas();
	if (!RootCanvas)
	{
		return;
	}
	EchoPanelScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("EchoPanelScale"));
	EchoPanelScale->SetStretch(EStretch::ScaleToFit);
	EchoPanelScale->SetStretchDirection(EStretchDirection::DownOnly);
	EchoPanelScale->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* EchoSlotCanvas = RootCanvas->AddChildToCanvas(EchoPanelScale);
	EchoSlotCanvas->SetAnchors(FAnchors(0.32f, 0.16f, 0.68f, 0.82f));
	EchoSlotCanvas->SetOffsets(FMargin(0.0f));
	EchoSlotCanvas->SetZOrder(40);

	UBorder* EchoPopupFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EchoPopupFrame"));
	EchoPopupFrame->SetBrushColor(FLinearColor::Black);
	EchoPopupFrame->SetPadding(FMargin(6.0f));
	EchoPanelScale->SetContent(EchoPopupFrame);
	UBorder* EchoPopupSurface = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EchoPopupSurface"));
	EchoPopupSurface->SetBrushColor(FLinearColor(0.96f, 0.96f, 0.96f, 1.0f));
	EchoPopupSurface->SetPadding(FMargin(28.0f, 22.0f));
	EchoPopupFrame->SetContent(EchoPopupSurface);

	EchoPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EchoPanel"));
	EchoPanel->SetVisibility(ESlateVisibility::Collapsed);
	EchoPopupSurface->SetContent(EchoPanel);

	EchoPopupCloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EchoPopupCloseButton"));
	EchoPopupCloseButton->SetBackgroundColor(FLinearColor(0.12f, 0.12f, 0.12f, 1.0f));
	UTextBlock* EchoPopupCloseLabel = CreateText(WidgetTree, TEXT("EchoPopupCloseLabel"), 20, FLinearColor::White);
	EchoPopupCloseLabel->SetText(NSLOCTEXT("ReEcho", "EchoPopupClose", "关闭回响存储"));
	EchoPopupCloseLabel->SetJustification(ETextJustify::Center);
	EchoPopupCloseButton->SetContent(EchoPopupCloseLabel);
	EchoPopupCloseButton->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleEchoPopupCloseClicked);
	EchoPanel->AddChildToVerticalBox(EchoPopupCloseButton);

	EchoCapacityText = CreateText(WidgetTree, TEXT("EchoCapacity"), 24, FLinearColor(0.06f, 0.06f, 0.06f));
	EchoCapacityText->SetJustification(ETextJustify::Center);
	EchoPanel->AddChildToVerticalBox(EchoCapacityText);

	EchoReplayModeText = CreateText(WidgetTree, TEXT("EchoReplayMode"), 20, FLinearColor(0.12f, 0.12f, 0.12f));
	EchoReplayModeText->SetAutoWrapText(true);
	EchoPanel->AddChildToVerticalBox(EchoReplayModeText);

	EchoPendingInfoText = CreateText(WidgetTree, TEXT("EchoPendingInfo"), 22, FLinearColor(0.12f, 0.12f, 0.12f));
	EchoPendingInfoText->SetAutoWrapText(true);
	EchoPanel->AddChildToVerticalBox(EchoPendingInfoText);

	EchoStoreButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EchoStoreButton"));
	EchoStoreButton->SetBackgroundColor(FLinearColor(0.2f, 0.5f, 0.2f, 0.9f));
	{
		UTextBlock* T = CreateText(WidgetTree, TEXT("EchoStoreLabel"), 22, FLinearColor(1.0f, 1.0f, 1.0f));
		T->SetText(NSLOCTEXT("ReEcho", "StorePendingEcho", "存储本场回响"));
		T->SetJustification(ETextJustify::Center);
		EchoStoreButton->SetContent(T);
	}
	EchoStoreButton->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleStoreClicked);
	EchoPanel->AddChildToVerticalBox(EchoStoreButton);

	EchoSkipButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EchoSkipButton"));
	EchoSkipButton->SetBackgroundColor(FLinearColor(0.5f, 0.2f, 0.2f, 0.9f));
	{
		UTextBlock* T = CreateText(WidgetTree, TEXT("EchoSkipLabel"), 22, FLinearColor(1.0f, 1.0f, 1.0f));
		T->SetText(NSLOCTEXT("ReEcho", "SkipPendingEcho", "跳过本场回响"));
		T->SetJustification(ETextJustify::Center);
		EchoSkipButton->SetContent(T);
	}
	EchoSkipButton->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleSkipClicked);
	EchoPanel->AddChildToVerticalBox(EchoSkipButton);

	EchoReplaceInstructionText =
	    CreateText(WidgetTree, TEXT("EchoReplaceInstruction"), 20, FLinearColor(0.12f, 0.12f, 0.12f));
	EchoReplaceInstructionText->SetAutoWrapText(true);
	EchoPanel->AddChildToVerticalBox(EchoReplaceInstructionText);

	EchoCancelReplaceButton =
	    WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EchoCancelReplaceButton"));
	EchoCancelReplaceButton->SetBackgroundColor(FLinearColor(0.4f, 0.4f, 0.4f, 0.9f));
	{
		UTextBlock* T = CreateText(WidgetTree, TEXT("EchoCancelReplaceLabel"), 22, FLinearColor(1.0f, 1.0f, 1.0f));
		T->SetText(NSLOCTEXT("ReEcho", "CancelEchoReplacement", "取消替换"));
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
		    WidgetTree, *FString::Printf(TEXT("EchoSlotText%d"), SlotIndex), 18, FLinearColor(0.12f, 0.12f, 0.12f));
		EchoSlotTexts[SlotIndex]->SetAutoWrapText(true);
		EchoPanel->AddChildToVerticalBox(EchoSlotTexts[SlotIndex]);

		EchoReplaceButtons[SlotIndex] = WidgetTree->ConstructWidget<UButton>(
		    UButton::StaticClass(), *FString::Printf(TEXT("EchoReplace%d"), SlotIndex));
		EchoReplaceButtons[SlotIndex]->SetBackgroundColor(FLinearColor(0.4f, 0.3f, 0.15f, 0.9f));
		UTextBlock* RLabel = CreateText(
		    WidgetTree, *FString::Printf(TEXT("EchoReplaceLabel%d"), SlotIndex), 18, FLinearColor(1.0f, 1.0f, 1.0f));
		RLabel->SetText(NSLOCTEXT("ReEcho", "ReplaceThisEcho", "替换这个回响"));
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
		EchoSelectLabels[SlotIndex]->SetText(NSLOCTEXT("ReEcho", "SelectEchoForReplay", "选为下场回放"));
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

	EchoSelectionText = CreateText(WidgetTree, TEXT("EchoSelection"), 20, FLinearColor(0.08f, 0.22f, 0.08f));
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
		if (Offer.Type == EReEchoShopOfferType::WeaponPart || Offer.Type == EReEchoShopOfferType::Weapon)
		{
			VisibleWeaponPartOffers.Add(Offer);
		}
	}
	if (!WeaponPartOfferPanel)
	{
		WeaponPartOfferPanel =
		    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WeaponPartOfferPanel"));
		ShopLogicPanel->AddChildToVerticalBox(WeaponPartOfferPanel);
		UTextBlock* Title = CreateText(WidgetTree, TEXT("WeaponPartOfferTitle"), 20, FLinearColor(0.9f, 0.82f, 0.66f));
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
		WeaponLoadoutText = CreateText(WidgetTree, TEXT("WeaponLoadoutText"), 18, FLinearColor(0.9f, 0.82f, 0.66f));
		UVerticalBoxSlot* LoadoutTextSlot = WeaponLoadoutPanel->AddChildToVerticalBox(WeaponLoadoutText);
		LoadoutTextSlot->SetPadding(FMargin(18.0f, 16.0f, 18.0f, 8.0f));
	}
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
			Button->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleWeaponPartOfferClicked);
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
	OrderShopLogicBlocks();
}

void UReEchoInventoryShopWidget::BuildTargetShopPresentation()
{
	BindDesignerLoadoutLayout();
	if (BindAuthoredShopPresentation())
	{
		return;
	}
	if (ShopPresentationLayer || !WidgetTree)
	{
		return;
	}
	UCanvasPanel* RootCanvas = GetLayoutCanvas();
	if (!RootCanvas)
	{
		return;
	}

	ShopPresentationLayer =
	    WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ShopPresentationLayer"));
	UCanvasPanelSlot* LayerSlot = RootCanvas->AddChildToCanvas(ShopPresentationLayer);
	LayerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	LayerSlot->SetOffsets(FMargin(0.0f));
	LayerSlot->SetZOrder(15);
	ShopPresentationLayer->SetVisibility(ESlateVisibility::Collapsed);
	UImage* TargetTitle = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TargetShopTitleArt"));
	TargetTitle->SetBrushFromTexture(ShopTitleTexture, true);
	TargetTitle->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* TargetTitleSlot = ShopPresentationLayer->AddChildToCanvas(TargetTitle);
	TargetTitleSlot->SetPosition(FVector2D(53.0f, 90.0f));
	TargetTitleSlot->SetSize(FVector2D(228.0f, 58.0f));
	UImage* CurrencyFrame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TargetShopCurrencyFrame"));
	CurrencyFrame->SetBrushFromTexture(ShopCurrencyFrameTexture, true);
	CurrencyFrame->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* CurrencyFrameSlot = ShopPresentationLayer->AddChildToCanvas(CurrencyFrame);
	CurrencyFrameSlot->SetPosition(FVector2D(390.0f, 103.0f));
	CurrencyFrameSlot->SetSize(FVector2D(270.0f, 38.0f));

	auto AddLabel = [&](const TCHAR* Name,
	                    const FString& Value,
	                    const FVector2D Position,
	                    const FVector2D Size,
	                    const int32 FontSize)
	{
		UTextBlock* Label = CreateText(WidgetTree, Name, FontSize, FLinearColor(0.05f, 0.05f, 0.05f));
		Label->SetText(FText::FromString(Value));
		UCanvasPanelSlot* Slot = ShopPresentationLayer->AddChildToCanvas(Label);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		return Label;
	};
	AddLabel(TEXT("TargetPartTitle"), TEXT("配件"), FVector2D(84.0f, 178.0f), FVector2D(180.0f, 38.0f), 24);
	AddLabel(TEXT("TargetCardTitle"), TEXT("卡牌"), FVector2D(84.0f, 570.0f), FVector2D(180.0f, 38.0f), 24);

	TargetCurrencyText =
	    AddLabel(TEXT("TargetShopCurrency"), TEXT(""), FVector2D(415.0f, 106.0f), FVector2D(230.0f, 32.0f), 19);
	TargetCurrencyText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TargetCurrencyText->SetJustification(ETextJustify::Center);
	TargetRefreshLimitText =
	    AddLabel(TEXT("TargetRefreshLimitText"), TEXT(""), FVector2D(670.0f, 145.0f), FVector2D(200.0f, 30.0f), 16);
	TargetRefreshLimitText->SetColorAndOpacity(FSlateColor(FLinearColor(0.20f, 0.20f, 0.20f, 1.0f)));
	TargetRefreshLimitText->SetJustification(ETextJustify::Center);
	UTextBlock* CurrencyDiamond =
	    AddLabel(TEXT("TargetShopCurrencyDiamond"), TEXT("◆"), FVector2D(402.0f, 104.0f), FVector2D(42.0f, 34.0f), 25);
	CurrencyDiamond->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.82f, 0.0f, 1.0f)));

	TargetPartOfferRow =
	    WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TargetPartOfferRow"));
	UCanvasPanelSlot* PartRowSlot = ShopPresentationLayer->AddChildToCanvas(TargetPartOfferRow);
	PartRowSlot->SetPosition(FVector2D(106.0f, 238.0f));
	PartRowSlot->SetSize(FVector2D(760.0f, 330.0f));
	TargetCardOfferRow =
	    WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TargetCardOfferRow"));
	UCanvasPanelSlot* CardRowSlot = ShopPresentationLayer->AddChildToCanvas(TargetCardOfferRow);
	CardRowSlot->SetPosition(FVector2D(106.0f, 628.0f));
	CardRowSlot->SetSize(FVector2D(760.0f, 330.0f));

	if (ShopRefreshButton)
	{
		ShopRefreshButton->RemoveFromParent();
	}
	else
	{
		ShopRefreshButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ShopRefreshButton"));
	}
	ShopRefreshButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
	UImage* RefreshArt = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TargetRefreshArt"));
	RefreshArt->SetBrushFromTexture(ShopRefreshTexture, true);
	RefreshArt->SetVisibility(ESlateVisibility::HitTestInvisible);
	ShopRefreshButton->SetContent(RefreshArt);
	ShopRefreshButton->OnClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleRefreshClicked);
	UCanvasPanelSlot* RefreshSlot = ShopPresentationLayer->AddChildToCanvas(ShopRefreshButton);
	RefreshSlot->SetPosition(FVector2D(691.0f, 103.0f));
	RefreshSlot->SetSize(FVector2D(156.0f, 44.0f));
}

void UReEchoInventoryShopWidget::BindSaveAndLeaveVisualFeedback()
{
	UWidget* SaveAndLeaveArt = GetWidgetFromName(TEXT("ArtFormalSaveAndLeave"));
	if (!CloseButton || !SaveAndLeaveArt)
	{
		return;
	}

	if (!SaveAndLeaveVisualFeedback)
	{
		SaveAndLeaveVisualFeedback = NewObject<UReEchoButtonVisualFeedback>(this);
	}
	SaveAndLeaveVisualFeedback->Bind(CloseButton, SaveAndLeaveArt, TEXT("InventoryShop"), TEXT("SaveAndLeave"));
}

bool UReEchoInventoryShopWidget::BindAuthoredShopPresentation()
{
	UCanvasPanel* AuthoredCanvas = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("DesignerShopPresentationCanvas")));
	if (!AuthoredCanvas)
	{
		return false;
	}

	ShopPresentationLayer = AuthoredCanvas;
	TargetCurrencyText = Cast<UTextBlock>(GetWidgetFromName(TEXT("DesignerCurrencyText")));
	TargetRefreshLimitText = Cast<UTextBlock>(GetWidgetFromName(TEXT("DesignerRefreshLimitText")));
	ShopRefreshButton = Cast<UButton>(GetWidgetFromName(TEXT("ShopRefreshButton")));
	if (ShopRefreshButton)
	{
		// BuildOfferEntries() also supports the legacy shop and therefore creates a text-only
		// child first. Restore the authored Plan110 button art after that legacy pass.
		UImage* RefreshArt = Cast<UImage>(GetWidgetFromName(TEXT("DesignerRefreshArt")));
		if (!RefreshArt)
		{
			RefreshArt = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DesignerRefreshArt"));
		}
		RefreshArt->RemoveFromParent();
		RefreshArt->SetBrushFromTexture(ShopRefreshTexture.Get(), false);
		RefreshArt->SetColorAndOpacity(FLinearColor::White);
		RefreshArt->SetVisibility(ESlateVisibility::HitTestInvisible);
		ShopRefreshButton->SetContent(RefreshArt);
		if (UButtonSlot* RefreshContentSlot = Cast<UButtonSlot>(RefreshArt->Slot))
		{
			RefreshContentSlot->SetPadding(FMargin(0.0f));
			RefreshContentSlot->SetHorizontalAlignment(HAlign_Fill);
			RefreshContentSlot->SetVerticalAlignment(VAlign_Fill);
		}
		FButtonStyle RefreshStyle = ShopRefreshButton->GetStyle();
		RefreshStyle.NormalPadding = FMargin(0.0f);
		RefreshStyle.PressedPadding = FMargin(0.0f);
		ShopRefreshButton->SetStyle(RefreshStyle);
		if (ShopRefreshText)
		{
			ShopRefreshText->SetVisibility(ESlateVisibility::Collapsed);
		}
		ShopRefreshButton->OnClicked.RemoveDynamic(this, &UReEchoInventoryShopWidget::HandleRefreshClicked);
		ShopRefreshButton->OnClicked.AddDynamic(this, &UReEchoInventoryShopWidget::HandleRefreshClicked);
	}

	DesignerPartOfferCards.Reset();
	DesignerPartOfferIcons.Reset();
	DesignerPartOfferDescriptions.Reset();
	DesignerPartOfferCosts.Reset();
	DesignerPartOfferBuyButtons.Reset();
	DesignerPartOfferBuyArts.Reset();
	DesignerPartOfferBuyLabels.Reset();
	DesignerPackOfferCards.Reset();
	DesignerPackOfferIcons.Reset();
	DesignerPackOfferDescriptions.Reset();
	DesignerPackOfferCosts.Reset();
	DesignerPackOfferBuyButtons.Reset();
	DesignerPackOfferBuyArts.Reset();
	DesignerPackOfferBuyLabels.Reset();

	for (int32 Index = 0; Index < 3; ++Index)
	{
		DesignerPartOfferCards.Add(
		    Cast<UCanvasPanel>(GetWidgetFromName(*FString::Printf(TEXT("DesignerPartOfferCard%d"), Index))));
		DesignerPartOfferIcons.Add(
		    Cast<UImage>(GetWidgetFromName(*FString::Printf(TEXT("DesignerPartOfferIcon%d"), Index))));
		DesignerPartOfferDescriptions.Add(
		    Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("DesignerPartOfferDescription%d"), Index))));
		DesignerPartOfferCosts.Add(
		    Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("DesignerPartOfferCost%d"), Index))));
		UReEchoIndexedButton* PartBuy =
		    Cast<UReEchoIndexedButton>(GetWidgetFromName(*FString::Printf(TEXT("DesignerPartOfferBuy%d"), Index)));
		DesignerPartOfferBuyButtons.Add(PartBuy);
		DesignerPartOfferBuyArts.Add(
		    Cast<UImage>(GetWidgetFromName(*FString::Printf(TEXT("DesignerPartOfferBuy%dArt"), Index))));
		DesignerPartOfferBuyLabels.Add(
		    Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("DesignerPartOfferBuy%dLabel"), Index))));
		if (PartBuy)
		{
			PartBuy->SetEntryIndex(Index);
			PartBuy->OnIndexedClicked.RemoveDynamic(this, &UReEchoInventoryShopWidget::HandleWeaponPartOfferClicked);
			PartBuy->OnIndexedClicked.AddDynamic(this, &UReEchoInventoryShopWidget::HandleWeaponPartOfferClicked);
		}

		DesignerPackOfferCards.Add(
		    Cast<UCanvasPanel>(GetWidgetFromName(*FString::Printf(TEXT("DesignerPackOfferCard%d"), Index))));
		DesignerPackOfferIcons.Add(
		    Cast<UImage>(GetWidgetFromName(*FString::Printf(TEXT("DesignerPackOfferIcon%d"), Index))));
		DesignerPackOfferDescriptions.Add(
		    Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("DesignerPackOfferDescription%d"), Index))));
		DesignerPackOfferCosts.Add(
		    Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("DesignerPackOfferCost%d"), Index))));
		UReEchoIndexedButton* PackBuy =
		    Cast<UReEchoIndexedButton>(GetWidgetFromName(*FString::Printf(TEXT("DesignerPackOfferBuy%d"), Index)));
		DesignerPackOfferBuyButtons.Add(PackBuy);
		DesignerPackOfferBuyArts.Add(
		    Cast<UImage>(GetWidgetFromName(*FString::Printf(TEXT("DesignerPackOfferBuy%dArt"), Index))));
		DesignerPackOfferBuyLabels.Add(
		    Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("DesignerPackOfferBuy%dLabel"), Index))));
		if (PackBuy)
		{
			PackBuy->SetEntryIndex(Index);
			PackBuy->OnIndexedClicked.RemoveDynamic(this, &UReEchoInventoryShopWidget::HandleCardPackClicked);
			PackBuy->OnIndexedClicked.AddDynamic(this, &UReEchoInventoryShopWidget::HandleCardPackClicked);
		}
	}
	return true;
}

void UReEchoInventoryShopWidget::BindDesignerLoadoutLayout()
{
	DesignerLoadoutCanvas = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("DesignerLoadoutCanvas")));
	DesignerWeaponPanelWidget = Cast<UImage>(GetWidgetFromName(TEXT("DesignerWeaponPanel")));
	DesignerEquippedWeaponButton = Cast<UButton>(GetWidgetFromName(TEXT("DesignerWeaponInteractionButton")));
	DesignerEquippedWeaponArt = Cast<UImage>(GetWidgetFromName(TEXT("DesignerWeaponInteractionArt")));
	if (!DesignerEquippedWeaponButton)
	{
		DesignerEquippedWeaponButton = Cast<UButton>(GetWidgetFromName(TEXT("DesignerEquippedWeaponButton")));
		DesignerEquippedWeaponArt = Cast<UImage>(GetWidgetFromName(TEXT("DesignerEquippedWeaponArt")));
	}
	if (DesignerLoadoutCanvas && DesignerWeaponPanelWidget && !DesignerEquippedWeaponButton)
	{
		DesignerEquippedWeaponButton =
		    WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DesignerEquippedWeaponButton"));
		DesignerEquippedWeaponButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
		UScaleBox* WeaponScale =
		    WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("DesignerEquippedWeaponScale"));
		WeaponScale->SetStretch(EStretch::ScaleToFit);
		WeaponScale->SetStretchDirection(EStretchDirection::Both);
		DesignerEquippedWeaponArt =
		    WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DesignerEquippedWeaponArt"));
		DesignerEquippedWeaponArt->SetVisibility(ESlateVisibility::HitTestInvisible);
		WeaponScale->SetContent(DesignerEquippedWeaponArt);
		DesignerEquippedWeaponButton->SetContent(WeaponScale);
		if (UButtonSlot* WeaponContentSlot = Cast<UButtonSlot>(WeaponScale->Slot))
		{
			WeaponContentSlot->SetPadding(FMargin(0.0f));
			WeaponContentSlot->SetHorizontalAlignment(HAlign_Fill);
			WeaponContentSlot->SetVerticalAlignment(VAlign_Fill);
		}
		UCanvasPanelSlot* WeaponButtonSlot = DesignerLoadoutCanvas->AddChildToCanvas(DesignerEquippedWeaponButton);
		if (const UCanvasPanelSlot* WeaponPanelSlot = Cast<UCanvasPanelSlot>(DesignerWeaponPanelWidget->Slot))
		{
			WeaponButtonSlot->SetPosition(WeaponPanelSlot->GetPosition());
			WeaponButtonSlot->SetSize(WeaponPanelSlot->GetSize());
		}
		WeaponButtonSlot->SetZOrder(9);
	}
	if (DesignerEquippedWeaponButton)
	{
		DesignerEquippedWeaponButton->OnClicked.RemoveDynamic(this,
		                                                      &UReEchoInventoryShopWidget::HandleEquippedWeaponClicked);
		DesignerEquippedWeaponButton->OnClicked.AddDynamic(this,
		                                                   &UReEchoInventoryShopWidget::HandleEquippedWeaponClicked);
	}
	DesignerAttachmentSlotButtons.Reset();
	DesignerAttachmentSlotArts.Reset();
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UButton* AttachmentButton =
		    Cast<UButton>(GetWidgetFromName(*FString::Printf(TEXT("DesignerAttachmentSlot%d"), Index)));
		UImage* AttachmentArt =
		    Cast<UImage>(GetWidgetFromName(*FString::Printf(TEXT("DesignerAttachmentSlotArt%d"), Index)));
		DesignerAttachmentSlotButtons.Add(AttachmentButton);
		DesignerAttachmentSlotArts.Add(AttachmentArt);
		ApplyPersistentSlotFrame(AttachmentButton, ShopAttachmentSlotTexture.Get());
		if (AttachmentArt)
		{
			if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(AttachmentArt->Slot))
			{
				ContentSlot->SetPadding(FMargin(0.0f));
				ContentSlot->SetHorizontalAlignment(HAlign_Fill);
				ContentSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
	}

	DesignerCardSlotButtons.Reset();
	DesignerCardSlotArts.Reset();
	for (int32 Index = 0; Index < 12; ++Index)
	{
		UButton* CardButton = Cast<UButton>(GetWidgetFromName(*FString::Printf(TEXT("DesignerCardSlot%d"), Index)));
		UImage* CardArt = Cast<UImage>(GetWidgetFromName(*FString::Printf(TEXT("DesignerCardSlotArt%d"), Index)));
		DesignerCardSlotButtons.Add(CardButton);
		DesignerCardSlotArts.Add(CardArt);
		ApplyPersistentSlotFrame(CardButton, ShopCardSlotTexture.Get());
		if (CardArt)
		{
			if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(CardArt->Slot))
			{
				ContentSlot->SetPadding(FMargin(0.0f));
				ContentSlot->SetHorizontalAlignment(HAlign_Fill);
				ContentSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
	}
}

void UReEchoInventoryShopWidget::AddTargetOfferCard(UHorizontalBox* Row,
                                                    const FReEchoShopOffer& Offer,
                                                    const int32 OfferIndex,
                                                    const bool bWeaponPart)
{
	if (!Row)
	{
		return;
	}
	USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(
	    USizeBox::StaticClass(),
	    *FString::Printf(TEXT("Target%sCardSize%d"), bWeaponPart ? TEXT("Part") : TEXT("Build"), OfferIndex));
	CardSize->SetWidthOverride(200.0f);
	CardSize->SetHeightOverride(292.0f);
	UHorizontalBoxSlot* RowSlot = Row->AddChildToHorizontalBox(CardSize);
	RowSlot->SetPadding(FMargin(4.0f, 0.0f, 25.0f, 0.0f));

	UCanvasPanel* Card = WidgetTree->ConstructWidget<UCanvasPanel>(
	    UCanvasPanel::StaticClass(),
	    *FString::Printf(TEXT("Target%sCard%d"), bWeaponPart ? TEXT("Part") : TEXT("Build"), OfferIndex));
	CardSize->SetContent(Card);
	const bool bEmptyBuildCardSlot =
	    !bWeaponPart && Offer.Type == EReEchoShopOfferType::BuildCard && Offer.ItemId.IsNone();
	if (!bEmptyBuildCardSlot)
	{
		Card->SetToolTip(BuildSlotTooltip(Offer));
	}
	auto AddImage = [&](const TCHAR* Name, UTexture2D* Texture, FVector2D Position, FVector2D Size)
	{
		UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		Image->SetBrushFromTexture(Texture, true);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* Slot = Card->AddChildToCanvas(Image);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		return Image;
	};
	AddImage(*FString::Printf(TEXT("TargetCardBase%d_%d"), bWeaponPart, OfferIndex),
	         ShopItemCardTexture,
	         FVector2D::ZeroVector,
	         FVector2D(200.0f, 292.0f));
	const FVector2D IconPosition = bWeaponPart ? FVector2D(37.0f, 17.0f) : FVector2D(29.0f, 4.0f);
	const FVector2D IconSize = bWeaponPart ? FVector2D(127.0f, 124.0f) : FVector2D(141.0f, 173.0f);
	// 图标按 Offer 类型区分：配件走 PartId 动态加载；武器走对应配图(开局选武器界面同款)；其余回退默认卡片图标
	const bool bIsPart = (Offer.Type == EReEchoShopOfferType::WeaponPart);
	UTexture2D* ResolvedIcon = ShopCardIconTexture.Get();
	if (!bIsPart && !Offer.IconTexturePath.IsEmpty())
	{
		if (UTexture2D* LoadedWeaponIcon = LoadObject<UTexture2D>(nullptr, *Offer.IconTexturePath))
		{
			ResolvedIcon = LoadedWeaponIcon;
		}
	}
	UImage* OfferIcon = AddImage(*FString::Printf(TEXT("TargetCardIcon%d_%d"), bWeaponPart, OfferIndex),
	                             bIsPart ? ResolveWeaponPartIcon(Offer.ContentId) : ResolvedIcon,
	                             IconPosition,
	                             IconSize);
	OfferIcon->SetVisibility(bEmptyBuildCardSlot ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (!bWeaponPart)
	{
		UImage* TierPatch = AddImage(*FString::Printf(TEXT("TargetTierPatch%d"), OfferIndex),
		                             WhiteTexture,
		                             FVector2D(66.0f, 76.0f),
		                             FVector2D(70.0f, 34.0f));
		TierPatch->SetColorAndOpacity(FLinearColor(0.94f, 0.94f, 0.94f, 1.0f));
		UTextBlock* TierText = CreateText(
		    WidgetTree, *FString::Printf(TEXT("TargetTierText%d"), OfferIndex), 18, FLinearColor(0.05f, 0.05f, 0.05f));
		TierText->SetText(FText::FromString(Offer.Tier == 1   ? TEXT("一级")
		                                    : Offer.Tier == 2 ? TEXT("二级")
		                                                      : TEXT("三级")));
		TierText->SetJustification(ETextJustify::Center);
		UCanvasPanelSlot* TierSlot = Card->AddChildToCanvas(TierText);
		TierSlot->SetPosition(FVector2D(66.0f, 78.0f));
		TierSlot->SetSize(FVector2D(70.0f, 30.0f));
	}

	UTextBlock* Description = CreateText(WidgetTree,
	                                     *FString::Printf(TEXT("TargetOfferDescription%d_%d"), bWeaponPart, OfferIndex),
	                                     15,
	                                     FLinearColor(0.05f, 0.05f, 0.05f));
	Description->SetText(Offer.DisplayName);
	Description->SetJustification(ETextJustify::Center);
	Description->SetAutoWrapText(true);
	UCanvasPanelSlot* DescriptionSlot = Card->AddChildToCanvas(Description);
	DescriptionSlot->SetPosition(FVector2D(10.0f, 180.0f));
	DescriptionSlot->SetSize(FVector2D(180.0f, 55.0f));

	UTextBlock* Cost = CreateText(WidgetTree,
	                              *FString::Printf(TEXT("TargetOfferCost%d_%d"), bWeaponPart, OfferIndex),
	                              17,
	                              FLinearColor(0.04f, 0.04f, 0.04f));
	Cost->SetText(bEmptyBuildCardSlot ? FText::FromString(TEXT("—"))
	                                  : FText::Format(NSLOCTEXT("ReEcho", "TargetShopCost", "◆ {0}"),
	                                                  FText::AsNumber(Offer.EffectivePrice)));
	Cost->SetJustification(ETextJustify::Right);
	UCanvasPanelSlot* CostSlot = Card->AddChildToCanvas(Cost);
	CostSlot->SetPosition(FVector2D(112.0f, 145.0f));
	CostSlot->SetSize(FVector2D(80.0f, 30.0f));

	UReEchoIndexedButton* Buy = WidgetTree->ConstructWidget<UReEchoIndexedButton>(
	    UReEchoIndexedButton::StaticClass(),
	    *FString::Printf(TEXT("Target%sBuy%d"), bWeaponPart ? TEXT("Part") : TEXT("Build"), OfferIndex));
	Buy->SetEntryIndex(OfferIndex);
	Buy->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
	const bool bOwnedPart = bWeaponPart && CurrentPartShopView.OwnedParts.ContainsByPredicate(
	                                           [&](const FReEchoShopOffer& Owned)
	                                           {
		                                           return Owned.ContentId == Offer.ContentId;
	                                           });
	const bool bPurchasedCard = !bWeaponPart && CurrentOwnedItems.Contains(Offer.ItemId);
	const bool bOwnedWeapon =
	    Offer.Type == EReEchoShopOfferType::Weapon && CurrentPartShopView.OwnedWeapons.Contains(Offer.ContentId);
	const bool bPurchasedSlot = PurchasedItemIds.Contains(Offer.ItemId);
	const bool bShowPurchased = bOwnedPart || bPurchasedCard || bPurchasedSlot || bOwnedWeapon;
	UOverlay* ButtonOverlay = WidgetTree->ConstructWidget<UOverlay>(
	    UOverlay::StaticClass(), *FString::Printf(TEXT("TargetBuyOverlay%d_%d"), bWeaponPart, OfferIndex));
	UImage* BuyArt = WidgetTree->ConstructWidget<UImage>(
	    UImage::StaticClass(), *FString::Printf(TEXT("TargetBuyArt%d_%d"), bWeaponPart, OfferIndex));
	BuyArt->SetBrushFromTexture(bEmptyBuildCardSlot || bShowPurchased ? WhiteTexture : ShopBuyTexture, true);
	if (bEmptyBuildCardSlot)
	{
		BuyArt->SetColorAndOpacity(FLinearColor(0.55f, 0.55f, 0.55f, 1.0f));
	}
	else if (bOwnedPart || bPurchasedCard)
	{
		BuyArt->SetColorAndOpacity(bPurchasedCard ? FLinearColor(0.55f, 0.55f, 0.55f, 1.0f)
		                                          : FLinearColor(0.65f, 0.9f, 0.65f, 1.0f));
	}
	BuyArt->SetVisibility(ESlateVisibility::HitTestInvisible);
	ButtonOverlay->AddChildToOverlay(BuyArt);
	if (bEmptyBuildCardSlot || bShowPurchased)
	{
		UTextBlock* BuyText = CreateText(WidgetTree,
		                                 *FString::Printf(TEXT("TargetBuyText%d_%d"), bWeaponPart, OfferIndex),
		                                 18,
		                                 FLinearColor(0.03f, 0.03f, 0.03f));
		BuyText->SetText(bEmptyBuildCardSlot ? Offer.DisplayName
		                 : bOwnedWeapon      ? FText::FromString(TEXT("已获得"))
		                                     : (bPurchasedSlot ? FText::FromString(TEXT("已购"))
		                                                       : (bOwnedPart ? (IsPartEquipped(Offer.ContentId)
		                                                                            ? FText::FromString(TEXT("已装备"))
		                                                                            : FText::FromString(TEXT("已获得")))
		                                                                     : FText::FromString(TEXT("已获得")))));
		BuyText->SetJustification(ETextJustify::Center);
		UOverlaySlot* TextSlot = ButtonOverlay->AddChildToOverlay(BuyText);
		TextSlot->SetHorizontalAlignment(HAlign_Fill);
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}
	Buy->SetContent(ButtonOverlay);
	Buy->SetIsEnabled(!bEmptyBuildCardSlot && !bShowPurchased &&
	                  (bOwnedPart || (!bPurchasedCard && Offer.bCanPurchase)));
	if (bWeaponPart)
	{
		Buy->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleWeaponPartOfferClicked);
	}
	else
	{
		Buy->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleOfferClicked);
	}
	UCanvasPanelSlot* BuySlot = Card->AddChildToCanvas(Buy);
	BuySlot->SetPosition(FVector2D(22.0f, 242.0f));
	BuySlot->SetSize(FVector2D(156.0f, 44.0f));
}

void UReEchoInventoryShopWidget::AddTargetCardPack(UHorizontalBox* Row,
                                                   const FReEchoShopCardPackOffer& Pack,
                                                   const int32 PackIndex)
{
	if (!Row)
	{
		return;
	}
	USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(
	    USizeBox::StaticClass(), *FString::Printf(TEXT("TargetCardPackSize%d"), PackIndex));
	CardSize->SetWidthOverride(200.0f);
	CardSize->SetHeightOverride(292.0f);
	UHorizontalBoxSlot* RowSlot = Row->AddChildToHorizontalBox(CardSize);
	RowSlot->SetPadding(FMargin(4.0f, 0.0f, 25.0f, 0.0f));

	UCanvasPanel* Card = WidgetTree->ConstructWidget<UCanvasPanel>(
	    UCanvasPanel::StaticClass(), *FString::Printf(TEXT("TargetCardPack%d"), PackIndex));
	CardSize->SetContent(Card);
	UImage* Base = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
	                                                   *FString::Printf(TEXT("TargetCardPackBase%d"), PackIndex));
	Base->SetBrushFromTexture(ShopItemCardTexture, true);
	Base->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* BaseSlot = Card->AddChildToCanvas(Base);
	BaseSlot->SetPosition(FVector2D::ZeroVector);
	BaseSlot->SetSize(FVector2D(200.0f, 292.0f));

	// Intentionally no card icon: this is a tier pack entrance. A future authored pack image can occupy this area.
	UTextBlock* TierText = CreateText(
	    WidgetTree, *FString::Printf(TEXT("TargetCardPackTier%d"), PackIndex), 34, FLinearColor(0.08f, 0.08f, 0.08f));
	TierText->SetText(Pack.DisplayName);
	TierText->SetJustification(ETextJustify::Center);
	UCanvasPanelSlot* TierSlot = Card->AddChildToCanvas(TierText);
	TierSlot->SetPosition(FVector2D(20.0f, 66.0f));
	TierSlot->SetSize(FVector2D(160.0f, 58.0f));

	UTextBlock* RemainingText = CreateText(WidgetTree,
	                                       *FString::Printf(TEXT("TargetCardPackRemaining%d"), PackIndex),
	                                       17,
	                                       FLinearColor(0.12f, 0.12f, 0.12f));
	RemainingText->SetText(Pack.Status == EReEchoShopCardPackStatus::Available
	                           ? FText::Format(NSLOCTEXT("ReEcho", "ShopCardPackRemaining", "剩余候选 {0}"),
	                                           FText::AsNumber(Pack.Choices.Num()))
	                           : Pack.StatusText);
	RemainingText->SetJustification(ETextJustify::Center);
	UCanvasPanelSlot* RemainingSlot = Card->AddChildToCanvas(RemainingText);
	RemainingSlot->SetPosition(FVector2D(14.0f, 160.0f));
	RemainingSlot->SetSize(FVector2D(172.0f, 44.0f));

	UReEchoIndexedButton* Button = WidgetTree->ConstructWidget<UReEchoIndexedButton>(
	    UReEchoIndexedButton::StaticClass(), *FString::Printf(TEXT("TargetCardPackButton%d"), PackIndex));
	Button->SetEntryIndex(PackIndex);
	const bool bPendingChoice = Pack.Status == EReEchoShopCardPackStatus::PaidPendingChoice;
	const bool bCanPurchase = Pack.bCanPurchase;
	Button->SetBackgroundColor((bCanPurchase || bPendingChoice) ? FLinearColor(0.95f, 0.78f, 0.34f, 1.0f)
	                                                            : FLinearColor(0.55f, 0.55f, 0.55f, 1.0f));
	Button->SetIsEnabled(bCanPurchase || bPendingChoice);
	Button->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleCardPackClicked);
	UTextBlock* ButtonText = CreateText(WidgetTree,
	                                    *FString::Printf(TEXT("TargetCardPackButtonText%d"), PackIndex),
	                                    18,
	                                    FLinearColor(0.03f, 0.03f, 0.03f));
	ButtonText->SetText(Pack.IsAvailable() ? FText::Format(NSLOCTEXT("ReEcho", "ShopCardPackBuy", "购买 · {0}"),
	                                                       FText::AsNumber(Pack.EffectivePrice))
	                    : bPendingChoice   ? NSLOCTEXT("ReEcho", "ShopCardPackContinue", "继续选择")
	                                       : Pack.StatusText);
	ButtonText->SetJustification(ETextJustify::Center);
	Button->SetContent(ButtonText);
	UCanvasPanelSlot* ButtonSlot = Card->AddChildToCanvas(Button);
	ButtonSlot->SetPosition(FVector2D(22.0f, 242.0f));
	ButtonSlot->SetSize(FVector2D(156.0f, 44.0f));
}

void UReEchoInventoryShopWidget::RebuildTargetOfferRows()
{
	if (DesignerPartOfferCards.Num() == 3 && DesignerPackOfferCards.Num() == 3)
	{
		RefreshAuthoredOfferCards();
		return;
	}
	if (!TargetPartOfferRow || !TargetCardOfferRow)
	{
		return;
	}
	TargetPartOfferRow->ClearChildren();
	TargetCardOfferRow->ClearChildren();
	for (int32 Index = 0; Index < VisibleWeaponPartOffers.Num(); ++Index)
	{
		AddTargetOfferCard(TargetPartOfferRow, VisibleWeaponPartOffers[Index], Index, true);
	}
	for (int32 Index = 0; Index < CurrentPartShopView.CardPackOffers.Num(); ++Index)
	{
		AddTargetCardPack(TargetCardOfferRow, CurrentPartShopView.CardPackOffers[Index], Index);
	}
}

void UReEchoInventoryShopWidget::RefreshAuthoredOfferCards()
{
	const auto SetBuyState = [](UReEchoIndexedButton* Button,
	                            UImage* Art,
	                            UTextBlock* Label,
	                            const bool bEnabled,
	                            const FText& OverrideLabel)
	{
		if (Button)
		{
			Button->SetIsEnabled(bEnabled);
		}
		const bool bUseOverride = !OverrideLabel.IsEmpty();
		if (Art)
		{
			Art->SetVisibility(bUseOverride ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
			Art->SetColorAndOpacity(bEnabled ? FLinearColor::White : FLinearColor(0.58f, 0.58f, 0.58f, 1.0f));
		}
		if (Label)
		{
			Label->SetText(OverrideLabel);
			Label->SetVisibility(bUseOverride ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	};

	for (int32 Index = 0; Index < DesignerPartOfferCards.Num(); ++Index)
	{
		UCanvasPanel* Card = DesignerPartOfferCards[Index];
		const bool bHasOffer = VisibleWeaponPartOffers.IsValidIndex(Index);
		if (Card)
		{
			Card->SetVisibility(bHasOffer ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
			Card->SetToolTip(nullptr);
		}
		if (!bHasOffer)
		{
			continue;
		}

		const FReEchoShopOffer& Offer = VisibleWeaponPartOffers[Index];
		const bool bOwnedPart = CurrentPartShopView.OwnedParts.ContainsByPredicate(
		    [&](const FReEchoShopOffer& Owned)
		    {
			    return Owned.ContentId == Offer.ContentId;
		    });
		const bool bOwnedWeapon =
		    Offer.Type == EReEchoShopOfferType::Weapon && CurrentPartShopView.OwnedWeapons.Contains(Offer.ContentId);
		const bool bOwned = bOwnedPart || bOwnedWeapon;
		const bool bEquipped = IsPartEquipped(Offer.ContentId);
		const int32 EffectivePrice = Offer.EffectivePrice;
		const bool bCanBuy = !bOwned && Offer.bCanPurchase;
		if (Card)
		{
			Card->SetToolTip(BuildSlotTooltip(Offer));
		}
		if (DesignerPartOfferIcons.IsValidIndex(Index) && DesignerPartOfferIcons[Index])
		{
			UTexture2D* OfferIcon = Offer.Type == EReEchoShopOfferType::Weapon ? ShopAttachmentSlotTexture.Get()
			                                                                   : ResolveWeaponPartIcon(Offer.ContentId);
			if (Offer.Type == EReEchoShopOfferType::Weapon && !Offer.IconTexturePath.IsEmpty())
			{
				if (UTexture2D* WeaponIcon = LoadObject<UTexture2D>(nullptr, *Offer.IconTexturePath))
				{
					OfferIcon = WeaponIcon;
				}
			}
			DesignerPartOfferIcons[Index]->SetBrushFromTexture(OfferIcon, false);
			DesignerPartOfferIcons[Index]->SetColorAndOpacity(FLinearColor::White);
		}
		if (DesignerPartOfferDescriptions.IsValidIndex(Index) && DesignerPartOfferDescriptions[Index])
		{
			// The offer card only carries the rune name; the full effect remains in its tooltip.
			DesignerPartOfferDescriptions[Index]->SetText(Offer.DisplayName);
		}
		if (DesignerPartOfferCosts.IsValidIndex(Index) && DesignerPartOfferCosts[Index])
		{
			DesignerPartOfferCosts[Index]->SetText(FText::AsNumber(EffectivePrice));
		}
		SetBuyState(DesignerPartOfferBuyButtons.IsValidIndex(Index) ? DesignerPartOfferBuyButtons[Index] : nullptr,
		            DesignerPartOfferBuyArts.IsValidIndex(Index) ? DesignerPartOfferBuyArts[Index] : nullptr,
		            DesignerPartOfferBuyLabels.IsValidIndex(Index) ? DesignerPartOfferBuyLabels[Index] : nullptr,
		            bCanBuy,
		            bOwned ? (bEquipped ? FText::FromString(TEXT("已装备")) : FText::FromString(TEXT("已获得")))
		                   : FText::GetEmpty());
	}

	for (int32 Index = 0; Index < DesignerPackOfferCards.Num(); ++Index)
	{
		UCanvasPanel* Card = DesignerPackOfferCards[Index];
		const bool bHasPack = CurrentPartShopView.CardPackOffers.IsValidIndex(Index);
		if (Card)
		{
			Card->SetVisibility(bHasPack ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (!bHasPack)
		{
			continue;
		}

		const FReEchoShopCardPackOffer& Pack = CurrentPartShopView.CardPackOffers[Index];
		const bool bPendingChoice = Pack.Status == EReEchoShopCardPackStatus::PaidPendingChoice;
		const int32 EffectivePrice = Pack.EffectivePrice;
		const bool bCanPurchase = Pack.bCanPurchase && bCurrentExtraCardPurchaseAllowed;
		if (DesignerPackOfferIcons.IsValidIndex(Index) && DesignerPackOfferIcons[Index])
		{
			DesignerPackOfferIcons[Index]->SetBrushFromTexture(ShopEmptyCardSlotIconTexture.Get(), false);
			DesignerPackOfferIcons[Index]->SetColorAndOpacity(FLinearColor::White);
			DesignerPackOfferIcons[Index]->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (DesignerPackOfferDescriptions.IsValidIndex(Index) && DesignerPackOfferDescriptions[Index])
		{
			DesignerPackOfferDescriptions[Index]->SetText(
			    FText::Format(NSLOCTEXT("ReEcho", "AuthoredPackTierOnly", "{0}卡组"), Pack.DisplayName));
		}
		if (DesignerPackOfferCosts.IsValidIndex(Index) && DesignerPackOfferCosts[Index])
		{
			DesignerPackOfferCosts[Index]->SetText(FText::AsNumber(EffectivePrice));
		}
		const FText OverrideLabel = bPendingChoice        ? NSLOCTEXT("ReEcho", "ShopCardPackContinue", "继续选择")
		                            : !Pack.IsAvailable() ? Pack.StatusText
		                                                  : FText::GetEmpty();
		SetBuyState(DesignerPackOfferBuyButtons.IsValidIndex(Index) ? DesignerPackOfferBuyButtons[Index] : nullptr,
		            DesignerPackOfferBuyArts.IsValidIndex(Index) ? DesignerPackOfferBuyArts[Index] : nullptr,
		            DesignerPackOfferBuyLabels.IsValidIndex(Index) ? DesignerPackOfferBuyLabels[Index] : nullptr,
		            bCanPurchase || bPendingChoice,
		            OverrideLabel);
	}
}

bool UReEchoInventoryShopWidget::HasEchoStorageCard() const
{
	return CurrentPartShopView.OwnedCards.ContainsByPredicate(
	    [](const FReEchoShopOffer& Card)
	    {
		    return Card.ContentId == FName(ReEchoEchoStorage::StorageUnlockCardId);
	    });
}

void UReEchoInventoryShopWidget::RebuildOwnedCardSlots()
{
	if (DesignerCardSlotButtons.IsEmpty() || DesignerCardSlotArts.IsEmpty())
	{
		return;
	}
	DisplayedOwnedCards.Reset();
	for (const FReEchoShopOffer& Card : CurrentPartShopView.OwnedCards)
	{
		if (DisplayedOwnedCards.Num() >= DesignerCardSlotButtons.Num())
		{
			break;
		}
		DisplayedOwnedCards.Add(Card);
	}
	const FReEchoShopOffer* StorageCard = CurrentPartShopView.OwnedCards.FindByPredicate(
	    [](const FReEchoShopOffer& Card)
	    {
		    return Card.ContentId == FName(ReEchoEchoStorage::StorageUnlockCardId);
	    });
	if (StorageCard && !DisplayedOwnedCards.ContainsByPredicate(
	                       [&](const FReEchoShopOffer& Card)
	                       {
		                       return Card.ContentId == StorageCard->ContentId;
	                       }))
	{
		if (DisplayedOwnedCards.IsEmpty())
		{
			DisplayedOwnedCards.Add(*StorageCard);
		}
		else
		{
			DisplayedOwnedCards.Last() = *StorageCard;
		}
	}
	const int32 VisibleCount = DisplayedOwnedCards.Num();
	const int32 SlotCount = FMath::Min(DesignerCardSlotButtons.Num(), DesignerCardSlotArts.Num());
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UButton* CardSlotButton = DesignerCardSlotButtons[Index];
		UImage* SlotImage = DesignerCardSlotArts[Index];
		if (!CardSlotButton || !SlotImage)
		{
			continue;
		}
		CardSlotButton->OnClicked.Clear();
		CardSlotButton->SetToolTip(nullptr);
		CardSlotButton->SetVisibility(ESlateVisibility::Visible);
		UTexture2D* CardTexture = Index < VisibleCount ? ShopCardIconTexture.Get() : nullptr;
		if (Index < VisibleCount && !DisplayedOwnedCards[Index].IconTexturePath.IsEmpty())
		{
			if (UTexture2D* LoadedCardTexture =
			        LoadObject<UTexture2D>(nullptr, *DisplayedOwnedCards[Index].IconTexturePath))
			{
				CardTexture = LoadedCardTexture;
			}
		}
		SlotImage->SetBrushFromTexture(CardTexture, false);
		SlotImage->SetColorAndOpacity(FLinearColor::White);
		SlotImage->SetVisibility(Index < VisibleCount ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		if (Index < VisibleCount)
		{
			const bool bStorageCard =
			    DisplayedOwnedCards[Index].ContentId == FName(ReEchoEchoStorage::StorageUnlockCardId);
			CardSlotButton->SetToolTip(BuildSlotTooltip(DisplayedOwnedCards[Index]));
			if (bStorageCard)
			{
				CardSlotButton->OnClicked.AddUniqueDynamic(
				    this, &UReEchoInventoryShopWidget::HandleEchoStorageCardSlotClicked);
			}
		}
	}
}

void UReEchoInventoryShopWidget::RebuildAttachmentHoverSlots()
{
	// 按固定槽位顺序（Slots 已排序：核心在最左）填充 UI 左中右槽位，避免随装备插入顺序变化。
	TArray<const FReEchoShopOffer*> DisplayedAttachmentParts;
	DisplayedAttachmentParts.SetNumZeroed(DesignerAttachmentSlotButtons.Num());
	for (int32 SlotIndex = 0;
	     SlotIndex < CurrentPartShopView.Slots.Num() && SlotIndex < DesignerAttachmentSlotButtons.Num();
	     ++SlotIndex)
	{
		const FName SlotTypeId = CurrentPartShopView.Slots[SlotIndex].SlotTypeId;
		const FReEchoEquippedPartSnapshot* Equipped = CurrentPartShopView.EquippedParts.FindByPredicate(
		    [&](const FReEchoEquippedPartSnapshot& E)
		    {
			    return E.SlotTypeId == SlotTypeId;
		    });
		if (!Equipped)
		{
			continue;
		}
		const FReEchoShopOffer* Part = CurrentPartShopView.OwnedParts.FindByPredicate(
		    [&](const FReEchoShopOffer& Candidate)
		    {
			    return Candidate.ContentId == Equipped->PartId;
		    });
		if (!Part)
		{
			// 刚购买即装备的符文尚未进入 OwnedParts 快照，回落到投放槽报价中取展示信息。
			Part = VisibleWeaponPartOffers.FindByPredicate(
			    [&](const FReEchoShopOffer& Candidate)
			    {
				    return Candidate.ContentId == Equipped->PartId;
			    });
		}
		if (Part)
		{
			DisplayedAttachmentParts[SlotIndex] = Part;
		}
	}

	const int32 SlotCount = FMath::Min(DesignerAttachmentSlotButtons.Num(), DesignerAttachmentSlotArts.Num());
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UButton* HoverButton = DesignerAttachmentSlotButtons[Index];
		UImage* AttachmentArt = DesignerAttachmentSlotArts[Index];
		const bool bHasPart =
		    DisplayedAttachmentParts.IsValidIndex(Index) && DisplayedAttachmentParts[Index] != nullptr;
		ApplyPersistentSlotFrame(HoverButton, ShopAttachmentSlotTexture.Get());
		if (AttachmentArt)
		{
			// Keep the slot frame in the Button style and layer only the equipped rune in its child art.
			// This prevents the runtime icon from replacing the authored socket frame.
			if (bHasPart)
			{
				AttachmentArt->SetBrushFromTexture(ResolveWeaponPartIcon(DisplayedAttachmentParts[Index]->ContentId),
				                                   false);
			}
			AttachmentArt->SetColorAndOpacity(FLinearColor::White);
			AttachmentArt->SetVisibility(bHasPart ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		}
		if (!HoverButton)
		{
			continue;
		}
		HoverButton->SetVisibility(ESlateVisibility::Visible);
		HoverButton->SetToolTip(nullptr);
		if (bHasPart)
		{
			HoverButton->SetToolTip(BuildSlotTooltip(*DisplayedAttachmentParts[Index]));
		}
	}

	// 给每个槽位按钮绑无参点击处理（点击弹出对应槽位的背包）。
	if (DesignerAttachmentSlotButtons.IsValidIndex(0) && DesignerAttachmentSlotButtons[0])
	{
		DesignerAttachmentSlotButtons[0]->OnClicked.RemoveDynamic(
		    this, &UReEchoInventoryShopWidget::HandleAttachmentSlot0Clicked);
		DesignerAttachmentSlotButtons[0]->OnClicked.AddDynamic(
		    this, &UReEchoInventoryShopWidget::HandleAttachmentSlot0Clicked);
	}
	if (DesignerAttachmentSlotButtons.IsValidIndex(1) && DesignerAttachmentSlotButtons[1])
	{
		DesignerAttachmentSlotButtons[1]->OnClicked.RemoveDynamic(
		    this, &UReEchoInventoryShopWidget::HandleAttachmentSlot1Clicked);
		DesignerAttachmentSlotButtons[1]->OnClicked.AddDynamic(
		    this, &UReEchoInventoryShopWidget::HandleAttachmentSlot1Clicked);
	}
	if (DesignerAttachmentSlotButtons.IsValidIndex(2) && DesignerAttachmentSlotButtons[2])
	{
		DesignerAttachmentSlotButtons[2]->OnClicked.RemoveDynamic(
		    this, &UReEchoInventoryShopWidget::HandleAttachmentSlot2Clicked);
		DesignerAttachmentSlotButtons[2]->OnClicked.AddDynamic(
		    this, &UReEchoInventoryShopWidget::HandleAttachmentSlot2Clicked);
	}
}

void UReEchoInventoryShopWidget::RebuildEquippedWeaponDisplay()
{
	if (!DesignerEquippedWeaponButton || !DesignerEquippedWeaponArt)
	{
		return;
	}
	const bool bHasWeapon = !CurrentPartShopView.WeaponId.IsNone();
	DesignerEquippedWeaponButton->SetVisibility(bHasWeapon ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	DesignerEquippedWeaponButton->SetToolTip(nullptr);
	if (!bHasWeapon)
	{
		return;
	}

	UTexture2D* WeaponTexture = nullptr;
	if (!CurrentPartShopView.WeaponIconTexturePath.IsEmpty())
	{
		WeaponTexture = LoadObject<UTexture2D>(nullptr, *CurrentPartShopView.WeaponIconTexturePath);
	}
	DesignerEquippedWeaponArt->SetBrushFromTexture(WeaponTexture ? WeaponTexture : ShopCardIconTexture.Get(), true);
	DesignerEquippedWeaponArt->SetColorAndOpacity(FLinearColor::White);
	if (const FReEchoShopOffer* CurrentWeapon = CurrentPartShopView.OwnedWeaponOffers.FindByPredicate(
	        [&](const FReEchoShopOffer& Candidate)
	        {
		        return Candidate.ContentId == CurrentPartShopView.WeaponId;
	        }))
	{
		DesignerEquippedWeaponButton->SetToolTip(BuildSlotTooltip(*CurrentWeapon));
	}
}

void UReEchoInventoryShopWidget::HandleAttachmentSlot0Clicked()
{
	HandleAttachmentSlotClicked(0);
}

void UReEchoInventoryShopWidget::HandleAttachmentSlot1Clicked()
{
	HandleAttachmentSlotClicked(1);
}

void UReEchoInventoryShopWidget::HandleAttachmentSlot2Clicked()
{
	HandleAttachmentSlotClicked(2);
}

FName UReEchoInventoryShopWidget::GetSlotTypeIdForIndex(int32 SlotIndex) const
{
	// UI 左中右槽位与 Slots 固定顺序一一对应（核心最左），点击打开背包时按同一映射取槽位类型，避免随装备插入顺序错位。
	if (CurrentPartShopView.Slots.IsValidIndex(SlotIndex))
	{
		return CurrentPartShopView.Slots[SlotIndex].SlotTypeId;
	}
	return NAME_None;
}

const FReEchoShopOffer* UReEchoInventoryShopWidget::FindOwnedPartByContentId(const FName ContentId) const
{
	return CurrentPartShopView.OwnedParts.FindByPredicate(
	    [&](const FReEchoShopOffer& Candidate)
	    {
		    return Candidate.ContentId == ContentId;
	    });
}

UCanvasPanel* UReEchoInventoryShopWidget::EnsureBackpackPopupLayer()
{
	if (BackpackPopupLayer)
	{
		return BackpackPopupLayer.Get();
	}
	UCanvasPanel* RootCanvas = GetLayoutCanvas();
	if (!RootCanvas || !WidgetTree)
	{
		return nullptr;
	}
	BackpackPopupLayer =
	    WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BackpackPopupLayer"));
	BackpackPopupLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UCanvasPanelSlot* LayerSlot = RootCanvas->AddChildToCanvas(BackpackPopupLayer);
	LayerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	LayerSlot->SetOffsets(FMargin(0.0f));
	LayerSlot->SetZOrder(BackpackPopupLayerZOrder);
	return BackpackPopupLayer.Get();
}

FVector2D UReEchoInventoryShopWidget::ResolveBackpackPopupPosition(const UWidget* AnchorWidget,
                                                                   const FVector2D& PopupSize,
                                                                   const FVector2D& FallbackPosition) const
{
	const UCanvasPanel* RootCanvas = GetLayoutCanvas();
	if (!RootCanvas || !AnchorWidget)
	{
		return FallbackPosition;
	}
	const FGeometry& RootGeometry = RootCanvas->GetCachedGeometry();
	const FGeometry& AnchorGeometry = AnchorWidget->GetCachedGeometry();
	const FVector2D RootSize = RootGeometry.GetLocalSize();
	const FVector2D AnchorSize = AnchorGeometry.GetLocalSize();
	if (RootSize.IsNearlyZero() || AnchorSize.IsNearlyZero())
	{
		return FallbackPosition;
	}

	constexpr float PopupMargin = 16.0f;
	const FVector2D AnchorTopLeft = RootGeometry.AbsoluteToLocal(AnchorGeometry.LocalToAbsolute(FVector2D::ZeroVector));
	FVector2D Position(AnchorTopLeft.X + AnchorSize.X + PopupMargin, AnchorTopLeft.Y);
	if (Position.X + PopupSize.X > RootSize.X - PopupMargin)
	{
		Position.X = AnchorTopLeft.X - PopupSize.X - PopupMargin;
	}
	Position.X = FMath::Clamp(Position.X, PopupMargin, FMath::Max(PopupMargin, RootSize.X - PopupSize.X - PopupMargin));
	Position.Y = FMath::Clamp(Position.Y, PopupMargin, FMath::Max(PopupMargin, RootSize.Y - PopupSize.Y - PopupMargin));
	return Position;
}

void UReEchoInventoryShopWidget::HandleAttachmentSlotClicked(const int32 SlotIndex)
{
	HideWeaponBackpackPopup();
	const FName SlotTypeId = GetSlotTypeIdForIndex(SlotIndex);
	if (SlotTypeId.IsNone())
	{
		return;
	}
	// 统计该槽位下“拥有但未装备”的配件数量；为空则无背包可弹。
	const bool bHasBackpack = CurrentPartShopView.OwnedParts.ContainsByPredicate(
	    [&](const FReEchoShopOffer& Candidate)
	    {
		    return Candidate.SlotTypeId == SlotTypeId && IsPartCompatibleWithCurrentWeapon(Candidate) &&
		           !IsPartEquipped(Candidate.ContentId);
	    });
	if (!bHasBackpack)
	{
		HideBackpackPopup();
		return;
	}
	if (ActiveBackpackSlotIndex == SlotIndex && BackpackPopupPanel)
	{
		HideBackpackPopup();
		return;
	}
	BuildBackpackPopup(SlotIndex);
}

void UReEchoInventoryShopWidget::HideBackpackPopup()
{
	if (BackpackPopupPanel)
	{
		BackpackPopupPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	ActiveBackpackSlotIndex = INDEX_NONE;
	CachedBackpackItemIds.Reset();
}

void UReEchoInventoryShopWidget::HandleEquippedWeaponClicked()
{
	HideBackpackPopup();
	if (WeaponBackpackPopupPanel && WeaponBackpackPopupPanel->GetVisibility() == ESlateVisibility::Visible)
	{
		HideWeaponBackpackPopup();
		return;
	}
	BuildWeaponBackpackPopup();
}

void UReEchoInventoryShopWidget::HideWeaponBackpackPopup()
{
	if (WeaponBackpackPopupPanel)
	{
		WeaponBackpackPopupPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	CachedWeaponBackpackIds.Reset();
}

void UReEchoInventoryShopWidget::HandleWeaponBackpackItemClicked(const int32 ItemIndex)
{
	if (CachedWeaponBackpackIds.IsValidIndex(ItemIndex) &&
	    CachedWeaponBackpackIds[ItemIndex] != CurrentPartShopView.WeaponId)
	{
		OnWeaponEquipRequested.Broadcast(CachedWeaponBackpackIds[ItemIndex]);
	}
}

void UReEchoInventoryShopWidget::BuildWeaponBackpackPopup()
{
	UCanvasPanel* PopupLayer = EnsureBackpackPopupLayer();
	if (!PopupLayer || CurrentPartShopView.OwnedWeaponOffers.IsEmpty())
	{
		return;
	}
	if (!WeaponBackpackPopupPanel)
	{
		WeaponBackpackPopupPanel =
		    WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WeaponBackpackPopupPanel"));
		PopupLayer->AddChildToCanvas(WeaponBackpackPopupPanel);
	}
	if (UCanvasPanelSlot* PopupSlot = Cast<UCanvasPanelSlot>(WeaponBackpackPopupPanel->Slot))
	{
		PopupSlot->SetPosition(ResolveBackpackPopupPosition(
		    DesignerEquippedWeaponButton, WeaponBackpackPopupSize, FVector2D(1040.0f, 180.0f)));
		PopupSlot->SetSize(WeaponBackpackPopupSize);
		PopupSlot->SetZOrder(1);
	}
	WeaponBackpackPopupPanel->ClearChildren();
	CachedWeaponBackpackIds.Reset();

	UBorder* Surface = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WeaponBackpackSurface"));
	Surface->SetBrushColor(FLinearColor(0.04f, 0.04f, 0.04f, 0.96f));
	Surface->SetPadding(FMargin(14.0f));
	UCanvasPanelSlot* SurfaceSlot = WeaponBackpackPopupPanel->AddChildToCanvas(Surface);
	SurfaceSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	SurfaceSlot->SetOffsets(FMargin(0.0f));

	UScrollBox* Scroll =
	    WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("WeaponBackpackScroll"));
	Scroll->SetScrollBarVisibility(ESlateVisibility::Visible);
	UVerticalBox* List =
	    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WeaponBackpackList"));
	Scroll->AddChild(List);
	Surface->SetContent(Scroll);

	UTextBlock* Title = CreateText(WidgetTree, TEXT("WeaponBackpackTitle"), 20, FLinearColor::White);
	Title->SetText(NSLOCTEXT("ReEcho", "WeaponBackpackTitle", "武器背包"));
	UVerticalBoxSlot* TitleSlot = List->AddChildToVerticalBox(Title);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

	for (const FReEchoShopOffer& Weapon : CurrentPartShopView.OwnedWeaponOffers)
	{
		const int32 EntryIndex = CachedWeaponBackpackIds.Add(Weapon.ContentId);
		UReEchoIndexedButton* Button = WidgetTree->ConstructWidget<UReEchoIndexedButton>(
		    UReEchoIndexedButton::StaticClass(), *FString::Printf(TEXT("WeaponBackpackItem%d"), EntryIndex));
		Button->SetEntryIndex(EntryIndex);
		Button->SetIsEnabled(Weapon.ContentId != CurrentPartShopView.WeaponId);
		Button->SetToolTip(BuildSlotTooltip(Weapon));
		Button->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoInventoryShopWidget::HandleWeaponBackpackItemClicked);

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
		    UHorizontalBox::StaticClass(), *FString::Printf(TEXT("WeaponBackpackRow%d"), EntryIndex));
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
		                                                   *FString::Printf(TEXT("WeaponBackpackIcon%d"), EntryIndex));
		UTexture2D* Texture =
		    Weapon.IconTexturePath.IsEmpty() ? nullptr : LoadObject<UTexture2D>(nullptr, *Weapon.IconTexturePath);
		Icon->SetBrushFromTexture(Texture ? Texture : ShopCardIconTexture.Get(), false);
		Icon->SetDesiredSizeOverride(FVector2D(72.0f, 72.0f));
		UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(Icon);
		IconSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		IconSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Label = CreateText(
		    WidgetTree, *FString::Printf(TEXT("WeaponBackpackLabel%d"), EntryIndex), 17, FLinearColor::White);
		Label->SetText(
		    Weapon.ContentId == CurrentPartShopView.WeaponId
		        ? FText::Format(NSLOCTEXT("ReEcho", "WeaponBackpackEquipped", "{0}（已装备）"), Weapon.DisplayName)
		        : Weapon.DisplayName);
		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		Button->SetContent(Row);
		UVerticalBoxSlot* ButtonSlot = List->AddChildToVerticalBox(Button);
		ButtonSlot->SetPadding(FMargin(0.0f, 3.0f));
	}
	WeaponBackpackPopupPanel->SetVisibility(ESlateVisibility::Visible);
}

void UReEchoInventoryShopWidget::HandleBackpackItemClicked(const int32 ItemIndex)
{
	if (!CachedBackpackItemIds.IsValidIndex(ItemIndex))
	{
		return;
	}
	OnPurchaseRequested.Broadcast(CachedBackpackItemIds[ItemIndex]);
}

void UReEchoInventoryShopWidget::BuildBackpackPopup(const int32 SlotIndex)
{
	UCanvasPanel* PopupLayer = EnsureBackpackPopupLayer();
	if (!PopupLayer)
	{
		return;
	}
	const FName SlotTypeId = GetSlotTypeIdForIndex(SlotIndex);
	if (SlotTypeId.IsNone())
	{
		return;
	}

	if (!BackpackPopupPanel)
	{
		BackpackPopupPanel =
		    WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BackpackPopupPanel"));
		PopupLayer->AddChildToCanvas(BackpackPopupPanel);
	}
	if (UCanvasPanelSlot* PopupSlot = Cast<UCanvasPanelSlot>(BackpackPopupPanel->Slot))
	{
		const UWidget* AnchorWidget = DesignerAttachmentSlotButtons.IsValidIndex(SlotIndex)
		                                  ? DesignerAttachmentSlotButtons[SlotIndex].Get()
		                                  : nullptr;
		PopupSlot->SetPosition(
		    ResolveBackpackPopupPosition(AnchorWidget, RuneBackpackPopupSize, FVector2D(1120.0f, 360.0f)));
		PopupSlot->SetSize(RuneBackpackPopupSize);
		PopupSlot->SetZOrder(1);
	}
	BackpackPopupPanel->ClearChildren();

	UBorder* Surface = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackpackPopupSurface"));
	Surface->SetBrushColor(FLinearColor(0.04f, 0.04f, 0.04f, 0.96f));
	Surface->SetPadding(FMargin(14.0f));
	UCanvasPanelSlot* SurfaceSlot = BackpackPopupPanel->AddChildToCanvas(Surface);
	SurfaceSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	SurfaceSlot->SetOffsets(FMargin(0.0f));

	UScrollBox* Scroll =
	    WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("BackpackPopupScroll"));
	Scroll->SetScrollBarVisibility(ESlateVisibility::Visible);
	UVerticalBox* PopupList =
	    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BackpackPopupList"));
	Scroll->AddChild(PopupList);
	Surface->SetContent(Scroll);

	UTextBlock* Title = CreateText(WidgetTree, TEXT("BackpackPopupTitle"), 20, FLinearColor::White);
	Title->SetText(NSLOCTEXT("ReEcho", "RuneBackpackTitle", "符文背包"));
	Title->SetAutoWrapText(false);
	UVerticalBoxSlot* TitleSlot = PopupList->AddChildToVerticalBox(Title);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

	TArray<FName> BackpackItemIds;
	int32 EntryCount = 0;
	for (const FReEchoShopOffer& Candidate : CurrentPartShopView.OwnedParts)
	{
		if (Candidate.SlotTypeId != SlotTypeId || !IsPartCompatibleWithCurrentWeapon(Candidate) ||
		    IsPartEquipped(Candidate.ContentId))
		{
			continue;
		}
		BackpackItemIds.Add(Candidate.ItemId);
		const int32 ThisIndex = EntryCount++;
		UReEchoIndexedButton* ItemButton = WidgetTree->ConstructWidget<UReEchoIndexedButton>(
		    UReEchoIndexedButton::StaticClass(), *FString::Printf(TEXT("BackpackItem%d"), ThisIndex));
		ItemButton->SetEntryIndex(ThisIndex);
		ItemButton->SetToolTip(BuildSlotTooltip(Candidate));
		UHorizontalBox* ItemRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		    UHorizontalBox::StaticClass(), *FString::Printf(TEXT("BackpackItemRow%d"), ThisIndex));
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
		                                                   *FString::Printf(TEXT("BackpackItemIcon%d"), ThisIndex));
		Icon->SetBrushFromTexture(ResolveWeaponPartIcon(Candidate.ContentId), false);
		Icon->SetBrushTintColor(FLinearColor::White);
		Icon->SetDesiredSizeOverride(FVector2D(72.0f, 72.0f));
		UHorizontalBoxSlot* IconSlot = ItemRow->AddChildToHorizontalBox(Icon);
		IconSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		IconSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* NameText =
		    CreateText(WidgetTree, *FString::Printf(TEXT("BackpackItemName%d"), ThisIndex), 17, FLinearColor::White);
		NameText->SetText(Candidate.DisplayName);
		NameText->SetAutoWrapText(false);
		UHorizontalBoxSlot* NameSlot = ItemRow->AddChildToHorizontalBox(NameText);
		NameSlot->SetVerticalAlignment(VAlign_Center);
		ItemButton->SetContent(ItemRow);
		UVerticalBoxSlot* ButtonSlot = PopupList->AddChildToVerticalBox(ItemButton);
		ButtonSlot->SetPadding(FMargin(0.0f, 3.0f));
		ItemButton->OnIndexedClicked.RemoveDynamic(this, &UReEchoInventoryShopWidget::HandleBackpackItemClicked);
		ItemButton->OnIndexedClicked.AddDynamic(this, &UReEchoInventoryShopWidget::HandleBackpackItemClicked);
	}
	CachedBackpackItemIds = MoveTemp(BackpackItemIds);

	if (EntryCount == 0)
	{
		HideBackpackPopup();
		return;
	}

	BackpackPopupPanel->SetVisibility(ESlateVisibility::Visible);
	ActiveBackpackSlotIndex = SlotIndex;
}

UTexture2D* UReEchoInventoryShopWidget::ResolveWeaponPartIcon(const FName PartId) const
{
	if (const TObjectPtr<UTexture2D>* Icon = WeaponPartIconTextures.Find(PartId))
	{
		if (Icon->Get())
		{
			return Icon->Get();
		}
	}
	// 动态加载：按 PartId 在 Icons 目录查找纹理（覆盖硬编码 map 之外的所有武器符文）
	const FString IconPath =
	    FString::Printf(TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_%s.T_UI_Part_%s"),
	                    *PartId.ToString(),
	                    *PartId.ToString());
	if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *IconPath))
	{
		return Tex;
	}
	// Plan76 兼容回退：部分符文的纹理按开普勒源行命名（T_UI_Part_P_AUDIT_C_<SrcRow>），
	// 与 parts.csv 的 PartId 命名不一致，动态加载 T_UI_Part_<PartId> 会落空。这里复用源行命名资产。
	static const TMap<FName, FString> LegacyPartIconPaths = {
	    {TEXT("P_CORE_PRIMORDIAL"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_2.T_UI_Part_P_AUDIT_C_2")},
	    {TEXT("P_CORE_TIDE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_3.T_UI_Part_P_AUDIT_C_3")},
	    {TEXT("P_CORE_FOREST"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_4.T_UI_Part_P_AUDIT_C_4")},
	    {TEXT("P_CORE_FLAME"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_5.T_UI_Part_P_AUDIT_C_5")},
	    {TEXT("P_CORE_THUNDER"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_6.T_UI_Part_P_AUDIT_C_6")},
	    {TEXT("P_CORE_PRISM"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_7.T_UI_Part_P_AUDIT_C_7")},
	    {TEXT("P_BOW_SPLIT_ARROWHEAD"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_16.T_UI_Part_P_AUDIT_C_16")},
	    {TEXT("P_BOW_EXPLOSIVE_ARROWHEAD"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_17.T_UI_Part_P_AUDIT_C_17")},
	    {TEXT("P_BOW_PIERCING_ARROWHEAD"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_18.T_UI_Part_P_AUDIT_C_18")},
	    {TEXT("P_BOW_CRITBLEED_ARROWHEAD"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_19.T_UI_Part_P_AUDIT_C_19")},
	    {TEXT("P_BOW_MULTISHOT_ARROWHEAD"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_20.T_UI_Part_P_AUDIT_C_20")},
	    {TEXT("P_BOW_KILLSHARD_ARROWHEAD"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_21.T_UI_Part_P_AUDIT_C_21")},
	    {TEXT("P_BOW_HASTE_BOWSTRING"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_22.T_UI_Part_P_AUDIT_C_22")},
	    {TEXT("P_BOW_HEAVY_BOWSTRING"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_23.T_UI_Part_P_AUDIT_C_23")},
	    {TEXT("P_BOW_KILLHASTE_BOWSTRING"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_24.T_UI_Part_P_AUDIT_C_24")},
	    {TEXT("P_BOW_COMBOHASTE_BOWSTRING"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_25.T_UI_Part_P_AUDIT_C_25")},
	    {TEXT("P_SCYTHE_GROUPGROWTH_ROTARYBLADE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_26.T_UI_Part_P_AUDIT_C_26")},
	    {TEXT("P_SCYTHE_LIFESTEAL_ROTARYBLADE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_27.T_UI_Part_P_AUDIT_C_27")},
	    {TEXT("P_SCYTHE_HASTE_ROTARYBLADE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_28.T_UI_Part_P_AUDIT_C_28")},
	    {TEXT("P_SCYTHE_STUN_ROTARYBLADE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_29.T_UI_Part_P_AUDIT_C_29")},
	    {TEXT("P_SCYTHE_BLEED_ROTARYBLADE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_30.T_UI_Part_P_AUDIT_C_30")},
	    {TEXT("P_SCYTHE_OUTERRING_ROTARYBLADE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_31.T_UI_Part_P_AUDIT_C_31")},
	    {TEXT("P_SCYTHE_MOVESTACK_GRIP"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_32.T_UI_Part_P_AUDIT_C_32")},
	    {TEXT("P_SCYTHE_GROUPINVULN_GRIP"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_33.T_UI_Part_P_AUDIT_C_33")},
	    {TEXT("P_SCYTHE_ATTACKSTACK_GRIP"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_34.T_UI_Part_P_AUDIT_C_34")},
	    {TEXT("P_SCYTHE_THROWRECALL_GRIP"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_35.T_UI_Part_P_AUDIT_C_35")},
	    {TEXT("P_LONGSWORD_NARROWWIDE_SWORDBLADE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_36.T_UI_Part_P_AUDIT_C_36")},
	    {TEXT("P_LONGSWORD_SLOWWIDE_SWORDBLADE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_37.T_UI_Part_P_AUDIT_C_37")},
	    {TEXT("P_LONGSWORD_GROUPGROWTH_SWORDBLADE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_38.T_UI_Part_P_AUDIT_C_38")},
	    {TEXT("P_LONGSWORD_KILLHEAL_SWORDBLADE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_39.T_UI_Part_P_AUDIT_C_39")},
	    {TEXT("P_LONGSWORD_HITSHARD_SWORDBLADE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_40.T_UI_Part_P_AUDIT_C_40")},
	    {TEXT("P_LONGSWORD_CRITBLEED_SWORDBLADE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_41.T_UI_Part_P_AUDIT_C_41")},
	    {TEXT("P_LONGSWORD_METEOR_SWORDBLADE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_42.T_UI_Part_P_AUDIT_C_42")},
	    {TEXT("P_LONGSWORD_HASTE_GRIP"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_43.T_UI_Part_P_AUDIT_C_43")},
	    {TEXT("P_LONGSWORD_MOVESTACK_GRIP"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_44.T_UI_Part_P_AUDIT_C_44")},
	    {TEXT("P_LONGSWORD_ATTACKSTACK_GRIP"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_45.T_UI_Part_P_AUDIT_C_45")},
	    {TEXT("P_LONGSWORD_STUN_GRIP"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_46.T_UI_Part_P_AUDIT_C_46")},
	    {TEXT("P_LONGSWORD_HEAVY_GRIP"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_47.T_UI_Part_P_AUDIT_C_47")},
	    {TEXT("P_GUN_TRIPLESPREAD_MUZZLE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_48.T_UI_Part_P_AUDIT_C_48")},
	    {TEXT("P_GUN_CHARGED_MUZZLE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_49.T_UI_Part_P_AUDIT_C_49")},
	    {TEXT("P_GUN_EXPLOSIVE_MUZZLE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_51.T_UI_Part_P_AUDIT_C_51")},
	    {TEXT("P_GUN_PIERCING_MUZZLE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_52.T_UI_Part_P_AUDIT_C_52")},
	    {TEXT("P_GUN_BLEED_MUZZLE"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_53.T_UI_Part_P_AUDIT_C_53")},
	    {TEXT("P_GUN_LIFESTEAL_GUNACTION"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_54.T_UI_Part_P_AUDIT_C_54")},
	    {TEXT("P_GUN_HITSHARD_GUNACTION"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_55.T_UI_Part_P_AUDIT_C_55")},
	    {TEXT("P_GUN_ATTACKSTACK_GUNACTION"),
	     TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_AUDIT_C_56.T_UI_Part_P_AUDIT_C_56")},
	};
	if (const FString* LegacyPath = LegacyPartIconPaths.Find(PartId))
	{
		if (UTexture2D* LegacyTex = LoadObject<UTexture2D>(nullptr, *LegacyPath))
		{
			return LegacyTex;
		}
	}
	return ShopAttachmentSlotTexture.Get();
}

UWidget* UReEchoInventoryShopWidget::BuildSlotTooltip(const FReEchoShopOffer& Offer)
{
	UVerticalBox* TooltipStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), NAME_None);
	auto AddTooltipPanel = [&](const FText& Title, const FText& Body, const bool bOutcome)
	{
		USizeBox* TooltipSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), NAME_None);
		TooltipSize->SetWidthOverride(280.0f);
		UBorder* TooltipFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), NAME_None);
		TooltipFrame->SetBrushColor(bOutcome ? FLinearColor(0.96f, 0.80f, 0.34f, 1.0f) : FLinearColor::White);
		TooltipFrame->SetPadding(FMargin(3.0f));
		UBorder* TooltipSurface = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), NAME_None);
		TooltipSurface->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.97f));
		TooltipSurface->SetPadding(FMargin(14.0f, 11.0f));
		UVerticalBox* TooltipContent =
		    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), NAME_None);
		UTextBlock* TooltipTitle = CreateText(WidgetTree,
		                                      NAME_None,
		                                      bOutcome ? 17 : 19,
		                                      bOutcome ? FLinearColor(0.96f, 0.80f, 0.34f) : FLinearColor::White);
		TooltipTitle->SetText(Title);
		TooltipTitle->SetJustification(ETextJustify::Center);
		TooltipTitle->SetAutoWrapText(false);
		UVerticalBoxSlot* TitleSlot = TooltipContent->AddChildToVerticalBox(TooltipTitle);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		UTextBlock* TooltipEffect = CreateText(WidgetTree, NAME_None, 16, FLinearColor::White);
		TooltipEffect->SetText(Body);
		TooltipEffect->SetJustification(ETextJustify::Center);
		TooltipEffect->SetAutoWrapText(true);
		TooltipContent->AddChildToVerticalBox(TooltipEffect);
		TooltipSurface->SetContent(TooltipContent);
		TooltipFrame->SetContent(TooltipSurface);
		TooltipSize->SetContent(TooltipFrame);
		UVerticalBoxSlot* PanelSlot = TooltipStack->AddChildToVerticalBox(TooltipSize);
		PanelSlot->SetPadding(bOutcome ? FMargin(0.0f, 3.0f, 0.0f, 0.0f) : FMargin(0.0f));
	};

	AddTooltipPanel(Offer.DisplayName, Offer.EffectText, false);
	if (!Offer.OutcomeText.IsEmpty())
	{
		AddTooltipPanel(NSLOCTEXT("ReEcho", "ResolvedCardOutcomeTitle", "实际效果"), Offer.OutcomeText, true);
	}
	return TooltipStack;
}

void UReEchoInventoryShopWidget::SetPlayerStats(const FReEchoStatBlock& Stats)
{
	CachedPlayerStats = Stats;
	UE_LOG(LogReEcho,
	       Log,
	       TEXT("[AttrPanel] SetPlayerStats called; HpMax=%.1f Atk=%.1f"),
	       Stats.HpMax,
	       Stats.PhysicalAttack);
	// DesignerShopClock is a designer-authored widget whose concrete type is not guaranteed to be UImage
	// (it is typically wrapped in a UBorder/UOverlay/UButton or is a UUserWidget). Attach the tooltip to
	// the widget itself so hover works regardless of its concrete type.
	if (UWidget* Clock = GetWidgetFromName(TEXT("DesignerShopClock")))
	{
		UE_LOG(LogReEcho,
		       Log,
		       TEXT("[AttrPanel] DesignerShopClock FOUND type=%s vis=%d"),
		       *Clock->GetClass()->GetName(),
		       (int32)Clock->GetVisibility());
		// The designer authored this image as HitTestInvisible (vis=3), which renders it but makes it
		// ignore mouse hit-testing, so Slate never fires OnMouseEnter and the tooltip never appears.
		// Switch to Visible so the tooltip triggers on hover. Rendering is unchanged.
		if (Clock->GetVisibility() != ESlateVisibility::Visible)
		{
			Clock->SetVisibility(ESlateVisibility::Visible);
			UE_LOG(LogReEcho, Log, TEXT("[AttrPanel] DesignerShopClock visibility forced to Visible for hover"));
		}
		UWidget* Panel = BuildAttributePanel(Stats);
		UE_LOG(LogReEcho,
		       Log,
		       TEXT("[AttrPanel] BuildAttributePanel returned %s; attaching tooltip"),
		       Panel ? TEXT("valid") : TEXT("NULL"));
		Clock->SetToolTip(Panel);
	}
	else
	{
		UE_LOG(LogReEcho, Warning, TEXT("[AttrPanel] DesignerShopClock NOT FOUND (GetWidgetFromName returned null)"));
	}
}

UWidget* UReEchoInventoryShopWidget::BuildAttributePanel(const FReEchoStatBlock& Stats) const
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	UE_LOG(LogReEcho,
	       Log,
	       TEXT("[AttrPanel] BuildAttributePanel Snapshot=%s"),
	       Snapshot.IsValid() ? TEXT("valid") : TEXT("NULL"));
	USizeBox* TooltipSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), NAME_None);
	TooltipSize->SetWidthOverride(320.0f);
	UBorder* TooltipFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), NAME_None);
	TooltipFrame->SetBrushColor(FLinearColor::White);
	TooltipFrame->SetPadding(FMargin(3.0f));
	UBorder* TooltipSurface = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), NAME_None);
	TooltipSurface->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.97f));
	TooltipSurface->SetPadding(FMargin(14.0f, 11.0f));
	UVerticalBox* TooltipContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), NAME_None);

	if (Snapshot)
	{
		UE_LOG(LogReEcho, Log, TEXT("[AttrPanel] AttributeOrder count=%d"), Snapshot->GetAttributeOrder().Num());
		for (const FName& AttrId : Snapshot->GetAttributeOrder())
		{
			const FReEchoCsvAttributeRow* Attr = Snapshot->FindAttribute(AttrId);
			if (!Attr)
			{
				continue;
			}
			const float Raw = GetAttributeRawValue(Attr->Id, Stats);
			const FString ValueText = Attr->ValueKind == FName(TEXT("Percent"))
			                              ? FString::FromInt(FMath::RoundToInt(Raw * 100.0f)) + TEXT("%")
			                              : FString::FromInt(FMath::RoundToInt(Raw));
			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), NAME_None);
			UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), NAME_None);
			const FString IconPath =
			    FString::Printf(TEXT("/Game/ReEcho/Textures/UI/Attributes/%s.%s"), *Attr->IconName, *Attr->IconName);
			if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *IconPath))
			{
				Icon->SetBrushFromTexture(Tex);
			}
			Icon->SetDesiredSizeOverride(FVector2D(28.0f, 28.0f));
			UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(Icon);
			IconSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
			IconSlot->SetVerticalAlignment(VAlign_Center);
			UTextBlock* Name = CreateText(WidgetTree, NAME_None, 16, FLinearColor::White);
			Name->SetText(FText::FromString(FString::Printf(TEXT("%s：%s"), *Attr->DisplayName, *ValueText)));
			Name->SetJustification(ETextJustify::Left);
			UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(Name);
			NameSlot->SetVerticalAlignment(VAlign_Center);
			TooltipContent->AddChildToVerticalBox(Row);
		}
	}
	TooltipSurface->SetContent(TooltipContent);
	TooltipFrame->SetContent(TooltipSurface);
	TooltipSize->SetContent(TooltipFrame);
	return TooltipSize;
}

void UReEchoInventoryShopWidget::Refresh()
{
	if (!InventoryPanel || !ShopPanel || !InventoryText || !CurrencyText)
	{
		return;
	}

	HideBackpackPopup();
	HideWeaponBackpackPopup();

	BuildOfferEntries();
	BuildLoadoutEntries();
	BuildTargetShopPresentation();
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
	for (int32 PackIndex = 0; PackIndex < CurrentPartShopView.CardPackOffers.Num(); ++PackIndex)
	{
		if (!CardPackButtons.IsValidIndex(PackIndex) || !CardPackTexts.IsValidIndex(PackIndex))
		{
			continue;
		}
		const FReEchoShopCardPackOffer& Pack = CurrentPartShopView.CardPackOffers[PackIndex];
		const bool bPendingChoice = Pack.Status == EReEchoShopCardPackStatus::PaidPendingChoice;
		const bool bCanPurchase = Pack.bCanPurchase;
		CardPackButtons[PackIndex]->SetIsEnabled(bCanPurchase || bPendingChoice);
		const FText ActionText = Pack.IsAvailable()
		                             ? FText::Format(NSLOCTEXT("ReEcho", "ShopCardPackBuy", "购买 · {0}"),
		                                             FText::AsNumber(Pack.EffectivePrice))
		                         : bPendingChoice ? NSLOCTEXT("ReEcho", "ShopCardPackContinue", "继续选择")
		                                          : Pack.StatusText;
		CardPackTexts[PackIndex]->SetText(FText::Format(
		    NSLOCTEXT("ReEcho", "ShopCardPackLogicFormat", "{0}卡组\n{1}"), Pack.DisplayName, ActionText));
	}

	InventoryPanel->SetVisibility(bShowingShop ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	ShopPanel->SetVisibility(bShowingShop ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (ShopLogicScrollBox)
	{
		ShopLogicScrollBox->SetVisibility(ShopPresentationLayer ? ESlateVisibility::Collapsed
		                                  : bShowingShop        ? ESlateVisibility::Visible
		                                                        : ESlateVisibility::Collapsed);
		UpdateShopLogicViewportBounds();
	}
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

	CurrencyText->SetText(FText::Format(NSLOCTEXT("ReEcho", "ShopCurrency", "时间碎片  {0}"),
	                                    FText::AsNumber(GetDisplayedTimeShardBalance())));
	for (int32 OfferIndex = 0; OfferIndex < VisibleRunItemOffers.Num(); ++OfferIndex)
	{
		const FReEchoShopOffer& Offer = VisibleRunItemOffers[OfferIndex];
		const bool bAvailable = !Offer.ItemId.IsNone();
		const bool bOwned = CurrentOwnedItems.Contains(Offer.ItemId);
		OfferButtons[OfferIndex]->SetIsEnabled(bAvailable && !bOwned && Offer.bCanPurchase);
		OfferTexts[OfferIndex]->SetText(FText::Format(
		    NSLOCTEXT("ReEcho", "ShopOfferFormat", "{0}\n{1}\n{2}"),
		    Offer.DisplayName,
		    Offer.EffectText,
		    !bAvailable ? Offer.DisplayName
		    : bOwned
		        ? NSLOCTEXT("ReEcho", "ShopOwned", "已获得")
		        : FText::Format(NSLOCTEXT("ReEcho", "ShopPrice", "{0} 碎片"), FText::AsNumber(Offer.EffectivePrice))));
	}
	if (ShopRefreshButton && ShopRefreshText)
	{
		const bool bCanRefresh = bCurrentShopRefreshAllowed && CurrentPartShopView.bWeaponRuneRefreshAllowed;
		ShopRefreshButton->SetIsEnabled(bCanRefresh);
		const FText PaidRemaining = CurrentPartShopView.bWeaponRuneRefreshUnlimited
		                                ? NSLOCTEXT("ReEcho", "ShopRefreshUnlimited", "∞")
		                                : FText::AsNumber(CurrentPartShopView.WeaponRuneRefreshesRemaining);
		ShopRefreshText->SetText(
		    CurrentFreeShopRefreshes > 0
		        ? FText::Format(NSLOCTEXT("ReEcho", "ShopRefreshWithFree", "刷新（免费 {0} / 付费 {1}）"),
		                        FText::AsNumber(CurrentFreeShopRefreshes),
		                        PaidRemaining)
		        : FText::Format(NSLOCTEXT("ReEcho", "ShopRefreshCounted", "刷新（剩余 {0}） · {1}"),
		                        PaidRemaining,
		                        FText::AsNumber(CurrentPartShopView.WeaponRuneRefreshCost)));
	}
	if (TargetRefreshLimitText)
	{
		const FText PaidRemaining = CurrentPartShopView.bWeaponRuneRefreshUnlimited
		                                ? NSLOCTEXT("ReEcho", "TargetRefreshUnlimited", "∞")
		                                : FText::AsNumber(CurrentPartShopView.WeaponRuneRefreshesRemaining);
		TargetRefreshLimitText->SetText(
		    CurrentFreeShopRefreshes > 0
		        ? FText::Format(NSLOCTEXT("ReEcho", "TargetRefreshWithFree", "武器/符文刷新：免费 {0} / 付费 {1}"),
		                        FText::AsNumber(CurrentFreeShopRefreshes),
		                        PaidRemaining)
		        : FText::Format(NSLOCTEXT("ReEcho", "TargetRefreshCounted", "武器/符文刷新：剩余 {0} · {1} 碎片"),
		                        PaidRemaining,
		                        FText::AsNumber(CurrentPartShopView.WeaponRuneRefreshCost)));
	}
	if (ShopRuleText)
	{
		ShopRuleText->SetText(FText::Format(NSLOCTEXT("ReEcho", "ShopCardGroupRule", "商店折扣 {0}%　额外卡牌组：{1}"),
		                                    FText::AsNumber(FMath::RoundToInt(CurrentShopDiscount * 100.0f)),
		                                    bCurrentExtraCardPurchaseAllowed
		                                        ? NSLOCTEXT("ReEcho", "ShopCardGroupAllowed", "可购买")
		                                        : NSLOCTEXT("ReEcho", "ShopCardGroupDisabled", "已被永久代价禁用")));
	}
	if (WeaponLoadoutPanel && WeaponLoadoutText)
	{
		const bool bShowWeaponBlocks = bShowingShop && !CurrentPartShopView.WeaponId.IsNone();
		WeaponLoadoutPanel->SetVisibility(bShowWeaponBlocks ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		WeaponPartOfferPanel->SetVisibility(bShowWeaponBlocks && !VisibleWeaponPartOffers.IsEmpty()
		                                        ? ESlateVisibility::Visible
		                                        : ESlateVisibility::Collapsed);
		UpdateWeaponLoadoutText();
		for (int32 Index = 0; Index < VisibleWeaponPartOffers.Num(); ++Index)
		{
			const FReEchoShopOffer& PartOffer = VisibleWeaponPartOffers[Index];
			const bool bOwned = CurrentPartShopView.OwnedParts.ContainsByPredicate(
			    [&](const FReEchoShopOffer& Owned)
			    {
				    return Owned.ContentId == PartOffer.ContentId;
			    });
			const bool bEquipped = IsPartEquipped(PartOffer.ContentId);
			WeaponPartOfferButtons[Index]->SetIsEnabled(!bOwned && PartOffer.bCanPurchase);
			WeaponPartOfferButtons[Index]->SetBackgroundColor(bEquipped ? FLinearColor(0.2f, 0.55f, 0.25f, 0.95f)
			                                                            : FLinearColor(0.15f, 0.11f, 0.07f, 0.88f));
			WeaponPartOfferTexts[Index]->SetText(
			    FText::Format(NSLOCTEXT("ReEcho", "WeaponPartOfferFormat", "{0} · {1}{2}"),
			                  PartOffer.DisplayName,
			                  FText::FromName(PartOffer.SlotTypeId),
			                  bEquipped ? NSLOCTEXT("ReEcho", "WeaponPartEquipped", "（已装备）")
			                  : bOwned  ? NSLOCTEXT("ReEcho", "WeaponPartOwned", "（已获得）")
			                            : FText::Format(NSLOCTEXT("ReEcho", "WeaponPartPrice", "（{0} 碎片）"),
                                                       FText::AsNumber(PartOffer.EffectivePrice))));
		}
	}
	if (ShopPresentationLayer)
	{
		ShopPresentationLayer->SetVisibility(bShowingShop ? ESlateVisibility::SelfHitTestInvisible
		                                                  : ESlateVisibility::Collapsed);
		if (TargetCurrencyText)
		{
			TargetCurrencyText->SetText(FText::Format(NSLOCTEXT("ReEcho", "TargetShopCurrency", "时间碎片：{0}"),
			                                          FText::AsNumber(GetDisplayedTimeShardBalance())));
		}
		CurrencyText->SetVisibility(ESlateVisibility::Collapsed);
		if (ShopRefreshButton)
		{
			ShopRefreshButton->SetIsEnabled(bCurrentShopRefreshAllowed &&
			                                CurrentPartShopView.bWeaponRuneRefreshAllowed);
		}
		if (RunItemOfferPanel)
		{
			RunItemOfferPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (WeaponPartOfferPanel)
		{
			WeaponPartOfferPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (ShopControlPanel)
		{
			ShopControlPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (WeaponLoadoutPanel)
		{
			WeaponLoadoutPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
		RebuildTargetOfferRows();
		RebuildOwnedCardSlots();
		RebuildAttachmentHoverSlots();
		RebuildEquippedWeaponDisplay();
	}
	if (DesignerLoadoutCanvas)
	{
		DesignerLoadoutCanvas->SetVisibility(bShowingShop ? ESlateVisibility::SelfHitTestInvisible
		                                                  : ESlateVisibility::Collapsed);
	}
}

void UReEchoInventoryShopWidget::RequestPurchase(const int32 OfferIndex)
{
	if (VisibleRunItemOffers.IsValidIndex(OfferIndex) && !VisibleRunItemOffers[OfferIndex].ItemId.IsNone())
	{
		OnPurchaseRequested.Broadcast(VisibleRunItemOffers[OfferIndex].ItemId);
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

void UReEchoInventoryShopWidget::HandleCardPackClicked(const int32 PackIndex)
{
	if (!CurrentPartShopView.CardPackOffers.IsValidIndex(PackIndex))
	{
		ReEchoUIInteractionAudit::Write(
		    TEXT("CARD_PACK_BUTTON_REJECTED"),
		    FString::Printf(TEXT("screen=InventoryShop packIndex=%d reason=InvalidIndex packCount=%d"),
		                    PackIndex,
		                    CurrentPartShopView.CardPackOffers.Num()));
		return;
	}
	const FReEchoShopCardPackOffer& Pack = CurrentPartShopView.CardPackOffers[PackIndex];
	ReEchoUIInteractionAudit::Write(
	    TEXT("CARD_PACK_BUTTON_COMMAND"),
	    FString::Printf(TEXT("screen=InventoryShop packIndex=%d tier=%d status=%d candidates=%d extraAllowed=%d"),
	                    PackIndex,
	                    Pack.Tier,
	                    static_cast<int32>(Pack.Status),
	                    Pack.Choices.Num(),
	                    bCurrentExtraCardPurchaseAllowed ? 1 : 0));
	const bool bPendingChoice = Pack.Status == EReEchoShopCardPackStatus::PaidPendingChoice;
	const bool bCanPurchase = Pack.bCanPurchase;
	if (bCanPurchase || bPendingChoice)
	{
		OnCardPackRequested.Broadcast(Pack.Tier);
		return;
	}
	ReEchoUIInteractionAudit::Write(
	    TEXT("CARD_PACK_BUTTON_REJECTED"),
	    FString::Printf(TEXT("screen=InventoryShop packIndex=%d tier=%d reason=%s"),
	                    PackIndex,
	                    Pack.Tier,
	                    Pack.IsAvailable() ? TEXT("PackPurchaseBlocked") : TEXT("PackUnavailable")));
}

void UReEchoInventoryShopWidget::HandleRefreshClicked()
{
	ReEchoUIInteractionAudit::Write(
	    TEXT("WEAPON_RUNE_REFRESH_BUTTON"),
	    FString::Printf(TEXT("screen=InventoryShop remaining=%d cost=%d enabled=%d shards=%d"),
	                    CurrentPartShopView.WeaponRuneRefreshesRemaining,
	                    CurrentPartShopView.WeaponRuneRefreshCost,
	                    CurrentPartShopView.bWeaponRuneRefreshAllowed ? 1 : 0,
	                    CurrentTimeShards));
	OnRefreshRequested.Broadcast();
}

bool UReEchoInventoryShopWidget::IsPartEquipped(const FName PartId) const
{
	return CurrentPartShopView.EquippedParts.ContainsByPredicate(
	    [&](const FReEchoEquippedPartSnapshot& Equipped)
	    {
		    return Equipped.PartId == PartId;
	    });
}

bool UReEchoInventoryShopWidget::IsPartCompatibleWithCurrentWeapon(const FReEchoShopOffer& PartOffer) const
{
	return PartOffer.WeaponTypeId.IsNone() || CurrentPartShopView.WeaponTypeId.IsNone() ||
	       PartOffer.WeaponTypeId == TEXT("Any") || PartOffer.WeaponTypeId == CurrentPartShopView.WeaponTypeId;
}

void UReEchoInventoryShopWidget::HandleWeaponPartOfferClicked(const int32 PartOfferIndex)
{
	if (!VisibleWeaponPartOffers.IsValidIndex(PartOfferIndex))
	{
		return;
	}
	const FReEchoShopOffer& PartOffer = VisibleWeaponPartOffers[PartOfferIndex];
	const bool bAlreadyOwned = CurrentPartShopView.OwnedParts.ContainsByPredicate(
	    [&](const FReEchoShopOffer& Owned)
	    {
		    return Owned.ContentId == PartOffer.ContentId;
	    });
	if (bAlreadyOwned)
	{
		// 购买即装备：已拥有的配件没有二次装配动作。
		return;
	}
	OnPurchaseRequested.Broadcast(PartOffer.ItemId);
}

void UReEchoInventoryShopWidget::HandleEchoStorageCardSlotClicked()
{
	if (Mode != EReEchoInventoryShopMode::PostTraitIntermission || !HasEchoStorageCard())
	{
		return;
	}
	bEchoStoragePopupOpen = true;
	BuildEchoPanel();
}

void UReEchoInventoryShopWidget::HandleEchoPopupCloseClicked()
{
	bEchoStoragePopupOpen = false;
	EchoPendingDecision = EReEchoShopEchoPendingDecision::Undecided;
	BuildEchoPanel();
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
	if (Mode == EReEchoInventoryShopMode::PostTraitIntermission && EchoSummary.bHasPendingRecording)
	{
		if (!HasEchoStorageCard())
		{
			// Permanent storage is a G_3_02 benefit. Without it, keep the rolling
			// latest echo but resolve the permanent-storage decision automatically.
			OnEchoSkipAndCloseRequested.Broadcast();
			return;
		}
		if (CloseConfirmWidget)
		{
			CloseConfirmWidget->SetVisibility(ESlateVisibility::Visible);
			return;
		}
	}
	if (bEchoStoragePopupOpen)
	{
		bEchoStoragePopupOpen = false;
		BuildEchoPanel();
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
	if (Mode != EReEchoInventoryShopMode::PostTraitIntermission || !HasEchoStorageCard() || !bEchoStoragePopupOpen)
	{
		EchoPanel->SetVisibility(ESlateVisibility::Collapsed);
		if (EchoPanelScale)
		{
			EchoPanelScale->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}
	if (EchoPanelScale)
	{
		EchoPanelScale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
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
	if (HasEchoStorageCard())
	{
		OnEchoStoreRequested.Broadcast();
	}
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

void UReEchoInventoryShopWidget::SetTimeShards(int32 NewShards)
{
	CurrentTimeShards = NewShards;
	if (TargetCurrencyText)
	{
		TargetCurrencyText->SetText(FText::Format(NSLOCTEXT("ReEcho", "TargetShopCurrency", "时间碎片：{0}"),
		                                          FText::AsNumber(GetDisplayedTimeShardBalance())));
	}
	if (CurrencyText)
	{
		CurrencyText->SetText(FText::Format(NSLOCTEXT("ReEcho", "ShopCurrency", "时间碎片  {0}"),
		                                    FText::AsNumber(GetDisplayedTimeShardBalance())));
	}
}

void UReEchoInventoryShopWidget::MarkItemPurchased(FName ItemId)
{
	PurchasedItemIds.Add(ItemId);
	if (const FReEchoShopOffer* PurchasedCard = VisibleRunItemOffers.FindByPredicate(
	        [&](const FReEchoShopOffer& Offer)
	        {
		        return Offer.ItemId == ItemId && Offer.Type == EReEchoShopOfferType::BuildCard;
	        }))
	{
		if (!CurrentPartShopView.OwnedCards.ContainsByPredicate(
		        [&](const FReEchoShopOffer& OwnedCard)
		        {
			        return OwnedCard.ContentId == PurchasedCard->ContentId;
		        }))
		{
			FReEchoShopOffer OwnedCard = *PurchasedCard;
			OwnedCard.ItemId = OwnedCard.ContentId;
			CurrentPartShopView.OwnedCards.Add(MoveTemp(OwnedCard));
		}
		RebuildOwnedCardSlots();
	}
	RebuildTargetOfferRows();
	if (bShowingShop)
	{
		if (TargetCurrencyText)
		{
			TargetCurrencyText->SetText(FText::Format(NSLOCTEXT("ReEcho", "TargetShopCurrency", "时间碎片：{0}"),
			                                          FText::AsNumber(GetDisplayedTimeShardBalance())));
		}
	}
	else if (CurrencyText)
	{
		CurrencyText->SetText(FText::Format(NSLOCTEXT("ReEcho", "ShopCurrency", "时间碎片  {0}"),
		                                    FText::AsNumber(GetDisplayedTimeShardBalance())));
	}
}

int32 UReEchoInventoryShopWidget::GetDisplayedTimeShardBalance() const
{
	return bShowingShop ? CurrentTimeShards - FMath::Max(0, CurrentPartShopView.TimeShardDebt) : CurrentTimeShards;
}

void UReEchoInventoryShopWidget::UpdateWeaponLoadoutText()
{
	if (!WeaponLoadoutText)
	{
		return;
	}
	FString LoadoutDescription =
	    FString::Printf(TEXT("装配室 · %s\n"), *CurrentPartShopView.WeaponDisplayName.ToString());
	for (const FReEchoWeaponSlotShopView& SlotView : CurrentPartShopView.Slots)
	{
		TArray<FString> Names;
		for (const FReEchoEquippedPartSnapshot& EquippedPart : CurrentPartShopView.EquippedParts)
		{
			const FReEchoShopOffer* OwnedPart = CurrentPartShopView.OwnedParts.FindByPredicate(
			    [&](const FReEchoShopOffer& Owned)
			    {
				    return Owned.ContentId == EquippedPart.PartId && Owned.SlotTypeId == SlotView.SlotTypeId;
			    });
			if (!OwnedPart)
			{
				// 刚购买即装备的符文尚未进入 OwnedParts 快照，回落到投放槽报价中取展示信息。
				OwnedPart = VisibleWeaponPartOffers.FindByPredicate(
				    [&](const FReEchoShopOffer& Owned)
				    {
					    return Owned.ContentId == EquippedPart.PartId && Owned.SlotTypeId == SlotView.SlotTypeId;
				    });
			}
			if (OwnedPart)
			{
				Names.Add(OwnedPart->DisplayName.ToString());
			}
		}
		LoadoutDescription += FString::Printf(TEXT("%s%s [%d/%d]：%s\n"),
		                                      SlotView.bRequired ? TEXT("必需 ") : TEXT(""),
		                                      *SlotView.DisplayName.ToString(),
		                                      Names.Num(),
		                                      SlotView.Capacity,
		                                      Names.IsEmpty() ? TEXT("未装备") : *FString::Join(Names, TEXT("、")));
	}
	LoadoutDescription += TEXT("\n在“武器配件”区购买配件，购买后立即装备；槽位已满时最早的旧配件回落背包。");
	WeaponLoadoutText->SetText(FText::FromString(LoadoutDescription));
}

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
	UpdateShopLogicViewportBounds();
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
