#include "Combat/ReEchoElementReaction.h"

namespace ReEchoElementReaction
{
bool IsCombatElement(const EReEchoElement Element)
{
	return Element == EReEchoElement::Water || Element == EReEchoElement::Flame || Element == EReEchoElement::Grass;
}

bool IsDoubleDamagePair(const EReEchoElement First, const EReEchoElement Second)
{
	if (!IsCombatElement(First) || !IsCombatElement(Second) || First == Second)
	{
		return false;
	}
	const bool bWaterGrass = (First == EReEchoElement::Water && Second == EReEchoElement::Grass) ||
	                         (First == EReEchoElement::Grass && Second == EReEchoElement::Water);
	const bool bGrassFlame = (First == EReEchoElement::Grass && Second == EReEchoElement::Flame) ||
	                         (First == EReEchoElement::Flame && Second == EReEchoElement::Grass);
	return bWaterGrass || bGrassFlame;
}

FReEchoElementHitResult ResolveHit(FReEchoElementState& State,
                                   const EReEchoElement IncomingElement,
                                   const float BaseDamage,
                                   const float ReactionEfficiency)
{
	FReEchoElementHitResult Result;
	Result.Damage = FMath::Max(0.0f, BaseDamage);
	Result.PreviousElement = State.Attached;
	Result.IncomingElement = IncomingElement;
	if (!IsCombatElement(IncomingElement))
	{
		return Result;
	}

	Result.bTriggeredReaction = IsDoubleDamagePair(State.Attached, IncomingElement);
	if (Result.bTriggeredReaction)
	{
		Result.Multiplier = 2.0f * FMath::Max(0.0f, ReactionEfficiency);
		Result.Damage *= Result.Multiplier;
		State.Attached = EReEchoElement::None;
	}
	else
	{
		// Same-element hits refresh the attachment; unsupported pairs replace it deterministically.
		State.Attached = IncomingElement;
	}
	return Result;
}

FString GetElementLabel(const EReEchoElement Element)
{
	switch (Element)
	{
		case EReEchoElement::Water:
			return TEXT("WATER");
		case EReEchoElement::Flame:
			return TEXT("FIRE");
		case EReEchoElement::Grass:
			return TEXT("GRASS");
		default:
			return TEXT("NONE");
	}
}

FLinearColor GetElementColor(const EReEchoElement Element)
{
	switch (Element)
	{
		case EReEchoElement::Water:
			return FLinearColor(0.08f, 0.42f, 1.0f, 1.0f);
		case EReEchoElement::Flame:
			return FLinearColor(1.0f, 0.12f, 0.025f, 1.0f);
		case EReEchoElement::Grass:
			return FLinearColor(0.16f, 0.85f, 0.20f, 1.0f);
		default:
			return FLinearColor::White;
	}
}
}
