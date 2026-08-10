#include "ReEchoWeaponCsvReader.h"

#include "Misc/Paths.h"

namespace ReEchoWeaponCsv
{
namespace
{
constexpr const TCHAR* WeaponTypesTableId = TEXT("WeaponTypes");
constexpr const TCHAR* WeaponsTableId = TEXT("Weapons");
constexpr const TCHAR* AttackStepsTableId = TEXT("AttackSteps");
constexpr const TCHAR* SlotTypesTableId = TEXT("SlotTypes");
constexpr const TCHAR* SlotProfilesTableId = TEXT("SlotProfiles");
constexpr const TCHAR* PartsTableId = TEXT("Parts");
constexpr const TCHAR* PartEffectsTableId = TEXT("PartEffects");
constexpr const TCHAR* NoneId = TEXT("None");

TArray<FName> ParseNameList(const FString& Text)
{
	TArray<FName> Result;
	TArray<FString> Parts;
	Text.ParseIntoArray(Parts, TEXT("|"), true);
	for (const FString& Part : Parts)
	{
		const FString Trimmed = Part.TrimStartAndEnd();
		if (!Trimmed.IsEmpty())
		{
			Result.Add(FName(*Trimmed));
		}
	}
	return Result;
}

bool RequireRegisteredAttackPattern(const ReEchoCsv::FTable& Table,
                                    const ReEchoCsv::FRow& Row,
                                    const FString& Field,
                                    const FName AttackPatternId,
                                    TArray<FReEchoCsvIssue>& Issues)
{
	if (!FReEchoCsvDataRegistry::IsAttackPatternIdRegistered(AttackPatternId))
	{
		ReEchoCsv::AddIssue(
		    Issues, Table.File, Row.Line, Field, TEXT("Unknown registered C++ attack pattern handler id"));
		return false;
	}
	return true;
}

bool ParseInputSlot(const ReEchoCsv::FTable& Table,
                    const ReEchoCsv::FRow& Row,
                    const FString& Field,
                    EReEchoInputSlot& OutSlot,
                    TArray<FReEchoCsvIssue>& Issues)
{
	FString Text;
	if (!ReEchoCsv::RequireCell(Table, Row, Field, Text, Issues))
	{
		return false;
	}
	if (Text == TEXT("None"))
	{
		OutSlot = EReEchoInputSlot::None;
		return true;
	}
	if (Text == TEXT("1"))
	{
		OutSlot = EReEchoInputSlot::Slot1;
		return true;
	}
	if (Text == TEXT("2"))
	{
		OutSlot = EReEchoInputSlot::Slot2;
		return true;
	}
	if (Text == TEXT("3"))
	{
		OutSlot = EReEchoInputSlot::Slot3;
		return true;
	}
	ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, Field, TEXT("InputSlot must be None, 1, 2 or 3"));
	return false;
}

bool IsAllowedWeaponEffectTarget(const FName Target)
{
	static const TSet<FName> AllowedTargets = {
	    TEXT("DamageChannel"),
	    TEXT("AttackSpeed"),
	    TEXT("AttackIntervalSeconds"),
	    TEXT("PhysicalCoefficient"),
	    TEXT("ElementalCoefficient"),
	    TEXT("AttackPattern"),
	    TEXT("OnKill"),
	};
	return AllowedTargets.Contains(Target);
}

bool IsAllowedPartEffectPair(const FName EffectKind, const FName BehaviorId)
{
	if (EffectKind == TEXT("WeaponDamageChannel"))
	{
		return BehaviorId == TEXT("Part.CoreDamageChannel");
	}
	if (EffectKind == TEXT("StatModifier"))
	{
		return BehaviorId == TEXT("Part.StatModifier");
	}
	if (EffectKind == TEXT("AttackPatternReplacement"))
	{
		return BehaviorId == TEXT("Part.AttackPatternReplacement");
	}
	if (EffectKind == TEXT("UniqueBehavior"))
	{
		return BehaviorId == TEXT("Part.OnKillHealPercent");
	}
	return false;
}

bool ReadWeaponTypesTable(const FString& DataDirectory,
                          const ReEchoCsv::FManifestEntry& Entry,
                          FReEchoCsvDataSnapshot& Snapshot,
                          TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::FTable Table;
	const FString TablePath = FPaths::Combine(DataDirectory, Entry.FileName);
	if (!ReEchoCsv::ParseCsvFile(TablePath, Table, Issues))
	{
		return false;
	}

	ReEchoCsv::HasExactColumns(Table,
	                           {TEXT("Id"),
	                            TEXT("DisplayName"),
	                            TEXT("BaseAttackPatternId"),
	                            TEXT("SlotProfileId"),
	                            TEXT("BaseIntervalSeconds"),
	                            TEXT("BaseRangeCm"),
	                            TEXT("BaseArcDegrees"),
	                            TEXT("BaseProjectileCount"),
	                            TEXT("BaseConcentrationDegrees"),
	                            TEXT("BaseExplosionRadiusCm"),
	                            TEXT("ChainWindowSeconds"),
	                            TEXT("Enabled"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow"),
	                            TEXT("DisabledReason")},
	                           Issues);

	TSet<FName> SeenIds;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvWeaponTypeRow WeaponType;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), WeaponType.Id, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("DisplayName"), WeaponType.DisplayName, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("BaseAttackPatternId"), WeaponType.BaseAttackPatternId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("SlotProfileId"), WeaponType.SlotProfileId, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("BaseIntervalSeconds"), 0.0f, 60.0f, WeaponType.BaseIntervalSeconds, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("BaseRangeCm"), 0.0f, 100000.0f, WeaponType.BaseRangeCm, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("BaseArcDegrees"), 0.0f, 360.0f, WeaponType.BaseArcDegrees, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("BaseProjectileCount"), WeaponType.BaseProjectileCount, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("BaseConcentrationDegrees"), 0.0f, 360.0f, WeaponType.BaseConcentrationDegrees, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("BaseExplosionRadiusCm"), 0.0f, 100000.0f, WeaponType.BaseExplosionRadiusCm, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("ChainWindowSeconds"), 0.0f, 60.0f, WeaponType.ChainWindowSeconds, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Enabled"), WeaponType.bEnabled, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("SourceSheet"), WeaponType.SourceSheet, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("SourceRow"), WeaponType.SourceRow, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("DisabledReason"), WeaponType.DisabledReason);

		if (SeenIds.Contains(WeaponType.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		RequireRegisteredAttackPattern(Table, Row, TEXT("BaseAttackPatternId"), WeaponType.BaseAttackPatternId, Issues);
		if (!WeaponType.bEnabled && WeaponType.DisabledReason.IsEmpty())
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("DisabledReason"), TEXT("Disabled source row requires a reason"));
		}
		SeenIds.Add(WeaponType.Id);
		Snapshot.WeaponTypeOrder.Add(WeaponType.Id);
		Snapshot.WeaponTypes.Add(WeaponType.Id, WeaponType);
	}
	return Issues.Num() == 0;
}

