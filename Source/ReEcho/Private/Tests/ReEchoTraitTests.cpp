#include "Misc/AutomationTest.h"
#include "Data/ReEchoCsvDataRegistry.h"
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
		RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_02"));
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
	RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_02"));
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
	TestTrue(TEXT("The selected card enters the build"),
	         RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Contains(SelectedCardId));

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
	RunSubsystem->StartRun(TEXT("J_SPADE"), NAME_None);
	TestEqual(TEXT("CSV default weapon is used when none is supplied"),
	          RunSubsystem->CurrentBuild.WeaponId,
	          FName(TEXT("W_J_02")));
	TestEqual(
	    TEXT("CSV character base physical attack is used"), RunSubsystem->CurrentBuild.Stats.PhysicalAttack, 5.0f);

	RunSubsystem->Phase = EReEchoRunPhase::CardChoice;
	const TArray<FReEchoTraitCardOffer> Offers = RunSubsystem->GenerateTraitCardOffers(39);
	TestEqual(TEXT("All 39 enabled trait cards are offered"), Offers.Num(), 39);
	if (!Offers.ContainsByPredicate(
	        [](const FReEchoTraitCardOffer& Offer)
	        {
		        return Offer.CardId == TEXT("G_1_03");
	        }))
	{
		AddError(TEXT("G_1_03 was not present in the 39-card trait pool"));
		return false;
	}
	const FReEchoTraitCardOffer* TaggedOffer = Offers.FindByPredicate(
	    [](const FReEchoTraitCardOffer& Offer)
	    {
		    return Offer.CardId == TEXT("G_2_05");
	    });
	TestNotNull(TEXT("G_2_05 is available for tag projection"), TaggedOffer);
	if (TaggedOffer)
	{
		TestEqual(TEXT("Trait offers preserve the authored Tags column order"), TaggedOffer->Tags.Num(), 3);
		TestTrue(TEXT("Trait offer includes the authored Critical tag"), TaggedOffer->Tags.Contains(TEXT("Critical")));
		TestTrue(TEXT("Trait offer includes the authored Echo tag"), TaggedOffer->Tags.Contains(TEXT("Echo")));
		TestTrue(TEXT("Trait offer includes the authored Body tag"), TaggedOffer->Tags.Contains(TEXT("Body")));
	}

	const float PhysicalBefore = RunSubsystem->CurrentBuild.Stats.PhysicalAttack;
	TestTrue(TEXT("A CSV numeric trait can be applied"), RunSubsystem->ApplyTraitCard(TEXT("G_1_03")));
	TestEqual(TEXT("Physical attack add comes from card_effects.csv"),
	          RunSubsystem->CurrentBuild.Stats.PhysicalAttack,
	          PhysicalBefore + 4.0f);
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
	EnabledWeapon.WeaponTypeId = TEXT("Dagger");
	EnabledWeapon.AttackPatternId = TEXT("Pattern.DaggerCombo");
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

#endif
