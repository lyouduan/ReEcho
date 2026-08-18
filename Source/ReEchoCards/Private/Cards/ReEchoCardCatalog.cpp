#include "Cards/ReEchoCardCatalog.h"

namespace
{
const TSet<FName>& SupportedTriggers()
{
	static const TSet<FName> Values = {TEXT("OnApply"),
	                                   TEXT("OnGrant"),
	                                   TEXT("OnEncounterStart"),
	                                   TEXT("OnEncounterTick"),
	                                   TEXT("OnEncounterEnd"),
	                                   TEXT("BeforeOutgoingHit"),
	                                   TEXT("BeforeIncomingHit"),
	                                   TEXT("OnHitResolved"),
	                                   TEXT("OnReaction"),
	                                   TEXT("OnPurchase"),
	                                   TEXT("OnEchoKilled"),
	                                   TEXT("OnResolveEchoes"),
	                                   TEXT("OnCompileRules")};
	return Values;
}

const TSet<FName>& SupportedBehaviors()
{
	static const TSet<FName> Values = {TEXT("Card.StatModifier"),
	                                   TEXT("Card.InstantRecovery"),
	                                   TEXT("Card.GrantTier"),
	                                   TEXT("Card.RandomStatTrade"),
	                                   TEXT("Card.TrackNextKills"),
	                                   TEXT("Card.TrackNextReactions"),
	                                   TEXT("Card.EchoElementAura"),
	                                   TEXT("Card.EchoSlowAura"),
	                                   TEXT("Card.ElementAttachedCrit"),
	                                   TEXT("Card.ReactionHeal"),
	                                   TEXT("Card.DamageSubstitution"),
	                                   TEXT("Card.NextShardDrop"),
	                                   TEXT("Card.ShopContract"),
	                                   TEXT("Card.EncounterStun"),
	                                   TEXT("Card.DoubleEcho"),
	                                   TEXT("Card.TimeAnchor"),
	                                   TEXT("Card.SoloBody"),
	                                   TEXT("Card.TauntEcho"),
	                                   TEXT("Card.SoulResonance"),
	                                   TEXT("Card.EnemyImmunity"),
	                                   TEXT("Card.CriticalElement"),
	                                   TEXT("Card.ElementCanCrit"),
	                                   TEXT("Card.DistanceDamage"),
	                                   TEXT("Card.DoubleCritRoll"),
	                                   TEXT("Card.BloodForging"),
	                                   TEXT("Card.EndKillRefresh"),
	                                   TEXT("Card.HarvestPenalty"),
	                                   TEXT("Card.ShardOutgoingDamage"),
	                                   TEXT("Card.ShardIncomingBarrier"),
	                                   TEXT("Card.ReactionDiversity"),
	                                   TEXT("Card.DoubleNonCoreSlots")};
	return Values;
}
} // namespace

bool FReEchoCardCatalog::Initialize(const TArray<FReEchoCardDefinition>& Definitions,
                                    const FString& DomainRevision,
                                    FString& OutError)
{
	if (DomainRevision.IsEmpty())
	{
		OutError = TEXT("Card domain revision is empty");
		return false;
	}

	TMap<FName, FReEchoCardDefinition> CandidateCards;
	TArray<FName> CandidateOrder;
	for (const FReEchoCardDefinition& Definition : Definitions)
	{
		if (Definition.Id.IsNone() || CandidateCards.Contains(Definition.Id))
		{
			OutError = FString::Printf(TEXT("Card id is empty or duplicated: %s"), *Definition.Id.ToString());
			return false;
		}
		if (Definition.bOfferable && !Definition.bEnabled)
		{
			OutError = FString::Printf(TEXT("Offerable card is disabled: %s"), *Definition.Id.ToString());
			return false;
		}
		int32 PreviousOrder = INDEX_NONE;
		for (const FReEchoCardEffectDefinition& Effect : Definition.Effects)
		{
			if (Effect.Id.IsNone() || Effect.Order <= PreviousOrder || !IsSupportedTrigger(Effect.Trigger) ||
			    !IsSupportedBehavior(Effect.BehaviorId))
			{
				OutError = FString::Printf(TEXT("Card %s has an invalid effect definition"), *Definition.Id.ToString());
				return false;
			}
			PreviousOrder = Effect.Order;
		}
		if (Definition.bEnabled && Definition.Effects.IsEmpty())
		{
			OutError = FString::Printf(TEXT("Enabled card has no effects: %s"), *Definition.Id.ToString());
			return false;
		}
		CandidateOrder.Add(Definition.Id);
		CandidateCards.Add(Definition.Id, Definition);
	}

	Cards = MoveTemp(CandidateCards);
	CardOrder = MoveTemp(CandidateOrder);
	Revision = DomainRevision;
	OutError.Reset();
	return true;
}

const FReEchoCardDefinition* FReEchoCardCatalog::Find(const FName CardId) const
{
	return Cards.Find(CardId);
}

TArray<FReEchoCardDefinition> FReEchoCardCatalog::GetOfferable(const FName OfferGroup, const int32 Tier) const
{
	TArray<FReEchoCardDefinition> Result;
	for (const FName CardId : CardOrder)
	{
		const FReEchoCardDefinition* Card = Cards.Find(CardId);
		if (Card && Card->bEnabled && Card->bOfferable && Card->OfferGroup == OfferGroup &&
		    (Tier == INDEX_NONE || Card->Tier == Tier))
		{
			Result.Add(*Card);
		}
	}
	return Result;
}

bool FReEchoCardCatalog::IsSupportedTrigger(const FName Trigger)
{
	return SupportedTriggers().Contains(Trigger);
}

bool FReEchoCardCatalog::IsSupportedBehavior(const FName BehaviorId)
{
	return SupportedBehaviors().Contains(BehaviorId);
}
