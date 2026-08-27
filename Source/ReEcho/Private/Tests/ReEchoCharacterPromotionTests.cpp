#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Run/CharacterAbilities/ReEchoCharacterAbilityRuntime.h"
#include "Run/ReEchoCharacterPromotion.h"
#include "Run/ReEchoRunSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCharacterPromotionRoleTest,
                                 "ReEcho.Characters.PromotionRoles",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCharacterPromotionRoleTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	TestEqual(TEXT("A new run starts with fifteen maximum health"), RunSubsystem->CurrentBuild.Stats.HpMax, 15.0f);

	FReEchoBuildSnapshot Hunter;
	Hunter.Stats.HpMax = 15.0f;
	Hunter.Stats.PhysicalAttack = 5.0f;
	Hunter.Stats.ElementalAttack = 5.0f;
	Hunter.CardState.OwnedCardIds = {TEXT("G_1_03"), TEXT("G_1_03"), TEXT("G_1_05"), TEXT("G_1_02")};
	TestTrue(TEXT("Four cards promote the initial character"), ReEchoCharacterPromotion::TryPromote(Hunter));
	TestEqual(TEXT("Physical majority promotes Hunter"), Hunter.Stats.RoleId, FName(TEXT("Hunter")));
	TestEqual(TEXT("Hunter uses the diamond character art"), Hunter.CharacterId, FName(TEXT("J_DIAMOND")));
	TestEqual(TEXT("Hunter promotion leaves twelve maximum health"), Hunter.Stats.HpMax, 12.0f);
	TestEqual(TEXT("Hunter static ability grants twenty percent movement"), Hunter.Stats.MovementSpeed, 1.2f);
	TestEqual(TEXT("Hunter static ability grants twenty critical-rate points"), Hunter.Stats.CriticalRate, 0.4f);
	TestEqual(TEXT("Hunter static ability grants thirty critical-effect points"), Hunter.Stats.CriticalEffect, 0.8f);
	TestTrue(TEXT("Promotion is applied only once"), !ReEchoCharacterPromotion::TryPromote(Hunter));
	UReEchoRunSubsystem* HunterRun = NewObject<UReEchoRunSubsystem>(GameInstance);
	HunterRun->StartRun(TEXT("J_DIAMOND"), TEXT("W_J_01"));
	TestEqual(
	    TEXT("Hunter starts with configured movement ability once"), HunterRun->CurrentBuild.Stats.MovementSpeed, 1.2f);
	TestEqual(TEXT("Hunter starts with configured critical-rate ability once"),
	          HunterRun->CurrentBuild.Stats.CriticalRate,
	          0.4f);
	TestEqual(TEXT("Hunter starts with configured critical-effect ability once"),
	          HunterRun->CurrentBuild.Stats.CriticalEffect,
	          0.8f);

	FReEchoBuildSnapshot Poet;
	Poet.CardState.OwnedCardIds = {TEXT("G_1_04"), TEXT("G_1_04"), TEXT("G_1_06"), TEXT("G_1_01")};
	ReEchoCharacterPromotion::TryPromote(Poet);
	TestEqual(TEXT("Element majority promotes Poet"), Poet.Stats.RoleId, FName(TEXT("Poet")));

	FReEchoBuildSnapshot Brave;
	Brave.CardState.OwnedCardIds = {TEXT("G_1_02"), TEXT("G_1_02"), TEXT("G_2_14"), TEXT("G_1_01")};
	ReEchoCharacterPromotion::TryPromote(Brave);
	TestEqual(TEXT("Survival majority promotes Brave"), Brave.Stats.RoleId, FName(TEXT("Brave")));

	FReEchoBuildSnapshot Sage;
	Sage.CardState.OwnedCardIds = {TEXT("G_1_01"), TEXT("G_1_08"), TEXT("G_1_07"), TEXT("G_1_05")};
	ReEchoCharacterPromotion::TryPromote(Sage);
	TestEqual(TEXT("Utility majority promotes Sage"), Sage.Stats.RoleId, FName(TEXT("Sage")));

	TestEqual(TEXT("Tie priority chooses physical first"),
	          ReEchoCharacterPromotion::EvaluateRole({TEXT("G_1_03"), TEXT("G_1_04"), TEXT("G_1_02"), TEXT("G_1_01")}),
	          FName(TEXT("Hunter")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoSageBonusCadenceTest,
                                 "ReEcho.Characters.SageBonusCadence",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoSageBonusCadenceTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	RunSubsystem->CurrentBuild.Stats.RoleId = TEXT("Sage");
	RunSubsystem->CurrentBuild.RuleFlags.Add(TEXT("Promoted"), TEXT("1"));
	RunSubsystem->CurrentBuild.EquipmentBaseStats.RoleId = TEXT("Sage");
	RunSubsystem->CurrentBuild.EquipmentBaseRuleFlags.Add(TEXT("Promoted"), TEXT("1"));
	RunSubsystem->EncounterIndex = 2;

	auto ApplyAvailableCard = [this, RunSubsystem]()
	{
		RunSubsystem->Phase = EReEchoRunPhase::CardChoice;
		const TArray<FReEchoTraitCardOffer> Offers = RunSubsystem->GenerateTraitCardOffers(3);
		return Offers.Num() > 0 && RunSubsystem->ApplyTraitCard(Offers[0].CardId);
	};

	for (int32 Index = 0; Index < 5; ++Index)
	{
		TestTrue(TEXT("A normal Sage trait choice applies"), ApplyAvailableCard());
	}
	TestEqual(TEXT("Five normal choices open one Sage bonus choice"), RunSubsystem->Phase, EReEchoRunPhase::CardChoice);
	TestTrue(TEXT("The Sage bonus choice applies"), ApplyAvailableCard());
	TestEqual(TEXT("The Sage bonus choice does not recursively grant another bonus"),
	          RunSubsystem->Phase,
	          EReEchoRunPhase::Planning);

	for (int32 Index = 0; Index < 4; ++Index)
	{
		TestTrue(TEXT("A later normal Sage trait choice applies"), ApplyAvailableCard());
		TestEqual(TEXT("Fewer than five later normal choices grant no bonus"),
		          RunSubsystem->Phase,
		          EReEchoRunPhase::Planning);
	}
	TestTrue(TEXT("The fifth later normal Sage trait choice applies"), ApplyAvailableCard());
	TestEqual(TEXT("The next Sage bonus waits for five additional normal choices"),
	          RunSubsystem->Phase,
	          EReEchoRunPhase::CardChoice);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoSageMixedCardGroupCadenceTest,
                                 "ReEcho.Characters.SageBonusCadenceCountsShopGroups",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoSageMixedCardGroupCadenceTest::RunTest(const FString&)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	Run->CurrentBuild.Stats.RoleId = TEXT("Sage");
	Run->CurrentBuild.RuleFlags.Add(TEXT("Promoted"), TEXT("1"));
	Run->CurrentBuild.EquipmentBaseStats.RoleId = TEXT("Sage");
	Run->CurrentBuild.EquipmentBaseRuleFlags.Add(TEXT("Promoted"), TEXT("1"));
	Run->EncounterIndex = 4;
	for (int32 GroupIndex = 0; GroupIndex < 4; ++GroupIndex)
	{
		Run->Phase = EReEchoRunPhase::CardChoice;
		const TArray<FReEchoTraitCardOffer> Offers = Run->GenerateTraitCardOffers(3);
		if (!TestEqual(TEXT("Each ordinary free group exposes three cards"), Offers.Num(), 3) ||
		    !TestTrue(TEXT("An ordinary free group can be claimed"), Run->ApplyTraitCard(Offers[0].CardId)))
		{
			return false;
		}
	}
	TestEqual(TEXT("Four free card groups are counted"),
	          FCString::Atoi(*Run->CurrentBuild.RuleFlags.FindRef(TEXT("NormalTraitSelections"))),
	          4);
	TestEqual(TEXT("Four groups do not open the Sage bonus"), Run->Phase, EReEchoRunPhase::Planning);
	UReEchoRunSaveGame* FourGroupSave = Run->CreateSaveSnapshot();
	UGameInstance* FourGroupRestoredGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* FourGroupRestored = NewObject<UReEchoRunSubsystem>(FourGroupRestoredGameInstance);
	if (!TestNotNull(TEXT("The four-group Sage cadence can be saved"), FourGroupSave) ||
	    !TestTrue(TEXT("The four-group Sage cadence restores"), FourGroupRestored->RestoreSaveSnapshot(*FourGroupSave)))
	{
		return false;
	}
	Run = FourGroupRestored;
	TestEqual(TEXT("Restore preserves the four-group Sage cadence"),
	          FCString::Atoi(*Run->CurrentBuild.RuleFlags.FindRef(TEXT("NormalTraitSelections"))),
	          4);

	Run->TimeShards = 1000;
	const FReEchoWeaponPartShopView ShopView = Run->GetWeaponPartShopView();
	const FReEchoShopCardPackOffer* AvailablePack = ShopView.CardPackOffers.FindByPredicate(
	    [](const FReEchoShopCardPackOffer& Pack)
	    {
		    return Pack.IsAvailable();
	    });
	if (!TestNotNull(TEXT("Encounter four exposes a shop card group"), AvailablePack))
	{
		return false;
	}
	const int32 Tier = AvailablePack->Tier;
	TestTrue(TEXT("Prepaying the fifth group succeeds"), Run->PurchaseShopCardPackDetailed(Tier).IsSuccess());
	TestEqual(TEXT("Prepayment alone does not advance the Sage cadence"),
	          FCString::Atoi(*Run->CurrentBuild.RuleFlags.FindRef(TEXT("NormalTraitSelections"))),
	          4);
	FString RefreshError;
	Run->TryRefreshShopCardSlot(Tier, 0, RefreshError);
	TestEqual(TEXT("Refreshing a paid group does not advance the Sage cadence"),
	          FCString::Atoi(*Run->CurrentBuild.RuleFlags.FindRef(TEXT("NormalTraitSelections"))),
	          4);
	UReEchoRunSaveGame* PaidGroupSave = Run->CreateSaveSnapshot();
	UGameInstance* PaidGroupRestoredGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* PaidGroupRestored = NewObject<UReEchoRunSubsystem>(PaidGroupRestoredGameInstance);
	if (!TestNotNull(TEXT("The paid pending group can be saved"), PaidGroupSave) ||
	    !TestTrue(TEXT("The paid pending group restores"), PaidGroupRestored->RestoreSaveSnapshot(*PaidGroupSave)))
	{
		return false;
	}
	Run = PaidGroupRestored;

	const FReEchoWeaponPartShopView PaidView = Run->GetWeaponPartShopView();
	const FReEchoShopCardPackOffer* PaidPack = PaidView.CardPackOffers.FindByPredicate(
	    [Tier](const FReEchoShopCardPackOffer& Pack)
	    {
		    return Pack.Tier == Tier && Pack.Status == EReEchoShopCardPackStatus::PaidPendingChoice;
	    });
	if (!TestTrue(TEXT("The prepaid group retains a claimable choice"), PaidPack && !PaidPack->Choices.IsEmpty()))
	{
		return false;
	}
	const FName ClaimedItemId = PaidPack->Choices[0].ItemId;
	TestTrue(TEXT("Claiming the restored shop group succeeds"), Run->ClaimPaidShopCardChoice(ClaimedItemId).IsSuccess());
	TestEqual(TEXT("The claimed shop group is the fifth ordinary group"),
	          FCString::Atoi(*Run->CurrentBuild.RuleFlags.FindRef(TEXT("NormalTraitSelections"))),
	          5);
	TestEqual(TEXT("The fifth mixed-source group opens exactly one Sage bonus choice"),
	          Run->Phase,
	          EReEchoRunPhase::CardChoice);
	TestFalse(TEXT("Claiming the same paid group twice is rejected"),
	          Run->ClaimPaidShopCardChoice(ClaimedItemId).IsSuccess());
	TestEqual(TEXT("A rejected duplicate claim does not advance the Sage cadence"),
	          FCString::Atoi(*Run->CurrentBuild.RuleFlags.FindRef(TEXT("NormalTraitSelections"))),
	          5);

	const TArray<FReEchoTraitCardOffer> BonusOffers = Run->GenerateTraitCardOffers(3);
	if (!TestEqual(TEXT("The Sage bonus reuses the three-card choice flow"), BonusOffers.Num(), 3))
	{
		return false;
	}
	TestTrue(TEXT("The Sage bonus card can be claimed"), Run->ApplyTraitCard(BonusOffers[0].CardId));
	TestEqual(TEXT("The bonus itself does not increment ordinary group count"),
	          FCString::Atoi(*Run->CurrentBuild.RuleFlags.FindRef(TEXT("NormalTraitSelections"))),
	          5);
	TestEqual(TEXT("One bonus resolves back to planning"), Run->Phase, EReEchoRunPhase::Planning);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoDataDrivenCharacterAbilitiesTest,
                                 "ReEcho.Characters.DataDrivenAbilities",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoDataDrivenCharacterAbilitiesTest::RunTest(const FString& Parameters)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	TestTrue(TEXT("Character ability snapshot is published"), Snapshot.IsValid());
	if (!Snapshot.IsValid())
	{
		return false;
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_HEART"), TEXT("W_J_01"));
	RunSubsystem->BeginEncounter();
	RunSubsystem->CompleteEncounter(FReEchoRecording(), true, false);
	TestEqual(
	    TEXT("Brave skips the unconfigured encounter-one card choice"), RunSubsystem->Phase, EReEchoRunPhase::Planning);
	RunSubsystem->BeginEncounter();
	RunSubsystem->CompleteEncounter(FReEchoRecording(), true, false);
	TestEqual(
	    TEXT("Brave uses the configured encounter-two card choice"), RunSubsystem->Phase, EReEchoRunPhase::CardChoice);
	TestTrue(TEXT("Forge cards are absent from production data"), Snapshot->FindCard(TEXT("FORGE_LIGHT")) == nullptr);
	FReEchoCsvDataSnapshot ConfigurableSnapshot = *Snapshot;
	if (FReEchoCsvCharacterAbilityRow* SageAbility =
	        ConfigurableSnapshot.CharacterAbilities.Find(TEXT("SAGE_BONUS_CHOICE")))
	{
		SageAbility->Interval = 2.0f;
	}
	TestEqual(TEXT("Changing the ability interval changes cadence without a code constant"),
	          ReEchoCharacterAbilityRuntime::ResolveExtraTraitChoices(ConfigurableSnapshot, TEXT("J_SPADE"), 2),
	          1);

	const FVector2D OneStep =
	    ReEchoCharacterAbilityRuntime::ResolveCurrentMissingHealthAttackBonus(*Snapshot, TEXT("J_HEART"), 18.0f, 20.0f);
	const FVector2D TwoSteps =
	    ReEchoCharacterAbilityRuntime::ResolveCurrentMissingHealthAttackBonus(*Snapshot, TEXT("J_HEART"), 15.9f, 20.0f);
	const FVector2D HealedBack =
	    ReEchoCharacterAbilityRuntime::ResolveCurrentMissingHealthAttackBonus(*Snapshot, TEXT("J_HEART"), 18.0f, 20.0f);
	const FVector2D FullHealth =
	    ReEchoCharacterAbilityRuntime::ResolveCurrentMissingHealthAttackBonus(*Snapshot, TEXT("J_HEART"), 20.0f, 20.0f);
	TestEqual(TEXT("Ten percent missing grants one physical point"), OneStep.X, 1.0);
	TestEqual(TEXT("Ten percent missing grants one elemental point"), OneStep.Y, 1.0);
	TestEqual(TEXT("Twenty percent missing grants two physical points"), TwoSteps.X, 2.0);
	TestEqual(TEXT("Healing across a threshold rolls back to one stack"), HealedBack.X, 1.0);
	TestTrue(TEXT("Full health clears Brave stacks"), FullHealth.IsNearlyZero());

	UReEchoCombatantComponent* Combatant = NewObject<UReEchoCombatantComponent>();
	FReEchoStatBlock BaseStats;
	BaseStats.PhysicalAttack = 2.0f;
	BaseStats.ElementalAttack = 3.0f;
	Combatant->InitializeFromStats(BaseStats, true);
	Combatant->SetAdditiveAttackModifier(TEXT("Character.MissingHealthSteps"), TwoSteps.X, TwoSteps.Y);
	TestEqual(TEXT("Combat applies current physical stacks"), Combatant->Stats.PhysicalAttack, 4.0f);
	Combatant->SetAdditiveAttackModifier(TEXT("Character.MissingHealthSteps"), HealedBack.X, HealedBack.Y);
	TestEqual(TEXT("Combat replaces rather than accumulates stacks"), Combatant->Stats.PhysicalAttack, 3.0f);

	UReEchoRunSubsystem* PoetRun = NewObject<UReEchoRunSubsystem>(GameInstance);
	PoetRun->StartRun(TEXT("J_CLOVER"), TEXT("W_J_01"));
	const float ReactionBefore = PoetRun->CurrentBuild.Stats.ReactionEfficiency;
	PoetRun->BeginEncounter();
	PoetRun->CompleteEncounter(FReEchoRecording(), true, false);
	TestEqual(TEXT("Poet gains ten reaction-efficiency points per completed encounter"),
	          PoetRun->CurrentBuild.Stats.ReactionEfficiency,
	          ReactionBefore + 0.1f);
	PoetRun->CompleteEncounter(FReEchoRecording(), true, false);
	TestEqual(TEXT("A repeated completion callback does not grant Poet growth twice"),
	          PoetRun->CurrentBuild.Stats.ReactionEfficiency,
	          ReactionBefore + 0.1f);
	UReEchoRunSubsystem* FailedPoetRun = NewObject<UReEchoRunSubsystem>(GameInstance);
	FailedPoetRun->StartRun(TEXT("J_CLOVER"), TEXT("W_J_01"));
	const float FailedReactionBefore = FailedPoetRun->CurrentBuild.Stats.ReactionEfficiency;
	FailedPoetRun->BeginEncounter();
	FailedPoetRun->CompleteEncounter(FReEchoRecording(), false, false);
	TestEqual(TEXT("A failed encounter does not grant Poet growth"),
	          FailedPoetRun->CurrentBuild.Stats.ReactionEfficiency,
	          FailedReactionBefore);
	return true;
}

#endif
