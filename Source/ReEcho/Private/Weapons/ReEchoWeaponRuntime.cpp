#include "Weapons/ReEchoWeaponRuntime.h"

#include "Math/RotationMatrix.h"

namespace
{
const FName AnyWeaponTypeId = TEXT("Any");
const FName NoneId = TEXT("None");
const FName DamageChannelRule = TEXT("Weapon.DamageChannel");
const FName AttackPatternRule = TEXT("Weapon.AttackPatternId");
const FName AttackIntervalRule = TEXT("Weapon.AttackIntervalSeconds");
const FName PhysicalCoefficientRule = TEXT("Weapon.PhysicalCoefficient");
const FName ElementalCoefficientRule = TEXT("Weapon.ElementalCoefficient");
const FName OnKillHealRule = TEXT("Weapon.OnKillHealPercent");

bool IsNoneName(const FName Name)
{
	return Name.IsNone() || Name == NoneId;
}

bool ReadRuleFloat(const FReEchoBuildSnapshot& Build, const FName Key, const float DefaultValue, float& OutValue)
{
	const FString* Text = Build.RuleFlags.Find(Key);
	if (!Text)
	{
		OutValue = DefaultValue;
		return false;
	}
	OutValue = FCString::Atof(**Text);
	return true;
}

void WriteRuleFloat(FReEchoBuildSnapshot& Build, const FName Key, const float Value)
{
	Build.RuleFlags.Add(Key, FString::SanitizeFloat(Value));
}

int32 FindSlotLimit(const FReEchoCsvDataSnapshot& Snapshot, const FName WeaponTypeId, const FName SlotTypeId)
{
	for (const TPair<FName, FReEchoCsvSlotProfileRow>& Pair : Snapshot.SlotProfiles)
	{
		const FReEchoCsvSlotProfileRow& Profile = Pair.Value;
		if (Profile.bEnabled && Profile.WeaponTypeId == WeaponTypeId && Profile.SlotTypeId == SlotTypeId)
		{
			return Profile.SlotCount;
		}
	}
	return 0;
}

bool IsPartCompatibleWithWeapon(const FReEchoCsvDataSnapshot& Snapshot,
                                const FReEchoCsvPartRow& Part,
                                const FReEchoCsvWeaponRow& Weapon)
{
	return Part.bEnabled && (Part.WeaponTypeId == AnyWeaponTypeId || Part.WeaponTypeId == Weapon.WeaponTypeId) &&
	       FindSlotLimit(Snapshot, Weapon.WeaponTypeId, Part.SlotTypeId) > 0;
}

bool ApplyPartEffect(const FReEchoCsvWeaponRow& Weapon,
                     const FReEchoCsvPartEffectRow& Effect,
                     FReEchoBuildSnapshot& Build)
{
	if (!Effect.bEnabled)
	{
		return true;
	}

	if (Effect.EffectKind == TEXT("WeaponDamageChannel") && Effect.Target == TEXT("DamageChannel"))
	{
		Build.RuleFlags.Add(DamageChannelRule, Effect.ParamName.ToString());
		return true;
	}

	if (Effect.EffectKind == TEXT("StatModifier") && Effect.Target == TEXT("AttackSpeed"))
	{
		Build.Stats.AttackSpeed = FMath::Max(
		    0.1f, ReEchoWeaponRuntime::ApplyValueOperation(Build.Stats.AttackSpeed, Effect.ValueOp, Effect.Value));
		return true;
	}

	if (Effect.EffectKind == TEXT("StatModifier") && Effect.Target == TEXT("AttackIntervalSeconds"))
	{
		float Current = Weapon.AttackIntervalSeconds;
		ReadRuleFloat(Build, AttackIntervalRule, Current, Current);
		WriteRuleFloat(
		    Build,
		    AttackIntervalRule,
		    FMath::Max(0.01f, ReEchoWeaponRuntime::ApplyValueOperation(Current, Effect.ValueOp, Effect.Value)));
		return true;
	}

	if (Effect.EffectKind == TEXT("StatModifier") && Effect.Target == TEXT("PhysicalCoefficient"))
	{
		float Current = Weapon.PhysicalCoefficient;
		ReadRuleFloat(Build, PhysicalCoefficientRule, Current, Current);
		WriteRuleFloat(
		    Build,
		    PhysicalCoefficientRule,
		    FMath::Max(0.0f, ReEchoWeaponRuntime::ApplyValueOperation(Current, Effect.ValueOp, Effect.Value)));
		return true;
	}

	if (Effect.EffectKind == TEXT("StatModifier") && Effect.Target == TEXT("ElementalCoefficient"))
	{
		float Current = Weapon.ElementalCoefficient;
		ReadRuleFloat(Build, ElementalCoefficientRule, Current, Current);
		WriteRuleFloat(
		    Build,
		    ElementalCoefficientRule,
		    FMath::Max(0.0f, ReEchoWeaponRuntime::ApplyValueOperation(Current, Effect.ValueOp, Effect.Value)));
		return true;
	}

	if (Effect.EffectKind == TEXT("AttackPatternReplacement") && Effect.Target == TEXT("AttackPattern"))
	{
		Build.RuleFlags.Add(AttackPatternRule, Effect.AttackPatternId.ToString());
		return true;
	}

	if (Effect.EffectKind == TEXT("UniqueBehavior") && Effect.Target == TEXT("OnKill") &&
	    Effect.BehaviorId == TEXT("Part.OnKillHealPercent"))
	{
		float Current = 0.0f;
		ReadRuleFloat(Build, OnKillHealRule, Current, Current);
		const float Value = !Effect.ParamName.IsNone() && Effect.ParamName == TEXT("HealMaxHpPercent")
		                        ? Effect.ParamValue
		                        : Effect.Value;
		WriteRuleFloat(Build,
		               OnKillHealRule,
		               FMath::Max(0.0f, ReEchoWeaponRuntime::ApplyValueOperation(Current, Effect.ValueOp, Value)));
		return true;
	}

	return false;
}

FName RuleNameOrDefault(const FReEchoBuildSnapshot& Build, const FName Key, const FName DefaultValue)
{
	const FString* Text = Build.RuleFlags.Find(Key);
	return Text ? FName(**Text) : DefaultValue;
}
} // namespace

