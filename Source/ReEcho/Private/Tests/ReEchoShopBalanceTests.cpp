#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoRunSubsystem.h"
#include "UI/ReEchoIndexedButton.h"
#include "UI/ReEchoInventoryShopWidget.h"
#include "UI/ReEchoTraitCardChoiceWidget.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoShopBalanceTransactionsTest,
                                 "ReEcho.Shop.BalanceTransactions",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoShopBalanceTransactionsTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	Run->Phase = EReEchoRunPhase::Shop;
	Run->DebugSetTimeShards(100);
	TArray<FReEchoTimeShardBalance> BeforeEvents;
	TArray<FReEchoTimeShardBalance> AfterEvents;
	bool bSawCompletedPurchase = false;
	bool bSawCompletedGrant = false;
	const FDelegateHandle Handle = Run->OnTimeShardBalanceChanged.AddLambda(
	    [&](const FReEchoTimeShardBalance& Before, const FReEchoTimeShardBalance& After)
	    {
		    BeforeEvents.Add(Before);
		    AfterEvents.Add(After);
		    TestTrue(TEXT("Listener receives the authoritative final state"), After == Run->GetTimeShardBalance());
		    bSawCompletedPurchase |= Run->InventoryItems.Contains(TEXT("SHOP_RUSTED_SCISSORS"));
		    bSawCompletedGrant |= Run->CurrentBuild.CardState.OwnedCardIds.Contains(TEXT("G_2_22")) &&
		                          Run->OwnedPartIds.IsEmpty() &&
		                          Run->CurrentBuild.CardState.Runtime.FreeShopRefreshes > 0;
	    });
	Run->DebugSetTimeShards(100);
	Run->DebugAddTimeShards(0);
	TestEqual(TEXT("Unchanged GM commands do not broadcast"), AfterEvents.Num(), 0);
	TestTrue(TEXT("Purchase succeeds"), Run->PurchaseShopItemDetailed(TEXT("SHOP_RUSTED_SCISSORS")).IsSuccess());
	TestEqual(TEXT("One purchase emits one final event"), AfterEvents.Num(), 1);
	TestEqual(TEXT("Purchase before value"), BeforeEvents.Last().Cash, 100);
	TestEqual(TEXT("Purchase after value"), AfterEvents.Last().Cash, 85);
	TestTrue(TEXT("Purchase inventory is committed before broadcast"), bSawCompletedPurchase);
	TestFalse(TEXT("Duplicate purchase fails"),
	          Run->PurchaseShopItemDetailed(TEXT("SHOP_RUSTED_SCISSORS")).IsSuccess());
	TestFalse(TEXT("Unknown purchase fails"), Run->PurchaseShopItemDetailed(TEXT("UNKNOWN")).IsSuccess());
	TestFalse(TEXT("Unknown card fails"), Run->DebugGrantCard(TEXT("UNKNOWN")));
	TestEqual(TEXT("Rejected transactions do not broadcast"), AfterEvents.Num(), 1);

	TestTrue(TEXT("Economic GM card is granted"), Run->DebugGrantCard(TEXT("G_2_22")));
	TestEqual(TEXT("Nested grant produces one event"), AfterEvents.Num(), 2);
	TestTrue(TEXT("Grant notification observes rune clearing and refresh effects too"), bSawCompletedGrant);
	TestTrue(TEXT("Grant increases shards"), AfterEvents.Last().Cash > BeforeEvents.Last().Cash);
	Run->DebugSetTimeShards(-100);
	TestEqual(TEXT("GM setter preserves nonnegative clamp"), Run->TimeShards, 0);
	Run->DebugAddTimeShards(MAX_int32);
	Run->DebugAddTimeShards(MAX_int32);
	TestEqual(TEXT("GM add preserves saturation"), Run->TimeShards, MAX_int32);
	const int32 EventsBeforeReset = AfterEvents.Num();
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	TestEqual(TEXT("New run emits one final reset"), AfterEvents.Num(), EventsBeforeReset + 1);
	TestEqual(TEXT("Reset has no cash"), AfterEvents.Last().Cash, 0);
	TestEqual(TEXT("Reset has no debt"), AfterEvents.Last().Debt, 0);

	Run->Phase = EReEchoRunPhase::Shop;
	Run->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_2_19"));
	int32 Count = AfterEvents.Num();
	TestTrue(TEXT("Bank purchase succeeds"), Run->PurchaseShopItemDetailed(TEXT("SHOP_OLD_COIN")).IsSuccess());
	TestEqual(TEXT("Debt-only purchase notifies once"), AfterEvents.Num(), Count + 1);
	TestEqual(TEXT("Debt reflected in display"), AfterEvents.Last().GetDisplayedBalance(), -25);
	UReEchoRunSaveGame* Save = Run->CreateSaveSnapshot();
	Save->TimeShards = 10;
	Save->CurrentBuild.CardState.Runtime.TimeShardDebt = 35;
	Count = AfterEvents.Num();
	TestTrue(TEXT("Restore accepts cash and debt"), Run->RestoreSaveSnapshot(*Save));
	TestEqual(TEXT("Equal net balance but different cash and debt still notifies"), AfterEvents.Num(), Count + 1);
	TestEqual(TEXT("Restore before net"), BeforeEvents.Last().GetDisplayedBalance(), -25);
	TestEqual(TEXT("Restore after net"), AfterEvents.Last().GetDisplayedBalance(), -25);
	Count = AfterEvents.Num();
	Save->SaveVersion = UReEchoRunSaveGame::CurrentSaveVersion + 1;
	TestFalse(TEXT("Future save is rejected"), Run->RestoreSaveSnapshot(*Save));
	TestEqual(TEXT("Failed restore does not broadcast"), AfterEvents.Num(), Count);
	TestTrue(TEXT("Repayment outside combat succeeds"), Run->GrantTimeShards(10));
	TestEqual(TEXT("Debt-only repayment notifies once"), AfterEvents.Num(), Count + 1);
	Run->BeginEncounter();
	Count = AfterEvents.Num();
	TestTrue(TEXT("Combat pickup continues working"), Run->GrantTimeShards(1));
	TestEqual(TEXT("Combat pickup is outside this event contract"), AfterEvents.Num(), Count);
	Run->CompleteEncounter(FReEchoRecording(), true, false);
	TestEqual(TEXT("Settlement debt interest notifies once after encounter"), AfterEvents.Num(), Count + 1);
	TestTrue(TEXT("Interest increases debt"), AfterEvents.Last().Debt > BeforeEvents.Last().Debt);
	Run->OnTimeShardBalanceChanged.Remove(Handle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoShopBalanceSubscriptionTest,
                                 "ReEcho.UI.Shop.BalanceSubscription",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoShopBalanceSubscriptionTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	Run->Phase = EReEchoRunPhase::Shop;
	Run->DebugSetTimeShards(500);
	Run->EncounterIndex = 2;
	UClass* ShopClass = LoadClass<UReEchoInventoryShopWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen.WBP_ReEchoInventoryShopScreen_C"));
	if (!TestNotNull(TEXT("Authored shop class loads"), ShopClass))
	{
		return false;
	}
	UReEchoInventoryShopWidget* Widget = NewObject<UReEchoInventoryShopWidget>(GameInstance, ShopClass);
	Widget->Initialize();
	TSharedPtr<SWidget> Slate = Widget->TakeWidget();
	Widget->RefreshShopFromRun(Run, EReEchoInventoryShopMode::ManualShop);
	UTextBlock* Currency = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("DesignerCurrencyText")));
	if (!TestNotNull(TEXT("Authored balance label exists"), Currency))
	{
		return false;
	}
	const auto CheckBalance = [&]()
	{
		TestEqual(TEXT("Cash cache agrees with Run"), Widget->CurrentTimeShards, Run->TimeShards);
		TestEqual(TEXT("Debt cache agrees with Run"),
		          Widget->CurrentPartShopView.TimeShardDebt,
		          Run->GetTimeShardBalance().Debt);
		TestEqual(TEXT("Authored text shows final net balance"),
		          Currency->GetText().ToString(),
		          FText::Format(NSLOCTEXT("ReEcho", "TargetShopCurrency", "时间碎片：{0}"),
		                        FText::AsNumber(Run->GetDisplayedTimeShardBalance()))
		              .ToString());
	};
	CheckBalance();
	const FDelegateHandle InitialHandle = Widget->BalanceChangedHandle;
	Widget->RefreshShopFromRun(Run, EReEchoInventoryShopMode::ManualShop);
	TestTrue(TEXT("Content refresh does not duplicate subscription"), InitialHandle == Widget->BalanceChangedHandle);
	FSlateFontInfo Font = Currency->GetFont();
	Font.Size = 31;
	Currency->SetFont(Font);
	const FSlateColor Color(FLinearColor(0.8f, 0.2f, 0.4f, 0.7f));
	Currency->SetColorAndOpacity(Color);
	Currency->SetJustification(ETextJustify::Right);

	// Seed only the presentation cache to exercise the existing second-page geometry without changing card rules.
	Widget->CurrentPartShopView.OwnedCards.Reset();
	for (int32 Index = 0; Index < 13; ++Index)
	{
		FReEchoShopOffer Card;
		Card.ItemId = Card.ContentId = *FString::Printf(TEXT("BALANCE_TEST_CARD_%d"), Index);
		Card.Type = EReEchoShopOfferType::BuildCard;
		Card.Tier = 1;
		Widget->CurrentPartShopView.OwnedCards.Add(Card);
	}
	Widget->RebuildOwnedCardSlots();
	Widget->HandleOwnedCardPageRightClicked();
	TestEqual(TEXT("Second page selected"), Widget->ActiveOwnedCardPage, 1);
	Widget->BuildWeaponBackpackPopup();
	UWidget* Popup = Widget->WeaponBackpackPopupPanel;
	UWidget* OfferCard = Widget->DesignerPartOfferCards.IsEmpty() ? nullptr : Widget->DesignerPartOfferCards[0];
	UWidget* Tooltip = OfferCard ? OfferCard->GetToolTip() : nullptr;
	TArray<UWidget*> BeforeWidgets;
	Widget->WidgetTree->GetAllWidgets(BeforeWidgets);
	const FReEchoWeaponPartShopView BeforeView = Run->GetWeaponPartShopView();
	Run->DebugSetTimeShards(0);
	CheckBalance();
	TestEqual(TEXT("Pure balance event preserves second page"), Widget->ActiveOwnedCardPage, 1);
	TestTrue(TEXT("Weapon backpack remains open"), Popup && Popup->GetVisibility() != ESlateVisibility::Collapsed);
	TestTrue(TEXT("Existing offer tooltip is not replaced"), OfferCard && OfferCard->GetToolTip() == Tooltip);
	TArray<UWidget*> AfterWidgets;
	Widget->WidgetTree->GetAllWidgets(AfterWidgets);
	TestTrue(TEXT("Pure balance event never rebuilds widget tree"), BeforeWidgets == AfterWidgets);
	const FReEchoWeaponPartShopView PoorView = Run->GetWeaponPartShopView();
	TestEqual(TEXT("Slot count remains stable"), PoorView.SlotOffers.Num(), BeforeView.SlotOffers.Num());
	for (int32 Index = 0; Index < PoorView.SlotOffers.Num(); ++Index)
	{
		TestEqual(TEXT("Balance change does not reroll offer identity"),
		          PoorView.SlotOffers[Index].ItemId,
		          BeforeView.SlotOffers[Index].ItemId);
		TestEqual(TEXT("Authored buy button consumes Run eligibility"),
		          Widget->DesignerPartOfferBuyButtons[Index]->GetIsEnabled(),
		          PoorView.SlotOffers[Index].bCanPurchase);
	}
	TestEqual(TEXT("Currency font size preserved"), static_cast<int32>(Currency->GetFont().Size), 31);
	TestTrue(TEXT("Currency color preserved"), Currency->GetColorAndOpacity() == Color);
	const FByteProperty* Justification = FindFProperty<FByteProperty>(Currency->GetClass(), TEXT("Justification"));
	TestTrue(TEXT("Currency alignment preserved"),
	         Justification && Justification->GetPropertyValue_InContainer(Currency) == ETextJustify::Right);

	Run->DebugSetTimeShards(500);
	const FReEchoWeaponPartShopView View = Run->GetWeaponPartShopView();
	const FReEchoShopCardPackOffer* Available = View.CardPackOffers.FindByPredicate(
	    [](const FReEchoShopCardPackOffer& Pack)
	    {
		    return Pack.IsAvailable() && Pack.bCanPurchase;
	    });
	if (!TestNotNull(TEXT("At least one card pack is purchasable"), Available))
	{
		return false;
	}
	const int32 Tier = Available->Tier;
	TestTrue(TEXT("Pay before opening choices"), Run->PurchaseShopCardPackDetailed(Tier).IsSuccess());
	CheckBalance();
	TestEqual(TEXT("Shop projects pending-choice status immediately"),
	          Widget->CurrentPartShopView.CardPackOffers[Tier - 1].Status,
	          EReEchoShopCardPackStatus::PaidPendingChoice);
	UReEchoTraitCardChoiceWidget* Choice = NewObject<UReEchoTraitCardChoiceWidget>(GameInstance);
	Choice->Initialize();
	Choice->TakeWidget();
	Choice->InitializeShopOffers(Run->GetWeaponPartShopView().CardPackOffers[Tier - 1].Choices, Run->TimeShards, Tier);
	FString Error;
	TestTrue(TEXT("Paid choice refresh succeeds"), Run->TryRefreshShopCardSlot(Tier, 0, Error));
	CheckBalance();
	// Removing the layered view must not unsubscribe the underlying shop. GameMode cancellation only closes/focuses it.
	Choice->RemoveFromParent();
	TestTrue(TEXT("Shop remains subscribed after the choice view closes"),
	         Run->OnTimeShardBalanceChanged.IsBoundToObject(Widget));
	CheckBalance();
	TestEqual(TEXT("Nested choices never reset underlying card page"), Widget->ActiveOwnedCardPage, 1);
	TestTrue(TEXT("Nested balance changes keep weapon popup alive"),
	         Popup->GetVisibility() != ESlateVisibility::Collapsed);
	Run->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_2_19"));
	Run->DebugSetTimeShards(0);
	TestTrue(TEXT("Credit purchase updates UI"), Run->PurchaseShopItemDetailed(TEXT("SHOP_OLD_COIN")).IsSuccess());
	CheckBalance();
	TestTrue(TEXT("Non-combat repayment updates UI"), Run->GrantTimeShards(10));
	CheckBalance();

	// Keep Slate alive: closing must unsubscribe immediately, not wait for its destruction.
	Widget->RemoveFromParent();
	TestFalse(TEXT("Closing removes balance subscription"), Run->OnTimeShardBalanceChanged.IsBoundToObject(Widget));
	const int32 ClosedCash = Widget->CurrentTimeShards;
	Run->DebugSetTimeShards(333);
	TestEqual(TEXT("Closed widget receives no updates"), Widget->CurrentTimeShards, ClosedCash);
	Widget->RefreshShopFromRun(Run, EReEchoInventoryShopMode::ManualShop);
	CheckBalance();
	TestTrue(TEXT("Reopening installs a new subscription"), Widget->BalanceChangedHandle.IsValid());
	const FDelegateHandle ReopenedHandle = Widget->BalanceChangedHandle;
	Widget->RefreshShopFromRun(Run, EReEchoInventoryShopMode::ManualShop);
	TestTrue(TEXT("Repeated opening remains idempotent"), Widget->BalanceChangedHandle == ReopenedHandle);
	Widget->NativeDestruct();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoShopBalanceOfferCommandsTest,
                                 "ReEcho.Shop.BalanceOfferCommands",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoShopBalanceOfferCommandsTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	Run->EncounterIndex = 2;
	Run->Phase = EReEchoRunPhase::Shop;
	Run->DebugSetTimeShards(1000);
	int32 EventCount = 0;
	FReEchoTimeShardBalance LastBefore;
	FReEchoTimeShardBalance LastAfter;
	const FDelegateHandle Handle = Run->OnTimeShardBalanceChanged.AddLambda(
	    [&](const FReEchoTimeShardBalance& Before, const FReEchoTimeShardBalance& After)
	    {
		    ++EventCount;
		    LastBefore = Before;
		    LastAfter = After;
		    TestTrue(TEXT("Offer command callback observes final balance"), After == Run->GetTimeShardBalance());
	    });
	const auto CheckDelta = [&](const TCHAR* Label, const FReEchoTimeShardBalance Before, const int32 CountBefore)
	{
		const FReEchoTimeShardBalance After = Run->GetTimeShardBalance();
		const bool bChanged = !(Before == After);
		TestEqual(Label, EventCount - CountBefore, bChanged ? 1 : 0);
		if (bChanged)
		{
			TestTrue(TEXT("Event captures the whole command delta"), LastBefore == Before && LastAfter == After);
		}
	};
	FReEchoWeaponPartShopView View = Run->GetWeaponPartShopView();
	const FReEchoWeaponSlotOffer* Rune = View.SlotOffers.FindByPredicate(
	    [](const FReEchoWeaponSlotOffer& Offer)
	    {
		    return Offer.bCanPurchase;
	    });
	if (!TestNotNull(TEXT("A real weapon/rune offer is available"), Rune))
	{
		return false;
	}
	FReEchoTimeShardBalance Before = Run->GetTimeShardBalance();
	int32 CountBefore = EventCount;
	TestTrue(TEXT("Real weapon/rune purchase succeeds"), Run->PurchaseShopItemDetailed(Rune->ItemId).IsSuccess());
	CheckDelta(TEXT("Real weapon/rune purchase is one balance transaction"), Before, CountBefore);
	FString Error;
	Before = Run->GetTimeShardBalance();
	CountBefore = EventCount;
	TestTrue(TEXT("Paid main refresh succeeds"), Run->TryRefreshWeaponRuneShop(Error));
	CheckDelta(TEXT("Paid main refresh is one balance transaction"), Before, CountBefore);
	Run->CurrentBuild.CardState.Runtime.FreeShopRefreshes = 1;
	Before = Run->GetTimeShardBalance();
	CountBefore = EventCount;
	TestTrue(TEXT("Free main refresh succeeds"), Run->TryRefreshWeaponRuneShop(Error));
	CheckDelta(TEXT("Free main refresh does not invent a balance change"), Before, CountBefore);
	Before = Run->GetTimeShardBalance();
	CountBefore = EventCount;
	TestTrue(TEXT("Tier-one pack payment succeeds"), Run->PurchaseShopCardPackDetailed(1).IsSuccess());
	CheckDelta(TEXT("Pack payment is one balance transaction"), Before, CountBefore);
	Before = Run->GetTimeShardBalance();
	CountBefore = EventCount;
	TestTrue(TEXT("Paid pack slot refresh succeeds"), Run->TryRefreshShopCardSlot(1, 0, Error));
	CheckDelta(TEXT("Paid pack slot refresh is one balance transaction"), Before, CountBefore);
	View = Run->GetWeaponPartShopView();
	if (!TestTrue(TEXT("Paid pack still has choices"), !View.CardPackOffers[0].Choices.IsEmpty()))
	{
		return false;
	}
	Before = Run->GetTimeShardBalance();
	CountBefore = EventCount;
	TestTrue(TEXT("Paid pack claim succeeds"),
	         Run->ClaimPaidShopCardChoice(View.CardPackOffers[0].Choices[0].ItemId).IsSuccess());
	CheckDelta(TEXT("Card claim reports its final delta without a second charge"), Before, CountBefore);
	Run->Phase = EReEchoRunPhase::CardChoice;
	Run->ResetPendingTraitCardChoice();
	TestTrue(TEXT("Free encounter choices are generated"), !Run->GenerateTraitCardOffers(3).IsEmpty());
	Before = Run->GetTimeShardBalance();
	CountBefore = EventCount;
	TestTrue(TEXT("Free-choice paid slot refresh succeeds"), Run->TryRefreshTraitCardSlot(0, Error));
	CheckDelta(TEXT("Free-choice slot refresh is one balance transaction"), Before, CountBefore);
	const TArray<FReEchoTraitCardOffer> Choices = Run->GenerateTraitCardOffers(3);
	if (!TestTrue(TEXT("Refreshed free choices remain available"), !Choices.IsEmpty()))
	{
		return false;
	}
	Before = Run->GetTimeShardBalance();
	CountBefore = EventCount;
	TestTrue(TEXT("Normal free card grant succeeds"), Run->ApplyTraitCard(Choices[0].CardId));
	CheckDelta(TEXT("Normal grant reports only the final currency result"), Before, CountBefore);
	TestEqual(TEXT("Normal grant closes its phase before return"), Run->Phase, EReEchoRunPhase::Planning);
	Run->OnTimeShardBalanceChanged.Remove(Handle);
	return true;
}

#endif
