#include "Combat/ReEchoElementReaction.h"

#include "Combat/ReEchoCombatContracts.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Graybox/ReEchoEnemyActor.h"

namespace ReEchoElementReaction
{
namespace
{
const FReEchoCsvElementRow* FindElementRow(const EReEchoElement Element)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	return Snapshot.IsValid() ? Snapshot->FindElement(Element) : nullptr;
}

uint8 ParseHexPair(const FString& Text, const int32 Index)
{
	return static_cast<uint8>(FCString::Strtoi(*Text.Mid(Index, 2), nullptr, 16));
}

FLinearColor ParseColorHex(const FString& ColorHex)
{
	if (!ColorHex.StartsWith(TEXT("#")) || ColorHex.Len() != 7)
	{
		return FLinearColor::White;
	}
	return FLinearColor::FromSRGBColor(
	    FColor(ParseHexPair(ColorHex, 1), ParseHexPair(ColorHex, 3), ParseHexPair(ColorHex, 5)));
}

FLinearColor FromReferenceSrgb(const uint8 Red, const uint8 Green, const uint8 Blue)
{
	return FLinearColor::FromSRGBColor(FColor(Red, Green, Blue));
}
} // namespace

bool IsCombatElement(const EReEchoElement Element)
{
	return ReEchoElementRuntime::IsCombatElement(Element);
}

bool IsDoubleDamagePair(const EReEchoElement First, const EReEchoElement Second)
{
	return ReEchoElementRuntime::IsDoubleDamagePair(First, Second);
}

FReEchoElementHitResult ResolveHit(FReEchoElementState& State,
                                   const EReEchoElement IncomingElement,
                                   const float BaseDamage,
                                   const float ReactionEfficiency,
                                   const float CurrentTimeSeconds)
{
	return ReEchoElementRuntime::ResolveHit(State, IncomingElement, BaseDamage, ReactionEfficiency, CurrentTimeSeconds);
}

FReEchoElementExecutionResult ApplyHitToWorld(AReEchoEnemyActor& Target,
                                              const EReEchoElement IncomingElement,
                                              const float BaseDamage,
                                              const FReEchoElementHitContext& Context)
{
	return ReEchoHitResolver::ResolveElementHit(Target, IncomingElement, BaseDamage, Context);
}

int32 TickElementStatuses(AReEchoEnemyActor& Target, const float CurrentTimeSeconds)
{
	return ReEchoHitResolver::TickElementStatuses(Target, CurrentTimeSeconds);
}

FString GetElementLabel(const EReEchoElement Element)
{
	const FReEchoCsvElementRow* Row = FindElementRow(Element);
	return Row ? Row->VisualKey.ToString().ToUpper() : TEXT("NONE");
}

FLinearColor GetElementColor(const EReEchoElement Element)
{
	const FReEchoCsvElementRow* Row = FindElementRow(Element);
	return Row ? ParseColorHex(Row->ColorHex) : FLinearColor::White;
}

FLinearColor GetDamageNumberColor(const FReEchoDamageEvent& Event)
{
	if (Event.ReactionBehaviorId == TEXT("Reaction.Vaporize"))
	{
		return FromReferenceSrgb(165, 203, 243);
	}
	if (Event.ReactionBehaviorId == TEXT("Reaction.Conduct"))
	{
		return FromReferenceSrgb(235, 192, 44);
	}
	if (Event.ReactionBehaviorId == TEXT("Reaction.Burn"))
	{
		return FromReferenceSrgb(232, 106, 18);
	}
	if (Event.ReactionBehaviorId == TEXT("Reaction.Growth"))
	{
		return FromReferenceSrgb(146, 192, 57);
	}
	if (Event.ReactionBehaviorId == TEXT("Reaction.Enhance"))
	{
		return FromReferenceSrgb(241, 184, 76);
	}
	return Event.Element == EReEchoElement::None ? FLinearColor::White : GetElementColor(Event.Element);
}

FName GetElementId(const EReEchoElement Element)
{
	switch (Element)
	{
		case EReEchoElement::Flame:
			return TEXT("Flame");
		case EReEchoElement::Lightning:
			return TEXT("Lightning");
		case EReEchoElement::Grass:
			return TEXT("Grass");
		case EReEchoElement::Water:
			return TEXT("Water");
		default:
			return NAME_None;
	}
}
} // namespace ReEchoElementReaction
