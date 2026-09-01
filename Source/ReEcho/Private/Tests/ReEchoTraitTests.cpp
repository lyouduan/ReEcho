#include "Misc/AutomationTest.h"
#include "Cards/ReEchoCardCatalog.h"
#include "Cards/ReEchoCardRuntime.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/GameInstance.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoRunSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoOwnedEasterCardQueryTest,
                                 "ReEcho.Traits.OwnedEasterCardQuery",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoOwnedEasterCardQueryTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	TestFalse(TEXT("A fresh build has no Easter card"), Run->HasOwnedEasterEggCard());
	TestTrue(TEXT("The production Easter card can be granted"), Run->DebugGrantCard(TEXT("G_4_1")));
	TestTrue(TEXT("An owned Easter card is detected by offer group"), Run->HasOwnedEasterEggCard());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBossPhase3FinalStatMultiplierTest,
                                 "ReEcho.Traits.BossPhase3FinalStatMultiplierPersists",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBossPhase3FinalStatMultiplierTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	const FReEchoStatBlock CanonicalStats = Run->CurrentBuild.Stats;
	const FReEchoStatBlock CanonicalEquipmentBase = Run->CurrentBuild.EquipmentBaseStats;

	TestTrue(TEXT("Phase3 final-stat multiplier activates once"), Run->ActivateBossPhase3FinalStatMultiplier());
	TestFalse(TEXT("Phase3 final-stat multiplier cannot stack repeatedly"),
	          Run->ActivateBossPhase3FinalStatMultiplier());
	TestEqual(TEXT("Phase3 doubles the effective physical attack"),
	          Run->CurrentBuild.Stats.PhysicalAttack,
	          CanonicalStats.PhysicalAttack * 2.0f);
	TestEqual(TEXT("Phase3 leaves canonical equipment-base attack unchanged"),
	          Run->CurrentBuild.EquipmentBaseStats.PhysicalAttack,
	          CanonicalEquipmentBase.PhysicalAttack);

	Run->NotifyCardDamageResolved(10.0f, 10.0f, false);
	TestEqual(TEXT("Damage-triggered card rebuild preserves the Phase3 multiplier"),
	          Run->CurrentBuild.Stats.PhysicalAttack,
	          CanonicalStats.PhysicalAttack * 2.0f);
	Run->NotifyCardKill(false, TEXT("M_TEST"));
	TestEqual(TEXT("Kill-triggered card rebuild preserves the Phase3 multiplier"),
	          Run->CurrentBuild.Stats.PhysicalAttack,
	          CanonicalStats.PhysicalAttack * 2.0f);
	Run->NotifyCardReaction(TEXT("REACTION_TEST"), true);
	TestEqual(TEXT("Reaction-triggered card rebuild preserves the Phase3 multiplier"),
	          Run->CurrentBuild.Stats.ElementalAttack,
	          CanonicalStats.ElementalAttack * 2.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoTraitOffersAreDeterministicTest,
                                 "ReEcho.Traits.OffersAreDeterministicAndDiverse",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoTraitOffersAreDeterministicTest::RunTest(const FString& Parameters)
{
	auto PrepareChoice = []()
	{
		UGameInstance* GameInstance = NewObject<UGameInstance>();
		UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
		RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
		RunSubsystem->EncounterIndex = 2;
		RunSubsystem->Phase = EReEchoRunPhase::CardChoice;
		return RunSubsystem;
	};

	UReEchoRunSubsystem* FirstRun = PrepareChoice();
	UReEchoRunSaveGame* SavedChoice = FirstRun->CreateSaveSnapshot();
	UGameInstance* RestoredGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* SecondRun = NewObject<UReEchoRunSubsystem>(RestoredGameInstance);
	TestTrue(TEXT("Card-choice snapshot restores"), SecondRun->RestoreSaveSnapshot(*SavedChoice));
	const TArray<FReEchoTraitCardOffer> FirstOffers = FirstRun->GenerateTraitCardOffers(3);
	const TArray<FReEchoTraitCardOffer> SecondOffers = SecondRun->GenerateTraitCardOffers(3);

	TestEqual(TEXT("A draw returns three offers"), FirstOffers.Num(), 3);
	TestEqual(TEXT("The restored run returns three offers"), SecondOffers.Num(), 3);
	TSet<FName> UniqueIds;
	for (int32 Index = 0; Index < FirstOffers.Num(); ++Index)
	{
		UniqueIds.Add(FirstOffers[Index].CardId);
		if (SecondOffers.IsValidIndex(Index))
		{
			TestEqual(TEXT("Save/load preserves the current offer order"),
			          FirstOffers[Index].CardId,
			          SecondOffers[Index].CardId);
		}
	}
	TestEqual(TEXT("A draw never repeats a card"), UniqueIds.Num(), FirstOffers.Num());

	TSet<FString> EncounterOfferSignatures;
	for (int32 Encounter = 2; Encounter <= 7; ++Encounter)
	{
		FirstRun->EncounterIndex = Encounter;
		FirstRun->Phase = EReEchoRunPhase::CardChoice;
		const TArray<FReEchoTraitCardOffer> EncounterOffers = FirstRun->GenerateTraitCardOffers(3);
		FString Signature;
		for (const FReEchoTraitCardOffer& Offer : EncounterOffers)
		{
			Signature += Offer.CardId.ToString() + TEXT("|");
		}
		EncounterOfferSignatures.Add(Signature);
	}
	TestTrue(TEXT("Different encounters do not reuse one fixed card sequence"), EncounterOfferSignatures.Num() > 1);

	TSet<FString> NewRunOfferSignatures;
	TSet<int32> NewRunSeeds;
	for (int32 RunIndex = 0; RunIndex < 6; ++RunIndex)
	{
		UReEchoRunSubsystem* FreshRun = PrepareChoice();
		NewRunSeeds.Add(FreshRun->CreateSaveSnapshot()->TraitOfferSeed);
		const TArray<FReEchoTraitCardOffer> FreshOffers = FreshRun->GenerateTraitCardOffers(3);
		FString Signature;
		for (const FReEchoTraitCardOffer& Offer : FreshOffers)
		{
			Signature += Offer.CardId.ToString() + TEXT("|");
		}
		NewRunOfferSignatures.Add(Signature);
	}
	TestTrue(TEXT("Fresh runs capture more than one real-time offer seed"), NewRunSeeds.Num() > 1);
	TestTrue(TEXT("Fresh runs do not reuse one fixed card sequence"), NewRunOfferSignatures.Num() > 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoTraitDropMatrixTest,
                                 "ReEcho.Traits.PostEncounterDropMatrix",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoTraitDropMatrixTest::RunTest(const FString& Parameters)
{
	const TArray<int32> ExpectedFreeTiers = {INDEX_NONE, 2, 3, 1, 2, 3, 2, INDEX_NONE};
	for (int32 EncounterIndex = 1; EncounterIndex <= ExpectedFreeTiers.Num(); ++EncounterIndex)
	{
		UGameInstance* GameInstance = NewObject<UGameInstance>();
		UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
		RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
		RunSubsystem->EncounterIndex = EncounterIndex;
		RunSubsystem->Phase = EReEchoRunPhase::Encounter;
		RunSubsystem->CompleteEncounter(FReEchoRecording(), true, EncounterIndex == ExpectedFreeTiers.Num());

		const int32 ExpectedTier = ExpectedFreeTiers[EncounterIndex - 1];
		if (EncounterIndex == ExpectedFreeTiers.Num())
		{
			TestEqual(TEXT("The final boss encounter enters summary without a card drop"),
			          RunSubsystem->Phase,
			          EReEchoRunPhase::Summary);
			continue;
		}
		if (ExpectedTier == INDEX_NONE)
		{
			TestEqual(TEXT("An encounter with no free drop continues to the shop phase route"),
			          RunSubsystem->Phase,
			          EReEchoRunPhase::Planning);
			continue;
		}

		TestEqual(TEXT("A configured free drop enters card choice"), RunSubsystem->Phase, EReEchoRunPhase::CardChoice);
		const TArray<FReEchoTraitCardOffer> Offers = RunSubsystem->GenerateTraitCardOffers(3);
		TestEqual(
		    *FString::Printf(TEXT("Encounter %d produces one three-card group"), EncounterIndex), Offers.Num(), 3);
		for (const FReEchoTraitCardOffer& Offer : Offers)
		{
			TestEqual(*FString::Printf(TEXT("Encounter %d offer stays in configured tier"), EncounterIndex),
			          Offer.Tier,
			          ExpectedTier);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoTraitDropFailureFallbackTest,
                                 "ReEcho.Traits.DropFailureContinuesToShop",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoTraitDropFailureFallbackTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));

	RunSubsystem->EncounterIndex = 2;
	RunSubsystem->Phase = EReEchoRunPhase::CardChoice;
	AddExpectedError(TEXT("requires 100 card offers"), EAutomationExpectedErrorFlags::Contains, 1);
	TestTrue(TEXT("An undersized tier pool returns no partial or cross-tier group"),
	         RunSubsystem->GenerateTraitCardOffers(100).IsEmpty());
	TestEqual(TEXT("An undersized tier pool continues through the shop route"),
	          RunSubsystem->Phase,
	          EReEchoRunPhase::Planning);

	RunSubsystem->EncounterIndex = 99;
	RunSubsystem->Phase = EReEchoRunPhase::CardChoice;
	AddExpectedError(TEXT("has no shop_drop_levels row"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("has no configured free card tier"), EAutomationExpectedErrorFlags::Contains, 1);
	TestTrue(TEXT("A missing encounter row returns no offers"), RunSubsystem->GenerateTraitCardOffers(3).IsEmpty());
	TestEqual(TEXT("A missing encounter row continues through the shop route"),
	          RunSubsystem->Phase,
	          EReEchoRunPhase::Planning);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoTraitOfferApplicationTest,
                                 "ReEcho.Traits.AppliesOnlyPendingOffer",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoTraitOfferApplicationTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	RunSubsystem->EncounterIndex = 2;
	RunSubsystem->Phase = EReEchoRunPhase::CardChoice;
	const TArray<FReEchoTraitCardOffer> FirstOffers = RunSubsystem->GenerateTraitCardOffers(3);
	if (!TestEqual(TEXT("Initial draw returns three offers"), FirstOffers.Num(), 3))
	{
		return false;
	}

	TestFalse(TEXT("A card outside the pending offer is rejected"), RunSubsystem->ApplyTraitCard(TEXT("INVALID")));
	const FName SelectedCardId = FirstOffers[0].CardId;
	TestTrue(TEXT("A pending card can be applied"), RunSubsystem->ApplyTraitCard(SelectedCardId));
	TestFalse(TEXT("The same offer cannot be applied twice"), RunSubsystem->ApplyTraitCard(SelectedCardId));
	TestTrue(TEXT("The selected card enters the build"),
	         RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Contains(SelectedCardId));

	RunSubsystem->Phase = EReEchoRunPhase::CardChoice;
	const TArray<FReEchoTraitCardOffer> SecondOffers = RunSubsystem->GenerateTraitCardOffers(3);
	TestEqual(TEXT("The next draw returns three offers"), SecondOffers.Num(), 3);
	TestFalse(TEXT("An owned trait is excluded from later free offers"),
	          SecondOffers.ContainsByPredicate(
	              [&](const FReEchoTraitCardOffer& Offer)
	              {
		              return Offer.CardId == SelectedCardId;
	              }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoTierOneRepeatableFreeOfferTest,
                                 "ReEcho.Traits.TierOneOwnedCardsRemainInFreePool",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoTierOneRepeatableFreeOfferTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	const TArray<FName> TierOneCardIds = {TEXT("G_1_01"),
	                                      TEXT("G_1_02"),
	                                      TEXT("G_1_03"),
	                                      TEXT("G_1_04"),
	                                      TEXT("G_1_05"),
	                                      TEXT("G_1_06"),
	                                      TEXT("G_1_07"),
	                                      TEXT("G_1_08")};
	RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Append(TierOneCardIds);
	RunSubsystem->EncounterIndex = 4;
	RunSubsystem->Phase = EReEchoRunPhase::CardChoice;

	const TArray<FReEchoTraitCardOffer> Offers = RunSubsystem->GenerateTraitCardOffers(3);
	if (!TestEqual(TEXT("A fully-owned tier-one free pool still returns three repeatable cards"), Offers.Num(), 3))
	{
		return false;
	}
	for (const FReEchoTraitCardOffer& Offer : Offers)
	{
		TestTrue(TEXT("Every repeated free offer comes from the owned tier-one pool"),
		         TierOneCardIds.Contains(Offer.CardId));
		TestFalse(TEXT("Refresh never offers an already obtained tier-one card"), Offer.bCanRefresh);
	}
	const FName SelectedCardId = Offers[0].CardId;
	const int32 StackCountBefore = ReEchoCardRuntime::CountOwned(RunSubsystem->CurrentBuild.CardState, SelectedCardId);
	TestTrue(TEXT("An owned tier-one free offer can be selected again"), RunSubsystem->ApplyTraitCard(SelectedCardId));
	TestEqual(TEXT("Repeated free selection adds one tier-one stack"),
	          ReEchoCardRuntime::CountOwned(RunSubsystem->CurrentBuild.CardState, SelectedCardId),
	          StackCountBefore + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoTierOneFairFreeOfferTest,
                                 "ReEcho.Traits.TierOneFreePoolHasNoOwnedStackBias",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoTierOneFairFreeOfferTest::RunTest(const FString&)
{
	constexpr int32 SeedSampleCount = 64;
	const FName OwnedTierOneCard = TEXT("G_1_01");
	int32 SamplesContainingOwnedCard = 0;
	for (int32 Seed = 1; Seed <= SeedSampleCount; ++Seed)
	{
		UGameInstance* SourceGameInstance = NewObject<UGameInstance>();
		UReEchoRunSubsystem* Source = NewObject<UReEchoRunSubsystem>(SourceGameInstance);
		Source->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
		Source->CurrentBuild.CardState.OwnedCardIds.Add(OwnedTierOneCard);
		Source->EncounterIndex = 4;
		Source->Phase = EReEchoRunPhase::CardChoice;
		UReEchoRunSaveGame* Save = Source->CreateSaveSnapshot();
		Save->TraitOfferSeed = Seed;

		UGameInstance* RestoredGameInstance = NewObject<UGameInstance>();
		UReEchoRunSubsystem* Restored = NewObject<UReEchoRunSubsystem>(RestoredGameInstance);
		if (!TestTrue(TEXT("A seeded free-choice sample restores"), Restored->RestoreSaveSnapshot(*Save)))
		{
			return false;
		}
		const TArray<FReEchoTraitCardOffer> Offers = Restored->GenerateTraitCardOffers(3);
		if (!TestEqual(TEXT("Every seeded sample produces three tier-one cards"), Offers.Num(), 3))
		{
			return false;
		}
		SamplesContainingOwnedCard += Offers.ContainsByPredicate(
		    [&](const FReEchoTraitCardOffer& Offer)
		    {
			    return Offer.CardId == OwnedTierOneCard;
		    });
	}
	TestTrue(TEXT("An owned repeatable tier-one card is not systematically excluded behind unowned cards"),
	         SamplesContainingOwnedCard > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoFreeTraitSlotRefreshTest,
                                 "ReEcho.Traits.FreeChoiceSlotsRefreshIndependently",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoFreeTraitSlotRefreshTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	Run->EncounterIndex = 2;
	Run->Phase = EReEchoRunPhase::CardChoice;
	Run->TimeShards = 100;
	const TArray<FReEchoTraitCardOffer> InitialOffers = Run->GenerateTraitCardOffers(3);
	if (!TestEqual(TEXT("The configured free draw exposes three cards"), InitialOffers.Num(), 3))
	{
		return false;
	}
	const FReEchoTraitCardOffer* Refreshable = InitialOffers.FindByPredicate(
	    [](const FReEchoTraitCardOffer& Offer)
	    {
		    return Offer.bCanRefresh;
	    });
	if (!TestNotNull(TEXT("A free-draw card slot exposes its independent refresh"), Refreshable))
	{
		return false;
	}
	TMap<int32, FName> InitialIds;
	for (const FReEchoTraitCardOffer& Offer : InitialOffers)
	{
		InitialIds.Add(Offer.SlotIndex, Offer.CardId);
	}
	const int32 RefreshedSlot = Refreshable->SlotIndex;
	const int32 BeforeRefreshShards = Run->TimeShards;
	FString Error;
	TestTrue(TEXT("One free-draw slot refreshes"), Run->TryRefreshTraitCardSlot(RefreshedSlot, Error));
	UReEchoRunSaveGame* HistorySave = Run->CreateSaveSnapshot();
	TestEqual(TEXT("The free-choice group records all three initial cards plus its replacement"),
	          HistorySave ? HistorySave->PendingTraitCardOfferHistoryIds.Num() : 0,
	          4);
	TestTrue(TEXT("The replaced free card remains in this group's display history"),
	         HistorySave && HistorySave->PendingTraitCardOfferHistoryIds.Contains(InitialIds[RefreshedSlot]));
	const TArray<FReEchoTraitCardOffer> RefreshedOffers = Run->GenerateTraitCardOffers(3);
	TestEqual(TEXT("Free-draw refresh deducts the configured five shards"),
	          Run->TimeShards,
	          BeforeRefreshShards - Refreshable->RefreshCost);
	for (const FReEchoTraitCardOffer& Offer : RefreshedOffers)
	{
		if (Offer.SlotIndex == RefreshedSlot)
		{
			TestNotEqual(
			    TEXT("The selected free-draw slot receives a replacement"), Offer.CardId, InitialIds[Offer.SlotIndex]);
			TestEqual(TEXT("The free-draw replacement remains the configured tier"),
			          Offer.Tier,
			          InitialOffers[RefreshedSlot].Tier);
			TestEqual(TEXT("The refreshed free-draw slot has no remaining use"), Offer.RemainingRefreshes, 0);
		}
		else
		{
			TestEqual(TEXT("Free-draw sibling slots remain unchanged"), Offer.CardId, InitialIds[Offer.SlotIndex]);
		}
	}
	const int32 BeforeRepeatShards = Run->TimeShards;
	TestFalse(TEXT("The same free-draw slot cannot refresh twice"), Run->TryRefreshTraitCardSlot(RefreshedSlot, Error));
	TestEqual(TEXT("Rejected free-draw repeat keeps currency"), Run->TimeShards, BeforeRepeatShards);

	const FReEchoTraitCardOffer* Unused = RefreshedOffers.FindByPredicate(
	    [](const FReEchoTraitCardOffer& Offer)
	    {
		    return Offer.RemainingRefreshes > 0;
	    });
	if (TestNotNull(TEXT("A sibling free-draw slot remains unused"), Unused))
	{
		Run->CurrentBuild.CardState.Runtime.EconomyPenalty = EReEchoCardEconomyPenalty::NoShopRefresh;
		TestFalse(TEXT("NoShopRefresh blocks free-draw card refreshes"),
		          Run->TryRefreshTraitCardSlot(Unused->SlotIndex, Error));
		Run->CurrentBuild.CardState.Runtime.EconomyPenalty = EReEchoCardEconomyPenalty::None;
	}

	UReEchoRunSaveGame* Save = Run->CreateSaveSnapshot();
	UGameInstance* RestoredGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Restored = NewObject<UReEchoRunSubsystem>(RestoredGameInstance);
	if (TestNotNull(TEXT("The refreshed free-draw page can be saved"), Save) &&
	    TestTrue(TEXT("The refreshed free-draw page can be restored"), Restored->RestoreSaveSnapshot(*Save)))
	{
		const TArray<FReEchoTraitCardOffer> RestoredOffers = Restored->GenerateTraitCardOffers(3);
		TestEqual(TEXT("Save/load preserves all three free-draw choices"), RestoredOffers.Num(), 3);
		for (int32 SlotIndex = 0; SlotIndex < RestoredOffers.Num(); ++SlotIndex)
		{
			TestEqual(TEXT("Save/load preserves the free-draw replacement ids"),
			          RestoredOffers[SlotIndex].CardId,
			          RefreshedOffers[SlotIndex].CardId);
			TestEqual(TEXT("Save/load preserves each free-draw refresh budget"),
			          RestoredOffers[SlotIndex].RemainingRefreshes,
			          RefreshedOffers[SlotIndex].RemainingRefreshes);
		}
		TestTrue(TEXT("Save/load preserves the free-choice group's full display history"),
		         Restored->CreateSaveSnapshot()->PendingTraitCardOfferHistoryIds ==
		             Save->PendingTraitCardOfferHistoryIds);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoTraitCsvEffectsTest,
                                 "ReEcho.Traits.CsvEffectsApply",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoTraitCsvEffectsTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_SPADE"), NAME_None);
	TestEqual(TEXT("CSV default weapon is used when none is supplied"),
	          RunSubsystem->CurrentBuild.WeaponId,
	          FName(TEXT("W_J_01")));
	// Egao Party: every character shares the 66 / 16 / 16 baseline (was 22 / 5 / 5 for the Sage).
	TestEqual(
	    TEXT("CSV character base physical attack is used"), RunSubsystem->CurrentBuild.Stats.PhysicalAttack, 16.0f);

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = RunSubsystem->GetRunDataSnapshot();
	if (!TestTrue(TEXT("Card data snapshot is available"), Snapshot.IsValid() && Snapshot->CardCatalog.IsValid()))
	{
		return false;
	}
	const FReEchoCardDefinition* TaggedCard = Snapshot->CardCatalog->Find(TEXT("G_2_05"));
	TestNotNull(TEXT("G_2_05 is available for tag projection"), TaggedCard);
	if (TaggedCard)
	{
		TestEqual(TEXT("Trait definitions preserve the authored Tags column order"), TaggedCard->Tags.Num(), 3);
		TestTrue(TEXT("Trait definition includes the authored Critical tag"),
		         TaggedCard->Tags.Contains(TEXT("Critical")));
		TestTrue(TEXT("Trait definition includes the authored Echo tag"), TaggedCard->Tags.Contains(TEXT("Echo")));
		TestTrue(TEXT("Trait definition includes the authored Body tag"), TaggedCard->Tags.Contains(TEXT("Body")));
	}

	const float PhysicalBefore = RunSubsystem->CurrentBuild.Stats.PhysicalAttack;
	const FReEchoCardDefinition* PhysicalCard = Snapshot->CardCatalog->Find(TEXT("G_1_03"));
	if (!TestNotNull(TEXT("Physical-strength card exists in the current CSV catalog"), PhysicalCard))
	{
		return false;
	}
	const FReEchoCardEffectDefinition* PhysicalEffect = PhysicalCard->Effects.FindByPredicate(
	    [](const FReEchoCardEffectDefinition& Effect)
	    {
		    return Effect.Target == TEXT("PhysicalAttack");
	    });
	if (!TestNotNull(TEXT("Physical-strength card defines its authored attack gain"), PhysicalEffect))
	{
		return false;
	}
	// Egao Party: suppress the bonus 样样都通 so this asserts the card's own authored effect in
	// isolation. Without it the bonus delivers every tier-one card, some of which are detrimental,
	// making the exact delta unpredictable. The bonus itself is covered by the Egao card tests.
	TestTrue(TEXT("A CSV numeric trait can be applied"), RunSubsystem->DebugGrantCard(TEXT("G_1_03"), true));
	TestEqual(TEXT("Physical attack add comes from card_effects.csv"),
	          RunSubsystem->CurrentBuild.Stats.PhysicalAttack,
	          PhysicalBefore + PhysicalEffect->Value);

	EReEchoHealthAdjustment CommittedHealthAdjustment = EReEchoHealthAdjustment::None;
	FReEchoStatBlock CommittedStats;
	RunSubsystem->OnCardHealthCommitted.AddLambda(
	    [&CommittedHealthAdjustment, &CommittedStats](const FReEchoStatBlock& Stats,
	                                                  const EReEchoHealthAdjustment HealthAdjustment)
	    {
		    CommittedStats = Stats;
		    CommittedHealthAdjustment = HealthAdjustment;
	    });
	const float MaximumHealthBeforeStrength = RunSubsystem->CurrentBuild.Stats.HpMax;
	const float CurrentHealthBeforeStrength = RunSubsystem->CurrentBuild.Stats.HpPoint;
	TestTrue(TEXT("Life Strength can be granted through the authoritative Run transaction"),
	         RunSubsystem->DebugGrantCard(TEXT("G_1_02"), true));
	TestEqual(TEXT("Run publishes Life Strength's typed health adjustment after commit"),
	          CommittedHealthAdjustment,
	          EReEchoHealthAdjustment::SetToStatPoint);
	TestEqual(TEXT("Life Strength commits its authored maximum-health increase"),
	          RunSubsystem->CurrentBuild.Stats.HpMax,
	          MaximumHealthBeforeStrength + 8.0f);
	TestEqual(TEXT("Life Strength commits its authored current-health increase"),
	          RunSubsystem->CurrentBuild.Stats.HpPoint,
	          CurrentHealthBeforeStrength + 8.0f);
	CommittedHealthAdjustment = EReEchoHealthAdjustment::None;
	const float MaximumHealthBeforeForging = RunSubsystem->CurrentBuild.Stats.HpMax;
	const float ExpectedForgedMaximum = MaximumHealthBeforeForging + RunSubsystem->CurrentBuild.Stats.PhysicalAttack +
	                                    RunSubsystem->CurrentBuild.Stats.ElementalAttack;
	RunSubsystem->EncounterIndex = 5;
	TestTrue(TEXT("Blood Forging can be granted through the authoritative Run transaction"),
	         RunSubsystem->DebugGrantCard(TEXT("G_3_14"), true));
	TestEqual(TEXT("Run publishes Blood Forging's typed health adjustment after commit"),
	          CommittedHealthAdjustment,
	          EReEchoHealthAdjustment::FillToMax);
	TestEqual(
	    TEXT("The committed event contains the final maximum health"), CommittedStats.HpMax, ExpectedForgedMaximum);
	TestEqual(TEXT("The committed build health is full"),
	          RunSubsystem->CurrentBuild.Stats.HpPoint,
	          RunSubsystem->CurrentBuild.Stats.HpMax);

	RunSubsystem->EncounterIndex = 4;
	TestTrue(TEXT("Sacrificial Echo can be granted for its authored encounter"),
	         RunSubsystem->DebugGrantCard(TEXT("G_3_04"), true));
	CommittedHealthAdjustment = EReEchoHealthAdjustment::None;
	const float MaximumHealthBeforeEchoDefeat = RunSubsystem->CurrentBuild.Stats.HpMax;
	const float CurrentHealthBeforeEchoDefeat = RunSubsystem->CurrentBuild.Stats.HpPoint;
	RunSubsystem->NotifyCardEchoDefeated();
	TestEqual(TEXT("Sacrificial Echo defeat commits its authored maximum-health gain"),
	          RunSubsystem->CurrentBuild.Stats.HpMax,
	          MaximumHealthBeforeEchoDefeat + 5.0f);
	TestEqual(TEXT("Sacrificial Echo defeat commits its authored current-health gain"),
	          RunSubsystem->CurrentBuild.Stats.HpPoint,
	          CurrentHealthBeforeEchoDefeat + 5.0f);
	TestEqual(TEXT("Sacrificial Echo defeat publishes the unified health update"),
	          CommittedHealthAdjustment,
	          EReEchoHealthAdjustment::SetToStatPoint);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoStartRunResolveErrorsTest,
                                 "ReEcho.Traits.StartRunResolveErrors",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoStartRunResolveErrorsTest::RunTest(const FString& Parameters)
{
	const FReEchoStartRunResolveResult NoSnapshot =
	    ReEchoRunData::ResolveStartingBuildFromSnapshot(nullptr, TEXT("J_MISSING_SNAPSHOT"), NAME_None);
	TestFalse(TEXT("Snapshot absence fails"), NoSnapshot.bSuccess);
	TestTrue(TEXT("Snapshot error contains CharacterId"), NoSnapshot.Error.Contains(TEXT("J_MISSING_SNAPSHOT")));
	TestTrue(TEXT("Snapshot error explains unavailable data"), NoSnapshot.Error.Contains(TEXT("snapshot")));

	FReEchoCsvDataSnapshot Snapshot;
	FReEchoCsvCharacterRow DisabledCharacter;
	DisabledCharacter.Id = TEXT("J_DISABLED");
	DisabledCharacter.bEnabled = false;
	Snapshot.Characters.Add(DisabledCharacter.Id, DisabledCharacter);

	const FReEchoStartRunResolveResult MissingCharacter =
	    ReEchoRunData::ResolveStartingBuildFromSnapshot(&Snapshot, TEXT("J_UNKNOWN"), NAME_None);
	TestFalse(TEXT("Unknown character fails"), MissingCharacter.bSuccess);
	TestTrue(TEXT("Unknown-character error contains CharacterId"), MissingCharacter.Error.Contains(TEXT("J_UNKNOWN")));
	TestTrue(TEXT("Unknown-character error explains lookup failure"),
	         MissingCharacter.Error.Contains(TEXT("not found")));

	const FReEchoStartRunResolveResult DisabledResult =
	    ReEchoRunData::ResolveStartingBuildFromSnapshot(&Snapshot, TEXT("J_DISABLED"), NAME_None);
	TestFalse(TEXT("Disabled character fails"), DisabledResult.bSuccess);
	TestTrue(TEXT("Disabled-character error contains CharacterId"), DisabledResult.Error.Contains(TEXT("J_DISABLED")));
	TestTrue(TEXT("Disabled-character error explains disabled state"), DisabledResult.Error.Contains(TEXT("disabled")));

	FReEchoCsvCharacterRow EnabledCharacter;
	EnabledCharacter.Id = TEXT("J_ENABLED");
	EnabledCharacter.bEnabled = true;
	EnabledCharacter.DefaultWeaponId = TEXT("W_DEFAULT");
	EnabledCharacter.RoleId = TEXT("None");
	EnabledCharacter.BaseStats.HpMax = 12.0f;
	EnabledCharacter.BaseStats.HpPoint = 12.0f;
	EnabledCharacter.BaseStats.PhysicalAttack = 3.0f;
	EnabledCharacter.BaseStats.ElementalAttack = 4.0f;
	EnabledCharacter.BaseStats.AttackSpeed = 1.0f;
	EnabledCharacter.BaseStats.MovementSpeed = 1.0f;
	Snapshot.Characters.Add(EnabledCharacter.Id, EnabledCharacter);

	FReEchoCsvWeaponRow EnabledWeapon;
	EnabledWeapon.Id = TEXT("W_DEFAULT");
	EnabledWeapon.WeaponTypeId = TEXT("LongSword");
	EnabledWeapon.AttackPatternId = TEXT("Pattern.LongSwordCombo");
	EnabledWeapon.DisplayName = TEXT("Default Test Weapon");
	EnabledWeapon.bEnabled = true;
	EnabledWeapon.DataRevision = 7;
	Snapshot.Weapons.Add(EnabledWeapon.Id, EnabledWeapon);

	const FReEchoStartRunResolveResult Success =
	    ReEchoRunData::ResolveStartingBuildFromSnapshot(&Snapshot, TEXT("J_ENABLED"), NAME_None);
	TestTrue(TEXT("Enabled character resolves"), Success.bSuccess);
	TestEqual(TEXT("Default weapon comes from character row"), Success.Build.WeaponId, FName(TEXT("W_DEFAULT")));
	TestEqual(TEXT("Weapon revision is captured in the build snapshot"), Success.Build.WeaponDataRevision, 7);
	TestEqual(TEXT("Character stats are copied"), Success.Build.Stats.HpMax, 12.0f);

	const FReEchoStartRunResolveResult UnknownWeapon =
	    ReEchoRunData::ResolveStartingBuildFromSnapshot(&Snapshot, TEXT("J_ENABLED"), TEXT("W_UNKNOWN"));
	TestFalse(TEXT("Unknown weapon fails"), UnknownWeapon.bSuccess);
	TestTrue(TEXT("Unknown-weapon error contains WeaponId"), UnknownWeapon.Error.Contains(TEXT("W_UNKNOWN")));

	EnabledWeapon.bEnabled = false;
	Snapshot.Weapons[EnabledWeapon.Id] = EnabledWeapon;
	const FReEchoStartRunResolveResult DisabledWeapon =
	    ReEchoRunData::ResolveStartingBuildFromSnapshot(&Snapshot, TEXT("J_ENABLED"), NAME_None);
	TestFalse(TEXT("Disabled default weapon fails"), DisabledWeapon.bSuccess);
	TestTrue(TEXT("Disabled-weapon error explains disabled state"), DisabledWeapon.Error.Contains(TEXT("disabled")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCardEffectsApplyAtomicallyTest,
                                 "ReEcho.Traits.CardEffectsApplyAtomically",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCardEffectsApplyAtomicallyTest::RunTest(const FString& Parameters)
{
	FReEchoBuildSnapshot Build;
	Build.Stats.HpMax = 15.0f;
	Build.Stats.HpPoint = 15.0f;
	Build.Stats.PhysicalAttack = 5.0f;
	Build.Stats.MovementSpeed = 1.0f;

	FReEchoCsvCardRow Card;
	Card.Id = TEXT("TEST_ATOMIC_CARD");

	FReEchoCsvCardEffectRow FirstEffect;
	FirstEffect.Id = TEXT("TEST_ATOMIC_VALID");
	FirstEffect.CardId = Card.Id;
	FirstEffect.Order = 1;
	FirstEffect.Trigger = TEXT("OnApply");
	FirstEffect.EffectKind = TEXT("StatModifier");
	FirstEffect.Target = TEXT("PhysicalAttack");
	FirstEffect.ValueOp = EReEchoCsvValueOp::Add;
	FirstEffect.Value = 10.0f;
	FirstEffect.BehaviorId = TEXT("Card.StatModifier");

	FReEchoCsvCardEffectRow InvalidEffect = FirstEffect;
	InvalidEffect.Id = TEXT("TEST_ATOMIC_INVALID");
	InvalidEffect.Order = 2;
	InvalidEffect.Target = TEXT("UnsupportedRuntimeTarget");
	Card.Effects = {FirstEffect, InvalidEffect};

	FReEchoBuildSnapshot FailedBuild = Build;
	TestFalse(TEXT("Invalid later effect rejects the whole card"),
	          ReEchoRunData::TryApplyCardEffectsToBuild(Card, Build, FailedBuild));
	TestEqual(TEXT("No partial physical attack increase is committed"),
	          FailedBuild.Stats.PhysicalAttack,
	          Build.Stats.PhysicalAttack);

	Card.Effects[1].Target = TEXT("MovementSpeed");
	Card.Effects[1].Value = 0.25f;
	FReEchoBuildSnapshot AppliedBuild = Build;
	TestTrue(TEXT("Valid multi-effect card applies"),
	         ReEchoRunData::TryApplyCardEffectsToBuild(Card, Build, AppliedBuild));
	TestEqual(TEXT("First valid effect is present after success"), AppliedBuild.Stats.PhysicalAttack, 15.0f);
	TestEqual(TEXT("Second valid effect is present after success"), AppliedBuild.Stats.MovementSpeed, 1.25f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoResolvedCardOutcomeProjectionTest,
                                 "ReEcho.Traits.ResolvedOutcomesPersistAndProject",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoResolvedCardOutcomeProjectionTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));

	auto FindOwnedCard = [](const FReEchoWeaponPartShopView& View, const FName CardId)
	{
		return View.OwnedCards.FindByPredicate(
		    [&](const FReEchoShopOffer& Offer)
		    {
			    return Offer.ContentId == CardId;
		    });
	};

	Run->EncounterIndex = 5;
	TestTrue(TEXT("Harvest penalty card grants for outcome projection"), Run->DebugGrantCard(TEXT("G_3_17")));
	const EReEchoCardEconomyPenalty Penalty = Run->CurrentBuild.CardState.Runtime.EconomyPenalty;
	const FReEchoWeaponPartShopView HarvestView = Run->GetWeaponPartShopView();
	const FReEchoShopOffer* Harvest = FindOwnedCard(HarvestView, TEXT("G_3_17"));
	if (!TestNotNull(TEXT("Harvest is present in the owned-card projection"), Harvest))
	{
		return false;
	}
	const FString HarvestOutcome = Harvest->OutcomeText.ToString();
	TestFalse(TEXT("Harvest exposes a non-empty resolved penalty"), HarvestOutcome.IsEmpty());
	if (Penalty == EReEchoCardEconomyPenalty::NoShopRefresh)
	{
		TestTrue(TEXT("Harvest text matches the no-refresh rule"), HarvestOutcome.Contains(TEXT("不能再刷新商店")));
	}
	else if (Penalty == EReEchoCardEconomyPenalty::NoExtraCardPurchase)
	{
		TestTrue(TEXT("Harvest text matches the no-extra-card rule"),
		         HarvestOutcome.Contains(TEXT("不能再购买额外卡牌组")));
	}
	else
	{
		TestTrue(TEXT("Harvest text matches the no-enemy-shard rule"),
		         HarvestOutcome.Contains(TEXT("不再掉落时间碎片")));
	}

	Run->EncounterIndex = 2;
	TestTrue(TEXT("Random stat trade grants for generic outcome projection"), Run->DebugGrantCard(TEXT("G_2_04")));
	const FReEchoWeaponPartShopView StatTradeView = Run->GetWeaponPartShopView();
	const FReEchoShopOffer* StatTrade = FindOwnedCard(StatTradeView, TEXT("G_2_04"));
	if (TestNotNull(TEXT("Random stat trade is present in the owned-card projection"), StatTrade))
	{
		const FString StatTradeOutcome = StatTrade->OutcomeText.ToString();
		TestTrue(TEXT("Random stat trade names physical attack"), StatTradeOutcome.Contains(TEXT("物理攻击力")));
		TestTrue(TEXT("Random stat trade names elemental attack"), StatTradeOutcome.Contains(TEXT("元素攻击力")));
		TestTrue(TEXT("Random stat trade exposes the resolved positive roll"), StatTradeOutcome.Contains(TEXT("+60%")));
		TestTrue(TEXT("Random stat trade exposes the resolved negative roll"), StatTradeOutcome.Contains(TEXT("-30%")));
	}

	Run->EncounterIndex = 1;
	TestTrue(TEXT("Hunt tracker grants for pending outcome projection"), Run->DebugGrantCard(TEXT("G_2_05")));
	const FReEchoWeaponPartShopView PendingHuntView = Run->GetWeaponPartShopView();
	const FReEchoShopOffer* PendingHunt = FindOwnedCard(PendingHuntView, TEXT("G_2_05"));
	if (!TestNotNull(TEXT("Hunt tracker is projected while pending"), PendingHunt))
	{
		return false;
	}
	TestTrue(TEXT("Hunt tracker shows its pending encounter"),
	         PendingHunt->OutcomeText.ToString().Contains(TEXT("等待")));
	Run->BeginEncounter();
	const float PhysicalBefore = Run->CurrentBuild.Stats.PhysicalAttack;
	for (int32 KillIndex = 0; KillIndex < 7; ++KillIndex)
	{
		Run->NotifyCardKill(false);
	}
	Run->CompleteEncounter(FReEchoRecording(), true, false);
	TestEqual(TEXT("Hunt tracker still applies its authored permanent gain"),
	          Run->CurrentBuild.Stats.PhysicalAttack,
	          PhysicalBefore + 1.0f);
	const FReEchoWeaponPartShopView SettledHuntView = Run->GetWeaponPartShopView();
	const FReEchoShopOffer* SettledHunt = FindOwnedCard(SettledHuntView, TEXT("G_2_05"));
	if (TestNotNull(TEXT("Settled hunt remains projected"), SettledHunt))
	{
		TestTrue(TEXT("Settled hunt exposes the actual physical gain"),
		         SettledHunt->OutcomeText.ToString().Contains(TEXT("物理攻击力 +1")));
	}

	Run->EncounterIndex = 1;
	TestTrue(TEXT("Reaction tracker grants for pending outcome projection"), Run->DebugGrantCard(TEXT("G_2_06")));
	const TSharedPtr<const FReEchoCsvDataSnapshot> ReactionSnapshot = Run->GetRunDataSnapshot();
	const FReEchoCardDefinition* ReactionCard = ReactionSnapshot.IsValid() && ReactionSnapshot->CardCatalog.IsValid()
	                                                ? ReactionSnapshot->CardCatalog->Find(TEXT("G_2_06"))
	                                                : nullptr;
	if (!TestTrue(TEXT("Reaction tracker has an authored effect"), ReactionCard && !ReactionCard->Effects.IsEmpty()))
	{
		return false;
	}
	const FReEchoCardEffectDefinition& ReactionEffect = ReactionCard->Effects[0];
	const int32 ReactionThreshold = FMath::RoundToInt(ReactionEffect.ParamValue);
	if (!TestTrue(TEXT("Reaction tracker has a positive threshold"), ReactionThreshold > 0))
	{
		return false;
	}
	constexpr int32 ReactionCount = 9;
	const float ExpectedReactionGain = (ReactionCount / ReactionThreshold) * ReactionEffect.Value;
	const FString ExpectedReactionText = FString::Printf(TEXT("元素攻击力 +%g"), ExpectedReactionGain);
	Run->BeginEncounter();
	const float ElementalBefore = Run->CurrentBuild.Stats.ElementalAttack;
	for (int32 ReactionIndex = 0; ReactionIndex < ReactionCount; ++ReactionIndex)
	{
		Run->NotifyCardReaction(*FString::Printf(TEXT("REACTION_%d"), ReactionIndex), true);
	}
	Run->CompleteEncounter(FReEchoRecording(), true, false);
	TestEqual(TEXT("Reaction tracker still applies its authored permanent gain"),
	          Run->CurrentBuild.Stats.ElementalAttack,
	          ElementalBefore + ExpectedReactionGain);
	const FReEchoWeaponPartShopView SettledReactionView = Run->GetWeaponPartShopView();
	const FReEchoShopOffer* SettledReaction = FindOwnedCard(SettledReactionView, TEXT("G_2_06"));
	if (TestNotNull(TEXT("Settled reaction tracker remains projected"), SettledReaction))
	{
		TestTrue(TEXT("Settled reaction tracker exposes the actual elemental gain"),
		         SettledReaction->OutcomeText.ToString().Contains(ExpectedReactionText));
	}

	UReEchoRunSaveGame* Save = Run->CreateSaveSnapshot();
	UGameInstance* RestoredGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Restored = NewObject<UReEchoRunSubsystem>(RestoredGameInstance);
	if (TestNotNull(TEXT("Resolved outcomes can be saved"), Save) &&
	    TestTrue(TEXT("Resolved outcomes restore with SaveVersion 19"), Restored->RestoreSaveSnapshot(*Save)))
	{
		const FReEchoWeaponPartShopView RestoredView = Restored->GetWeaponPartShopView();
		const FReEchoShopOffer* RestoredHarvest = FindOwnedCard(RestoredView, TEXT("G_3_17"));
		const FReEchoShopOffer* RestoredHunt = FindOwnedCard(RestoredView, TEXT("G_2_05"));
		const FReEchoShopOffer* RestoredReaction = FindOwnedCard(RestoredView, TEXT("G_2_06"));
		TestTrue(TEXT("Harvest outcome survives save/load"),
		         RestoredHarvest && RestoredHarvest->OutcomeText.ToString() == HarvestOutcome);
		TestTrue(TEXT("Hunt outcome survives save/load"),
		         RestoredHunt && RestoredHunt->OutcomeText.ToString().Contains(TEXT("物理攻击力 +1")));
		TestTrue(TEXT("Reaction outcome survives save/load"),
		         RestoredReaction && RestoredReaction->OutcomeText.ToString().Contains(ExpectedReactionText));
	}

	UReEchoRunSaveGame* LegacySave = Run->CreateSaveSnapshot();
	UGameInstance* LegacyGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* LegacyRestored = NewObject<UReEchoRunSubsystem>(LegacyGameInstance);
	if (TestNotNull(TEXT("Legacy outcome migration has a source snapshot"), LegacySave))
	{
		LegacySave->SaveVersion = 18;
		LegacySave->CurrentBuild.CardState.Runtime.ResolvedOutcomes.Reset();
		if (TestTrue(TEXT("SaveVersion 18 outcome state migrates"), LegacyRestored->RestoreSaveSnapshot(*LegacySave)))
		{
			const FReEchoWeaponPartShopView LegacyView = LegacyRestored->GetWeaponPartShopView();
			const FReEchoShopOffer* LegacyHarvest = FindOwnedCard(LegacyView, TEXT("G_3_17"));
			const FReEchoShopOffer* LegacyHunt = FindOwnedCard(LegacyView, TEXT("G_2_05"));
			const FReEchoShopOffer* LegacyReaction = FindOwnedCard(LegacyView, TEXT("G_2_06"));
			TestTrue(TEXT("Legacy migration reconstructs the provable harvest penalty"),
			         LegacyHarvest && !LegacyHarvest->OutcomeText.IsEmpty());
			TestTrue(TEXT("Legacy migration does not invent a completed hunt result"),
			         LegacyHunt && LegacyHunt->OutcomeText.IsEmpty());
			TestTrue(TEXT("Legacy migration does not invent a completed reaction result"),
			         LegacyReaction && LegacyReaction->OutcomeText.IsEmpty());
		}
	}
	return true;
}

#endif
