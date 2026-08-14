#include "Combat/ReEchoElementReaction.h"

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
