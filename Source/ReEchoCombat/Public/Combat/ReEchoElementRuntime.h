#pragma once

#include "Combat/ReEchoCombatTypes.h"

struct REECHOCOMBAT_API FReEchoElementRuleDefinition
{
	FName ElementId = NAME_None;
	EReEchoElement Element = EReEchoElement::None;
	bool bAttachment = false;
	bool bEnabled = false;
};

struct REECHOCOMBAT_API FReEchoStatusRuleDefinition
{
	FName StatusId = NAME_None;
	float DurationSeconds = 0.0f;
	bool bEnabled = false;
};

struct REECHOCOMBAT_API FReEchoReactionRuleDefinition
{
	FName ReactionId = NAME_None;
	FName TriggerElementId = NAME_None;
	FName AttachmentElementId = NAME_None;
	FName BehaviorId = NAME_None;
	FName FormulaId = NAME_None;
	float DamageMultiplier = 1.0f;
	float DamageIncrease = 0.0f;
	float RadiusCm = 0.0f;
	FName StatusId = NAME_None;
	float StatusDurationSeconds = 0.0f;
	float EnhancementMultiplier = 1.0f;
	bool bCanCrit = false;
	bool bAffectedByEchoEfficiency = false;
	bool bClearsAttachment = true;
	bool bEnabled = false;
};

struct REECHOCOMBAT_API FReEchoElementRuleSet
{
	TMap<EReEchoElement, FReEchoElementRuleDefinition> Elements;
	TMap<FName, FReEchoReactionRuleDefinition> Reactions;
	TMap<FName, FReEchoStatusRuleDefinition> Statuses;

	const FReEchoElementRuleDefinition* FindElement(EReEchoElement Element) const;
	const FReEchoReactionRuleDefinition* FindReaction(FName TriggerElementId, FName AttachmentElementId) const;
	float GetStatusDuration(FName StatusId) const;
};

struct REECHOCOMBAT_API FReEchoElementHitResult
{
	float Damage = 0.0f;
	float Multiplier = 1.0f;
	float RadiusCm = 0.0f;
	float StatusDurationSeconds = 0.0f;
	float ImmunityDurationSeconds = 0.0f;
	float EnhancementMultiplier = 1.0f;
	FName ReactionId;
	FName ReactionBehaviorId;
	FName FormulaId;
	FName AppliedStatusId;
	EReEchoElement PreviousElement = EReEchoElement::None;
	EReEchoElement IncomingElement = EReEchoElement::None;
	bool bTriggeredReaction = false;
	bool bBlockedByImmunity = false;
	bool bAppliedEnhancement = false;
	bool bClearedAttachmentBlock = false;
	bool bCanCrit = false;
	bool bAffectedByEchoEfficiency = false;
};

namespace ReEchoElementRuntime
{
REECHOCOMBAT_API void PublishRuleSet(TSharedRef<const FReEchoElementRuleSet> RuleSet);
REECHOCOMBAT_API void ClearRuleSetForTests();
REECHOCOMBAT_API TSharedPtr<const FReEchoElementRuleSet> GetRuleSet();
REECHOCOMBAT_API bool IsCombatElement(EReEchoElement Element);
REECHOCOMBAT_API bool IsDoubleDamagePair(EReEchoElement First, EReEchoElement Second);
REECHOCOMBAT_API FReEchoElementHitResult ResolveHit(FReEchoElementState& State,
                                                    EReEchoElement IncomingElement,
                                                    float BaseDamage,
                                                    float ReactionEfficiency = 1.0f,
                                                    float CurrentTimeSeconds = -1.0f);
}