float ReEchoWeaponRuntime::ApplyValueOperation(const float CurrentValue,
                                               const EReEchoCsvValueOp ValueOp,
                                               const float Value)
{
	switch (ValueOp)
	{
		case EReEchoCsvValueOp::Add:
			return CurrentValue + Value;
		case EReEchoCsvValueOp::Multiply:
			return CurrentValue * Value;
		case EReEchoCsvValueOp::Override:
			return Value;
		default:
			return CurrentValue;
	}
}

FString ReEchoWeaponRuntime::GetBuildConfigurationError(const FReEchoCsvDataSnapshot& Snapshot,
                                                        const FReEchoBuildSnapshot& Build)
{
	const FReEchoCsvCharacterRow* Character = Snapshot.FindCharacter(Build.CharacterId);
	const FReEchoCsvWeaponRow* Weapon = Snapshot.FindWeapon(Build.WeaponId);
	if (!Character)
	{
		return FString::Printf(TEXT("CharacterId '%s' is not configured"), *Build.CharacterId.ToString());
	}
	if (!Character->bEnabled)
	{
		return FString::Printf(TEXT("CharacterId '%s' is disabled"), *Build.CharacterId.ToString());
	}
	if (!Weapon)
	{
		return FString::Printf(TEXT("WeaponId '%s' is not configured"), *Build.WeaponId.ToString());
	}
	if (!Weapon->bEnabled)
	{
		return FString::Printf(TEXT("WeaponId '%s' is disabled"), *Build.WeaponId.ToString());
	}
	if (Build.WeaponDataRevision != Weapon->DataRevision)
	{
		return FString::Printf(TEXT("WeaponId '%s' data revision mismatch saved=%d current=%d"),
		                       *Build.WeaponId.ToString(),
		                       Build.WeaponDataRevision,
		                       Weapon->DataRevision);
	}
	if (Build.WeaponDomainRevision.IsEmpty() || Build.WeaponDomainRevision != Snapshot.WeaponDomainRevision)
	{
		return FString::Printf(TEXT("Weapon domain revision mismatch saved=%s current=%s"),
		                       *Build.WeaponDomainRevision,
		                       *Snapshot.WeaponDomainRevision);
	}

	FString PartError;
	FReEchoBuildSnapshot Candidate;
	TArray<FName> PartIds;
	for (const FReEchoEquippedPartSnapshot& EquippedPart : Build.EquippedParts)
	{
		PartIds.Add(EquippedPart.PartId);
	}
	if (!TryEquipParts(Snapshot, Build, PartIds, Candidate, PartError))
	{
		return PartError;
	}
	return FString();
}