bool ReadWeaponsTable(const FString& DataDirectory,
                      const ReEchoCsv::FManifestEntry& Entry,
                      FReEchoCsvDataSnapshot& Snapshot,
                      TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::FTable Table;
	const FString TablePath = FPaths::Combine(DataDirectory, Entry.FileName);
	if (!ReEchoCsv::ParseCsvFile(TablePath, Table, Issues))
	{
		return false;
	}

	ReEchoCsv::HasExactColumns(Table,
	                           {TEXT("Id"),
	                            TEXT("WeaponTypeId"),
	                            TEXT("DisplayName"),
	                            TEXT("VisualKey"),
	                            TEXT("InputSlot"),
	                            TEXT("StartSelectable"),
	                            TEXT("LoadoutOrder"),
	                            TEXT("AttackPatternId"),
	                            TEXT("AttackIntervalSeconds"),
	                            TEXT("PhysicalCoefficient"),
	                            TEXT("ElementalCoefficient"),
	                            TEXT("RangeCm"),
	                            TEXT("ArcDegrees"),
	                            TEXT("ProjectileCount"),
	                            TEXT("ConcentrationDegrees"),
	                            TEXT("ExplosionRadiusCm"),
	                            TEXT("DataRevision"),
	                            TEXT("Enabled"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow"),
	                            TEXT("DisabledReason")},
	                           Issues);

	TSet<FName> SeenIds;
	TSet<EReEchoInputSlot> SeenInputSlots;
	TSet<int32> SeenLoadoutOrders;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvWeaponRow Weapon;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), Weapon.Id, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("WeaponTypeId"), Weapon.WeaponTypeId, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("DisplayName"), Weapon.DisplayName, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("VisualKey"), Weapon.VisualKey, Issues);
		ParseInputSlot(Table, Row, TEXT("InputSlot"), Weapon.InputSlot, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("StartSelectable"), Weapon.bStartSelectable, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("LoadoutOrder"), Weapon.LoadoutOrder, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("AttackPatternId"), Weapon.AttackPatternId, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("AttackIntervalSeconds"), 0.01f, 60.0f, Weapon.AttackIntervalSeconds, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("PhysicalCoefficient"), 0.0f, 100.0f, Weapon.PhysicalCoefficient, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("ElementalCoefficient"), 0.0f, 100.0f, Weapon.ElementalCoefficient, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("RangeCm"), 0.0f, 100000.0f, Weapon.RangeCm, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("ArcDegrees"), 0.0f, 360.0f, Weapon.ArcDegrees, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("ProjectileCount"), Weapon.ProjectileCount, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("ConcentrationDegrees"), 0.0f, 360.0f, Weapon.ConcentrationDegrees, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("ExplosionRadiusCm"), 0.0f, 100000.0f, Weapon.ExplosionRadiusCm, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("DataRevision"), Weapon.DataRevision, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Enabled"), Weapon.bEnabled, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("SourceSheet"), Weapon.SourceSheet, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("SourceRow"), Weapon.SourceRow, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("DisabledReason"), Weapon.DisabledReason);

		if (SeenIds.Contains(Weapon.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (!Snapshot.WeaponTypes.Contains(Weapon.WeaponTypeId))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("WeaponTypeId"), TEXT("Unknown WeaponTypes.Id reference"));
		}
		RequireRegisteredAttackPattern(Table, Row, TEXT("AttackPatternId"), Weapon.AttackPatternId, Issues);
		if (Weapon.InputSlot != EReEchoInputSlot::None)
		{
			if (SeenInputSlots.Contains(Weapon.InputSlot))
			{
				ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("InputSlot"), TEXT("Duplicate InputSlot"));
			}
			SeenInputSlots.Add(Weapon.InputSlot);
		}
		if (Weapon.bStartSelectable)
		{
			if (Weapon.InputSlot == EReEchoInputSlot::None)
			{
				ReEchoCsv::AddIssue(
				    Issues, Table.File, Row.Line, TEXT("InputSlot"), TEXT("StartSelectable weapon needs InputSlot"));
			}
			if (Weapon.LoadoutOrder < 1 || Weapon.LoadoutOrder > 3)
			{
				ReEchoCsv::AddIssue(
				    Issues, Table.File, Row.Line, TEXT("LoadoutOrder"), TEXT("StartSelectable LoadoutOrder must be 1..3"));
			}
			if (SeenLoadoutOrders.Contains(Weapon.LoadoutOrder))
			{
				ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("LoadoutOrder"), TEXT("Duplicate LoadoutOrder"));
			}
			SeenLoadoutOrders.Add(Weapon.LoadoutOrder);
		}
		if (!Weapon.bEnabled && Weapon.DisabledReason.IsEmpty())
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("DisabledReason"), TEXT("Disabled source row requires a reason"));
		}

		SeenIds.Add(Weapon.Id);
		Snapshot.WeaponOrder.Add(Weapon.Id);
		Snapshot.Weapons.Add(Weapon.Id, Weapon);
	}
	return Issues.Num() == 0;
}

