#include "Run/CharacterAbilities/ReEchoCharacterAbilityRuntime.h"

#include "Data/ReEchoCsvDataRegistry.h"

namespace
{
void AddStatValue(FReEchoStatBlock& Stats, const FName Target, const float Value)
{
	if (Target == TEXT("MovementSpeed"))
	{
		Stats.MovementSpeed += Value;
	}
	else if (Target == TEXT("CriticalRate"))
	{
		Stats.CriticalRate += Value;
	}
	else if (Target == TEXT("CriticalEffect"))
	{
		Stats.CriticalEffect += Value;
	}
	else if (Target == TEXT("ReactionEfficiency"))
	{
		Stats.ReactionEfficiency += Value;
	}
}
}

namespace ReEchoCharacterAbilityRuntime
{
void ApplyStaticBuildEffects(const FReEchoCsvDataSnapshot& Snapshot,
                             const FName CharacterId,
                             FReEchoStatBlock& Stats)
{
	for (const FReEchoCsvCharacterAbilityRow& Ability :
	     Snapshot.GetCharacterAbilities(CharacterId, TEXT("OnBuildInitialized")))
	{
		if (Ability.BehaviorId == TEXT("Character.StaticStat") && Ability.EffectKind == TEXT("StatModifier") &&
		    Ability.ValueOp == EReEchoCsvValueOp::Add)
		{
			AddStatValue(Stats, Ability.Target, Ability.Value);
		}
	}
}

void ApplyEncounterCompletedEffects(const FReEchoCsvDataSnapshot& Snapshot,
                                    const FName CharacterId,
                                    FReEchoStatBlock& Stats)
{
	for (const FReEchoCsvCharacterAbilityRow& Ability :
	     Snapshot.GetCharacterAbilities(CharacterId, TEXT("OnEncounterCompleted")))
	{
		if (Ability.BehaviorId == TEXT("Character.PersistentGrowth") &&
		    Ability.EffectKind == TEXT("StatModifier") && Ability.ValueOp == EReEchoCsvValueOp::Add)
		{
			AddStatValue(Stats, Ability.Target, Ability.Value);
		}
	}
}

int32 ResolveExtraTraitChoices(const FReEchoCsvDataSnapshot& Snapshot,
                               const FName CharacterId,
                               const int32 NormalSelectionCount)
{
	int32 Result = 0;
	for (const FReEchoCsvCharacterAbilityRow& Ability :
	     Snapshot.GetCharacterAbilities(CharacterId, TEXT("OnTraitChoiceApplied")))
	{
		const int32 Interval = FMath::RoundToInt(Ability.Interval);
		if (Ability.BehaviorId == TEXT("Character.EveryNth") && Ability.EffectKind == TEXT("ExtraCardChoice") &&
		    Ability.Target == TEXT("TraitCardChoice") && Ability.ValueOp == EReEchoCsvValueOp::Add && Interval > 0 &&
		    NormalSelectionCount > 0 && NormalSelectionCount % Interval == 0)
		{
			Result += FMath::Max(0, FMath::RoundToInt(Ability.Value));
		}
	}
	return Result;
}

int32 ResolveExtraTraitChoicesForTier(const FReEchoCsvDataSnapshot& Snapshot,
                                      const FName CharacterId,
                                      const int32 NormalSelectionCount,
                                      const int32 CardPackTier)
{
	int32 Result = 0;
	for (const FReEchoCsvCharacterAbilityRow& Ability :
	     Snapshot.GetCharacterAbilities(CharacterId, TEXT("OnTraitChoiceApplied")))
	{
		const int32 Interval = FMath::RoundToInt(Ability.Interval);
		if (Ability.BehaviorId != TEXT("Character.EveryNth") || Ability.EffectKind != TEXT("ExtraCardChoice") ||
		    Ability.Target != TEXT("TraitCardChoice") || Ability.ValueOp != EReEchoCsvValueOp::Add || Interval <= 0 ||
		    NormalSelectionCount <= 0 || NormalSelectionCount % Interval != 0)
		{
			continue;
		}
		// Tier gate: MinCardPackTier <= 0 triggers on every pack, otherwise only at or above that tier.
		if (Ability.MinCardPackTier > 0 && CardPackTier < Ability.MinCardPackTier)
		{
			continue;
		}
		Result += FMath::Max(0, FMath::RoundToInt(Ability.Value));
	}
	return Result;
}

bool DoesCardPackTierAdvanceTraitBonus(const FReEchoCsvDataSnapshot& Snapshot,
                                       const FName CharacterId,
                                       const int32 CardPackTier)
{
	bool bFoundCadenceAbility = false;
	for (const FReEchoCsvCharacterAbilityRow& Ability :
	     Snapshot.GetCharacterAbilities(CharacterId, TEXT("OnTraitChoiceApplied")))
	{
		if (Ability.BehaviorId != TEXT("Character.EveryNth") || Ability.EffectKind != TEXT("ExtraCardChoice") ||
		    Ability.Target != TEXT("TraitCardChoice") || Ability.ValueOp != EReEchoCsvValueOp::Add ||
		    FMath::RoundToInt(Ability.Interval) <= 0)
		{
			continue;
		}
		bFoundCadenceAbility = true;
		// MinCardPackTier <= 0 counts every pack; otherwise only packs at or above the tier count.
		if (Ability.MinCardPackTier <= 0 || CardPackTier >= Ability.MinCardPackTier)
		{
			return true;
		}
	}
	// No tier-gated cadence ability on this character: leave the legacy counter behaviour untouched.
	return !bFoundCadenceAbility;
}

FVector2D ResolveCurrentMissingHealthAttackBonus(const FReEchoCsvDataSnapshot& Snapshot,
                                                  const FName CharacterId,
                                                  const float CurrentHealth,
                                                  const float MaximumHealth)
{
	if (MaximumHealth <= UE_SMALL_NUMBER)
	{
		return FVector2D::ZeroVector;
	}
	FVector2D Result = FVector2D::ZeroVector;
	const float MissingFraction = FMath::Clamp((MaximumHealth - CurrentHealth) / MaximumHealth, 0.0f, 1.0f);
	for (const FReEchoCsvCharacterAbilityRow& Ability :
	     Snapshot.GetCharacterAbilities(CharacterId, TEXT("OnHealthChanged")))
	{
		if (Ability.BehaviorId != TEXT("Character.MissingHealthSteps") ||
		    Ability.EffectKind != TEXT("StatModifier") || Ability.ValueOp != EReEchoCsvValueOp::Add ||
		    Ability.Interval <= UE_SMALL_NUMBER)
		{
			continue;
		}
		const int32 Steps = FMath::Clamp(FMath::FloorToInt((MissingFraction + KINDA_SMALL_NUMBER) / Ability.Interval),
		                                     0,
		                                     FMath::CeilToInt(1.0f / Ability.Interval));
		const float Bonus = Steps * Ability.Value;
		if (Ability.Target == TEXT("PhysicalAttack"))
		{
			Result.X += Bonus;
		}
		else if (Ability.Target == TEXT("ElementalAttack"))
		{
			Result.Y += Bonus;
		}
	}
	return Result;
}
}
