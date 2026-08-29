#include "ReEchoWeaponCsvReader.h"

#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

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
constexpr const TCHAR* ShopPriceRangesTableId = TEXT("shop_price_ranges");
constexpr const TCHAR* ShopDropLevelsTableId = TEXT("shop_drop_levels");
constexpr const TCHAR* ShopRefreshRulesTableId = TEXT("shop_refresh_rules");
constexpr const TCHAR* RuneUpgradesTableId = TEXT("RuneUpgrades");
constexpr const TCHAR* NoneId = TEXT("None");
constexpr int32 MaxStartSelectableLoadoutOrder = static_cast<int32>(EReEchoInputSlot::Slot6);
constexpr int32 ExpectedPartSourceRows = 111;

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
	if (Text == TEXT("4"))
	{
		OutSlot = EReEchoInputSlot::Slot4;
		return true;
	}
	if (Text == TEXT("5"))
	{
		OutSlot = EReEchoInputSlot::Slot5;
		return true;
	}
	if (Text == TEXT("6"))
	{
		OutSlot = EReEchoInputSlot::Slot6;
		return true;
	}
	ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, Field, TEXT("InputSlot must be None, 1, 2, 3, 4, 5 or 6"));
	return false;
}

bool IsAllowedWeaponEffectTarget(const FName Target)
{
	static const TSet<FName> AllowedTargets = {
	    TEXT("DamageChannel"),
	    TEXT("AttackSpeed"),
	    TEXT("AttackIntervalSeconds"),
	    TEXT("DamageCoefficient"),
	    TEXT("AttackPattern"),
	    TEXT("OnKill"),
	    // Weapon-parameter StatModifier targets. Each one is landed by
	    // ReEchoWeaponRuntime::ApplyPartEffect as a RuleFlag and read back by
	    // BuildEffectiveWeaponDefinition. Keep in sync with validate_project.py
	    // WEAPON_EFFECT_TARGETS.
	    TEXT("AttackRange"),
	    TEXT("AttackArc"),
	    TEXT("ProjectileCount"),
	    TEXT("ConcentrationDegrees"),
	    TEXT("ExplosionRadius"),
	    TEXT("RuntimeBehavior"),
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
		return BehaviorId.ToString().StartsWith(TEXT("Part.")) && BehaviorId != TEXT("Part.CoreDamageChannel") &&
		       BehaviorId != TEXT("Part.StatModifier") && BehaviorId != TEXT("Part.AttackPatternReplacement");
	}
	return false;
}

void AppendCanonicalField(FString& Out, const FString& Value)
{
	Out += Value.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("|"), TEXT("\\|")).Replace(TEXT("\n"), TEXT("\\n"));
	Out += TEXT("|");
}

void AppendCanonicalField(FString& Out, const FName Value)
{
	AppendCanonicalField(Out, Value.ToString());
}

void AppendCanonicalField(FString& Out, const bool Value)
{
	AppendCanonicalField(Out, FString(Value ? TEXT("true") : TEXT("false")));
}

void AppendCanonicalField(FString& Out, const int32 Value)
{
	AppendCanonicalField(Out, FString::FromInt(Value));
}

void AppendCanonicalField(FString& Out, const float Value)
{
	AppendCanonicalField(Out, FString::SanitizeFloat(Value));
}

void AppendRowEnd(FString& Out)
{
	Out += TEXT("\n");
}

template <typename KeyType, typename RowType, typename AppendFunc>
void AppendOrderedRows(FString& Out,
                       const FString& TableName,
                       const TArray<KeyType>& Order,
                       const TMap<KeyType, RowType>& Rows,
                       AppendFunc Append)
{
	Out += TEXT("[");
	Out += TableName;
	Out += TEXT("]\n");
	for (const KeyType& RowId : Order)
	{
		if (const RowType* Row = Rows.Find(RowId))
		{
			Append(*Row);
			AppendRowEnd(Out);
		}
	}
}

