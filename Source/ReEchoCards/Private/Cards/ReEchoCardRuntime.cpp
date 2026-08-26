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
		Rules.DistanceDamageBonusPerMeter += StackedValue;
	}
	else if (Effect.BehaviorId == TEXT("Card.CriticalElement"))
	{
		Rules.bCriticalOverridesElement = true;
	}
	else if (Effect.BehaviorId == TEXT("Card.DoubleNonCoreSlots"))
	{
		Rules.bDoubleNonCoreSlotCapacity = true;
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
                                 const FReEchoCardDefinition& Card)
{
	if (!Card.bEnabled || !Card.bOfferable)
	{
		return false;
	}
	// Tier-one cards are the repeatable growth pool. Owned tier-two/three cards are one-time acquisitions and
	// must never return through either the free-draw or shop offer paths.
	if (HasCard(State, Card.Id) && Card.Tier != 1)
	{
		return false;
	}
	const FReEchoCardDefinition* SoloBody = Catalog.Find(TEXT("G_3_03"));
	if (HasCard(State, TEXT("G_3_03")) && SoloBody && HasTag(Card, TEXT("Echo")))
	{
		return false;
	}
	return true;
}

TArray<FReEchoCardDefinition> ReEchoCardRuntime::BuildOfferPool(const FReEchoCardCatalog& Catalog,
                                                                const FReEchoCardBuildState& State,
                                                                const FName OfferGroup,
                                                                const int32 Tier)
{
	TArray<FReEchoCardDefinition> Result;
	for (const FReEchoCardDefinition& Card : Catalog.GetOfferable(OfferGroup, Tier))
	{
		if (CanOffer(Catalog, State, Card))
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
	TFunction<bool(FName, bool)> GrantSingle;
	GrantSingle = [&](const FName RequestedCardId, const bool bRecordOwnership)
	{
		const FReEchoCardDefinition* Card = Catalog.Find(RequestedCardId);
		if (!Card || !Card->bEnabled || (bRecordOwnership && !CanOffer(Catalog, Result.CardState, *Card)) ||
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
					    BuildOfferPool(Catalog, Result.CardState, TEXT("Trait"), RequiredTier);
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
					if (!GrantSingle(GrantedCardId, true))
					{
						return false;
					}
					FReEchoCardOutcomeState& Outcome =
					    FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::GrantedCards);
					Outcome.RelatedCardIds.Add(GrantedCardId);
					++Outcome.ResolutionCount;
				}
			}
		}
		return true;
	};

	if (!GrantSingle(CardId, Input.bRecordOwnership))
	{
		const FString Error = Result.Error;
		Result = {};
		Result.Stats = Input.Stats;
		Result.CardState = Input.CardState;
		Result.TimeShards = Input.TimeShards;
		Result.Error = Error;
		return Result;
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
	Result.Runtime.ReactionHealCooldownRemaining = 0.0f;
	Result.Runtime.EncounterKillCount = 0;
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
	Result.RawDamage *= 1.0f + Rules.DistanceDamageBonusPerMeter * FMath::Max(0.0f, Input.DistanceCm / 100.0f);
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
	    [&](const FReEchoCardDefinition&, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
	    {
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
                                                                  const int32 TimeShards)
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
		    if (Effect.BehaviorId == TEXT("Card.ReactionHeal") &&
		        Result.CardState.Runtime.ReactionHealCooldownRemaining <= 0.0f)
		    {
			    Result.Healing += Effect.Value * StackCount;
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

FReEchoCardEventResult ReEchoCardRuntime::OnKillResolved(const FReEchoCardCatalog& Catalog,
                                                         const FReEchoCardBuildState& State,
                                                         const FReEchoStatBlock& Stats,
                                                         const bool bKilledByEcho)
{
	FReEchoCardEventResult Result;
	Result.CardState = State;
	Result.Stats = Stats;
	++Result.CardState.Runtime.EncounterKillCount;
	if (Result.CardState.Runtime.HuntTrackingEncounterIndex == State.Runtime.ActiveEncounterIndex)
	{
		++Result.CardState.Runtime.HuntKillCount;
	}
	ForEachOwnedEffect(
	    Catalog,
	    State,
	    TEXT("OnHitResolved"),
	    [&](const FReEchoCardDefinition& Card, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
	    {
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
	return Result;
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

FReEchoCardEventResult ReEchoCardRuntime::EndEncounter(const FReEchoCardCatalog& Catalog,
                                                       const FReEchoCardBuildState& State,
                                                       const FReEchoStatBlock& Stats,
                                                       const int32 EncounterIndex,
                                                       const int32 PlayerKillCount)
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
		    if (Effect.BehaviorId == TEXT("Card.EndKillRefresh"))
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
