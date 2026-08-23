#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "Run/ReEchoRunSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoShopPurchaseTest,
                                 "ReEcho.Shop.PurchaseUpdatesInventory",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoShopPurchaseTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->TimeShards = 50;
	const float InitialPhysicalAttack = RunSubsystem->CurrentBuild.Stats.PhysicalAttack;

	TestTrue(TEXT("Known affordable item can be purchased"),
	         RunSubsystem->PurchaseShopItem(TEXT("SHOP_RUSTED_SCISSORS")));
	TestEqual(TEXT("Purchase deducts its exact price"), RunSubsystem->TimeShards, 35);
	TestTrue(TEXT("Purchased item enters inventory"),
	         RunSubsystem->InventoryItems.Contains(TEXT("SHOP_RUSTED_SCISSORS")));
	TestEqual(TEXT("Purchased item applies its build effect"),
	          RunSubsystem->CurrentBuild.Stats.PhysicalAttack,
	          InitialPhysicalAttack + 2.0f);

	TestFalse(TEXT("Owned item cannot be purchased twice"),
	          RunSubsystem->PurchaseShopItem(TEXT("SHOP_RUSTED_SCISSORS")));
	TestEqual(TEXT("Rejected duplicate does not deduct currency"), RunSubsystem->TimeShards, 35);
	TestFalse(TEXT("Unknown item is rejected"), RunSubsystem->PurchaseShopItem(TEXT("SHOP_UNKNOWN")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoPostDrawShopPurchaseTest,
                                 "ReEcho.Shop.PostDrawCurrencyCanPurchase",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoPostDrawShopPurchaseTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_02"));
	RunSubsystem->BeginEncounter();
	RunSubsystem->CompleteEncounter(FReEchoRecording(), true, false);

	const TArray<FReEchoTraitCardOffer> Offers = RunSubsystem->GenerateTraitCardOffers(3);
	if (!TestEqual(TEXT("Post-encounter draw returns three offers"), Offers.Num(), 3))
	{
		return false;
	}
	TestTrue(TEXT("A post-encounter trait can be selected"), RunSubsystem->ApplyTraitCard(Offers[0].CardId));
	TestEqual(TEXT("Completed draw enters planning before the shop"), RunSubsystem->Phase, EReEchoRunPhase::Planning);
	TestEqual(TEXT("Encounter reward provides shop currency"), RunSubsystem->TimeShards, 15);
	TestTrue(TEXT("Post-draw currency can buy the entry-price item"),
	         RunSubsystem->PurchaseShopItem(TEXT("SHOP_RUSTED_SCISSORS")));
	TestEqual(TEXT("Post-draw purchase deducts the available shards"), RunSubsystem->TimeShards, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCardShopRulesTest,
                                 "ReEcho.Shop.CardRulesAreAtomicAndPersistent",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCardShopRulesTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_02"));
	RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_2_16"));
	RunSubsystem->TimeShards = 50;
	const float InitialHpMax = RunSubsystem->CurrentBuild.Stats.HpMax;

	TestEqual(TEXT("Prosperity contract shows the same rounded price that purchase charges"),
	          RunSubsystem->GetDiscountedShopPrice(15),
	          12);
	TestTrue(TEXT("Discounted purchase succeeds"), RunSubsystem->PurchaseShopItem(TEXT("SHOP_RUSTED_SCISSORS")));
	TestEqual(TEXT("Discounted purchase deducts 12 shards"), RunSubsystem->TimeShards, 38);
	TestEqual(TEXT("Successful purchase applies permanent maximum health growth"),
	          RunSubsystem->CurrentBuild.Stats.HpMax,
	          InitialHpMax + 2.0f);

	RunSubsystem->CurrentBuild.CardState.Runtime.FreeShopRefreshes = 1;
	const int32 InitialRefreshSequence = RunSubsystem->CurrentBuild.CardState.Runtime.ShopRefreshSequence;
	TestTrue(TEXT("Free refresh is consumed before currency"), RunSubsystem->TryConsumeShopRefresh(0));
	TestEqual(TEXT("Free refresh leaves currency unchanged"), RunSubsystem->TimeShards, 38);
	TestEqual(TEXT("Free refresh advances the deterministic shop page"),
	          RunSubsystem->CurrentBuild.CardState.Runtime.ShopRefreshSequence,
	          InitialRefreshSequence + 1);
	TestFalse(TEXT("Zero-price refresh cannot become an infinite paid refresh"),
	          RunSubsystem->TryConsumeShopRefresh(0));

	RunSubsystem->CurrentBuild.CardState.Runtime.FreeShopRefreshes = 1;
	RunSubsystem->CurrentBuild.CardState.Runtime.EconomyPenalty = EReEchoCardEconomyPenalty::NoShopRefresh;
	TestFalse(TEXT("Permanent no-refresh penalty blocks even a free refresh"), RunSubsystem->TryConsumeShopRefresh(10));
	TestEqual(TEXT("Blocked refresh does not consume the free count"),
	          RunSubsystem->CurrentBuild.CardState.Runtime.FreeShopRefreshes,
	          1);
	RunSubsystem->CurrentBuild.CardState.Runtime.EconomyPenalty = EReEchoCardEconomyPenalty::NoExtraCardPurchase;
	TestFalse(TEXT("Permanent extra-card penalty closes the purchase gate"), RunSubsystem->CanPurchaseExtraShopCard());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponPartShopLoadoutTest,
                                 "ReEcho.Shop.WeaponPartsPurchaseThenSaveThreeSlotLoadout",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponPartShopLoadoutTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_08"));
	RunSubsystem->TimeShards = 100;

	const FReEchoWeaponPartShopView InitialView = RunSubsystem->GetWeaponPartShopView();
	TestEqual(TEXT("Bow shop exposes three data-driven slot groups"), InitialView.Slots.Num(), 3);
	TestEqual(TEXT("Shop page exposes exactly three weapon-part offers"),
	          InitialView.Offers.FilterByPredicate([](const FReEchoShopOffer& Offer)
	                                               { return Offer.Type == EReEchoShopOfferType::WeaponPart; }).Num(),
	          ReEchoShopOfferCountPerGroup);

	FString Error;
	// Plan 67: committing a loadout equals equipping it; ownership is no longer a
	// precondition. The submit path still rejects invalid (None) part ids.
	TestFalse(TEXT("Invalid (None) part id is rejected by the equip path"),
	          RunSubsystem->TryEquipParts({NAME_None}, Error));
	TestFalse(TEXT("Unknown part id is rejected by the equip path"),
	          RunSubsystem->TryEquipParts({TEXT("P_UNKNOWN_PART")}, Error));
	auto FindAndBuy = [&](const FName PartId)
	{
		for (int32 Attempt = 0; Attempt < 64; ++Attempt)
		{
			const FReEchoWeaponPartShopView Page = RunSubsystem->GetWeaponPartShopView();
			if (Page.Offers.ContainsByPredicate([&](const FReEchoShopOffer& Offer)
			                                    { return Offer.Type == EReEchoShopOfferType::WeaponPart && Offer.ContentId == PartId; }))
			{
				return RunSubsystem->PurchaseShopItem(PartId);
			}
			RunSubsystem->TryConsumeShopRefresh(1);
		}
		return false;
	};
	RunSubsystem->TimeShards = 1000;
	TestTrue(TEXT("Core purchase succeeds when it is on the current page"), FindAndBuy(TEXT("P_CORE_FLAME")));
	TestTrue(TEXT("Arrowhead purchase succeeds when it is on the current page"), FindAndBuy(TEXT("P_BOW_SPLIT_ARROWHEAD")));
	TestTrue(TEXT("Purchased rune enters part ownership"),
	         RunSubsystem->OwnedPartIds.Contains(TEXT("P_CORE_FLAME")));
	TestFalse(TEXT("Purchased rune stays out of ordinary item inventory"),
	          RunSubsystem->InventoryItems.Contains(TEXT("P_CORE_FLAME")));
	TestTrue(TEXT("Purchased runes are equipped immediately"),
	         RunSubsystem->CurrentBuild.EquippedParts.ContainsByPredicate([](const FReEchoEquippedPartSnapshot& Part)
	                                                                    { return Part.PartId == TEXT("P_CORE_FLAME"); }));
	TestTrue(TEXT("Purchased weapon-specific rune is equipped immediately"),
	         RunSubsystem->CurrentBuild.EquippedParts.ContainsByPredicate([](const FReEchoEquippedPartSnapshot& Part)
	                                                                    { return Part.PartId == TEXT("P_BOW_SPLIT_ARROWHEAD"); }));
	const int32 ShardsAfterDuplicate = RunSubsystem->TimeShards;
	TestFalse(TEXT("Duplicate rune purchase is rejected"),
	          RunSubsystem->PurchaseShopItem(TEXT("P_CORE_FLAME")));
	TestEqual(TEXT("Rejected duplicate is atomic"), RunSubsystem->TimeShards, ShardsAfterDuplicate);

	// The public equip operation remains available for owned/backpack selection and idempotent re-commit.
	TestTrue(TEXT("Core and arrowhead equip as one loadout"),
	         RunSubsystem->TryEquipParts(
	             {TEXT("P_CORE_FLAME"), TEXT("P_BOW_SPLIT_ARROWHEAD")}, Error));
	TestEqual(
	    TEXT("Committed loadout contains both equipped slot groups"), RunSubsystem->CurrentBuild.EquippedParts.Num(), 2);
	TestEqual(
	    TEXT("Committed strength grip applies its runtime effect"), RunSubsystem->CurrentBuild.Stats.AttackSpeed, 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCardAndPartShopPageTest,
	                             "ReEcho.Shop.PageUsesFixedPartSlotsAndConfiguredCardTiers",
	                             EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCardAndPartShopPageTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_CAT"), TEXT("W_J_08"));
	RunSubsystem->EncounterIndex = 4;
	RunSubsystem->TimeShards = 200;

	const FReEchoWeaponPartShopView FirstPage = RunSubsystem->GetWeaponPartShopView();
	const TArray<FReEchoShopOffer> FirstCards = FirstPage.Offers.FilterByPredicate(
	    [](const FReEchoShopOffer& Offer) { return Offer.Type == EReEchoShopOfferType::BuildCard; });
	TestEqual(TEXT("First page has three fixed weapon/part slots"), FirstPage.SlotOffers.Num(), ReEchoShopOfferCountPerGroup);
	for (const FReEchoWeaponSlotOffer& Slot : FirstPage.SlotOffers)
	{
		TestTrue(TEXT("Each fixed weapon/part slot contains an offer"), !Slot.ItemId.IsNone());
	}
	TestEqual(TEXT("Encounter four exposes the configured tier-2 and tier-3 card slots"), FirstCards.Num(), 2);
	for (const FReEchoShopOffer& Card : FirstCards)
	{
		TestTrue(TEXT("Card tier follows encounter-four configuration"), Card.Tier == 2 || Card.Tier == 3);
		TestEqual(
		    TEXT("Card offer carries its authored card icon path"),
		    Card.IconTexturePath,
		    FString::Printf(
		        TEXT("/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_%s.T_UI_CardIcon_%s"),
		        *Card.ContentId.ToString(),
		        *Card.ContentId.ToString()));
		const int32 MinPrice = Card.Tier == 2 ? 100 : 150;
		const int32 MaxPrice = Card.Tier == 2 ? 120 : 200;
		TestTrue(TEXT("Card price comes from the configured tier range"), Card.Price >= MinPrice && Card.Price <= MaxPrice);
	}

	const FReEchoShopOffer PurchasedCard = FirstCards[0];
	const int32 OwnedBefore = RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Num();
	const int32 ShardsBefore = RunSubsystem->TimeShards;
	TestTrue(TEXT("Current card offer can be purchased"), RunSubsystem->PurchaseShopItem(PurchasedCard.ItemId));
	TestEqual(TEXT("Card purchase grants one owned-card slot"),
	          RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Num(), OwnedBefore + 1);
	TestTrue(TEXT("Granted card id is the offer content id"),
	         RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Contains(PurchasedCard.ContentId));
	TestEqual(TEXT("Card purchase deducts the tier price"),
	          RunSubsystem->TimeShards, ShardsBefore - PurchasedCard.Price);
	const FReEchoWeaponPartShopView PurchasedPage = RunSubsystem->GetWeaponPartShopView();
	TestTrue(TEXT("Purchased card is projected into the right-side owned slots"),
	         PurchasedPage.OwnedCards.ContainsByPredicate([&](const FReEchoShopOffer& Card)
	                                                   { return Card.ContentId == PurchasedCard.ContentId; }));
	TestTrue(TEXT("Purchased card remains on the same page as a disabled offer"),
	         PurchasedPage.Offers.ContainsByPredicate([&](const FReEchoShopOffer& Offer)
	                                                { return Offer.ItemId == PurchasedCard.ItemId; }));
	TestFalse(TEXT("Same card offer cannot be bought twice on one page"),
	          RunSubsystem->PurchaseShopItem(PurchasedCard.ItemId));

	const int32 SequenceBefore = RunSubsystem->CurrentBuild.CardState.Runtime.ShopRefreshSequence;
	const int32 BeforeRefreshShards = RunSubsystem->TimeShards;
	TestTrue(TEXT("Paid refresh succeeds after free refreshes are exhausted"),
	         RunSubsystem->TryConsumeShopRefresh(ReEchoShopRefreshPrice));
	TestEqual(TEXT("Joint refresh advances one shared sequence"),
	          RunSubsystem->CurrentBuild.CardState.Runtime.ShopRefreshSequence, SequenceBefore + 1);
	TestEqual(TEXT("Paid refresh deducts the configured price"),
	          RunSubsystem->TimeShards, BeforeRefreshShards - ReEchoShopRefreshPrice);
	const FReEchoWeaponPartShopView RefreshedPage = RunSubsystem->GetWeaponPartShopView();
	TestEqual(TEXT("Refreshed page still has three fixed weapon/part slots"),
	          RefreshedPage.SlotOffers.Num(),
	          ReEchoShopOfferCountPerGroup);
	TestEqual(TEXT("Refreshed page keeps the two configured card tiers"),
	          RefreshedPage.Offers.FilterByPredicate([](const FReEchoShopOffer& Offer)
	                                                { return Offer.Type == EReEchoShopOfferType::BuildCard; }).Num(),
	          2);
	return true;
}
#endif
