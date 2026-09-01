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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoOwnedCardTierOfferRulesTest,
                                 "ReEcho.Cards.Offer.OwnedTierOneRepeatsHigherTiersAreExcluded",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoOwnedCardTierOfferRulesTest::RunTest(const FString&)
{
	const FReEchoCardDefinition TierOne =
	    MakeCard(TEXT("TIER_ONE"), 1, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("PhysicalAttack"), 1.0f);
	const FReEchoCardDefinition TierTwoOwned =
	    MakeCard(TEXT("TIER_TWO_OWNED"), 2, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("PhysicalAttack"), 2.0f);
	const FReEchoCardDefinition TierTwoUnowned = MakeCard(
	    TEXT("TIER_TWO_UNOWNED"), 2, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("ElementalAttack"), 2.0f);
	const FReEchoCardCatalog Catalog = BuildCatalog({TierOne, TierTwoOwned, TierTwoUnowned});
	FReEchoCardBuildState State;
	State.DomainRevision = Catalog.GetDomainRevision();
	State.OwnedCardIds = {TierOne.Id, TierTwoOwned.Id};

	TestTrue(TEXT("An owned tier-one card remains offerable"), ReEchoCardRuntime::CanOffer(Catalog, State, TierOne));
	TestFalse(TEXT("An owned tier-two card is no longer offerable"),
	          ReEchoCardRuntime::CanOffer(Catalog, State, TierTwoOwned));
	const TArray<FReEchoCardDefinition> TierOnePool =
	    ReEchoCardRuntime::BuildOfferPool(Catalog, State, TEXT("Trait"), 1);
	TestEqual(TEXT("The tier-one pool retains its owned repeatable card"), TierOnePool.Num(), 1);
	TestEqual(TEXT("The repeatable tier-one card remains in the pool"), TierOnePool[0].Id, TierOne.Id);
	const TArray<FReEchoCardDefinition> TierTwoPool =
	    ReEchoCardRuntime::BuildOfferPool(Catalog, State, TEXT("Trait"), 2);
	TestEqual(TEXT("The tier-two pool contains only the unowned card"), TierTwoPool.Num(), 1);
	TestEqual(TEXT("The remaining tier-two offer is unowned"), TierTwoPool[0].Id, TierTwoUnowned.Id);

	FReEchoCardGrantInput RepeatInput;
	RepeatInput.CardState = State;
	const FReEchoCardGrantResult RepeatGrant = ReEchoCardRuntime::TryGrantCard(Catalog, TierOne.Id, RepeatInput);
	TestTrue(TEXT("An offered tier-one card can be granted again"), RepeatGrant.bSucceeded);
	TestEqual(TEXT("A repeated tier-one grant adds another stack"),
	          ReEchoCardRuntime::CountOwned(RepeatGrant.CardState, TierOne.Id),
	          2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEgaoBonusGrantTest,
                                 "ReEcho.Cards.Egao.BonusCardIsGrantedAndFiresItsEffect",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEgaoBonusGrantTest::RunTest(const FString&)
{
	// A tier-one card that only 样样都通 can deliver.
	const FReEchoCardDefinition TierOne =
	    MakeCard(TEXT("G_1_01"), 1, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("PhysicalAttack"), 1.0f);
	// 样样都通: on grant it hands out every enabled tier-one card.
	const FReEchoCardDefinition AllRounder =
	    MakeCard(TEXT("G_3_23"), 3, TEXT("Card.GrantAllTier1"), TEXT("OnGrant"), TEXT("Tier"), 1.0f);
	// The card the player actually picked.
	const FReEchoCardDefinition Chosen =
	    MakeCard(TEXT("G_2_05"), 2, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("ElementalAttack"), 2.0f);

	const FReEchoCardCatalog Catalog = BuildCatalog({TierOne, AllRounder, Chosen});
	FReEchoCardGrantInput Input;
	Input.CardState.DomainRevision = Catalog.GetDomainRevision();
	Input.RandomSeed = 20260901;

	const FReEchoCardGrantResult Grant = ReEchoCardRuntime::TryGrantCard(Catalog, Chosen.Id, Input);

	TestTrue(TEXT("The chosen card is granted"), Grant.bSucceeded);
	TestTrue(TEXT("The chosen card is owned"), Grant.CardState.OwnedCardIds.Contains(Chosen.Id));
	TestTrue(TEXT("The bonus 样样都通 is granted"), Grant.CardState.OwnedCardIds.Contains(TEXT("G_3_23")));
	// Every system-given G_3_23 is a real grant and must resolve its GrantAllTier1 effect.
	TestTrue(TEXT("The system bonus card also grants tier-one cards"),
	         ReEchoCardRuntime::CountOwned(Grant.CardState, TierOne.Id) >= 1);
	TestTrue(TEXT("The bonus card stacks at least one copy"),
	         ReEchoCardRuntime::CountOwned(Grant.CardState, TEXT("G_3_23")) >= 1);
	const int32 PreviousBonusCount = ReEchoCardRuntime::CountOwned(Grant.CardState, TEXT("G_3_23"));
	const int32 PreviousTierOneCount = ReEchoCardRuntime::CountOwned(Grant.CardState, TierOne.Id);
	FReEchoCardGrantInput LaterInput;
	LaterInput.CardState = Grant.CardState;
	LaterInput.Stats = Grant.Stats;
	LaterInput.RandomSeed = 20260902;
	const FReEchoCardGrantResult LaterGrant = ReEchoCardRuntime::TryGrantCard(Catalog, TierOne.Id, LaterInput);
	TestTrue(TEXT("A later card-pack grant succeeds"), LaterGrant.bSucceeded);
	TestTrue(TEXT("A later grant adds another system bonus"),
	         ReEchoCardRuntime::CountOwned(LaterGrant.CardState, TEXT("G_3_23")) > PreviousBonusCount);
	TestTrue(TEXT("A later system bonus resolves its tier-one grant"),
	         ReEchoCardRuntime::CountOwned(LaterGrant.CardState, TierOne.Id) > PreviousTierOneCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoGrantAllTierOneCardsTest,
                                 "ReEcho.Cards.Grant.GrantAllTierOneAddsEveryEnabledCard",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoGrantAllTierOneCardsTest::RunTest(const FString&)
{
	const FReEchoCardDefinition TierOneOwned =
	    MakeCard(TEXT("TIER_ONE_OWNED"), 1, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("PhysicalAttack"), 1.0f);
	const FReEchoCardDefinition TierOneMissing = MakeCard(
	    TEXT("TIER_ONE_MISSING"), 1, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("ElementalAttack"), 1.0f);
	const FReEchoCardDefinition TierOneHealth =
	    MakeCard(TEXT("TIER_ONE_HEALTH"), 1, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("HpMax"), 8.0f);
	FReEchoCardDefinition TierOneDisabled =
	    MakeCard(TEXT("TIER_ONE_DISABLED"), 1, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("HpMax"), 1.0f);
	TierOneDisabled.bEnabled = false;
	TierOneDisabled.bOfferable = false;
	const FReEchoCardDefinition GrantAll =
	    MakeCard(TEXT("GRANT_ALL_TIER_ONE"), 3, TEXT("Card.GrantAllTier1"), TEXT("OnGrant"), TEXT("Tier"), 1.0f);
	FReEchoCardCatalog Catalog;
	FString CatalogError;
	if (!TestTrue(TEXT("Grant-all fixture satisfies the catalog contract"),
	              Catalog.Initialize({TierOneOwned, TierOneHealth, TierOneMissing, TierOneDisabled, GrantAll},
	                                 TEXT("cards-test-v1"),
	                                 CatalogError)))
	{
		AddError(CatalogError);
		return false;
	}

	FReEchoCardGrantInput Input;
	Input.CardState.DomainRevision = Catalog.GetDomainRevision();
	Input.CardState.OwnedCardIds = {TierOneOwned.Id};
	const FReEchoCardGrantResult Grant = ReEchoCardRuntime::TryGrantCard(Catalog, GrantAll.Id, Input);

	TestTrue(TEXT("Grant-all transaction succeeds"), Grant.bSucceeded);
	TestEqual(TEXT("An already owned enabled tier-one card receives another stack"),
	          ReEchoCardRuntime::CountOwned(Grant.CardState, TierOneOwned.Id),
	          2);
	TestEqual(TEXT("A missing enabled tier-one card is granted"),
	          ReEchoCardRuntime::CountOwned(Grant.CardState, TierOneMissing.Id),
	          1);
	TestEqual(
	    TEXT("An enabled health card is granted"), ReEchoCardRuntime::CountOwned(Grant.CardState, TierOneHealth.Id), 1);
	TestEqual(TEXT("A disabled tier-one card is not granted"),
	          ReEchoCardRuntime::CountOwned(Grant.CardState, TierOneDisabled.Id),
	          0);
	TestEqual(TEXT("A health adjustment survives later non-health grants in the same transaction"),
	          Grant.HealthAdjustment,
	          EReEchoHealthAdjustment::SetToStatPoint);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEncounterAndConflictOfferRulesTest,
                                 "ReEcho.Cards.Offer.EncounterAndConflictsAreShared",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEncounterAndConflictOfferRulesTest::RunTest(const FString&)
{
	FReEchoCardDefinition Restricted =
	    MakeCard(TEXT("RESTRICTED"), 2, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("PhysicalAttack"), 1.0f);
	Restricted.Tags = {TEXT("OfferEncounter2"), TEXT("OfferEncounter3")};
	FReEchoCardDefinition Tide =
	    MakeCard(TEXT("TIDE"), 2, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("ElementalAttack"), 1.0f);
	Tide.ConflictPolicy = TEXT("EchoElementAura");
	FReEchoCardDefinition Forest =
	    MakeCard(TEXT("FOREST"), 2, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("ElementalAttack"), 1.0f);
	Forest.ConflictPolicy = TEXT("EchoElementAura");
	FReEchoCardDefinition EchoCard =
	    MakeCard(TEXT("ECHO_CARD"), 2, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("EchoEfficiency"), 0.1f);
	EchoCard.Tags = {TEXT("Echo")};
	FReEchoCardDefinition Solo =
	    MakeCard(TEXT("G_3_03"), 3, TEXT("Card.SoloBody"), TEXT("OnGrant"), TEXT("AllBaseStats"), 2.0f);
	Solo.ConflictPolicy = TEXT("EchoKeystone");
	const FReEchoCardCatalog Catalog = BuildCatalog({Restricted, Tide, Forest, EchoCard, Solo});

	FReEchoCardBuildState EmptyState;
	EmptyState.DomainRevision = Catalog.GetDomainRevision();
	TestTrue(TEXT("Encounter-tagged card appears in an allowed encounter"),
	         ReEchoCardRuntime::CanOffer(Catalog, EmptyState, Restricted, 2));
	TestFalse(TEXT("Encounter-tagged card is excluded from other encounters"),
	          ReEchoCardRuntime::CanOffer(Catalog, EmptyState, Restricted, 5));

	FReEchoCardBuildState TideState = EmptyState;
	TideState.OwnedCardIds.Add(Tide.Id);
	TestFalse(TEXT("Shared conflict policy excludes the opposite card"),
	          ReEchoCardRuntime::CanOffer(Catalog, TideState, Forest, 2));
	FReEchoCardBuildState EchoState = EmptyState;
	EchoState.OwnedCardIds.Add(EchoCard.Id);
	TestFalse(TEXT("Solo-body offer is excluded after owning an echo card"),
	          ReEchoCardRuntime::CanOffer(Catalog, EchoState, Solo, 2));
	FReEchoCardBuildState SoloState = EmptyState;
	SoloState.OwnedCardIds.Add(Solo.Id);
	TestFalse(TEXT("Echo-card offer is excluded after owning solo body"),
	          ReEchoCardRuntime::CanOffer(Catalog, SoloState, EchoCard, 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoTauntEchoRetirementRuleTest,
                                 "ReEcho.Cards.Echo.TauntEchoRetiresOnDefeat",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoTauntEchoRetirementRuleTest::RunTest(const FString&)
{
	FReEchoCardDefinition TauntEcho =
	    MakeCard(TEXT("TAUNT_ECHO"), 3, TEXT("Card.TauntEcho"), TEXT("OnCompileRules"), TEXT("EchoHealth"), 2.0f);
	FReEchoCardEffectDefinition DeathReward;
	DeathReward.Id = TEXT("TAUNT_ECHO_DEATH");
	DeathReward.Order = 2;
	DeathReward.Trigger = TEXT("OnEchoKilled");
	DeathReward.BehaviorId = TEXT("Card.TauntEcho");
	DeathReward.Target = TEXT("HpMaxAndPoint");
	DeathReward.Value = 5.0f;
	TauntEcho.Effects.Add(DeathReward);
	const FReEchoCardCatalog Catalog = BuildCatalog({TauntEcho});
	FReEchoCardBuildState State;
	State.DomainRevision = Catalog.GetDomainRevision();
	State.OwnedCardIds = {TauntEcho.Id};

	const FReEchoCardRuleSnapshot Rules = ReEchoCardRuntime::CompileRules(Catalog, State);
	TestFalse(TEXT("Taunt Echo still cannot attack"), Rules.bEchoesCanAttack);
	TestTrue(TEXT("Taunt Echo explicitly retires after authoritative defeat"), Rules.bRetireEchoOnDefeat);
	TestTrue(TEXT("Taunt Echo still taunts enemies"), Rules.bEchoTaunts);

	FReEchoStatBlock Stats;
	Stats.HpMax = 20.0f;
	Stats.HpPoint = 12.0f;
	const FReEchoCardEventResult Reward = ReEchoCardRuntime::OnEchoKilled(Catalog, State, Stats);
	TestEqual(TEXT("Existing defeat reward still increases maximum health"), Reward.Stats.HpMax, 25.0f);
	TestEqual(TEXT("Existing defeat reward still heals current health"), Reward.Stats.HpPoint, 17.0f);
	TestEqual(TEXT("The defeat reward requests live-health synchronization"),
	          Reward.HealthAdjustment,
	          EReEchoHealthAdjustment::SetToStatPoint);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoReactionHealPercentTest,
                                 "ReEcho.Cards.ReactionHeal.UsesMaxHealthPercentAndConfiguredCooldown",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoReactionHealPercentTest::RunTest(const FString&)
{
	const FReEchoCardDefinition Card = MakeCard(TEXT("REACTION_HEAL"),
	                                            2,
	                                            TEXT("Card.ReactionHeal"),
	                                            TEXT("OnReaction"),
	                                            TEXT("HpPoint"),
	                                            0.1f,
	                                            TEXT("CooldownSeconds"),
	                                            2.0f,
	                                            true);
	const FReEchoCardCatalog Catalog = BuildCatalog({Card});
	FReEchoCardBuildState State;
	State.DomainRevision = Catalog.GetDomainRevision();
	State.OwnedCardIds.Add(Card.Id);
	FReEchoStatBlock Stats;
	Stats.HpMax = 120.0f;
	Stats.HpPoint = 40.0f;

	const FReEchoCardEventResult Result = ReEchoCardRuntime::OnReaction(Catalog, State, Stats, TEXT("Y_ER_F_G"), true);
	TestEqual(TEXT("Reaction heal uses ten percent of maximum health"), Result.Healing, 12.0f);
	TestEqual(TEXT("Reaction heal uses the configured two-second cooldown"),
	          Result.CardState.Runtime.ReactionHealCooldownRemaining,
	          2.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRandomRateTradeTest,
                                 "ReEcho.Cards.Grant.RandomRateTradeIsDeterministic",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRandomRateTradeTest::RunTest(const FString&)
{
	const FReEchoCardDefinition Card = MakeCard(TEXT("RATE_TRADE"),
	                                            2,
	                                            TEXT("Card.RandomRateTrade"),
	                                            TEXT("OnGrant"),
	                                            TEXT("ReactionOrCritical"),
	                                            2.0f,
	                                            TEXT("PenaltyMultiplier"),
	                                            0.5f,
	                                            true);
	const FReEchoCardCatalog Catalog = BuildCatalog({Card});
	FReEchoCardGrantInput Input;
	Input.CardState.DomainRevision = Catalog.GetDomainRevision();
	Input.Stats.ReactionEfficiency = 1.0f;
	Input.Stats.CriticalEffect = 1.0f;
	Input.RandomSeed = 773;
	const FReEchoCardGrantResult First = ReEchoCardRuntime::TryGrantCard(Catalog, Card.Id, Input);
	const FReEchoCardGrantResult Second = ReEchoCardRuntime::TryGrantCard(Catalog, Card.Id, Input);
	TestTrue(TEXT("Random rate trade grants successfully"), First.bSucceeded);
	TestEqual(TEXT("Identical seed preserves reaction result"),
	          First.Stats.ReactionEfficiency,
	          Second.Stats.ReactionEfficiency);
	TestEqual(
	    TEXT("Identical seed preserves critical result"), First.Stats.CriticalEffect, Second.Stats.CriticalEffect);
	TestTrue(TEXT("Exactly one rate doubles and the other halves"),
	         (FMath::IsNearlyEqual(First.Stats.ReactionEfficiency, 2.0f) &&
	          FMath::IsNearlyEqual(First.Stats.CriticalEffect, 0.5f)) ||
	             (FMath::IsNearlyEqual(First.Stats.ReactionEfficiency, 0.5f) &&
	              FMath::IsNearlyEqual(First.Stats.CriticalEffect, 2.0f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoLowRiskEconomyCardsTest,
                                 "ReEcho.Cards.Grant.LowRiskEconomyCommandsArePersistent",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoLowRiskEconomyCardsTest::RunTest(const FString&)
{
	const FReEchoCardDefinition Reset = MakeCard(TEXT("RESET_RUNES"),
	                                             2,
	                                             TEXT("Card.ResetRunes"),
	                                             TEXT("OnGrant"),
	                                             TEXT("WeaponRunes"),
	                                             300.0f,
	                                             TEXT("FreeShopRefresh"),
	                                             5.0f,
	                                             true);
	const FReEchoCardDefinition FreeShop = MakeCard(TEXT("FREE_SHOP"),
	                                                2,
	                                                TEXT("Card.FreeShopVisit"),
	                                                TEXT("OnGrant"),
	                                                TEXT("ShopPrice"),
	                                                0.0f,
	                                                NAME_None,
	                                                0.0f,
	                                                true);
	FReEchoCardDefinition Unlimited = MakeCard(TEXT("UNLIMITED_REFRESH"),
	                                           2,
	                                           TEXT("Card.UnlimitedShopRefresh"),
	                                           TEXT("OnGrant"),
	                                           TEXT("WeaponRuneShop"),
	                                           1.0f,
	                                           NAME_None,
	                                           0.0f,
	                                           true);
	FReEchoCardEffectDefinition Consume = Unlimited.Effects[0];
	Consume.Id = TEXT("UNLIMITED_REFRESH_CONSUME");
	Consume.Order = 2;
	Consume.Trigger = TEXT("OnPurchase");
	Consume.Value = 0.0f;
	Unlimited.Effects.Add(Consume);
	const FReEchoCardCatalog Catalog = BuildCatalog({Reset, FreeShop, Unlimited});

	FReEchoCardGrantInput Input;
	Input.CardState.DomainRevision = Catalog.GetDomainRevision();
	Input.TimeShards = 100;
	Input.EncounterIndex = 4;
	const FReEchoCardGrantResult ResetResult = ReEchoCardRuntime::TryGrantCard(Catalog, Reset.Id, Input);
	TestTrue(TEXT("Reset task grants successfully"), ResetResult.bSucceeded);
	TestTrue(TEXT("Reset task emits the run-owned rune clear command"), ResetResult.bClearWeaponRunes);
	TestEqual(TEXT("Reset task grants 300 shards"), ResetResult.TimeShards, 400);
	TestEqual(
	    TEXT("Reset task grants five free weapon/rune refreshes"), ResetResult.CardState.Runtime.FreeShopRefreshes, 5);

	Input.CardState = ResetResult.CardState;
	const FReEchoCardGrantResult FreeResult = ReEchoCardRuntime::TryGrantCard(Catalog, FreeShop.Id, Input);
	TestEqual(TEXT("Free shop visit binds to the current post-encounter shop"),
	          FreeResult.CardState.Runtime.FreeShopEncounterIndex,
	          4);
	const FReEchoCardBuildState NextEncounter = ReEchoCardRuntime::BeginEncounter(FreeResult.CardState, 5);
	TestEqual(TEXT("Entering the next encounter consumes the free shop visit"),
	          NextEncounter.Runtime.FreeShopEncounterIndex,
	          INDEX_NONE);

	Input.CardState = NextEncounter;
	const FReEchoCardGrantResult UnlimitedResult = ReEchoCardRuntime::TryGrantCard(Catalog, Unlimited.Id, Input);
	TestTrue(TEXT("Unlimited refresh is armed on grant"),
	         UnlimitedResult.CardState.Runtime.bUnlimitedWeaponRuneRefresh);
	const FReEchoCardEventResult Purchase =
	    ReEchoCardRuntime::OnPurchase(Catalog, UnlimitedResult.CardState, UnlimitedResult.Stats);
	TestFalse(TEXT("The next committed purchase consumes unlimited refresh"),
	          Purchase.CardState.Runtime.bUnlimitedWeaponRuneRefresh);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCoreCollectionCardTest,
                                 "ReEcho.Cards.Inventory.CoreCollectionCompletesOnce",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCoreCollectionCardTest::RunTest(const FString&)
{
	const FReEchoCardDefinition Card = MakeCard(TEXT("CORE_COLLECTION"),
	                                            3,
	                                            TEXT("Card.CollectCores"),
	                                            TEXT("OnInventoryChanged"),
	                                            TEXT("CoreCollection"),
	                                            0.5f,
	                                            TEXT("RequiredCount"),
	                                            6.0f,
	                                            true);
	const FReEchoCardCatalog Catalog = BuildCatalog({Card});
	FReEchoCardBuildState State;
	State.DomainRevision = Catalog.GetDomainRevision();
	State.OwnedCardIds.Add(Card.Id);
	FReEchoStatBlock Stats;
	Stats.CriticalRate = 0.2f;
	Stats.CriticalEffect = 0.5f;
	Stats.ReactionEfficiency = 1.0f;
	const FReEchoCardEventResult Incomplete = ReEchoCardRuntime::OnCoreInventoryChanged(Catalog, State, Stats, 5);
	TestFalse(TEXT("Five cores do not complete the collection"), Incomplete.CardState.Runtime.bDragonSoulCompleted);
	const FReEchoCardEventResult Complete = ReEchoCardRuntime::OnCoreInventoryChanged(Catalog, State, Stats, 6);
	TestTrue(TEXT("Six distinct cores complete the collection"), Complete.CardState.Runtime.bDragonSoulCompleted);
	TestTrue(TEXT("Completion grants all three configured rates"),
	         FMath::IsNearlyEqual(Complete.Stats.CriticalRate, 0.7f) &&
	             FMath::IsNearlyEqual(Complete.Stats.CriticalEffect, 1.0f) &&
	             FMath::IsNearlyEqual(Complete.Stats.ReactionEfficiency, 1.5f));
	const FReEchoCardEventResult Repeated =
	    ReEchoCardRuntime::OnCoreInventoryChanged(Catalog, Complete.CardState, Complete.Stats, 7);
	TestEqual(TEXT("Collection completion is idempotent"), Repeated.Stats.CriticalRate, Complete.Stats.CriticalRate);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoDragonSoulTieredGrantTest,
                                 "ReEcho.Cards.Inventory.DragonSoulGrantsRepeatableTierOne",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoDragonSoulTieredGrantTest::RunTest(const FString&)
{
	const FReEchoCardDefinition TierOne =
	    MakeCard(TEXT("TIER_ONE"), 1, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("PhysicalAttack"), 1.0f);
	const FReEchoCardDefinition TierTwo =
	    MakeCard(TEXT("TIER_TWO"), 2, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("ElementalAttack"), 1.0f);
	const FReEchoCardDefinition TierThree =
	    MakeCard(TEXT("TIER_THREE"), 3, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("HpMax"), 1.0f);
	const FReEchoCardDefinition DragonSoul = MakeCard(TEXT("DRAGON_SOUL"),
	                                                  3,
	                                                  TEXT("Card.CollectCoresGrantTiered"),
	                                                  TEXT("OnInventoryChanged"),
	                                                  TEXT("CoreCollection"),
	                                                  1.0f,
	                                                  TEXT("RequiredCount"),
	                                                  6.0f,
	                                                  true);
	const FReEchoCardCatalog Catalog = BuildCatalog({TierOne, TierTwo, TierThree, DragonSoul});

	FReEchoCardBuildState State;
	State.DomainRevision = Catalog.GetDomainRevision();
	State.OwnedCardIds = {DragonSoul.Id, TierOne.Id};
	const FReEchoCardEventResult Complete = ReEchoCardRuntime::OnCoreInventoryChanged(Catalog, State, {}, 6);

	TestTrue(TEXT("Six cores complete dragon soul"), Complete.CardState.Runtime.bDragonSoulCompleted);
	TestEqual(TEXT("Dragon soul repeats an already owned tier-one Stackable card"),
	          ReEchoCardRuntime::CountOwned(Complete.CardState, TierOne.Id),
	          2);
	TestEqual(
	    TEXT("Dragon soul grants one tier-two card"), ReEchoCardRuntime::CountOwned(Complete.CardState, TierTwo.Id), 1);
	TestEqual(TEXT("Dragon soul grants one tier-three card"),
	          ReEchoCardRuntime::CountOwned(Complete.CardState, TierThree.Id),
	          1);
	return true;
}

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
	const FReEchoCardOutcomeState* GrantedCardsOutcome = First.CardState.Runtime.ResolvedOutcomes.FindByPredicate(
	    [](const FReEchoCardOutcomeState& Outcome)
	    {
		    return Outcome.CardId == TEXT("G_1_08") && Outcome.Kind == EReEchoCardOutcomeKind::GrantedCards;
	    });
	TestTrue(TEXT("Tier grant records the exact granted card for presentation"),
	         GrantedCardsOutcome && GrantedCardsOutcome->RelatedCardIds.Num() == 1 &&
	             GrantedCardsOutcome->RelatedCardIds[0] == First.GrantedCardIds[1]);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBloodForgingGrantTest,
                                 "ReEcho.Cards.Grant.BloodForgingFillsHealth",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBloodForgingGrantTest::RunTest(const FString&)
{
	const FReEchoCardCatalog Catalog = BuildCatalog(
	    {MakeCard(TEXT("BLOOD_FORGING"), 3, TEXT("Card.BloodForging"), TEXT("OnGrant"), TEXT("HpMaxAndPoint"), 1.0f)});
	FReEchoCardGrantInput Input;
	Input.CardState.DomainRevision = Catalog.GetDomainRevision();
	Input.Stats.HpMax = 100.0f;
	Input.Stats.HpPoint = 25.0f;
	Input.Stats.PhysicalAttack = 17.0f;
	Input.Stats.ElementalAttack = 13.0f;

	const FReEchoCardGrantResult Grant = ReEchoCardRuntime::TryGrantCard(Catalog, TEXT("BLOOD_FORGING"), Input);
	TestTrue(TEXT("Blood Forging grants successfully"), Grant.bSucceeded);
	TestEqual(TEXT("Blood Forging adds both attack values to maximum health"), Grant.Stats.HpMax, 130.0f);
	TestEqual(TEXT("Blood Forging fills the build health value"), Grant.Stats.HpPoint, 130.0f);
	TestEqual(TEXT("Blood Forging emits a typed fill-to-maximum request"),
	          Grant.HealthAdjustment,
	          EReEchoHealthAdjustment::FillToMax);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoPermanentHealthGrantTest,
                                 "ReEcho.Cards.Grant.PermanentHealthChangesRequestSynchronization",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoPermanentHealthGrantTest::RunTest(const FString&)
{
	FReEchoCardDefinition LifeStrength =
	    MakeCard(TEXT("G_1_02"), 1, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("HpMax"), 8.0f);
	FReEchoCardEffectDefinition Recovery = LifeStrength.Effects[0];
	Recovery.Id = TEXT("G_1_02_RECOVERY");
	Recovery.Order = 2;
	Recovery.BehaviorId = TEXT("Card.InstantRecovery");
	Recovery.Target = TEXT("HpPoint");
	LifeStrength.Effects.Add(Recovery);
	const FReEchoCardDefinition SoloBody =
	    MakeCard(TEXT("G_3_03"), 3, TEXT("Card.SoloBody"), TEXT("OnGrant"), TEXT("AllBaseStats"), 2.0f);
	const FReEchoCardCatalog Catalog = BuildCatalog({LifeStrength, SoloBody});

	FReEchoCardGrantInput Input;
	Input.CardState.DomainRevision = Catalog.GetDomainRevision();
	Input.Stats.HpMax = 20.0f;
	Input.Stats.HpPoint = 7.0f;
	const FReEchoCardGrantResult LifeGrant = ReEchoCardRuntime::TryGrantCard(Catalog, LifeStrength.Id, Input);
	TestTrue(TEXT("Life Strength grants successfully"), LifeGrant.bSucceeded);
	TestEqual(TEXT("Life Strength increases maximum health by the authored amount"), LifeGrant.Stats.HpMax, 28.0f);
	TestEqual(TEXT("Life Strength increases current health by the authored amount"), LifeGrant.Stats.HpPoint, 15.0f);
	TestEqual(TEXT("Life Strength requests live-health synchronization"),
	          LifeGrant.HealthAdjustment,
	          EReEchoHealthAdjustment::SetToStatPoint);

	Input.CardState = LifeGrant.CardState;
	Input.Stats = LifeGrant.Stats;
	const FReEchoCardGrantResult SoloGrant = ReEchoCardRuntime::TryGrantCard(Catalog, SoloBody.Id, Input);
	TestTrue(TEXT("Solo Body grants successfully"), SoloGrant.bSucceeded);
	TestEqual(TEXT("Solo Body doubles maximum health"), SoloGrant.Stats.HpMax, 56.0f);
	TestEqual(TEXT("Solo Body doubles current health"), SoloGrant.Stats.HpPoint, 30.0f);
	TestEqual(TEXT("Solo Body requests live-health synchronization"),
	          SoloGrant.HealthAdjustment,
	          EReEchoHealthAdjustment::SetToStatPoint);
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
	FReEchoCardDefinition ShopContract =
	    MakeCard(TEXT("G_2_16"), 2, TEXT("Card.ShopContract"), TEXT("OnCompileRules"), TEXT("ShopDiscount"), 0.2f);
	FReEchoCardEffectDefinition PurchaseGrowth = ShopContract.Effects[0];
	PurchaseGrowth.Id = TEXT("G_2_16_PURCHASE");
	PurchaseGrowth.Order = 2;
	PurchaseGrowth.Trigger = TEXT("OnPurchase");
	PurchaseGrowth.Target = TEXT("HpMaxAndPoint");
	PurchaseGrowth.Value = 2.0f;
	ShopContract.Effects.Add(PurchaseGrowth);
	Cards.Add(ShopContract);
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
	const FReEchoCardOutcomeState* RefreshOutcome = End.CardState.Runtime.ResolvedOutcomes.FindByPredicate(
	    [](const FReEchoCardOutcomeState& Outcome)
	    {
		    return Outcome.CardId == TEXT("G_3_16") && Outcome.Kind == EReEchoCardOutcomeKind::FreeShopRefreshes;
	    });
	TestTrue(TEXT("Encounter-end refreshes record their exact cumulative result"),
	         RefreshOutcome && FMath::IsNearlyEqual(RefreshOutcome->PrimaryValue, 2.0f));
	FReEchoStatBlock PurchaseStats;
	PurchaseStats.HpMax = 10.0f;
	PurchaseStats.HpPoint = 5.0f;
	const FReEchoCardEventResult Purchase = ReEchoCardRuntime::OnPurchase(Catalog, End.CardState, PurchaseStats);
	const FReEchoCardOutcomeState* PurchaseOutcome = Purchase.CardState.Runtime.ResolvedOutcomes.FindByPredicate(
	    [](const FReEchoCardOutcomeState& Outcome)
	    {
		    return Outcome.CardId == TEXT("G_2_16") && Outcome.Kind == EReEchoCardOutcomeKind::CumulativeStatGain;
	    });
	TestEqual(TEXT("Shop contract still applies permanent max-health growth"), Purchase.Stats.HpMax, 12.0f);
	TestEqual(TEXT("Shop contract also applies the authored current-health growth"), Purchase.Stats.HpPoint, 7.0f);
	TestEqual(TEXT("Shop contract requests live-health synchronization"),
	          Purchase.HealthAdjustment,
	          EReEchoHealthAdjustment::SetToStatPoint);
	TestTrue(TEXT("Shop contract records the exact cumulative permanent growth"),
	         PurchaseOutcome && PurchaseOutcome->PrimaryTarget == TEXT("HpMaxAndPoint") &&
	             FMath::IsNearlyEqual(PurchaseOutcome->PrimaryValue, 2.0f));
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
	TestEqual(TEXT("Physical damage consumes one extra roll on top of the weapon layer roll"),
	          PhysicalResult.CardState.Runtime.RandomSequence,
	          1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoChainedCriticalTest,
                                 "ReEcho.Cards.Crit.ChainedCriticalStacks",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoChainedCriticalTest::RunTest(const FString&)
{
	TArray<FReEchoCardDefinition> Cards;
	Cards.Add(MakeCard(
	    TEXT("G_3_10"), 3, TEXT("Card.ElementCanCrit"), TEXT("BeforeOutgoingHit"), TEXT("ElementCanCrit"), 1.0f));
	Cards.Add(MakeCard(
	    TEXT("G_3_12"), 3, TEXT("Card.DoubleCritRoll"), TEXT("BeforeOutgoingHit"), TEXT("CriticalRollCount"), 2.0f));
	const FReEchoCardCatalog Catalog = BuildCatalog(Cards);

	const float RawDamage = 10.0f;
	const float CriticalEffect = 0.5f;
	const float SingleCritDamage = RawDamage * (1.0f + CriticalEffect);
	const float DoubleCritDamage = SingleCritDamage * (1.0f + CriticalEffect);

	auto ResolveHit = [&](const TArray<FName>& OwnedCardIds,
	                      const EReEchoElement Element,
	                      const bool bAlreadyCritical) -> FReEchoCardOutgoingHitResult
	{
		FReEchoCardBuildState State;
		State.DomainRevision = Catalog.GetDomainRevision();
		State.OwnedCardIds = OwnedCardIds;
		FReEchoCardOutgoingHitInput Hit;
		Hit.RawDamage = bAlreadyCritical ? SingleCritDamage : RawDamage;
		Hit.CriticalRate = 1.0f;
		Hit.CriticalEffect = CriticalEffect;
		Hit.bCritical = bAlreadyCritical;
		Hit.Element = Element;
		return ReEchoCardRuntime::ModifyOutgoingHit(Catalog, State, Hit);
	};

	const FReEchoCardOutgoingHitResult ElementLocked =
	    ResolveHit({FName(TEXT("G_3_12"))}, EReEchoElement::Flame, false);
	TestFalse(TEXT("Element damage cannot crit until the element-crit card lifts the restriction"),
	          ElementLocked.bCritical);
	TestEqual(TEXT("Locked element damage keeps its raw value"), ElementLocked.RawDamage, RawDamage, 0.001f);

	const FReEchoCardOutgoingHitResult ElementUnlocked =
	    ResolveHit({FName(TEXT("G_3_10"))}, EReEchoElement::Flame, false);
	TestTrue(TEXT("The element-crit card lets element damage crit"), ElementUnlocked.bCritical);
	TestEqual(TEXT("Unlocked element damage takes exactly one crit layer"),
	          ElementUnlocked.RawDamage,
	          SingleCritDamage,
	          0.001f);

	const FReEchoCardOutgoingHitResult ElementChained =
	    ResolveHit({FName(TEXT("G_3_10")), FName(TEXT("G_3_12"))}, EReEchoElement::Flame, false);
	TestTrue(TEXT("Element damage crits twice with both cards"), ElementChained.bCritical);
	TestEqual(TEXT("Element damage applies two crit layers"), ElementChained.RawDamage, DoubleCritDamage, 0.001f);

	const FReEchoCardOutgoingHitResult PhysicalSingle =
	    ResolveHit({FName(TEXT("G_3_12"))}, EReEchoElement::None, false);
	TestEqual(TEXT("A non-crit physical hit gains one layer"), PhysicalSingle.RawDamage, SingleCritDamage, 0.001f);

	const FReEchoCardOutgoingHitResult PhysicalChained =
	    ResolveHit({FName(TEXT("G_3_12"))}, EReEchoElement::None, true);
	TestTrue(TEXT("A critical physical hit stays critical"), PhysicalChained.bCritical);
	TestEqual(TEXT("A critical physical hit crits again when the card is owned"),
	          PhysicalChained.RawDamage,
	          DoubleCritDamage,
	          0.001f);

	const FReEchoCardOutgoingHitResult PhysicalBaseline = ResolveHit({}, EReEchoElement::None, true);
	TestEqual(TEXT("Without the card a weapon crit stays a single layer"),
	          PhysicalBaseline.RawDamage,
	          SingleCritDamage,
	          0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoProgressAndOverkillCardsTest,
                                 "ReEcho.Cards.Events.NumericChallengeOverkillAndSelfRace",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoProgressAndOverkillCardsTest::RunTest(const FString&)
{
	FReEchoCardDefinition Challenge = MakeCard(TEXT("NUMERIC_CHALLENGE"),
	                                           2,
	                                           TEXT("Card.NumericChallenge"),
	                                           TEXT("OnHitResolved"),
	                                           TEXT("TimeShards"),
	                                           666.0f,
	                                           TEXT("KillThreshold"),
	                                           111.0f,
	                                           true);
	FReEchoCardEffectDefinition HighHealth = Challenge.Effects[0];
	HighHealth.Id = TEXT("NUMERIC_CHALLENGE_HIGH");
	HighHealth.Order = 2;
	HighHealth.Trigger = TEXT("OnEncounterEnd");
	HighHealth.ParamName = TEXT("MinimumHp");
	HighHealth.ParamValue = 22.0f;
	Challenge.Effects.Add(HighHealth);
	FReEchoCardEffectDefinition LowHealth = HighHealth;
	LowHealth.Id = TEXT("NUMERIC_CHALLENGE_LOW");
	LowHealth.Order = 3;
	LowHealth.ParamName = TEXT("MaximumHp");
	LowHealth.ParamValue = 3.0f;
	Challenge.Effects.Add(LowHealth);
	const FReEchoCardDefinition Dracula = MakeCard(TEXT("DRACULA_ONE"),
	                                               2,
	                                               TEXT("Card.OverkillHeal"),
	                                               TEXT("OnDamageResolved"),
	                                               TEXT("HpPoint"),
	                                               0.2f,
	                                               NAME_None,
	                                               0.0f,
	                                               true);
	const FReEchoCardDefinition Race = MakeCard(TEXT("SELF_RACE"),
	                                            2,
	                                            TEXT("Card.SelfRace"),
	                                            TEXT("OnEncounterEnd"),
	                                            TEXT("Damage"),
	                                            0.25f,
	                                            NAME_None,
	                                            0.0f,
	                                            true);
	const FReEchoCardDefinition StatusHeal = MakeCard(TEXT("STATUS_HEAL"),
	                                                  2,
	                                                  TEXT("Card.NegativeStatusHeal"),
	                                                  TEXT("OnStatusApplied"),
	                                                  TEXT("HpPoint"),
	                                                  1.0f,
	                                                  NAME_None,
	                                                  0.0f,
	                                                  true);
	const FReEchoCardDefinition SlimeKiller = MakeCard(TEXT("SLIME_KILLER"),
	                                                   2,
	                                                   TEXT("Card.TargetKillCurse"),
	                                                   TEXT("BeforeOutgoingHit"),
	                                                   TEXT("Status"),
	                                                   1.0f,
	                                                   TEXT("M_SLIME"),
	                                                   2.0f,
	                                                   true);
	const FReEchoCardDefinition RecordedReaction = MakeCard(TEXT("RECORDED_REACTION"),
	                                                        3,
	                                                        TEXT("Card.RecordReaction"),
	                                                        TEXT("OnReaction"),
	                                                        TEXT("Damage"),
	                                                        1.0f,
	                                                        TEXT("OtherReactionPenalty"),
	                                                        1.0f,
	                                                        true);
	const FReEchoCardCatalog Catalog =
	    BuildCatalog({Challenge, Dracula, Race, StatusHeal, SlimeKiller, RecordedReaction});
	FReEchoCardBuildState State;
	State.DomainRevision = Catalog.GetDomainRevision();
	State.OwnedCardIds = {Challenge.Id, Dracula.Id, Race.Id, StatusHeal.Id, SlimeKiller.Id, RecordedReaction.Id};
	State = ReEchoCardRuntime::BeginEncounter(State, 2);

	FReEchoCardEventResult Kill;
	for (int32 Index = 0; Index < 111; ++Index)
	{
		Kill = ReEchoCardRuntime::OnKillResolved(Catalog, State, FReEchoStatBlock{}, false);
		State = Kill.CardState;
	}
	TestTrue(TEXT("The 111th kill completes the challenge"), State.Runtime.bNumericChallengeCompleted);
	TestEqual(TEXT("The challenge grants exactly 666 shards once"), Kill.TimeShardsGranted, 666);
	const FReEchoCardEventResult ExtraKill =
	    ReEchoCardRuntime::OnKillResolved(Catalog, State, FReEchoStatBlock{}, false);
	TestEqual(TEXT("A completed challenge cannot grant twice"), ExtraKill.TimeShardsGranted, 0);

	FReEchoCardBuildState HealthState = ReEchoCardRuntime::BeginEncounter(State, 3);
	HealthState.Runtime.bNumericChallengeCompleted = false;
	FReEchoStatBlock HighStats;
	HighStats.HpPoint = 22.0f;
	const FReEchoCardEventResult HealthEnd = ReEchoCardRuntime::EndEncounter(Catalog, HealthState, HighStats, 3, 0);
	TestEqual(TEXT("Ending with 22 health completes the alternate challenge"), HealthEnd.TimeShardsGranted, 666);

	const FReEchoCardEventResult Overkill =
	    ReEchoCardRuntime::OnDamageResolved(Catalog, HealthState, HighStats, 30.0f, 10.0f, false);
	TestEqual(TEXT("Dracula I heals twenty percent of overkill damage"), Overkill.Healing, 4.0f);
	const FReEchoCardEventResult EchoOverkill =
	    ReEchoCardRuntime::OnDamageResolved(Catalog, HealthState, HighStats, 30.0f, 10.0f, true);
	TestEqual(TEXT("Echo overkill does not count as damage dealt by the player"), EchoOverkill.Healing, 0.0f);
	const FReEchoCardEventResult StatusApplied =
	    ReEchoCardRuntime::OnNegativeStatusApplied(Catalog, HealthState, HighStats, TEXT("Z_Bleeding"), true);
	TestEqual(TEXT("A negative status applied by an echo heals the player once"), StatusApplied.Healing, 1.0f);
	FReEchoCardBuildState KillerState = HealthState;
	KillerState = ReEchoCardRuntime::OnKillResolved(Catalog, KillerState, HighStats, false, TEXT("M_SLIME")).CardState;
	KillerState = ReEchoCardRuntime::OnKillResolved(Catalog, KillerState, HighStats, true, TEXT("M_SLIME")).CardState;
	FReEchoCardOutgoingHitInput SlimeHit;
	SlimeHit.RawDamage = 1.0f;
	SlimeHit.TargetDefinitionId = TEXT("M_SLIME");
	TestEqual(TEXT("Echo kills do not advance the player-only slime threshold"),
	          ReEchoCardRuntime::ModifyOutgoingHit(Catalog, KillerState, SlimeHit).PreDamageStatusIds.Num(),
	          0);
	KillerState = ReEchoCardRuntime::OnKillResolved(Catalog, KillerState, HighStats, false, TEXT("M_SLIME")).CardState;
	const FReEchoCardOutgoingHitResult CursingSlimeHit =
	    ReEchoCardRuntime::ModifyOutgoingHit(Catalog, KillerState, SlimeHit);
	TestTrue(TEXT("The completed species threshold applies curse before damage"),
	         CursingSlimeHit.PreDamageStatusIds.Contains(TEXT("Z_Cursed")));
	SlimeHit.TargetDefinitionId = TEXT("M_RABBIT");
	TestEqual(TEXT("The species rule does not affect other enemy definitions"),
	          ReEchoCardRuntime::ModifyOutgoingHit(Catalog, KillerState, SlimeHit).PreDamageStatusIds.Num(),
	          0);
	TestEqual(TEXT("The first reaction is unmodified before it is recorded"),
	          ReEchoCardRuntime::GetReactionDamageMultiplier(Catalog, KillerState, TEXT("Y_Vaporize")),
	          1.0f);
	KillerState = ReEchoCardRuntime::OnReaction(Catalog, KillerState, HighStats, TEXT("Y_Vaporize"), true).CardState;
	TestEqual(TEXT("The recorded reaction receives one hundred percent increased damage"),
	          ReEchoCardRuntime::GetReactionDamageMultiplier(Catalog, KillerState, TEXT("Y_Vaporize")),
	          2.0f);
	TestEqual(TEXT("Other reactions receive one hundred percent reduced damage"),
	          ReEchoCardRuntime::GetReactionDamageMultiplier(Catalog, KillerState, TEXT("Y_Burn")),
	          0.0f);
	KillerState = ReEchoCardRuntime::OnReaction(Catalog, KillerState, HighStats, TEXT("Y_Burn"), true).CardState;
	TestEqual(TEXT("Later reactions cannot replace the first recorded identity"),
	          KillerState.Runtime.RecordedReactionId,
	          FName(TEXT("Y_Vaporize")));

	FReEchoCardBuildState RaceState = HealthState;
	RaceState.Runtime.EncounterPlayerDamage = 100.0f;
	RaceState.Runtime.EncounterEchoDamage = 40.0f;
	const FReEchoCardEventResult RaceEnd = ReEchoCardRuntime::EndEncounter(Catalog, RaceState, HighStats, 3, 0);
	TestEqual(TEXT("Player victory grants the echo next-encounter damage bonus"),
	          RaceEnd.CardState.Runtime.EchoDamageMultiplier,
	          1.25f);
	FReEchoCardOutgoingHitInput EchoHit;
	EchoHit.RawDamage = 10.0f;
	EchoHit.DamageSource = EReEchoDamageSource::Echo;
	const FReEchoCardOutgoingHitResult Boosted =
	    ReEchoCardRuntime::ModifyOutgoingHit(Catalog, RaceEnd.CardState, EchoHit);
	TestEqual(TEXT("The selected self-race bonus modifies final echo outgoing damage"), Boosted.RawDamage, 12.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoDistanceAndSourceCardRulesTest,
                                 "ReEcho.Cards.Runtime.ProximityAndAlternatingSources",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoDistanceAndSourceCardRulesTest::RunTest(const FString&)
{
	const FReEchoCardDefinition Fusion = MakeCard(TEXT("FUSION"),
	                                              3,
	                                              TEXT("Card.ProximityDamage"),
	                                              TEXT("OnCompileRules"),
	                                              TEXT("Damage"),
	                                              1.0f,
	                                              TEXT("ZeroBonusDistanceCm"),
	                                              3000.0f,
	                                              true);
	const FReEchoCardDefinition BothHands = MakeCard(TEXT("BOTH_HANDS"),
	                                                 3,
	                                                 TEXT("Card.AlternatingSources"),
	                                                 TEXT("OnCompileRules"),
	                                                 TEXT("Damage"),
	                                                 1.0f,
	                                                 NAME_None,
	                                                 0.0f,
	                                                 true);
	const FReEchoCardDefinition Distance = MakeCard(TEXT("DISTANCE"),
	                                                3,
	                                                TEXT("Card.DistanceDamage"),
	                                                TEXT("BeforeOutgoingHit"),
	                                                TEXT("Damage"),
	                                                0.04f,
	                                                TEXT("DistanceCm"),
	                                                200.0f,
	                                                true);
	const FReEchoCardCatalog Catalog = BuildCatalog({Fusion, BothHands, Distance});
	FReEchoCardBuildState State;
	State.DomainRevision = Catalog.GetDomainRevision();
	State.OwnedCardIds = {Fusion.Id, BothHands.Id};

	FReEchoCardOutgoingHitInput Hit;
	Hit.RawDamage = 10.0f;
	Hit.bHasLivingEcho = true;
	Hit.NearestEchoDistanceCm = 0.0f;
	Hit.DamageSource = EReEchoDamageSource::Player;
	TestEqual(TEXT("Fusion grants one hundred percent damage at zero metres"),
	          ReEchoCardRuntime::ModifyOutgoingHit(Catalog, State, Hit).RawDamage,
	          20.0f);
	Hit.NearestEchoDistanceCm = 3000.0f;
	TestEqual(TEXT("Fusion grants no damage at thirty metres"),
	          ReEchoCardRuntime::ModifyOutgoingHit(Catalog, State, Hit).RawDamage,
	          10.0f);
	Hit.bPreviousPlayerEchoSourceKnown = true;
	Hit.PreviousPlayerEchoSource = EReEchoDamageSource::Echo;
	TestEqual(TEXT("A previous Echo hit doubles the next Player hit"),
	          ReEchoCardRuntime::ModifyOutgoingHit(Catalog, State, Hit).RawDamage,
	          20.0f);
	Hit.PreviousPlayerEchoSource = EReEchoDamageSource::Player;
	TestEqual(TEXT("Repeating the same source does not double damage"),
	          ReEchoCardRuntime::ModifyOutgoingHit(Catalog, State, Hit).RawDamage,
	          10.0f);
	State.OwnedCardIds.Add(Distance.Id);
	Hit.DistanceCm = 399.0f;
	TestEqual(TEXT("Time-distance resonance grants four percent for one complete two-metre step"),
	          ReEchoCardRuntime::ModifyOutgoingHit(Catalog, State, Hit).RawDamage,
	          10.4f);
	Hit.DistanceCm = 400.0f;
	TestEqual(TEXT("Time-distance resonance grants eight percent at two complete steps"),
	          ReEchoCardRuntime::ModifyOutgoingHit(Catalog, State, Hit).RawDamage,
	          10.8f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoTrinityCardRulesTest,
                                 "ReEcho.Cards.Runtime.EchoTrinity",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoTrinityCardRulesTest::RunTest(const FString&)
{
	const FReEchoCardDefinition Head = MakeCard(TEXT("HEAD"),
	                                            2,
	                                            TEXT("Card.EchoTrinityHead"),
	                                            TEXT("OnCompileRules"),
	                                            TEXT("Status"),
	                                            1.0f,
	                                            NAME_None,
	                                            0.0f,
	                                            true);
	const FReEchoCardDefinition Body = MakeCard(TEXT("BODY"),
	                                            2,
	                                            TEXT("Card.EchoTrinityBody"),
	                                            TEXT("OnCompileRules"),
	                                            TEXT("Status"),
	                                            2.0f,
	                                            NAME_None,
	                                            0.0f,
	                                            true);
	const FReEchoCardDefinition Legs = MakeCard(TEXT("LEGS"),
	                                            2,
	                                            TEXT("Card.EchoTrinityLegs"),
	                                            TEXT("OnCompileRules"),
	                                            TEXT("EchoEfficiency"),
	                                            0.3f,
	                                            TEXT("CompleteBonus"),
	                                            1.0f,
	                                            true);
	const FReEchoCardCatalog Catalog = BuildCatalog({Head, Body, Legs});
	FReEchoCardGrantInput Input;
	Input.CardState.DomainRevision = Catalog.GetDomainRevision();
	Input.Stats.EchoEfficiency = 1.0f;
	FReEchoCardGrantResult Grant = ReEchoCardRuntime::TryGrantCard(Catalog, Legs.Id, Input);
	TestEqual(TEXT("Legs alone grants thirty percent echo efficiency"), Grant.Stats.EchoEfficiency, 1.3f);
	Input.CardState = Grant.CardState;
	Input.Stats = Grant.Stats;
	Grant = ReEchoCardRuntime::TryGrantCard(Catalog, Head.Id, Input);
	Input.CardState = Grant.CardState;
	Input.Stats = Grant.Stats;
	Grant = ReEchoCardRuntime::TryGrantCard(Catalog, Body.Id, Input);
	TestTrue(TEXT("Owning all three pieces compiles the complete set"),
	         ReEchoCardRuntime::CompileRules(Catalog, Grant.CardState).bEchoTrinityComplete);
	TestEqual(TEXT("Completing the set upgrades the materialized bonus to one hundred percent"),
	          Grant.Stats.EchoEfficiency,
	          2.0f);

	FReEchoCardBuildState EncounterState = ReEchoCardRuntime::BeginEncounter(Grant.CardState, 1);
	const FReEchoCardEncounterTickResult Tick = ReEchoCardRuntime::AdvanceEncounter(Catalog, EncounterState, 1.6f);
	TestEqual(TEXT("Complete head pulses every half second"), Tick.EchoHeadCursePulseCount, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEasterCardRuntimeTest,
                                 "ReEcho.Cards.Easter.RuntimeAndOfferContracts",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEasterCardRuntimeTest::RunTest(const FString&)
{
	FReEchoCardDefinition Swing = MakeCard(TEXT("G_4_1"),
	                                       0,
	                                       TEXT("Card.EasterShardSwing"),
	                                       TEXT("OnEncounterEnd"),
	                                       TEXT("TimeShards"),
	                                       2.5f,
	                                       TEXT("PositiveMultiplier"),
	                                       2.5f,
	                                       true);
	Swing.OfferGroup = TEXT("EasterEgg");
	FReEchoCardEffectDefinition Negative = Swing.Effects[0];
	Negative.Id = TEXT("G_4_1_NEG");
	Negative.Order = 2;
	Negative.Value = 0.5f;
	Negative.ParamName = TEXT("NegativeMultiplier");
	Negative.ParamValue = 0.5f;
	Swing.Effects.Add(Negative);
	FReEchoCardEffectDefinition Bonus = Swing.Effects[0];
	Bonus.Id = TEXT("G_4_1_BONUS");
	Bonus.Order = 3;
	Bonus.Value = 30.0f;
	Bonus.ParamName = TEXT("Bonus");
	Bonus.ParamValue = 30.0f;
	Swing.Effects.Add(Bonus);

	FReEchoCardDefinition Attendance = MakeCard(TEXT("G_4_9"),
	                                            0,
	                                            TEXT("Card.EasterAttendance"),
	                                            TEXT("OnEncounterEnd"),
	                                            TEXT("HpMaxAndPoint"),
	                                            10.0f,
	                                            NAME_None,
	                                            0.0f,
	                                            true);
	Attendance.OfferGroup = TEXT("EasterEgg");
	for (const TPair<FName, float>& Stat :
	     {TPair<FName, float>(TEXT("ElementalAttack"), 1.0f), TPair<FName, float>(TEXT("PhysicalAttack"), 1.0f)})
	{
		FReEchoCardEffectDefinition Effect = Attendance.Effects[0];
		Effect.Id = FName(*(FString(TEXT("G_4_9_")) + Stat.Key.ToString()));
		Effect.Order = Attendance.Effects.Num() + 1;
		Effect.Target = Stat.Key;
		Effect.Value = Stat.Value;
		Attendance.Effects.Add(Effect);
	}

	const FReEchoCardCatalog Catalog = BuildCatalog({Swing, Attendance});
	FReEchoCardBuildState State;
	State.DomainRevision = Catalog.GetDomainRevision();
	State.OwnedCardIds = {Swing.Id, Attendance.Id};
	// Egao Party: easter-egg cards stack without limit, so an owned copy stays offerable.
	TestTrue(TEXT("Owned tier-zero Easter cards return to their pool so they can stack"),
	         ReEchoCardRuntime::CanOffer(Catalog, State, Swing));
	FReEchoStatBlock Stats;
	Stats.HpMax = 100.0f;
	Stats.HpPoint = 50.0f;
	Stats.PhysicalAttack = 5.0f;
	Stats.ElementalAttack = 7.0f;
	const FReEchoCardEventResult End = ReEchoCardRuntime::EndEncounter(Catalog, State, Stats, 1, 0, 101, 12345, 0);
	TestTrue(TEXT("Shard swing floors after the chosen multiplier and then adds thirty"),
	         End.ProjectedTimeShards == 282 || End.ProjectedTimeShards == 80);
	TestEqual(TEXT("Attendance grants ten maximum health"), End.Stats.HpMax, 110.0f);
	TestEqual(TEXT("Attendance grants ten current health"), End.Stats.HpPoint, 60.0f);
	TestEqual(TEXT("Attendance grants one physical attack"), End.Stats.PhysicalAttack, 6.0f);
	TestEqual(TEXT("Attendance grants one elemental attack"), End.Stats.ElementalAttack, 8.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoExpectedOutcomeEasterCardTest,
                                 "ReEcho.Cards.Easter.ExpectedOutcomeAppliesLiteralAndActualAttackBonuses",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoExpectedOutcomeEasterCardTest::RunTest(const FString&)
{
	FReEchoCardDefinition Expected = MakeCard(TEXT("G_4_10"),
	                                          0,
	                                          TEXT("Card.ExpectedOutcome"),
	                                          TEXT("OnGrant"),
	                                          TEXT("PhysicalAndElementalAttack"),
	                                          5.0f,
	                                          NAME_None,
	                                          0.0f,
	                                          true);
	Expected.OfferGroup = TEXT("EasterEgg");
	FReEchoCardEffectDefinition EnemyAttack = Expected.Effects[0];
	EnemyAttack.Id = TEXT("G_4_10_ENEMY_ATTACK");
	EnemyAttack.Order = 2;
	EnemyAttack.Trigger = TEXT("OnCompileRules");
	EnemyAttack.Target = TEXT("EnemyAttackFlatBonus");
	Expected.Effects.Add(EnemyAttack);
	const FReEchoCardCatalog Catalog = BuildCatalog({Expected});

	FReEchoCardGrantInput Input;
	Input.CardState.DomainRevision = Catalog.GetDomainRevision();
	Input.Stats.PhysicalAttack = 4.0f;
	Input.Stats.ElementalAttack = 7.0f;
	const FReEchoCardGrantResult Grant = ReEchoCardRuntime::TryGrantCard(Catalog, Expected.Id, Input);

	TestTrue(TEXT("The expected-outcome Easter card grants atomically"), Grant.bSucceeded);
	TestEqual(TEXT("The literal card effect permanently adds five physical attack"), Grant.Stats.PhysicalAttack, 9.0f);
	TestEqual(TEXT("The literal card effect also adds five elemental attack"), Grant.Stats.ElementalAttack, 12.0f);
	TestEqual(TEXT("The hidden interpretation adds five to every enemy direct attack"),
	          ReEchoCardRuntime::CompileRules(Catalog, Grant.CardState).EnemyAttackFlatBonus,
	          5.0f);
	// Egao Party: easter-egg cards repeat, so owning one keeps it offerable.
	TestTrue(TEXT("The owned Easter card stays offerable so it can be taken again"),
	         ReEchoCardRuntime::CanOffer(Catalog, Grant.CardState, Expected));

	const FReEchoCardOutcomeState* Outcome = Grant.CardState.Runtime.ResolvedOutcomes.FindByPredicate(
	    [](const FReEchoCardOutcomeState& Candidate)
	    {
		    return Candidate.CardId == TEXT("G_4_10") && Candidate.Kind == EReEchoCardOutcomeKind::RandomDetails;
	    });
	TestTrue(TEXT("The tooltip records the complete actual interpretation"), Outcome != nullptr);
	TestEqual(
	    TEXT("The tooltip exposes all three affected attack domains"), Outcome ? Outcome->DetailTargets.Num() : 0, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEasterDamageCardRewardTest,
                                 "ReEcho.Cards.Easter.DamageThresholdGrantsOnlyNormalUnownedCards",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEasterDamageCardRewardTest::RunTest(const FString&)
{
	FReEchoCardDefinition Easter = MakeCard(TEXT("G_4_6"),
	                                        0,
	                                        TEXT("Card.EasterDamageCards"),
	                                        TEXT("OnEncounterEnd"),
	                                        TEXT("MinimumGuaranteedTier"),
	                                        1.0f,
	                                        TEXT("GrantTier"),
	                                        1.0f,
	                                        true);
	Easter.OfferGroup = TEXT("EasterEgg");
	TArray<FReEchoCardDefinition> Cards = {Easter};
	for (int32 Index = 1; Index <= 6; ++Index)
	{
		Cards.Add(MakeCard(FName(*FString::Printf(TEXT("NORMAL_%d"), Index)),
		                   Index <= 2 ? 1 : 2,
		                   TEXT("Card.StatModifier"),
		                   TEXT("OnGrant"),
		                   TEXT("PhysicalAttack"),
		                   1.0f,
		                   NAME_None,
		                   0.0f,
		                   true));
	}
	const FReEchoCardCatalog Catalog = BuildCatalog(Cards);
	FReEchoCardBuildState State;
	State.DomainRevision = Catalog.GetDomainRevision();
	State.OwnedCardIds = {Easter.Id, Cards[1].Id};

	// Damage taken is only recorded while the encounter runs; nothing is granted the moment it lands.
	const FReEchoCardGrantResult Recorded =
	    ReEchoCardRuntime::OnPlayerDamageReceived(Catalog, State, FReEchoStatBlock{}, 0, 55.0f, 1, 99);
	TestEqual(TEXT("Damage taken grants nothing until the encounter ends"), Recorded.GrantedCardIds.Num(), 0);
	TestEqual(TEXT("The encounter records the damage taken"), Recorded.CardState.Runtime.EasterDamageTaken, 55.0f);

	// Ending the encounter converts the recorded damage into tier-1 cards.
	const FReEchoCardEventResult Settled =
	    ReEchoCardRuntime::EndEncounter(Catalog, Recorded.CardState, FReEchoStatBlock{}, 1, 0, 0, 99);
	const FReEchoCardOutcomeState* Reward = Settled.CardState.Runtime.ResolvedOutcomes.FindByPredicate(
	    [](const FReEchoCardOutcomeState& Outcome)
	    {
		    return Outcome.CardId == TEXT("G_4_6") && Outcome.Kind == EReEchoCardOutcomeKind::GrantedCards;
	    });
	TestTrue(TEXT("Ending the encounter records a card reward"), Reward != nullptr);
	TestTrue(TEXT("The reward grants at least one card"), Reward && Reward->RelatedCardIds.Num() >= 1);
	TestFalse(TEXT("The Easter card never grants itself"), Reward && Reward->RelatedCardIds.Contains(Easter.Id));
	// The new contract allows any enabled tier-1 card, including an already-owned stackable card;
	// tier-2 cards and the Easter card itself remain impossible because the pool is tier-one only.
	TestFalse(TEXT("Tier-2 normals are never granted"), Reward && Reward->RelatedCardIds.Contains(Cards[3].Id));
	TestFalse(TEXT("The Easter card never appears in its own reward"), Reward && Reward->RelatedCardIds.Contains(Easter.Id));
	TestEqual(TEXT("The one-shot payout is marked complete"), Settled.CardState.Runtime.bEasterDamageCardsGranted, true);
	TestEqual(TEXT("The recorded damage is cleared after settling"), Settled.CardState.Runtime.EasterDamageTaken, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEasterOfferSelectionTest,
                                 "ReEcho.Cards.Easter.IndependentOfferSelection",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEasterOfferSelectionTest::RunTest(const FString&)
{
	FReEchoCardDefinition Normal =
	    MakeCard(TEXT("NORMAL"), 2, TEXT("Card.StatModifier"), TEXT("OnGrant"), TEXT("PhysicalAttack"), 1.0f);
	FReEchoCardDefinition EasterA = MakeCard(TEXT("EASTER_A"),
	                                         0,
	                                         TEXT("Card.EasterAttendance"),
	                                         TEXT("OnEncounterEnd"),
	                                         TEXT("PhysicalAttack"),
	                                         1.0f,
	                                         NAME_None,
	                                         0.0f,
	                                         true);
	FReEchoCardDefinition EasterB = EasterA;
	EasterA.OfferGroup = TEXT("EasterEgg");
	EasterB.Id = TEXT("EASTER_B");
	EasterB.Effects[0].Id = TEXT("EASTER_B_E");
	EasterB.OfferGroup = TEXT("EasterEgg");
	const FReEchoCardCatalog Catalog = BuildCatalog({Normal, EasterA, EasterB});
	FReEchoCardBuildState State;
	State.DomainRevision = Catalog.GetDomainRevision();

	const FName ForcedNormal = ReEchoCardRuntime::SelectOfferForSlot(Catalog, State, 2, 1, {}, 11, 0.0f);
	TestEqual(TEXT("A missed Easter roll uses the requested normal tier"), ForcedNormal, Normal.Id);
	const FName ForcedEaster = ReEchoCardRuntime::SelectOfferForSlot(Catalog, State, 2, 1, {}, 11, 1.0f);
	TestTrue(TEXT("A hit uses the independent Easter pool"), ForcedEaster == EasterA.Id || ForcedEaster == EasterB.Id);
	const FName OtherEaster = ReEchoCardRuntime::SelectOfferForSlot(Catalog, State, 2, 1, {ForcedEaster}, 11, 1.0f);
	TestTrue(TEXT("A group history prevents an Easter duplicate"),
	         OtherEaster != ForcedEaster && OtherEaster != Normal.Id);
	const FName ExhaustedEaster =
	    ReEchoCardRuntime::SelectOfferForSlot(Catalog, State, 2, 1, {EasterA.Id, EasterB.Id}, 11, 1.0f);
	TestEqual(TEXT("An exhausted Easter pool falls back to the normal tier"), ExhaustedEaster, Normal.Id);
	const FName ExhaustedNormal = ReEchoCardRuntime::SelectOfferForSlot(Catalog, State, 2, 1, {Normal.Id}, 11, 0.0f);
	TestTrue(TEXT("Normal exhaustion never promotes Easter into a guaranteed offer"), ExhaustedNormal.IsNone());

	int32 EasterHits = 0;
	for (int32 Seed = 1; Seed <= 4096; ++Seed)
	{
		const FName Selected = ReEchoCardRuntime::SelectOfferForSlot(Catalog, State, 2, 1, {}, Seed, 0.01f);
		EasterHits += Selected == EasterA.Id || Selected == EasterB.Id ? 1 : 0;
	}
	TestTrue(TEXT("One-percent production probability produces both hit and miss outcomes"),
	         EasterHits > 0 && EasterHits < 4096);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEasterGrantRandomnessTest,
                                 "ReEcho.Cards.Easter.IndependentGrantAndSacrifice",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEasterGrantRandomnessTest::RunTest(const FString&)
{
	FReEchoCardDefinition Independent = MakeCard(TEXT("G_4_2"),
	                                             0,
	                                             TEXT("Card.EasterIndependentGrant"),
	                                             TEXT("OnGrant"),
	                                             TEXT("CriticalRate"),
	                                             0.3f,
	                                             TEXT("Chance"),
	                                             0.5f,
	                                             true);
	Independent.OfferGroup = TEXT("EasterEgg");
	FReEchoCardEffectDefinition Physical = Independent.Effects[0];
	Physical.Id = TEXT("G_4_2_PHYSICAL");
	Physical.Order = 2;
	Physical.Target = TEXT("PhysicalAttack");
	Physical.Value = 15.0f;
	Independent.Effects.Add(Physical);
	FReEchoCardEffectDefinition Health = Independent.Effects[0];
	Health.Id = TEXT("G_4_2_HP");
	Health.Order = 3;
	Health.Target = TEXT("HpPoint");
	Health.Operation = EReEchoCardValueOperation::Override;
	Health.Value = 3.0f;
	Independent.Effects.Add(Health);

	FReEchoCardDefinition Sacrifice = MakeCard(TEXT("G_4_4"),
	                                           0,
	                                           TEXT("Card.EasterShardSacrifice"),
	                                           TEXT("OnGrant"),
	                                           TEXT("HpMaxAndPoint"),
	                                           1.0f,
	                                           TEXT("ShardUnit"),
	                                           15.0f,
	                                           true);
	Sacrifice.OfferGroup = TEXT("EasterEgg");
	for (const TPair<FName, float>& Reward : {TPair<FName, float>(TEXT("PhysicalAttack"), 1.0f),
	                                          TPair<FName, float>(TEXT("ElementalAttack"), 1.0f),
	                                          TPair<FName, float>(TEXT("CriticalRate"), 0.05f),
	                                          TPair<FName, float>(TEXT("CriticalEffect"), 0.1f),
	                                          TPair<FName, float>(TEXT("EchoEfficiency"), 0.05f),
	                                          TPair<FName, float>(TEXT("ReactionEfficiency"), 0.05f)})
	{
		FReEchoCardEffectDefinition Effect = Sacrifice.Effects[0];
		Effect.Id = FName(*(FString(TEXT("G_4_4_")) + Reward.Key.ToString()));
		Effect.Order = Sacrifice.Effects.Num() + 1;
		Effect.Target = Reward.Key;
		Effect.Value = Reward.Value;
		Sacrifice.Effects.Add(Effect);
	}
	const FReEchoCardCatalog Catalog = BuildCatalog({Independent, Sacrifice});
	bool bSawNone = false;
	bool bSawAll = false;
	for (int32 Seed = 1; Seed <= 512; ++Seed)
	{
		FReEchoCardGrantInput Input;
		Input.CardState.DomainRevision = Catalog.GetDomainRevision();
		Input.Stats.HpMax = 100.0f;
		Input.Stats.HpPoint = 50.0f;
		Input.RandomSeed = Seed;
		const FReEchoCardGrantResult Grant = ReEchoCardRuntime::TryGrantCard(Catalog, Independent.Id, Input);
		const int32 HitCount = (Grant.Stats.CriticalRate > Input.Stats.CriticalRate ? 1 : 0) +
		                       (Grant.Stats.PhysicalAttack > Input.Stats.PhysicalAttack ? 1 : 0) +
		                       (Grant.Stats.HpPoint == 3.0f ? 1 : 0);
		bSawNone |= HitCount == 0;
		bSawAll |= HitCount == 3;
	}
	TestTrue(TEXT("Three independent fifty-percent rolls can all miss"), bSawNone);
	TestTrue(TEXT("Three independent fifty-percent rolls can all hit"), bSawAll);

	FReEchoCardGrantInput SacrificeInput;
	SacrificeInput.CardState.DomainRevision = Catalog.GetDomainRevision();
	SacrificeInput.Stats.HpMax = 100.0f;
	SacrificeInput.Stats.HpPoint = 50.0f;
	SacrificeInput.TimeShards = 31;
	SacrificeInput.RandomSeed = 2468;
	const FReEchoCardGrantResult SacrificeGrant =
	    ReEchoCardRuntime::TryGrantCard(Catalog, Sacrifice.Id, SacrificeInput);
	TestTrue(TEXT("The shard sacrifice commits atomically"), SacrificeGrant.bSucceeded);
	TestEqual(TEXT("The shard sacrifice removes the entire balance including remainder"), SacrificeGrant.TimeShards, 0);
	const FReEchoCardOutcomeState* SacrificeOutcome = SacrificeGrant.CardState.Runtime.ResolvedOutcomes.FindByPredicate(
	    [](const FReEchoCardOutcomeState& Outcome)
	    {
		    return Outcome.CardId == TEXT("G_4_4") && Outcome.Kind == EReEchoCardOutcomeKind::RandomDetails;
	    });
	TestTrue(TEXT("The shard sacrifice records concrete rewards"), SacrificeOutcome != nullptr);
	if (SacrificeOutcome)
	{
		TestEqual(TEXT("Thirty-one shards produce two reward draws"), SacrificeOutcome->ResolutionCount, 2);
		TestEqual(TEXT("Recorded targets and values stay aligned"),
		          SacrificeOutcome->DetailTargets.Num(),
		          SacrificeOutcome->DetailValues.Num());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEasterEncounterRulesTest,
                                 "ReEcho.Cards.Easter.EncounterRulesAndShardComparison",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEasterEncounterRulesTest::RunTest(const FString&)
{
	FReEchoCardDefinition Contact = MakeCard(TEXT("G_4_3"),
	                                         0,
	                                         TEXT("Card.EasterEchoContact"),
	                                         TEXT("OnCompileRules"),
	                                         TEXT("Damage"),
	                                         12.0f,
	                                         TEXT("PlayerHealing"),
	                                         6.0f,
	                                         true);
	Contact.OfferGroup = TEXT("EasterEgg");
	FReEchoCardDefinition Stun = MakeCard(TEXT("G_4_5"),
	                                      0,
	                                      TEXT("Card.EasterRandomStun"),
	                                      TEXT("OnEncounterTick"),
	                                      TEXT("Stun"),
	                                      1.0f,
	                                      TEXT("PulseInterval"),
	                                      0.5f,
	                                      true);
	Stun.OfferGroup = TEXT("EasterEgg");
	FReEchoCardEffectDefinition Radius = Stun.Effects[0];
	Radius.Id = TEXT("G_4_5_RADIUS");
	Radius.Order = 2;
	Radius.Target = TEXT("Damage");
	Radius.Value = 400.0f;
	Radius.ParamName = TEXT("RadiusCm");
	Radius.ParamValue = 400.0f;
	Stun.Effects.Add(Radius);
	FReEchoCardDefinition Compare = MakeCard(TEXT("G_4_8"),
	                                         0,
	                                         TEXT("Card.EasterShardComparison"),
	                                         TEXT("OnEncounterEnd"),
	                                         TEXT("TimeShards"),
	                                         -0.01f,
	                                         TEXT("Step"),
	                                         5.0f,
	                                         true);
	Compare.OfferGroup = TEXT("EasterEgg");
	FReEchoCardEffectDefinition Less = Compare.Effects[0];
	Less.Id = TEXT("G_4_8_LESS");
	Less.Order = 2;
	Less.Value = 0.05f;
	Compare.Effects.Add(Less);
	const FReEchoCardCatalog Catalog = BuildCatalog({Contact, Stun, Compare});
	FReEchoCardBuildState State;
	State.DomainRevision = Catalog.GetDomainRevision();
	State.OwnedCardIds = {Contact.Id, Stun.Id, Compare.Id};
	const FReEchoCardRuleSnapshot Rules = ReEchoCardRuntime::CompileRules(Catalog, State);
	TestTrue(TEXT("Echo contact rules compile"), Rules.bEasterEchoContact);
	TestEqual(TEXT("Echo contact damage comes from data"), Rules.EasterEchoContactDamage, 12.0f);
	TestEqual(TEXT("Echo contact healing comes from data"), Rules.EasterEchoContactHealing, 6.0f);
	TestTrue(TEXT("Random stun rules compile"), Rules.bEasterRandomStun);
	TestEqual(TEXT("Random stun radius is four metres"), Rules.EasterRandomStunRadiusCm, 400.0f);

	State = ReEchoCardRuntime::BeginEncounter(State, 1);
	FReEchoCardEncounterTickResult Tick = ReEchoCardRuntime::AdvanceEncounter(Catalog, State, 0.49f);
	TestEqual(TEXT("No stun pulse occurs before half a second"), Tick.EasterRandomStunPulseCount, 0);
	Tick = ReEchoCardRuntime::AdvanceEncounter(Catalog, Tick.CardState, 0.5f);
	TestEqual(TEXT("One stun pulse occurs at half a second"), Tick.EasterRandomStunPulseCount, 1);
	Tick = ReEchoCardRuntime::AdvanceEncounter(Catalog, Tick.CardState, 1.6f);
	TestEqual(TEXT("Elapsed pulse boundaries are caught up exactly once"), Tick.EasterRandomStunPulseCount, 2);

	FReEchoCardEventResult End = ReEchoCardRuntime::EndEncounter(Catalog, Tick.CardState, {}, 1, 0, 0, 1, 20);
	TestTrue(TEXT("The first completed encounter establishes a comparison baseline"),
	         End.CardState.Runtime.bHasPreviousEncounterShardIncome);
	TestEqual(TEXT("The baseline keeps a neutral next-encounter multiplier"),
	          End.CardState.Runtime.EncounterShardIncomeMultiplier,
	          1.0f);
	End = ReEchoCardRuntime::EndEncounter(Catalog, End.CardState, {}, 2, 0, 0, 2, 30);
	TestEqual(TEXT("Ten more gross shards reduce the next multiplier by two percent"),
	          End.CardState.Runtime.EncounterShardIncomeMultiplier,
	          0.98f);
	End = ReEchoCardRuntime::EndEncounter(Catalog, End.CardState, {}, 3, 0, 0, 3, 20);
	TestEqual(TEXT("Ten fewer gross shards increase the next multiplier by ten percent"),
	          End.CardState.Runtime.EncounterShardIncomeMultiplier,
	          1.1f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEasterPhysicalLotteryTest,
                                 "ReEcho.Cards.Easter.PhysicalDamageLottery",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEasterPhysicalLotteryTest::RunTest(const FString&)
{
	FReEchoCardDefinition Lottery = MakeCard(TEXT("G_4_7"),
	                                         0,
	                                         TEXT("Card.EasterPhysicalLottery"),
	                                         TEXT("BeforeOutgoingHit"),
	                                         TEXT("Damage"),
	                                         0.0f,
	                                         TEXT("Probability"),
	                                         0.15f,
	                                         true);
	Lottery.OfferGroup = TEXT("EasterEgg");
	const TArray<TPair<float, float>> Outcomes = {{10.0f, 0.35f}, {100.0f, 0.40f}, {1000.0f, 0.08f}, {10000.0f, 0.02f}};
	for (const TPair<float, float>& Config : Outcomes)
	{
		FReEchoCardEffectDefinition Effect = Lottery.Effects[0];
		Effect.Id = FName(*FString::Printf(TEXT("G_4_7_%d"), Lottery.Effects.Num() + 1));
		Effect.Order = Lottery.Effects.Num() + 1;
		Effect.Value = Config.Key;
		Effect.ParamValue = Config.Value;
		Lottery.Effects.Add(Effect);
	}
	const FReEchoCardCatalog Catalog = BuildCatalog({Lottery});
	FReEchoCardBuildState State;
	State.DomainRevision = Catalog.GetDomainRevision();
	State.OwnedCardIds = {Lottery.Id};
	TMap<int32, int32> Counts;
	for (int32 Seed = 1; Seed <= 20000; ++Seed)
	{
		FReEchoCardOutgoingHitInput Hit;
		Hit.RawDamage = 37.0f;
		Hit.RandomSeed = Seed;
		Hit.DamageSource = Seed % 2 == 0 ? EReEchoDamageSource::Player : EReEchoDamageSource::Echo;
		Hit.Element = EReEchoElement::None;
		const float Damage = ReEchoCardRuntime::ModifyOutgoingHit(Catalog, State, Hit).RawDamage;
		Counts.FindOrAdd(FMath::RoundToInt(Damage))++;
	}
	const TMap<int32, float> Expected = {{0, 0.15f}, {10, 0.35f}, {100, 0.40f}, {1000, 0.08f}, {10000, 0.02f}};
	for (const TPair<int32, float>& Pair : Expected)
	{
		const float Actual = Counts.FindRef(Pair.Key) / 20000.0f;
		TestTrue(*FString::Printf(TEXT("Damage outcome %d follows its configured weight"), Pair.Key),
		         FMath::Abs(Actual - Pair.Value) < 0.025f);
	}
	FReEchoCardOutgoingHitInput Elemental;
	Elemental.RawDamage = 37.0f;
	Elemental.RandomSeed = 1;
	Elemental.Element = EReEchoElement::Flame;
	TestEqual(TEXT("Elemental damage bypasses the physical lottery"),
	          ReEchoCardRuntime::ModifyOutgoingHit(Catalog, State, Elemental).RawDamage,
	          37.0f);
	return true;
}

#endif
