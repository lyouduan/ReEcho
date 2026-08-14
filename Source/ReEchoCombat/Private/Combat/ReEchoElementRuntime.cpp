#include "Combat/ReEchoElementRuntime.h"

#include "Misc/ScopeLock.h"

namespace
{
FCriticalSection RuleSetCriticalSection;
TSharedPtr<const FReEchoElementRuleSet> PublishedRuleSet;
const FName ElementalImmunityStatusId = TEXT("Z_Elemental_Immunity");
const FName StatusNone = TEXT("None");

bool ShouldApplyElementalImmunity(const FName BehaviorId)
{
	return BehaviorId == TEXT("Reaction.Burn") || BehaviorId == TEXT("Reaction.Vaporize") ||
	       BehaviorId == TEXT("Reaction.Conduct");
}
} // namespace

const FReEchoElementRuleDefinition* FReEchoElementRuleSet::FindElement(const EReEchoElement Element) const
{
	return Elements.Find(Element);
}

const FReEchoReactionRuleDefinition* FReEchoElementRuleSet::FindReaction(const FName TriggerElementId,
                                                                         const FName AttachmentElementId) const
{
	for (const TPair<FName, FReEchoReactionRuleDefinition>& Pair : Reactions)
	{
		const FReEchoReactionRuleDefinition& Reaction = Pair.Value;
		if (Reaction.bEnabled && Reaction.TriggerElementId == TriggerElementId &&
		    Reaction.AttachmentElementId == AttachmentElementId)
		{
			return &Reaction;
		}
	}
	return nullptr;
}

float FReEchoElementRuleSet::GetStatusDuration(const FName StatusId) const
{
	const FReEchoStatusRuleDefinition* Status = Statuses.Find(StatusId);
	return Status && Status->bEnabled ? Status->DurationSeconds : 0.0f;
}

void ReEchoElementRuntime::PublishRuleSet(TSharedRef<const FReEchoElementRuleSet> RuleSet)
{
	FScopeLock Lock(&RuleSetCriticalSection);
	PublishedRuleSet = RuleSet;
}

void ReEchoElementRuntime::ClearRuleSetForTests()
{
	FScopeLock Lock(&RuleSetCriticalSection);
	PublishedRuleSet.Reset();
}

TSharedPtr<const FReEchoElementRuleSet> ReEchoElementRuntime::GetRuleSet()
{
	FScopeLock Lock(&RuleSetCriticalSection);
	return PublishedRuleSet;
}

bool ReEchoElementRuntime::IsCombatElement(const EReEchoElement Element)
{
	const TSharedPtr<const FReEchoElementRuleSet> Rules = GetRuleSet();
	const FReEchoElementRuleDefinition* Rule = Rules.IsValid() ? Rules->FindElement(Element) : nullptr;
	return Rule && Rule->bEnabled;
}

bool ReEchoElementRuntime::IsDoubleDamagePair(const EReEchoElement First, const EReEchoElement Second)
{
	const TSharedPtr<const FReEchoElementRuleSet> Rules = GetRuleSet();
	const FReEchoElementRuleDefinition* FirstRule = Rules.IsValid() ? Rules->FindElement(First) : nullptr;
	const FReEchoElementRuleDefinition* SecondRule = Rules.IsValid() ? Rules->FindElement(Second) : nullptr;
	return Rules.IsValid() && FirstRule && SecondRule && First != Second &&
	       (Rules->FindReaction(FirstRule->ElementId, SecondRule->ElementId) ||
	        Rules->FindReaction(SecondRule->ElementId, FirstRule->ElementId));
}

FReEchoElementHitResult ReEchoElementRuntime::ResolveHit(FReEchoElementState& State,
                                                         const EReEchoElement IncomingElement,
                                                         const float BaseDamage,
                                                         const float ReactionEfficiency,
                                                         const float CurrentTimeSeconds)
{
	FReEchoElementHitResult Result;
	Result.Damage = FMath::Max(0.0f, BaseDamage);
	Result.PreviousElement = State.Attached;
	Result.IncomingElement = IncomingElement;
	const TSharedPtr<const FReEchoElementRuleSet> Rules = GetRuleSet();
	const FReEchoElementRuleDefinition* IncomingRule = Rules.IsValid() ? Rules->FindElement(IncomingElement) : nullptr;
	if (!Rules.IsValid() || !IncomingRule || !IncomingRule->bEnabled)
	{
		return Result;
	}
	if ((CurrentTimeSeconds >= 0.0f && State.ImmunityUntil > CurrentTimeSeconds) ||
	    State.BlockedAttachment == IncomingElement)
	{
		Result.bBlockedByImmunity = true;
		return Result;
	}

	const FReEchoElementRuleDefinition* AttachedRule = Rules->FindElement(State.Attached);
	const FReEchoReactionRuleDefinition* Reaction =
	    AttachedRule && AttachedRule->bEnabled ? Rules->FindReaction(IncomingRule->ElementId, AttachedRule->ElementId)
	                                           : nullptr;
	if (!Reaction)
	{
		if (IncomingRule->bAttachment)
		{
			State.Attached = IncomingElement;
		}
		return Result;
	}

	const bool bEnhance = Reaction->FormulaId == TEXT("Element.EnhanceNextReaction");
	Result.bTriggeredReaction = true;
	Result.ReactionId = Reaction->ReactionId;
	Result.ReactionBehaviorId = Reaction->BehaviorId;
	Result.FormulaId = Reaction->FormulaId;
	Result.RadiusCm = Reaction->RadiusCm;
	Result.AppliedStatusId = Reaction->StatusId == StatusNone ? NAME_None : Reaction->StatusId;
	Result.StatusDurationSeconds = Reaction->StatusDurationSeconds;
	Result.EnhancementMultiplier = Reaction->EnhancementMultiplier;
	Result.bCanCrit = Reaction->bCanCrit;
	Result.bAffectedByEchoEfficiency = Reaction->bAffectedByEchoEfficiency;

	if (State.bEnhancedNextReaction && !bEnhance)
	{
		Result.bAppliedEnhancement = true;
		Result.EnhancementMultiplier = FMath::Max(1.0f, State.EnhancementMultiplier);
		State.bEnhancedNextReaction = false;
		State.EnhancementMultiplier = 1.0f;
		State.BlockedAttachment = EReEchoElement::None;
		Result.bClearedAttachmentBlock = true;
	}
	Result.Damage = 0.0f;
	if (bEnhance)
	{
		State.bEnhancedNextReaction = true;
		State.EnhancementMultiplier = FMath::Max(State.EnhancementMultiplier, Reaction->EnhancementMultiplier);
		State.BlockedAttachment = State.Attached;
	}
	else
	{
		Result.Multiplier = FMath::Max(0.0f, ReactionEfficiency) *
		                    (Result.bAppliedEnhancement ? FMath::Max(1.0f, Result.EnhancementMultiplier) : 1.0f);
	}
	if (ShouldApplyElementalImmunity(Reaction->BehaviorId))
	{
		Result.ImmunityDurationSeconds = Rules->GetStatusDuration(ElementalImmunityStatusId);
		if (CurrentTimeSeconds >= 0.0f && Result.ImmunityDurationSeconds > 0.0f)
		{
			State.ImmunityUntil = CurrentTimeSeconds + Result.ImmunityDurationSeconds;
		}
	}
	if (Reaction->bClearsAttachment)
	{
		State.Attached = EReEchoElement::None;
	}
	return Result;
}
