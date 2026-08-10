#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"

class AReEchoEnemyActor;

struct REECHO_API FReEchoElementHitResult
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

struct REECHO_API FReEchoElementHitContext
{
	FVector SourceLocation = FVector::ZeroVector;
	TWeakObjectPtr<AActor> SourceActor;
	float ReactionEfficiency = 1.0f;
	float SourceElementalAttack = 0.0f;
	float SourceEchoEfficiency = 1.0f;
};

struct REECHO_API FReEchoElementExecutionResult
{
	FReEchoElementHitResult Primary;
	float ImmediateDamageApplied = 0.0f;
	int32 DotTicksScheduled = 0;
	TArray<float> DotTickDelaySeconds;
	TArray<TWeakObjectPtr<AReEchoEnemyActor>> AffectedTargets;
};

/** Pure, deterministic elemental attachment and reaction rules shared by player and echo attacks. */
namespace ReEchoElementReaction
{
REECHO_API bool IsCombatElement(EReEchoElement Element);
REECHO_API bool IsDoubleDamagePair(EReEchoElement First, EReEchoElement Second);
REECHO_API FReEchoElementHitResult ResolveHit(FReEchoElementState& State,
                                              EReEchoElement IncomingElement,
                                              float BaseDamage,
                                              float ReactionEfficiency = 1.0f,
                                              float CurrentTimeSeconds = -1.0f);
REECHO_API FReEchoElementExecutionResult ApplyHitToWorld(AReEchoEnemyActor& Target,
                                                         EReEchoElement IncomingElement,
                                                         float BaseDamage,
                                                         const FReEchoElementHitContext& Context);
REECHO_API FLinearColor GetElementColor(EReEchoElement Element);
REECHO_API FString GetElementLabel(EReEchoElement Element);
REECHO_API FName GetElementId(EReEchoElement Element);
}