bool ReadAttackStepsTable(const FString& DataDirectory,
                          const ReEchoCsv::FManifestEntry& Entry,
                          FReEchoCsvDataSnapshot& Snapshot,
                          TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::FTable Table;
	const FString TablePath = FPaths::Combine(DataDirectory, Entry.FileName);
	if (!ReEchoCsv::ParseCsvFile(TablePath, Table, Issues))
	{
		return false;
	}

	ReEchoCsv::HasExactColumns(Table,
	                           {TEXT("Id"),
	                            TEXT("AttackPatternId"),
	                            TEXT("StepIndex"),
	                            TEXT("DurationSeconds"),
	                            TEXT("PhysicalCoefficient"),
	                            TEXT("ElementalCoefficient"),
	                            TEXT("RangeCm"),
	                            TEXT("ArcDegrees"),
	                            TEXT("ProjectileCount"),
	                            TEXT("ConcentrationDegrees"),
	                            TEXT("ExplosionRadiusCm"),
	                            TEXT("MovementCm"),
	                            TEXT("Invulnerable"),
	                            TEXT("BehaviorId"),
	                            TEXT("FormulaId"),
	                            TEXT("ConditionId"),
	                            TEXT("Enabled"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow"),
	                            TEXT("DisabledReason")},
	                           Issues);

	TSet<FName> SeenIds;
	TSet<FString> SeenPatternSteps;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvAttackStepRow Step;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), Step.Id, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("AttackPatternId"), Step.AttackPatternId, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("StepIndex"), Step.StepIndex, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("DurationSeconds"), 0.0f, 60.0f, Step.DurationSeconds, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("PhysicalCoefficient"), 0.0f, 100.0f, Step.PhysicalCoefficient, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("ElementalCoefficient"), 0.0f, 100.0f, Step.ElementalCoefficient, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("RangeCm"), 0.0f, 100000.0f, Step.RangeCm, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("ArcDegrees"), 0.0f, 360.0f, Step.ArcDegrees, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("ProjectileCount"), Step.ProjectileCount, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("ConcentrationDegrees"), 0.0f, 360.0f, Step.ConcentrationDegrees, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("ExplosionRadiusCm"), 0.0f, 100000.0f, Step.ExplosionRadiusCm, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("MovementCm"), 0.0f, 100000.0f, Step.MovementCm, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Invulnerable"), Step.bInvulnerable, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("BehaviorId"), Step.BehaviorId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("FormulaId"), Step.FormulaId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("ConditionId"), Step.ConditionId, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Enabled"), Step.bEnabled, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("SourceSheet"), Step.SourceSheet, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("SourceRow"), Step.SourceRow, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("DisabledReason"), Step.DisabledReason);

		if (SeenIds.Contains(Step.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		const FString PatternStepKey =
		    FString::Printf(TEXT("%s:%d"), *Step.AttackPatternId.ToString(), Step.StepIndex);
		if (SeenPatternSteps.Contains(PatternStepKey))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("StepIndex"), TEXT("Duplicate AttackPatternId/StepIndex"));
		}
		RequireRegisteredAttackPattern(Table, Row, TEXT("AttackPatternId"), Step.AttackPatternId, Issues);
		if (!FReEchoCsvDataRegistry::IsBehaviorIdRegistered(Step.BehaviorId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("BehaviorId"), TEXT("Unknown registered C++ behavior id"));
		}
		if (!FReEchoCsvDataRegistry::IsFormulaIdRegistered(Step.FormulaId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("FormulaId"), TEXT("Unknown registered C++ formula id"));
		}
		if (!Step.bEnabled && Step.DisabledReason.IsEmpty())
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("DisabledReason"), TEXT("Disabled source row requires a reason"));
		}
		SeenIds.Add(Step.Id);
		SeenPatternSteps.Add(PatternStepKey);
		Snapshot.AttackStepOrder.Add(Step.Id);
		Snapshot.AttackSteps.Add(Step.Id, Step);
	}
	return Issues.Num() == 0;
}