bool ReEchoWeaponRuntime::TryEquipParts(const FReEchoCsvDataSnapshot& Snapshot,
                                        const FReEchoBuildSnapshot& Build,
                                        const TArray<FName>& PartIds,
                                        FReEchoBuildSnapshot& OutBuild,
                                        FString& OutError)
{
	const FReEchoCsvWeaponRow* Weapon = Snapshot.FindEnabledWeapon(Build.WeaponId);
	if (!Weapon)
	{
		OutError = FString::Printf(TEXT("Cannot equip parts: WeaponId '%s' is unknown or disabled"),
		                           *Build.WeaponId.ToString());
		return false;
	}

	if (Build.WeaponDomainRevision.IsEmpty() || Build.WeaponDomainRevision != Snapshot.WeaponDomainRevision)
	{
		OutError = FString::Printf(TEXT("Cannot equip parts: weapon domain revision mismatch saved=%s current=%s"),
		                           *Build.WeaponDomainRevision,
		                           *Snapshot.WeaponDomainRevision);
		return false;
	}

	FReEchoBuildSnapshot Candidate;
	if (!TryGetEquipmentBaseBuild(Build, Candidate, OutError))
	{
		return false;
	}
	Candidate.EquippedParts.Reset();

	TSet<FName> SeenPartIds;
	TMap<FName, int32> SlotCounts;
	TArray<const FReEchoCsvPartRow*> ValidParts;
	for (const FName PartId : PartIds)
	{
		if (IsNoneName(PartId))
		{
			OutError = TEXT("Cannot equip part: PartId None is not equipable");
			return false;
		}
		if (SeenPartIds.Contains(PartId))
		{
			OutError = FString::Printf(TEXT("Cannot equip part '%s': duplicate PartId"), *PartId.ToString());
			return false;
		}
		SeenPartIds.Add(PartId);

		const FReEchoCsvPartRow* Part = Snapshot.Parts.Find(PartId);
		if (!Part || Part->PartId != PartId)
		{
			OutError = FString::Printf(TEXT("Cannot equip part '%s': part was not found"), *PartId.ToString());
			return false;
		}
		if (!Part->bEnabled)
		{
			OutError = FString::Printf(TEXT("Cannot equip part '%s': part is disabled"), *PartId.ToString());
			return false;
		}
		if (Part->WeaponTypeId != AnyWeaponTypeId && Part->WeaponTypeId != Weapon->WeaponTypeId)
		{
			OutError = FString::Printf(TEXT("Cannot equip part '%s': WeaponTypeId '%s' does not match '%s'"),
			                           *PartId.ToString(),
			                           *Part->WeaponTypeId.ToString(),
			                           *Weapon->WeaponTypeId.ToString());
			return false;
		}
		const int32 SlotLimit = FindSlotLimit(Snapshot, Weapon->WeaponTypeId, Part->SlotTypeId);
		if (SlotLimit <= 0)
		{
			OutError =
			    FString::Printf(TEXT("Cannot equip part '%s': SlotTypeId '%s' is not valid for WeaponTypeId '%s'"),
			                    *PartId.ToString(),
			                    *Part->SlotTypeId.ToString(),
			                    *Weapon->WeaponTypeId.ToString());
			return false;
		}
		const int32 NewSlotCount = SlotCounts.FindRef(Part->SlotTypeId) + 1;
		if (NewSlotCount > SlotLimit)
		{
			OutError = FString::Printf(TEXT("Cannot equip part '%s': SlotTypeId '%s' exceeds slot limit %d"),
			                           *PartId.ToString(),
			                           *Part->SlotTypeId.ToString(),
			                           SlotLimit);
			return false;
		}
		SlotCounts.Add(Part->SlotTypeId, NewSlotCount);
		ValidParts.Add(Part);
	}

	for (const FReEchoCsvPartRow* Part : ValidParts)
	{
		FReEchoEquippedPartSnapshot EquippedPart;
		EquippedPart.PartId = Part->PartId;
		EquippedPart.SlotTypeId = Part->SlotTypeId;
		Candidate.EquippedParts.Add(EquippedPart);
		for (const FReEchoCsvPartEffectRow& Effect : Part->Effects)
		{
			if (!ApplyPartEffect(*Weapon, Effect, Candidate))
			{
				OutError = FString::Printf(TEXT("Cannot equip part '%s': unsupported EffectKind/Target '%s/%s'"),
				                           *Part->PartId.ToString(),
				                           *Effect.EffectKind.ToString(),
				                           *Effect.Target.ToString());
				return false;
			}
		}
	}

	OutBuild = Candidate;
	OutError.Reset();
	return true;
}

