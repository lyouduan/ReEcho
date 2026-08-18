#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
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
	RunSubsystem->StartRun(TEXT("J_CAT"), TEXT("W_J_01"));
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
	TestTrue(TEXT("Promotion is applied only once"), !ReEchoCharacterPromotion::TryPromote(Hunter));

	FReEchoBuildSnapshot Poet;
	Poet.CardState.OwnedCardIds = {TEXT("G_1_04"), TEXT("G_1_04"), TEXT("G_1_06"), TEXT("G_1_01")};
	ReEchoCharacterPromotion::TryPromote(Poet);
	TestEqual(TEXT("Element majority promotes Poet"), Poet.Stats.RoleId, FName(TEXT("Poet")));
	TestTrue(TEXT("Poet projectiles use random combat elements"), Poet.Stats.bRandomElementProjectiles);

	FReEchoBuildSnapshot Brave;
	Brave.CardState.OwnedCardIds = {TEXT("G_1_02"), TEXT("G_1_02"), TEXT("G_2_14"), TEXT("G_1_01")};
	ReEchoCharacterPromotion::TryPromote(Brave);
	TestEqual(TEXT("Survival majority promotes Brave"), Brave.Stats.RoleId, FName(TEXT("Brave")));
	TestTrue(TEXT("Brave has a second-hit bonus"), Brave.Stats.EverySecondAttackBonus > 0.0f);

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

	auto ApplyAvailableCard = [this, RunSubsystem]()
	{
		RunSubsystem->Phase = EReEchoRunPhase::CardChoice;
		const TArray<FReEchoTraitCardOffer> Offers = RunSubsystem->GenerateTraitCardOffers(3);
		return Offers.Num() > 0 && RunSubsystem->ApplyTraitCard(Offers[0].CardId);
	};

	for (int32 Index = 0; Index < 4; ++Index)
	{
		TestTrue(TEXT("A normal Sage trait choice applies"), ApplyAvailableCard());
	}
	TestEqual(TEXT("Four normal choices open one Sage bonus choice"), RunSubsystem->Phase, EReEchoRunPhase::CardChoice);
	TestTrue(TEXT("The Sage bonus choice applies"), ApplyAvailableCard());
	TestEqual(TEXT("The Sage bonus choice does not recursively grant another bonus"),
	          RunSubsystem->Phase,
	          EReEchoRunPhase::Planning);

	for (int32 Index = 0; Index < 3; ++Index)
	{
		TestTrue(TEXT("A later normal Sage trait choice applies"), ApplyAvailableCard());
		TestEqual(TEXT("Fewer than four later normal choices grant no bonus"),
		          RunSubsystem->Phase,
		          EReEchoRunPhase::Planning);
	}
	TestTrue(TEXT("The fourth later normal Sage trait choice applies"), ApplyAvailableCard());
	TestEqual(TEXT("The next Sage bonus waits for four additional normal choices"),
	          RunSubsystem->Phase,
	          EReEchoRunPhase::CardChoice);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBraveForgeTest,
                                 "ReEcho.Characters.BraveForge",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBraveForgeTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->StartRun(TEXT("J_HEART"), TEXT("W_J_01"));
	RunSubsystem->CurrentBuild.Stats.RoleId = TEXT("Brave");
	RunSubsystem->CurrentBuild.EquipmentBaseStats.RoleId = TEXT("Brave");
	RunSubsystem->BeginEncounter();
	RunSubsystem->CompleteEncounter(FReEchoRecording(), true, false);
	TestEqual(
	    TEXT("Brave enters forge choice after a cleared encounter"), RunSubsystem->Phase, EReEchoRunPhase::ForgeChoice);

	const TArray<FReEchoTraitCardOffer> Offers = RunSubsystem->GenerateForgeOffers();
	TestEqual(TEXT("Forge presents three risk levels"), Offers.Num(), 3);
	const float HealthBefore = RunSubsystem->CurrentBuild.Stats.HpMax;
	TestTrue(TEXT("Pending extreme forge can be applied"), RunSubsystem->ApplyForgeChoice(TEXT("FORGE_EXTREME")));
	TestEqual(
	    TEXT("Extreme forge consumes six maximum health"), RunSubsystem->CurrentBuild.Stats.HpMax, HealthBefore - 6.0f);
	TestEqual(TEXT("Forge continues into regular card choice"), RunSubsystem->Phase, EReEchoRunPhase::CardChoice);
	return true;
}

#endif
