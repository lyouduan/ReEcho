#if WITH_DEV_AUTOMATION_TESTS

#include "Cards/ReEchoCardRuntime.h"
#include "Misc/AutomationTest.h"

namespace
{
FReEchoCardDefinition MakeCard(const FName Id,
                               const int32 Tier,
                               const FName Behavior,
                               const FName Trigger,
                               const FName Target,
                               const float Value,
                               const FName ParamName = NAME_None,
                               const float ParamValue = 0.0f,
                               const bool bUnique = false)
{
	FReEchoCardDefinition Card;
	Card.Id = Id;
	Card.Tier = Tier;
	Card.DisplayName = Id.ToString();
	Card.OfferGroup = TEXT("Trait");
	Card.StackPolicy = bUnique ? TEXT("Unique") : TEXT("Stackable");
	Card.ConflictPolicy = TEXT("None");
	Card.bEnabled = true;
	Card.bOfferable = true;
	FReEchoCardEffectDefinition Effect;
	Effect.Id = FName(*(Id.ToString() + TEXT("_E")));
	Effect.Order = 1;
	Effect.Trigger = Trigger;
	Effect.BehaviorId = Behavior;
	Effect.Target = Target;
	Effect.Value = Value;
	Effect.ParamName = ParamName;
	Effect.ParamValue = ParamValue;
	Card.Effects.Add(Effect);
	return Card;
}

FReEchoCardCatalog BuildCatalog(const TArray<FReEchoCardDefinition>& Cards)
{
	FReEchoCardCatalog Catalog;
	FString Error;
	ensureAlways(Catalog.Initialize(Cards, TEXT("cards-test-v1"), Error));
	return Catalog;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCardTierGrantTest,
                                 "ReEcho.Cards.Grant.TierIsAtomicAndDeterministic",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCardTierGrantTest::RunTest(const FString&)
{
	TArray<FReEchoCardDefinition> Cards;
	Cards.Add(
	    MakeCard(TEXT("G_1_08"), 1, TEXT("Card.GrantTier"), TEXT("OnGrant"), TEXT("Tier"), 2.0f, TEXT("Count"), 1.0f));
	Cards.Add(MakeCard(TEXT("L2_A"), 2, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("PhysicalAttack"), 4.0f));
	Cards.Add(MakeCard(TEXT("L2_B"), 2, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("ElementalAttack"), 4.0f));
	const FReEchoCardCatalog Catalog = BuildCatalog(Cards);
	FReEchoCardGrantInput Input;
	Input.CardState.DomainRevision = Catalog.GetDomainRevision();
	Input.Stats.PhysicalAttack = 10.0f;
	Input.Stats.ElementalAttack = 10.0f;
	Input.RandomSeed = 91;
	const FReEchoCardGrantResult First = ReEchoCardRuntime::TryGrantCard(Catalog, TEXT("G_1_08"), Input);
	const FReEchoCardGrantResult Second = ReEchoCardRuntime::TryGrantCard(Catalog, TEXT("G_1_08"), Input);
	TestTrue(TEXT("Tier grant succeeds"), First.bSucceeded);
	TestEqual(TEXT("Parent plus one tier card are granted"), First.GrantedCardIds.Num(), 2);
	TestEqual(TEXT("The same seed selects the same tier card"), First.GrantedCardIds, Second.GrantedCardIds);
	TestTrue(TEXT("Both grants become owned"),
	         First.CardState.OwnedCardIds.Contains(TEXT("G_1_08")) && First.CardState.OwnedCardIds.Num() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCardEncounterRulesTest,
                                 "ReEcho.Cards.Encounter.ThresholdsAndDamageResources",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCardEncounterRulesTest::RunTest(const FString&)
{
	TArray<FReEchoCardDefinition> Cards;
	FReEchoCardDefinition Stun = MakeCard(TEXT("G_2_17"),
	                                      2,
	                                      TEXT("Card.EncounterStun"),
	                                      TEXT("OnEncounterTick"),
	                                      TEXT("Stun"),
	                                      2.0f,
	                                      TEXT("TriggerSeconds"),
	                                      10.0f,
	                                      true);
	FReEchoCardEffectDefinition Stun20 = Stun.Effects[0];
	Stun20.Id = TEXT("G_2_17_E20");
	Stun20.Order = 2;
	Stun20.ParamValue = 20.0f;
	Stun.Effects.Add(Stun20);
	Cards.Add(Stun);
	Cards.Add(MakeCard(TEXT("G_2_14"),
	                   2,
	                   TEXT("Card.DamageSubstitution"),
	                   TEXT("BeforeIncomingHit"),
	                   TEXT("Damage"),
	                   0.0f,
	                   TEXT("FreeHitCount"),
	                   3.0f));
	Cards.Add(MakeCard(TEXT("G_3_19"),
	                   3,
	                   TEXT("Card.ShardOutgoingDamage"),
	                   TEXT("BeforeOutgoingHit"),
	                   TEXT("Damage"),
	                   0.2f,
	                   TEXT("ShardCost"),
	                   1.0f));
	Cards.Add(MakeCard(TEXT("G_3_20"),
	                   3,
	                   TEXT("Card.ShardIncomingBarrier"),
	                   TEXT("BeforeIncomingHit"),
	                   TEXT("Damage"),
	                   -1.0f,
	                   TEXT("ShardCost"),
	                   5.0f));
	const FReEchoCardCatalog Catalog = BuildCatalog(Cards);
	FReEchoCardBuildState State;
	State.DomainRevision = Catalog.GetDomainRevision();
	State.OwnedCardIds = {TEXT("G_2_17"), TEXT("G_2_14"), TEXT("G_3_19"), TEXT("G_3_20")};
	State = ReEchoCardRuntime::BeginEncounter(State, 2);
	const FReEchoCardEncounterTickResult Tick10 = ReEchoCardRuntime::AdvanceEncounter(Catalog, State, 10.0f);
	const FReEchoCardEncounterTickResult Tick20 = ReEchoCardRuntime::AdvanceEncounter(Catalog, Tick10.CardState, 20.0f);
	TestEqual(TEXT("10 seconds fires once"), Tick10.EnemyStunDurations.Num(), 1);
	TestEqual(TEXT("20 seconds fires once"), Tick20.EnemyStunDurations.Num(), 1);

	FReEchoCardBuildState IncomingState = Tick20.CardState;
	int32 Shards = 10;
	for (int32 HitIndex = 0; HitIndex < 3; ++HitIndex)
	{
		const FReEchoCardIncomingHitResult Hit =
		    ReEchoCardRuntime::ModifyIncomingHit(Catalog, IncomingState, 5.0f, Shards);
		TestEqual(TEXT("The first three hits are zero"), Hit.RawDamage, 0.0f);
		IncomingState = Hit.CardState;
		Shards = Hit.TimeShards;
	}
	const FReEchoCardIncomingHitResult Fourth =
	    ReEchoCardRuntime::ModifyIncomingHit(Catalog, IncomingState, 5.0f, Shards);
	TestEqual(TEXT("Fourth hit consumes five shards and subtracts one"), Fourth.RawDamage, 4.0f);
	TestEqual(TEXT("Incoming barrier cost"), Fourth.TimeShards, 5);
	TestTrue(TEXT("Fourth hit requests echo removal"), Fourth.bRemoveAllEchoes);

	FReEchoCardOutgoingHitInput Outgoing;
	Outgoing.RawDamage = 10.0f;
	Outgoing.TimeShards = 1;
	const FReEchoCardOutgoingHitResult Boosted = ReEchoCardRuntime::ModifyOutgoingHit(Catalog, State, Outgoing);
	TestEqual(TEXT("One shard multiplies outgoing damage"), Boosted.RawDamage, 12.0f);
	TestEqual(TEXT("Outgoing shard is consumed"), Boosted.TimeShards, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCardEconomyRuleTest,
                                 "ReEcho.Cards.Economy.RulesAndFreeRefresh",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCardEconomyRuleTest::RunTest(const FString&)
{
	TArray<FReEchoCardDefinition> Cards;
	Cards.Add(
	    MakeCard(TEXT("G_2_16"), 2, TEXT("Card.ShopContract"), TEXT("OnCompileRules"), TEXT("ShopDiscount"), 0.2f));
	Cards.Add(MakeCard(TEXT("G_3_16"),
	                   3,
	                   TEXT("Card.EndKillRefresh"),
	                   TEXT("OnEncounterEnd"),
	                   TEXT("FreeShopRefresh"),
	                   1.0f,
	                   TEXT("KillThreshold"),
	                   10.0f));
	const FReEchoCardCatalog Catalog = BuildCatalog(Cards);
	FReEchoCardBuildState State;
	State.DomainRevision = Catalog.GetDomainRevision();
	State.OwnedCardIds = {TEXT("G_2_16"), TEXT("G_3_16")};
	State.Runtime.ActiveEncounterIndex = 1;
	State.Runtime.EncounterKillCount = 29;
	const FReEchoCardRuleSnapshot Rules = ReEchoCardRuntime::CompileRules(Catalog, State);
	TestEqual(TEXT("Shop discount compiles"), Rules.ShopDiscount, 0.2f);
	const FReEchoCardEventResult End = ReEchoCardRuntime::EndEncounter(Catalog, State, FReEchoStatBlock{}, 1, 0);
	TestEqual(TEXT("29 kills grant floor(29/10) refreshes"), End.CardState.Runtime.FreeShopRefreshes, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCardPersistenceAndRollCountTest,
                                 "ReEcho.Cards.State.PersistentProgressAndExactCritRolls",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCardPersistenceAndRollCountTest::RunTest(const FString&)
{
	TArray<FReEchoCardDefinition> Cards;
	Cards.Add(
	    MakeCard(TEXT("G_1_08"), 1, TEXT("Card.GrantTier"), TEXT("OnGrant"), TEXT("Tier"), 2.0f, TEXT("Count"), 1.0f));
	Cards.Add(MakeCard(
	    TEXT("G_3_10"), 3, TEXT("Card.ElementCanCrit"), TEXT("BeforeOutgoingHit"), TEXT("ElementCanCrit"), 1.0f));
	Cards.Add(MakeCard(
	    TEXT("G_3_12"), 3, TEXT("Card.DoubleCritRoll"), TEXT("BeforeOutgoingHit"), TEXT("CriticalRollCount"), 2.0f));
	const FReEchoCardCatalog Catalog = BuildCatalog(Cards);

	FReEchoCardGrantInput GrantInput;
	GrantInput.CardState.DomainRevision = Catalog.GetDomainRevision();
	GrantInput.CardState.OwnedCardIds = {TEXT("G_3_10")};
	GrantInput.Stats.PhysicalAttack = 12.0f;
	GrantInput.TimeShards = 9;
	const FReEchoCardGrantResult Failed = ReEchoCardRuntime::TryGrantCard(Catalog, TEXT("G_1_08"), GrantInput);
	TestFalse(TEXT("Tier grant fails when no legal tier candidate exists"), Failed.bSucceeded);
	TestEqual(TEXT("Failed grant returns the original card state"),
	          Failed.CardState.OwnedCardIds,
	          GrantInput.CardState.OwnedCardIds);
	TestEqual(TEXT("Failed grant returns the original stats"), Failed.Stats.PhysicalAttack, 12.0f);
	TestEqual(TEXT("Failed grant returns the original currency"), Failed.TimeShards, 9);
	TestTrue(TEXT("Failed grant exposes no partial grant list"), Failed.GrantedCardIds.IsEmpty());

	FReEchoCardBuildState PersistentState;
	PersistentState.DomainRevision = Catalog.GetDomainRevision();
	PersistentState.Runtime.PlayerKillProgress = 14;
	PersistentState.Runtime.EchoKillProgress = 7;
	PersistentState.Runtime.DistinctReactionIds = {TEXT("Burn"), TEXT("Bloom"), TEXT("Shock")};
	const FReEchoCardBuildState Begun = ReEchoCardRuntime::BeginEncounter(PersistentState, 4);
	TestEqual(TEXT("Player kill remainder persists between encounters"), Begun.Runtime.PlayerKillProgress, 14);
	TestEqual(TEXT("Echo kill remainder persists between encounters"), Begun.Runtime.EchoKillProgress, 7);
	TestEqual(TEXT("Reaction diversity set persists between encounters"), Begun.Runtime.DistinctReactionIds.Num(), 3);

	FReEchoCardBuildState CritState;
	CritState.DomainRevision = Catalog.GetDomainRevision();
	CritState.OwnedCardIds = {TEXT("G_3_10"), TEXT("G_3_12")};
	FReEchoCardOutgoingHitInput ElementHit;
	ElementHit.RawDamage = 10.0f;
	ElementHit.Element = EReEchoElement::Flame;
	ElementHit.CriticalRate = 0.0f;
	const FReEchoCardOutgoingHitResult ElementResult =
	    ReEchoCardRuntime::ModifyOutgoingHit(Catalog, CritState, ElementHit);
	TestEqual(
	    TEXT("Element damage performs exactly two failed rolls"), ElementResult.CardState.Runtime.RandomSequence, 2);

	CritState.Runtime.RandomSequence = 0;
	CritState.OwnedCardIds = {TEXT("G_3_12")};
	FReEchoCardOutgoingHitInput PhysicalHit;
	PhysicalHit.RawDamage = 10.0f;
	PhysicalHit.CriticalRate = 0.0f;
	const FReEchoCardOutgoingHitResult PhysicalResult =
	    ReEchoCardRuntime::ModifyOutgoingHit(Catalog, CritState, PhysicalHit);
	TestEqual(TEXT("Physical damage consumes one extra roll after the caller's first failed roll"),
	          PhysicalResult.CardState.Runtime.RandomSequence,
	          1);
	return true;
}

#endif
