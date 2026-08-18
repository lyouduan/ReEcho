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
	RunSubsystem->StartRun(TEXT("J_CAT"), TEXT("W_J_02"));
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
	RunSubsystem->StartRun(TEXT("J_CAT"), TEXT("W_J_02"));
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
	RunSubsystem->StartRun(TEXT("J_CAT"), TEXT("W_J_05"));
	RunSubsystem->TimeShards = 100;

	const FReEchoWeaponPartShopView InitialView = RunSubsystem->GetWeaponPartShopView();
	TestEqual(TEXT("Dagger shop exposes three data-driven slot groups"), InitialView.Slots.Num(), 3);
	TestTrue(TEXT("Core is offered in the shop"),
	         InitialView.Offers.ContainsByPredicate(
	             [](const FReEchoShopOffer& Offer)
	             {
		             return Offer.ItemId == TEXT("P_CORE_FLAME") && Offer.Type == EReEchoShopOfferType::WeaponPart;
	             }));

	FString Error;
	TestFalse(TEXT("Unowned parts cannot be committed"),
	          RunSubsystem->TrySaveWeaponPartLoadout({TEXT("P_CORE_FLAME")}, Error));
	TestTrue(TEXT("Core purchase succeeds"), RunSubsystem->PurchaseShopItem(TEXT("P_CORE_FLAME")));
	TestTrue(TEXT("Grip purchase succeeds"), RunSubsystem->PurchaseShopItem(TEXT("P_DAGGER_STRENGTH_GRIP")));
	TestTrue(TEXT("Blade purchase succeeds"), RunSubsystem->PurchaseShopItem(TEXT("P_DAGGER_HOLY_BLADE")));
	TestEqual(TEXT("Three base-price parts deduct 30 shards"), RunSubsystem->TimeShards, 70);
	TestTrue(TEXT("Purchased rune enters part ownership"),
	         RunSubsystem->OwnedPartIds.Contains(TEXT("P_DAGGER_STRENGTH_GRIP")));
	TestFalse(TEXT("Purchased rune stays out of ordinary item inventory"),
	          RunSubsystem->InventoryItems.Contains(TEXT("P_DAGGER_STRENGTH_GRIP")));
	TestTrue(TEXT("Purchases do not auto-replace committed equipment"),
	         RunSubsystem->CurrentBuild.EquippedParts.IsEmpty());
	TestFalse(TEXT("Duplicate rune purchase is rejected"),
	          RunSubsystem->PurchaseShopItem(TEXT("P_DAGGER_STRENGTH_GRIP")));
	TestEqual(TEXT("Rejected duplicate is atomic"), RunSubsystem->TimeShards, 70);

	TestTrue(TEXT("Owned core, grip and blade commit as one loadout"),
	         RunSubsystem->TrySaveWeaponPartLoadout(
	             {TEXT("P_CORE_FLAME"), TEXT("P_DAGGER_STRENGTH_GRIP"), TEXT("P_DAGGER_HOLY_BLADE")}, Error));
	TestEqual(
	    TEXT("Committed loadout contains all three slot groups"), RunSubsystem->CurrentBuild.EquippedParts.Num(), 3);
	TestEqual(
	    TEXT("Committed strength grip applies its runtime effect"), RunSubsystem->CurrentBuild.Stats.AttackSpeed, 1.2f);
	return true;
}
#endif