bool ReadSlotTypesTable(const FString& DataDirectory,
                        const ReEchoCsv::FManifestEntry& Entry,
                        FReEchoCsvDataSnapshot& Snapshot,
                        TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::FTable Table;
	const FString TablePath = FPaths::Combine(DataDirectory, Entry.FileName);
	if (!ReEchoCsv::ParseCsvFile(TablePath, Table, Issues))
	{
		return false;
	}
	ReEchoCsv::HasExactColumns(
	    Table, {TEXT("Id"), TEXT("DisplayName"), TEXT("Enabled"), TEXT("SourceSheet"), TEXT("SourceRow"), TEXT("DisabledReason")}, Issues);

	TSet<FName> SeenIds;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvSlotTypeRow SlotType;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), SlotType.Id, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("DisplayName"), SlotType.DisplayName, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Enabled"), SlotType.bEnabled, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("SourceSheet"), SlotType.SourceSheet, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("SourceRow"), SlotType.SourceRow, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("DisabledReason"), SlotType.DisabledReason);
		if (SeenIds.Contains(SlotType.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (!SlotType.bEnabled && SlotType.DisabledReason.IsEmpty())
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("DisabledReason"), TEXT("Disabled source row requires a reason"));
		}
		SeenIds.Add(SlotType.Id);
		Snapshot.SlotTypes.Add(SlotType.Id, SlotType);
	}
	return Issues.Num() == 0;
}

