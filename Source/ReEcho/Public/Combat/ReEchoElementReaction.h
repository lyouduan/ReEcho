#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"

struct REECHO_API FReEchoElementHitResult
{
	float Damage = 0.0f;
	float Multiplier = 1.0f;
	EReEchoElement PreviousElement = EReEchoElement::None;
	EReEchoElement IncomingElement = EReEchoElement::None;
	bool bTriggeredReaction = false;
};

/** Pure, deterministic elemental attachment and reaction rules shared by player and echo attacks. */
namespace ReEchoElementReaction
{
REECHO_API bool IsCombatElement(EReEchoElement Element);
REECHO_API bool IsDoubleDamagePair(EReEchoElement First, EReEchoElement Second);
REECHO_API FReEchoElementHitResult ResolveHit(FReEchoElementState& State,
                                              EReEchoElement IncomingElement,
                                              float BaseDamage,
                                              float ReactionEfficiency = 1.0f);
REECHO_API FLinearColor GetElementColor(EReEchoElement Element);
REECHO_API FString GetElementLabel(EReEchoElement Element);
}
