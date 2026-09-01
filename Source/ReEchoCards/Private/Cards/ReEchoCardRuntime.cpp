#include "Cards/ReEchoCardRuntime.h"

namespace
{
EReEchoHealthAdjustment MergeHealthAdjustments(const EReEchoHealthAdjustment Current,
                                               const EReEchoHealthAdjustment Incoming)
{
	if (Current == EReEchoHealthAdjustment::FillToMax || Incoming == EReEchoHealthAdjustment::FillToMax)
	{
		return EReEchoHealthAdjustment::FillToMax;
	}
	if (Current == EReEchoHealthAdjustment::SetToStatPoint || Incoming == EReEchoHealthAdjustment::SetToStatPoint)
	{
		return EReEchoHealthAdjustment::SetToStatPoint;
	}
	return EReEchoHealthAdjustment::None;
}

void RequestHealthAdjustment(EReEchoHealthAdjustment& Current, const EReEchoHealthAdjustment Incoming)
{
	Current = MergeHealthAdjustments(Current, Incoming);
}

bool IsCommittedHealthTarget(const FName Target)
{
	return Target == TEXT("HpMax") || Target == TEXT("HpPoint") || Target == TEXT("HpMaxAndPoint");
}

bool HasTag(const FReEchoCardDefinition& Card, const FName Tag)
{
	return Card.Tags.Contains(Tag);
}

bool HasAdditionalCardGrantEffect(const FReEchoCardDefinition& Card)
{
	for (const FReEchoCardEffectDefinition& Effect : Card.Effects)
	{
		if (Effect.BehaviorId == TEXT("Card.GrantTier") || Effect.BehaviorId == TEXT("Card.GrantAllTier1") ||
		    Effect.BehaviorId == TEXT("Card.CollectCoresGrantTiered") ||
		    Effect.BehaviorId == TEXT("Card.EasterShardThresholdGrantAllTier1") ||
		    Effect.BehaviorId == TEXT("Card.EasterDamageCards") ||
		    Effect.BehaviorId == TEXT("Card.EasterGrantRandomCards"))
		{
			return true;
		}
	}
	return false;
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
		Rules.bRetireEchoOnDefeat = true;
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
		else if (Effect.ParamName == TEXT("TargetCount"))
		{
			Rules.EasterRandomStunTargetCount =
			    FMath::Max(Rules.EasterRandomStunTargetCount, FMath::RoundToInt(Effect.Value));
		}
	}
	else if (Effect.BehaviorId == TEXT("Card.ExpectedOutcome") && Effect.Target == TEXT("EnemyAttackFlatBonus"))
	{
		Rules.EnemyAttackFlatBonus += StackedValue;
	}
	else if (Effect.BehaviorId == TEXT("Card.EasterExtraEchoes"))
	{
		// G_4_14 (他们像山一样): each copy adds one echo, capped at the authored maximum (7).
		Rules.MaximumEchoes = FMath::Min(FMath::Max(1, FMath::RoundToInt(Effect.ParamValue)),
		                                 Rules.MaximumEchoes + FMath::Max(1, StackCount));
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

bool ReEchoCardRuntime::IsRepeatableCard(const FReEchoCardDefinition& Card)
{
	return (Card.Tier == 1 && Card.StackPolicy != TEXT("Unique")) || Card.OfferGroup == TEXT("EasterEgg") ||
	       Card.Id == TEXT("G_3_23");
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
	// Egao Party: repeatable cards are exempt so they can be acquired again and again.
	if (HasCard(State, Card.Id) && (Card.Tier != 1 || Card.StackPolicy == TEXT("Unique")) &&
	    !IsRepeatableCard(Card))
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
	// G_4_3 doubles its Echo contact damage and healing after every completed encounter. Authored values
	// are compiled first, then scaled here where the saved per-run doubling progress is available.
	if (Rules.bEasterEchoContact && State.Runtime.EasterEchoContactScale > 1.0f)
	{
		Rules.EasterEchoContactDamage *= State.Runtime.EasterEchoContactScale;
		Rules.EasterEchoContactHealing *= State.Runtime.EasterEchoContactScale;
	}
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
	TFunction<bool(FName, bool, bool, bool)> GrantSingle;
	GrantSingle = [&](const FName RequestedCardId,
	                  const bool bRecordOwnership,
	                  const bool bForce,
	                  const bool bApplyCardPackPostEffect)
	{
		const FReEchoCardDefinition* Card = Catalog.Find(RequestedCardId);
		const bool bAlreadyGranted = GrantedThisTransaction.Contains(RequestedCardId);
		if (!Card || !Card->bEnabled ||
		    (bRecordOwnership && !bForce && !CanOffer(Catalog, Result.CardState, *Card, Input.EncounterIndex)) ||
		    (bAlreadyGranted && !IsRepeatableCard(*Card)))
		{
			Result.Error = FString::Printf(TEXT("Card cannot be granted: %s"), *RequestedCardId.ToString());
			return false;
		}
		GrantedThisTransaction.Add(RequestedCardId);
		if (bRecordOwnership)
		{
			Result.CardState.OwnedCardIds.Add(RequestedCardId);
			if (RequestedCardId == TEXT("G_4_6"))
			{
				Result.CardState.Runtime.bEasterDamageCardsGranted = false;
			}
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
				if (IsCommittedHealthTarget(Effect.Target))
				{
					RequestHealthAdjustment(Result.HealthAdjustment, EReEchoHealthAdjustment::SetToStatPoint);
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
					if (Effect.Target == TEXT("CritNegateAmplification"))
					{
						// Not a stat: G_4_2's downside lives in persistent run state, so it must not be routed
						// through ApplyStatEffect, which would reject the target and fail the whole grant.
						Result.CardState.Runtime.bEasterCritNegatesAmplification = true;
						Outcome.DetailTargets.Add(Effect.Target);
						Outcome.DetailValues.Add(Effect.Value);
					}
					else if (!ApplyStatEffect(Result.Stats, Effect))
					{
						Result.Error =
						    FString::Printf(TEXT("Unsupported Easter stat target: %s"), *Effect.Target.ToString());
						return false;
					}
					else
					{
						Outcome.DetailTargets.Add(Effect.Target);
						Outcome.DetailValues.Add(Effect.Value);
						if (Effect.Target == TEXT("HpPoint"))
						{
							RequestHealthAdjustment(Result.HealthAdjustment, EReEchoHealthAdjustment::SetToStatPoint);
						}
					}
				}
			}
			else if (Effect.BehaviorId == TEXT("Card.ExpectedOutcome") &&
			         Effect.Target == TEXT("PhysicalAndElementalAttack"))
			{
				Result.Stats.PhysicalAttack = FMath::Max(0.0f, Result.Stats.PhysicalAttack + Effect.Value);
				Result.Stats.ElementalAttack = FMath::Max(0.0f, Result.Stats.ElementalAttack + Effect.Value);
				FReEchoCardOutcomeState& Outcome =
				    FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::RandomDetails);
				Outcome.DetailTargets = {TEXT("PhysicalAttack"), TEXT("ElementalAttack"), TEXT("EnemyAttack")};
				Outcome.DetailValues = {Effect.Value, Effect.Value, Effect.Value};
				Outcome.ResolutionCount = 1;
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
						RequestHealthAdjustment(Result.HealthAdjustment, EReEchoHealthAdjustment::SetToStatPoint);
					}
					else if (!ApplyStatEffect(Result.Stats, Reward))
					{
						Result.Error =
						    FString::Printf(TEXT("Unsupported Easter sacrifice target: %s"), *Reward.Target.ToString());
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
			else if (Effect.BehaviorId == TEXT("Card.EasterGrantRandomCards"))
		{
			// G_4_11 (灵感の喷涌): exactly five random cards. Its global Egao bonus is deliberately
			// suppressed, and cards that themselves grant cards are excluded to prevent recursive chains.
			const int32 GrantCount = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
			TArray<FReEchoCardDefinition> Pool =
			    BuildOfferPool(Catalog, Result.CardState, TEXT("Trait"), INDEX_NONE, Input.EncounterIndex);
			Pool.RemoveAll(
			    [&](const FReEchoCardDefinition& Candidate)
			    {
				    return !Candidate.bEnabled || Candidate.Id == Card->Id || HasAdditionalCardGrantEffect(Candidate);
			    });
			FRandomStream Random(
			    HashCombine(GetTypeHash(Input.RandomSeed), GetTypeHash(Result.CardState.Runtime.RandomSequence++)));
			FReEchoCardOutcomeState& Outcome =
			    FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::GrantedCards);
			for (int32 Index = 0; Index < GrantCount && !Pool.IsEmpty(); ++Index)
			{
				const int32 Pick = Random.RandRange(0, Pool.Num() - 1);
				FReEchoCardGrantInput SubInput;
				SubInput.Stats = Result.Stats;
				SubInput.CardState = Result.CardState;
				SubInput.TimeShards = Result.TimeShards;
				SubInput.EncounterIndex = Input.EncounterIndex;
				SubInput.RandomSeed = HashCombine(GetTypeHash(Input.RandomSeed), GetTypeHash(Index));
				SubInput.bSuppressEgaoBonusGrant = true;
				const FReEchoCardGrantResult Grant = TryGrantCard(Catalog, Pool[Pick].Id, SubInput);
				if (!Grant.bSucceeded)
				{
					break;
				}
				Result.Stats = Grant.Stats;
				Result.CardState = Grant.CardState;
				Result.TimeShards = Grant.TimeShards;
				Outcome.RelatedCardIds.Append(Grant.GrantedCardIds);
				if (Pool[Pick].StackPolicy == TEXT("Unique"))
				{
					Pool.RemoveAtSwap(Pick);
				}
			}
			Outcome.ResolutionCount = Outcome.RelatedCardIds.Num();
		}
		else if (Effect.BehaviorId == TEXT("Card.EasterAscendInit"))
		{
			// G_4_13: acquisition resets both maximum and current health to one.
			Result.Stats.HpMax = 1.0f;
			Result.Stats.HpPoint = 1.0f;
			RequestHealthAdjustment(Result.HealthAdjustment, EReEchoHealthAdjustment::SetToStatPoint);
			FReEchoCardOutcomeState& Outcome =
			    FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::RandomDetails);
			Outcome.DetailTargets = {TEXT("HpPoint")};
			Outcome.DetailValues = {Effect.Value};
			Outcome.ResolutionCount = 1;
		}
		else if (Effect.BehaviorId == TEXT("Card.EasterReshuffleTier1"))
		{
			// G_4_15 (酱料派对): each owned tier-1 card is independently re-rolled into a normal
			// tier-1 card. Rebuild the materialized OnGrant stats as well as the owned-card ids.
			TArray<FReEchoCardDefinition> Tier1Cards = Catalog.GetAll(1);
			Tier1Cards.RemoveAll([&](const FReEchoCardDefinition& Candidate) { return !Candidate.bEnabled; });
			FReEchoCardOutcomeState& Outcome =
			    FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::RandomDetails);
			if (!Tier1Cards.IsEmpty())
			{
				auto ApplyTierOneGrantEffects = [&](const FReEchoCardDefinition& TierOneCard, const bool bRemove)
				{
					for (int32 EffectIndex = TierOneCard.Effects.Num() - 1; EffectIndex >= 0; --EffectIndex)
					{
						const FReEchoCardEffectDefinition& TierOneEffect = TierOneCard.Effects[EffectIndex];
						if ((TierOneEffect.Trigger != TEXT("OnGrant") && TierOneEffect.Trigger != TEXT("OnApply")) ||
						    (TierOneEffect.BehaviorId != TEXT("Card.StatModifier") &&
						     TierOneEffect.BehaviorId != TEXT("Card.InstantRecovery")))
						{
							continue;
						}
						FReEchoCardEffectDefinition Applied = TierOneEffect;
						if (bRemove)
						{
							if (Applied.Operation == EReEchoCardValueOperation::Add)
							{
								Applied.Value = -Applied.Value;
							}
							else if (Applied.Operation == EReEchoCardValueOperation::Multiply &&
							         !FMath::IsNearlyZero(Applied.Value))
							{
								Applied.Value = 1.0f / Applied.Value;
							}
							else
							{
								continue;
							}
						}
						if (!ApplyStatEffect(Result.Stats, Applied))
						{
							Result.Error = FString::Printf(TEXT("Unsupported tier-one reshuffle target: %s"),
							                              *Applied.Target.ToString());
							return false;
						}
						if (IsCommittedHealthTarget(Applied.Target))
						{
							RequestHealthAdjustment(Result.HealthAdjustment, EReEchoHealthAdjustment::SetToStatPoint);
						}
					}
					return true;
				};

				FRandomStream Random(HashCombine(GetTypeHash(Input.RandomSeed),
				                                     GetTypeHash(Result.CardState.Runtime.RandomSequence++)));
				TArray<FName> NewOwned;
				int32 Tier1Count = 0;
				for (const FName OwnedId : Result.CardState.OwnedCardIds)
				{
					const FReEchoCardDefinition* Owned = Catalog.Find(OwnedId);
					if (!Owned || Owned->Tier != 1 || !Owned->bEnabled)
					{
						NewOwned.Add(OwnedId);
						continue;
					}
					++Tier1Count;
					if (!ApplyTierOneGrantEffects(*Owned, true))
					{
						return false;
					}
				}
				if (Tier1Count > 0)
				{
					// Sauce Party chooses one tier-1 card once, then rebuilds exactly the old total count.
					const FReEchoCardDefinition& Replacement = Tier1Cards[Random.RandRange(0, Tier1Cards.Num() - 1)];
					for (int32 Index = 0; Index < Tier1Count; ++Index)
					{
						NewOwned.Add(Replacement.Id);
						if (!ApplyTierOneGrantEffects(Replacement, false))
						{
							return false;
						}
					}
				}
				Result.CardState.OwnedCardIds = MoveTemp(NewOwned);
				Outcome.DetailTargets = {TEXT("ReshuffledCount")};
				Outcome.DetailValues = {static_cast<float>(Tier1Count)};
				Outcome.ResolutionCount = 1;
			}
		}
		else if (Effect.BehaviorId == TEXT("Card.EasterResetToBase"))
		{
			// G_4_16 (有人自告奋勇): snapshot current stats as the base; each encounter then grows them.
			Result.CardState.Runtime.EasterBaseStatsSnapshot = Result.Stats;
			Result.CardState.Runtime.EasterResetGrowthCount = 0;
			FReEchoCardOutcomeState& Outcome =
			    FindOrAddOutcome(Result.CardState.Runtime, Card->Id, EReEchoCardOutcomeKind::RandomDetails);
			Outcome.ResolutionCount = 1;
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
				RequestHealthAdjustment(Result.HealthAdjustment, EReEchoHealthAdjustment::FillToMax);
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
				RequestHealthAdjustment(Result.HealthAdjustment, EReEchoHealthAdjustment::SetToStatPoint);
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
					if (!GrantSingle(GrantedCardId, true, false, false))
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
					if (GrantedThisTransaction.Contains(Tier1Card.Id) && !IsRepeatableCard(Tier1Card))
					{
						continue;
					}
					// bForce=true：绕过 CanOffer 的上架/冲突/遭遇限制，强制入袋。
					if (!GrantSingle(Tier1Card.Id, true, true, false))
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
		// G_4_13 (飞升的策划们): only a card selected from a card pack resets current health to 1.
		// Nested rewards (including G_4_11's five cards and G_3_23's tier-one cards) do not qualify.
		if (bApplyCardPackPostEffect && Input.bFromCardPackSelection && RequestedCardId != TEXT("G_4_13") &&
		    HasCard(Result.CardState, TEXT("G_4_13")) && Result.Stats.HpMax > 0.0f)
		{
			Result.Stats.HpMax = FMath::Max(1.0f, Result.Stats.HpMax * 2.0f);
			Result.Stats.HpPoint = FMath::Clamp(Result.Stats.HpPoint * 2.0f, 0.0f, Result.Stats.HpMax);
			RequestHealthAdjustment(Result.HealthAdjustment, EReEchoHealthAdjustment::SetToStatPoint);
		}
		return true;
	};

