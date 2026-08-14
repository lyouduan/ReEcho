#pragma once

#include "CoreMinimal.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoElementRuntime.h"
#include "Combat/ReEchoHitResolver.h"

class AReEchoEnemyActor;

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
REECHO_API int32 TickElementStatuses(AReEchoEnemyActor& Target, float CurrentTimeSeconds);
REECHO_API FLinearColor GetElementColor(EReEchoElement Element);
REECHO_API FString GetElementLabel(EReEchoElement Element);
REECHO_API FName GetElementId(EReEchoElement Element);
}
