#pragma once

#include "Combat/ReEchoElementRuntime.h"
#include "Combat/ReEchoCombatTypes.h"

struct REECHOCOMBAT_API FReEchoElementHitContext
{
	FVector SourceLocation = FVector::ZeroVector;
	FReEchoAttackIdentity Attack;
	float ReactionEfficiency = 1.0f;
	float SourceElementalAttack = 0.0f;
	float SourceEchoEfficiency = 1.0f;
};

struct REECHOCOMBAT_API FReEchoElementExecutionResult
{
	FReEchoElementHitResult Primary;
	float ImmediateDamageApplied = 0.0f;
	int32 DotTicksScheduled = 0;
	TArray<float> DotTickDelaySeconds;
	TArray<TWeakObjectPtr<AActor>> AffectedTargets;
};

namespace ReEchoHitResolver
{
/** Single physical-hit adjudication entry. Weapons and presentation must not apply health directly. */
REECHOCOMBAT_API FReEchoHitResolved ResolvePhysicalHit(const FReEchoHitIntent& Intent);
/** Unified entry: dispatches physical or elemental adjudication without a presentation callback. */
REECHOCOMBAT_API FReEchoHitResolved ResolveHit(const FReEchoHitIntent& Intent);
/** Elemental attachment/reaction orchestration over generic combat targets. */
REECHOCOMBAT_API FReEchoElementExecutionResult ResolveElementHit(AActor& Target,
                                                                 EReEchoElement IncomingElement,
                                                                 float BaseDamage,
                                                                 const FReEchoElementHitContext& Context);
REECHOCOMBAT_API int32 TickElementStatuses(AActor& Target, float CurrentTimeSeconds);
}