FString ComputeWeaponDomainRevision(const FReEchoCsvDataSnapshot& Snapshot)
{
	FString Canonical;
	AppendOrderedRows(Canonical,
	                  TEXT("weapon_types"),
	                  Snapshot.WeaponTypeOrder,
	                  Snapshot.WeaponTypes,
	                  [&](const FReEchoCsvWeaponTypeRow& Row)
	                  {
		                  AppendCanonicalField(Canonical, Row.Id);
		                  AppendCanonicalField(Canonical, Row.DisplayName);
		                  AppendCanonicalField(Canonical, Row.Description);
		                  AppendCanonicalField(Canonical, Row.BaseAttackPatternId);
		                  AppendCanonicalField(Canonical, Row.SlotProfileId);
		                  AppendCanonicalField(Canonical, Row.BaseIntervalSeconds);
		                  AppendCanonicalField(Canonical, Row.BaseRangeCm);
		                  AppendCanonicalField(Canonical, Row.BaseArcDegrees);
		                  AppendCanonicalField(Canonical, Row.BaseProjectileCount);
		                  AppendCanonicalField(Canonical, Row.BaseConcentrationDegrees);
		                  AppendCanonicalField(Canonical, Row.BaseExplosionRadiusCm);
		                  AppendCanonicalField(Canonical, Row.ChainWindowSeconds);
		                  AppendCanonicalField(Canonical, Row.bEnabled);
		                  AppendCanonicalField(Canonical, Row.SourceSheet);
		                  AppendCanonicalField(Canonical, Row.SourceRow);
		                  AppendCanonicalField(Canonical, Row.DisabledReason);
	                  });
	AppendOrderedRows(Canonical,
	                  TEXT("weapons"),
	                  Snapshot.WeaponOrder,
	                  Snapshot.Weapons,
	                  [&](const FReEchoCsvWeaponRow& Row)
	                  {
		                  AppendCanonicalField(Canonical, Row.Id);
		                  AppendCanonicalField(Canonical, Row.WeaponTypeId);
		                  AppendCanonicalField(Canonical, Row.DisplayName);
		                  AppendCanonicalField(Canonical, Row.VisualKey);
		                  AppendCanonicalField(Canonical, static_cast<int32>(Row.InputSlot));
		                  AppendCanonicalField(Canonical, Row.bStartSelectable);
		                  AppendCanonicalField(Canonical, Row.LoadoutOrder);
		                  AppendCanonicalField(Canonical, Row.AttackPatternId);
		                  AppendCanonicalField(Canonical, Row.AttackIntervalSeconds);
		                  AppendCanonicalField(Canonical, Row.DamageCoefficient);
		                  AppendCanonicalField(Canonical, Row.RangeCm);
		                  AppendCanonicalField(Canonical, Row.ArcDegrees);
		                  AppendCanonicalField(Canonical, Row.ProjectileCount);
		                  AppendCanonicalField(Canonical, Row.ConcentrationDegrees);
		                  AppendCanonicalField(Canonical, Row.ExplosionRadiusCm);
		                  AppendCanonicalField(Canonical, Row.DataRevision);
		                  AppendCanonicalField(Canonical, Row.bEnabled);
		                  AppendCanonicalField(Canonical, Row.SourceSheet);
		                  AppendCanonicalField(Canonical, Row.SourceRow);
		                  AppendCanonicalField(Canonical, Row.DisabledReason);
	                  });
	AppendOrderedRows(Canonical,
	                  TEXT("attack_steps"),
	                  Snapshot.AttackStepOrder,
	                  Snapshot.AttackSteps,
	                  [&](const FReEchoCsvAttackStepRow& Row)
	                  {
		                  AppendCanonicalField(Canonical, Row.Id);
		                  AppendCanonicalField(Canonical, Row.AttackPatternId);
		                  AppendCanonicalField(Canonical, Row.StepIndex);
		                  AppendCanonicalField(Canonical, Row.DurationSeconds);
		                  AppendCanonicalField(Canonical, Row.DamageCoefficient);
		                  AppendCanonicalField(Canonical, Row.RangeCm);
		                  AppendCanonicalField(Canonical, Row.ArcDegrees);
		                  AppendCanonicalField(Canonical, Row.ProjectileCount);
		                  AppendCanonicalField(Canonical, Row.ConcentrationDegrees);
		                  AppendCanonicalField(Canonical, Row.ExplosionRadiusCm);
		                  AppendCanonicalField(Canonical, Row.MovementCm);
		                  AppendCanonicalField(Canonical, Row.bInvulnerable);
		                  AppendCanonicalField(Canonical, Row.BehaviorId);
		                  AppendCanonicalField(Canonical, Row.FormulaId);
		                  AppendCanonicalField(Canonical, Row.ConditionId);
		                  AppendCanonicalField(Canonical, Row.bEnabled);
		                  AppendCanonicalField(Canonical, Row.SourceSheet);
		                  AppendCanonicalField(Canonical, Row.SourceRow);
		                  AppendCanonicalField(Canonical, Row.DisabledReason);
		                  AppendCanonicalField(Canonical, Row.OuterRingStartFraction);
		                  AppendCanonicalField(Canonical, Row.OuterRingBonusMultiplier);
	                  });

	TArray<FName> SlotTypeIds;
	Snapshot.SlotTypes.GetKeys(SlotTypeIds);
	SlotTypeIds.Sort(
	    [](const FName& Left, const FName& Right)
	    {
		    return Left.ToString() < Right.ToString();
	    });
	AppendOrderedRows(Canonical,
	                  TEXT("slot_types"),
	                  SlotTypeIds,
	                  Snapshot.SlotTypes,
	                  [&](const FReEchoCsvSlotTypeRow& Row)
	                  {
		                  AppendCanonicalField(Canonical, Row.Id);
		                  AppendCanonicalField(Canonical, Row.DisplayName);
		                  AppendCanonicalField(Canonical, Row.bEnabled);
		                  AppendCanonicalField(Canonical, Row.SourceSheet);
		                  AppendCanonicalField(Canonical, Row.SourceRow);
		                  AppendCanonicalField(Canonical, Row.DisabledReason);
	                  });

	TArray<FName> SlotProfileIds;
	Snapshot.SlotProfiles.GetKeys(SlotProfileIds);
	SlotProfileIds.Sort(
	    [](const FName& Left, const FName& Right)
	    {
		    return Left.ToString() < Right.ToString();
	    });
	AppendOrderedRows(Canonical,
	                  TEXT("slot_profiles"),
	                  SlotProfileIds,
	                  Snapshot.SlotProfiles,
	                  [&](const FReEchoCsvSlotProfileRow& Row)
	                  {
		                  AppendCanonicalField(Canonical, Row.Id);
		                  AppendCanonicalField(Canonical, Row.WeaponTypeId);
		                  AppendCanonicalField(Canonical, Row.SlotTypeId);
		                  AppendCanonicalField(Canonical, Row.SlotCount);
		                  AppendCanonicalField(Canonical, Row.bRequired);
		                  AppendCanonicalField(Canonical, Row.bEnabled);
		                  AppendCanonicalField(Canonical, Row.SourceSheet);
		                  AppendCanonicalField(Canonical, Row.SourceRow);
		                  AppendCanonicalField(Canonical, Row.DisabledReason);
	                  });

	TArray<FName> ShopPriceRangeCategories;
	Snapshot.ShopPriceRanges.GetKeys(ShopPriceRangeCategories);
	ShopPriceRangeCategories.Sort(
	    [](const FName& Left, const FName& Right)
	    {
		    return Left.ToString() < Right.ToString();
	    });
	AppendOrderedRows(Canonical,
	                  TEXT("shop_price_ranges"),
	                  ShopPriceRangeCategories,
	                  Snapshot.ShopPriceRanges,
	                  [&](const FReEchoCsvShopPriceRangeRow& Row)
	                  {
		                  AppendCanonicalField(Canonical, Row.PriceCategory);
		                  AppendCanonicalField(Canonical, Row.MinPrice);
		                  AppendCanonicalField(Canonical, Row.MaxPrice);
		                  AppendCanonicalField(Canonical, Row.SourceSheet);
		                  AppendCanonicalField(Canonical, Row.SourceRow);
		                  AppendCanonicalField(Canonical, Row.DisabledReason);
	                  });

	TArray<int32> ShopDropEncounterIndices;
	Snapshot.ShopDropLevels.GetKeys(ShopDropEncounterIndices);
	ShopDropEncounterIndices.Sort(
	    [](const int32& Left, const int32& Right)
	    {
		    return Left < Right;
	    });
	AppendOrderedRows(Canonical,
	                  TEXT("shop_drop_levels"),
	                  ShopDropEncounterIndices,
	                  Snapshot.ShopDropLevels,
	                  [&](const FReEchoCsvShopDropLevelRow& Row)
	                  {
		                  AppendCanonicalField(Canonical, Row.EncounterIndex);
		                  AppendCanonicalField(Canonical, Row.FreeTier);
		                  AppendCanonicalField(Canonical, Row.ShopTiers);
		                  AppendCanonicalField(Canonical, Row.SourceSheet);
		                  AppendCanonicalField(Canonical, Row.SourceRow);
		                  AppendCanonicalField(Canonical, Row.DisabledReason);
	                  });

	TArray<FName> ShopRefreshRuleIds;
	Snapshot.ShopRefreshRules.GetKeys(ShopRefreshRuleIds);
	ShopRefreshRuleIds.Sort(
	    [](const FName& Left, const FName& Right)
	    {
		    return Left.ToString() < Right.ToString();
	    });
	AppendOrderedRows(Canonical,
	                  TEXT("shop_refresh_rules"),
	                  ShopRefreshRuleIds,
	                  Snapshot.ShopRefreshRules,
	                  [&](const FReEchoCsvShopRefreshRuleRow& Row)
	                  {
		                  AppendCanonicalField(Canonical, Row.RuleId);
		                  AppendCanonicalField(Canonical, Row.CardSlotRefreshLimit);
		                  AppendCanonicalField(Canonical, Row.WeaponRuneRefreshLimit);
		                  AppendCanonicalField(Canonical, Row.CardSlotRefreshCost);
		                  AppendCanonicalField(Canonical, Row.WeaponRuneRefreshCost);
		                  AppendCanonicalField(Canonical, Row.SourceSheet);
		                  AppendCanonicalField(Canonical, Row.SourceRow);
	                  });

	TArray<FName> PartIds;
	Snapshot.Parts.GetKeys(PartIds);
	PartIds.Sort(
	    [](const FName& Left, const FName& Right)
	    {
		    return Left.ToString() < Right.ToString();
	    });
	AppendOrderedRows(Canonical,
	                  TEXT("parts"),
	                  PartIds,
	                  Snapshot.Parts,
	                  [&](const FReEchoCsvPartRow& Row)
	                  {
		                  AppendCanonicalField(Canonical, Row.Id);
		                  AppendCanonicalField(Canonical, Row.PartId);
		                  AppendCanonicalField(Canonical, Row.WeaponTypeId);
		                  AppendCanonicalField(Canonical, Row.SlotTypeId);
		                  AppendCanonicalField(Canonical, Row.DisplayName);
		                  AppendCanonicalField(Canonical, Row.Description);
		                  AppendCanonicalField(Canonical, Row.Rarity);
		                  TArray<FName> Tags = Row.Tags;
		                  Tags.Sort(
		                      [](const FName& Left, const FName& Right)
		                      {
			                      return Left.ToString() < Right.ToString();
		                      });
		                  FString TagsText;
		                  for (const FName Tag : Tags)
		                  {
			                  if (!TagsText.IsEmpty())
			                  {
				                  TagsText += TEXT(",");
			                  }
			                  TagsText += Tag.ToString();
		                  }
		                  AppendCanonicalField(Canonical, TagsText);
		                  AppendCanonicalField(Canonical, Row.bEnabled);
		                  AppendCanonicalField(Canonical, Row.ReviewStatus);
		                  AppendCanonicalField(Canonical, Row.ImplementationStatus);
		                  AppendCanonicalField(Canonical, Row.SourceSheet);
		                  AppendCanonicalField(Canonical, Row.SourceRow);
		                  AppendCanonicalField(Canonical, Row.DisabledReason);
	                  });

	Canonical += TEXT("[part_effects]\n");
	for (const FName PartId : PartIds)
	{
		const FReEchoCsvPartRow* Part = Snapshot.Parts.Find(PartId);
		if (!Part)
		{
			continue;
		}
		for (const FReEchoCsvPartEffectRow& Row : Part->Effects)
		{
			AppendCanonicalField(Canonical, Row.Id);
			AppendCanonicalField(Canonical, Row.PartId);
			AppendCanonicalField(Canonical, Row.Order);
			AppendCanonicalField(Canonical, Row.Trigger);
			AppendCanonicalField(Canonical, Row.EffectKind);
			AppendCanonicalField(Canonical, Row.Target);
			AppendCanonicalField(Canonical, static_cast<int32>(Row.ValueOp));
			AppendCanonicalField(Canonical, Row.Value);
			AppendCanonicalField(Canonical, Row.BehaviorId);
			AppendCanonicalField(Canonical, Row.FormulaId);
			AppendCanonicalField(Canonical, Row.AttackPatternId);
			AppendCanonicalField(Canonical, Row.ParamName);
			AppendCanonicalField(Canonical, Row.ParamValue);
			AppendCanonicalField(Canonical, Row.DurationSeconds);
			AppendCanonicalField(Canonical, Row.CooldownSeconds);
			AppendCanonicalField(Canonical, Row.StackPolicy);
			AppendCanonicalField(Canonical, Row.bEnabled);
			AppendCanonicalField(Canonical, Row.SourceSheet);
			AppendCanonicalField(Canonical, Row.SourceRow);
			AppendCanonicalField(Canonical, Row.DisabledReason);
			AppendRowEnd(Canonical);
		}
	}

	FMD5 Md5;
	FTCHARToUTF8 Utf8(*Canonical);
	Md5.Update(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
	uint8 Digest[16];
	Md5.Final(Digest);
	return BytesToHex(Digest, UE_ARRAY_COUNT(Digest)).ToLower();
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
	                            TEXT("Description"),
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
		ReEchoCsv::RequireCell(Table, Row, TEXT("Description"), WeaponType.Description, Issues);
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
	                            TEXT("DamageCoefficient"),
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
		ReEchoCsv::RequireFloat(Table, Row, TEXT("DamageCoefficient"), 0.0f, 100.0f, Weapon.DamageCoefficient, Issues);
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
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("WeaponTypeId"), TEXT("Unknown WeaponTypes.Id reference"));
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
			if (Weapon.LoadoutOrder < 1 || Weapon.LoadoutOrder > MaxStartSelectableLoadoutOrder)
			{
				ReEchoCsv::AddIssue(Issues,
				                    Table.File,
				                    Row.Line,
				                    TEXT("LoadoutOrder"),
				                    TEXT("StartSelectable LoadoutOrder must be 1..6"));
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
	                            TEXT("DamageCoefficient"),
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
	                            TEXT("DisabledReason"),
	                            TEXT("OuterRingStartFraction"),
	                            TEXT("OuterRingBonusMultiplier")},
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
		ReEchoCsv::RequireFloat(Table, Row, TEXT("DamageCoefficient"), 0.0f, 100.0f, Step.DamageCoefficient, Issues);
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
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("OuterRingStartFraction"), 0.0f, 1.0f, Step.OuterRingStartFraction, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("OuterRingBonusMultiplier"), 0.0f, 100.0f, Step.OuterRingBonusMultiplier, Issues);

		if (SeenIds.Contains(Step.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		const FString PatternStepKey = FString::Printf(TEXT("%s:%d"), *Step.AttackPatternId.ToString(), Step.StepIndex);
		if (SeenPatternSteps.Contains(PatternStepKey))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("StepIndex"), TEXT("Duplicate AttackPatternId/StepIndex"));
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
	ReEchoCsv::HasExactColumns(Table,
	                           {TEXT("Id"),
	                            TEXT("DisplayName"),
	                            TEXT("Enabled"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow"),
	                            TEXT("DisabledReason")},
	                           Issues);

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
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("SlotTypeId"), TEXT("Duplicate WeaponTypeId/SlotTypeId"));
		}
		if (!Snapshot.WeaponTypes.Contains(SlotProfile.WeaponTypeId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("WeaponTypeId"), TEXT("Unknown WeaponTypes.Id reference"));
		}
		if (!Snapshot.SlotTypes.Contains(SlotProfile.SlotTypeId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("SlotTypeId"), TEXT("Unknown SlotTypes.Id reference"));
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

bool ReadShopPriceRangesTable(const FString& DataDirectory,
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
	TArray<FString> ExpectedColumns = {TEXT("PriceCategory"), TEXT("MinPrice"), TEXT("MaxPrice")};
	if (!ReEchoCsv::HasExactColumns(Table, ExpectedColumns, Issues))
	{
		return false;
	}
	TArray<FName> SeenCategories;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvShopPriceRangeRow Range;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("PriceCategory"), Range.PriceCategory, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("MinPrice"), Range.MinPrice, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("MaxPrice"), Range.MaxPrice, Issues);
		Range.SourceSheet = Entry.FileName;
		Range.SourceRow = Row.Line;
		if (Range.MinPrice > Range.MaxPrice)
		{
			ReEchoCsv::AddIssue(Issues,
			                    Table.File,
			                    Row.Line,
			                    TEXT("MinPrice"),
			                    FString::Printf(TEXT("shop_price_ranges %s min>max"), *Range.PriceCategory.ToString()));
		}
		if (SeenCategories.Contains(Range.PriceCategory))
		{
			ReEchoCsv::AddIssue(
			    Issues,
			    Table.File,
			    Row.Line,
			    TEXT("PriceCategory"),
			    FString::Printf(TEXT("shop_price_ranges duplicate PriceCategory %s"), *Range.PriceCategory.ToString()));
		}
		SeenCategories.Add(Range.PriceCategory);
		Snapshot.ShopPriceRanges.Add(Range.PriceCategory, Range);
	}
	return Issues.Num() == 0;
}

