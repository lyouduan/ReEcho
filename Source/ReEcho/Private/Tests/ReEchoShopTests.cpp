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

	const FReEchoShopPurchaseOutcome SuccessfulPurchase =
	    RunSubsystem->PurchaseShopItemDetailed(TEXT("SHOP_RUSTED_SCISSORS"));
	TestTrue(TEXT("Known affordable item can be purchased"), SuccessfulPurchase.IsSuccess());
	TestEqual(TEXT("Successful purchase has a structured result"),
	          SuccessfulPurchase.Result,
	          EReEchoShopPurchaseResult::Succeeded);
	TestFalse(TEXT("Successful purchase has a transaction id"), SuccessfulPurchase.TransactionId.IsEmpty());
	TestEqual(TEXT("Structured result reports the effective price"), SuccessfulPurchase.EffectivePrice, 15);
	TestEqual(TEXT("Purchase deducts its exact price"), RunSubsystem->TimeShards, 35);
	TestTrue(TEXT("Purchased item enters inventory"),
	         RunSubsystem->InventoryItems.Contains(TEXT("SHOP_RUSTED_SCISSORS")));
	TestEqual(TEXT("Purchased item applies its build effect"),
	          RunSubsystem->CurrentBuild.Stats.PhysicalAttack,
	          InitialPhysicalAttack + 2.0f);

	const FReEchoShopPurchaseOutcome DuplicatePurchase =
	    RunSubsystem->PurchaseShopItemDetailed(TEXT("SHOP_RUSTED_SCISSORS"));
	TestFalse(TEXT("Owned item cannot be purchased twice"), DuplicatePurchase.IsSuccess());
	TestEqual(TEXT("Duplicate purchase reports ownership"),
	          DuplicatePurchase.Result,
	          EReEchoShopPurchaseResult::AlreadyOwned);
	TestNotEqual(TEXT("Every attempt receives a distinct transaction id"),
	             DuplicatePurchase.TransactionId,
	             SuccessfulPurchase.TransactionId);
	TestEqual(TEXT("Rejected duplicate does not deduct currency"), RunSubsystem->TimeShards, 35);
	const FReEchoShopPurchaseOutcome UnknownPurchase = RunSubsystem->PurchaseShopItemDetailed(TEXT("SHOP_UNKNOWN"));
	TestEqual(TEXT("Unknown item reports the missing offer"),
	          UnknownPurchase.Result,
	          EReEchoShopPurchaseResult::OfferNotFound);

	UReEchoRunSubsystem* PoorRunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	PoorRunSubsystem->TimeShards = 0;
	const FReEchoShopPurchaseOutcome UnaffordablePurchase =
	    PoorRunSubsystem->PurchaseShopItemDetailed(TEXT("SHOP_RUSTED_SCISSORS"));
	TestEqual(TEXT("Unaffordable item reports insufficient currency"),
	          UnaffordablePurchase.Result,
	          EReEchoShopPurchaseResult::InsufficientCurrency);
	TestEqual(TEXT("Rejected purchase leaves currency unchanged"), PoorRunSubsystem->TimeShards, 0);
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
	for (int32 SpawnIndex = 1; SpawnIndex <= 6; ++SpawnIndex)
	{
		RunSubsystem->GrantTimeShards(RunSubsystem->ResolveEnemyDeathTimeShardDrop(TEXT("M_Grunt"), SpawnIndex));
	}
	RunSubsystem->CompleteEncounter(FReEchoRecording(), true, false);
	TestEqual(TEXT("Encounter one has no free card group"), RunSubsystem->Phase, EReEchoRunPhase::Planning);
	RunSubsystem->BeginEncounter();
	for (int32 SpawnIndex = 1; SpawnIndex <= 6; ++SpawnIndex)
	{
		RunSubsystem->GrantTimeShards(RunSubsystem->ResolveEnemyDeathTimeShardDrop(TEXT("M_Grunt"), SpawnIndex));
	}
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
	const int32 ShardsBeforePurchase = RunSubsystem->TimeShards;
	TestTrue(TEXT("Per-enemy rewards provide shop currency"), ShardsBeforePurchase >= 24 && ShardsBeforePurchase <= 36);
	const int32 ExpectedPrice = RunSubsystem->GetDiscountedShopPrice(15);
	TestTrue(TEXT("Post-draw currency can buy the entry-price item"),
	         RunSubsystem->PurchaseShopItem(TEXT("SHOP_RUSTED_SCISSORS")));
	TestEqual(TEXT("Post-draw purchase deducts the current effective price"),
	          RunSubsystem->TimeShards,
	          ShardsBeforePurchase - ExpectedPrice);
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
		if (OriginalOffer.Kind == EReEchoShopOfferKind::Part)
		{
			TestTrue(TEXT("A purchased rune is immediately present in the refreshed owned-rune projection"),
			         PageAfterPurchase.OwnedParts.ContainsByPredicate(
			             [&](const FReEchoShopOffer& OwnedPart)
			             {
				             return OwnedPart.ContentId == OriginalOffer.PartId;
			             }));
		}
		else
		{
			TestTrue(TEXT("A purchased weapon is immediately present in the refreshed owned-weapon projection"),
			         PageAfterPurchase.OwnedWeapons.Contains(OriginalOffer.WeaponId));
		}
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
	RunSubsystem->TimeShards = 1000;
	const TArray<TArray<int32>> ExpectedShopTiers = {{}, {1}, {1}, {2, 3}, {}, {1, 2}, {1, 3}, {}};
	for (int32 EncounterIndex = 1; EncounterIndex <= ExpectedShopTiers.Num(); ++EncounterIndex)
	{
		RunSubsystem->EncounterIndex = EncounterIndex;
		const FReEchoWeaponPartShopView EncounterPage = RunSubsystem->GetWeaponPartShopView();
		const TArray<int32>& ExpectedTiers = ExpectedShopTiers[EncounterIndex - 1];
		TestEqual(*FString::Printf(TEXT("Encounter %d always exposes three fixed shop card packs"), EncounterIndex),
		          EncounterPage.CardPackOffers.Num(),
		          ReEchoShopOfferCountPerGroup);
		for (int32 PackIndex = 0; PackIndex < EncounterPage.CardPackOffers.Num(); ++PackIndex)
		{
			const FReEchoShopCardPackOffer& Pack = EncounterPage.CardPackOffers[PackIndex];
			const int32 ExpectedTier = PackIndex + 1;
			const bool bExpectedOffered = ExpectedTiers.Contains(ExpectedTier);
			TestEqual(*FString::Printf(TEXT("Encounter %d pack %d owns its fixed tier"), EncounterIndex, PackIndex),
			          Pack.Tier,
			          ExpectedTier);
			TestEqual(*FString::Printf(TEXT("Encounter %d tier %d follows ShopTiers"), EncounterIndex, ExpectedTier),
			          Pack.Status != EReEchoShopCardPackStatus::NotOffered,
			          bExpectedOffered);
			if (Pack.Status == EReEchoShopCardPackStatus::Available)
			{
				TestTrue(TEXT("An offered pack exposes between one and three choices"),
				         Pack.Choices.Num() >= 1 && Pack.Choices.Num() <= ReEchoShopOfferCountPerGroup);
				TSet<FName> UniqueChoices;
				for (const FReEchoShopCardChoiceOffer& Choice : Pack.Choices)
				{
					TestEqual(TEXT("Every pack choice matches the fixed pack tier"), Choice.Tier, ExpectedTier);
					TestFalse(TEXT("One pack never repeats a card"), UniqueChoices.Contains(Choice.CardId));
					UniqueChoices.Add(Choice.CardId);
					const int32 MinPrice = Choice.Tier == 1 ? 30 : Choice.Tier == 2 ? 100 : 150;
					const int32 MaxPrice = Choice.Tier == 1 ? 50 : Choice.Tier == 2 ? 120 : 200;
					TestTrue(TEXT("Each choice carries its own configured price"),
					         Choice.Price >= MinPrice && Choice.Price <= MaxPrice);
				}
			}
		}
		TestFalse(TEXT("Card packs are not flattened into icon-bearing direct-purchase offers"),
		          EncounterPage.Offers.ContainsByPredicate(
		              [](const FReEchoShopOffer& Offer)
		              {
			              return Offer.Type == EReEchoShopOfferType::BuildCard;
		              }));
	}

	RunSubsystem->EncounterIndex = 2;
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = RunSubsystem->GetRunDataSnapshot();
	if (!TestTrue(TEXT("Card-pack test has a card catalog"), Snapshot.IsValid() && Snapshot->CardCatalog.IsValid()))
	{
		return false;
	}
	TArray<FName> TierOneCardIds;
	for (const FReEchoCardDefinition& Card : Snapshot->CardCatalog->GetOfferable(TEXT("Trait"), 1))
	{
		TierOneCardIds.Add(Card.Id);
	}
	RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Append(TierOneCardIds);
	++RunSubsystem->CurrentBuild.CardState.Runtime.ShopRefreshSequence;
	const FReEchoWeaponPartShopView OwnedFilterPage = RunSubsystem->GetWeaponPartShopView();
	if (!TestEqual(TEXT("A fully-owned tier-one pool keeps the three fixed packs"),
	               OwnedFilterPage.CardPackOffers.Num(),
	               ReEchoShopOfferCountPerGroup))
	{
		return false;
	}
	TestTrue(TEXT("The tier-one pack remains available from fully-owned repeatable state"),
	         OwnedFilterPage.CardPackOffers[0].IsAvailable());
	TestTrue(TEXT("Every tier-one choice may already be owned"),
	         OwnedFilterPage.CardPackOffers[0].Choices.ContainsByPredicate(
	             [&](const FReEchoShopCardChoiceOffer& Choice)
	             {
		             return TierOneCardIds.Contains(Choice.CardId);
	             }));
	TestEqual(TEXT("The unconfigured tier-two pack stays unoffered"),
	          OwnedFilterPage.CardPackOffers[1].Status,
	          EReEchoShopCardPackStatus::NotOffered);
	const FReEchoShopCardChoiceOffer RepeatedTierOneOffer = OwnedFilterPage.CardPackOffers[0].Choices[0];
	const int32 TierOneStackCountBefore =
	    ReEchoCardRuntime::CountOwned(RunSubsystem->CurrentBuild.CardState, RepeatedTierOneOffer.CardId);
	TestTrue(TEXT("An owned tier-one card can be purchased from its pack"),
	         RunSubsystem->PurchaseShopItem(RepeatedTierOneOffer.ItemId));
	TestEqual(TEXT("Repeated tier-one shop purchase adds one stack"),
	          ReEchoCardRuntime::CountOwned(RunSubsystem->CurrentBuild.CardState, RepeatedTierOneOffer.CardId),
	          TierOneStackCountBefore + 1);
	const FReEchoWeaponPartShopView ConsumedTierOnePage = RunSubsystem->GetWeaponPartShopView();
	TestEqual(TEXT("A successful choice marks only its pack purchased"),
	          ConsumedTierOnePage.CardPackOffers[0].Status,
	          EReEchoShopCardPackStatus::Purchased);
	if (OwnedFilterPage.CardPackOffers[0].Choices.Num() > 1)
	{
		TestFalse(TEXT("A second choice from the same pack is rejected"),
		          RunSubsystem->PurchaseShopItem(OwnedFilterPage.CardPackOffers[0].Choices[1].ItemId));
	}

	RunSubsystem->EncounterIndex = 4;
	RunSubsystem->TimeShards = 1000;
	const FReEchoWeaponPartShopView FirstPage = RunSubsystem->GetWeaponPartShopView();
	TestEqual(
	    TEXT("First page has three fixed weapon/part slots"), FirstPage.SlotOffers.Num(), ReEchoShopOfferCountPerGroup);
	TestEqual(TEXT("Encounter four projects all three fixed card packs"),
	          FirstPage.CardPackOffers.Num(),
	          ReEchoShopOfferCountPerGroup);
	TestEqual(TEXT("Encounter four tier one is not offered"),
	          FirstPage.CardPackOffers[0].Status,
	          EReEchoShopCardPackStatus::NotOffered);
	TestTrue(TEXT("Encounter four tier two is available"), FirstPage.CardPackOffers[1].IsAvailable());
	TestTrue(TEXT("Encounter four tier three is available"), FirstPage.CardPackOffers[2].IsAvailable());
	const FReEchoShopCardChoiceOffer* PurchasedCardCandidate = FirstPage.CardPackOffers[1].Choices.FindByPredicate(
	    [](const FReEchoShopCardChoiceOffer& Offer)
	    {
		    return Offer.CardId != TEXT("G_2_15");
	    });
	if (!PurchasedCardCandidate)
	{
		PurchasedCardCandidate =
		    FirstPage.CardPackOffers[2].Choices.IsEmpty() ? nullptr : &FirstPage.CardPackOffers[2].Choices[0];
	}
	if (!TestNotNull(TEXT("Configured shop page includes a purchasable card"), PurchasedCardCandidate))
	{
		return false;
	}
	const FReEchoShopCardChoiceOffer PurchasedCard = *PurchasedCardCandidate;
	const int32 OwnedBefore = RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Num();
	const int32 ShardsBefore = RunSubsystem->TimeShards;
	const int32 EffectivePriceBeforePurchase = RunSubsystem->GetDiscountedShopPrice(PurchasedCard.Price);
	TestTrue(TEXT("Current card offer can be purchased"), RunSubsystem->PurchaseShopItem(PurchasedCard.ItemId));
	TestEqual(TEXT("Card purchase grants one owned-card slot"),
	          RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Num(),
	          OwnedBefore + 1);
	TestTrue(TEXT("Granted card id is the selected choice id"),
	         RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Contains(PurchasedCard.CardId));
	TestEqual(TEXT("Card purchase deducts the effective tier price"),
	          RunSubsystem->TimeShards,
	          ShardsBefore - EffectivePriceBeforePurchase);
	const FReEchoWeaponPartShopView PurchasedPage = RunSubsystem->GetWeaponPartShopView();
	TestTrue(TEXT("Purchased card is projected into the right-side owned slots"),
	         PurchasedPage.OwnedCards.ContainsByPredicate(
	             [&](const FReEchoShopOffer& Card)
	             {
		             return Card.ContentId == PurchasedCard.CardId;
	             }));
	const FReEchoShopCardPackOffer& PurchasedPack = PurchasedPage.CardPackOffers[PurchasedCard.Tier - 1];
	TestEqual(
	    TEXT("The selected pack is marked purchased"), PurchasedPack.Status, EReEchoShopCardPackStatus::Purchased);
	TestTrue(TEXT("A purchased pack preserves its cached candidate ids for save/load stability"),
	         RunSubsystem->CurrentBuild.CardState.Runtime.ShopCardPackStates[PurchasedCard.Tier - 1]
	             .CandidateCardIds.Contains(PurchasedCard.CardId));
	TestFalse(TEXT("The newly owned tier-two/three card is no longer projected as a shop choice"),
	          PurchasedPack.Choices.ContainsByPredicate(
	              [&](const FReEchoShopCardChoiceOffer& Choice)
	              {
		              return Choice.ItemId == PurchasedCard.ItemId || Choice.CardId == PurchasedCard.CardId;
	              }));
	TestFalse(TEXT("Same card offer cannot be bought twice on one page"),
	          RunSubsystem->PurchaseShopItem(PurchasedCard.ItemId));
	const int32 OtherPackIndex = PurchasedCard.Tier == 2 ? 2 : 1;
	TestTrue(TEXT("Purchasing one pack leaves the other configured pack available"),
	         PurchasedPage.CardPackOffers[OtherPackIndex].IsAvailable());

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
	TestEqual(TEXT("Refreshed page keeps three fixed card packs"),
	          RefreshedPage.CardPackOffers.Num(),
	          ReEchoShopOfferCountPerGroup);
	TestTrue(TEXT("Full refresh clears purchased state on every offered pack"),
	         RefreshedPage.CardPackOffers[1].Status != EReEchoShopCardPackStatus::Purchased &&
	             RefreshedPage.CardPackOffers[2].Status != EReEchoShopCardPackStatus::Purchased);
	TestFalse(TEXT("A non-tier-one purchased card stays excluded after refresh"),
	          RefreshedPage.CardPackOffers[PurchasedCard.Tier - 1].Choices.ContainsByPredicate(
	              [&](const FReEchoShopCardChoiceOffer& Choice)
	              {
		              return Choice.CardId == PurchasedCard.CardId;
	              }));

	UGameInstance* OwnershipChangeGameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* OwnershipChangeRun = NewObject<UReEchoRunSubsystem>(OwnershipChangeGameInstance);
	OwnershipChangeRun->StartRun(TEXT("J_CAT"), TEXT("W_J_08"));
	OwnershipChangeRun->EncounterIndex = 4;
	const FReEchoWeaponPartShopView BeforeOwnershipChange = OwnershipChangeRun->GetWeaponPartShopView();
	if (!TestTrue(TEXT("Ownership-change fixture exposes tier-three candidates"),
	              BeforeOwnershipChange.CardPackOffers[2].IsAvailable()))
	{
		return false;
	}
	const FName NewlyOwnedTierThreeCard = BeforeOwnershipChange.CardPackOffers[2].Choices[0].CardId;
	OwnershipChangeRun->CurrentBuild.CardState.OwnedCardIds.Add(NewlyOwnedTierThreeCard);
	const FReEchoWeaponPartShopView AfterOwnershipChange = OwnershipChangeRun->GetWeaponPartShopView();
	TestFalse(TEXT("A tier-two/three candidate acquired after page generation is hidden without rerolling"),
	          AfterOwnershipChange.CardPackOffers[2].Choices.ContainsByPredicate(
	              [&](const FReEchoShopCardChoiceOffer& Choice)
	              {
		              return Choice.CardId == NewlyOwnedTierThreeCard;
	              }));
	TestEqual(TEXT("Hiding a newly owned candidate preserves the rest of the cached page"),
	          AfterOwnershipChange.CardPackOffers[2].Choices.Num(),
	          BeforeOwnershipChange.CardPackOffers[2].Choices.Num() - 1);

	UGameInstance* StablePageGameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* StablePageRun = NewObject<UReEchoRunSubsystem>(StablePageGameInstance);
	StablePageRun->StartRun(TEXT("J_CAT"), TEXT("W_J_08"));
	StablePageRun->EncounterIndex = 4;
	StablePageRun->TimeShards = 1000;
	const FReEchoWeaponPartShopView StableInitialPage = StablePageRun->GetWeaponPartShopView();
	if (!TestTrue(TEXT("Tier-two/three page contains two available packs"),
	              StableInitialPage.CardPackOffers[1].IsAvailable() &&
	                  StableInitialPage.CardPackOffers[2].IsAvailable()))
	{
		return false;
	}
	const FReEchoShopCardChoiceOffer* StableTierTwoChoice = StableInitialPage.CardPackOffers[1].Choices.FindByPredicate(
	    [](const FReEchoShopCardChoiceOffer& Choice)
	    {
		    // Keep this persistence/independence fixture focused: G_2_10 grants a tier-three card and
		    // G_2_15 clears shards as their intended OnGrant effects.
		    return Choice.CardId != TEXT("G_2_10") && Choice.CardId != TEXT("G_2_15");
	    });
	if (!TestNotNull(TEXT("Stable page exposes a tier-two choice without cross-tier/currency side effects"),
	                 StableTierTwoChoice))
	{
		return false;
	}
	TestTrue(TEXT("One choice can be purchased before saving"),
	         StablePageRun->PurchaseShopItem(StableTierTwoChoice->ItemId));
	const FReEchoWeaponPartShopView StablePurchasedPage = StablePageRun->GetWeaponPartShopView();
	UReEchoRunSaveGame* StablePageSave = StablePageRun->CreateSaveSnapshot();
	UGameInstance* RestoredPageGameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* RestoredPageRun = NewObject<UReEchoRunSubsystem>(RestoredPageGameInstance);
	if (TestNotNull(TEXT("Stable card page can be captured in a save snapshot"), StablePageSave) &&
	    TestTrue(TEXT("Stable card page restores into a fresh run subsystem"),
	             RestoredPageRun->RestoreSaveSnapshot(*StablePageSave)))
	{
		const FReEchoWeaponPartShopView RestoredPage = RestoredPageRun->GetWeaponPartShopView();
		for (int32 PackIndex = 0; PackIndex < StablePurchasedPage.CardPackOffers.Num(); ++PackIndex)
		{
			const FReEchoShopCardPackOffer& ExpectedPack = StablePurchasedPage.CardPackOffers[PackIndex];
			const FReEchoShopCardPackOffer& ActualPack = RestoredPage.CardPackOffers[PackIndex];
			TestEqual(TEXT("Save and restore preserves each pack status"), ActualPack.Status, ExpectedPack.Status);
			TestEqual(TEXT("Save and restore preserves each pack candidate count"),
			          ActualPack.Choices.Num(),
			          ExpectedPack.Choices.Num());
			for (int32 ChoiceIndex = 0; ChoiceIndex < ExpectedPack.Choices.Num(); ++ChoiceIndex)
			{
				TestEqual(TEXT("Save and restore preserves candidate item ids"),
				          ActualPack.Choices[ChoiceIndex].ItemId,
				          ExpectedPack.Choices[ChoiceIndex].ItemId);
				TestEqual(TEXT("Save and restore preserves candidate prices"),
				          ActualPack.Choices[ChoiceIndex].Price,
				          ExpectedPack.Choices[ChoiceIndex].Price);
			}
		}
	}
	const FReEchoShopCardChoiceOffer StableTierThreeChoice = StablePurchasedPage.CardPackOffers[2].Choices[0];
	TestTrue(TEXT("A different tier pack remains independently purchasable"),
	         StablePageRun->PurchaseShopItem(StableTierThreeChoice.ItemId));

	UGameInstance* FailureGameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* FailureRun = NewObject<UReEchoRunSubsystem>(FailureGameInstance);
	FailureRun->StartRun(TEXT("J_CAT"), TEXT("W_J_08"));
	FailureRun->EncounterIndex = 4;
	FailureRun->TimeShards = 0;
	const FReEchoWeaponPartShopView BeforeFailurePage = FailureRun->GetWeaponPartShopView();
	const FReEchoShopCardChoiceOffer FailureChoice = BeforeFailurePage.CardPackOffers[1].Choices[0];
	const FReEchoShopPurchaseOutcome FailureOutcome = FailureRun->PurchaseShopItemDetailed(FailureChoice.ItemId);
	TestEqual(TEXT("Insufficient currency returns a structured failure"),
	          FailureOutcome.Result,
	          EReEchoShopPurchaseResult::InsufficientCurrency);
	const FReEchoWeaponPartShopView AfterFailurePage = FailureRun->GetWeaponPartShopView();
	TestEqual(TEXT("A failed purchase leaves its pack available"),
	          AfterFailurePage.CardPackOffers[1].Status,
	          EReEchoShopCardPackStatus::Available);
	TestEqual(TEXT("A failed purchase preserves the exact candidate count"),
	          AfterFailurePage.CardPackOffers[1].Choices.Num(),
	          BeforeFailurePage.CardPackOffers[1].Choices.Num());
	for (int32 ChoiceIndex = 0; ChoiceIndex < BeforeFailurePage.CardPackOffers[1].Choices.Num(); ++ChoiceIndex)
	{
		TestEqual(TEXT("A failed purchase never rerolls candidate ids"),
		          AfterFailurePage.CardPackOffers[1].Choices[ChoiceIndex].ItemId,
		          BeforeFailurePage.CardPackOffers[1].Choices[ChoiceIndex].ItemId);
	}

	UGameInstance* PartialGameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* PartialRun = NewObject<UReEchoRunSubsystem>(PartialGameInstance);
	PartialRun->StartRun(TEXT("J_CAT"), TEXT("W_J_08"));
	PartialRun->EncounterIndex = 4;
	const TSharedPtr<const FReEchoCsvDataSnapshot> PartialSnapshot = PartialRun->GetRunDataSnapshot();
	if (TestTrue(TEXT("Partial-pool test has a card catalog"),
	             PartialSnapshot.IsValid() && PartialSnapshot->CardCatalog.IsValid()))
	{
		const TArray<FReEchoCardDefinition> TierTwoCards = PartialSnapshot->CardCatalog->GetOfferable(TEXT("Trait"), 2);
		if (!TestTrue(TEXT("Partial-pool test has at least two tier-two cards"), TierTwoCards.Num() >= 2))
		{
			return false;
		}
		for (int32 CardIndex = 0; CardIndex < TierTwoCards.Num() - 2; ++CardIndex)
		{
			PartialRun->CurrentBuild.CardState.OwnedCardIds.Add(TierTwoCards[CardIndex].Id);
		}
		const FReEchoWeaponPartShopView TwoRemainingPage = PartialRun->GetWeaponPartShopView();
		TestEqual(TEXT("A tier with exactly two eligible cards exposes exactly two choices"),
		          TwoRemainingPage.CardPackOffers[1].Choices.Num(),
		          2);
		PartialRun->CurrentBuild.CardState.OwnedCardIds.Add(TwoRemainingPage.CardPackOffers[1].Choices[0].CardId);
		++PartialRun->CurrentBuild.CardState.Runtime.ShopRefreshSequence;
		const FReEchoWeaponPartShopView OneRemainingPage = PartialRun->GetWeaponPartShopView();
		TestEqual(TEXT("A tier with one eligible card exposes one choice without cross-tier fill"),
		          OneRemainingPage.CardPackOffers[1].Choices.Num(),
		          1);
		TestEqual(TEXT("The one remaining choice keeps the pack tier"),
		          OneRemainingPage.CardPackOffers[1].Choices[0].Tier,
		          2);
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
		TestEqual(TEXT("An exhausted configured tier-two pack is sold out"),
		          ExhaustedPage.CardPackOffers[1].Status,
		          EReEchoShopCardPackStatus::SoldOut);
		TestTrue(TEXT("Exhausting tier two does not consume tier three"),
		         ExhaustedPage.CardPackOffers[2].IsAvailable());
		TestEqual(TEXT("The exhausted tier-two pack keeps its tier identity"), ExhaustedPage.CardPackOffers[1].Tier, 2);
		TestEqual(TEXT("The tier-three pack never backfills tier two"), ExhaustedPage.CardPackOffers[2].Tier, 3);
	}
	return true;
}
#endif
