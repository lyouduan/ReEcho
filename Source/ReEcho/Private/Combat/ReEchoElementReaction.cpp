#include "Combat/ReEchoElementReaction.h"

#include "Data/ReEchoCsvDataRegistry.h"

namespace ReEchoElementReaction
{
namespace
{
constexpr const TCHAR* ElementImmunityStatusId = TEXT("Z_Elemental_Immunity");
constexpr const TCHAR* StatusNone = TEXT("None");

const FReEchoCsvElementRow* FindElementRow(const EReEchoElement Element)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	return Snapshot.IsValid() ? Snapshot->FindElement(Element) : nullptr;
}

uint8 ParseHexPair(const FString& Text, const int32 Index)
{
	const FString Pair = Text.Mid(Index, 2);
	return static_cast<uint8>(FCString::Strtoi(*Pair, nullptr, 16));
}

FLinearColor ParseColorHex(const FString& ColorHex)
{
	if (!ColorHex.StartsWith(TEXT("#")) || ColorHex.Len() != 7)
	{
		return FLinearColor::White;
	}
	const FColor Color(ParseHexPair(ColorHex, 1), ParseHexPair(ColorHex, 3), ParseHexPair(ColorHex, 5));
	return FLinearColor::FromSRGBColor(Color);
}

bool IsAttachmentRole(const FReEchoCsvElementRow& Element)
{
	return Element.Role == EReEchoElementRole::Attachment;
}

float GetEnabledStatusDuration(const FReEchoCsvDataSnapshot& Snapshot, const FName StatusId)
{
	const FReEchoCsvStatusRow* Status = Snapshot.FindStatus(StatusId);
	return Status && Status->bEnabled ? Status->DurationSeconds : 0.0f;
}
}

bool IsCombatElement(const EReEchoElement Element)
{
	const FReEchoCsvElementRow* Row = FindElementRow(Element);
	return Row && Row->bEnabled;
}

bool IsDoubleDamagePair(const EReEchoElement First, const EReEchoElement Second)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	const FReEchoCsvElementRow* FirstRow = Snapshot.IsValid() ? Snapshot->FindElement(First) : nullptr;
	const FReEchoCsvElementRow* SecondRow = Snapshot.IsValid() ? Snapshot->FindElement(Second) : nullptr;
	if (!Snapshot.IsValid() || !FirstRow || !SecondRow || First == Second)
	{
		return false;
	}
	return Snapshot->FindReaction(FirstRow->Id, SecondRow->Id) != nullptr ||
	       Snapshot->FindReaction(SecondRow->Id, FirstRow->Id) != nullptr;
}

FReEchoElementHitResult ResolveHit(FReEchoElementState& State,
                                   const EReEchoElement IncomingElement,
                                   const float BaseDamage,
                                   const float ReactionEfficiency,
                                   const float CurrentTimeSeconds)
{
	FReEchoElementHitResult Result;
	Result.Damage = FMath::Max(0.0f, BaseDamage);
	Result.PreviousElement = State.Attached;
	Result.IncomingElement = IncomingElement;
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	const FReEchoCsvElementRow* IncomingRow = Snapshot.IsValid() ? Snapshot->FindElement(IncomingElement) : nullptr;
	if (!Snapshot.IsValid() || !IncomingRow || !IncomingRow->bEnabled)
	{
		return Result;
	}

	if (CurrentTimeSeconds >= 0.0f && State.ImmunityUntil > CurrentTimeSeconds)
	{
		Result.bBlockedByImmunity = true;
		return Result;
	}
	if (State.BlockedAttachment == IncomingElement)
	{
		Result.bBlockedByImmunity = true;
		return Result;
	}

	const FReEchoCsvElementRow* AttachedRow = Snapshot->FindElement(State.Attached);
	const FReEchoCsvReactionRow* Reaction =
	    AttachedRow && AttachedRow->bEnabled ? Snapshot->FindReaction(IncomingRow->Id, AttachedRow->Id) : nullptr;
	if (Reaction)
	{
		Result.bTriggeredReaction = true;
		Result.ReactionId = Reaction->Id;
		Result.ReactionBehaviorId = Reaction->BehaviorId;
		Result.FormulaId = Reaction->FormulaId;
		Result.RadiusCm = Reaction->RadiusCm;
		Result.AppliedStatusId = Reaction->StatusId == StatusNone ? NAME_None : Reaction->StatusId;
		Result.StatusDurationSeconds = Reaction->StatusDurationSeconds;
		Result.EnhancementMultiplier = Reaction->EnhancementMultiplier;
		Result.bCanCrit = Reaction->bCanCrit;
		Result.bAffectedByEchoEfficiency = Reaction->bAffectedByEchoEfficiency;

		float EffectiveReactionEfficiency = FMath::Max(0.0f, ReactionEfficiency);
		if (State.bEnhancedNextReaction)
		{
			EffectiveReactionEfficiency *= FMath::Max(1.0f, State.EnhancementMultiplier);
			Result.bAppliedEnhancement = true;
			State.bEnhancedNextReaction = false;
			State.EnhancementMultiplier = 1.0f;
		}

		if (Reaction->FormulaId == TEXT("Element.DamageIncrease"))
		{
			Result.Multiplier = 1.0f + Reaction->DamageIncrease * EffectiveReactionEfficiency;
			Result.Damage *= Result.Multiplier;
		}
		else if (Reaction->FormulaId == TEXT("Element.EnhanceNextReaction"))
		{
			Result.Multiplier = 1.0f;
			Result.Damage = FMath::Max(0.0f, BaseDamage);
			State.bEnhancedNextReaction = true;
			State.EnhancementMultiplier = FMath::Max(State.EnhancementMultiplier, Reaction->EnhancementMultiplier);
		}
		else
		{
			Result.Multiplier = Reaction->DamageMultiplier * EffectiveReactionEfficiency;
			Result.Damage *= Result.Multiplier;
		}

		if (Result.AppliedStatusId != NAME_None && CurrentTimeSeconds >= 0.0f)
		{
			const float Duration = Result.StatusDurationSeconds > 0.0f
			                           ? Result.StatusDurationSeconds
			                           : GetEnabledStatusDuration(*Snapshot, Result.AppliedStatusId);
			State.ActiveStatusUntilSeconds.Add(Result.AppliedStatusId, CurrentTimeSeconds + Duration);
		}
		const float ImmunityDuration = GetEnabledStatusDuration(*Snapshot, ElementImmunityStatusId);
		if (ImmunityDuration > 0.0f)
		{
			Result.ImmunityDurationSeconds = ImmunityDuration;
			if (CurrentTimeSeconds >= 0.0f)
			{
				State.ImmunityUntil = CurrentTimeSeconds + ImmunityDuration;
				State.ActiveStatusUntilSeconds.Add(FName(ElementImmunityStatusId), State.ImmunityUntil);
			}
		}
		if (Reaction->bClearsAttachment)
		{
			State.Attached = EReEchoElement::None;
		}
	}
	else if (IsAttachmentRole(*IncomingRow))
	{
		State.Attached = IncomingElement;
	}
	return Result;
}

FString GetElementLabel(const EReEchoElement Element)
{
	const FReEchoCsvElementRow* Row = FindElementRow(Element);
	if (Row)
	{
		return Row->VisualKey.ToString().ToUpper();
	}
	return TEXT("NONE");
}

FLinearColor GetElementColor(const EReEchoElement Element)
{
	const FReEchoCsvElementRow* Row = FindElementRow(Element);
	if (Row)
	{
		return ParseColorHex(Row->ColorHex);
	}
	return FLinearColor::White;
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
}
