#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "Run/ReEchoRunSubsystem.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Cards/ReEchoCardRuntime.h"
#include "Cards/ReEchoCardTypes.h"

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
	TestEqual(TEXT("Encounter one has no free card group"), RunSubsystem->Phase, EReEchoRunPhase::Planning);
	RunSubsystem->BeginEncounter();
	RunSubsystem->CompleteEncounter(FReEchoRecording(), true, false);

	const TArray<FReEchoTraitCardOffer> Offers = RunSubsystem->GenerateTraitCardOffers(3);
	if (!TestEqual(TEXT("Post-encounter draw returns three offers"), Offers.Num(), 3))
	{
		return false;
	}
	const FReEchoTraitCardOffer* NonCurrencyResetOffer = Offers.FindByPredicate(
	    [](const FReEchoTraitCardOffer& Offer)
	    {
		    return Offer.CardId != TEXT("G_2_15");
	    });
	if (!TestNotNull(TEXT("The draw includes a trait that does not clear encounter reward currency"),
	                 NonCurrencyResetOffer))
	{
		return false;
	}
	TestTrue(TEXT("A post-encounter trait can be selected"),
	         RunSubsystem->ApplyTraitCard(NonCurrencyResetOffer->CardId));
	TestEqual(TEXT("Completed draw enters planning before the shop"), RunSubsystem->Phase, EReEchoRunPhase::Planning);
	TestEqual(TEXT("Two encounter rewards provide shop currency"), RunSubsystem->TimeShards, 30);
	TestTrue(TEXT("Post-draw currency can buy the entry-price item"),
	         RunSubsystem->PurchaseShopItem(TEXT("SHOP_RUSTED_SCISSORS")));
	TestEqual(TEXT("Post-draw purchase deducts the effective price"),
	          RunSubsystem->TimeShards,
	          30 - RunSubsystem->GetDiscountedShopPrice(15));
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
	TestEqual(TEXT("Shop page exposes exactly three weapon/rune offers"),
	          InitialView.Offers
	              .FilterByPredicate(
	                  [](const FReEchoShopOffer& Offer)
	                  {
		                  return Offer.Type == EReEchoShopOfferType::WeaponPart ||
		                         Offer.Type == EReEchoShopOfferType::Weapon;
	                  })
	              .Num(),
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
			if (Page.Offers.ContainsByPredicate(
			        [&](const FReEchoShopOffer& Offer)
			        {
				        return Offer.Type == EReEchoShopOfferType::WeaponPart && Offer.ContentId == PartId;
			        }))
			{
				return RunSubsystem->PurchaseShopItem(PartId);
			}
			RunSubsystem->TryConsumeShopRefresh(1);
		}
		return false;
	};
	RunSubsystem->TimeShards = 1000;
	TestTrue(TEXT("Core purchase succeeds when it is on the current page"), FindAndBuy(TEXT("P_CORE_FLAME")));
	TestTrue(TEXT("Arrowhead purchase succeeds when it is on the current page"),
	         FindAndBuy(TEXT("P_BOW_SPLIT_ARROWHEAD")));
	TestTrue(TEXT("Purchased rune enters part ownership"), RunSubsystem->OwnedPartIds.Contains(TEXT("P_CORE_FLAME")));
	TestFalse(TEXT("Purchased rune stays out of ordinary item inventory"),
	          RunSubsystem->InventoryItems.Contains(TEXT("P_CORE_FLAME")));
	TestTrue(TEXT("Purchased runes are equipped immediately"),
	         RunSubsystem->CurrentBuild.EquippedParts.ContainsByPredicate(
	             [](const FReEchoEquippedPartSnapshot& Part)
	             {
		             return Part.PartId == TEXT("P_CORE_FLAME");
	             }));
	TestTrue(TEXT("Purchased weapon-specific rune is equipped immediately"),
	         RunSubsystem->CurrentBuild.EquippedParts.ContainsByPredicate(
	             [](const FReEchoEquippedPartSnapshot& Part)
	             {
		             return Part.PartId == TEXT("P_BOW_SPLIT_ARROWHEAD");
	             }));
	const int32 ShardsAfterDuplicate = RunSubsystem->TimeShards;
	TestFalse(TEXT("Duplicate rune purchase is rejected"), RunSubsystem->PurchaseShopItem(TEXT("P_CORE_FLAME")));
	TestEqual(TEXT("Rejected duplicate is atomic"), RunSubsystem->TimeShards, ShardsAfterDuplicate);

	// Plan85 Step 3 regression: an owned rune must not be re-offered after a manual shop refresh.
	RunSubsystem->TimeShards = 1000;
	for (int32 RefreshIndex = 0; RefreshIndex < 8; ++RefreshIndex)
	{
		TestTrue(TEXT("Shop refresh succeeds during Step 3 rune regression"), RunSubsystem->TryConsumeShopRefresh(1));
		const FReEchoWeaponPartShopView RefreshedPage = RunSubsystem->GetWeaponPartShopView();
		TestFalse(TEXT("Owned rune is excluded from offers after refresh"),
		          RefreshedPage.Offers.ContainsByPredicate(
		              [](const FReEchoShopOffer& Offer)
		              {
			              return Offer.Type == EReEchoShopOfferType::WeaponPart &&
			                     Offer.ContentId == TEXT("P_CORE_FLAME");
		              }));
	}

	// The public equip operation remains available for owned/backpack selection and idempotent re-commit.
	TestTrue(TEXT("Core and arrowhead equip as one loadout"),
	         RunSubsystem->TryEquipParts({TEXT("P_CORE_FLAME"), TEXT("P_BOW_SPLIT_ARROWHEAD")}, Error));
	TestEqual(TEXT("Committed loadout contains both equipped slot groups"),
	          RunSubsystem->CurrentBuild.EquippedParts.Num(),
	          2);
	TestEqual(
	    TEXT("Committed strength grip applies its runtime effect"), RunSubsystem->CurrentBuild.Stats.AttackSpeed, 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoStableWeaponPartPageTest,
                                 "ReEcho.Shop.WeaponPartPageRemainsStableAfterSequentialPurchases",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoStableWeaponPartPageTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_09"));
	RunSubsystem->TimeShards = 10000;
	const FReEchoWeaponPartShopView InitialPage = RunSubsystem->GetWeaponPartShopView();
	if (!TestEqual(TEXT("Gun page exposes three stable weapon/rune slots"),
	               InitialPage.SlotOffers.Num(),
	               ReEchoShopOfferCountPerGroup))
	{
		return false;
	}
	TSet<FName> InitialIds;
	for (const FReEchoWeaponSlotOffer& Offer : InitialPage.SlotOffers)
	{
		TestFalse(TEXT("Each gun-page offer has a purchase id"), Offer.ItemId.IsNone());
		TestFalse(TEXT("One weapon/rune page never duplicates an item"), InitialIds.Contains(Offer.ItemId));
		InitialIds.Add(Offer.ItemId);
	}

	UReEchoRunSaveGame* StablePageSave = RunSubsystem->CreateSaveSnapshot();
	UGameInstance* RestoredGameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* RestoredRun = NewObject<UReEchoRunSubsystem>(RestoredGameInstance);
	if (TestNotNull(TEXT("Stable weapon/rune page is included in the save snapshot"), StablePageSave) &&
	    TestTrue(TEXT("Stable weapon/rune page restores"), RestoredRun->RestoreSaveSnapshot(*StablePageSave)))
	{
		const FReEchoWeaponPartShopView RestoredPage = RestoredRun->GetWeaponPartShopView();
		for (int32 SlotIndex = 0; SlotIndex < InitialPage.SlotOffers.Num(); ++SlotIndex)
		{
			TestEqual(TEXT("Save and restore preserves each weapon/rune offer id"),
			          RestoredPage.SlotOffers[SlotIndex].ItemId,
			          InitialPage.SlotOffers[SlotIndex].ItemId);
			TestEqual(TEXT("Save and restore preserves each weapon/rune offer price"),
			          RestoredPage.SlotOffers[SlotIndex].Price,
			          InitialPage.SlotOffers[SlotIndex].Price);
		}
	}

	for (const FReEchoWeaponSlotOffer& OriginalOffer : InitialPage.SlotOffers)
	{
		TestTrue(TEXT("Every original weapon/rune offer remains purchasable after earlier purchases"),
		         RunSubsystem->PurchaseShopItem(OriginalOffer.ItemId));
		const FReEchoWeaponPartShopView PageAfterPurchase = RunSubsystem->GetWeaponPartShopView();
		for (int32 SlotIndex = 0; SlotIndex < InitialPage.SlotOffers.Num(); ++SlotIndex)
		{
			TestEqual(TEXT("Purchase preserves the other weapon/rune offer ids"),
			          PageAfterPurchase.SlotOffers[SlotIndex].ItemId,
			          InitialPage.SlotOffers[SlotIndex].ItemId);
			TestEqual(TEXT("Purchase preserves the other weapon/rune offer prices"),
			          PageAfterPurchase.SlotOffers[SlotIndex].Price,
			          InitialPage.SlotOffers[SlotIndex].Price);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoOwnedWeaponBackpackTest,
                                 "ReEcho.Shop.OwnedWeaponBackpackSwitchesAtomically",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoOwnedWeaponBackpackTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_08"));
	FString Error;
	TestTrue(TEXT("Starting weapon enters the owned weapon backpack"),
	         RunSubsystem->OwnedWeaponIds.Contains(TEXT("W_J_08")));
	RunSubsystem->TimeShards = 10000;
	FName PurchasedWeaponId = NAME_None;
	for (int32 Attempt = 0; Attempt < 128 && PurchasedWeaponId.IsNone(); ++Attempt)
	{
		const FReEchoWeaponPartShopView Page = RunSubsystem->GetWeaponPartShopView();
		if (const FReEchoWeaponSlotOffer* WeaponOffer = Page.SlotOffers.FindByPredicate(
		        [](const FReEchoWeaponSlotOffer& Offer)
		        {
			        return Offer.Kind == EReEchoShopOfferKind::Weapon && !Offer.WeaponId.IsNone();
		        }))
		{
			PurchasedWeaponId = WeaponOffer->WeaponId;
			TestTrue(TEXT("A whole-weapon shop offer can be purchased"),
			         RunSubsystem->PurchaseShopItem(PurchasedWeaponId));
			break;
		}
		RunSubsystem->TryConsumeShopRefresh(1);
	}
	TestFalse(TEXT("The deterministic shop sequence eventually exposes a weapon"), PurchasedWeaponId.IsNone());
	TestTrue(TEXT("Purchased weapon enters the owned weapon backpack"),
	         !PurchasedWeaponId.IsNone() && RunSubsystem->OwnedWeaponIds.Contains(PurchasedWeaponId));
	TestTrue(TEXT("Purchasing another weapon keeps the starting weapon owned"),
	         RunSubsystem->OwnedWeaponIds.Contains(TEXT("W_J_08")));
	TestTrue(TEXT("The starting weapon can be re-equipped after a weapon purchase"),
	         RunSubsystem->TryEquipOwnedWeapon(TEXT("W_J_08"), Error));
	RunSubsystem->OwnedWeaponIds.Add(TEXT("W_J_02"));

	Error.Reset();
	TestTrue(TEXT("Bow accepts a universal core and bow-specific arrowhead"),
	         RunSubsystem->TryEquipParts({TEXT("P_CORE_FLAME"), TEXT("P_BOW_SPLIT_ARROWHEAD")}, Error));
	TestTrue(TEXT("An owned alternate weapon can be equipped for free"),
	         RunSubsystem->TryEquipOwnedWeapon(TEXT("W_J_02"), Error));
	TestEqual(TEXT("Owned weapon selection updates the authoritative build"),
	          RunSubsystem->CurrentBuild.WeaponId,
	          FName(TEXT("W_J_02")));
	TestTrue(TEXT("Compatible universal rune stays equipped after weapon switch"),
	         RunSubsystem->CurrentBuild.EquippedParts.ContainsByPredicate(
	             [](const FReEchoEquippedPartSnapshot& Part)
	             {
		             return Part.PartId == TEXT("P_CORE_FLAME");
	             }));
	TestFalse(TEXT("Incompatible bow rune is unequipped after weapon switch"),
	          RunSubsystem->CurrentBuild.EquippedParts.ContainsByPredicate(
	              [](const FReEchoEquippedPartSnapshot& Part)
	              {
		              return Part.PartId == TEXT("P_BOW_SPLIT_ARROWHEAD");
	              }));

	const FReEchoBuildSnapshot BeforeRejectedSwitch = RunSubsystem->CurrentBuild;
	TestFalse(TEXT("Unowned weapon selection is rejected"), RunSubsystem->TryEquipOwnedWeapon(TEXT("W_J_03"), Error));
	TestEqual(TEXT("Rejected weapon selection keeps the equipped weapon unchanged"),
	          RunSubsystem->CurrentBuild.WeaponId,
	          BeforeRejectedSwitch.WeaponId);
	TestEqual(TEXT("Rejected weapon selection keeps the rune loadout unchanged"),
	          RunSubsystem->CurrentBuild.EquippedParts.Num(),
	          BeforeRejectedSwitch.EquippedParts.Num());

	const FReEchoWeaponPartShopView View = RunSubsystem->GetWeaponPartShopView();
	TestFalse(TEXT("Current weapon shop projection includes a display texture"), View.WeaponIconTexturePath.IsEmpty());
	TestTrue(TEXT("Weapon backpack projection includes every acquired weapon"), View.OwnedWeaponOffers.Num() >= 2);
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
	RunSubsystem->TimeShards = 200;
	const TArray<TArray<int32>> ExpectedShopTiers = {{}, {1}, {1}, {2, 3}, {}, {1, 2}, {1, 3}, {}};
	for (int32 EncounterIndex = 1; EncounterIndex <= ExpectedShopTiers.Num(); ++EncounterIndex)
	{
		RunSubsystem->EncounterIndex = EncounterIndex;
		const FReEchoWeaponPartShopView EncounterPage = RunSubsystem->GetWeaponPartShopView();
		const TArray<int32>& ExpectedTiers = ExpectedShopTiers[EncounterIndex - 1];
		TestEqual(*FString::Printf(TEXT("Encounter %d always exposes three fixed shop card slots"), EncounterIndex),
		          EncounterPage.CardSlotOffers.Num(),
		          ReEchoShopOfferCountPerGroup);
		for (int32 SlotIndex = 0; SlotIndex < EncounterPage.CardSlotOffers.Num(); ++SlotIndex)
		{
			const FReEchoCardSlotOffer& Offer = EncounterPage.CardSlotOffers[SlotIndex];
			const int32 ExpectedTier = SlotIndex + 1;
			const bool bExpectedAvailable = ExpectedTiers.Contains(ExpectedTier);
			TestEqual(*FString::Printf(TEXT("Encounter %d slot %d owns its fixed tier"), EncounterIndex, SlotIndex),
			          Offer.Tier,
			          ExpectedTier);
			TestEqual(*FString::Printf(TEXT("Encounter %d tier %d follows ShopTiers"), EncounterIndex, ExpectedTier),
			          Offer.bAvailable,
			          bExpectedAvailable);
			TestEqual(*FString::Printf(
			              TEXT("Encounter %d tier %d availability matches its card id"), EncounterIndex, ExpectedTier),
			          !Offer.CardId.IsNone(),
			          bExpectedAvailable);
		}
	}

	RunSubsystem->EncounterIndex = 2;
	const TArray<FName> TierOneCardIds = {TEXT("G_1_01"),
	                                      TEXT("G_1_02"),
	                                      TEXT("G_1_03"),
	                                      TEXT("G_1_04"),
	                                      TEXT("G_1_05"),
	                                      TEXT("G_1_06"),
	                                      TEXT("G_1_07"),
	                                      TEXT("G_1_08")};
	RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Append(TierOneCardIds);
	++RunSubsystem->CurrentBuild.CardState.Runtime.ShopRefreshSequence;
	const FReEchoWeaponPartShopView OwnedFilterPage = RunSubsystem->GetWeaponPartShopView();
	if (!TestEqual(TEXT("A fully-owned tier-one shop pool keeps the three fixed tier slots"),
	               OwnedFilterPage.CardSlotOffers.Num(),
	               ReEchoShopOfferCountPerGroup))
	{
		return false;
	}
	TestTrue(TEXT("The tier-one slot remains available from fully-owned repeatable state"),
	         OwnedFilterPage.CardSlotOffers[0].bAvailable);
	TestTrue(TEXT("The tier-one slot may offer an already-owned card"),
	         TierOneCardIds.Contains(OwnedFilterPage.CardSlotOffers[0].CardId));
	TestFalse(TEXT("The unconfigured tier-two slot stays empty"), OwnedFilterPage.CardSlotOffers[1].bAvailable);
	TestFalse(TEXT("The unconfigured tier-three slot stays empty"), OwnedFilterPage.CardSlotOffers[2].bAvailable);
	const FReEchoCardSlotOffer RepeatedTierOneOffer = OwnedFilterPage.CardSlotOffers[0];
	const int32 TierOneStackCountBefore =
	    ReEchoCardRuntime::CountOwned(RunSubsystem->CurrentBuild.CardState, RepeatedTierOneOffer.CardId);
	TestTrue(TEXT("An owned tier-one card can be purchased from a later shop page"),
	         RunSubsystem->PurchaseShopItem(RepeatedTierOneOffer.ItemId));
	TestEqual(TEXT("Repeated tier-one shop purchase adds one stack"),
	          ReEchoCardRuntime::CountOwned(RunSubsystem->CurrentBuild.CardState, RepeatedTierOneOffer.CardId),
	          TierOneStackCountBefore + 1);
	RunSubsystem->EncounterIndex = 3;
	const FReEchoWeaponPartShopView NextEncounterTierOnePage = RunSubsystem->GetWeaponPartShopView();
	if (!TestEqual(TEXT("The next encounter preserves all three fixed tier positions"),
	               NextEncounterTierOnePage.CardSlotOffers.Num(),
	               ReEchoShopOfferCountPerGroup))
	{
		return false;
	}
	const FReEchoCardSlotOffer NextEncounterTierOneOffer = NextEncounterTierOnePage.CardSlotOffers[0];
	TestTrue(TEXT("An owned tier-one card can be purchased again in a later encounter"),
	         RunSubsystem->PurchaseShopItem(NextEncounterTierOneOffer.ItemId));

	RunSubsystem->EncounterIndex = 4;
	RunSubsystem->TimeShards = 400;

	const FReEchoWeaponPartShopView FirstPage = RunSubsystem->GetWeaponPartShopView();
	const TArray<FReEchoShopOffer> FirstCardSlots = FirstPage.Offers.FilterByPredicate(
	    [](const FReEchoShopOffer& Offer)
	    {
		    return Offer.Type == EReEchoShopOfferType::BuildCard;
	    });
	const TArray<FReEchoShopOffer> FirstCards = FirstCardSlots.FilterByPredicate(
	    [](const FReEchoShopOffer& Offer)
	    {
		    return !Offer.ItemId.IsNone();
	    });
	TestEqual(
	    TEXT("First page has three fixed weapon/part slots"), FirstPage.SlotOffers.Num(), ReEchoShopOfferCountPerGroup);
	for (const FReEchoWeaponSlotOffer& Slot : FirstPage.SlotOffers)
	{
		TestTrue(TEXT("Each fixed weapon/part slot contains an offer"), !Slot.ItemId.IsNone());
	}
	TestEqual(TEXT("Encounter four projects all three fixed card positions"),
	          FirstCardSlots.Num(),
	          ReEchoShopOfferCountPerGroup);
	TestEqual(TEXT("Encounter four fills only the configured tier-two and tier-three positions"), FirstCards.Num(), 2);
	TestTrue(TEXT("Encounter four tier-one position is empty"), FirstCardSlots[0].ItemId.IsNone());
	TestEqual(TEXT("Encounter four second position contains a tier-two card"), FirstCardSlots[1].Tier, 2);
	TestEqual(TEXT("Encounter four third position contains a tier-three card"), FirstCardSlots[2].Tier, 3);
	for (const FReEchoShopOffer& Card : FirstCards)
	{
		TestTrue(TEXT("Card tier follows encounter-four configuration"), Card.Tier == 2 || Card.Tier == 3);
		TestEqual(TEXT("Card offer carries its authored card icon path"),
		          Card.IconTexturePath,
		          FString::Printf(TEXT("/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_%s.T_UI_CardIcon_%s"),
		                          *Card.ContentId.ToString(),
		                          *Card.ContentId.ToString()));
		const int32 MinPrice = Card.Tier == 2 ? 100 : 150;
		const int32 MaxPrice = Card.Tier == 2 ? 120 : 200;
		TestTrue(TEXT("Card price comes from the configured tier range"),
		         Card.Price >= MinPrice && Card.Price <= MaxPrice);
	}

	const FReEchoShopOffer* PurchasedCardCandidate = FirstCards.FindByPredicate(
	    [](const FReEchoShopOffer& Offer)
	    {
		    return Offer.ContentId != TEXT("G_2_15");
	    });
	if (!TestNotNull(TEXT("Configured shop page includes a card that does not reset currency"), PurchasedCardCandidate))
	{
		return false;
	}
	const FReEchoShopOffer PurchasedCard = *PurchasedCardCandidate;
	const int32 OwnedBefore = RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Num();
	const int32 ShardsBefore = RunSubsystem->TimeShards;
	TestTrue(TEXT("Current card offer can be purchased"), RunSubsystem->PurchaseShopItem(PurchasedCard.ItemId));
	TestEqual(TEXT("Card purchase grants one owned-card slot"),
	          RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Num(),
	          OwnedBefore + 1);
	TestTrue(TEXT("Granted card id is the offer content id"),
	         RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Contains(PurchasedCard.ContentId));
	TestEqual(TEXT("Card purchase deducts the effective tier price"),
	          RunSubsystem->TimeShards,
	          ShardsBefore - RunSubsystem->GetDiscountedShopPrice(PurchasedCard.Price));
	const FReEchoWeaponPartShopView PurchasedPage = RunSubsystem->GetWeaponPartShopView();
	TestTrue(TEXT("Purchased card is projected into the right-side owned slots"),
	         PurchasedPage.OwnedCards.ContainsByPredicate(
	             [&](const FReEchoShopOffer& Card)
	             {
		             return Card.ContentId == PurchasedCard.ContentId;
	             }));
	TestTrue(TEXT("Purchased card remains on the stable current page as a sold offer"),
	         PurchasedPage.Offers.ContainsByPredicate(
	             [&](const FReEchoShopOffer& Offer)
	             {
		             return Offer.Type == EReEchoShopOfferType::BuildCard && Offer.ItemId == PurchasedCard.ItemId &&
		                    Offer.ContentId == PurchasedCard.ContentId && Offer.Price == PurchasedCard.Price;
	             }));
	TestFalse(TEXT("Same card offer cannot be bought twice on one page"),
	          RunSubsystem->PurchaseShopItem(PurchasedCard.ItemId));

	const int32 SequenceBefore = RunSubsystem->CurrentBuild.CardState.Runtime.ShopRefreshSequence;
	const int32 BeforeRefreshShards = RunSubsystem->TimeShards;
	TestTrue(TEXT("Paid refresh succeeds after free refreshes are exhausted"),
	         RunSubsystem->TryConsumeShopRefresh(ReEchoShopRefreshPrice));
	TestEqual(TEXT("Joint refresh advances one shared sequence"),
	          RunSubsystem->CurrentBuild.CardState.Runtime.ShopRefreshSequence,
	          SequenceBefore + 1);
	TestEqual(TEXT("Paid refresh deducts the configured price"),
	          RunSubsystem->TimeShards,
	          BeforeRefreshShards - ReEchoShopRefreshPrice);
	const FReEchoWeaponPartShopView RefreshedPage = RunSubsystem->GetWeaponPartShopView();
	TestEqual(TEXT("Refreshed page still has three fixed weapon/part slots"),
	          RefreshedPage.SlotOffers.Num(),
	          ReEchoShopOfferCountPerGroup);
	TestEqual(TEXT("Refreshed page keeps three fixed card positions"),
	          RefreshedPage.Offers
	              .FilterByPredicate(
	                  [](const FReEchoShopOffer& Offer)
	                  {
		                  return Offer.Type == EReEchoShopOfferType::BuildCard;
	                  })
	              .Num(),
	          ReEchoShopOfferCountPerGroup);
	TestEqual(TEXT("Refreshed page keeps exactly the two configured positions purchasable"),
	          RefreshedPage.Offers
	              .FilterByPredicate(
	                  [](const FReEchoShopOffer& Offer)
	                  {
		                  return Offer.Type == EReEchoShopOfferType::BuildCard && !Offer.ItemId.IsNone();
	                  })
	              .Num(),
	          2);

	TestFalse(TEXT("Purchased card stays excluded after refresh regardless of stack policy"),
	          RefreshedPage.Offers.ContainsByPredicate(
	              [&](const FReEchoShopOffer& Offer)
	              {
		              return Offer.Type == EReEchoShopOfferType::BuildCard &&
		                     Offer.ContentId == PurchasedCard.ContentId;
	              }));

	UGameInstance* StablePageGameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* StablePageRun = NewObject<UReEchoRunSubsystem>(StablePageGameInstance);
	StablePageRun->StartRun(TEXT("J_CAT"), TEXT("W_J_08"));
	StablePageRun->EncounterIndex = 4;
	StablePageRun->TimeShards = 1000;
	const FReEchoWeaponPartShopView StableInitialPage = StablePageRun->GetWeaponPartShopView();
	const TArray<FReEchoShopOffer> StableInitialCards = StableInitialPage.Offers.FilterByPredicate(
	    [](const FReEchoShopOffer& Offer)
	    {
		    return Offer.Type == EReEchoShopOfferType::BuildCard && !Offer.ItemId.IsNone();
	    });
	if (!TestEqual(TEXT("Tier-two/three page contains two purchasable fixed-tier cards"), StableInitialCards.Num(), 2))
	{
		return false;
	}
	UReEchoRunSaveGame* StablePageSave = StablePageRun->CreateSaveSnapshot();
	UGameInstance* RestoredPageGameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* RestoredPageRun = NewObject<UReEchoRunSubsystem>(RestoredPageGameInstance);
	if (TestNotNull(TEXT("Stable card page can be captured in a save snapshot"), StablePageSave) &&
	    TestTrue(TEXT("Stable card page restores into a fresh run subsystem"),
	             RestoredPageRun->RestoreSaveSnapshot(*StablePageSave)))
	{
		const FReEchoWeaponPartShopView RestoredPage = RestoredPageRun->GetWeaponPartShopView();
		for (const FReEchoShopOffer& ExpectedOffer : StableInitialCards)
		{
			TestTrue(TEXT("Save and restore preserves the current card ids and prices"),
			         RestoredPage.Offers.ContainsByPredicate(
			             [&](const FReEchoShopOffer& CurrentOffer)
			             {
				             return CurrentOffer.Type == EReEchoShopOfferType::BuildCard &&
				                    CurrentOffer.ItemId == ExpectedOffer.ItemId &&
				                    CurrentOffer.ContentId == ExpectedOffer.ContentId &&
				                    CurrentOffer.Price == ExpectedOffer.Price;
			             }));
		}
	}
	for (const FReEchoShopOffer& OriginalOffer : StableInitialCards)
	{
		TestTrue(TEXT("Every remaining card on one page can be purchased sequentially"),
		         StablePageRun->PurchaseShopItem(OriginalOffer.ItemId));
		const FReEchoWeaponPartShopView StablePurchasedPage = StablePageRun->GetWeaponPartShopView();
		for (const FReEchoShopOffer& ExpectedOffer : StableInitialCards)
		{
			TestTrue(TEXT("Purchasing one card preserves all three current-page card ids and prices"),
			         StablePurchasedPage.Offers.ContainsByPredicate(
			             [&](const FReEchoShopOffer& CurrentOffer)
			             {
				             return CurrentOffer.Type == EReEchoShopOfferType::BuildCard &&
				                    CurrentOffer.ItemId == ExpectedOffer.ItemId &&
				                    CurrentOffer.ContentId == ExpectedOffer.ContentId &&
				                    CurrentOffer.Price == ExpectedOffer.Price;
			             }));
		}
	}
	for (const FReEchoShopOffer& PurchasedOffer : StableInitialCards)
	{
		TestTrue(TEXT("Sequential purchase grants every card from the original page"),
		         StablePageRun->CurrentBuild.CardState.OwnedCardIds.Contains(PurchasedOffer.ContentId));
	}

	UGameInstance* ExhaustedGameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* ExhaustedRun = NewObject<UReEchoRunSubsystem>(ExhaustedGameInstance);
	ExhaustedRun->StartRun(TEXT("J_CAT"), TEXT("W_J_08"));
	ExhaustedRun->EncounterIndex = 4;
	const TSharedPtr<const FReEchoCsvDataSnapshot> ExhaustedSnapshot = ExhaustedRun->GetRunDataSnapshot();
	if (TestTrue(TEXT("Exhaustion test has a card catalog"),
	             ExhaustedSnapshot.IsValid() && ExhaustedSnapshot->CardCatalog.IsValid()))
	{
		for (const FReEchoCardDefinition& TierTwoCard : ExhaustedSnapshot->CardCatalog->GetOfferable(TEXT("Trait"), 2))
		{
			ExhaustedRun->CurrentBuild.CardState.OwnedCardIds.Add(TierTwoCard.Id);
		}
		const FReEchoWeaponPartShopView ExhaustedPage = ExhaustedRun->GetWeaponPartShopView();
		TestFalse(TEXT("An exhausted configured tier-two slot is unavailable"),
		          ExhaustedPage.CardSlotOffers[1].bAvailable);
		TestTrue(TEXT("Exhausting tier two does not consume the configured tier-three slot"),
		         ExhaustedPage.CardSlotOffers[2].bAvailable);
		TestEqual(TEXT("The exhausted tier-two slot keeps its tier identity"), ExhaustedPage.CardSlotOffers[1].Tier, 2);
		TestEqual(TEXT("The tier-three slot never backfills tier two"), ExhaustedPage.CardSlotOffers[2].Tier, 3);
	}
	return true;
}
#endif