bool ReadShopDropLevelsTable(const FString& DataDirectory,
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
	TArray<FString> ExpectedColumns = {TEXT("EncounterIndex"), TEXT("FreeTier"), TEXT("ShopTiers")};
	if (!ReEchoCsv::HasExactColumns(Table, ExpectedColumns, Issues))
	{
		return false;
	}
	TArray<int32> SeenIndices;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvShopDropLevelRow Level;
		ReEchoCsv::RequireInt(Table, Row, TEXT("EncounterIndex"), Level.EncounterIndex, Issues);
		FString FreeTierStr;
		ReEchoCsv::ReadOptionalCell(Row, TEXT("FreeTier"), FreeTierStr);
		if (FreeTierStr.IsEmpty())
		{
			Level.FreeTier = -1;
		}
		else
		{
			ReEchoCsv::RequireInt(Table, Row, TEXT("FreeTier"), Level.FreeTier, Issues);
		}
		ReEchoCsv::ReadOptionalCell(Row, TEXT("ShopTiers"), Level.ShopTiers);
		Level.SourceSheet = Entry.FileName;
		Level.SourceRow = Row.Line;
		if (SeenIndices.Contains(Level.EncounterIndex))
		{
			ReEchoCsv::AddIssue(
			    Issues,
			    Table.File,
			    Row.Line,
			    TEXT("EncounterIndex"),
			    FString::Printf(TEXT("shop_drop_levels duplicate EncounterIndex %d"), Level.EncounterIndex));
		}
		SeenIndices.Add(Level.EncounterIndex);
		Snapshot.ShopDropLevels.Add(Level.EncounterIndex, Level);
	}
	return Issues.Num() == 0;
}