bool ReadSlotProfilesTable(const FString& DataDirectory,
                           const ReEchoCsv::FManifestEntry& Entry,
                           FReEchoCsvDataSnapshot& Snapshot,
                           TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::FTable Table;
	const FString TablePath = FPaths::Combine(DataDirectory, Entry.FileName);
	if (!ReEchoCsv::ParseCsvFile(TablePath, Table, Issues))
	{
		return false;
	}
	ReEchoCsv::HasExactColumns(Table,
	                           {TEXT("Id"),
	                            TEXT("WeaponTypeId"),
	                            TEXT("SlotTypeId"),
	                            TEXT("SlotCount"),
	                            TEXT("Required"),
	                            TEXT("Enabled"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow"),
	                            TEXT("DisabledReason")},
	                           Issues);

	TSet<FName> SeenIds;
	TSet<FString> SeenCombinations;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvSlotProfileRow SlotProfile;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), SlotProfile.Id, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("WeaponTypeId"), SlotProfile.WeaponTypeId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("SlotTypeId"), SlotProfile.SlotTypeId, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("SlotCount"), SlotProfile.SlotCount, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Required"), SlotProfile.bRequired, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Enabled"), SlotProfile.bEnabled, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("SourceSheet"), SlotProfile.SourceSheet, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("SourceRow"), SlotProfile.SourceRow, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("DisabledReason"), SlotProfile.DisabledReason);
		if (SeenIds.Contains(SlotProfile.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		const FString CombinationKey =
		    FString::Printf(TEXT("%s:%s"), *SlotProfile.WeaponTypeId.ToString(), *SlotProfile.SlotTypeId.ToString());
		if (SeenCombinations.Contains(CombinationKey))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("SlotTypeId"), TEXT("Duplicate WeaponTypeId/SlotTypeId"));
		}
		if (!Snapshot.WeaponTypes.Contains(SlotProfile.WeaponTypeId))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("WeaponTypeId"), TEXT("Unknown WeaponTypes.Id reference"));
		}
		if (!Snapshot.SlotTypes.Contains(SlotProfile.SlotTypeId))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("SlotTypeId"), TEXT("Unknown SlotTypes.Id reference"));
		}
		if (!SlotProfile.bEnabled && SlotProfile.DisabledReason.IsEmpty())
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("DisabledReason"), TEXT("Disabled source row requires a reason"));
		}
		SeenIds.Add(SlotProfile.Id);
		SeenCombinations.Add(CombinationKey);
		Snapshot.SlotProfiles.Add(SlotProfile.Id, SlotProfile);
	}
	return Issues.Num() == 0;
}

bool ReadPartsTable(const FString& DataDirectory,
                    const ReEchoCsv::FManifestEntry& Entry,
                    FReEchoCsvDataSnapshot& Snapshot,
                    TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::FTable Table;
	const FString TablePath = FPaths::Combine(DataDirectory, Entry.FileName);
	if (!ReEchoCsv::ParseCsvFile(TablePath, Table, Issues))
	{
		return false;
	}
	ReEchoCsv::HasExactColumns(Table,
	                           {TEXT("Id"),
	                            TEXT("PartId"),
	                            TEXT("WeaponTypeId"),
	                            TEXT("SlotTypeId"),
	                            TEXT("DisplayName"),
	                            TEXT("Description"),
	                            TEXT("Rarity"),
	                            TEXT("Tags"),
	                            TEXT("Enabled"),
	                            TEXT("ReviewStatus"),
	                            TEXT("ImplementationStatus"),
	                            TEXT("DisabledReason"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow")},
	                           Issues);

	TSet<FName> SeenIds;
	int32 NamedRows = 0;
	int32 UnnamedDisabledRows = 0;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvPartRow Part;
		FString Tags;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), Part.Id, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("PartId"), Part.PartId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("WeaponTypeId"), Part.WeaponTypeId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("SlotTypeId"), Part.SlotTypeId, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("DisplayName"), Part.DisplayName);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("Description"), Part.Description);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Rarity"), Part.Rarity, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("Tags"), Tags, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Enabled"), Part.bEnabled, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("ReviewStatus"), Part.ReviewStatus, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("ImplementationStatus"), Part.ImplementationStatus, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("DisabledReason"), Part.DisabledReason);
		ReEchoCsv::RequireCell(Table, Row, TEXT("SourceSheet"), Part.SourceSheet, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("SourceRow"), Part.SourceRow, Issues);
		Part.Tags = ParseNameList(Tags);

		if (SeenIds.Contains(Part.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (!Part.DisplayName.IsEmpty())
		{
			++NamedRows;
		}
		else if (!Part.bEnabled && Part.PartId == NoneId)
		{
			++UnnamedDisabledRows;
		}
		if (!Snapshot.WeaponTypes.Contains(Part.WeaponTypeId) && Part.WeaponTypeId != TEXT("Any"))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("WeaponTypeId"), TEXT("Unknown WeaponTypes.Id reference"));
		}
		if (!Snapshot.SlotTypes.Contains(Part.SlotTypeId))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("SlotTypeId"), TEXT("Unknown SlotTypes.Id reference"));
		}
		if (Part.bEnabled)
		{
			if (Part.PartId == NoneId || Part.DisplayName.IsEmpty())
			{
				ReEchoCsv::AddIssue(
				    Issues, Table.File, Row.Line, TEXT("PartId"), TEXT("Enabled part needs a stable PartId and name"));
			}
			if (Part.ReviewStatus != TEXT("Approved") || Part.ImplementationStatus != TEXT("Implemented"))
			{
				ReEchoCsv::AddIssue(
				    Issues, Table.File, Row.Line, TEXT("ImplementationStatus"), TEXT("Enabled part must be approved and implemented"));
			}
		}
		else if (Part.DisabledReason.IsEmpty())
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("DisabledReason"), TEXT("Disabled source row requires a reason"));
		}

		SeenIds.Add(Part.Id);
		Snapshot.Parts.Add(Part.Id, Part);
	}
	if (Table.Rows.Num() != 78)
	{
		ReEchoCsv::AddIssue(Issues, Table.File, 1, TEXT("SourceRow"), TEXT("Weapon slot audit must contain 78 source rows"));
	}
	if (NamedRows != 16 || UnnamedDisabledRows != 62)
	{
		ReEchoCsv::AddIssue(Issues,
		                    Table.File,
		                    1,
		                    TEXT("DisplayName"),
		                    TEXT("Weapon slot audit must contain 16 named rows and 62 unnamed disabled rows"));
	}
	return Issues.Num() == 0;
}