bool ReEchoWeaponRuntime::TryGetEquipmentBaseBuild(const FReEchoBuildSnapshot& Build,
                                                   FReEchoBuildSnapshot& OutBaseBuild,
                                                   FString& OutError)
{
	FReEchoBuildSnapshot Candidate = Build;
	if (Build.bHasEquipmentBase)
	{
		Candidate.Stats = Build.EquipmentBaseStats;
		Candidate.RuleFlags = Build.EquipmentBaseRuleFlags;
	}
	else if (Build.EquippedParts.IsEmpty())
	{
		Candidate.EquipmentBaseStats = Build.Stats;
		Candidate.EquipmentBaseRuleFlags = Build.RuleFlags;
		Candidate.bHasEquipmentBase = true;
	}
	else
	{
		OutError = TEXT("Cannot rebuild equipment: authoritative non-equipment build is missing");
		return false;
	}

	Candidate.EquippedParts.Reset();
	OutBaseBuild = Candidate;
	OutError.Reset();
	return true;
}

bool ReEchoWeaponRuntime::TrySelectWeapon(const FReEchoCsvDataSnapshot& Snapshot,
                                          const FReEchoBuildSnapshot& Build,
                                          const FName WeaponId,
                                          FReEchoBuildSnapshot& OutBuild,
                                          FString& OutError)
{
	const FReEchoCsvWeaponRow* Weapon = Snapshot.FindEnabledWeapon(WeaponId);
	if (!Weapon)
	{
		OutError =
		    FString::Printf(TEXT("Cannot select WeaponId '%s': weapon is unknown or disabled"), *WeaponId.ToString());
		return false;
	}
	if (Build.WeaponDomainRevision.IsEmpty() || Build.WeaponDomainRevision != Snapshot.WeaponDomainRevision)
	{
		OutError =
		    FString::Printf(TEXT("Cannot select WeaponId '%s': weapon domain revision mismatch"), *WeaponId.ToString());
		return false;
	}

	FReEchoBuildSnapshot Candidate = Build;
	Candidate.WeaponId = Weapon->Id;
	Candidate.WeaponDataRevision = Weapon->DataRevision;
	Candidate.WeaponDomainRevision = Snapshot.WeaponDomainRevision;

	TArray<FName> CompatiblePartIds;
	TMap<FName, int32> RetainedSlotCounts;
	for (const FReEchoEquippedPartSnapshot& EquippedPart : Build.EquippedParts)
	{
		const FReEchoCsvPartRow* Part = Snapshot.Parts.Find(EquippedPart.PartId);
		if (!Part || !IsPartCompatibleWithWeapon(Snapshot, *Part, *Weapon))
		{
			continue;
		}
		const int32 SlotLimit = FindSlotLimit(Snapshot, Weapon->WeaponTypeId, Part->SlotTypeId);
		const int32 RetainedCount = RetainedSlotCounts.FindRef(Part->SlotTypeId);
		if (RetainedCount >= SlotLimit)
		{
			continue;
		}
		RetainedSlotCounts.Add(Part->SlotTypeId, RetainedCount + 1);
		CompatiblePartIds.Add(Part->PartId);
	}

	return TryEquipParts(Snapshot, Candidate, CompatiblePartIds, OutBuild, OutError);
}

