#include "Cards/ReEchoCardRuntime.h"

namespace
{
bool HasTag(const FReEchoCardDefinition& Card, const FName Tag)
{
	return Card.Tags.Contains(Tag);
}

bool ApplyStatEffect(FReEchoStatBlock& Stats, const FReEchoCardEffectDefinition& Effect)
{
	auto ApplyFloat = [&](float& Target)
	{
		Target = ReEchoCardRuntime::ApplyValueOperation(Target, Effect.Operation, Effect.Value);
	};
	if (Effect.Target == TEXT("HpMax"))
	{
		ApplyFloat(Stats.HpMax);
		Stats.HpMax = FMath::Max(1.0f, Stats.HpMax);
		Stats.HpPoint = FMath::Min(Stats.HpPoint, Stats.HpMax);
		return true;
	}
	if (Effect.Target == TEXT("HpPoint"))
	{
		ApplyFloat(Stats.HpPoint);
		Stats.HpPoint = FMath::Clamp(Stats.HpPoint, 0.0f, Stats.HpMax);
		return true;
	}
	if (Effect.Target == TEXT("PhysicalAttack"))
	{
		ApplyFloat(Stats.PhysicalAttack);
		Stats.PhysicalAttack = FMath::Max(0.0f, Stats.PhysicalAttack);
		return true;
	}
	if (Effect.Target == TEXT("ElementalAttack"))
	{
		ApplyFloat(Stats.ElementalAttack);
		Stats.ElementalAttack = FMath::Max(0.0f, Stats.ElementalAttack);
		return true;
	}
	if (Effect.Target == TEXT("MovementSpeed"))
	{
		ApplyFloat(Stats.MovementSpeed);
		Stats.MovementSpeed = FMath::Max(0.1f, Stats.MovementSpeed);
		return true;
	}
	if (Effect.Target == TEXT("CriticalRate"))
	{
		ApplyFloat(Stats.CriticalRate);
		Stats.CriticalRate = FMath::Max(0.0f, Stats.CriticalRate);
		return true;
	}
	if (Effect.Target == TEXT("CriticalEffect"))
	{
		ApplyFloat(Stats.CriticalEffect);
		Stats.CriticalEffect = FMath::Max(0.0f, Stats.CriticalEffect);
		return true;
	}
	if (Effect.Target == TEXT("EchoEfficiency"))
	{
		ApplyFloat(Stats.EchoEfficiency);
		Stats.EchoEfficiency = FMath::Max(0.0f, Stats.EchoEfficiency);
		return true;
	}
	if (Effect.Target == TEXT("ReactionEfficiency"))
	{
		ApplyFloat(Stats.ReactionEfficiency);
		Stats.ReactionEfficiency = FMath::Max(0.0f, Stats.ReactionEfficiency);
		return true;
	}
	return false;
}

FReEchoCardOutcomeState&
FindOrAddOutcome(FReEchoCardRuntimeState& Runtime, const FName CardId, const EReEchoCardOutcomeKind Kind)
{
	FReEchoCardOutcomeState* Existing = Runtime.ResolvedOutcomes.FindByPredicate(
	    [&](const FReEchoCardOutcomeState& Outcome)
	    {
		    return Outcome.CardId == CardId && Outcome.Kind == Kind;
	    });
	if (!Existing)
	{
		FReEchoCardOutcomeState& Added = Runtime.ResolvedOutcomes.AddDefaulted_GetRef();
		Added.CardId = CardId;
		Added.Kind = Kind;
		Existing = &Added;
	}
	return *Existing;
}

void RemoveOutcomesForCard(FReEchoCardRuntimeState& Runtime, const FName CardId)
{
	Runtime.ResolvedOutcomes.RemoveAll(
	    [&](const FReEchoCardOutcomeState& Outcome)
	    {
		    return Outcome.CardId == CardId;
	    });
}

void SetPendingOutcome(FReEchoCardRuntimeState& Runtime,
                       const FName CardId,
                       const FName Target,
                       const int32 EncounterIndex)
{
	RemoveOutcomesForCard(Runtime, CardId);
	FReEchoCardOutcomeState& Outcome = FindOrAddOutcome(Runtime, CardId, EReEchoCardOutcomeKind::PendingEncounter);
	Outcome.PrimaryTarget = Target;
	Outcome.PrimaryValue = 0.0f;
	Outcome.SecondaryTarget = NAME_None;
	Outcome.SecondaryValue = 0.0f;
	Outcome.ResolutionCount = 0;
	Outcome.EncounterIndex = EncounterIndex;
	Outcome.EconomyPenalty = EReEchoCardEconomyPenalty::None;
	Outcome.RelatedCardIds.Reset();
}

void SetStatGainOutcome(FReEchoCardRuntimeState& Runtime, const FName CardId, const FName Target, const float Value)
{
	RemoveOutcomesForCard(Runtime, CardId);
	FReEchoCardOutcomeState& Outcome = FindOrAddOutcome(Runtime, CardId, EReEchoCardOutcomeKind::StatGain);
	Outcome.PrimaryTarget = Target;
	Outcome.PrimaryValue = Value;
	Outcome.SecondaryTarget = NAME_None;
	Outcome.SecondaryValue = 0.0f;
	Outcome.ResolutionCount = 1;
	Outcome.EncounterIndex = INDEX_NONE;
	Outcome.EconomyPenalty = EReEchoCardEconomyPenalty::None;
	Outcome.RelatedCardIds.Reset();
}

void AccumulateOutcome(FReEchoCardRuntimeState& Runtime,
                       const FName CardId,
                       const FName PrimaryTarget,
                       const float PrimaryValue,
                       const FName SecondaryTarget = NAME_None,
                       const float SecondaryValue = 0.0f)
{
	FReEchoCardOutcomeState* Existing = Runtime.ResolvedOutcomes.FindByPredicate(
	    [&](const FReEchoCardOutcomeState& Outcome)
	    {
		    return Outcome.CardId == CardId && Outcome.Kind == EReEchoCardOutcomeKind::CumulativeStatGain &&
		           Outcome.PrimaryTarget == PrimaryTarget && Outcome.SecondaryTarget == SecondaryTarget;
	    });
	if (!Existing)
	{
		FReEchoCardOutcomeState& Added = Runtime.ResolvedOutcomes.AddDefaulted_GetRef();
		Added.CardId = CardId;
		Added.Kind = EReEchoCardOutcomeKind::CumulativeStatGain;
		Added.PrimaryTarget = PrimaryTarget;
		Added.SecondaryTarget = SecondaryTarget;
		Existing = &Added;
	}
	FReEchoCardOutcomeState& Outcome = *Existing;
	Outcome.PrimaryValue += PrimaryValue;
	Outcome.SecondaryValue += SecondaryValue;
	++Outcome.ResolutionCount;
	Outcome.EncounterIndex = INDEX_NONE;
	Outcome.EconomyPenalty = EReEchoCardEconomyPenalty::None;
}

void ApplyRuleEffect(FReEchoCardRuleSnapshot& Rules, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
{
	const float StackedValue = Effect.Value * FMath::Max(1, StackCount);
	if (Effect.BehaviorId == TEXT("Card.ShopContract") && Effect.Target == TEXT("ShopDiscount"))
	{
		Rules.ShopDiscount = FMath::Clamp(Rules.ShopDiscount + StackedValue, 0.0f, 1.0f);
	}
	else if (Effect.BehaviorId == TEXT("Card.DoubleEcho"))
	{
		Rules.MaximumEchoes = FMath::Max(Rules.MaximumEchoes, FMath::RoundToInt(Effect.Value));
	}
	else if (Effect.BehaviorId == TEXT("Card.SoloBody"))
	{
		Rules.bEchoesDisabled = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.TauntEcho"))
	{
		Rules.bEchoesCanAttack = false;
		Rules.bEchoTaunts = true;
		Rules.EchoHealthMultiplier = FMath::Max(Rules.EchoHealthMultiplier, Effect.Value);
	}
	else if (Effect.BehaviorId == TEXT("Card.EchoElementAura"))
	{
		Rules.bWaterEchoAura |= Effect.Target == TEXT("Water");
		Rules.bGrassEchoAura |= Effect.Target == TEXT("Grass");
	}
	else if (Effect.BehaviorId == TEXT("Card.EchoSlowAura"))
	{
		Rules.EchoSlowAura = FMath::Max(Rules.EchoSlowAura, Effect.Value);
	}
	else if (Effect.BehaviorId == TEXT("Card.EnemyImmunity"))
	{
		Rules.EnemyElementImmunitySeconds = Effect.Value;
	}
	else if (Effect.BehaviorId == TEXT("Card.ElementCanCrit"))
	{
		Rules.bElementDamageCanCrit = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.DoubleCritRoll"))
	{
		Rules.CriticalRollCount = FMath::Max(Rules.CriticalRollCount, FMath::RoundToInt(Effect.Value));
	}
	else if (Effect.BehaviorId == TEXT("Card.ElementAttachedCrit"))
	{
		Rules.ElementAttachedCriticalEffectBonus += StackedValue;
	}
	else if (Effect.BehaviorId == TEXT("Card.DistanceDamage"))
	{
		Rules.DistanceDamageBonusPerStep += StackedValue;
		Rules.DistanceDamageStepCm = FMath::Max(1.0f, Effect.ParamValue);
	}
	else if (Effect.BehaviorId == TEXT("Card.ProximityDamage"))
	{
		Rules.ProximityDamageBonus += StackedValue;
	}
	else if (Effect.BehaviorId == TEXT("Card.CurseBank"))
	{
		Rules.bUnlimitedShopCredit = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.AlternatingSources"))
	{
		Rules.bAlternatingPlayerEchoDamage = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.ConnectionLine"))
	{
		Rules.bConnectionLineDamage = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.WeaponMaster"))
	{
		Rules.bWeaponMaster = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.EchoTrinityHead"))
	{
		Rules.bEchoHead = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.EchoTrinityBody"))
	{
		Rules.bEchoBody = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.EchoTrinityLegs"))
	{
		Rules.bEchoLegs = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.InfiniteStackingBurn"))
	{
		Rules.bInfiniteStackingBurn = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.VaporizeWaterSplash"))
	{
		Rules.bVaporizeWaterSplash = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.ConductDamageGrowth"))
	{
		Rules.bConductDamageGrowth = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.OverhealCapacity"))
	{
		Rules.bOverhealCapacity = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.CriticalElement"))
	{
		Rules.bCriticalOverridesElement = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.DoubleNonCoreSlots"))
	{
		Rules.bDoubleNonCoreSlotCapacity = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.EasterEchoContact"))
	{
		Rules.bEasterEchoContact = true;
		Rules.EasterEchoContactDamage = FMath::Max(Rules.EasterEchoContactDamage, Effect.Value);
		Rules.EasterEchoContactHealing = FMath::Max(Rules.EasterEchoContactHealing, Effect.ParamValue);
	}
	else if (Effect.BehaviorId == TEXT("Card.EasterRandomStun"))
	{
		Rules.bEasterRandomStun = true;
		if (Effect.ParamName == TEXT("PulseInterval"))
		{
			Rules.EasterRandomStunDuration = FMath::Max(Rules.EasterRandomStunDuration, Effect.Value);
			Rules.EasterRandomStunInterval = FMath::Max(0.1f, Effect.ParamValue);
		}
		else if (Effect.ParamName == TEXT("RadiusCm"))
		{
			Rules.EasterRandomStunRadiusCm = FMath::Max(Rules.EasterRandomStunRadiusCm, Effect.ParamValue);
		}
	}
}

void ForEachOwnedEffect(
    const FReEchoCardCatalog& Catalog,
    const FReEchoCardBuildState& State,
    const FName Trigger,
    TFunctionRef<void(const FReEchoCardDefinition&, const FReEchoCardEffectDefinition&, int32)> Visitor)
{
	TSet<FName> Visited;
	for (const FName CardId : State.OwnedCardIds)
	{
		if (Visited.Contains(CardId))
		{
			continue;
		}
		Visited.Add(CardId);
		const FReEchoCardDefinition* Card = Catalog.Find(CardId);
		if (!Card || !Card->bEnabled)
		{
			continue;
		}
		const int32 StackCount = Card->StackPolicy == TEXT("Unique") ? 1 : ReEchoCardRuntime::CountOwned(State, CardId);
		for (const FReEchoCardEffectDefinition& Effect : Card->Effects)
		{
			if (Effect.Trigger == Trigger)
			{
				Visitor(*Card, Effect, StackCount);
			}
		}
	}
}
} // namespace

float ReEchoCardRuntime::ApplyValueOperation(const float CurrentValue,
                                             const EReEchoCardValueOperation Operation,
                                             const float Value)
{
	switch (Operation)
	{
		case EReEchoCardValueOperation::Add:
			return CurrentValue + Value;
		case EReEchoCardValueOperation::Multiply:
			return CurrentValue * Value;
		case EReEchoCardValueOperation::Override:
			return Value;
		default:
			return CurrentValue;
	}
}

int32 ReEchoCardRuntime::CountOwned(const FReEchoCardBuildState& State, const FName CardId)
{
	int32 Count = 0;
	for (const FName OwnedCardId : State.OwnedCardIds)
	{
		Count += OwnedCardId == CardId ? 1 : 0;
	}
	return Count;
}

bool ReEchoCardRuntime::HasCard(const FReEchoCardBuildState& State, const FName CardId)
{
	return State.OwnedCardIds.Contains(CardId);
}

bool ReEchoCardRuntime::CanOffer(const FReEchoCardCatalog& Catalog,
                                 const FReEchoCardBuildState& State,
                                 const FReEchoCardDefinition& Card,
                                 const int32 EncounterIndex)
{
	if (!Card.bEnabled || !Card.bOfferable)
	{
		return false;
	}
	// Tier-one cards are the repeatable growth pool. Owned tier-two/three cards are one-time acquisitions and
	// must never return through either the free-draw or shop offer paths.
	if (HasCard(State, Card.Id) && (Card.Tier != 1 || Card.StackPolicy == TEXT("Unique")))
	{
		return false;
	}
	const FReEchoCardDefinition* SoloBody = Catalog.Find(TEXT("G_3_03"));
	if (HasCard(State, TEXT("G_3_03")) && SoloBody && HasTag(Card, TEXT("Echo")))
	{
		return false;
	}
	bool bHasEncounterRestriction = false;
	bool bEncounterAllowed = false;
	for (const FName Tag : Card.Tags)
	{
		const FString TagText = Tag.ToString();
		if (!TagText.StartsWith(TEXT("OfferEncounter")))
		{
			continue;
		}
		bHasEncounterRestriction = true;
		bEncounterAllowed |= EncounterIndex == FCString::Atoi(*TagText.RightChop(14));
	}
	if (bHasEncounterRestriction && EncounterIndex != INDEX_NONE && !bEncounterAllowed)
	{
		return false;
	}
	for (const FName OwnedCardId : State.OwnedCardIds)
	{
		const FReEchoCardDefinition* OwnedCard = Catalog.Find(OwnedCardId);
		if (!OwnedCard)
		{
			continue;
		}
		if (Card.ConflictPolicy != NAME_None && Card.ConflictPolicy != TEXT("None") &&
		    Card.ConflictPolicy == OwnedCard->ConflictPolicy)
		{
			return false;
		}
		if (Card.Id == TEXT("G_3_03") && HasTag(*OwnedCard, TEXT("Echo")))
		{
			return false;
		}
	}
	return true;
}

TArray<FReEchoCardDefinition> ReEchoCardRuntime::BuildOfferPool(const FReEchoCardCatalog& Catalog,
                                                                const FReEchoCardBuildState& State,
                                                                const FName OfferGroup,
                                                                const int32 Tier,
                                                                const int32 EncounterIndex)
{
	TArray<FReEchoCardDefinition> Result;
	for (const FReEchoCardDefinition& Card : Catalog.GetOfferable(OfferGroup, Tier))
	{
		if (CanOffer(Catalog, State, Card, EncounterIndex))
		{
			Result.Add(Card);
		}
	}
	return Result;
}

FReEchoCardRuleSnapshot ReEchoCardRuntime::CompileRules(const FReEchoCardCatalog& Catalog,
                                                        const FReEchoCardBuildState& State)
{
	FReEchoCardRuleSnapshot Rules;
	TSet<FName> AppliedUniqueCards;
	for (const FName CardId : State.OwnedCardIds)
	{
		const FReEchoCardDefinition* Card = Catalog.Find(CardId);
		if (!Card || !Card->bEnabled || AppliedUniqueCards.Contains(CardId))
		{
			continue;
		}
		AppliedUniqueCards.Add(CardId);
		const int32 StackCount = Card->StackPolicy == TEXT("Unique") ? 1 : CountOwned(State, CardId);
		for (const FReEchoCardEffectDefinition& Effect : Card->Effects)
		{
			ApplyRuleEffect(Rules, Effect, StackCount);
		}
	}
	Rules.bDisableShopRefresh = State.Runtime.EconomyPenalty == EReEchoCardEconomyPenalty::NoShopRefresh;
	Rules.bDisableExtraCardPurchase = State.Runtime.EconomyPenalty == EReEchoCardEconomyPenalty::NoExtraCardPurchase;
	Rules.bDisableEnemyShardDrops = State.Runtime.EconomyPenalty == EReEchoCardEconomyPenalty::NoEnemyShardDrops;
	Rules.bEchoTrinityComplete = Rules.bEchoHead && Rules.bEchoBody && Rules.bEchoLegs;
	return Rules;
}

FReEchoCardGrantResult ReEchoCardRuntime::TryGrantCard(const FReEchoCardCatalog& Catalog,
                                                       const FName CardId,
                                                       const FReEchoCardGrantInput& Input)
{
	FReEchoCardGrantResult Result;
	Result.Stats = Input.Stats;
	Result.CardState = Input.CardState;
	Result.TimeShards = Input.TimeShards;
	if (Result.CardState.DomainRevision.IsEmpty())
	{
		Result.CardState.DomainRevision = Catalog.GetDomainRevision();
	}
	if (Result.CardState.DomainRevision != Catalog.GetDomainRevision())
	{
		Result.Error = TEXT("Card domain revision mismatch");
		return Result;
	}

	TSet<FName> GrantedThisTransaction;
	TFunction<bool(FName, bool, bool)> GrantSingle;
	GrantSingle = [&](const FName RequestedCardId, const bool bRecordOwnership, const bool bForce)
	{
		const FReEchoCardDefinition* Card = Catalog.Find(RequestedCardId);
		if (!Card || !Card->bEnabled ||
		    (bRecordOwnership && !bForce && !CanOffer(Catalog, Result.CardState, *Card, Input.EncounterIndex)) ||
		    GrantedThisTransaction.Contains(RequestedCardId))
		{
			Result.Error = FString::Printf(TEXT("Card cannot be granted: %s"), *RequestedCardId.ToString());
			return false;
		}
		GrantedThisTransaction.Add(RequestedCardId);
		if (bRecordOwnership)
		{
			Result.CardState.OwnedCardIds.Add(RequestedCardId);
		}
		Result.GrantedCardIds.Add(RequestedCardId);
		TOptional<FRandomStream> EasterGrantRandom;
		auto GetEasterGrantRandom = [&]() -> FRandomStream&
		{
			if (!EasterGrantRandom.IsSet())
			{
				uint32 Seed = HashCombine(GetTypeHash(Input.RandomSeed), GetTypeHash(Card->Id));
				Seed = HashCombine(Seed, GetTypeHash(Result.CardState.Runtime.RandomSequence++));
				EasterGrantRandom.Emplace(static_cast<int32>(Seed));
			}
			return EasterGrantRandom.GetValue();
		};

		for (const FReEchoCardEffectDefinition& Effect : Card->Effects)
		{
			if (Effect.Trigger != TEXT("OnGrant") && Effect.Trigger != TEXT("OnApply"))
			{
				continue;
			}
			if (Effect.BehaviorId == TEXT("Card.StatModifier") || Effect.BehaviorId == TEXT("Card.InstantRecovery"))
			{
				if (!ApplyStatEffect(Result.Stats, Effect))
				{
					Result.Error = FString::Printf(TEXT("Unsupported stat target: %s"), *Effect.Target.ToString());
					return false;
				}
			}
			else if (Effect.BehaviorId == TEXT("Card.EasterIndependentGrant"))
			{
				FReEchoCardOutcomeState& Outcome =
				    FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::RandomDetails);
				if (Effect.Order == 1)
				{
					Outcome.DetailTargets.Reset();
					Outcome.DetailValues.Reset();
					Outcome.ResolutionCount = 1;
				}
				if (GetEasterGrantRandom().FRand() < FMath::Clamp(Effect.ParamValue, 0.0f, 1.0f))
				{
					if (!ApplyStatEffect(Result.Stats, Effect))
					{
						Result.Error = FString::Printf(TEXT("Unsupported Easter stat target: %s"), *Effect.Target.ToString());
						return false;
					}
					Outcome.DetailTargets.Add(Effect.Target);
					Outcome.DetailValues.Add(Effect.Value);
					if (Effect.Target == TEXT("HpPoint"))
					{
						Result.HealthAdjustment = EReEchoHealthAdjustment::SetToStatPoint;
					}
				}
			}
			else if (Effect.BehaviorId == TEXT("Card.EasterShardSacrifice") && Effect.Order == 1)
			{
				FReEchoCardOutcomeState& Outcome =
				    FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::RandomDetails);
				Outcome.DetailTargets.Reset();
				Outcome.DetailValues.Reset();
				const int32 ShardUnit = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
				const int32 RewardCount = FMath::Max(0, Result.TimeShards / ShardUnit);
				Result.TimeShards = 0;
				for (int32 RewardIndex = 0; RewardIndex < RewardCount; ++RewardIndex)
				{
					const FReEchoCardEffectDefinition& Reward =
					    Card->Effects[GetEasterGrantRandom().RandRange(0, Card->Effects.Num() - 1)];
					if (Reward.Target == TEXT("HpMaxAndPoint"))
					{
						Result.Stats.HpMax += Reward.Value;
						Result.Stats.HpPoint += Reward.Value;
						Result.HealthAdjustment = EReEchoHealthAdjustment::SetToStatPoint;
					}
					else if (!ApplyStatEffect(Result.Stats, Reward))
					{
						Result.Error = FString::Printf(TEXT("Unsupported Easter sacrifice target: %s"), *Reward.Target.ToString());
						return false;
					}
					const int32 ExistingIndex = Outcome.DetailTargets.IndexOfByKey(Reward.Target);
					if (ExistingIndex == INDEX_NONE)
					{
						Outcome.DetailTargets.Add(Reward.Target);
						Outcome.DetailValues.Add(Reward.Value);
					}
					else
					{
						Outcome.DetailValues[ExistingIndex] += Reward.Value;
					}
				}
				Outcome.ResolutionCount = RewardCount;
			}
			else if (Effect.BehaviorId == TEXT("Card.RandomStatTrade"))
			{
				FRandomStream Random(
				    HashCombine(GetTypeHash(Input.RandomSeed), GetTypeHash(Result.CardState.Runtime.RandomSequence++)));
				const bool bPhysicalWins = Random.RandRange(0, 1) == 0;
				float& Winner = bPhysicalWins ? Result.Stats.PhysicalAttack : Result.Stats.ElementalAttack;
				float& Loser = bPhysicalWins ? Result.Stats.ElementalAttack : Result.Stats.PhysicalAttack;
				Winner *= 1.0f + Effect.Value;
				Loser *= 1.0f - Effect.ParamValue;
				FReEchoCardOutcomeState& Outcome =
				    FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::StatTrade);
				Outcome.PrimaryTarget = bPhysicalWins ? TEXT("PhysicalAttack") : TEXT("ElementalAttack");
				Outcome.PrimaryValue = Effect.Value;
				Outcome.SecondaryTarget = bPhysicalWins ? TEXT("ElementalAttack") : TEXT("PhysicalAttack");
				Outcome.SecondaryValue = -Effect.ParamValue;
				++Outcome.ResolutionCount;
				Outcome.EncounterIndex = INDEX_NONE;
			}
			else if (Effect.BehaviorId == TEXT("Card.RandomRateTrade"))
			{
				FRandomStream Random(
				    HashCombine(GetTypeHash(Input.RandomSeed), GetTypeHash(Result.CardState.Runtime.RandomSequence++)));
				const bool bReactionWins = Random.RandRange(0, 1) == 0;
				float& Winner = bReactionWins ? Result.Stats.ReactionEfficiency : Result.Stats.CriticalEffect;
				float& Loser = bReactionWins ? Result.Stats.CriticalEffect : Result.Stats.ReactionEfficiency;
				Winner *= Effect.Value;
				Loser *= Effect.ParamValue;
				FReEchoCardOutcomeState& Outcome =
				    FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::StatTrade);
				Outcome.PrimaryTarget = bReactionWins ? TEXT("ReactionEfficiency") : TEXT("CriticalEffect");
				Outcome.PrimaryValue = Effect.Value - 1.0f;
				Outcome.SecondaryTarget = bReactionWins ? TEXT("CriticalEffect") : TEXT("ReactionEfficiency");
				Outcome.SecondaryValue = Effect.ParamValue - 1.0f;
				++Outcome.ResolutionCount;
				Outcome.EncounterIndex = INDEX_NONE;
			}
			else if (Effect.BehaviorId == TEXT("Card.ResetRunes"))
			{
				Result.bClearWeaponRunes = true;
				Result.TimeShards = FMath::Max(0, Result.TimeShards + FMath::RoundToInt(Effect.Value));
				const int32 GrantedRefreshes = FMath::Max(0, FMath::RoundToInt(Effect.ParamValue));
				Result.CardState.Runtime.FreeShopRefreshes += GrantedRefreshes;
				FReEchoCardOutcomeState& Outcome =
				    FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::RunReset);
				Outcome.PrimaryTarget = TEXT("TimeShards");
				Outcome.PrimaryValue = Effect.Value;
				Outcome.SecondaryTarget = TEXT("FreeShopRefresh");
				Outcome.SecondaryValue = GrantedRefreshes;
				Outcome.ResolutionCount = 1;
			}
			else if (Effect.BehaviorId == TEXT("Card.FreeShopVisit"))
			{
				Result.CardState.Runtime.FreeShopEncounterIndex = Input.EncounterIndex;
				FReEchoCardOutcomeState& Outcome =
				    FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::FreeShopVisit);
				Outcome.PrimaryTarget = TEXT("ShopPrice");
				Outcome.PrimaryValue = 0.0f;
				Outcome.EncounterIndex = Input.EncounterIndex;
				Outcome.ResolutionCount = 1;
			}
			else if (Effect.BehaviorId == TEXT("Card.UnlimitedShopRefresh"))
			{
				Result.CardState.Runtime.bUnlimitedWeaponRuneRefresh = true;
				FReEchoCardOutcomeState& Outcome =
				    FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::UnlimitedRefresh);
				Outcome.PrimaryTarget = TEXT("WeaponRuneShop");
				Outcome.PrimaryValue = 1.0f;
				Outcome.ResolutionCount = 1;
			}
			else if (Effect.BehaviorId == TEXT("Card.BloodForging"))
			{
				const float GrantedHp = Result.Stats.PhysicalAttack + Result.Stats.ElementalAttack;
				Result.Stats.HpMax += GrantedHp;
				Result.Stats.HpPoint = Result.Stats.HpMax;
				Result.HealthAdjustment = EReEchoHealthAdjustment::FillToMax;
				AccumulateOutcome(Result.CardState.Runtime, Card->Id, TEXT("HpMax"), GrantedHp);
			}
			else if (Effect.BehaviorId == TEXT("Card.NextShardDrop"))
			{
				Result.TimeShards = 0;
				Result.CardState.Runtime.BonusShardDropEncounterIndex = Input.EncounterIndex + 1;
			}
			else if (Effect.BehaviorId == TEXT("Card.TrackNextKills"))
			{
				Result.CardState.Runtime.HuntTrackingEncounterIndex = Input.EncounterIndex + 1;
				Result.CardState.Runtime.HuntKillCount = 0;
				SetPendingOutcome(Result.CardState.Runtime,
				                  Card->Id,
				                  Effect.Target,
				                  Result.CardState.Runtime.HuntTrackingEncounterIndex);
			}
			else if (Effect.BehaviorId == TEXT("Card.TrackNextReactions"))
			{
				Result.CardState.Runtime.ReactionTrackingEncounterIndex = Input.EncounterIndex + 1;
				Result.CardState.Runtime.ReactionCount = 0;
				SetPendingOutcome(Result.CardState.Runtime,
				                  Card->Id,
				                  Effect.Target,
				                  Result.CardState.Runtime.ReactionTrackingEncounterIndex);
			}
			else if (Effect.BehaviorId == TEXT("Card.HarvestPenalty"))
			{
				Result.TimeShards += FMath::RoundToInt(Effect.Value);
				FRandomStream Random(
				    HashCombine(GetTypeHash(Input.RandomSeed), GetTypeHash(Result.CardState.Runtime.RandomSequence++)));
				const int32 ChoiceCount = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
				Result.CardState.Runtime.EconomyPenalty =
				    static_cast<EReEchoCardEconomyPenalty>(Random.RandRange(0, ChoiceCount - 1) + 1);
				FReEchoCardOutcomeState& Outcome =
				    FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::EconomyPenalty);
				Outcome.EconomyPenalty = Result.CardState.Runtime.EconomyPenalty;
				Outcome.ResolutionCount = 1;
				Outcome.EncounterIndex = INDEX_NONE;
			}
			else if (Effect.BehaviorId == TEXT("Card.SoloBody"))
			{
				Result.Stats.HpMax *= Effect.Value;
				Result.Stats.HpPoint *= Effect.Value;
				Result.Stats.PhysicalAttack *= Effect.Value;
				Result.Stats.ElementalAttack *= Effect.Value;
				Result.Stats.AttackSpeed *= Effect.Value;
				Result.Stats.MovementSpeed *= Effect.Value;
				Result.Stats.CriticalRate *= Effect.Value;
				Result.Stats.CriticalEffect *= Effect.Value;
				Result.Stats.EchoEfficiency *= Effect.Value;
				Result.Stats.ReactionEfficiency *= Effect.Value;
			}
			else if (Effect.BehaviorId == TEXT("Card.GrantTier"))
			{
				const int32 GrantCount = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
				for (int32 GrantIndex = 0; GrantIndex < GrantCount; ++GrantIndex)
				{
					const int32 RequiredTier = Effect.Target == TEXT("MinimumGuaranteedTier") && GrantIndex > 0
					                               ? INDEX_NONE
					                               : FMath::RoundToInt(Effect.Value);
					TArray<FReEchoCardDefinition> Pool =
					    BuildOfferPool(Catalog, Result.CardState, TEXT("Trait"), RequiredTier, Input.EncounterIndex);
					Pool.RemoveAll(
					    [&](const FReEchoCardDefinition& Candidate)
					    {
						    return GrantedThisTransaction.Contains(Candidate.Id);
					    });
					if (Pool.IsEmpty())
					{
						Result.Error = TEXT("Tier grant has no eligible card");
						return false;
					}
					FRandomStream Random(HashCombine(GetTypeHash(Input.RandomSeed),
					                                 GetTypeHash(Result.CardState.Runtime.RandomSequence++)));
					const FName GrantedCardId = Pool[Random.RandRange(0, Pool.Num() - 1)].Id;
					if (!GrantSingle(GrantedCardId, true, false))
					{
						return false;
					}
					FReEchoCardOutcomeState& Outcome =
					    FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::GrantedCards);
					Outcome.RelatedCardIds.Add(GrantedCardId);
                    ++Outcome.ResolutionCount;
                }
            }
            else if (Effect.BehaviorId == TEXT("Card.GrantAllTier1"))
            {
                // 立即获得每张启用的目标阶级卡牌各 1 张；已拥有的可堆叠卡也额外获得一张。
                // 仅跳过当前事务已发放的卡，防止嵌套 OnGrant 行为产生重复授予。
                TArray<FReEchoCardDefinition> Tier1Cards = Catalog.GetAll(FMath::RoundToInt(Effect.Value));
                for (const FReEchoCardDefinition& Tier1Card : Tier1Cards)
                {
                    if (GrantedThisTransaction.Contains(Tier1Card.Id))
                    {
                        continue;
                    }
                    // bForce=true：绕过 CanOffer 的上架/冲突/遭遇限制，强制入袋。
                    if (!GrantSingle(Tier1Card.Id, true, true))
                    {
                        return false;
                    }
                    FReEchoCardOutcomeState& Outcome =
                        FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::GrantedCards);
                    Outcome.RelatedCardIds.Add(Tier1Card.Id);
                    ++Outcome.ResolutionCount;
                }
            }
        }
        return true;
    };

	if (!GrantSingle(CardId, Input.bRecordOwnership, false))
	{
		const FString Error = Result.Error;
		Result = {};
		Result.Stats = Input.Stats;
		Result.CardState = Input.CardState;
		Result.TimeShards = Input.TimeShards;
		Result.Error = Error;
		return Result;
	}
	const FReEchoCardRuleSnapshot GrantedRules = CompileRules(Catalog, Result.CardState);
	const float DesiredEchoEfficiency =
	    GrantedRules.bEchoLegs ? (GrantedRules.bEchoTrinityComplete ? 1.0f : 0.3f) : 0.0f;
	const float EchoEfficiencyDelta = DesiredEchoEfficiency - Result.CardState.Runtime.EchoTrinityEfficiencyGranted;
	if (!FMath::IsNearlyZero(EchoEfficiencyDelta))
	{
		Result.Stats.EchoEfficiency += EchoEfficiencyDelta;
		Result.CardState.Runtime.EchoTrinityEfficiencyGranted = DesiredEchoEfficiency;
	}
	Result.bSucceeded = true;
	return Result;
}

FReEchoCardBuildState ReEchoCardRuntime::BeginEncounter(const FReEchoCardBuildState& State, const int32 EncounterIndex)
{
	FReEchoCardBuildState Result = State;
	Result.Runtime.ActiveEncounterIndex = EncounterIndex;
	Result.Runtime.LastEncounterTimeSeconds = 0.0f;
	Result.Runtime.PreventedDamageCount = 0;
	Result.Runtime.bSubstitutionEchoRemovalFired = false;
	Result.Runtime.bTenSecondStunFired = false;
	Result.Runtime.bTwentySecondStunFired = false;
	Result.Runtime.LastEchoAuraPulseIndex = 0;
	Result.Runtime.LastEchoHeadCursePulseIndex = 0;
	Result.Runtime.LastEasterStunPulseIndex = 0;
	Result.Runtime.EasterDamageTaken = 0.0f;
	Result.Runtime.CurrentEncounterGrossShardIncome = 0;
	Result.Runtime.ReactionHealCooldownRemaining = 0.0f;
	Result.Runtime.EncounterKillCount = 0;
	Result.Runtime.EncounterPlayerDamage = 0.0f;
	Result.Runtime.EncounterEchoDamage = 0.0f;
	Result.Runtime.ConductAffectedCount = 0;
	Result.Runtime.ConductAffectedSpawnIndices.Reset();
	Result.Runtime.ConductPlayerDamageMultiplier = 1.0f;
	if (Result.Runtime.FreeShopEncounterIndex != INDEX_NONE && Result.Runtime.FreeShopEncounterIndex < EncounterIndex)
	{
		Result.Runtime.FreeShopEncounterIndex = INDEX_NONE;
	}
	return Result;
}

FReEchoCardEncounterTickResult ReEchoCardRuntime::AdvanceEncounter(const FReEchoCardCatalog& Catalog,
                                                                   const FReEchoCardBuildState& State,
                                                                   const float EncounterTimeSeconds)
{
	FReEchoCardEncounterTickResult Result;
	Result.CardState = State;
	const float ClampedTime = FMath::Max(0.0f, EncounterTimeSeconds);
	const float Delta = FMath::Max(0.0f, ClampedTime - Result.CardState.Runtime.LastEncounterTimeSeconds);
	Result.CardState.Runtime.LastEncounterTimeSeconds = ClampedTime;
	Result.CardState.Runtime.ReactionHealCooldownRemaining =
	    FMath::Max(0.0f, Result.CardState.Runtime.ReactionHealCooldownRemaining - Delta);
	const FReEchoCardRuleSnapshot TickRules = CompileRules(Catalog, State);
	if (TickRules.bEchoHead)
	{
		const float Interval = TickRules.bEchoTrinityComplete ? 0.5f : 3.0f;
		const int32 PulseIndex = FMath::FloorToInt(ClampedTime / Interval);
		if (PulseIndex > Result.CardState.Runtime.LastEchoHeadCursePulseIndex)
		{
			Result.EchoHeadCursePulseCount = PulseIndex - Result.CardState.Runtime.LastEchoHeadCursePulseIndex;
			Result.CardState.Runtime.LastEchoHeadCursePulseIndex = PulseIndex;
		}
	}
	if (TickRules.bEasterRandomStun)
	{
		const int32 PulseIndex = FMath::FloorToInt(ClampedTime / TickRules.EasterRandomStunInterval);
		if (PulseIndex > Result.CardState.Runtime.LastEasterStunPulseIndex)
		{
			Result.EasterRandomStunPulseCount = PulseIndex - Result.CardState.Runtime.LastEasterStunPulseIndex;
			Result.EasterRandomStunRadiusCm = TickRules.EasterRandomStunRadiusCm;
			Result.EasterRandomStunDuration = TickRules.EasterRandomStunDuration;
			Result.CardState.Runtime.LastEasterStunPulseIndex = PulseIndex;
		}
	}
	ForEachOwnedEffect(
	    Catalog,
	    State,
	    TEXT("OnEncounterTick"),
	    [&](const FReEchoCardDefinition&, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
	    {
		    if (Effect.BehaviorId == TEXT("Card.EncounterStun"))
		    {
			    const bool bTenSecond = Effect.ParamValue <= 10.0f;
			    bool& bFired = bTenSecond ? Result.CardState.Runtime.bTenSecondStunFired
			                              : Result.CardState.Runtime.bTwentySecondStunFired;
			    if (!bFired && ClampedTime >= Effect.ParamValue)
			    {
				    bFired = true;
				    Result.EnemyStunDurations.Add(Effect.Value * FMath::Max(1, StackCount));
			    }
		    }
		    else if (Effect.BehaviorId == TEXT("Card.EchoElementAura"))
		    {
			    const float Interval = FMath::Max(0.1f, Effect.Value);
			    const int32 PulseIndex = FMath::FloorToInt(ClampedTime / Interval);
			    if (PulseIndex > Result.CardState.Runtime.LastEchoAuraPulseIndex)
			    {
				    Result.EchoAuraPulseCount = FMath::Max(
				        Result.EchoAuraPulseCount, PulseIndex - Result.CardState.Runtime.LastEchoAuraPulseIndex);
				    Result.CardState.Runtime.LastEchoAuraPulseIndex = PulseIndex;
			    }
		    }
	    });
	return Result;
}

FReEchoCardOutgoingHitResult ReEchoCardRuntime::ModifyOutgoingHit(const FReEchoCardCatalog& Catalog,
                                                                  const FReEchoCardBuildState& State,
                                                                  const FReEchoCardOutgoingHitInput& Input)
{
	FReEchoCardOutgoingHitResult Result;
	Result.CardState = State;
	Result.RawDamage = FMath::Max(0.0f, Input.RawDamage);
	Result.bCritical = Input.bCritical;
	Result.Element = Input.Element;
	Result.TimeShards = FMath::Max(0, Input.TimeShards);
	if (Input.DamageSource == EReEchoDamageSource::Echo)
	{
		Result.RawDamage *= FMath::Max(0.0f, Result.CardState.Runtime.EchoDamageMultiplier);
	}
	else if (Input.DamageSource == EReEchoDamageSource::Player)
	{
		Result.RawDamage *= FMath::Max(0.0f, Result.CardState.Runtime.PlayerDamageMultiplier);
		Result.RawDamage *= FMath::Max(0.0f, Result.CardState.Runtime.ConductPlayerDamageMultiplier);
	}
	const FReEchoCardRuleSnapshot Rules = CompileRules(Catalog, State);

	const bool bWasCritical = Result.bCritical;
	const float CriticalEffect = FMath::Max(0.0f, Input.CriticalEffect);
	const float PreCriticalDamage =
	    bWasCritical ? Result.RawDamage / FMath::Max(1.0f, 1.0f + CriticalEffect) : Result.RawDamage;
	if (!Result.bCritical)
	{
		const int32 RuntimeRollCount = Result.Element != EReEchoElement::None
		                                   ? (Rules.bElementDamageCanCrit ? Rules.CriticalRollCount : 0)
		                                   : FMath::Max(0, Rules.CriticalRollCount - 1);
		for (int32 RollIndex = 0; RollIndex < RuntimeRollCount && !Result.bCritical; ++RollIndex)
		{
			FRandomStream Random(
			    HashCombine(GetTypeHash(Input.RandomSeed), GetTypeHash(Result.CardState.Runtime.RandomSequence++)));
			Result.bCritical |= Random.FRand() < FMath::Clamp(Input.CriticalRate, 0.0f, 1.0f);
		}
	}
	Result.RawDamage =
	    Result.bCritical
	        ? PreCriticalDamage *
	              (1.0f + CriticalEffect + (Input.bTargetHasElement ? Rules.ElementAttachedCriticalEffectBonus : 0.0f))
	        : PreCriticalDamage;
	const float DistanceDamageSteps =
	    FMath::FloorToFloat(FMath::Max(0.0f, Input.DistanceCm) / FMath::Max(1.0f, Rules.DistanceDamageStepCm));
	Result.RawDamage *= 1.0f + Rules.DistanceDamageBonusPerStep * DistanceDamageSteps;
	if (Input.bHasLivingEcho && Rules.ProximityDamageBonus > 0.0f &&
	    (Input.DamageSource == EReEchoDamageSource::Player || Input.DamageSource == EReEchoDamageSource::Echo))
	{
		const float DistanceAlpha = 1.0f - FMath::Clamp(Input.NearestEchoDistanceCm / 3000.0f, 0.0f, 1.0f);
		Result.RawDamage *= 1.0f + Rules.ProximityDamageBonus * DistanceAlpha;
	}
	if (Rules.bAlternatingPlayerEchoDamage &&
	    (Input.DamageSource == EReEchoDamageSource::Player || Input.DamageSource == EReEchoDamageSource::Echo) &&
	    Input.bPreviousPlayerEchoSourceKnown && Input.PreviousPlayerEchoSource != Input.DamageSource)
	{
		Result.RawDamage *= 2.0f;
	}
	if (Result.bCritical && Rules.bCriticalOverridesElement)
	{
		FRandomStream Random(
		    HashCombine(GetTypeHash(Input.RandomSeed), GetTypeHash(Result.CardState.Runtime.RandomSequence++)));
		Result.Element = static_cast<EReEchoElement>(
		    Random.RandRange(static_cast<int32>(EReEchoElement::Flame), static_cast<int32>(EReEchoElement::Water)));
	}

	ForEachOwnedEffect(
	    Catalog,
	    State,
	    TEXT("BeforeOutgoingHit"),
	    [&](const FReEchoCardDefinition& Card, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
	    {
		    if (Effect.BehaviorId == TEXT("Card.EasterPhysicalLottery") && Effect.Order == 1 &&
		        Result.Element == EReEchoElement::None &&
		        (Input.DamageSource == EReEchoDamageSource::Player || Input.DamageSource == EReEchoDamageSource::Echo))
		    {
			    FRandomStream Random(
			        HashCombine(GetTypeHash(Input.RandomSeed), GetTypeHash(Result.CardState.Runtime.RandomSequence++)));
			    const float Roll = Random.FRand();
			    float Cumulative = 0.0f;
			    for (const FReEchoCardEffectDefinition& Outcome : Card.Effects)
			    {
				    if (Outcome.BehaviorId != TEXT("Card.EasterPhysicalLottery"))
				    {
					    continue;
				    }
				    Cumulative += FMath::Max(0.0f, Outcome.ParamValue);
				    if (Roll <= Cumulative)
				    {
					    Result.RawDamage = FMath::Max(0.0f, Outcome.Value);
					    break;
				    }
			    }
			    return;
		    }
		    if (Effect.BehaviorId == TEXT("Card.TargetKillCurse"))
		    {
			    const int32 Threshold = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
			    if (Input.TargetDefinitionId == Effect.ParamName &&
			        Result.CardState.Runtime.PlayerKillCountByEnemyId.FindRef(Effect.ParamName) >= Threshold)
			    {
				    Result.PreDamageStatusIds.AddUnique(TEXT("Z_Cursed"));
			    }
			    return;
		    }
		    if (Effect.BehaviorId != TEXT("Card.ShardOutgoingDamage"))
		    {
			    return;
		    }
		    const int32 CostPerStack = FMath::Max(0, FMath::RoundToInt(Effect.ParamValue));
		    const int32 AffordableStacks =
		        CostPerStack == 0 ? StackCount : FMath::Min(StackCount, Result.TimeShards / CostPerStack);
		    Result.TimeShards -= AffordableStacks * CostPerStack;
		    Result.RawDamage *= 1.0f + Effect.Value * AffordableStacks;
	    });
	Result.RawDamage = FMath::Max(0.0f, Result.RawDamage);
	return Result;
}

FReEchoCardIncomingHitResult ReEchoCardRuntime::ModifyIncomingHit(const FReEchoCardCatalog& Catalog,
                                                                  const FReEchoCardBuildState& State,
                                                                  const float RawDamage,
                                                                  const int32 TimeShards,
                                                                  const FName AttackerDefinitionId)
{
	FReEchoCardIncomingHitResult Result;
	Result.CardState = State;
	Result.RawDamage = FMath::Max(0.0f, RawDamage);
	Result.TimeShards = FMath::Max(0, TimeShards);
	ForEachOwnedEffect(
	    Catalog,
	    State,
	    TEXT("BeforeIncomingHit"),
	    [&](const FReEchoCardDefinition&, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
	    {
		    if (Effect.BehaviorId == TEXT("Card.DamageSubstitution"))
		    {
			    const int32 FreeHits = FMath::Max(0, FMath::RoundToInt(Effect.ParamValue)) * StackCount;
			    if (Result.CardState.Runtime.PreventedDamageCount < FreeHits && Result.RawDamage > 0.0f)
			    {
				    ++Result.CardState.Runtime.PreventedDamageCount;
				    Result.RawDamage = 0.0f;
				    Result.bPrevented = true;
			    }
			    else if (!Result.CardState.Runtime.bSubstitutionEchoRemovalFired && Result.RawDamage > 0.0f)
			    {
				    Result.CardState.Runtime.bSubstitutionEchoRemovalFired = true;
				    Result.bRemoveAllEchoes = true;
			    }
		    }
		    else if (Effect.BehaviorId == TEXT("Card.ShardIncomingBarrier") && Result.RawDamage > 0.0f)
		    {
			    const int32 CostPerStack = FMath::Max(0, FMath::RoundToInt(Effect.ParamValue));
			    const int32 AffordableStacks =
			        CostPerStack == 0 ? StackCount : FMath::Min(StackCount, Result.TimeShards / CostPerStack);
			    Result.TimeShards -= AffordableStacks * CostPerStack;
            Result.RawDamage = FMath::Max(0.0f, Result.RawDamage + Effect.Value * AffordableStacks);
        }
    });
    if (!Result.bPrevented && !AttackerDefinitionId.IsNone() &&
        State.Runtime.ImmuneEnemyDefinitionIds.Contains(AttackerDefinitionId))
    {
        Result.RawDamage = 0.0f;
        Result.bPrevented = true;
    }
    return Result;
}

FReEchoCardEventResult ReEchoCardRuntime::OnReaction(const FReEchoCardCatalog& Catalog,
                                                     const FReEchoCardBuildState& State,
                                                     const FReEchoStatBlock& Stats,
                                                     const FName ReactionId,
                                                     const bool bTriggeredByPlayer)
{
	FReEchoCardEventResult Result;
	Result.CardState = State;
	Result.Stats = Stats;
	ForEachOwnedEffect(
	    Catalog,
	    State,
	    TEXT("OnReaction"),
	    [&](const FReEchoCardDefinition& Card, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
	    {
		    if (!bTriggeredByPlayer)
		    {
			    return;
		    }
		    if (Effect.BehaviorId == TEXT("Card.RecordReaction") &&
		        Result.CardState.Runtime.RecordedReactionId.IsNone() && !ReactionId.IsNone())
		    {
			    Result.CardState.Runtime.RecordedReactionId = ReactionId;
			    SetStatGainOutcome(Result.CardState.Runtime, Card.Id, TEXT("RecordedReaction"), 1.0f);
			    FReEchoCardOutcomeState& Outcome =
			        FindOrAddOutcome(Result.CardState.Runtime, Card.Id, EReEchoCardOutcomeKind::StatGain);
			    Outcome.SecondaryTarget = ReactionId;
			    Outcome.SecondaryValue = 1.0f;
		    }
		    else if (Effect.BehaviorId == TEXT("Card.ReactionHeal") &&
		             Result.CardState.Runtime.ReactionHealCooldownRemaining <= 0.0f)
		    {
			    Result.Healing += Result.Stats.HpMax * Effect.Value * StackCount;
			    Result.CardState.Runtime.ReactionHealCooldownRemaining = Effect.ParamValue;
		    }
		    else if (Effect.BehaviorId == TEXT("Card.ReactionDiversity") && !ReactionId.IsNone())
		    {
			    Result.CardState.Runtime.DistinctReactionIds.AddUnique(ReactionId);
			    const int32 Threshold = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
			    if (Result.CardState.Runtime.DistinctReactionIds.Num() >= Threshold)
			    {
				    const float Granted = Effect.Value * StackCount;
				    Result.Stats.ReactionEfficiency += Granted;
				    AccumulateOutcome(Result.CardState.Runtime, Card.Id, TEXT("ReactionEfficiency"), Granted);
				    Result.CardState.Runtime.DistinctReactionIds.Reset();
			    }
		    }
	    });
	if (Result.CardState.Runtime.ReactionTrackingEncounterIndex == Result.CardState.Runtime.ActiveEncounterIndex)
	{
		++Result.CardState.Runtime.ReactionCount;
	}
	return Result;
}

float ReEchoCardRuntime::GetReactionDamageMultiplier(const FReEchoCardCatalog& Catalog,
                                                     const FReEchoCardBuildState& State,
                                                     const FName ReactionId)
{
	if (ReactionId.IsNone() || State.Runtime.RecordedReactionId.IsNone())
	{
		return 1.0f;
	}
	float Multiplier = 1.0f;
	ForEachOwnedEffect(
	    Catalog,
	    State,
	    TEXT("OnReaction"),
	    [&](const FReEchoCardDefinition&, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
	    {
		    if (Effect.BehaviorId != TEXT("Card.RecordReaction"))
		    {
			    return;
		    }
		    Multiplier = ReactionId == State.Runtime.RecordedReactionId
		                     ? 1.0f + FMath::Max(0.0f, Effect.Value) * StackCount
		                     : FMath::Max(0.0f, 1.0f - FMath::Max(0.0f, Effect.ParamValue) * StackCount);
	    });
	return Multiplier;
}

FReEchoCardEventResult ReEchoCardRuntime::OnKillResolved(const FReEchoCardCatalog& Catalog,
                                                         const FReEchoCardBuildState& State,
                                                         const FReEchoStatBlock& Stats,
                                                         const bool bKilledByEcho,
                                                         const FName TargetDefinitionId)
{
	FReEchoCardEventResult Result;
	Result.CardState = State;
	Result.Stats = Stats;
	++Result.CardState.Runtime.EncounterKillCount;
	if (Result.CardState.Runtime.HuntTrackingEncounterIndex == State.Runtime.ActiveEncounterIndex)
	{
		++Result.CardState.Runtime.HuntKillCount;
	}
	if (!bKilledByEcho && !TargetDefinitionId.IsNone())
	{
		++Result.CardState.Runtime.PlayerKillCountByEnemyId.FindOrAdd(TargetDefinitionId);
	}
	ForEachOwnedEffect(
	    Catalog,
	    State,
	    TEXT("OnHitResolved"),
	    [&](const FReEchoCardDefinition& Card, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
	    {
		    if (Effect.BehaviorId == TEXT("Card.KillThresholdStatBoost") && !bKilledByEcho &&
		        Effect.ParamName == TargetDefinitionId &&
		        !Result.CardState.Runtime.CompletedKillThresholdCardIds.Contains(Card.Id))
		    {
			    const int32 KillCount = Result.CardState.Runtime.PlayerKillCountByEnemyId.FindRef(TargetDefinitionId);
			    if (KillCount >= FMath::Max(1, FMath::RoundToInt(Effect.ParamValue)))
			    {
				    if (Effect.Target == TEXT("HpMax"))
				    {
					    Result.Stats.HpMax *= Effect.Value;
					    Result.Stats.HpPoint = Result.Stats.HpMax;
				    }
				    Result.CardState.Runtime.CompletedKillThresholdCardIds.Add(Card.Id);
				    FReEchoCardOutcomeState& Outcome =
					    FindOrAddOutcome(Result.CardState.Runtime, Card.Id, EReEchoCardOutcomeKind::StatGain);
				    Outcome.PrimaryTarget = TEXT("KillThreshold");
				    Outcome.PrimaryValue = Effect.Value;
				    ++Outcome.ResolutionCount;
			    }
			    return;
		    }
		    if (Effect.BehaviorId == TEXT("Card.KillThresholdImmunity") && !bKilledByEcho &&
		        Effect.ParamName == TargetDefinitionId &&
		        !Result.CardState.Runtime.CompletedKillThresholdCardIds.Contains(Card.Id))
		    {
			    const int32 KillCount = Result.CardState.Runtime.PlayerKillCountByEnemyId.FindRef(TargetDefinitionId);
			    if (KillCount >= FMath::Max(1, FMath::RoundToInt(Effect.ParamValue)))
			    {
				    Result.CardState.Runtime.ImmuneEnemyDefinitionIds.Add(TargetDefinitionId);
				    Result.CardState.Runtime.CompletedKillThresholdCardIds.Add(Card.Id);
				    FReEchoCardOutcomeState& Outcome =
					    FindOrAddOutcome(Result.CardState.Runtime, Card.Id, EReEchoCardOutcomeKind::StatGain);
				    Outcome.PrimaryTarget = TEXT("EnemyImmunity");
				    Outcome.PrimaryValue = 1.0f;
				    ++Outcome.ResolutionCount;
			    }
			    return;
		    }
		    if (Effect.BehaviorId == TEXT("Card.TargetKillCurse") && !bKilledByEcho &&
		        Effect.ParamName == TargetDefinitionId)
		    {
			    const int32 KillCount = Result.CardState.Runtime.PlayerKillCountByEnemyId.FindRef(TargetDefinitionId);
			    FReEchoCardOutcomeState& Outcome =
			        FindOrAddOutcome(Result.CardState.Runtime, Card.Id, EReEchoCardOutcomeKind::StatGain);
			    Outcome.PrimaryTarget = TargetDefinitionId;
			    Outcome.PrimaryValue = KillCount;
			    Outcome.SecondaryTarget = TEXT("KillThreshold");
			    Outcome.SecondaryValue = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
			    Outcome.ResolutionCount = KillCount;
			    return;
		    }
		    if (Effect.BehaviorId != TEXT("Card.SoulResonance"))
		    {
			    return;
		    }
		    const bool bEchoEffect = Effect.ParamName == TEXT("EchoKillThreshold");
		    if (bEchoEffect != bKilledByEcho)
		    {
			    return;
		    }
		    int32& Progress =
		        bKilledByEcho ? Result.CardState.Runtime.EchoKillProgress : Result.CardState.Runtime.PlayerKillProgress;
		    ++Progress;
		    const int32 Threshold = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
		    const int32 GrowthCount = Progress / Threshold;
		    Progress %= Threshold;
		    if (bKilledByEcho)
		    {
			    const float Granted = GrowthCount * Effect.Value * StackCount;
			    Result.Stats.PhysicalAttack += Granted;
			    Result.Stats.ElementalAttack += Granted;
			    if (Granted > 0.0f)
			    {
				    AccumulateOutcome(Result.CardState.Runtime,
				                      Card.Id,
				                      TEXT("PhysicalAttack"),
				                      Granted,
				                      TEXT("ElementalAttack"),
				                      Granted);
			    }
		    }
		    else
		    {
			    const float Granted = GrowthCount * Effect.Value * StackCount;
			    Result.Stats.EchoEfficiency += Granted;
			    if (Granted > 0.0f)
			    {
				    AccumulateOutcome(Result.CardState.Runtime, Card.Id, TEXT("EchoEfficiency"), Granted);
			    }
		    }
	    });
	ForEachOwnedEffect(
	    Catalog,
	    State,
	    TEXT("OnHitResolved"),
	    [&](const FReEchoCardDefinition& Card, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
	    {
		    if (Effect.BehaviorId != TEXT("Card.NumericChallenge") || Effect.ParamName != TEXT("KillThreshold") ||
		        Result.CardState.Runtime.bNumericChallengeCompleted)
		    {
			    return;
		    }
		    const int32 Threshold = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
		    if (Result.CardState.Runtime.EncounterKillCount >= Threshold)
		    {
			    const int32 Granted = FMath::Max(0, FMath::RoundToInt(Effect.Value)) * FMath::Max(1, StackCount);
			    Result.TimeShardsGranted += Granted;
			    Result.CardState.Runtime.bNumericChallengeCompleted = true;
			    SetStatGainOutcome(Result.CardState.Runtime, Card.Id, TEXT("TimeShards"), Granted);
		    }
	    });
	return Result;
}

FName ReEchoCardRuntime::SelectOfferForSlot(const FReEchoCardCatalog& Catalog,
                                            const FReEchoCardBuildState& State,
                                            const int32 NormalTier,
                                            const int32 EncounterIndex,
	                                            const TArray<FName>& OfferHistory,
	                                            const int32 RandomSeed,
	                                            const float EasterChance,
	                                            const bool bExcludeOwnedNormalCards)
{
	TArray<FReEchoCardDefinition> NormalPool =
	    BuildOfferPool(Catalog, State, TEXT("Trait"), NormalTier, EncounterIndex);
	TArray<FReEchoCardDefinition> EasterPool =
	    BuildOfferPool(Catalog, State, TEXT("EasterEgg"), INDEX_NONE, EncounterIndex);
	NormalPool.RemoveAll(
	    [&](const FReEchoCardDefinition& Candidate)
	    {
		    // 与 CanOffer 的拥有判断保持一致：排除历史卡、以及“拥有即不可再发”的卡
		    // （二阶/三阶已拥有、或一阶 Unique 已拥有）；但保留“一阶可叠加卡已拥有”的情况，
		    // 使其仍可作为重复发牌/刷新的目标（叠加）。
		    return OfferHistory.Contains(Candidate.Id) ||
		           (HasCard(State, Candidate.Id) &&
		            (Candidate.Tier != 1 || Candidate.StackPolicy == TEXT("Unique") || bExcludeOwnedNormalCards));
	    });
	EasterPool.RemoveAll(
	    [&](const FReEchoCardDefinition& Candidate)
	    {
		    return OfferHistory.Contains(Candidate.Id) || HasCard(State, Candidate.Id);
	    });

	FRandomStream Random(RandomSeed);
	const bool bEasterHit = !EasterPool.IsEmpty() && Random.FRand() < FMath::Clamp(EasterChance, 0.0f, 1.0f);
	TArray<FReEchoCardDefinition>& SelectedPool = bEasterHit ? EasterPool : NormalPool;
	if (SelectedPool.IsEmpty())
	{
		return NAME_None;
	}
	return SelectedPool[Random.RandRange(0, SelectedPool.Num() - 1)].Id;
}

FReEchoCardEventResult ReEchoCardRuntime::OnPurchase(const FReEchoCardCatalog& Catalog,
                                                     const FReEchoCardBuildState& State,
                                                     const FReEchoStatBlock& Stats)
{
	FReEchoCardEventResult Result;
	Result.CardState = State;
	Result.Stats = Stats;
	ForEachOwnedEffect(
	    Catalog,
	    State,
	    TEXT("OnPurchase"),
	    [&](const FReEchoCardDefinition& Card, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
	    {
		    if (Effect.BehaviorId == TEXT("Card.ShopContract"))
		    {
			    Result.Stats.HpMax += Effect.Value * StackCount;
			    Result.Stats.HpPoint = FMath::Min(Result.Stats.HpMax, Result.Stats.HpPoint + Effect.Value * StackCount);
			    AccumulateOutcome(Result.CardState.Runtime, Card.Id, TEXT("HpMaxAndPoint"), Effect.Value * StackCount);
		    }
		    else if (Effect.BehaviorId == TEXT("Card.UnlimitedShopRefresh"))
		    {
			    Result.CardState.Runtime.bUnlimitedWeaponRuneRefresh = false;
			    FReEchoCardOutcomeState& Outcome =
			        FindOrAddOutcome(Result.CardState.Runtime, Card.Id, EReEchoCardOutcomeKind::UnlimitedRefresh);
			    Outcome.PrimaryValue = 0.0f;
		    }
	    });
	return Result;
}

FReEchoCardEventResult ReEchoCardRuntime::OnEchoKilled(const FReEchoCardCatalog& Catalog,
                                                       const FReEchoCardBuildState& State,
                                                       const FReEchoStatBlock& Stats)
{
	FReEchoCardEventResult Result;
	Result.CardState = State;
	Result.Stats = Stats;
	ForEachOwnedEffect(
	    Catalog,
	    State,
	    TEXT("OnEchoKilled"),
	    [&](const FReEchoCardDefinition& Card, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
	    {
		    if (Effect.BehaviorId == TEXT("Card.TauntEcho"))
		    {
			    Result.Stats.HpMax += Effect.Value * StackCount;
			    Result.Stats.HpPoint = FMath::Min(Result.Stats.HpMax, Result.Stats.HpPoint + Effect.Value * StackCount);
			    AccumulateOutcome(Result.CardState.Runtime, Card.Id, TEXT("HpMaxAndPoint"), Effect.Value * StackCount);
		    }
	    });
	return Result;
}

FReEchoCardEventResult ReEchoCardRuntime::OnCoreInventoryChanged(const FReEchoCardCatalog& Catalog,
                                                                 const FReEchoCardBuildState& State,
                                                                 const FReEchoStatBlock& Stats,
                                                                 const int32 DistinctOwnedCoreCount)
{
	FReEchoCardEventResult Result;
	Result.CardState = State;
	Result.Stats = Stats;
	if (Result.CardState.Runtime.bDragonSoulCompleted || DistinctOwnedCoreCount < 6)
	{
		return Result;
	}
	bool bGrantTieredCards = false;
	int32 TieredRequiredCores = 6;
	FName TieredCardId = NAME_None;
	ForEachOwnedEffect(
	    Catalog,
	    State,
	    TEXT("OnInventoryChanged"),
	    [&](const FReEchoCardDefinition& Card, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
	    {
		    if (Effect.BehaviorId == TEXT("Card.CollectCores") && !Result.CardState.Runtime.bDragonSoulCompleted)
		    {
			    const float Granted = Effect.Value * FMath::Max(1, StackCount);
			    Result.Stats.CriticalRate += Granted;
			    Result.Stats.CriticalEffect += Granted;
			    Result.Stats.ReactionEfficiency += Granted;
			    Result.CardState.Runtime.bDragonSoulCompleted = true;
			    SetStatGainOutcome(Result.CardState.Runtime, Card.Id, TEXT("CoreCollection"), Granted);
		    }
		    else if (Effect.BehaviorId == TEXT("Card.CollectCoresGrantTiered") && !Result.CardState.Runtime.bDragonSoulCompleted)
		    {
			    bGrantTieredCards = true;
			    TieredCardId = Card.Id;
			    TieredRequiredCores = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
		    }
	    });
	if (bGrantTieredCards && DistinctOwnedCoreCount >= TieredRequiredCores && !Result.CardState.Runtime.bDragonSoulCompleted)
	{
		for (int32 Tier = 1; Tier <= 3; ++Tier)
		{
			// BuildOfferPool already applies the canonical ownership rule: tier-one Stackable cards
			// remain eligible for repeat grants, while owned higher-tier and Unique cards are excluded.
			TArray<FReEchoCardDefinition> Pool = BuildOfferPool(Catalog, Result.CardState, TEXT("Trait"), Tier, Result.CardState.Runtime.ActiveEncounterIndex);
			if (Pool.IsEmpty())
			{
				continue;
			}
			FRandomStream Random(HashCombine(GetTypeHash(Result.CardState.Runtime.RandomSequence++), GetTypeHash(Tier)));
			const FName GrantedCardId = Pool[Random.RandRange(0, Pool.Num() - 1)].Id;
			FReEchoCardGrantInput GrantInput;
			GrantInput.Stats = Result.Stats;
			GrantInput.CardState = Result.CardState;
			GrantInput.TimeShards = 0;
			GrantInput.EncounterIndex = Result.CardState.Runtime.ActiveEncounterIndex;
			GrantInput.RandomSeed = HashCombine(GetTypeHash(Result.CardState.Runtime.RandomSequence++), GetTypeHash(GrantedCardId));
			GrantInput.bRecordOwnership = true;
			const FReEchoCardGrantResult Grant = ReEchoCardRuntime::TryGrantCard(Catalog, GrantedCardId, GrantInput);
			if (Grant.bSucceeded)
			{
				Result.Stats = Grant.Stats;
				Result.CardState = Grant.CardState;
				FReEchoCardOutcomeState& Outcome = FindOrAddOutcome(Result.CardState.Runtime, TieredCardId, EReEchoCardOutcomeKind::GrantedCards);
				Outcome.RelatedCardIds.Add(GrantedCardId);
				++Outcome.ResolutionCount;
			}
		}
		Result.CardState.Runtime.bDragonSoulCompleted = true;
	}
	return Result;
}

FReEchoCardEventResult ReEchoCardRuntime::OnDamageResolved(const FReEchoCardCatalog& Catalog,
                                                           const FReEchoCardBuildState& State,
                                                           const FReEchoStatBlock& Stats,
                                                           const float RawDamage,
                                                           const float AppliedDamage,
                                                           const bool bDealtByEcho)
{
	FReEchoCardEventResult Result;
	Result.CardState = State;
	Result.Stats = Stats;
	const float Applied = FMath::Max(0.0f, AppliedDamage);
	if (bDealtByEcho)
	{
		Result.CardState.Runtime.EncounterEchoDamage += Applied;
	}
	else
	{
		Result.CardState.Runtime.EncounterPlayerDamage += Applied;
	}
	if (bDealtByEcho || Applied <= 0.0f)
	{
		return Result;
	}
	const float OverkillDamage = FMath::Max(0.0f, RawDamage - AppliedDamage);
	ForEachOwnedEffect(
	    Catalog,
	    State,
	    TEXT("OnDamageResolved"),
	    [&](const FReEchoCardDefinition&, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
	    {
		    if (Effect.BehaviorId == TEXT("Card.OverkillHeal"))
		    {
			    Result.Healing += OverkillDamage * FMath::Max(0.0f, Effect.Value) * FMath::Max(1, StackCount);
		    }
	    });
	return Result;
}

FReEchoCardGrantResult ReEchoCardRuntime::OnPlayerDamageReceived(const FReEchoCardCatalog& Catalog,
                                                                 const FReEchoCardBuildState& State,
                                                                 const FReEchoStatBlock& Stats,
                                                                 const int32 TimeShards,
                                                                 const float AppliedDamage,
                                                                 const int32 EncounterIndex,
                                                                 const int32 RandomSeed)
{
	FReEchoCardGrantResult Result;
	Result.bSucceeded = true;
	Result.CardState = State;
	Result.Stats = Stats;
	Result.TimeShards = TimeShards;
	if (AppliedDamage <= 0.0f || Result.CardState.Runtime.bEasterDamageCardsGranted ||
	    !HasCard(Result.CardState, TEXT("G_4_6")))
	{
		return Result;
	}
	Result.CardState.Runtime.EasterDamageTaken += AppliedDamage;
	const FReEchoCardDefinition* Card = Catalog.Find(TEXT("G_4_6"));
	if (!Card || Card->Effects.IsEmpty() || Result.CardState.Runtime.EasterDamageTaken < Card->Effects[0].Value)
	{
		return Result;
	}
	Result.CardState.Runtime.bEasterDamageCardsGranted = true;
	TArray<FReEchoCardDefinition> Pool = BuildOfferPool(Catalog, Result.CardState, TEXT("Trait"), INDEX_NONE, EncounterIndex);
	Pool.RemoveAll(
	    [&](const FReEchoCardDefinition& Candidate)
	    {
		    return HasCard(Result.CardState, Candidate.Id);
	    });
	FRandomStream Random(
	    HashCombine(GetTypeHash(RandomSeed), GetTypeHash(Result.CardState.Runtime.RandomSequence++)));
	for (int32 Index = Pool.Num() - 1; Index > 0; --Index)
	{
		Pool.Swap(Index, Random.RandRange(0, Index));
	}
	const int32 GrantCount = FMath::Min(FMath::Max(0, FMath::RoundToInt(Card->Effects[0].ParamValue)), Pool.Num());
	for (int32 Index = 0; Index < GrantCount; ++Index)
	{
		FReEchoCardGrantInput Input;
		Input.Stats = Result.Stats;
		Input.CardState = Result.CardState;
		Input.TimeShards = Result.TimeShards;
		Input.EncounterIndex = EncounterIndex;
		Input.RandomSeed = HashCombine(GetTypeHash(RandomSeed), GetTypeHash(Index));
		const FReEchoCardGrantResult Grant = TryGrantCard(Catalog, Pool[Index].Id, Input);
		if (!Grant.bSucceeded)
		{
			Result.bSucceeded = false;
			Result.Error = Grant.Error;
			return Result;
		}
		Result.Stats = Grant.Stats;
		Result.CardState = Grant.CardState;
		Result.TimeShards = Grant.TimeShards;
		Result.GrantedCardIds.Append(Grant.GrantedCardIds);
		Result.bClearWeaponRunes |= Grant.bClearWeaponRunes;
		if (Grant.HealthAdjustment != EReEchoHealthAdjustment::None)
		{
			Result.HealthAdjustment = Grant.HealthAdjustment;
		}
	}
	FReEchoCardOutcomeState& Outcome =
	    FindOrAddOutcome(Result.CardState.Runtime, TEXT("G_4_6"), EReEchoCardOutcomeKind::GrantedCards);
	Outcome.RelatedCardIds = Result.GrantedCardIds;
	Outcome.ResolutionCount = Result.GrantedCardIds.Num();
	return Result;
}

FReEchoCardEventResult ReEchoCardRuntime::OnNegativeStatusApplied(const FReEchoCardCatalog& Catalog,
                                                                  const FReEchoCardBuildState& State,
                                                                  const FReEchoStatBlock& Stats,
                                                                  const FName StatusId,
                                                                  const bool bAppliedByEcho)
{
	(void)StatusId;
	(void)bAppliedByEcho;
	FReEchoCardEventResult Result;
	Result.CardState = State;
	Result.Stats = Stats;
	ForEachOwnedEffect(
	    Catalog,
	    State,
	    TEXT("OnStatusApplied"),
	    [&](const FReEchoCardDefinition&, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
	    {
		    if (Effect.BehaviorId == TEXT("Card.NegativeStatusHeal"))
		    {
			    Result.Healing += FMath::Max(0.0f, Effect.Value) * FMath::Max(1, StackCount);
		    }
	    });
	return Result;
}

FReEchoCardEventResult ReEchoCardRuntime::EndEncounter(const FReEchoCardCatalog& Catalog,
                                                       const FReEchoCardBuildState& State,
                                                       const FReEchoStatBlock& Stats,
                                                       const int32 EncounterIndex,
                                                       const int32 PlayerKillCount,
                                                       const int32 TimeShards,
                                                       const int32 RandomSeed,
                                                       const int32 GrossTimeShardIncome)
{
	FReEchoCardEventResult Result;
	Result.CardState = State;
	Result.Stats = Stats;
	ForEachOwnedEffect(
	    Catalog,
	    State,
	    TEXT("OnEncounterEnd"),
	    [&](const FReEchoCardDefinition& Card, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
	    {
		    if (Effect.BehaviorId == TEXT("Card.EasterShardSwing") && Effect.Order == 1)
		    {
			    FRandomStream Random(
			        HashCombine(GetTypeHash(RandomSeed), GetTypeHash(Result.CardState.Runtime.RandomSequence++)));
			    const bool bPositive = Random.RandRange(0, 1) == 0;
			    float Multiplier = 1.0f;
			    int32 Bonus = 0;
			    for (const FReEchoCardEffectDefinition& Config : Card.Effects)
			    {
				    if (Config.ParamName == (bPositive ? TEXT("PositiveMultiplier") : TEXT("NegativeMultiplier")))
				    {
					    Multiplier = Config.ParamValue;
				    }
				    else if (Config.ParamName == TEXT("Bonus"))
				    {
					    Bonus = FMath::RoundToInt(Config.ParamValue);
				    }
			    }
			    Result.ProjectedTimeShards = FMath::FloorToInt(FMath::Max(0, TimeShards) * Multiplier) + Bonus;
			    FReEchoCardOutcomeState& Outcome =
			        FindOrAddOutcome(Result.CardState.Runtime, Card.Id, EReEchoCardOutcomeKind::RandomDetails);
			    Outcome.DetailTargets = {TEXT("TimeShardMultiplier"), TEXT("TimeShards")};
			    Outcome.DetailValues = {Multiplier, static_cast<float>(Bonus)};
			    ++Outcome.ResolutionCount;
		    }
		    else if (Effect.BehaviorId == TEXT("Card.EasterShardComparison") && Effect.Order == 1)
		    {
			    float NextMultiplier = 1.0f;
			    if (Result.CardState.Runtime.bHasPreviousEncounterShardIncome)
			    {
				    const int32 Delta = GrossTimeShardIncome - Result.CardState.Runtime.PreviousEncounterGrossShardIncome;
				    const int32 Steps = FMath::Abs(Delta) / FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
				    if (Delta > 0)
				    {
					    NextMultiplier = FMath::Max(0.0f, 1.0f - Steps * FMath::Abs(Effect.Value));
				    }
				    else if (Delta < 0 && Card.Effects.Num() > 1)
				    {
					    NextMultiplier = 1.0f + Steps * FMath::Abs(Card.Effects[1].Value);
				    }
			    }
			    Result.CardState.Runtime.bHasPreviousEncounterShardIncome = true;
			    Result.CardState.Runtime.PreviousEncounterGrossShardIncome = GrossTimeShardIncome;
			    Result.CardState.Runtime.EncounterShardIncomeMultiplier = NextMultiplier;
			    FReEchoCardOutcomeState& Outcome =
			        FindOrAddOutcome(Result.CardState.Runtime, Card.Id, EReEchoCardOutcomeKind::RandomDetails);
			    Outcome.DetailTargets = {TEXT("EncounterGrossTimeShards"), TEXT("NextShardIncomeMultiplier")};
			    Outcome.DetailValues = {static_cast<float>(GrossTimeShardIncome), NextMultiplier};
			    ++Outcome.ResolutionCount;
		    }
		    else if (Effect.BehaviorId == TEXT("Card.EasterAttendance"))
		    {
			    if (Effect.Target == TEXT("HpMaxAndPoint"))
			    {
				    Result.Stats.HpMax += Effect.Value;
				    Result.Stats.HpPoint += Effect.Value;
				    Result.HealthAdjustment = EReEchoHealthAdjustment::SetToStatPoint;
			    }
			    else
			    {
				    ApplyStatEffect(Result.Stats, Effect);
			    }
			    AccumulateOutcome(Result.CardState.Runtime, Card.Id, Effect.Target, Effect.Value);
		    }
		    else if (Effect.BehaviorId == TEXT("Card.EndKillRefresh"))
		    {
			    const int32 TotalKills = FMath::Max(PlayerKillCount, Result.CardState.Runtime.EncounterKillCount);
			    const int32 Threshold = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
			    const int32 Granted = (TotalKills / Threshold) * FMath::RoundToInt(Effect.Value) * StackCount;
			    Result.CardState.Runtime.FreeShopRefreshes += Granted;
			    Result.FreeShopRefreshesGranted += Granted;
			    FReEchoCardOutcomeState& Outcome =
			        FindOrAddOutcome(Result.CardState.Runtime, Card.Id, EReEchoCardOutcomeKind::FreeShopRefreshes);
			    Outcome.PrimaryTarget = TEXT("FreeShopRefresh");
			    Outcome.PrimaryValue += Granted;
			    ++Outcome.ResolutionCount;
		    }
		    else if (Effect.BehaviorId == TEXT("Card.EndKillShards"))
		    {
			    const int32 TotalKills = FMath::Max(PlayerKillCount, Result.CardState.Runtime.EncounterKillCount);
			    // 战利回流：本场时之碎片收入 × 每击杀返还比例(Effect.Value) × 击杀数
			    const float RatePerKill = FMath::Max(0.0f, Effect.Value);
			    const int32 ShardCount = FMath::RoundToInt(GrossTimeShardIncome * RatePerKill * TotalKills * StackCount);
			    Result.TimeShardsGranted += ShardCount;
			    FReEchoCardOutcomeState& Outcome =
			        FindOrAddOutcome(Result.CardState.Runtime, Card.Id, EReEchoCardOutcomeKind::TimeShards);
			    Outcome.PrimaryTarget = TEXT("TimeShards");
			    Outcome.PrimaryValue += ShardCount;
			    ++Outcome.ResolutionCount;
		    }
		    else if (Effect.BehaviorId == TEXT("Card.NumericChallenge") &&
		             !Result.CardState.Runtime.bNumericChallengeCompleted)
		    {
			    const bool bCompleted =
			        (Effect.ParamName == TEXT("MinimumHp") && Result.Stats.HpPoint >= Effect.ParamValue) ||
			        (Effect.ParamName == TEXT("MaximumHp") && Result.Stats.HpPoint <= Effect.ParamValue);
			    if (bCompleted)
			    {
				    const int32 Granted = FMath::Max(0, FMath::RoundToInt(Effect.Value)) * FMath::Max(1, StackCount);
				    Result.TimeShardsGranted += Granted;
				    Result.CardState.Runtime.bNumericChallengeCompleted = true;
				    SetStatGainOutcome(Result.CardState.Runtime, Card.Id, TEXT("TimeShards"), Granted);
			    }
		    }
		    else if (Effect.BehaviorId == TEXT("Card.SelfRace"))
		    {
			    Result.CardState.Runtime.PlayerDamageMultiplier = 1.0f;
			    Result.CardState.Runtime.EchoDamageMultiplier = 1.0f;
			    if (Result.CardState.Runtime.EncounterPlayerDamage > Result.CardState.Runtime.EncounterEchoDamage)
			    {
				    Result.CardState.Runtime.EchoDamageMultiplier += Effect.Value * FMath::Max(1, StackCount);
				    SetStatGainOutcome(Result.CardState.Runtime, Card.Id, TEXT("EchoDamage"), Effect.Value);
			    }
			    else if (Result.CardState.Runtime.EncounterEchoDamage > Result.CardState.Runtime.EncounterPlayerDamage)
			    {
				    Result.CardState.Runtime.PlayerDamageMultiplier += Effect.Value * FMath::Max(1, StackCount);
				    SetStatGainOutcome(Result.CardState.Runtime, Card.Id, TEXT("PlayerDamage"), Effect.Value);
			    }
		    }
	    });
	if (Result.CardState.Runtime.HuntTrackingEncounterIndex == EncounterIndex)
	{
		float Granted = 0.0f;
		const FReEchoCardDefinition* Tracker = Catalog.Find(TEXT("G_2_05"));
		if (Tracker && !Tracker->Effects.IsEmpty())
		{
			const FReEchoCardEffectDefinition& Effect = Tracker->Effects[0];
			const int32 Threshold = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
			Granted = (Result.CardState.Runtime.HuntKillCount / Threshold) * Effect.Value;
			Result.Stats.PhysicalAttack += Granted;
		}
		SetStatGainOutcome(Result.CardState.Runtime, TEXT("G_2_05"), TEXT("PhysicalAttack"), Granted);
		Result.CardState.Runtime.HuntTrackingEncounterIndex = INDEX_NONE;
		Result.CardState.Runtime.HuntKillCount = 0;
	}
	if (Result.CardState.Runtime.ReactionTrackingEncounterIndex == EncounterIndex)
	{
		float Granted = 0.0f;
		const FReEchoCardDefinition* Tracker = Catalog.Find(TEXT("G_2_06"));
		if (Tracker && !Tracker->Effects.IsEmpty())
		{
			const FReEchoCardEffectDefinition& Effect = Tracker->Effects[0];
			const int32 Threshold = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
			Granted = (Result.CardState.Runtime.ReactionCount / Threshold) * Effect.Value;
			Result.Stats.ElementalAttack += Granted;
		}
		SetStatGainOutcome(Result.CardState.Runtime, TEXT("G_2_06"), TEXT("ElementalAttack"), Granted);
		Result.CardState.Runtime.ReactionTrackingEncounterIndex = INDEX_NONE;
		Result.CardState.Runtime.ReactionCount = 0;
	}
	return Result;
}