bool ReadPartEffectsTable(const FString& DataDirectory,
                          const ReEchoCsv::FManifestEntry& Entry,
                          FReEchoCsvDataSnapshot& Snapshot,
                          TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::FTable Table;
	const FString TablePath = FPaths::Combine(DataDirectory, Entry.FileName);
	if (!ReEchoCsv::ParseCsvFile(TablePath, Table, Issues))
	{
		return false;
	}
	ReEchoCsv::HasExactColumns(Table,
	                           {TEXT("Id"),
	                            TEXT("PartId"),
	                            TEXT("Order"),
	                            TEXT("Trigger"),
	                            TEXT("EffectKind"),
	                            TEXT("Target"),
	                            TEXT("ValueOp"),
	                            TEXT("Value"),
	                            TEXT("BehaviorId"),
	                            TEXT("FormulaId"),
	                            TEXT("AttackPatternId"),
	                            TEXT("ParamName"),
	                            TEXT("ParamValue"),
	                            TEXT("DurationSeconds"),
	                            TEXT("CooldownSeconds"),
	                            TEXT("StackPolicy"),
	                            TEXT("Enabled"),
	                            TEXT("DisabledReason"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow")},
	                           Issues);

	TSet<FName> SeenIds;
	TSet<FString> SeenPartOrders;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvPartEffectRow Effect;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), Effect.Id, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("PartId"), Effect.PartId, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("Order"), Effect.Order, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Trigger"), Effect.Trigger, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("EffectKind"), Effect.EffectKind, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Target"), Effect.Target, Issues);
		ReEchoCsv::RequireValueOp(Table, Row, TEXT("ValueOp"), Effect.ValueOp, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("Value"), -100000.0f, 100000.0f, Effect.Value, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("BehaviorId"), Effect.BehaviorId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("FormulaId"), Effect.FormulaId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("AttackPatternId"), Effect.AttackPatternId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("ParamName"), Effect.ParamName, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("ParamValue"), -100000.0f, 100000.0f, Effect.ParamValue, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("DurationSeconds"), 0.0f, 3600.0f, Effect.DurationSeconds, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("CooldownSeconds"), 0.0f, 3600.0f, Effect.CooldownSeconds, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("StackPolicy"), Effect.StackPolicy, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Enabled"), Effect.bEnabled, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("DisabledReason"), Effect.DisabledReason);
		ReEchoCsv::RequireCell(Table, Row, TEXT("SourceSheet"), Effect.SourceSheet, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("SourceRow"), Effect.SourceRow, Issues);

		if (SeenIds.Contains(Effect.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		const FString PartOrderKey = FString::Printf(TEXT("%s:%d"), *Effect.PartId.ToString(), Effect.Order);
		if (SeenPartOrders.Contains(PartOrderKey))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Order"), TEXT("Duplicate PartId/Order"));
		}
		if (!IsAllowedWeaponEffectTarget(Effect.Target))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Target"), TEXT("Unsupported weapon effect target"));
		}
		if (!FReEchoCsvDataRegistry::IsEffectKindRegistered(Effect.EffectKind))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("EffectKind"), TEXT("Unknown registered C++ effect kind"));
		}
		if (!FReEchoCsvDataRegistry::IsBehaviorIdRegistered(Effect.BehaviorId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("BehaviorId"), TEXT("Unknown registered C++ behavior id"));
		}
		if (!FReEchoCsvDataRegistry::IsFormulaIdRegistered(Effect.FormulaId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("FormulaId"), TEXT("Unknown registered C++ formula id"));
		}
		RequireRegisteredAttackPattern(Table, Row, TEXT("AttackPatternId"), Effect.AttackPatternId, Issues);
		if (!IsAllowedPartEffectPair(Effect.EffectKind, Effect.BehaviorId))
		{
			ReEchoCsv::AddIssue(Issues,
			                    Table.File,
			                    Row.Line,
			                    TEXT("BehaviorId"),
			                    TEXT("EffectKind/BehaviorId combination is invalid"));
		}
		FReEchoCsvPartRow* Part = Snapshot.Parts.Find(Effect.PartId);
		if (!Part)
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("PartId"), TEXT("Unknown Parts.Id reference"));
		}
		else
		{
			if (Effect.bEnabled && !Part->bEnabled)
			{
				ReEchoCsv::AddIssue(
				    Issues, Table.File, Row.Line, TEXT("PartId"), TEXT("Enabled effect references disabled part"));
			}
			Part->Effects.Add(Effect);
		}
		if (!Effect.bEnabled && Effect.DisabledReason.IsEmpty())
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("DisabledReason"), TEXT("Disabled source row requires a reason"));
		}
		SeenIds.Add(Effect.Id);
		SeenPartOrders.Add(PartOrderKey);
	}

	int32 EnabledCores = 0;
	bool bHasStrengthGrip = false;
	bool bHasPatternReplacement = false;
	bool bHasUniqueBehavior = false;
	for (auto& PartPair : Snapshot.Parts)
	{
		FReEchoCsvPartRow& Part = PartPair.Value;
		Part.Effects.Sort(
		    [](const FReEchoCsvPartEffectRow& Left, const FReEchoCsvPartEffectRow& Right)
		    {
			    return Left.Order < Right.Order;
		    });
		if (Part.bEnabled && Part.Effects.Num() == 0)
		{
			ReEchoCsv::AddIssue(Issues,
			                    Table.File,
			                    1,
			                    TEXT("PartId"),
			                    FString::Printf(TEXT("Enabled part %s has no effect rows"), *Part.Id.ToString()));
		}
		if (Part.bEnabled && Part.SlotTypeId == TEXT("Core"))
		{
			++EnabledCores;
		}
		bHasStrengthGrip |= Part.bEnabled && Part.Id == TEXT("P_DAGGER_STRENGTH_GRIP");
		for (const FReEchoCsvPartEffectRow& Effect : Part.Effects)
		{
			bHasPatternReplacement |= Effect.bEnabled && Effect.EffectKind == TEXT("AttackPatternReplacement");
			bHasUniqueBehavior |= Effect.bEnabled && Effect.EffectKind == TEXT("UniqueBehavior");
		}
	}
	if (EnabledCores < 6)
	{
		ReEchoCsv::AddIssue(Issues, Table.File, 1, TEXT("PartId"), TEXT("At least six generic cores must be enabled"));
	}
	if (!bHasStrengthGrip)
	{
		ReEchoCsv::AddIssue(Issues, Table.File, 1, TEXT("PartId"), TEXT("Strength grip must be enabled"));
	}
	if (!bHasPatternReplacement)
	{
		ReEchoCsv::AddIssue(Issues, Table.File, 1, TEXT("EffectKind"), TEXT("At least one AttackPatternReplacement must be enabled"));
	}
	if (!bHasUniqueBehavior)
	{
		ReEchoCsv::AddIssue(Issues, Table.File, 1, TEXT("EffectKind"), TEXT("At least one UniqueBehavior must be enabled"));
	}
	return Issues.Num() == 0;
}