bool ReEchoWeaponRuntime::BuildEffectiveWeaponDefinition(const FReEchoCsvDataSnapshot& Snapshot,
                                                         const FReEchoBuildSnapshot& Build,
                                                         FReEchoEffectiveWeaponDefinition& OutDefinition,
                                                         FString& OutError)
{
	const FString BuildError = GetBuildConfigurationError(Snapshot, Build);
	if (!BuildError.IsEmpty())
	{
		OutError = BuildError;
		return false;
	}

	const FReEchoCsvWeaponRow* Weapon = Snapshot.FindEnabledWeapon(Build.WeaponId);
	check(Weapon);
	OutDefinition = {};
	OutDefinition.Weapon = *Weapon;
	OutDefinition.Weapon.AttackPatternId = RuleNameOrDefault(Build, AttackPatternRule, Weapon->AttackPatternId);
	ReadRuleFloat(Build,
	              AttackIntervalRule,
	              OutDefinition.Weapon.AttackIntervalSeconds,
	              OutDefinition.Weapon.AttackIntervalSeconds);
	ReadRuleFloat(Build,
	              PhysicalCoefficientRule,
	              OutDefinition.Weapon.PhysicalCoefficient,
	              OutDefinition.Weapon.PhysicalCoefficient);
	ReadRuleFloat(Build,
	              ElementalCoefficientRule,
	              OutDefinition.Weapon.ElementalCoefficient,
	              OutDefinition.Weapon.ElementalCoefficient);
	ReadRuleFloat(Build, OnKillHealRule, 0.0f, OutDefinition.OnKillHealPercent);
	OutDefinition.DamageChannelId = RuleNameOrDefault(Build, DamageChannelRule, NAME_None);

	if (OutDefinition.DamageChannelId.IsNone())
	{
		OutDefinition.bUsesCyclingElement =
		    OutDefinition.Weapon.AttackPatternId == TEXT("Pattern.ElementalProjectile") ||
		    (OutDefinition.Weapon.ElementalCoefficient > 0.0f && OutDefinition.Weapon.PhysicalCoefficient <= 0.0f);
		OutDefinition.DamageChannelId = OutDefinition.bUsesCyclingElement ? TEXT("CycleElement") : TEXT("Physical");
	}
	OutDefinition.bUsesDeterministicRandomElement = OutDefinition.DamageChannelId == TEXT("RandomElement");
	OutDefinition.AttackSteps = Snapshot.GetAttackSteps(OutDefinition.Weapon.AttackPatternId);
	if (OutDefinition.AttackSteps.IsEmpty())
	{
		OutError = FString::Printf(TEXT("WeaponId '%s' has no enabled attack steps for AttackPatternId '%s'"),
		                           *Build.WeaponId.ToString(),
		                           *OutDefinition.Weapon.AttackPatternId.ToString());
		return false;
	}
	OutError.Reset();
	return true;
}

bool ReEchoWeaponRuntime::IsInsideMeleeArc(
    const FVector& Origin, const FVector& Forward, const FVector& Target, const float RangeCm, const float ArcDegrees)
{
	const FVector ToTarget = (Target - Origin).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		return true;
	}
	if (FVector::Dist2D(Origin, Target) > RangeCm)
	{
		return false;
	}
	if (ArcDegrees >= 359.9f)
	{
		return true;
	}
	const FVector Forward2D =
	    Forward.GetSafeNormal2D().IsNearlyZero() ? FVector::ForwardVector : Forward.GetSafeNormal2D();
	const float Dot = FMath::Clamp(FVector::DotProduct(Forward2D, ToTarget), -1.0f, 1.0f);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));
	return AngleDegrees <= ArcDegrees * 0.5f + KINDA_SMALL_NUMBER;
}

TArray<FVector> ReEchoWeaponRuntime::BuildProjectileDirections(const FVector& Forward,
                                                               const int32 ProjectileCount,
                                                               const float SpreadDegrees)
{
	const int32 Count = FMath::Max(1, ProjectileCount);
	const FVector Forward2D =
	    Forward.GetSafeNormal2D().IsNearlyZero() ? FVector::ForwardVector : Forward.GetSafeNormal2D();
	TArray<FVector> Directions;
	Directions.Reserve(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Alpha = Count == 1 ? 0.5f : static_cast<float>(Index) / static_cast<float>(Count - 1);
		const float YawOffset = (Alpha - 0.5f) * SpreadDegrees;
		Directions.Add(Forward2D.RotateAngleAxis(YawOffset, FVector::UpVector).GetSafeNormal());
	}
	return Directions;
}

EReEchoElement ReEchoWeaponRuntime::ElementFromDamageChannel(const FName DamageChannelId, const int32 AttackSequence)
{
	if (DamageChannelId == TEXT("Flame"))
	{
		return EReEchoElement::Flame;
	}
	if (DamageChannelId == TEXT("Lightning"))
	{
		return EReEchoElement::Lightning;
	}
	if (DamageChannelId == TEXT("Grass"))
	{
		return EReEchoElement::Grass;
	}
	if (DamageChannelId == TEXT("Water"))
	{
		return EReEchoElement::Water;
	}
	if (DamageChannelId == TEXT("RandomElement"))
	{
		switch ((AttackSequence * 17 + 5) % 4)
		{
			case 0:
				return EReEchoElement::Water;
			case 1:
				return EReEchoElement::Flame;
			case 2:
				return EReEchoElement::Lightning;
			default:
				return EReEchoElement::Grass;
		}
	}
	return EReEchoElement::None;
}
