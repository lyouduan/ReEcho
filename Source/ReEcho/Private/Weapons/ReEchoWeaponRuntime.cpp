#include "Weapons/ReEchoWeaponRuntime.h"

#include "Cards/ReEchoCardRuntime.h"
#include "Math/RotationMatrix.h"
#include "Weapons/ReEchoWeaponGeometry.h"

namespace
{
const FName AnyWeaponTypeId = TEXT("Any");
const FName NoneId = TEXT("None");
const FName DamageChannelRule = TEXT("Weapon.DamageChannel");
const FName AttackPatternRule = TEXT("Weapon.AttackPatternId");
const FName AttackIntervalRule = TEXT("Weapon.AttackIntervalSeconds");
const FName DamageCoefficientRule = TEXT("Weapon.DamageCoefficient");
const FName OnKillHealRule = TEXT("Weapon.OnKillHealPercent");
const FName WeaponRangeRule = TEXT("Weapon.RangeCm");
const FName WeaponArcRule = TEXT("Weapon.ArcDegrees");
const FName WeaponProjectileRule = TEXT("Weapon.ProjectileCount");
const FName WeaponConcentrationRule = TEXT("Weapon.ConcentrationDegrees");
const FName WeaponExplosionRule = TEXT("Weapon.ExplosionRadiusCm");

const TSet<FName> RuntimeRuneBehaviorIds = {
    TEXT("Part.ProjectileSplitOnHit"),
    TEXT("Part.ProjectilePierceOnCritical"),
    TEXT("Part.ApplyBleedOnCritical"),
    TEXT("Part.DropShardOnKill"),
    TEXT("Part.MoveSpeedOnKill"),
    TEXT("Part.AttackMoveSpeedOnAttack"),
    TEXT("Part.RangeOnGroupHit"),
    TEXT("Part.HealOnHit"),
    TEXT("Part.StunOnHit"),
    TEXT("Part.BleedEveryTargetHits"),
    TEXT("Part.OuterRingDamage"),
    TEXT("Part.MoveSpeedPerHit"),
    TEXT("Part.InvulnerableOnGroupHit"),
    TEXT("Part.AttackSpeedPerHit"),
    TEXT("Part.ScytheThrowRecall"),
    TEXT("Part.DropShardEveryHits"),
    TEXT("Part.MeteorOnGroupHit"),
    TEXT("Part.ApplyBleedOnHitChance"),
    TEXT("Part.AttackSpeedOnAttack"),
    TEXT("Part.MoveSpeedOnAttack"),
};

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

int32 FindEffectiveSlotLimit(const FReEchoCsvDataSnapshot& Snapshot,
                             const FReEchoBuildSnapshot& Build,
                             const FName WeaponTypeId,
                             const FName SlotTypeId)
{
	const int32 BaseLimit = FindSlotLimit(Snapshot, WeaponTypeId, SlotTypeId);
	if (BaseLimit <= 0 || SlotTypeId == TEXT("Core") || !Snapshot.CardCatalog.IsValid())
	{
		return BaseLimit;
	}
	const FReEchoCardRuleSnapshot Rules = ReEchoCardRuntime::CompileRules(*Snapshot.CardCatalog, Build.CardState);
	return Rules.bDoubleNonCoreSlotCapacity ? FMath::Max(2, BaseLimit * 2) : BaseLimit;
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

	if (Effect.EffectKind == TEXT("StatModifier") && Effect.Target == TEXT("DamageCoefficient"))
	{
		float Current = Weapon.DamageCoefficient;
		ReadRuleFloat(Build, DamageCoefficientRule, Current, Current);
		WriteRuleFloat(
		    Build,
		    DamageCoefficientRule,
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

	// Dynamic rune behaviors are compiled into the effective definition after equipment validation.
	// Applying equipment only needs to accept them here; WeaponActor executes the typed specs after Combat resolves.
	if (Effect.EffectKind == TEXT("UniqueBehavior") && Effect.Target == TEXT("RuntimeBehavior") &&
	    ReEchoWeaponRuntime::IsRuntimeRuneBehaviorSupported(Effect.BehaviorId))
	{
		return true;
	}

	// Weapon-parameter StatModifiers: override the matching weapon-level field through a RuleFlag,
	// consumed by BuildEffectiveWeaponDefinition. Excel effects like "攻击范围+30%" / "弹道固定为3" land here.
	// Each branch clamps into the field's legal domain, mirroring the coefficient branches above, so a
	// bad table value can never produce a zero/negative range, arc or projectile count at runtime.
	if (Effect.EffectKind == TEXT("StatModifier") && Effect.Target == TEXT("AttackRange"))
	{
		float Current = Weapon.RangeCm;
		ReadRuleFloat(Build, WeaponRangeRule, Current, Current);
		WriteRuleFloat(
		    Build,
		    WeaponRangeRule,
		    FMath::Max(1.0f, ReEchoWeaponRuntime::ApplyValueOperation(Current, Effect.ValueOp, Effect.Value)));
		return true;
	}
	if (Effect.EffectKind == TEXT("StatModifier") && Effect.Target == TEXT("AttackArc"))
	{
		float Current = Weapon.ArcDegrees;
		ReadRuleFloat(Build, WeaponArcRule, Current, Current);
		WriteRuleFloat(Build,
		               WeaponArcRule,
		               FMath::Clamp(ReEchoWeaponRuntime::ApplyValueOperation(Current, Effect.ValueOp, Effect.Value),
		                            1.0f,
		                            360.0f));
		return true;
	}
	if (Effect.EffectKind == TEXT("StatModifier") && Effect.Target == TEXT("ProjectileCount"))
	{
		float Current = static_cast<float>(Weapon.ProjectileCount);
		ReadRuleFloat(Build, WeaponProjectileRule, Current, Current);
		WriteRuleFloat(
		    Build,
		    WeaponProjectileRule,
		    FMath::Max(1.0f, ReEchoWeaponRuntime::ApplyValueOperation(Current, Effect.ValueOp, Effect.Value)));
		return true;
	}
	if (Effect.EffectKind == TEXT("StatModifier") && Effect.Target == TEXT("ConcentrationDegrees"))
	{
		float Current = Weapon.ConcentrationDegrees;
		ReadRuleFloat(Build, WeaponConcentrationRule, Current, Current);
		WriteRuleFloat(Build,
		               WeaponConcentrationRule,
		               FMath::Clamp(ReEchoWeaponRuntime::ApplyValueOperation(Current, Effect.ValueOp, Effect.Value),
		                            0.0f,
		                            360.0f));
		return true;
	}
	if (Effect.EffectKind == TEXT("StatModifier") && Effect.Target == TEXT("ExplosionRadius"))
	{
		float Current = Weapon.ExplosionRadiusCm;
		ReadRuleFloat(Build, WeaponExplosionRule, Current, Current);
		WriteRuleFloat(
		    Build,
		    WeaponExplosionRule,
		    FMath::Max(0.0f, ReEchoWeaponRuntime::ApplyValueOperation(Current, Effect.ValueOp, Effect.Value)));
		return true;
	}

	return false;
}

FName RuleNameOrDefault(const FReEchoBuildSnapshot& Build, const FName Key, const FName DefaultValue)
{
	const FString* Text = Build.RuleFlags.Find(Key);
	return Text ? FName(**Text) : DefaultValue;
}

void ApplyPartEffectToAttackSteps(const FReEchoCsvPartEffectRow& Effect, TArray<FReEchoCsvAttackStepRow>& AttackSteps)
{
	if (!Effect.bEnabled || Effect.EffectKind != TEXT("StatModifier"))
	{
		return;
	}
	for (FReEchoCsvAttackStepRow& Step : AttackSteps)
	{
		if (Effect.Target == TEXT("DamageCoefficient"))
		{
			Step.DamageCoefficient = FMath::Max(
			    0.0f, ReEchoWeaponRuntime::ApplyValueOperation(Step.DamageCoefficient, Effect.ValueOp, Effect.Value));
		}
		else if (Effect.Target == TEXT("AttackRange"))
		{
			Step.RangeCm =
			    FMath::Max(1.0f, ReEchoWeaponRuntime::ApplyValueOperation(Step.RangeCm, Effect.ValueOp, Effect.Value));
		}
		else if (Effect.Target == TEXT("AttackArc"))
		{
			Step.ArcDegrees = FMath::Clamp(
			    ReEchoWeaponRuntime::ApplyValueOperation(Step.ArcDegrees, Effect.ValueOp, Effect.Value), 1.0f, 360.0f);
		}
		else if (Effect.Target == TEXT("ProjectileCount"))
		{
			Step.ProjectileCount =
			    FMath::Max(1,
			               FMath::RoundToInt(ReEchoWeaponRuntime::ApplyValueOperation(
			                   static_cast<float>(Step.ProjectileCount), Effect.ValueOp, Effect.Value)));
		}
		else if (Effect.Target == TEXT("ConcentrationDegrees"))
		{
			Step.ConcentrationDegrees = FMath::Clamp(
			    ReEchoWeaponRuntime::ApplyValueOperation(Step.ConcentrationDegrees, Effect.ValueOp, Effect.Value),
			    0.0f,
			    360.0f);
		}
		else if (Effect.Target == TEXT("ExplosionRadius"))
		{
			Step.ExplosionRadiusCm = FMath::Max(
			    0.0f, ReEchoWeaponRuntime::ApplyValueOperation(Step.ExplosionRadiusCm, Effect.ValueOp, Effect.Value));
		}
	}
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

bool ReEchoWeaponRuntime::IsRuntimeRuneBehaviorSupported(const FName BehaviorId)
{
	return RuntimeRuneBehaviorIds.Contains(BehaviorId);
}

int32 ReEchoWeaponRuntime::GetEffectiveSlotCapacity(const FReEchoCsvDataSnapshot& Snapshot,
                                                    const FReEchoBuildSnapshot& Build,
                                                    const FName WeaponTypeId,
                                                    const FName SlotTypeId)
{
	return FindEffectiveSlotLimit(Snapshot, Build, WeaponTypeId, SlotTypeId);
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
		const int32 SlotLimit = FindEffectiveSlotLimit(Snapshot, Candidate, Weapon->WeaponTypeId, Part->SlotTypeId);
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
		const int32 SlotLimit = FindEffectiveSlotLimit(Snapshot, Candidate, Weapon->WeaponTypeId, Part->SlotTypeId);
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
	ReadRuleFloat(
	    Build, DamageCoefficientRule, OutDefinition.Weapon.DamageCoefficient, OutDefinition.Weapon.DamageCoefficient);
	ReadRuleFloat(Build, WeaponRangeRule, OutDefinition.Weapon.RangeCm, OutDefinition.Weapon.RangeCm);
	ReadRuleFloat(Build, WeaponArcRule, OutDefinition.Weapon.ArcDegrees, OutDefinition.Weapon.ArcDegrees);
	{
		float ProjectileCount = static_cast<float>(OutDefinition.Weapon.ProjectileCount);
		ReadRuleFloat(Build, WeaponProjectileRule, ProjectileCount, ProjectileCount);
		OutDefinition.Weapon.ProjectileCount = FMath::RoundToInt(ProjectileCount);
	}
	ReadRuleFloat(Build,
	              WeaponConcentrationRule,
	              OutDefinition.Weapon.ConcentrationDegrees,
	              OutDefinition.Weapon.ConcentrationDegrees);
	ReadRuleFloat(
	    Build, WeaponExplosionRule, OutDefinition.Weapon.ExplosionRadiusCm, OutDefinition.Weapon.ExplosionRadiusCm);
	ReadRuleFloat(Build, OnKillHealRule, 0.0f, OutDefinition.OnKillHealPercent);
	for (const FReEchoEquippedPartSnapshot& EquippedPart : Build.EquippedParts)
	{
		const FReEchoCsvPartRow* Part = Snapshot.Parts.Find(EquippedPart.PartId);
		if (!Part || !Part->bEnabled)
		{
			continue;
		}
		for (const FReEchoCsvPartEffectRow& Effect : Part->Effects)
		{
			if (!Effect.bEnabled || Effect.EffectKind != TEXT("UniqueBehavior") ||
			    Effect.Target != TEXT("RuntimeBehavior"))
			{
				continue;
			}
			FReEchoWeaponRuneEffectSpec& Spec = OutDefinition.RuneEffects.AddDefaulted_GetRef();
			Spec.PartId = Part->PartId;
			Spec.Trigger = Effect.Trigger;
			Spec.BehaviorId = Effect.BehaviorId;
			Spec.ParamName = Effect.ParamName;
			Spec.StackPolicy = Effect.StackPolicy;
			Spec.Value = Effect.Value;
			Spec.ParamValue = Effect.ParamValue;
			Spec.DurationSeconds = Effect.DurationSeconds;
		}
	}
	OutDefinition.DamageChannelId = RuleNameOrDefault(Build, DamageChannelRule, NAME_None);

	if (OutDefinition.DamageChannelId.IsNone())
	{
		OutDefinition.bUsesCyclingElement = OutDefinition.Weapon.AttackPatternId == TEXT("Pattern.ElementalProjectile");
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
	for (const FReEchoEquippedPartSnapshot& EquippedPart : Build.EquippedParts)
	{
		const FReEchoCsvPartRow* Part = Snapshot.Parts.Find(EquippedPart.PartId);
		if (!Part || !Part->bEnabled)
		{
			continue;
		}
		for (const FReEchoCsvPartEffectRow& Effect : Part->Effects)
		{
			ApplyPartEffectToAttackSteps(Effect, OutDefinition.AttackSteps);
		}
	}
	OutError.Reset();
	return true;
}

FReEchoWeaponDefinition ReEchoWeaponRuntime::CompileLogicDefinition(const FReEchoEffectiveWeaponDefinition& Definition)
{
	FReEchoWeaponDefinition Result;
	Result.WeaponId = Definition.Weapon.Id;
	Result.AttackPatternId = Definition.Weapon.AttackPatternId;
	Result.DamageChannelId = Definition.DamageChannelId;
	Result.AttackIntervalSeconds = Definition.Weapon.AttackIntervalSeconds;
	Result.DamageCoefficient = Definition.Weapon.DamageCoefficient;
	Result.RangeCm = Definition.Weapon.RangeCm;
	Result.ArcDegrees = Definition.Weapon.ArcDegrees;
	Result.ProjectileCount = Definition.Weapon.ProjectileCount;
	Result.SpreadDegrees = Definition.Weapon.ConcentrationDegrees;
	Result.ExplosionRadiusCm = Definition.Weapon.ExplosionRadiusCm;
	Result.OnKillHealPercent = Definition.OnKillHealPercent;
	Result.bUsesCyclingElement = Definition.bUsesCyclingElement;
	Result.bUsesDeterministicRandomElement = Definition.bUsesDeterministicRandomElement;

	for (const FReEchoCsvAttackStepRow& CsvStep : Definition.AttackSteps)
	{
		FReEchoWeaponStepDefinition Step;
		Step.StepId = CsvStep.Id;
		Step.StepIndex = CsvStep.StepIndex;
		Step.DurationSeconds = CsvStep.DurationSeconds;
		Step.DamageCoefficient = CsvStep.DamageCoefficient;
		Step.RangeCm = CsvStep.RangeCm;
		Step.ArcDegrees = CsvStep.ArcDegrees;
		Step.ProjectileCount = CsvStep.ProjectileCount;
		Step.SpreadDegrees = CsvStep.ConcentrationDegrees;
		Step.ExplosionRadiusCm = CsvStep.ExplosionRadiusCm;
		Step.MovementCm = CsvStep.MovementCm;
		Step.OuterRingStartFraction = CsvStep.OuterRingStartFraction;
		Step.OuterRingBonusMultiplier = CsvStep.OuterRingBonusMultiplier;
		Step.bInvulnerable = CsvStep.bInvulnerable;
		if (Definition.Weapon.AttackPatternId == TEXT("Pattern.MoonStaffWave"))
		{
			Step.Carrier = EReEchoWeaponAttackCarrier::Wave;
		}
		else if (FMath::Max(Definition.Weapon.ProjectileCount, CsvStep.ProjectileCount) > 0 ||
		         Definition.Weapon.AttackPatternId == TEXT("Pattern.ElementalProjectile") ||
		         Definition.Weapon.AttackPatternId == TEXT("Pattern.BowShot") ||
		         Definition.Weapon.AttackPatternId == TEXT("Pattern.GunShot"))
		{
			Step.Carrier = EReEchoWeaponAttackCarrier::Projectile;
		}
		Result.AttackSteps.Add(Step);
	}
	return Result;
}

bool ReEchoWeaponRuntime::IsInsideMeleeArc(
    const FVector& Origin, const FVector& Forward, const FVector& Target, const float RangeCm, const float ArcDegrees)
{
	return ReEchoWeaponGeometry::IsInsideMeleeArc(Origin, Forward, Target, RangeCm, ArcDegrees);
}

TArray<FVector> ReEchoWeaponRuntime::BuildProjectileDirections(const FVector& Forward,
                                                               const int32 ProjectileCount,
                                                               const float SpreadDegrees)
{
	return ReEchoWeaponGeometry::BuildProjectileDirections(Forward, ProjectileCount, SpreadDegrees);
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

EReEchoElement ReEchoWeaponRuntime::ResolveProjectileElement(const bool bUsesDeterministicRandomElement,
                                                             const EReEchoElement AttackElement,
                                                             const int64 AttackSequence,
                                                             const int32 ProjectileIndex)
{
	if (!bUsesDeterministicRandomElement)
	{
		return AttackElement;
	}

	// Each projectile receives its own stable random draw. Keeping the seed derived from the attack identity and
	// projectile slot preserves replay determinism without making all projectiles in one attack share an element.
	uint32 Seed = GetTypeHash(AttackSequence) ^ (0x9E3779B9u * static_cast<uint32>(FMath::Max(0, ProjectileIndex) + 1));
	Seed ^= Seed >> 16;
	Seed *= 0x7FEB352Du;
	Seed ^= Seed >> 15;
	Seed *= 0x846CA68Bu;
	Seed ^= Seed >> 16;
	switch (Seed % 4u)
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