void ValidateCrossDomain(FReEchoCsvDataSnapshot& Snapshot, TArray<FReEchoCsvIssue>& Issues)
{
	for (const TPair<FName, FReEchoCsvCharacterRow>& Pair : Snapshot.Characters)
	{
		const FReEchoCsvCharacterRow& Character = Pair.Value;
		if (!Character.bEnabled)
		{
			continue;
		}
		const FReEchoCsvWeaponRow* Weapon = Snapshot.FindEnabledWeapon(Character.DefaultWeaponId);
		if (!Weapon)
		{
			ReEchoCsv::AddIssue(Issues,
			                    TEXT("characters.csv"),
			                    1,
			                    TEXT("DefaultWeaponId"),
			                    FString::Printf(TEXT("Enabled character %s references unknown or disabled weapon %s"),
			                                    *Character.Id.ToString(),
			                                    *Character.DefaultWeaponId.ToString()));
		}
	}

	const TArray<FReEchoCsvWeaponRow> StartSelectable = Snapshot.GetStartSelectableWeapons();
	if (StartSelectable.Num() != 3)
	{
		ReEchoCsv::AddIssue(
		    Issues, TEXT("weapons.csv"), 1, TEXT("StartSelectable"), TEXT("Exactly three weapons must be StartSelectable"));
	}
	const TArray<FName> ExpectedLoadout = {TEXT("W_J_02"), TEXT("W_J_01"), TEXT("W_J_03")};
	for (int32 Index = 0; Index < StartSelectable.Num() && Index < ExpectedLoadout.Num(); ++Index)
	{
		if (StartSelectable[Index].Id != ExpectedLoadout[Index])
		{
			ReEchoCsv::AddIssue(
			    Issues, TEXT("weapons.csv"), 1, TEXT("LoadoutOrder"), TEXT("Legacy start weapon order changed"));
		}
	}
	const FReEchoCsvWeaponRow* Slot1 = Snapshot.FindWeaponByInputSlot(EReEchoInputSlot::Slot1);
	const FReEchoCsvWeaponRow* Slot2 = Snapshot.FindWeaponByInputSlot(EReEchoInputSlot::Slot2);
	const FReEchoCsvWeaponRow* Slot3 = Snapshot.FindWeaponByInputSlot(EReEchoInputSlot::Slot3);
	if (!Slot1 || Slot1->Id != TEXT("W_J_02") || !Slot2 || Slot2->Id != TEXT("W_J_01") || !Slot3 ||
	    Slot3->Id != TEXT("W_J_03"))
	{
		ReEchoCsv::AddIssue(
		    Issues, TEXT("weapons.csv"), 1, TEXT("InputSlot"), TEXT("Legacy W_J_02/W_J_01/W_J_03 input mapping changed"));
	}
}
}

