#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoRunSubsystem.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Cards/ReEchoCardRuntime.h"
#include "Cards/ReEchoCardTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoShopCardPackRequestEligibilityTest,
                                 "ReEcho.Shop.CardPackRequestEligibility",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoShopCardPackRequestEligibilityTest::RunTest(const FString& Parameters)
{
	FReEchoShopCardPackOffer UnpaidPack;
	UnpaidPack.Status = EReEchoShopCardPackStatus::Available;
	TestTrue(TEXT("An available unpaid pack can enter payment before candidates are rolled"),
	         UnpaidPack.Choices.IsEmpty() && UnpaidPack.CanRequestPurchaseOrOpenChoices());

	FReEchoShopCardPackOffer PendingWithoutCandidates = UnpaidPack;
	PendingWithoutCandidates.Status = EReEchoShopCardPackStatus::PaidPendingChoice;
	TestFalse(TEXT("A paid pack without candidates cannot open an empty choice screen"),
	          PendingWithoutCandidates.CanRequestPurchaseOrOpenChoices());

	FReEchoShopCardChoiceOffer Candidate;
	Candidate.CardId = TEXT("TEST_CARD");
	PendingWithoutCandidates.Choices.Add(Candidate);
	TestTrue(TEXT("A paid pack with candidates can reopen its choice screen"),
	         PendingWithoutCandidates.CanRequestPurchaseOrOpenChoices());

	FReEchoShopCardPackOffer NotOffered;
	NotOffered.Status = EReEchoShopCardPackStatus::NotOffered;
	TestFalse(TEXT("A tier that is not offered cannot enter the purchase flow"),
	          NotOffered.CanRequestPurchaseOrOpenChoices());
	return true;
}

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
	RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
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
	const int32 RewardShardsBeforeTrait = RunSubsystem->TimeShards;
	TestTrue(TEXT("Per-enemy rewards provide shop currency"),
	         RewardShardsBeforeTrait >= 24 && RewardShardsBeforeTrait <= 36);
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
	RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_2_16"));
	RunSubsystem->TimeShards = 50;
	const float InitialHpMax = RunSubsystem->CurrentBuild.Stats.HpMax;
	const float InitialHpPoint = RunSubsystem->CurrentBuild.Stats.HpPoint;
	int32 HealthCommitCount = 0;
	EReEchoHealthAdjustment LastHealthAdjustment = EReEchoHealthAdjustment::None;
	RunSubsystem->OnCardHealthCommitted.AddLambda(
	    [&HealthCommitCount, &LastHealthAdjustment](const FReEchoStatBlock&, const EReEchoHealthAdjustment Adjustment)
	    {
		    ++HealthCommitCount;
		    LastHealthAdjustment = Adjustment;
	    });
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = RunSubsystem->GetRunDataSnapshot();
	const FReEchoCardDefinition* Contract =
	    Snapshot.IsValid() && Snapshot->CardCatalog.IsValid() ? Snapshot->CardCatalog->Find(TEXT("G_2_16")) : nullptr;
	if (!TestNotNull(TEXT("Prosperity contract is present in the current catalog"), Contract))
	{
		return false;
	}
	const FReEchoCardEffectDefinition* Discount = Contract->Effects.FindByPredicate(
	    [](const FReEchoCardEffectDefinition& Effect)
	    {
		    return Effect.Target == TEXT("ShopDiscount");
	    });
	const FReEchoCardEffectDefinition* Growth = Contract->Effects.FindByPredicate(
	    [](const FReEchoCardEffectDefinition& Effect)
	    {
		    return Effect.Trigger == TEXT("OnPurchase");
	    });
	if (!TestNotNull(TEXT("Contract defines its discount"), Discount) ||
	    !TestNotNull(TEXT("Contract defines its purchase growth"), Growth))
	{
		return false;
	}
	const int32 ExpectedPrice = FMath::CeilToInt(15.0f * (1.0f - Discount->Value));
	const int32 ExpectedBalance = 50 - ExpectedPrice;

	TestEqual(TEXT("Prosperity contract shows the same rounded price that purchase charges"),
	          RunSubsystem->GetDiscountedShopPrice(15),
	          ExpectedPrice);
	TestTrue(TEXT("Discounted purchase succeeds"), RunSubsystem->PurchaseShopItem(TEXT("SHOP_RUSTED_SCISSORS")));
	TestEqual(
	    TEXT("Discounted purchase deducts the authored rounded price"), RunSubsystem->TimeShards, ExpectedBalance);
	TestEqual(TEXT("Successful purchase applies permanent maximum health growth"),
	          RunSubsystem->CurrentBuild.Stats.HpMax,
	          InitialHpMax + Growth->Value);
	TestEqual(TEXT("Successful purchase applies permanent current health growth"),
	          RunSubsystem->CurrentBuild.Stats.HpPoint,
	          InitialHpPoint + Growth->Value);
	TestEqual(TEXT("A shop-item purchase publishes one committed health update"), HealthCommitCount, 1);
	TestEqual(TEXT("The shop-item purchase uses the precise stat-point update"),
	          LastHealthAdjustment,
	          EReEchoHealthAdjustment::SetToStatPoint);

	RunSubsystem->CurrentBuild.CardState.Runtime.FreeShopRefreshes = 1;
	const FReEchoWeaponPartShopView InitialRefreshView = RunSubsystem->GetWeaponPartShopView();
	TestTrue(TEXT("Free refresh is consumed before currency"), RunSubsystem->TryConsumeShopRefresh(0));
	TestEqual(TEXT("Free refresh leaves currency unchanged"), RunSubsystem->TimeShards, ExpectedBalance);
	TestEqual(TEXT("Free refresh preserves the paid weapon/rune refresh budget"),
	          RunSubsystem->GetWeaponPartShopView().WeaponRuneRefreshesRemaining,
	          InitialRefreshView.WeaponRuneRefreshesRemaining);
	TestTrue(TEXT("The first paid weapon/rune refresh uses the configured cost"),
	         RunSubsystem->TryConsumeShopRefresh(0));
	TestEqual(TEXT("Paid weapon/rune refresh deducts the configured cost"),
	          RunSubsystem->TimeShards,
	          ExpectedBalance - InitialRefreshView.WeaponRuneRefreshCost);
	TestTrue(TEXT("The second paid weapon/rune refresh remains available after a free refresh"),
	         RunSubsystem->TryConsumeShopRefresh(0));
	TestEqual(TEXT("The second paid refresh deducts the configured cost"),
	          RunSubsystem->TimeShards,
	          ExpectedBalance - 2 * InitialRefreshView.WeaponRuneRefreshCost);
	TestFalse(TEXT("Per-encounter weapon/rune refresh limit prevents an infinite refresh"),
	          RunSubsystem->TryConsumeShopRefresh(0));

	UReEchoRunSubsystem* ResetTaskRun = NewObject<UReEchoRunSubsystem>(GameInstance);
	ResetTaskRun->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	TestTrue(TEXT("Reset Task grants through the authoritative run transaction"),
	         ResetTaskRun->DebugGrantCard(TEXT("G_2_22")));
	TestEqual(TEXT("Reset Task grants five free shop refreshes"),
	          ResetTaskRun->CurrentBuild.CardState.Runtime.FreeShopRefreshes,
	          5);
	const int32 PaidRefreshesBeforeResetTaskCredits =
	    ResetTaskRun->GetWeaponPartShopView().WeaponRuneRefreshesRemaining;
	for (int32 RefreshIndex = 0; RefreshIndex < 5; ++RefreshIndex)
	{
		TestTrue(*FString::Printf(TEXT("Reset Task free refresh %d is usable"), RefreshIndex + 1),
		         ResetTaskRun->TryConsumeShopRefresh(0));
	}
	TestEqual(TEXT("Reset Task consumes all five free refreshes"),
	          ResetTaskRun->CurrentBuild.CardState.Runtime.FreeShopRefreshes,
	          0);
	TestEqual(TEXT("Reset Task free refreshes do not consume the paid per-encounter budget"),
	          ResetTaskRun->GetWeaponPartShopView().WeaponRuneRefreshesRemaining,
	          PaidRefreshesBeforeResetTaskCredits);

	RunSubsystem->EncounterIndex = 2;
	RunSubsystem->TimeShards = 100;
	const FReEchoWeaponPartShopView CardPackPage = RunSubsystem->GetWeaponPartShopView();
	if (!TestTrue(TEXT("Purchase-trigger fixture exposes a tier-one pack"),
	              CardPackPage.CardPackOffers[0].IsAvailable()))
	{
		return false;
	}
	TestTrue(TEXT("Card-pack payment succeeds"), RunSubsystem->PurchaseShopCardPackDetailed(1).IsSuccess());
	TestEqual(TEXT("Card-pack payment publishes the same unified health update"), HealthCommitCount, 2);
	TestEqual(TEXT("The card-pack payment uses the precise stat-point update"),
	          LastHealthAdjustment,
	          EReEchoHealthAdjustment::SetToStatPoint);
	const FReEchoWeaponPartShopView PaidCardPackPage = RunSubsystem->GetWeaponPartShopView();
	if (!TestTrue(TEXT("Payment generates at least one tier-one candidate"),
	              !PaidCardPackPage.CardPackOffers[0].Choices.IsEmpty()))
	{
		return false;
	}
	const FName PaidChoiceId = PaidCardPackPage.CardPackOffers[0].Choices[0].ItemId;
	const FReEchoCardOutcomeState* OutcomeAfterPayment =
	    RunSubsystem->CurrentBuild.CardState.Runtime.ResolvedOutcomes.FindByPredicate(
	        [](const FReEchoCardOutcomeState& Outcome)
	        {
		        return Outcome.CardId == TEXT("G_2_16") && Outcome.Kind == EReEchoCardOutcomeKind::CumulativeStatGain;
	        });
	const float PurchaseGrowthAfterPayment = OutcomeAfterPayment ? OutcomeAfterPayment->PrimaryValue : 0.0f;
	TestEqual(TEXT("Prepaying a card pack fires the owned OnPurchase card exactly once"),
	          PurchaseGrowthAfterPayment,
	          2.0f * Growth->Value);
	TestTrue(TEXT("The paid card can be claimed"), RunSubsystem->ClaimPaidShopCardChoice(PaidChoiceId).IsSuccess());
	const FReEchoCardOutcomeState* OutcomeAfterClaim =
	    RunSubsystem->CurrentBuild.CardState.Runtime.ResolvedOutcomes.FindByPredicate(
	        [](const FReEchoCardOutcomeState& Outcome)
	        {
		        return Outcome.CardId == TEXT("G_2_16") && Outcome.Kind == EReEchoCardOutcomeKind::CumulativeStatGain;
	        });
	TestTrue(TEXT("Final card claim does not fire OnPurchase a second time"),
	         OutcomeAfterClaim && FMath::IsNearlyEqual(OutcomeAfterClaim->PrimaryValue, PurchaseGrowthAfterPayment));

	RunSubsystem->CurrentBuild.CardState.Runtime.FreeShopRefreshes = 1;
	RunSubsystem->CurrentBuild.CardState.Runtime.EconomyPenalty = EReEchoCardEconomyPenalty::NoShopRefresh;
	TestFalse(TEXT("Permanent no-refresh penalty blocks even a free refresh"), RunSubsystem->TryConsumeShopRefresh(10));
	TestEqual(TEXT("Blocked refresh does not consume the free count"),
	          RunSubsystem->CurrentBuild.CardState.Runtime.FreeShopRefreshes,
	          1);
	RunSubsystem->CurrentBuild.CardState.Runtime.EconomyPenalty = EReEchoCardEconomyPenalty::NoExtraCardPurchase;
	TestFalse(TEXT("Permanent extra-card penalty closes the purchase gate"), RunSubsystem->CanPurchaseExtraShopCard());
	RunSubsystem->EncounterIndex = 3;
	RunSubsystem->TimeShards = 100;
	const int32 ShardsBeforeBlockedPack = RunSubsystem->TimeShards;
	TestEqual(TEXT("NoExtraCardPurchase rejects card-pack payment with a structured result"),
	          RunSubsystem->PurchaseShopCardPackDetailed(1).Result,
	          EReEchoShopPurchaseResult::PurchaseDisabled);
	TestEqual(
	    TEXT("A rejected card-pack payment preserves currency"), RunSubsystem->TimeShards, ShardsBeforeBlockedPack);
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
	int32 SearchEncounterIndex = 1;
	auto FindAndBuy = [&](const FName PartId)
	{
		for (int32 EncounterAttempt = 0; EncounterAttempt < 32; ++EncounterAttempt)
		{
			RunSubsystem->EncounterIndex = SearchEncounterIndex++;
			for (int32 PageIndex = 0; PageIndex <= 2; ++PageIndex)
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
				if (PageIndex < 2 && !RunSubsystem->TryRefreshWeaponRuneShop(Error))
				{
					break;
				}
			}
		}
		return false;
	};
	RunSubsystem->TimeShards = 1000;
	TestTrue(TEXT("Core purchase succeeds when it is on the current page"), FindAndBuy(TEXT("P_CORE_FLAME")));
	TestTrue(TEXT("Arrowhead purchase succeeds when it is on the current page"),
	         FindAndBuy(TEXT("P_BOW_SPLIT_ARROWHEAD_I")));
	TestTrue(TEXT("Purchased rune enters part ownership"), RunSubsystem->OwnedPartIds.Contains(TEXT("P_CORE_FLAME")));
	TestFalse(TEXT("Purchased rune stays out of ordinary item inventory"),
	          RunSubsystem->InventoryItems.Contains(TEXT("P_CORE_FLAME")));
	TestTrue(TEXT("Purchased core fills its empty equipment slot"),
	         RunSubsystem->CurrentBuild.EquippedParts.ContainsByPredicate(
	             [](const FReEchoEquippedPartSnapshot& Part)
	             {
		             return Part.PartId == TEXT("P_CORE_FLAME");
	             }));
	TestTrue(TEXT("Purchased weapon-specific rune fills its empty equipment slot"),
	         RunSubsystem->CurrentBuild.EquippedParts.ContainsByPredicate(
	             [](const FReEchoEquippedPartSnapshot& Part)
	             {
		             return Part.PartId == TEXT("P_BOW_SPLIT_ARROWHEAD_I");
	             }));
	const int32 ShardsAfterDuplicate = RunSubsystem->TimeShards;
	TestFalse(TEXT("Duplicate rune purchase is rejected"), RunSubsystem->PurchaseShopItem(TEXT("P_CORE_FLAME")));
	TestEqual(TEXT("Rejected duplicate is atomic"), RunSubsystem->TimeShards, ShardsAfterDuplicate);

	// Plan85 Step 3 regression: an owned rune must not be re-offered after a manual shop refresh.
	RunSubsystem->TimeShards = 1000;
	for (int32 EncounterOffset = 0; EncounterOffset < 4; ++EncounterOffset)
	{
		RunSubsystem->EncounterIndex = 100 + EncounterOffset;
		for (int32 PageIndex = 0; PageIndex <= 2; ++PageIndex)
		{
			const FReEchoWeaponPartShopView Page = RunSubsystem->GetWeaponPartShopView();
			TestFalse(TEXT("Owned rune is excluded from every weapon/rune page"),
			          Page.Offers.ContainsByPredicate(
			              [](const FReEchoShopOffer& Offer)
			              {
				              return Offer.Type == EReEchoShopOfferType::WeaponPart &&
				                     Offer.ContentId == TEXT("P_CORE_FLAME");
			              }));
			if (PageIndex < 2)
			{
				TestTrue(TEXT("Each encounter permits both configured weapon/rune refreshes"),
				         RunSubsystem->TryRefreshWeaponRuneShop(Error));
			}
		}
	}

	// The public equip operation remains available for owned/backpack selection and idempotent re-commit.
	TestTrue(TEXT("Core and arrowhead equip as one loadout"),
	         RunSubsystem->TryEquipParts({TEXT("P_CORE_FLAME"), TEXT("P_BOW_SPLIT_ARROWHEAD_I")}, Error));
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

	TSet<FName> PurchasedRuneIds;
	for (const FReEchoWeaponSlotOffer& OriginalOffer : InitialPage.SlotOffers)
	{
		TestTrue(TEXT("Every original weapon/rune offer remains purchasable after earlier purchases"),
		         RunSubsystem->PurchaseShopItem(OriginalOffer.ItemId));
		if (OriginalOffer.Kind == EReEchoShopOfferKind::Part)
		{
			PurchasedRuneIds.Add(OriginalOffer.ItemId);
		}
		const FReEchoWeaponPartShopView PageAfterPurchase = RunSubsystem->GetWeaponPartShopView();
		if (OriginalOffer.Kind == EReEchoShopOfferKind::Part)
		{
			TestTrue(TEXT("A purchased rune is recorded in the authoritative owned inventory"),
			         RunSubsystem->OwnedPartIds.Contains(OriginalOffer.PartId));
			const bool bCompatibleWithCurrentWeapon = OriginalOffer.WeaponTypeId == TEXT("Any") ||
			                                          OriginalOffer.WeaponTypeId == PageAfterPurchase.WeaponTypeId;
			TestEqual(TEXT("The owned-rune projection only exposes runes compatible with the current weapon"),
			          PageAfterPurchase.OwnedParts.ContainsByPredicate(
			              [&](const FReEchoShopOffer& OwnedPart)
			              {
				              return OwnedPart.ContentId == OriginalOffer.PartId;
			              }),
			          bCompatibleWithCurrentWeapon);
		}
		else
		{
			TestTrue(TEXT("A purchased weapon is immediately present in the refreshed owned-weapon projection"),
			         PageAfterPurchase.OwnedWeapons.Contains(OriginalOffer.WeaponId));
		}
		for (int32 SlotIndex = 0; SlotIndex < InitialPage.SlotOffers.Num(); ++SlotIndex)
		{
			TestEqual(TEXT("Purchase never rerolls cached content IDs"),
			          RunSubsystem->CreateSaveSnapshot()->WeaponPartShopOfferIds[SlotIndex],
			          InitialPage.SlotOffers[SlotIndex].ItemId);
			if (PurchasedRuneIds.Contains(InitialPage.SlotOffers[SlotIndex].ItemId))
			{
				TestTrue(TEXT("Consumed rune is recorded on the authoritative page"),
				         RunSubsystem->PurchasedWeaponPartOfferIds.Contains(InitialPage.SlotOffers[SlotIndex].ItemId));
				TestTrue(TEXT("A rune bought on this page leaves an empty slot until refresh"),
				         PageAfterPurchase.SlotOffers[SlotIndex].ItemId.IsNone());
				TestEqual(TEXT("A consumed rune slot has no price"), PageAfterPurchase.SlotOffers[SlotIndex].Price, 0);
				TestFalse(TEXT("A consumed rune slot cannot be purchased"),
				          PageAfterPurchase.SlotOffers[SlotIndex].bCanPurchase);
				continue;
			}
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoFreshRunShopSeedTest,
                                 "ReEcho.Shop.FreshRunsUseDistinctOfferSeeds",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoFreshRunShopSeedTest::RunTest(const FString& Parameters)
{
	TSet<int32> RunSeeds;
	TSet<FString> ShopSignatures;
	constexpr int32 SampleRunCount = 16;
	for (int32 RunIndex = 0; RunIndex < SampleRunCount; ++RunIndex)
	{
		UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
		UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
		RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_09"));

		const UReEchoRunSaveGame* Snapshot = RunSubsystem->CreateSaveSnapshot();
		if (!TestNotNull(TEXT("Fresh run produces a save snapshot"), Snapshot))
		{
			return false;
		}
		RunSeeds.Add(Snapshot->RunSeed);

		FString Signature;
		for (const FReEchoWeaponSlotOffer& Offer : RunSubsystem->GetWeaponPartShopView().SlotOffers)
		{
			Signature += Offer.ItemId.ToString();
			Signature += TEXT("|");
		}
		ShopSignatures.Add(MoveTemp(Signature));
	}

	TestTrue(TEXT("Fresh runs do not reuse one deterministic random root"), RunSeeds.Num() > 1);
	TestTrue(TEXT("Identical run inputs can produce more than one initial weapon/rune shop page"),
	         ShopSignatures.Num() > 1);
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
	for (int32 EncounterAttempt = 0; EncounterAttempt < 64 && PurchasedWeaponId.IsNone(); ++EncounterAttempt)
	{
		RunSubsystem->EncounterIndex = EncounterAttempt + 1;
		for (int32 PageIndex = 0; PageIndex <= 2 && PurchasedWeaponId.IsNone(); ++PageIndex)
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
			if (PageIndex < 2 && !RunSubsystem->TryRefreshWeaponRuneShop(Error))
			{
				break;
			}
		}
	}
	TestFalse(TEXT("The deterministic shop sequence eventually exposes a weapon"), PurchasedWeaponId.IsNone());
	TestTrue(TEXT("Purchased weapon enters the owned weapon backpack"),
	         !PurchasedWeaponId.IsNone() && RunSubsystem->OwnedWeaponIds.Contains(PurchasedWeaponId));
	TestTrue(TEXT("Purchasing another weapon keeps the starting weapon owned"),
	         RunSubsystem->OwnedWeaponIds.Contains(TEXT("W_J_08")));
	TestTrue(TEXT("The starting weapon can be re-equipped after a weapon purchase"),
	         RunSubsystem->TryEquipOwnedWeapon(TEXT("W_J_08"), Error));
	RunSubsystem->OwnedWeaponIds.Add(TEXT("W_J_01"));

	Error.Reset();
	TestTrue(TEXT("Bow accepts a universal core and bow-specific arrowhead"),
	         RunSubsystem->TryEquipParts({TEXT("P_CORE_FLAME"), TEXT("P_BOW_SPLIT_ARROWHEAD_I")}, Error));
	TestTrue(TEXT("An owned alternate weapon can be equipped for free"),
	         RunSubsystem->TryEquipOwnedWeapon(TEXT("W_J_01"), Error));
	TestEqual(TEXT("Owned weapon selection updates the authoritative build"),
	          RunSubsystem->CurrentBuild.WeaponId,
	          FName(TEXT("W_J_01")));
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
		              return Part.PartId == TEXT("P_BOW_SPLIT_ARROWHEAD_I");
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
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = RunSubsystem->GetRunDataSnapshot();
	if (!TestTrue(TEXT("Card-pack test has current data and card catalog"),
	              Snapshot.IsValid() && Snapshot->CardCatalog.IsValid()))
	{
		return false;
	}
	const TArray<TArray<int32>> ExpectedShopTiers = {{}, {1}, {1}, {1, 2, 3}, {1, 3}, {1, 2}, {1, 3}, {}};
	const TArray<int32> ExpectedTierOneQuantities = {0, 3, 3, 3, 3, 3, 10, 0};
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
				const int32 ExpectedQuantity = Pack.Tier == 1 ? ExpectedTierOneQuantities[EncounterIndex - 1] : 1;
				TestEqual(
				    TEXT("Each pack exposes its configured purchase quantity"), Pack.TotalPurchases, ExpectedQuantity);
				TestEqual(TEXT("A fresh pack starts with all configured purchases"),
				          Pack.RemainingPurchases,
				          ExpectedQuantity);
				TestTrue(TEXT("An unpaid offered pack defers all choices until payment"), Pack.Choices.IsEmpty());
				const FReEchoCsvShopPriceRangeRow* PriceRange =
				    Snapshot->ShopPriceRanges.Find(FName(*FString::Printf(TEXT("Card_T%d"), Pack.Tier)));
				if (!TestNotNull(TEXT("Each pack tier has an authored price range"), PriceRange))
				{
					return false;
				}
				TestTrue(TEXT("Each pack carries one configured tier price"),
				         Pack.Price >= PriceRange->MinPrice && Pack.Price <= PriceRange->MaxPrice);
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
	TArray<FName> TierOneCardIds;
	for (const FReEchoCardDefinition& Card : Snapshot->CardCatalog->GetOfferable(TEXT("Trait"), 1))
	{
		TierOneCardIds.Add(Card.Id);
	}
	RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Append(TierOneCardIds);
	RunSubsystem->EncounterIndex = 3;
	const FReEchoWeaponPartShopView OwnedFilterPage = RunSubsystem->GetWeaponPartShopView();
	if (!TestEqual(TEXT("A fully-owned tier-one pool keeps the three fixed packs"),
	               OwnedFilterPage.CardPackOffers.Num(),
	               ReEchoShopOfferCountPerGroup))
	{
		return false;
	}
	TestTrue(TEXT("The tier-one pack remains available from fully-owned repeatable state"),
	         OwnedFilterPage.CardPackOffers[0].IsAvailable());
	TestTrue(TEXT("The repeatable tier-one pack remains unrolled before payment"),
	         OwnedFilterPage.CardPackOffers[0].Choices.IsEmpty());
	TestEqual(TEXT("The unconfigured tier-two pack stays unoffered"),
	          OwnedFilterPage.CardPackOffers[1].Status,
	          EReEchoShopCardPackStatus::NotOffered);
	TestTrue(TEXT("An owned tier-one pack can be prepaid"), RunSubsystem->PurchaseShopCardPackDetailed(1).IsSuccess());
	const FReEchoShopCardPackOffer PaidTierOnePack = RunSubsystem->GetWeaponPartShopView().CardPackOffers[0];
	if (!TestTrue(TEXT("Payment rolls the repeatable tier-one pack"), !PaidTierOnePack.Choices.IsEmpty()))
	{
		return false;
	}
	const FReEchoShopCardChoiceOffer RepeatedTierOneOffer = PaidTierOnePack.Choices[0];
	const int32 TierOneStackCountBefore =
	    ReEchoCardRuntime::CountOwned(RunSubsystem->CurrentBuild.CardState, RepeatedTierOneOffer.CardId);
	TestTrue(TEXT("An owned tier-one card can be claimed from its paid pack"),
	         RunSubsystem->ClaimPaidShopCardChoice(RepeatedTierOneOffer.ItemId).IsSuccess());
	TestEqual(TEXT("Repeated tier-one shop purchase adds one stack"),
	          ReEchoCardRuntime::CountOwned(RunSubsystem->CurrentBuild.CardState, RepeatedTierOneOffer.CardId),
	          TierOneStackCountBefore + 1);
	const FReEchoWeaponPartShopView ConsumedTierOnePage = RunSubsystem->GetWeaponPartShopView();
	TestEqual(TEXT("A multi-purchase pack becomes available again after one completed purchase"),
	          ConsumedTierOnePage.CardPackOffers[0].Status,
	          EReEchoShopCardPackStatus::Available);
	TestEqual(TEXT("A completed purchase consumes exactly one configured pack"),
	          ConsumedTierOnePage.CardPackOffers[0].RemainingPurchases,
	          OwnedFilterPage.CardPackOffers[0].RemainingPurchases - 1);
	if (PaidTierOnePack.Choices.Num() > 1)
	{
		TestFalse(TEXT("A second choice from the same pack is rejected"),
		          RunSubsystem->ClaimPaidShopCardChoice(PaidTierOnePack.Choices[1].ItemId).IsSuccess());
	}
	// Each remaining pack requires a fresh payment; only the last claim closes the tier for this encounter.
	for (int32 Remaining = ConsumedTierOnePage.CardPackOffers[0].RemainingPurchases; Remaining > 0; --Remaining)
	{
		const FReEchoShopCardPackOffer NextUnpaidPack = RunSubsystem->GetWeaponPartShopView().CardPackOffers[0];
		TestTrue(TEXT("A remaining tier-one pack stays unrolled before payment"), NextUnpaidPack.Choices.IsEmpty());
		if (!TestTrue(TEXT("Each remaining pack must be paid"),
		              RunSubsystem->PurchaseShopCardPackDetailed(1).IsSuccess()))
		{
			return false;
		}
		const FReEchoShopCardPackOffer NextPaidPack = RunSubsystem->GetWeaponPartShopView().CardPackOffers[0];
		if (!TestTrue(TEXT("Each paid remaining pack generates choices"), !NextPaidPack.Choices.IsEmpty()))
		{
			return false;
		}
		const FName NextChoiceId = NextPaidPack.Choices[0].ItemId;
		TestTrue(TEXT("Each paid remaining pack can be claimed"),
		         RunSubsystem->ClaimPaidShopCardChoice(NextChoiceId).IsSuccess());
		const FReEchoShopCardPackOffer AfterClaim = RunSubsystem->GetWeaponPartShopView().CardPackOffers[0];
		TestEqual(
		    TEXT("Each repeated purchase consumes one remaining pack"), AfterClaim.RemainingPurchases, Remaining - 1);
		TestEqual(TEXT("Only the final purchase marks the pack purchased"),
		          AfterClaim.Status,
		          Remaining > 1 ? EReEchoShopCardPackStatus::Available : EReEchoShopCardPackStatus::Purchased);
	}
	TestFalse(TEXT("An exhausted tier cannot be paid again"),
	          RunSubsystem->PurchaseShopCardPackDetailed(1).IsSuccess());

	RunSubsystem->EncounterIndex = 4;
	RunSubsystem->TimeShards = 1000;
	const FReEchoWeaponPartShopView FirstPage = RunSubsystem->GetWeaponPartShopView();
	TestEqual(
	    TEXT("First page has three fixed weapon/part slots"), FirstPage.SlotOffers.Num(), ReEchoShopOfferCountPerGroup);
	TestEqual(TEXT("Encounter four projects all three fixed card packs"),
	          FirstPage.CardPackOffers.Num(),
	          ReEchoShopOfferCountPerGroup);
	TestTrue(TEXT("Encounter four tier one is available"), FirstPage.CardPackOffers[0].IsAvailable());
	TestTrue(TEXT("Encounter four tier two is available"), FirstPage.CardPackOffers[1].IsAvailable());
	TestTrue(TEXT("Encounter four tier three is available"), FirstPage.CardPackOffers[2].IsAvailable());
	const int32 OwnedBefore = RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Num();
	const int32 ShardsBefore = RunSubsystem->TimeShards;
	const int32 EffectivePriceBeforePurchase = RunSubsystem->GetDiscountedShopPrice(FirstPage.CardPackOffers[1].Price);
	TestTrue(TEXT("Current card pack can be prepaid"), RunSubsystem->PurchaseShopCardPackDetailed(2).IsSuccess());
	const FReEchoWeaponPartShopView PaidPage = RunSubsystem->GetWeaponPartShopView();
	const FReEchoShopCardChoiceOffer* PurchasedCardCandidate = PaidPage.CardPackOffers[1].Choices.FindByPredicate(
	    [](const FReEchoShopCardChoiceOffer& Offer)
	    {
		    return Offer.CardId != TEXT("G_2_10") && Offer.CardId != TEXT("G_2_15") && Offer.CardId != TEXT("G_2_22");
	    });
	if (!TestNotNull(TEXT("Payment generates a purchasable tier-two card"), PurchasedCardCandidate))
	{
		return false;
	}
	const FReEchoShopCardChoiceOffer PurchasedCard = *PurchasedCardCandidate;
	TestFalse(TEXT("A candidate ItemId cannot bypass the paid choice transaction"),
	          RunSubsystem->PurchaseShopItem(PurchasedCard.ItemId));
	TestEqual(TEXT("A rejected direct candidate purchase preserves currency"),
	          RunSubsystem->TimeShards,
	          ShardsBefore - EffectivePriceBeforePurchase);
	TestEqual(TEXT("Payment leaves the pack pending a choice"),
	          PaidPage.CardPackOffers[1].Status,
	          EReEchoShopCardPackStatus::PaidPendingChoice);
	TestEqual(
	    TEXT("Payment does not grant a card"), RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Num(), OwnedBefore);
	TestEqual(TEXT("Pack payment deducts the effective tier price"),
	          RunSubsystem->TimeShards,
	          ShardsBefore - EffectivePriceBeforePurchase);
	TestFalse(TEXT("A paid pending pack cannot be charged twice"),
	          RunSubsystem->PurchaseShopCardPackDetailed(PurchasedCard.Tier).IsSuccess());
	TestTrue(TEXT("A paid candidate can be claimed"),
	         RunSubsystem->ClaimPaidShopCardChoice(PurchasedCard.ItemId).IsSuccess());
	// Egao Party: a claim also hands out 1-5 copies of 样样都通, and those fire their effect and
	// deliver every tier-one card, so the owned set grows well past one. The exact count depends on
	// the bonus roll and is asserted by the Egao card tests; here we only require real growth.
	const int32 OwnedAfterClaim = RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Num();
	TestTrue(TEXT("Card purchase grants the choice plus the Egao bonus cards"),
	         OwnedAfterClaim > OwnedBefore + 1);
	TestTrue(TEXT("Granted card id is the selected choice id"),
	         RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Contains(PurchasedCard.CardId));
	TestEqual(TEXT("Card claim does not charge again"),
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
	// Egao Party: easter-egg cards and 样样都通 stay projected after being owned because they are
	// repeatable by design. This check therefore only applies to ordinary cards.
	auto IsNonRepeatableMatch = [&](const FReEchoShopCardChoiceOffer& Choice)
	{
		if (Choice.ItemId != PurchasedCard.ItemId && Choice.CardId != PurchasedCard.CardId)
		{
			return false;
		}
		const FReEchoCardDefinition* Card =
		    Snapshot.IsValid() && Snapshot->CardCatalog.IsValid() ? Snapshot->CardCatalog->Find(Choice.CardId) : nullptr;
		return Card && !ReEchoCardRuntime::IsRepeatableCard(*Card);
	};
	TestFalse(TEXT("The newly owned non-repeatable tier-two/three card is no longer projected as a shop choice"),
	          PurchasedPack.Choices.ContainsByPredicate(IsNonRepeatableMatch));
	TestFalse(TEXT("Same card offer cannot be claimed twice on one page"),
	          RunSubsystem->ClaimPaidShopCardChoice(PurchasedCard.ItemId).IsSuccess());
	const int32 OtherPackIndex = PurchasedCard.Tier == 2 ? 2 : 1;
	TestTrue(TEXT("Purchasing one pack leaves the other configured pack available"),
	         PurchasedPage.CardPackOffers[OtherPackIndex].IsAvailable());

	const int32 BeforeRefreshShards = RunSubsystem->TimeShards;
	const int32 RefreshCost = PurchasedPage.WeaponRuneRefreshCost;
	RunSubsystem->CurrentBuild.CardState.Runtime.FreeShopRefreshes = 0;
	TestTrue(TEXT("Paid refresh succeeds after free refreshes are exhausted"),
	         RunSubsystem->TryConsumeShopRefresh(RefreshCost));
	TestEqual(
	    TEXT("Paid refresh deducts the configured price"), RunSubsystem->TimeShards, BeforeRefreshShards - RefreshCost);
	const FReEchoWeaponPartShopView RefreshedPage = RunSubsystem->GetWeaponPartShopView();
	TestEqual(TEXT("Refreshed page still has three fixed weapon/part slots"),
	          RefreshedPage.SlotOffers.Num(),
	          ReEchoShopOfferCountPerGroup);
	TestEqual(TEXT("Refreshed page keeps three fixed card packs"),
	          RefreshedPage.CardPackOffers.Num(),
	          ReEchoShopOfferCountPerGroup);
	TestEqual(TEXT("Weapon/rune refresh preserves the purchased card-pack state"),
	          RefreshedPage.CardPackOffers[PurchasedCard.Tier - 1].Status,
	          EReEchoShopCardPackStatus::Purchased);
	TestFalse(TEXT("A non-tier-one purchased non-repeatable card stays excluded after refresh"),
	          RefreshedPage.CardPackOffers[PurchasedCard.Tier - 1].Choices.ContainsByPredicate(IsNonRepeatableMatch));

	UGameInstance* OwnershipChangeGameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* OwnershipChangeRun = NewObject<UReEchoRunSubsystem>(OwnershipChangeGameInstance);
	OwnershipChangeRun->StartRun(TEXT("J_CAT"), TEXT("W_J_08"));
	OwnershipChangeRun->EncounterIndex = 4;
	OwnershipChangeRun->TimeShards = 1000;
	const FReEchoWeaponPartShopView BeforeOwnershipChange = OwnershipChangeRun->GetWeaponPartShopView();
	const TSharedPtr<const FReEchoCsvDataSnapshot> OwnershipSnapshot = OwnershipChangeRun->GetRunDataSnapshot();
	const TArray<FReEchoCardDefinition> TierThreeBeforePurchase = ReEchoCardRuntime::BuildOfferPool(
	    *OwnershipSnapshot->CardCatalog, OwnershipChangeRun->CurrentBuild.CardState, TEXT("Trait"), 3, 4);
	if (!TestTrue(TEXT("Ownership-change fixture exposes a tier-three pack and at least four eligible cards"),
	              BeforeOwnershipChange.CardPackOffers[2].IsAvailable() && TierThreeBeforePurchase.Num() >= 4))
	{
		return false;
	}
	TestTrue(TEXT("An unpaid tier-three pack has no stale candidates"),
	         BeforeOwnershipChange.CardPackOffers[2].Choices.IsEmpty());
	const FName NewlyOwnedTierThreeCard = TierThreeBeforePurchase[0].Id;
	OwnershipChangeRun->CurrentBuild.CardState.OwnedCardIds.Add(NewlyOwnedTierThreeCard);
	TestTrue(TEXT("Payment rolls against ownership changes made in the same shop"),
	         OwnershipChangeRun->PurchaseShopCardPackDetailed(3).IsSuccess());
	const FReEchoWeaponPartShopView AfterOwnershipChange = OwnershipChangeRun->GetWeaponPartShopView();
	TestFalse(TEXT("The card granted before payment is excluded from the paid candidates"),
	          AfterOwnershipChange.CardPackOffers[2].Choices.ContainsByPredicate(
	              [&](const FReEchoShopCardChoiceOffer& Choice)
	              {
		              return Choice.CardId == NewlyOwnedTierThreeCard;
	              }));
	TestEqual(TEXT("A newly owned collision is replaced so three choices still appear"),
	          AfterOwnershipChange.CardPackOffers[2].Choices.Num(),
	          ReEchoShopOfferCountPerGroup);

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
	TestTrue(TEXT("One pack can be prepaid before saving"), StablePageRun->PurchaseShopCardPackDetailed(2).IsSuccess());
	const FReEchoWeaponPartShopView StablePurchasedPage = StablePageRun->GetWeaponPartShopView();
	const FReEchoShopCardChoiceOffer* StableTierTwoChoice =
	    StablePurchasedPage.CardPackOffers[1].Choices.FindByPredicate(
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
	TestEqual(TEXT("The save fixture remains pending before save"),
	          StablePurchasedPage.CardPackOffers[1].Status,
	          EReEchoShopCardPackStatus::PaidPendingChoice);
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
			TestEqual(TEXT("Save and restore preserves each pack price"), ActualPack.Price, ExpectedPack.Price);
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
		TestTrue(TEXT("A restored paid pack can finish its card claim without another payment"),
		         RestoredPageRun->ClaimPaidShopCardChoice(StableTierTwoChoice->ItemId).IsSuccess());
	}
	TestTrue(TEXT("A different tier pack remains independently payable"),
	         StablePageRun->PurchaseShopCardPackDetailed(3).IsSuccess());
	const FReEchoShopCardPackOffer PaidTierThreePack = StablePageRun->GetWeaponPartShopView().CardPackOffers[2];
	if (!TestTrue(TEXT("The independently paid pack generates candidates"), !PaidTierThreePack.Choices.IsEmpty()))
	{
		return false;
	}
	const FReEchoShopCardChoiceOffer StableTierThreeChoice = PaidTierThreePack.Choices[0];
	TestTrue(TEXT("The independently paid pack can be claimed"),
	         StablePageRun->ClaimPaidShopCardChoice(StableTierThreeChoice.ItemId).IsSuccess());

	UGameInstance* FailureGameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* FailureRun = NewObject<UReEchoRunSubsystem>(FailureGameInstance);
	FailureRun->StartRun(TEXT("J_CAT"), TEXT("W_J_08"));
	FailureRun->EncounterIndex = 4;
	FailureRun->TimeShards = 0;
	const FReEchoWeaponPartShopView BeforeFailurePage = FailureRun->GetWeaponPartShopView();
	const FReEchoShopPurchaseOutcome FailureOutcome = FailureRun->PurchaseShopCardPackDetailed(2);
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
	FailureRun->TimeShards = 1000;
	TestTrue(TEXT("Grant-failure fixture can commit its pack payment"),
	         FailureRun->PurchaseShopCardPackDetailed(2).IsSuccess());
	const FReEchoShopCardChoiceOffer PaidFailureChoice =
	    FailureRun->GetWeaponPartShopView().CardPackOffers[1].Choices[0];
	const int32 PaidFailureShards = FailureRun->TimeShards;
	FailureRun->CurrentBuild.CardState.OwnedCardIds.Add(PaidFailureChoice.CardId);
	TestEqual(TEXT("An invalid paid claim is rejected without another charge"),
	          FailureRun->ClaimPaidShopCardChoice(PaidFailureChoice.ItemId).Result,
	          EReEchoShopPurchaseResult::OfferNotFound);
	TestEqual(
	    TEXT("A failed paid claim preserves the post-payment balance"), FailureRun->TimeShards, PaidFailureShards);
	TestEqual(TEXT("A failed paid claim preserves the pending pack state"),
	          FailureRun->CurrentBuild.CardState.Runtime.ShopCardPackStates[1].bPaymentCommitted,
	          true);
	TestFalse(TEXT("A failed paid claim does not consume the pack"),
	          FailureRun->CurrentBuild.CardState.Runtime.ShopCardPackStates[1].bPurchased);

	UGameInstance* PartialGameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* PartialRun = NewObject<UReEchoRunSubsystem>(PartialGameInstance);
	PartialRun->StartRun(TEXT("J_CAT"), TEXT("W_J_08"));
	PartialRun->EncounterIndex = 4;
	PartialRun->TimeShards = 1000;
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
		// Keep this fixture deterministic: Easter cards are a separate injection path, not part of the
		// corresponding tier's one/two-card remainder being asserted here.
		for (const FReEchoCardDefinition& EasterCard :
		     PartialSnapshot->CardCatalog->GetOfferable(TEXT("EasterEgg"), INDEX_NONE))
		{
			PartialRun->CurrentBuild.CardState.OwnedCardIds.Add(EasterCard.Id);
		}
		TestTrue(TEXT("A tier with two eligible cards can be paid"),
		         PartialRun->PurchaseShopCardPackDetailed(2).IsSuccess());
		const FReEchoWeaponPartShopView TwoRemainingPage = PartialRun->GetWeaponPartShopView();
		TestEqual(TEXT("A tier with exactly two eligible cards exposes exactly two choices"),
		          TwoRemainingPage.CardPackOffers[1].Choices.Num(),
		          2);
		TestTrue(TEXT("One of the two remaining cards can be claimed"),
		         PartialRun->ClaimPaidShopCardChoice(TwoRemainingPage.CardPackOffers[1].Choices[0].ItemId).IsSuccess());
		PartialRun->EncounterIndex = 6;
		TestTrue(TEXT("A tier with one eligible card can still be paid"),
		         PartialRun->PurchaseShopCardPackDetailed(2).IsSuccess());
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoSeparatedShopRefreshTest,
                                 "ReEcho.Shop.RefreshesWeaponRunesAndCardSlotsIndependently",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoSeparatedShopRefreshTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_CAT"), TEXT("W_J_08"));
	Run->EncounterIndex = 4;
	Run->TimeShards = 1000;
	const FReEchoWeaponPartShopView InitialPage = Run->GetWeaponPartShopView();
	if (!TestTrue(TEXT("Tier-two card pack is available but unrolled before payment"),
	              InitialPage.CardPackOffers[1].IsAvailable() && InitialPage.CardPackOffers[1].Choices.IsEmpty()))
	{
		return false;
	}
	if (!TestTrue(TEXT("Card-slot refresh begins only after pack payment"),
	              Run->PurchaseShopCardPackDetailed(2).IsSuccess()))
	{
		return false;
	}
	const FReEchoWeaponPartShopView PaidInitialPage = Run->GetWeaponPartShopView();
	const FReEchoShopCardChoiceOffer* InitialChoicePtr = PaidInitialPage.CardPackOffers[1].Choices.FindByPredicate(
	    [](const FReEchoShopCardChoiceOffer& Choice)
	    {
		    return Choice.bCanRefresh;
	    });
	if (!TestNotNull(TEXT("Payment generates a refreshable tier-two slot"), InitialChoicePtr))
	{
		return false;
	}
	const FReEchoShopCardChoiceOffer InitialChoice = *InitialChoicePtr;
	TMap<int32, FName> InitialCardsBySlot;
	for (const FReEchoShopCardChoiceOffer& Choice : PaidInitialPage.CardPackOffers[1].Choices)
	{
		InitialCardsBySlot.Add(Choice.SlotIndex, Choice.CardId);
	}
	const int32 BeforeCardRefreshShards = Run->TimeShards;
	FString RefreshError;
	TestTrue(TEXT("One card slot can refresh independently"),
	         Run->TryRefreshShopCardSlot(2, InitialChoice.SlotIndex, RefreshError));
	const FReEchoWeaponPartShopView CardRefreshedPage = Run->GetWeaponPartShopView();
	const FReEchoShopCardChoiceOffer* RefreshedChoice = CardRefreshedPage.CardPackOffers[1].Choices.FindByPredicate(
	    [&](const FReEchoShopCardChoiceOffer& Choice)
	    {
		    return Choice.SlotIndex == InitialChoice.SlotIndex;
	    });
	if (!TestNotNull(TEXT("The refreshed slot remains visible"), RefreshedChoice))
	{
		return false;
	}
	TestNotEqual(TEXT("Only the requested slot receives a replacement"), RefreshedChoice->CardId, InitialChoice.CardId);
	TestEqual(TEXT("The replacement remains in the same tier"), RefreshedChoice->Tier, 2);
	TestEqual(TEXT("The slot's single refresh is consumed"), RefreshedChoice->RemainingRefreshes, 0);
	TestEqual(TEXT("Card-slot refresh deducts the configured five shards"),
	          Run->TimeShards,
	          BeforeCardRefreshShards - InitialChoice.RefreshCost);
	const FReEchoShopCardPackRuntimeState& OnceRefreshedPack =
	    Run->CurrentBuild.CardState.Runtime.ShopCardPackStates[1];
	TestEqual(TEXT("The tier pack records all three initial cards plus its replacement"),
	          OnceRefreshedPack.OfferHistoryCardIds.Num(),
	          4);
	TestTrue(TEXT("The replaced card remains in the tier pack history"),
	         OnceRefreshedPack.OfferHistoryCardIds.Contains(InitialChoice.CardId));
	for (const FReEchoShopCardChoiceOffer& Choice : CardRefreshedPage.CardPackOffers[1].Choices)
	{
		if (Choice.SlotIndex != InitialChoice.SlotIndex)
		{
			TestEqual(TEXT("Sibling card slots remain unchanged"), Choice.CardId, InitialCardsBySlot[Choice.SlotIndex]);
		}
	}
	const int32 BeforeRejectedRepeatShards = Run->TimeShards;
	TestFalse(TEXT("The same card slot cannot refresh twice"),
	          Run->TryRefreshShopCardSlot(2, InitialChoice.SlotIndex, RefreshError));
	TestEqual(TEXT("Rejected repeated card refresh keeps currency"), Run->TimeShards, BeforeRejectedRepeatShards);
	const FReEchoShopCardChoiceOffer* IndependentChoice = CardRefreshedPage.CardPackOffers[1].Choices.FindByPredicate(
	    [&](const FReEchoShopCardChoiceOffer& Choice)
	    {
		    return Choice.SlotIndex != InitialChoice.SlotIndex && Choice.bCanRefresh;
	    });
	if (TestNotNull(TEXT("A sibling card slot keeps its independent refresh opportunity"), IndependentChoice))
	{
		const FName FirstReplacementId = RefreshedChoice->CardId;
		TestTrue(TEXT("A sibling card slot can spend its own refresh"),
		         Run->TryRefreshShopCardSlot(2, IndependentChoice->SlotIndex, RefreshError));
		const FReEchoWeaponPartShopView TwoSlotsRefreshedPage = Run->GetWeaponPartShopView();
		const FReEchoShopCardChoiceOffer* SecondReplacement =
		    TwoSlotsRefreshedPage.CardPackOffers[1].Choices.FindByPredicate(
		        [&](const FReEchoShopCardChoiceOffer& Choice)
		        {
			        return Choice.SlotIndex == IndependentChoice->SlotIndex;
		        });
		if (TestNotNull(TEXT("The sibling slot receives a replacement"), SecondReplacement))
		{
			TestTrue(TEXT("A sibling refresh cannot resurrect any card previously shown by this tier pack"),
			         InitialCardsBySlot.FindKey(SecondReplacement->CardId) == nullptr);
			TestNotEqual(TEXT("A sibling refresh cannot duplicate the first replacement"),
			             SecondReplacement->CardId,
			             FirstReplacementId);
		}
		const FReEchoShopCardChoiceOffer* StableFirstReplacement =
		    TwoSlotsRefreshedPage.CardPackOffers[1].Choices.FindByPredicate(
		        [&](const FReEchoShopCardChoiceOffer& Choice)
		        {
			        return Choice.SlotIndex == InitialChoice.SlotIndex;
		        });
		TestTrue(TEXT("Refreshing a sibling preserves the first replacement"),
		         StableFirstReplacement && StableFirstReplacement->CardId == FirstReplacementId);
	}
	const FReEchoWeaponPartShopView BeforeBlockedCardRefresh = Run->GetWeaponPartShopView();
	const FReEchoShopCardChoiceOffer* UnusedChoice = BeforeBlockedCardRefresh.CardPackOffers[1].Choices.FindByPredicate(
	    [](const FReEchoShopCardChoiceOffer& Choice)
	    {
		    return Choice.RemainingRefreshes > 0;
	    });
	if (TestNotNull(TEXT("One unused card slot remains for the no-refresh rule regression"), UnusedChoice))
	{
		const int32 BeforeBlockedShards = Run->TimeShards;
		Run->CurrentBuild.CardState.Runtime.EconomyPenalty = EReEchoCardEconomyPenalty::NoShopRefresh;
		TestFalse(TEXT("NoShopRefresh blocks card-slot refreshes as well as weapon/rune refreshes"),
		          Run->TryRefreshShopCardSlot(2, UnusedChoice->SlotIndex, RefreshError));
		TestEqual(TEXT("NoShopRefresh keeps card-refresh currency atomic"), Run->TimeShards, BeforeBlockedShards);
		Run->CurrentBuild.CardState.Runtime.EconomyPenalty = EReEchoCardEconomyPenalty::None;
	}

	const TArray<FReEchoShopCardPackRuntimeState> CardPacksBeforeWeaponRefresh =
	    Run->CurrentBuild.CardState.Runtime.ShopCardPackStates;
	const int32 BeforeWeaponRefreshShards = Run->TimeShards;
	TestTrue(TEXT("First weapon/rune refresh succeeds"), Run->TryRefreshWeaponRuneShop(RefreshError));
	TestEqual(TEXT("Weapon/rune refresh deducts its configured price"),
	          Run->TimeShards,
	          BeforeWeaponRefreshShards - InitialPage.WeaponRuneRefreshCost);
	TestTrue(TEXT("Weapon/rune refresh leaves all card-pack candidates intact"),
	         Run->CurrentBuild.CardState.Runtime.ShopCardPackStates[1].CandidateCardIds ==
	             CardPacksBeforeWeaponRefresh[1].CandidateCardIds);
	TestTrue(TEXT("Weapon/rune refresh leaves per-card-slot usage intact"),
	         Run->CurrentBuild.CardState.Runtime.ShopCardPackStates[1].SlotRefreshUses ==
	             CardPacksBeforeWeaponRefresh[1].SlotRefreshUses);
	TestTrue(TEXT("Second weapon/rune refresh succeeds"), Run->TryRefreshWeaponRuneShop(RefreshError));
	const int32 BeforeExhaustedWeaponRefreshShards = Run->TimeShards;
	TestFalse(TEXT("Third weapon/rune refresh is blocked by the per-encounter limit"),
	          Run->TryRefreshWeaponRuneShop(RefreshError));
	TestEqual(TEXT("Rejected exhausted weapon/rune refresh keeps currency"),
	          Run->TimeShards,
	          BeforeExhaustedWeaponRefreshShards);
	UReEchoRunSaveGame* RefreshSave = Run->CreateSaveSnapshot();
	UGameInstance* RestoredGameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* RestoredRun = NewObject<UReEchoRunSubsystem>(RestoredGameInstance);
	if (TestNotNull(TEXT("Separated refresh state can be saved"), RefreshSave) &&
	    TestTrue(TEXT("Separated refresh state can be restored"), RestoredRun->RestoreSaveSnapshot(*RefreshSave)))
	{
		const FReEchoWeaponPartShopView RestoredPage = RestoredRun->GetWeaponPartShopView();
		TestEqual(
		    TEXT("Save/load preserves the exhausted weapon/rune budget"), RestoredPage.WeaponRuneRefreshesRemaining, 0);
		const FReEchoShopCardChoiceOffer* RestoredCard = RestoredPage.CardPackOffers[1].Choices.FindByPredicate(
		    [&](const FReEchoShopCardChoiceOffer& Choice)
		    {
			    return Choice.SlotIndex == InitialChoice.SlotIndex;
		    });
		if (TestNotNull(TEXT("Save/load preserves the refreshed card slot"), RestoredCard))
		{
			TestEqual(
			    TEXT("Save/load preserves the card slot's exhausted budget"), RestoredCard->RemainingRefreshes, 0);
			TestEqual(
			    TEXT("Save/load preserves the card replacement id"), RestoredCard->CardId, RefreshedChoice->CardId);
		}
		TestTrue(TEXT("Save/load preserves the tier pack's full display history"),
		         RestoredRun->CurrentBuild.CardState.Runtime.ShopCardPackStates[1].OfferHistoryCardIds ==
		             Run->CurrentBuild.CardState.Runtime.ShopCardPackStates[1].OfferHistoryCardIds);
	}

	UGameInstance* ExhaustedGameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* ExhaustedRun = NewObject<UReEchoRunSubsystem>(ExhaustedGameInstance);
	ExhaustedRun->StartRun(TEXT("J_CAT"), TEXT("W_J_08"));
	ExhaustedRun->EncounterIndex = 4;
	ExhaustedRun->TimeShards = 1000;
	const FReEchoWeaponPartShopView ExhaustedInitialPage = ExhaustedRun->GetWeaponPartShopView();
	if (!TestTrue(TEXT("No-replacement fixture exposes tier two"),
	              ExhaustedInitialPage.CardPackOffers[1].IsAvailable()))
	{
		return false;
	}
	if (!TestTrue(TEXT("No-replacement fixture prepays its card pack"),
	              ExhaustedRun->PurchaseShopCardPackDetailed(2).IsSuccess()))
	{
		return false;
	}
	const FReEchoWeaponPartShopView ExhaustedPaidPage = ExhaustedRun->GetWeaponPartShopView();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = ExhaustedRun->GetRunDataSnapshot();
	for (const FReEchoCardDefinition& Card : Snapshot->CardCatalog->GetOfferable(TEXT("Trait"), 2))
	{
		if (!ExhaustedPaidPage.CardPackOffers[1].Choices.ContainsByPredicate(
		        [&](const FReEchoShopCardChoiceOffer& Choice)
		        {
			        return Choice.CardId == Card.Id;
		        }))
		{
			ExhaustedRun->CurrentBuild.CardState.OwnedCardIds.Add(Card.Id);
		}
	}
	const FReEchoShopCardChoiceOffer NoReplacementChoice = ExhaustedPaidPage.CardPackOffers[1].Choices[0];
	const int32 BeforeNoReplacementShards = ExhaustedRun->TimeShards;
	TestFalse(TEXT("A card slot with no legal unowned replacement is disabled transactionally"),
	          ExhaustedRun->TryRefreshShopCardSlot(2, NoReplacementChoice.SlotIndex, RefreshError));
	TestEqual(
	    TEXT("No-replacement failure does not deduct shards"), ExhaustedRun->TimeShards, BeforeNoReplacementShards);
	TestEqual(TEXT("No-replacement failure does not consume the slot use"),
	          ExhaustedRun->CurrentBuild.CardState.Runtime.ShopCardPackStates[1]
	              .SlotRefreshUses[NoReplacementChoice.SlotIndex],
	          0);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCurseBankAndWeaponMasterTest,
                                 "ReEcho.Shop.CurseBankAndWeaponMaster",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCurseBankAndWeaponMasterTest::RunTest(const FString&)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* BankRun = NewObject<UReEchoRunSubsystem>(GameInstance);
	BankRun->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	BankRun->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_2_19"));
	BankRun->TimeShards = 0;
	TestTrue(TEXT("Curse bank permits an otherwise unaffordable purchase"),
	         BankRun->PurchaseShopItem(TEXT("SHOP_OLD_COIN")));
	TestEqual(TEXT("Credit purchase leaves no positive cash"), BankRun->TimeShards, 0);
	TestEqual(TEXT("Credit purchase records the full shortfall as debt"),
	          BankRun->CurrentBuild.CardState.Runtime.TimeShardDebt,
	          25);
	BankRun->BeginEncounter();
	BankRun->CompleteEncounter(FReEchoRecording(), true, false);
	TestEqual(TEXT("Encounter interest is ten percent rounded down"),
	          BankRun->CurrentBuild.CardState.Runtime.TimeShardDebt,
	          27);
	TestTrue(TEXT("Incoming shards are accepted while debt exists"), BankRun->GrantTimeShards(10));
	TestEqual(
	    TEXT("Incoming shards repay debt before cash"), BankRun->CurrentBuild.CardState.Runtime.TimeShardDebt, 17);
	TestEqual(TEXT("No cash remains before debt is repaid"), BankRun->TimeShards, 0);
	TestTrue(TEXT("A grant larger than debt is accepted"), BankRun->GrantTimeShards(20));
	TestEqual(TEXT("Debt reaches zero before surplus becomes cash"),
	          BankRun->CurrentBuild.CardState.Runtime.TimeShardDebt,
	          0);
	TestEqual(TEXT("Only the post-repayment surplus becomes cash"), BankRun->TimeShards, 3);

	UReEchoRunSubsystem* MasterRun = NewObject<UReEchoRunSubsystem>(GameInstance);
	MasterRun->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	MasterRun->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_3_28"));
	const FReEchoStatBlock InitialStats = MasterRun->CurrentBuild.Stats;
	MasterRun->BeginEncounter();
	MasterRun->CompleteEncounter(FReEchoRecording(), true, false);
	TestEqual(TEXT("Completing with the first weapon grants both attacks"),
	          MasterRun->CurrentBuild.Stats.PhysicalAttack,
	          InitialStats.PhysicalAttack + 5.0f);
	TestEqual(TEXT("Completing with the first weapon grants maximum health"),
	          MasterRun->CurrentBuild.Stats.HpMax,
	          InitialStats.HpMax + 10.0f);
	MasterRun->BeginEncounter();
	MasterRun->CompleteEncounter(FReEchoRecording(), true, false);
	TestEqual(TEXT("Completing again with the same weapon grants nothing"),
	          MasterRun->CurrentBuild.Stats.PhysicalAttack,
	          InitialStats.PhysicalAttack + 5.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCurseBankDisplayBalanceTest,
                                 "ReEcho.Shop.CurseBankDisplayBalance",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCurseBankDisplayBalanceTest::RunTest(const FString&)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	Run->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_2_19"));
	Run->TimeShards = 0;
	if (!TestTrue(TEXT("Curse bank fixture can buy on credit"), Run->PurchaseShopItem(TEXT("SHOP_OLD_COIN"))))
	{
		return false;
	}
	TestEqual(TEXT("Debt remains stored separately from cash"), Run->TimeShards, 0);
	TestEqual(
	    TEXT("Curse bank exposes debt as a negative presentation balance"), Run->GetDisplayedTimeShardBalance(), -25);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoFreeShopAuthoritativeProjectionTest,
                                 "ReEcho.Shop.FreeShopUsesAuthoritativeProjection",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoFreeShopAuthoritativeProjectionTest::RunTest(const FString&)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	Run->EncounterIndex = 2;
	Run->Phase = EReEchoRunPhase::Planning;
	Run->TimeShards = 0;
	Run->CurrentBuild.CardState.Runtime.FreeShopEncounterIndex = Run->EncounterIndex;
	Run->CurrentBuild.CardState.Runtime.FreeShopRefreshes = 1;

	const FReEchoWeaponPartShopView InitialView = Run->GetWeaponPartShopView();
	const FReEchoShopOffer* LegacyOffer = InitialView.RunItemOffers.FindByPredicate(
	    [](const FReEchoShopOffer& Offer)
	    {
		    return Offer.ItemId == TEXT("SHOP_RUSTED_SCISSORS");
	    });
	if (!TestNotNull(TEXT("The authoritative view projects compatibility shop items"), LegacyOffer))
	{
		return false;
	}
	TestEqual(TEXT("A free-shop compatibility item projects zero price"), LegacyOffer->EffectivePrice, 0);
	TestTrue(TEXT("A free-shop compatibility item remains purchasable with zero shards"), LegacyOffer->bCanPurchase);
	const FReEchoShopPurchaseOutcome LegacyPurchase = Run->PurchaseShopItemDetailed(LegacyOffer->ItemId);
	TestTrue(TEXT("The zero-price compatibility item commits"), LegacyPurchase.IsSuccess());
	TestEqual(TEXT("The committed compatibility item reports zero price"), LegacyPurchase.EffectivePrice, 0);

	const FReEchoWeaponSlotOffer* WeaponRuneOffer = InitialView.SlotOffers.FindByPredicate(
	    [](const FReEchoWeaponSlotOffer& Offer)
	    {
		    return !Offer.ItemId.IsNone();
	    });
	if (!TestNotNull(TEXT("The free shop has a weapon/rune offer"), WeaponRuneOffer))
	{
		return false;
	}
	TestEqual(TEXT("The weapon/rune offer projects zero price"), WeaponRuneOffer->EffectivePrice, 0);
	TestTrue(TEXT("The weapon/rune offer is purchasable with zero shards"), WeaponRuneOffer->bCanPurchase);
	const FReEchoShopPurchaseOutcome WeaponRunePurchase = Run->PurchaseShopItemDetailed(WeaponRuneOffer->ItemId);
	TestTrue(TEXT("The zero-price weapon/rune offer commits"), WeaponRunePurchase.IsSuccess());
	TestEqual(TEXT("The weapon/rune transaction reports zero price"), WeaponRunePurchase.EffectivePrice, 0);

	const FReEchoShopCardPackOffer* TierOnePack = InitialView.CardPackOffers.FindByPredicate(
	    [](const FReEchoShopCardPackOffer& Pack)
	    {
		    return Pack.Tier == 1;
	    });
	if (!TestNotNull(TEXT("Encounter two projects its tier-one card pack"), TierOnePack))
	{
		return false;
	}
	TestEqual(TEXT("The free-shop card pack projects zero price"), TierOnePack->EffectivePrice, 0);
	TestTrue(TEXT("The free-shop card pack is purchasable with zero shards"), TierOnePack->bCanPurchase);
	const FReEchoShopPurchaseOutcome PackPayment = Run->PurchaseShopCardPackDetailed(1);
	TestTrue(TEXT("The zero-price card pack payment commits"), PackPayment.IsSuccess());
	TestEqual(TEXT("The card-pack transaction reports zero price"), PackPayment.EffectivePrice, 0);
	TestEqual(TEXT("Free purchases do not create currency or debt"), Run->GetDisplayedTimeShardBalance(), 0);

	FString RefreshError;
	TestTrue(TEXT("An existing free refresh can generate a new weapon/rune page"),
	         Run->TryRefreshWeaponRuneShop(RefreshError));
	const FReEchoWeaponPartShopView RefreshedView = Run->GetWeaponPartShopView();
	for (const FReEchoWeaponSlotOffer& Offer : RefreshedView.SlotOffers)
	{
		if (!Offer.ItemId.IsNone())
		{
			TestEqual(TEXT("A refreshed free-shop offer remains zero-price"), Offer.EffectivePrice, 0);
		}
	}

	Run->EncounterIndex = 3;
	const FReEchoWeaponPartShopView NextEncounterView = Run->GetWeaponPartShopView();
	const FReEchoShopOffer* NextLegacyOffer = NextEncounterView.RunItemOffers.FindByPredicate(
	    [](const FReEchoShopOffer& Offer)
	    {
		    return Offer.ItemId == TEXT("SHOP_DREAM_FRUIT");
	    });
	TestTrue(TEXT("The next encounter restores normal pricing and affordability"),
	         NextLegacyOffer && NextLegacyOffer->EffectivePrice > 0 && !NextLegacyOffer->bCanPurchase);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRuneBackpackWeaponCompatibilityTest,
                                 "ReEcho.Shop.RuneBackpackFiltersCurrentWeapon",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRuneBackpackWeaponCompatibilityTest::RunTest(const FString&)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_04"));
	Run->OwnedPartIds.AddUnique(TEXT("P_CORE_PRIMORDIAL"));
	Run->OwnedPartIds.AddUnique(TEXT("P_SCYTHE_MOVESTACK_GRIP_I"));
	Run->OwnedPartIds.AddUnique(TEXT("P_LONGSWORD_HASTE_GRIP_I"));

	const FReEchoWeaponPartShopView ScytheView = Run->GetWeaponPartShopView();
	TestTrue(TEXT("The scythe backpack contains its own grip rune"),
	         ScytheView.OwnedParts.ContainsByPredicate(
	             [](const FReEchoShopOffer& Offer)
	             {
		             return Offer.ContentId == TEXT("P_SCYTHE_MOVESTACK_GRIP_I");
	             }));
	TestFalse(TEXT("The scythe backpack excludes a longsword grip rune despite the shared slot name"),
	          ScytheView.OwnedParts.ContainsByPredicate(
	              [](const FReEchoShopOffer& Offer)
	              {
		              return Offer.ContentId == TEXT("P_LONGSWORD_HASTE_GRIP_I");
	              }));
	TestTrue(TEXT("A universal core remains visible for the scythe"),
	         ScytheView.OwnedParts.ContainsByPredicate(
	             [](const FReEchoShopOffer& Offer)
	             {
		             return Offer.ContentId == TEXT("P_CORE_PRIMORDIAL");
	             }));
	FString EquipError;
	TestFalse(TEXT("The backend still rejects the incompatible longsword rune"),
	          Run->TryEquipPurchasedPart(TEXT("P_LONGSWORD_HASTE_GRIP_I"), EquipError));

	Run->OwnedWeaponIds.Add(TEXT("W_J_01"));
	TestTrue(TEXT("The owned longsword can be equipped"), Run->TryEquipOwnedWeapon(TEXT("W_J_01"), EquipError));
	const FReEchoWeaponPartShopView LongSwordView = Run->GetWeaponPartShopView();
	TestTrue(TEXT("Switching weapons reveals the matching longsword grip rune"),
	         LongSwordView.OwnedParts.ContainsByPredicate(
	             [](const FReEchoShopOffer& Offer)
	             {
		             return Offer.ContentId == TEXT("P_LONGSWORD_HASTE_GRIP_I");
	             }));
	TestFalse(TEXT("Switching weapons hides the scythe-only grip rune"),
	          LongSwordView.OwnedParts.ContainsByPredicate(
	              [](const FReEchoShopOffer& Offer)
	              {
		              return Offer.ContentId == TEXT("P_SCYTHE_MOVESTACK_GRIP_I");
	              }));
	return true;
}

#endif
