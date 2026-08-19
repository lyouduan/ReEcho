#if WITH_DEV_AUTOMATION_TESTS

#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture2D.h"
#include "Misc/AutomationTest.h"
#include "UI/ReEchoIndexedButton.h"
#include "UI/ReEchoInventoryShopWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoShopLogicBlocksTest,
                                 "ReEcho.UI.Shop.LogicBlocks",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoShopLogicBlocksTest::RunTest(const FString& Parameters)
{
	UReEchoInventoryShopWidget* Widget = NewObject<UReEchoInventoryShopWidget>(GetTransientPackage());
	TestNotNull(TEXT("Shop widget can be created"), Widget);
	if (!Widget)
	{
		return false;
	}
	TestTrue(TEXT("Shop widget initializes its widget tree"), Widget->Initialize());
	Widget->TakeWidget();

	FReEchoShopOffer RunItem;
	RunItem.ItemId = TEXT("TEST_RUN_ITEM");
	RunItem.DisplayName = FText::FromString(TEXT("Run item"));
	RunItem.EffectText = FText::FromString(TEXT("Run effect"));
	RunItem.Price = 10;
	RunItem.Type = EReEchoShopOfferType::BuildCard;
	RunItem.ContentId = TEXT("TEST_BUILD_CARD");
	RunItem.Tier = 1;

	FReEchoShopOffer WeaponPart;
	WeaponPart.ItemId = TEXT("TEST_WEAPON_PART_ITEM");
	WeaponPart.DisplayName = FText::FromString(TEXT("Weapon part"));
	WeaponPart.EffectText = FText::FromString(TEXT("Part effect"));
	WeaponPart.Price = 20;
	WeaponPart.Type = EReEchoShopOfferType::WeaponPart;
	WeaponPart.ContentId = TEXT("TEST_WEAPON_PART");
	WeaponPart.SlotTypeId = TEXT("Core");

	FReEchoWeaponPartShopView View;
	View.WeaponId = TEXT("TEST_WEAPON");
	View.WeaponDisplayName = FText::FromString(TEXT("Test weapon"));
	View.Offers = {RunItem, WeaponPart};
	FReEchoWeaponSlotShopView Slot;
	Slot.SlotTypeId = TEXT("Core");
	Slot.DisplayName = FText::FromString(TEXT("Core"));
	Slot.Capacity = 1;
	View.Slots.Add(Slot);

	Widget->SetWeaponPartShopView(View, true);
	Widget->ShowShop(100, {});

	TestNotNull(TEXT("Run item offers have an independent block"),
	            Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("RunItemOfferPanel"))));
	TestNotNull(TEXT("Shop controls have an independent block"),
	            Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("ShopControlPanel"))));
	TestNotNull(TEXT("Weapon part offers have an independent block"),
	            Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("WeaponPartOfferPanel"))));
	TestNotNull(TEXT("Weapon loadout has an independent block"),
	            Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("WeaponLoadoutPanel"))));
	TestNotNull(TEXT("Echo management has an independent block"),
	            Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("EchoPanel"))));
	UScrollBox* ShopScrollBox = Cast<UScrollBox>(Widget->GetWidgetFromName(TEXT("ShopLogicScrollBox")));
	UVerticalBox* ShopLogicPanel = Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("ShopLogicPanel")));
	UVerticalBox* RunItemPanel = Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("RunItemOfferPanel")));
	UVerticalBox* WeaponPartPanel = Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("WeaponPartOfferPanel")));
	UVerticalBox* WeaponLoadoutPanel = Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("WeaponLoadoutPanel")));
	TestNotNull(TEXT("All shop logic blocks have a scrollable host"), ShopScrollBox);
	TestNotNull(TEXT("The scroll box owns a common logic panel"), ShopLogicPanel);
	if (ShopLogicPanel)
	{
		TestTrue(TEXT("Weapon parts are the first visible shop block"),
		         ShopLogicPanel->GetChildrenCount() > 0 && ShopLogicPanel->GetChildAt(0) == WeaponPartPanel);
		TestTrue(TEXT("Run item block is inside the scrollable host"),
		         RunItemPanel && RunItemPanel->GetParent() == ShopLogicPanel);
		TestTrue(TEXT("Weapon part block is inside the scrollable host"),
		         WeaponPartPanel && WeaponPartPanel->GetParent() == ShopLogicPanel);
		TestTrue(TEXT("Weapon loadout block is inside the scrollable host"),
		         WeaponLoadoutPanel && WeaponLoadoutPanel->GetParent() == ShopLogicPanel);
	}

	UScaleBox* EchoPanelScale = Cast<UScaleBox>(Widget->GetWidgetFromName(TEXT("EchoPanelScale")));
	TestNotNull(TEXT("Echo management has an independent popup host"), EchoPanelScale);
	if (EchoPanelScale)
	{
		const UCanvasPanelSlot* EchoCanvasSlot = Cast<UCanvasPanelSlot>(EchoPanelScale->Slot);
		TestNotNull(TEXT("Echo popup is independently anchored on the root canvas"), EchoCanvasSlot);
		if (EchoCanvasSlot)
		{
			TestEqual(TEXT("Echo popup renders over authored shop art"), EchoCanvasSlot->GetZOrder(), 40);
			TestTrue(TEXT("Echo popup is centered instead of occupying the bottom band"),
			         EchoCanvasSlot->GetAnchors().Minimum.Y < 0.30f && EchoCanvasSlot->GetAnchors().Maximum.Y > 0.70f);
		}
		TestEqual(TEXT("Echo popup starts hidden"), EchoPanelScale->GetVisibility(), ESlateVisibility::Collapsed);
	}

	TArray<FName> PurchaseRequests;
	Widget->OnPurchaseRequested.AddLambda(
	    [&PurchaseRequests](const FName ItemId)
	    {
		    PurchaseRequests.Add(ItemId);
	    });
	UReEchoIndexedButton* RunItemButton =
	    Cast<UReEchoIndexedButton>(Widget->GetWidgetFromName(TEXT("TargetBuildBuy0")));
	UReEchoIndexedButton* WeaponPartButton =
	    Cast<UReEchoIndexedButton>(Widget->GetWidgetFromName(TEXT("TargetPartBuy0")));
	TestNotNull(TEXT("Run item button exists"), RunItemButton);
	TestNotNull(TEXT("Weapon part button exists"), WeaponPartButton);
	if (!RunItemButton || !WeaponPartButton)
	{
		return false;
	}
	RunItemButton->OnClicked.Broadcast();
	WeaponPartButton->OnClicked.Broadcast();
	TestEqual(TEXT("Two independent purchase commands are emitted"), PurchaseRequests.Num(), 2);
	if (PurchaseRequests.Num() == 2)
	{
		TestEqual(TEXT("Run item click maps to the run item id"), PurchaseRequests[0], RunItem.ItemId);
		TestEqual(TEXT("Weapon part click maps to the weapon part id"), PurchaseRequests[1], WeaponPart.ItemId);
	}

	View.OwnedParts.Add(WeaponPart);
	Widget->SetWeaponPartShopView(View);
	WeaponPartButton->OnClicked.Broadcast();
	TestEqual(TEXT("Owned part click edits the draft instead of buying again"), PurchaseRequests.Num(), 2);

	TArray<FName> SavedDraft;
	Widget->OnWeaponLoadoutSaveRequested.AddLambda(
	    [&SavedDraft](const TArray<FName>& PartIds)
	    {
		    SavedDraft = PartIds;
	    });
	UButton* SaveButton = Cast<UButton>(Widget->GetWidgetFromName(TEXT("TargetSaveLoadoutButton")));
	TestNotNull(TEXT("Loadout save button exists"), SaveButton);
	if (SaveButton)
	{
		SaveButton->OnClicked.Broadcast();
	}
	TestTrue(TEXT("Saved draft contains the owned part selected from its offer"),
	         SavedDraft.Contains(WeaponPart.ContentId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAuthoredShopLayoutHostTest,
                                 "ReEcho.UI.Shop.AuthoredLayoutHosts",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAuthoredShopLayoutHostTest::RunTest(const FString& Parameters)
{
	UClass* ShopWidgetClass = LoadClass<UReEchoInventoryShopWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen.WBP_ReEchoInventoryShopScreen_C"));
	TestNotNull(TEXT("Authored inventory/shop widget class loads"), ShopWidgetClass);
	if (!ShopWidgetClass)
	{
		return false;
	}

	UReEchoInventoryShopWidget* Widget = NewObject<UReEchoInventoryShopWidget>(GetTransientPackage(), ShopWidgetClass);
	TestNotNull(TEXT("Authored inventory/shop widget can be instantiated"), Widget);
	if (!Widget)
	{
		return false;
	}
	TestTrue(TEXT("Authored inventory/shop widget initializes"), Widget->Initialize());
	Widget->TakeWidget();
	FReEchoShopOffer WeaponPart;
	WeaponPart.ItemId = TEXT("TEST_AUTHORED_WEAPON_PART_ITEM");
	WeaponPart.ContentId = TEXT("P_CORE_TIDE");
	WeaponPart.DisplayName = FText::FromString(TEXT("Authored weapon part"));
	WeaponPart.EffectText = FText::FromString(TEXT("Authored part effect"));
	WeaponPart.Price = 10;
	WeaponPart.Type = EReEchoShopOfferType::WeaponPart;
	WeaponPart.SlotTypeId = TEXT("Core");
	FReEchoWeaponPartShopView PartShopView;
	PartShopView.WeaponId = TEXT("TEST_AUTHORED_WEAPON");
	PartShopView.WeaponDisplayName = FText::FromString(TEXT("Authored weapon"));
	PartShopView.Offers.Add(WeaponPart);
	FReEchoWeaponSlotShopView PartSlot;
	PartSlot.SlotTypeId = TEXT("Core");
	PartSlot.DisplayName = FText::FromString(TEXT("Core"));
	PartSlot.Capacity = 1;
	PartShopView.Slots.Add(PartSlot);
	PartShopView.OwnedParts.Add(WeaponPart);
	FReEchoEquippedPartSnapshot EquippedPart;
	EquippedPart.PartId = WeaponPart.ContentId;
	EquippedPart.SlotTypeId = WeaponPart.SlotTypeId;
	PartShopView.EquippedParts.Add(EquippedPart);
	FReEchoShopOffer StorageCard;
	StorageCard.ItemId = TEXT("SHOP_CARD_G_3_02");
	StorageCard.ContentId = FName(ReEchoEchoStorage::StorageUnlockCardId);
	StorageCard.DisplayName = FText::FromString(TEXT("时空锚点"));
	StorageCard.EffectText = FText::FromString(TEXT("开启回响存储"));
	StorageCard.Type = EReEchoShopOfferType::BuildCard;
	StorageCard.Tier = 3;
	PartShopView.OwnedCards.Add(StorageCard);
	Widget->SetWeaponPartShopView(PartShopView, true);
	UImage* DesignerClock = Cast<UImage>(Widget->GetWidgetFromName(TEXT("DesignerShopClock")));
	UCanvasPanelSlot* DesignerClockSlot = DesignerClock ? Cast<UCanvasPanelSlot>(DesignerClock->Slot) : nullptr;
	TestNotNull(TEXT("Shop clock is authored as a direct Canvas child"), DesignerClockSlot);
	const FVector2D DesignerClockTestPosition(431.0f, 397.0f);
	if (DesignerClockSlot)
	{
		DesignerClockSlot->SetPosition(DesignerClockTestPosition);
	}
	FReEchoEchoStorageSummary EchoSummary;
	EchoSummary.StorageCapacity = 3;
	EchoSummary.bHasPendingRecording = true;
	Widget->ShowPostTraitIntermission(100, {}, EchoSummary);

	UScrollBox* ShopScrollBox = Cast<UScrollBox>(Widget->GetWidgetFromName(TEXT("ShopLogicScrollBox")));
	UScaleBox* EchoPanelScale = Cast<UScaleBox>(Widget->GetWidgetFromName(TEXT("EchoPanelScale")));
	UVerticalBox* EchoPanel = Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("EchoPanel")));
	UVerticalBox* WeaponPartPanel = Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("WeaponPartOfferPanel")));
	TestNotNull(TEXT("Authored shop keeps the legacy scroll host as a hidden compatibility host"), ShopScrollBox);
	TestNotNull(TEXT("Authored shop creates the target presentation layer"),
	            Cast<UCanvasPanel>(Widget->GetWidgetFromName(TEXT("ShopPresentationLayer"))));
	TestNotNull(TEXT("Authored shop creates the three-part target row"),
	            Cast<UHorizontalBox>(Widget->GetWidgetFromName(TEXT("TargetPartOfferRow"))));
	TestNotNull(TEXT("Authored shop creates a purchasable target weapon-part entry"),
	            Widget->GetWidgetFromName(TEXT("TargetPartBuy0")));
	UTexture2D* ExpectedPartIcon = LoadObject<UTexture2D>(
	    nullptr, TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_CORE_TIDE.T_UI_Part_P_CORE_TIDE"));
	UImage* OfferPartIcon = Cast<UImage>(Widget->GetWidgetFromName(TEXT("TargetCardIcon1_0")));
	TestNotNull(TEXT("Mapped weapon-part icon asset loads"), ExpectedPartIcon);
	TestTrue(TEXT("Weapon-part offer uses its PartId icon instead of the attachment placeholder"),
	         OfferPartIcon && OfferPartIcon->GetBrush().GetResourceObject() == ExpectedPartIcon);
	TestTrue(TEXT("Legacy weapon-part block is hidden behind the target composition"),
	         WeaponPartPanel && WeaponPartPanel->GetVisibility() == ESlateVisibility::Collapsed);
	TestNotNull(TEXT("Authored shop receives the independent echo popup"), EchoPanelScale);
	TestTrue(TEXT("Post-trait echo management starts hidden until the storage-card slot is clicked"),
	         EchoPanel && EchoPanel->GetVisibility() == ESlateVisibility::Collapsed);
	TestTrue(TEXT("Runtime refresh preserves the clock's Blueprint-authored position"),
	         DesignerClockSlot && DesignerClockSlot->GetPosition() == DesignerClockTestPosition);
	UImage* DesignerWeaponPanel = Cast<UImage>(Widget->GetWidgetFromName(TEXT("DesignerWeaponPanel")));
	TestNotNull(TEXT("Weapon panel is authored as a direct Canvas child"),
	            DesignerWeaponPanel ? Cast<UCanvasPanelSlot>(DesignerWeaponPanel->Slot) : nullptr);
	UButton* AttachmentHoverSlot = Cast<UButton>(Widget->GetWidgetFromName(TEXT("DesignerAttachmentSlot0")));
	TestNotNull(TEXT("Equipped attachment has a hover target below the weapon"), AttachmentHoverSlot);
	TestTrue(TEXT("Attachment hover uses a custom cursor-following tooltip"),
	         AttachmentHoverSlot && Cast<USizeBox>(AttachmentHoverSlot->GetToolTip()) != nullptr);
	if (AttachmentHoverSlot)
	{
		const USizeBox* TooltipSize = Cast<USizeBox>(AttachmentHoverSlot->GetToolTip());
		const UBorder* TooltipFrame = TooltipSize ? Cast<UBorder>(TooltipSize->GetContent()) : nullptr;
		const UBorder* TooltipSurface = TooltipFrame ? Cast<UBorder>(TooltipFrame->GetContent()) : nullptr;
		const UVerticalBox* TooltipContent =
		    TooltipSurface ? Cast<UVerticalBox>(TooltipSurface->GetContent()) : nullptr;
		const UTextBlock* TooltipEffect = TooltipContent && TooltipContent->GetChildrenCount() > 1
		                                      ? Cast<UTextBlock>(TooltipContent->GetChildAt(1))
		                                      : nullptr;
		TestTrue(TEXT("Attachment tooltip includes its effect explanation"),
		         TooltipEffect && TooltipEffect->GetText().EqualTo(WeaponPart.EffectText));
	}
	TestNull(TEXT("Hover detail does not add another fixed panel over the authored board"),
	         Widget->GetWidgetFromName(TEXT("SlotDetailPanel")));
	UImage* LoadoutStatsBoard = Cast<UImage>(Widget->GetWidgetFromName(TEXT("ArtLoadoutStats")));
	TestNotNull(TEXT("Authored loadout stats board still exists for layout compatibility"), LoadoutStatsBoard);
	TestTrue(TEXT("Large black loadout stats board is hidden behind the card slots"),
	         LoadoutStatsBoard && LoadoutStatsBoard->GetVisibility() == ESlateVisibility::Collapsed);
	UButton* StorageCardSlot = Cast<UButton>(Widget->GetWidgetFromName(TEXT("DesignerCardSlot0")));
	TestNotNull(TEXT("G_3_02 owns a clickable card slot"), StorageCardSlot);
	if (StorageCardSlot)
	{
		TestNotNull(TEXT("Owned card hover uses the same custom cursor-following tooltip"),
		            Cast<USizeBox>(StorageCardSlot->GetToolTip()));
		UImage* CardImage = Cast<UImage>(Widget->GetWidgetFromName(TEXT("DesignerCardSlotArt0")));
		const UCanvasPanelSlot* DesignerCardSlot = Cast<UCanvasPanelSlot>(StorageCardSlot->Slot);
		TestNotNull(TEXT("Owned card slot is directly editable on the designer canvas"), DesignerCardSlot);
		TestNotNull(TEXT("Owned card art remains the authored button content"), CardImage);
		if (DesignerCardSlot)
		{
			TestEqual(TEXT("Owned card slot remains 60 by 60"), DesignerCardSlot->GetSize(), FVector2D(60.0f, 60.0f));
		}
		StorageCardSlot->OnClicked.Broadcast();
	}
	TestTrue(TEXT("Clicking the G_3_02 slot opens echo storage"),
	         EchoPanel && EchoPanel->GetVisibility() == ESlateVisibility::Visible && EchoPanelScale &&
	             EchoPanelScale->GetVisibility() == ESlateVisibility::SelfHitTestInvisible);
	if (ShopScrollBox)
	{
		TestEqual(TEXT("Legacy shop scroll host does not intercept target buttons"),
		          ShopScrollBox->GetVisibility(),
		          ESlateVisibility::Collapsed);
	}
	if (EchoPanelScale)
	{
		const UCanvasPanelSlot* EchoCanvasSlot = Cast<UCanvasPanelSlot>(EchoPanelScale->Slot);
		TestNotNull(TEXT("Authored echo popup is attached to a canvas"), EchoCanvasSlot);
		if (EchoCanvasSlot)
		{
			TestEqual(TEXT("Authored echo popup renders above shop art"), EchoCanvasSlot->GetZOrder(), 40);
			TestTrue(TEXT("Authored echo popup occupies a centered modal region"),
			         EchoCanvasSlot->GetAnchors().Minimum.Y < 0.30f && EchoCanvasSlot->GetAnchors().Maximum.Y > 0.70f);
		}
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UButton* AttachmentSlotButton =
		    Cast<UButton>(Widget->GetWidgetFromName(*FString::Printf(TEXT("DesignerAttachmentSlot%d"), Index)));
		UImage* Attachment =
		    Cast<UImage>(Widget->GetWidgetFromName(*FString::Printf(TEXT("DesignerAttachmentSlotArt%d"), Index)));
		const UCanvasPanelSlot* AttachmentSlot =
		    AttachmentSlotButton ? Cast<UCanvasPanelSlot>(AttachmentSlotButton->Slot) : nullptr;
		TestNotNull(*FString::Printf(TEXT("Attachment slot %d exists in the authored loadout"), Index), AttachmentSlot);
		if (AttachmentSlot)
		{
			TestEqual(*FString::Printf(TEXT("Attachment slot %d keeps its authored size"), Index),
			          AttachmentSlot->GetSize(),
			          FVector2D(93.0f, 93.0f));
		}
		if (Index == 0)
		{
			TestTrue(TEXT("Equipped attachment uses the same mapped PartId icon"),
			         Attachment && Attachment->GetBrush().GetResourceObject() == ExpectedPartIcon);
		}
	}
	return true;
}

#endif