bool ReadTables(const FString& DataDirectory,
                const TMap<FString, ReEchoCsv::FManifestEntry>& ManifestEntries,
                FReEchoCsvDataSnapshot& Snapshot,
                TArray<FReEchoCsvIssue>& Issues)
{
	ReadWeaponTypesTable(DataDirectory, ManifestEntries[WeaponTypesTableId], Snapshot, Issues);
	if (Issues.Num() == 0)
	{
		ReadWeaponsTable(DataDirectory, ManifestEntries[WeaponsTableId], Snapshot, Issues);
	}
	if (Issues.Num() == 0)
	{
		ReadAttackStepsTable(DataDirectory, ManifestEntries[AttackStepsTableId], Snapshot, Issues);
	}
	if (Issues.Num() == 0)
	{
		ReadSlotTypesTable(DataDirectory, ManifestEntries[SlotTypesTableId], Snapshot, Issues);
	}
	if (Issues.Num() == 0)
	{
		ReadSlotProfilesTable(DataDirectory, ManifestEntries[SlotProfilesTableId], Snapshot, Issues);
	}
	if (Issues.Num() == 0)
	{
		ReadPartsTable(DataDirectory, ManifestEntries[PartsTableId], Snapshot, Issues);
	}
	if (Issues.Num() == 0)
	{
		ReadPartEffectsTable(DataDirectory, ManifestEntries[PartEffectsTableId], Snapshot, Issues);
	}
	if (Issues.Num() == 0)
	{
		ValidateCrossDomain(Snapshot, Issues);
	}
	return Issues.Num() == 0;
}
}
