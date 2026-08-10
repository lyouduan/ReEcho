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
#endif
