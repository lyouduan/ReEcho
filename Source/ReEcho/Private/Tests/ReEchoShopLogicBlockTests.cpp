#if WITH_DEV_AUTOMATION_TESTS

#include "Components/Button.h"
#include "Components/VerticalBox.h"
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
	RunItem.Type = EReEchoShopOfferType::RunItem;

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

	TArray<FName> PurchaseRequests;
	Widget->OnPurchaseRequested.AddLambda([&PurchaseRequests](const FName ItemId) { PurchaseRequests.Add(ItemId); });
	UReEchoIndexedButton* RunItemButton =
	    Cast<UReEchoIndexedButton>(Widget->GetWidgetFromName(TEXT("ShopOffer0")));
	UReEchoIndexedButton* WeaponPartButton =
	    Cast<UReEchoIndexedButton>(Widget->GetWidgetFromName(TEXT("WeaponPartOffer0")));
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
	    [&SavedDraft](const TArray<FName>& PartIds) { SavedDraft = PartIds; });
	UButton* SaveButton = Cast<UButton>(Widget->GetWidgetFromName(TEXT("SaveLoadoutButton")));
	TestNotNull(TEXT("Loadout save button exists"), SaveButton);
	if (SaveButton)
	{
		SaveButton->OnClicked.Broadcast();
	}
	TestTrue(TEXT("Saved draft contains the owned part selected from its offer"),
	         SavedDraft.Contains(WeaponPart.ContentId));
	return true;
}

#endif