	if (!GrantSingle(CardId, Input.bRecordOwnership, Input.bForceGrant, true))
	{
		const FString Error = Result.Error;
		Result = {};
		Result.Stats = Input.Stats;
		Result.CardState = Input.CardState;
		Result.TimeShards = Input.TimeShards;
		Result.Error = Error;
		return Result;
	}

	// Egao Party: a direct acquisition also hands out 1-5 copies of 样样都通. Each system-given copy
	// is a real grant, so its GrantAllTier1 effect resolves; nested tier-one grants cannot recurse because
	// this outer bonus block is only reached once after the top-level transaction.
	{
		const FReEchoCardDefinition* BonusCard = Catalog.Find(TEXT("G_3_23"));
		const FReEchoCardDefinition* GrantedCard = Catalog.Find(CardId);
		if (BonusCard && BonusCard->bEnabled && GrantedCard && Input.bRecordOwnership &&
		    !Input.bSuppressEgaoBonusGrant && !HasAdditionalCardGrantEffect(*GrantedCard))
		{
			const uint32 BonusSeed =
			    HashCombine(GetTypeHash(Input.RandomSeed), GetTypeHash(BonusCard->Id));
			FRandomStream BonusRandom(static_cast<int32>(BonusSeed));
			const int32 BonusCount = BonusRandom.RandRange(1, 5);
			for (int32 BonusIndex = 0; BonusIndex < BonusCount; ++BonusIndex)
			{
				if (!GrantSingle(BonusCard->Id, true, true, false))
				{
					const FString Error = Result.Error;
					Result = {};
					Result.Stats = Input.Stats;
					Result.CardState = Input.CardState;
					Result.TimeShards = Input.TimeShards;
					Result.Error = Error;
					return Result;
				}
			}
		}
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

FReEchoCardEventResult ReEchoCardRuntime::ApplyEncounterStart(const FReEchoCardCatalog& Catalog,
                                                              const FReEchoCardBuildState& State,
                                                              const FReEchoStatBlock& Stats,
                                                              const int32 EncounterIndex)
{
	FReEchoCardEventResult Result;
	Result.CardState = State;
	Result.Stats = Stats;
	ForEachOwnedEffect(
		Catalog,
		State,
		TEXT("OnEncounterStart"),
		[&](const FReEchoCardDefinition& Card, const FReEchoCardEffectDefinition& Effect, const int32 StackCount)
		{
			if (Effect.BehaviorId == TEXT("Card.EasterStageBuff"))
			{
				// G_4_12 (水果派对): "at the start of level 8" hand out the authored physical/elemental
				// attack bonus. It is a one-shot boon, so it only fires the first time the run reaches the
				// configured stage; for multi-copy ownership the stack count scales the single payout.
				const int32 RequiredStage = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
				if (!Result.CardState.Runtime.bEasterStageAttackBuffClaimed && EncounterIndex >= RequiredStage)
				{
					Result.CardState.Runtime.bEasterStageAttackBuffClaimed = true;
					FReEchoCardEffectDefinition Applied = Effect;
					Applied.Value = Effect.Value * FMath::Max(1, StackCount);
					ApplyStatEffect(Result.Stats, Applied);
					AccumulateOutcome(Result.CardState.Runtime, Card.Id, Effect.Target, Applied.Value);
				}
			}
		});
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
			Result.EasterRandomStunTargetCount = FMath::Max(1, TickRules.EasterRandomStunTargetCount);
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
	// Weapon logic already settled one crit layer for physical hits, so unwind it here and re-apply the
	// final stack count below instead of compounding on top of an already multiplied damage value.
	const float PreCriticalDamage =
	    bWasCritical ? Result.RawDamage / FMath::Max(1.0f, 1.0f + CriticalEffect) : Result.RawDamage;

	// Allowed crit rolls. Element damage never crits until G_3_10 (元素会心) lifts that restriction, and
	// the physical first roll already happened in the weapon layer, so only the remainder is rolled here.
	const int32 RuntimeRollCount = Result.Element != EReEchoElement::None
	                                   ? (Rules.bElementDamageCanCrit ? Rules.CriticalRollCount : 0)
	                                   : FMath::Max(0, Rules.CriticalRollCount - 1);

	// Crits chain: G_3_12 (突破天际) lets a critical hit crit again, so every successful roll adds another
	// layer instead of stopping at the first success.
	int32 CriticalStacks = bWasCritical ? 1 : 0;
	for (int32 RollIndex = 0; RollIndex < RuntimeRollCount; ++RollIndex)
	{
		FRandomStream Random(
		    HashCombine(GetTypeHash(Input.RandomSeed), GetTypeHash(Result.CardState.Runtime.RandomSequence++)));
		if (Random.FRand() < FMath::Clamp(Input.CriticalRate, 0.0f, 1.0f))
		{
			++CriticalStacks;
		}
	}

	Result.bCritical = CriticalStacks > 0;
	float CriticalMultiplier = 1.0f;
	for (int32 StackIndex = 0; StackIndex < CriticalStacks; ++StackIndex)
	{
		CriticalMultiplier *= 1.0f + CriticalEffect;
	}
	// The attached-element crit bonus stays a single additive term, matching its pre-chain behaviour.
	if (Result.bCritical && Input.bTargetHasElement)
	{
		CriticalMultiplier += Rules.ElementAttachedCriticalEffectBonus;
	}
	Result.RawDamage = PreCriticalDamage * CriticalMultiplier;
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

	// G_4_2 downside: a critical hit loses every damage amplification - crit multiplier, distance,
	// proximity and alternating-source bonuses - collapsing back to the un-amplified damage. Placed
	// after the amplification chain but before BeforeOutgoingHit, so effects that overwrite damage
	// outright (such as the G_4_7 lottery) are still honoured.
	if (Result.bCritical && Result.CardState.Runtime.bEasterCritNegatesAmplification)
	{
		Result.RawDamage = PreCriticalDamage;
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
					    RequestHealthAdjustment(Result.HealthAdjustment, EReEchoHealthAdjustment::FillToMax);
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
	// Egao Party: repeatable cards are still filtered by the current page's history so one page
	// cannot show the same card twice side by side.
	NormalPool.RemoveAll(
	    [&](const FReEchoCardDefinition& Candidate)
	    {
		    // 与 CanOffer 的拥有判断保持一致：排除历史卡、以及“拥有即不可再发”的卡
		    // （二阶/三阶已拥有、或一阶 Unique 已拥有）；但保留“一阶可叠加卡已拥有”的情况，
		    // 使其仍可作为重复发牌/刷新的目标（叠加）。
		    if (IsRepeatableCard(Candidate))
		    {
			    return OfferHistory.Contains(Candidate.Id);
		    }
		    return OfferHistory.Contains(Candidate.Id) ||
		           (HasCard(State, Candidate.Id) &&
		            (Candidate.Tier != 1 || Candidate.StackPolicy == TEXT("Unique") || bExcludeOwnedNormalCards));
	    });
	EasterPool.RemoveAll(
	    [&](const FReEchoCardDefinition& Candidate)
	    {
		    return OfferHistory.Contains(Candidate.Id) ||
		           (!IsRepeatableCard(Candidate) && HasCard(State, Candidate.Id));
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
			    RequestHealthAdjustment(Result.HealthAdjustment, EReEchoHealthAdjustment::SetToStatPoint);
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
			    RequestHealthAdjustment(Result.HealthAdjustment, EReEchoHealthAdjustment::SetToStatPoint);
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
		    else if (Effect.BehaviorId == TEXT("Card.CollectCoresGrantTiered") &&
		             !Result.CardState.Runtime.bDragonSoulCompleted)
		    {
			    bGrantTieredCards = true;
			    TieredCardId = Card.Id;
			    TieredRequiredCores = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
		    }
	    });
	if (bGrantTieredCards && DistinctOwnedCoreCount >= TieredRequiredCores &&
	    !Result.CardState.Runtime.bDragonSoulCompleted)
	{
		for (int32 Tier = 1; Tier <= 3; ++Tier)
		{
			// BuildOfferPool already applies the canonical ownership rule: tier-one Stackable cards
			// remain eligible for repeat grants, while owned higher-tier and Unique cards are excluded.
			TArray<FReEchoCardDefinition> Pool = BuildOfferPool(
			    Catalog, Result.CardState, TEXT("Trait"), Tier, Result.CardState.Runtime.ActiveEncounterIndex);
			if (Pool.IsEmpty())
			{
				continue;
			}
			FRandomStream Random(
			    HashCombine(GetTypeHash(Result.CardState.Runtime.RandomSequence++), GetTypeHash(Tier)));
			const FName GrantedCardId = Pool[Random.RandRange(0, Pool.Num() - 1)].Id;
			FReEchoCardGrantInput GrantInput;
			GrantInput.Stats = Result.Stats;
			GrantInput.CardState = Result.CardState;
			GrantInput.TimeShards = 0;
			GrantInput.EncounterIndex = Result.CardState.Runtime.ActiveEncounterIndex;
			GrantInput.RandomSeed =
			    HashCombine(GetTypeHash(Result.CardState.Runtime.RandomSequence++), GetTypeHash(GrantedCardId));
			GrantInput.bRecordOwnership = true;
			const FReEchoCardGrantResult Grant = ReEchoCardRuntime::TryGrantCard(Catalog, GrantedCardId, GrantInput);
			if (Grant.bSucceeded)
			{
				Result.Stats = Grant.Stats;
				Result.CardState = Grant.CardState;
				RequestHealthAdjustment(Result.HealthAdjustment, Grant.HealthAdjustment);
				FReEchoCardOutcomeState& Outcome =
				    FindOrAddOutcome(Result.CardState.Runtime, TieredCardId, EReEchoCardOutcomeKind::GrantedCards);
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
	if (AppliedDamage <= 0.0f || !HasCard(Result.CardState, TEXT("G_4_6")))
	{
		return Result;
	}
	// G_4_6 only records the damage taken during this encounter. EndEncounter converts the running total
	// into one tier-1 card per point of damage, so nothing is granted at the moment damage lands.
	Result.CardState.Runtime.EasterDamageTaken += AppliedDamage;
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
	// G_4_3 doubles its Echo contact damage and healing once per completed encounter, so the rule
	// compiled for the next encounter uses the enlarged values.
	if (HasCard(Result.CardState, TEXT("G_4_3")))
	{
		Result.CardState.Runtime.EasterEchoContactScale *= 2.0f;
	}
	// G_4_6 pays once at the end of the next completed encounter after acquisition:
	// one random tier-1 card per point of damage taken during that encounter.
	if (HasCard(Result.CardState, TEXT("G_4_6")) && !Result.CardState.Runtime.bEasterDamageCardsGranted)
	{
		Result.CardState.Runtime.bEasterDamageCardsGranted = true;
		const int32 StackCount = FMath::Max(1, CountOwned(Result.CardState, TEXT("G_4_6")));
		const int32 Requested = FMath::Max(
		    0, FMath::RoundToInt(Result.CardState.Runtime.EasterDamageTaken * StackCount));
		Result.CardState.Runtime.EasterDamageTaken = 0.0f;
		TArray<FReEchoCardDefinition> Pool = Catalog.GetAll(1);
		Pool.RemoveAll([&](const FReEchoCardDefinition& Candidate) { return !Candidate.bEnabled; });
		FRandomStream Random(
		    HashCombine(GetTypeHash(RandomSeed), GetTypeHash(Result.CardState.Runtime.RandomSequence++)));
		TArray<FName> GrantedRewardIds;
		for (int32 Index = 0; Index < Requested && !Pool.IsEmpty(); ++Index)
		{
			const FReEchoCardDefinition& RewardCard = Pool[Random.RandRange(0, Pool.Num() - 1)];
			FReEchoCardGrantInput Input;
			Input.Stats = Result.Stats;
			Input.CardState = Result.CardState;
			Input.TimeShards = TimeShards;
			Input.EncounterIndex = EncounterIndex;
			Input.RandomSeed = HashCombine(GetTypeHash(RandomSeed), GetTypeHash(Index));
			Input.bForceGrant = true;
			Input.bSuppressEgaoBonusGrant = true;
			const FReEchoCardGrantResult Grant = TryGrantCard(Catalog, RewardCard.Id, Input);
			if (!Grant.bSucceeded)
			{
				break;
			}
			Result.Stats = Grant.Stats;
			Result.CardState = Grant.CardState;
			RequestHealthAdjustment(Result.HealthAdjustment, Grant.HealthAdjustment);
			GrantedRewardIds.Append(Grant.GrantedCardIds);
		}
		FReEchoCardOutcomeState& Outcome =
		    FindOrAddOutcome(Result.CardState.Runtime, TEXT("G_4_6"), EReEchoCardOutcomeKind::GrantedCards);
		Outcome.RelatedCardIds.Append(GrantedRewardIds);
		Outcome.ResolutionCount = Outcome.RelatedCardIds.Num();
	}
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
		    else if (Effect.BehaviorId == TEXT("Card.EasterShardThresholdGrantAllTier1") && Effect.Order == 1)
		    {
		    	// G_4_1: once the settled shard balance crosses the threshold, hand out Copies sets of every
		    	// enabled tier-1 card (the 样样都通 boon). One-shot; checked after the swing sees the balance.
		    	const int32 Threshold = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
		    	const int32 Copies = FMath::Max(1, FMath::RoundToInt(Effect.Value));
		    	if (!Result.CardState.Runtime.bEasterShardBoonGranted && Result.ProjectedTimeShards >= Threshold)
		    	{
		    		Result.CardState.Runtime.bEasterShardBoonGranted = true;
		    		TArray<FReEchoCardDefinition> Tier1Cards = Catalog.GetAll(1);
		    		Tier1Cards.RemoveAll([&](const FReEchoCardDefinition& C) { return !C.bEnabled; });
		    		FRandomStream Random(HashCombine(GetTypeHash(RandomSeed),
		    		                                 GetTypeHash(Result.CardState.Runtime.RandomSequence++)));
		    		FReEchoCardOutcomeState& Outcome =
		    		    FindOrAddOutcome(Result.CardState.Runtime, Card.Id, EReEchoCardOutcomeKind::GrantedCards);
		    		for (int32 Copy = 0; Copy < Copies && !Tier1Cards.IsEmpty(); ++Copy)
		    		{
		    			for (const FReEchoCardDefinition& Tier1Card : Tier1Cards)
		    			{
		    				FReEchoCardGrantInput SubInput;
		    				SubInput.Stats = Result.Stats;
		    				SubInput.CardState = Result.CardState;
		    				SubInput.TimeShards = TimeShards;
		    				SubInput.EncounterIndex = EncounterIndex;
		    				SubInput.RandomSeed = HashCombine(GetTypeHash(RandomSeed), GetTypeHash(Copy * 1009 + Outcome.RelatedCardIds.Num()));
		    				const FReEchoCardGrantResult Grant = TryGrantCard(Catalog, Tier1Card.Id, SubInput);
		    				if (Grant.bSucceeded)
		    				{
		    					Result.Stats = Grant.Stats;
		    					Result.CardState = Grant.CardState;
		    					Outcome.RelatedCardIds.Append(Grant.GrantedCardIds);
		    				}
		    			}
		    		}
		    		Outcome.ResolutionCount = Outcome.RelatedCardIds.Num();
		    	}
		    }
		    else if (Effect.BehaviorId == TEXT("Card.EasterShardComparison") && Effect.Order == 1)
		    {
			    float NextMultiplier = 1.0f;
			    if (Result.CardState.Runtime.bHasPreviousEncounterShardIncome)
			    {
				    const int32 Delta =
				        GrossTimeShardIncome - Result.CardState.Runtime.PreviousEncounterGrossShardIncome;
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
			    // G_4_9 rolls every stat independently between the configured minimum and Max.
			    const float Maximum = FMath::Max(Effect.Value, Effect.ParamValue);
			    FReEchoCardEffectDefinition Rolled = Effect;
			    if (Maximum > Effect.Value)
			    {
				    FRandomStream Random(
				        HashCombine(GetTypeHash(RandomSeed), GetTypeHash(Result.CardState.Runtime.RandomSequence++)));
				    Rolled.Value = FMath::RoundToInt(FMath::FRandRange(Effect.Value, Maximum));
			    }
			    if (Rolled.Target == TEXT("HpMaxAndPoint"))
			    {
				    Result.Stats.HpMax += Rolled.Value;
				    Result.Stats.HpPoint += Rolled.Value;
				    RequestHealthAdjustment(Result.HealthAdjustment, EReEchoHealthAdjustment::SetToStatPoint);
			    }
			    else
			    {
				    ApplyStatEffect(Result.Stats, Rolled);
			    }
			    AccumulateOutcome(Result.CardState.Runtime, Card.Id, Rolled.Target, Rolled.Value);
		    }
		    else if (Effect.BehaviorId == TEXT("Card.EasterGrow30"))
		    {
			    // G_4_16 (有人自告奋勇): each encounter grows the snapshotted base stats by the factor.
			    Result.CardState.Runtime.EasterResetGrowthCount++;
			    const float Scale = FMath::Pow(FMath::Max(0.0f, Effect.Value), Result.CardState.Runtime.EasterResetGrowthCount);
			    const FReEchoStatBlock& Base = Result.CardState.Runtime.EasterBaseStatsSnapshot;
			    Result.Stats.HpMax = Base.HpMax * Scale;
			    Result.Stats.HpPoint = Base.HpPoint * Scale;
			    Result.Stats.PhysicalAttack = Base.PhysicalAttack * Scale;
			    Result.Stats.ElementalAttack = Base.ElementalAttack * Scale;
			    Result.Stats.AttackSpeed = Base.AttackSpeed * Scale;
			    Result.Stats.MovementSpeed = Base.MovementSpeed * Scale;
			    Result.Stats.CriticalRate = Base.CriticalRate * Scale;
			    Result.Stats.CriticalEffect = Base.CriticalEffect * Scale;
			    Result.Stats.EchoEfficiency = Base.EchoEfficiency * Scale;
			    Result.Stats.ReactionEfficiency = Base.ReactionEfficiency * Scale;
			    RequestHealthAdjustment(Result.HealthAdjustment, EReEchoHealthAdjustment::SetToStatPoint);
			    AccumulateOutcome(Result.CardState.Runtime, Card.Id, TEXT("AllBaseStats"), Scale);
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
			    const int32 ShardCount =
			        FMath::RoundToInt(GrossTimeShardIncome * RatePerKill * TotalKills * StackCount);
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