bool ReadRuneUpgradesTable(const FString& DataDirectory,
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
	TArray<FString> ExpectedColumns = {TEXT("FromPartId"), TEXT("NeedCount"), TEXT("ToPartId")};
	if (!ReEchoCsv::HasExactColumns(Table, ExpectedColumns, Issues))
	{
		return false;
	}
	TSet<FName> SeenFrom;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvRuneUpgradeRow Rule;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("FromPartId"), Rule.FromPartId, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("NeedCount"), Rule.NeedCount, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("ToPartId"), Rule.ToPartId, Issues);
		Rule.SourceSheet = Entry.FileName;
		Rule.SourceRow = Row.Line;
		if (SeenFrom.Contains(Rule.FromPartId))
		{
			ReEchoCsv::AddIssue(
			    Issues,
			    Table.File,
			    Row.Line,
			    TEXT("FromPartId"),
			    FString::Printf(TEXT("rune_upgrades duplicate FromPartId %s"), *Rule.FromPartId.ToString()));
		}
		SeenFrom.Add(Rule.FromPartId);
		Snapshot.RuneUpgrades.Add(Rule.FromPartId, Rule);
	}
	return Issues.Num() == 0;
}

bool ReadShopRefreshRulesTable(const FString& DataDirectory,
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
	const TArray<FString> ExpectedColumns = {TEXT("RuleId"),
	                                         TEXT("CardSlotRefreshLimit"),
	                                         TEXT("WeaponRuneRefreshLimit"),
	                                         TEXT("CardSlotRefreshCost"),
	                                         TEXT("WeaponRuneRefreshCost")};
	if (!ReEchoCsv::HasExactColumns(Table, ExpectedColumns, Issues))
	{
		return false;
	}
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvShopRefreshRuleRow Rule;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("RuleId"), Rule.RuleId, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("CardSlotRefreshLimit"), Rule.CardSlotRefreshLimit, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("WeaponRuneRefreshLimit"), Rule.WeaponRuneRefreshLimit, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("CardSlotRefreshCost"), Rule.CardSlotRefreshCost, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("WeaponRuneRefreshCost"), Rule.WeaponRuneRefreshCost, Issues);
		Rule.SourceSheet = Entry.FileName;
		Rule.SourceRow = Row.Line;
		if (Snapshot.ShopRefreshRules.Contains(Rule.RuleId))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("RuleId"), TEXT("Duplicate shop refresh rule"));
		}
		if (Rule.CardSlotRefreshLimit < 0 || Rule.WeaponRuneRefreshLimit < 0 || Rule.CardSlotRefreshCost < 0 ||
		    Rule.WeaponRuneRefreshCost < 0)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("RuleId"), TEXT("Refresh limits and costs must be non-negative"));
		}
		Snapshot.ShopRefreshRules.Add(Rule.RuleId, Rule);
	}
	if (Table.Rows.Num() != 1 || !Snapshot.ShopRefreshRules.Contains(TEXT("Default")))
	{
		ReEchoCsv::AddIssue(
		    Issues, Table.File, 1, TEXT("RuleId"), TEXT("shop_refresh_rules must contain exactly one Default row"));
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
	                            TEXT("SourceRow"),
	                            TEXT("ShopEnabled"),
	                            TEXT("ShopPrice")},
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
		ReEchoCsv::RequireBool(Table, Row, TEXT("ShopEnabled"), Part.bShopEnabled, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("ShopPrice"), Part.ShopPrice, Issues);
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
		else
		{
			// Unnamed audit row that is not an inert placeholder (enabled, or carrying a
			// PartId without a name): a half-migrated row that must not reach the runtime.
			// Mirrors the validator guard "unnamed audit row must stay disabled with PartId None".
			ReEchoCsv::AddIssue(Issues,
			                    Table.File,
			                    Row.Line,
			                    TEXT("DisplayName"),
			                    TEXT("Unnamed audit row must stay disabled with PartId None"));
		}
		if (!Snapshot.WeaponTypes.Contains(Part.WeaponTypeId) && Part.WeaponTypeId != TEXT("Any"))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("WeaponTypeId"), TEXT("Unknown WeaponTypes.Id reference"));
		}
		if (!Snapshot.SlotTypes.Contains(Part.SlotTypeId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("SlotTypeId"), TEXT("Unknown SlotTypes.Id reference"));
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
				ReEchoCsv::AddIssue(Issues,
				                    Table.File,
				                    Row.Line,
				                    TEXT("ImplementationStatus"),
				                    TEXT("Enabled part must be approved and implemented"));
			}
			if (Part.bShopEnabled && Part.ShopPrice <= 0)
			{
				ReEchoCsv::AddIssue(Issues,
				                    Table.File,
				                    Row.Line,
				                    TEXT("ShopPrice"),
				                    TEXT("Shop-enabled part requires a positive price"));
			}
		}
		else if (Part.DisabledReason.IsEmpty())
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("DisabledReason"), TEXT("Disabled source row requires a reason"));
		}
		if (!Part.bEnabled && Part.bShopEnabled)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("ShopEnabled"), TEXT("Disabled part cannot be sold in the shop"));
		}
		if (!Part.bShopEnabled && Part.ShopPrice != 0)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("ShopPrice"), TEXT("Non-shop part must use price 0"));
		}

		SeenIds.Add(Part.Id);
		Snapshot.Parts.Add(Part.Id, Part);
	}
	if (Table.Rows.Num() != ExpectedPartSourceRows)
	{
		ReEchoCsv::AddIssue(
		    Issues, Table.File, 1, TEXT("SourceRow"), TEXT("Production four-weapon rune catalog must contain 111 rows"));
	}
	// The named/unnamed split is no longer pinned to 10/60: weapon part families are implemented
	// incrementally, so naming + enabling rows is expected progress. The real invariant kept here is
	// that every source row is either a named part or an inert unnamed placeholder (disabled, PartId None),
	// which still blocks half-migrated rows from reaching the runtime. Mirrors the validator guard.
	if (NamedRows + UnnamedDisabledRows != Table.Rows.Num())
	{
		ReEchoCsv::AddIssue(Issues,
		                    Table.File,
		                    1,
		                    TEXT("DisplayName"),
		                    TEXT("Weapon slot audit: every source row must be a named part or an unnamed "
		                         "disabled placeholder (PartId None)"));
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
	ReEchoCsv::HasExactColumns(
	    Table,
	    {TEXT("Id"),         TEXT("PartId"),          TEXT("Order"),           TEXT("Trigger"),
	     TEXT("EffectKind"), TEXT("Target"),          TEXT("ValueOp"),         TEXT("Value"),
	     TEXT("BehaviorId"), TEXT("FormulaId"),       TEXT("AttackPatternId"), TEXT("ParamName"),
	     TEXT("ParamValue"), TEXT("DurationSeconds"), TEXT("CooldownSeconds"), TEXT("StackPolicy"),
	     TEXT("Enabled"),    TEXT("DisabledReason"),  TEXT("SourceSheet"),     TEXT("SourceRow")},
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
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("BehaviorId"), TEXT("EffectKind/BehaviorId combination is invalid"));
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
	}
	if (EnabledCores < 6)
	{
		ReEchoCsv::AddIssue(Issues, Table.File, 1, TEXT("PartId"), TEXT("At least six generic cores must be enabled"));
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
	if (StartSelectable.Num() < 1)
	{
		ReEchoCsv::AddIssue(Issues,
		                    TEXT("weapons.csv"),
		                    1,
		                    TEXT("StartSelectable"),
		                    TEXT("At least one weapon must be StartSelectable"));
	}
	const TArray<FName> ExpectedLoadout = {TEXT("W_J_04"), TEXT("W_J_01"), TEXT("W_J_08"), TEXT("W_J_09")};
	for (int32 Index = 0; Index < StartSelectable.Num() && Index < ExpectedLoadout.Num(); ++Index)
	{
		if (StartSelectable[Index].Id != ExpectedLoadout[Index])
		{
			ReEchoCsv::AddIssue(
			    Issues, TEXT("weapons.csv"), 1, TEXT("LoadoutOrder"), TEXT("Start weapon loadout order changed"));
		}
	}
	const FReEchoCsvWeaponRow* Slot1 = Snapshot.FindWeaponByInputSlot(EReEchoInputSlot::Slot1);
	const FReEchoCsvWeaponRow* Slot2 = Snapshot.FindWeaponByInputSlot(EReEchoInputSlot::Slot2);
	if (!Slot1 || Slot1->Id != TEXT("W_J_04") || !Slot2 || Slot2->Id != TEXT("W_J_01"))
	{
		ReEchoCsv::AddIssue(
		    Issues, TEXT("weapons.csv"), 1, TEXT("InputSlot"), TEXT("Required W_J_04/W_J_01 input mapping changed"));
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
		ReadShopPriceRangesTable(DataDirectory, ManifestEntries[ShopPriceRangesTableId], Snapshot, Issues);
	}
	if (Issues.Num() == 0)
	{
		ReadShopDropLevelsTable(DataDirectory, ManifestEntries[ShopDropLevelsTableId], Snapshot, Issues);
	}
	if (Issues.Num() == 0)
	{
		ReadRuneUpgradesTable(DataDirectory, ManifestEntries[RuneUpgradesTableId], Snapshot, Issues);
	}
	if (Issues.Num() == 0)
	{
		ReadShopRefreshRulesTable(DataDirectory, ManifestEntries[ShopRefreshRulesTableId], Snapshot, Issues);
	}
	if (Issues.Num() == 0)
	{
		ValidateCrossDomain(Snapshot, Issues);
	}
	if (Issues.Num() == 0)
	{
		Snapshot.WeaponDomainRevision = ComputeWeaponDomainRevision(Snapshot);
	}
	return Issues.Num() == 0;
}
}
