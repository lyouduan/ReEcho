#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Run/ReEchoRunSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoTraitOffersAreDeterministicTest,
                                 "ReEcho.Traits.OffersAreDeterministicAndDiverse",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoTraitOffersAreDeterministicTest::RunTest(const FString& Parameters)
{
	auto PrepareChoice = []()
	{
		UGameInstance* GameInstance = NewObject<UGameInstance>();
		UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
		RunSubsystem->StartRun(TEXT("J_CAT"), TEXT("W_J_02"));
		RunSubsystem->BeginEncounter();
		RunSubsystem->CompleteEncounter(FReEchoRecording(), true, false);
		return RunSubsystem;
	};

	UReEchoRunSubsystem* FirstRun = PrepareChoice();
	UReEchoRunSubsystem* SecondRun = PrepareChoice();
	const TArray<FReEchoTraitCardOffer> FirstOffers = FirstRun->GenerateTraitCardOffers(3);
	const TArray<FReEchoTraitCardOffer> SecondOffers = SecondRun->GenerateTraitCardOffers(3);

	TestEqual(TEXT("A draw returns three offers"), FirstOffers.Num(), 3);
	TestEqual(TEXT("Equivalent run state returns three offers"), SecondOffers.Num(), 3);
	TSet<FName> UniqueIds;
	for (int32 Index = 0; Index < FirstOffers.Num(); ++Index)
	{
		UniqueIds.Add(FirstOffers[Index].CardId);
		if (SecondOffers.IsValidIndex(Index))
		{
			TestEqual(TEXT("Equivalent run state preserves offer order"),
			          FirstOffers[Index].CardId,
			          SecondOffers[Index].CardId);
		}
	}
	TestEqual(TEXT("A draw never repeats a card"), UniqueIds.Num(), FirstOffers.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoTraitOfferApplicationTest,
                                 "ReEcho.Traits.AppliesOnlyPendingOffer",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoTraitOfferApplicationTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_CAT"), TEXT("W_J_02"));
	RunSubsystem->BeginEncounter();
	RunSubsystem->CompleteEncounter(FReEchoRecording(), true, false);
	const TArray<FReEchoTraitCardOffer> FirstOffers = RunSubsystem->GenerateTraitCardOffers(3);
	if (!TestEqual(TEXT("Initial draw returns three offers"), FirstOffers.Num(), 3))
	{
		return false;
	}

	TestFalse(TEXT("A card outside the pending offer is rejected"), RunSubsystem->ApplyTraitCard(TEXT("INVALID")));
	const FName SelectedCardId = FirstOffers[0].CardId;
	TestTrue(TEXT("A pending card can be applied"), RunSubsystem->ApplyTraitCard(SelectedCardId));
	TestFalse(TEXT("The same offer cannot be applied twice"), RunSubsystem->ApplyTraitCard(SelectedCardId));
	TestTrue(TEXT("The selected card enters the build"), RunSubsystem->CurrentBuild.Cards.Contains(SelectedCardId));

	RunSubsystem->BeginEncounter();
	RunSubsystem->CompleteEncounter(FReEchoRecording(), true, false);
	const TArray<FReEchoTraitCardOffer> SecondOffers = RunSubsystem->GenerateTraitCardOffers(3);
	TestEqual(TEXT("The next draw returns three offers"), SecondOffers.Num(), 3);
	TestFalse(TEXT("Unowned traits are preferred before repeating the selected card"),
	          SecondOffers.ContainsByPredicate(
	              [&](const FReEchoTraitCardOffer& Offer)
	              {
		              return Offer.CardId == SelectedCardId;
	              }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoTraitCsvEffectsTest,
                                 "ReEcho.Traits.CsvEffectsApply",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoTraitCsvEffectsTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_CAT"), NAME_None);
	TestEqual(TEXT("CSV default weapon is used when none is supplied"),
	          RunSubsystem->CurrentBuild.WeaponId,
	          FName(TEXT("W_J_02")));
	TestEqual(
	    TEXT("CSV character base physical attack is used"), RunSubsystem->CurrentBuild.Stats.PhysicalAttack, 5.0f);

	RunSubsystem->Phase = EReEchoRunPhase::CardChoice;
	const TArray<FReEchoTraitCardOffer> Offers = RunSubsystem->GenerateTraitCardOffers(6);
	TestEqual(TEXT("Only the six enabled trait cards are offered"), Offers.Num(), 6);
	TestFalse(TEXT("Disabled legacy cards are excluded from offers"),
	          Offers.ContainsByPredicate(
	              [](const FReEchoTraitCardOffer& Offer)
	              {
		              return Offer.CardId == TEXT("G_1_06");
	              }));
	if (!Offers.ContainsByPredicate(
	        [](const FReEchoTraitCardOffer& Offer)
	        {
		        return Offer.CardId == TEXT("G_1_04");
	        }))
	{
		AddError(TEXT("G_1_04 was not present in the six-card trait pool"));
		return false;
	}

	const float PhysicalBefore = RunSubsystem->CurrentBuild.Stats.PhysicalAttack;
	TestTrue(TEXT("A CSV numeric trait can be applied"), RunSubsystem->ApplyTraitCard(TEXT("G_1_04")));
	TestEqual(TEXT("Physical attack add comes from card_effects.csv"),
	          RunSubsystem->CurrentBuild.Stats.PhysicalAttack,
	          PhysicalBefore + 2.0f);
	return true;
}

#endif
