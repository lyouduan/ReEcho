#include "ReEchoEnemyCsvReader.h"

#include "Misc/Paths.h"

namespace ReEchoEnemyCsv
{
namespace
{
constexpr const TCHAR* EnemiesTableId = TEXT("Enemies");
constexpr const TCHAR* EnemyAbilitiesTableId = TEXT("EnemyAbilities");
constexpr const TCHAR* BossPhasesTableId = TEXT("BossPhases");
constexpr const TCHAR* EnemyCombatStatsTableId = TEXT("EnemyCombatStats");
constexpr const TCHAR* EnemyShardDropsTableId = TEXT("EnemyShardDrops");

bool ReadTable(const FString& DataDirectory,
               const ReEchoCsv::FManifestEntry& Entry,
               ReEchoCsv::FTable& OutTable,
               TArray<FReEchoCsvIssue>& Issues)
{
	return ReEchoCsv::ParseCsvFile(FPaths::Combine(DataDirectory, Entry.FileName), OutTable, Issues);
}

bool IsZero(const float Value)
{
	return FMath::IsNearlyZero(Value, KINDA_SMALL_NUMBER);
}

bool ReadEnemies(const FString& DataDirectory,
                 const ReEchoCsv::FManifestEntry& Entry,
                 FReEchoCsvDataSnapshot& Snapshot,
                 TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::FTable Table;
	if (!ReadTable(DataDirectory, Entry, Table, Issues))
	{
		return false;
	}
	ReEchoCsv::HasExactColumns(Table,
	                           {TEXT("Id"),
	                            TEXT("Archetype"),
	                            TEXT("BehaviorProfileId"),
	                            TEXT("PresentationId"),
	                            TEXT("Enabled"),
	                            TEXT("MaxHealth"),
	                            TEXT("MoveSpeedMultiplier"),
	                            TEXT("CollisionRadiusCm"),
	                            TEXT("CollisionHalfHeightCm"),
	                            TEXT("ContactDamage"),
	                            TEXT("AttackIntervalSeconds"),
	                            TEXT("ContactRangeCm"),
	                            TEXT("MovementStopDistanceCm"),
	                            TEXT("HitReactionDurationSeconds"),
	                            TEXT("KnockbackSpeedCmPerSecond"),
	                            TEXT("KnockbackDrag"),
	                            TEXT("TriggerRadiusCm"),
	                            TEXT("DamageRadiusCm"),
	                            TEXT("FuseSeconds"),
	                            TEXT("Boss"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow"),
	                            TEXT("Notes"),
	                            TEXT("Phase2Enabled"),
	                            TEXT("Phase2TriggerRangeCm"),
	                            TEXT("Phase2RequiredAttackCount"),
	                            TEXT("Phase2TransformSeconds"),
	                            TEXT("Phase2TriggerMode"),
	                            TEXT("Phase2HealthThresholdRatio"),
	                            TEXT("HateRangeCm")},
	                           Issues);

	const TMap<FName, FName> ExpectedProfiles = {
	    {TEXT("Grunt"), TEXT("Enemy.Grunt")},
	    {TEXT("Shield"), TEXT("Enemy.Shield")},
	    {TEXT("Bomber"), TEXT("Enemy.Bomber")},
	    {TEXT("Boss"), TEXT("Boss.TimeGuard")},
	    {TEXT("Slime"), TEXT("Enemy.Slime")},
	    {TEXT("Ranged"), TEXT("Enemy.Ranged")},
	    {TEXT("Elite"), TEXT("Enemy.Elite")},
	};
	TSet<FName> SeenIds;
	for (const ReEchoCsv::FRow& CsvRow : Table.Rows)
	{
		FReEchoCsvEnemyRow Row;
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("Id"), Row.Id, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("Archetype"), Row.Archetype, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("BehaviorProfileId"), Row.BehaviorProfileId, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("PresentationId"), Row.PresentationId, Issues);
		ReEchoCsv::RequireBool(Table, CsvRow, TEXT("Enabled"), Row.bEnabled, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("MaxHealth"), 1.0f, 1000000.0f, Row.MaxHealth, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("MoveSpeedMultiplier"), 0.01f, 10.0f, Row.MoveSpeedMultiplier, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("CollisionRadiusCm"), 0.1f, 100000.0f, Row.CollisionRadiusCm, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("CollisionHalfHeightCm"), 0.1f, 100000.0f, Row.CollisionHalfHeightCm, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("ContactDamage"), 0.0f, 100000.0f, Row.ContactDamage, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("AttackIntervalSeconds"), 0.0f, 3600.0f, Row.AttackIntervalSeconds, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("ContactRangeCm"), 0.0f, 100000.0f, Row.ContactRangeCm, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("MovementStopDistanceCm"), 0.0f, 100000.0f, Row.MovementStopDistanceCm, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("HitReactionDurationSeconds"), 0.0f, 3600.0f, Row.HitReactionDurationSeconds, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("KnockbackSpeedCmPerSecond"), 0.0f, 100000.0f, Row.KnockbackSpeedCmPerSecond, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("KnockbackDrag"), 0.0f, 100000.0f, Row.KnockbackDrag, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("TriggerRadiusCm"), 0.0f, 100000.0f, Row.TriggerRadiusCm, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("DamageRadiusCm"), 0.0f, 100000.0f, Row.DamageRadiusCm, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("FuseSeconds"), 0.0f, 3600.0f, Row.FuseSeconds, Issues);
		ReEchoCsv::RequireBool(Table, CsvRow, TEXT("Boss"), Row.bBoss, Issues);
		ReEchoCsv::RequireCell(Table, CsvRow, TEXT("SourceSheet"), Row.SourceSheet, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("SourceRow"), Row.SourceRow, Issues);
		ReEchoCsv::ReadOptionalCell(CsvRow, TEXT("Notes"), Row.Notes);
		ReEchoCsv::RequireBool(Table, CsvRow, TEXT("Phase2Enabled"), Row.bPhase2Enabled, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("Phase2TriggerRangeCm"), 0.0f, 100000.0f, Row.Phase2TriggerRangeCm, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("Phase2RequiredAttackCount"), Row.Phase2RequiredAttackCount, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("Phase2TransformSeconds"), 0.0f, 3600.0f, Row.Phase2TransformSeconds, Issues);
		ReEchoCsv::ReadOptionalCell(CsvRow, TEXT("Phase2TriggerMode"), Row.Phase2TriggerMode);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("Phase2HealthThresholdRatio"), 0.0f, 1.0f, Row.Phase2HealthThresholdRatio, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("HateRangeCm"), 0.0f, 100000.0f, Row.HateRangeCm, Issues);

		if (Row.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(Row.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, CsvRow.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		const FName* ExpectedProfile = ExpectedProfiles.Find(Row.Archetype);
		if (!ExpectedProfile)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("Archetype"), TEXT("Unsupported enemy archetype"));
		}
		else if (Row.BehaviorProfileId != *ExpectedProfile)
		{
			ReEchoCsv::AddIssue(Issues,
			                    Table.File,
			                    CsvRow.Line,
			                    TEXT("BehaviorProfileId"),
			                    TEXT("Behavior profile does not match Archetype"));
		}
		if (!FReEchoCsvDataRegistry::IsBehaviorIdRegistered(Row.BehaviorProfileId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("BehaviorProfileId"), TEXT("Unknown registered C++ behavior id"));
		}
		const bool bBossArchetype = Row.Archetype == TEXT("Boss");
		if (Row.bBoss != bBossArchetype)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("Boss"), TEXT("Boss flag must match Boss archetype"));
		}
		const bool bBomber = Row.Archetype == TEXT("Bomber");
		if (bBomber && (Row.TriggerRadiusCm <= 0.0f || Row.DamageRadiusCm <= 0.0f || Row.FuseSeconds <= 0.0f))
		{
			ReEchoCsv::AddIssue(Issues,
			                    Table.File,
			                    CsvRow.Line,
			                    TEXT("TriggerRadiusCm"),
			                    TEXT("Bomber radii and fuse must be positive"));
		}
		if (!bBomber && (!IsZero(Row.TriggerRadiusCm) || !IsZero(Row.DamageRadiusCm) || !IsZero(Row.FuseSeconds)))
		{
			ReEchoCsv::AddIssue(Issues,
			                    Table.File,
			                    CsvRow.Line,
			                    TEXT("TriggerRadiusCm"),
			                    TEXT("Non-Bomber fields must use explicit zero"));
		}
		SeenIds.Add(Row.Id);
		Snapshot.EnemyOrder.Add(Row.Id);
		Snapshot.Enemies.Add(Row.Id, MoveTemp(Row));
	}
	return Issues.Num() == 0;
}

bool ReadOptionalRewardBound(const ReEchoCsv::FTable& Table,
                             const ReEchoCsv::FRow& CsvRow,
                             const TCHAR* Field,
                             int32& OutValue,
                             TArray<FReEchoCsvIssue>& Issues)
{
	FString Text;
	if (!ReEchoCsv::ReadOptionalCell(CsvRow, Field, Text) || Text.IsEmpty())
	{
		OutValue = INDEX_NONE;
		return true;
	}
	return ReEchoCsv::RequireInt(Table, CsvRow, Field, OutValue, Issues);
}

bool ReadEnemyShardDrops(const FString& DataDirectory,
                         const ReEchoCsv::FManifestEntry& Entry,
                         FReEchoCsvDataSnapshot& Snapshot,
                         TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::FTable Table;
	if (!ReadTable(DataDirectory, Entry, Table, Issues))
	{
		return false;
	}
	ReEchoCsv::HasExactColumns(Table,
	                           {TEXT("EncounterIndex"),
	                            TEXT("MeleeMin"),
	                            TEXT("MeleeMax"),
	                            TEXT("RangedMin"),
	                            TEXT("RangedMax"),
	                            TEXT("EliteMin"),
	                            TEXT("EliteMax"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow"),
	                            TEXT("Notes")},
	                           Issues);

	for (const ReEchoCsv::FRow& CsvRow : Table.Rows)
	{
		FReEchoCsvEnemyShardDropRow Row;
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("EncounterIndex"), Row.EncounterIndex, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("MeleeMin"), Row.MeleeMin, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("MeleeMax"), Row.MeleeMax, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("RangedMin"), Row.RangedMin, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("RangedMax"), Row.RangedMax, Issues);
		ReadOptionalRewardBound(Table, CsvRow, TEXT("EliteMin"), Row.EliteMin, Issues);
		ReadOptionalRewardBound(Table, CsvRow, TEXT("EliteMax"), Row.EliteMax, Issues);
		ReEchoCsv::RequireCell(Table, CsvRow, TEXT("SourceSheet"), Row.SourceSheet, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("SourceRow"), Row.SourceRow, Issues);
		ReEchoCsv::ReadOptionalCell(CsvRow, TEXT("Notes"), Row.Notes);

		if (Row.EncounterIndex < 1 || Row.EncounterIndex > 8)
		{
			ReEchoCsv::AddIssue(Issues, Table.File, CsvRow.Line, TEXT("EncounterIndex"), TEXT("Expected 1..8"));
		}
		if (Snapshot.EnemyShardDrops.Contains(Row.EncounterIndex))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, CsvRow.Line, TEXT("EncounterIndex"), TEXT("Duplicate index"));
		}
		const bool bEliteMinMissing = Row.EliteMin == INDEX_NONE;
		const bool bEliteMaxMissing = Row.EliteMax == INDEX_NONE;
		if (Row.MeleeMin < 0 || Row.MeleeMax < Row.MeleeMin || Row.RangedMin < 0 || Row.RangedMax < Row.RangedMin)
		{
			ReEchoCsv::AddIssue(Issues,
			                    Table.File,
			                    CsvRow.Line,
			                    TEXT("MeleeMin"),
			                    TEXT("Reward ranges must be non-negative and ordered"));
		}
		if (bEliteMinMissing != bEliteMaxMissing ||
		    (!bEliteMinMissing && (Row.EliteMin < 0 || Row.EliteMax < Row.EliteMin)))
		{
			ReEchoCsv::AddIssue(Issues,
			                    Table.File,
			                    CsvRow.Line,
			                    TEXT("EliteMin"),
			                    TEXT("Elite reward must be a complete ordered pair or both cells blank"));
		}
		Snapshot.EnemyShardDrops.Add(Row.EncounterIndex, MoveTemp(Row));
	}
	for (int32 EncounterIndex = 1; EncounterIndex <= 8; ++EncounterIndex)
	{
		if (!Snapshot.EnemyShardDrops.Contains(EncounterIndex))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, 1, TEXT("EncounterIndex"), TEXT("Missing required encounter row"));
		}
	}
	return Issues.Num() == 0;
}

bool ReadAbilities(const FString& DataDirectory,
                   const ReEchoCsv::FManifestEntry& Entry,
                   FReEchoCsvDataSnapshot& Snapshot,
                   TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::FTable Table;
	if (!ReadTable(DataDirectory, Entry, Table, Issues))
	{
		return false;
	}
	ReEchoCsv::HasExactColumns(Table,
	                           {TEXT("Id"),
	                            TEXT("OwnerEnemyId"),
	                            TEXT("BehaviorId"),
	                            TEXT("SequenceOrder"),
	                            TEXT("Enabled"),
	                            TEXT("Damage"),
	                            TEXT("WindupSeconds"),
	                            TEXT("ActiveSeconds"),
	                            TEXT("RecoverySeconds"),
	                            TEXT("CooldownSeconds"),
	                            TEXT("MinRangeCm"),
	                            TEXT("MaxRangeCm"),
	                            TEXT("RadiusCm"),
	                            TEXT("WidthCm"),
	                            TEXT("LengthCm"),
	                            TEXT("ProjectileSpeedCmPerSecond"),
	                            TEXT("TeleportOffsetCm"),
	                            TEXT("TargetingMode"),
	                            TEXT("LockTiming"),
	                            TEXT("CleanseIntervalSeconds"),
	                            TEXT("ImmunitySeconds"),
	                            TEXT("ProjectileCount"),
	                            TEXT("SpreadAngleDegrees"),
	                            TEXT("bMovementDuringCast"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow"),
	                            TEXT("Notes")},
	                           Issues);

	const TSet<FName> AllowedBehaviors = {TEXT("Boss.MeleeSweep"),
	                                      TEXT("Boss.Projectile"),
	                                      TEXT("Boss.BlinkSlam"),
	                                      TEXT("Boss.PrayerBeam"),
	                                      TEXT("Boss.ElementCleanse"),
	                                      TEXT("Enemy.RangedBurst"),
	                                      TEXT("Enemy.EliteDash")};
	TSet<FName> SeenIds;
	TSet<FString> SeenOrders;
	TMap<FName, int32> EnabledActiveCounts;
	for (const TPair<FName, FReEchoCsvEnemyRow>& Pair : Snapshot.Enemies)
	{
		if (Pair.Value.bEnabled && Pair.Value.bBoss)
		{
			EnabledActiveCounts.Add(Pair.Key, 0);
		}
	}
	for (const ReEchoCsv::FRow& CsvRow : Table.Rows)
	{
		FReEchoCsvEnemyAbilityRow Row;
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("Id"), Row.Id, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("OwnerEnemyId"), Row.OwnerEnemyId, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("BehaviorId"), Row.BehaviorId, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("SequenceOrder"), Row.SequenceOrder, Issues);
		ReEchoCsv::RequireBool(Table, CsvRow, TEXT("Enabled"), Row.bEnabled, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("Damage"), 0.0f, 100000.0f, Row.Damage, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("WindupSeconds"), 0.0f, 3600.0f, Row.WindupSeconds, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("ActiveSeconds"), 0.0f, 3600.0f, Row.ActiveSeconds, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("RecoverySeconds"), 0.0f, 3600.0f, Row.RecoverySeconds, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("CooldownSeconds"), 0.0f, 3600.0f, Row.CooldownSeconds, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("MinRangeCm"), 0.0f, 100000.0f, Row.MinRangeCm, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("MaxRangeCm"), 0.0f, 100000.0f, Row.MaxRangeCm, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("RadiusCm"), 0.0f, 100000.0f, Row.RadiusCm, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("WidthCm"), 0.0f, 100000.0f, Row.WidthCm, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("LengthCm"), 0.0f, 100000.0f, Row.LengthCm, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("ProjectileSpeedCmPerSecond"), 0.0f, 100000.0f, Row.ProjectileSpeedCmPerSecond, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("TeleportOffsetCm"), 0.0f, 100000.0f, Row.TeleportOffsetCm, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("TargetingMode"), Row.TargetingMode, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("LockTiming"), Row.LockTiming, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("CleanseIntervalSeconds"), 0.0f, 3600.0f, Row.CleanseIntervalSeconds, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("ImmunitySeconds"), 0.0f, 3600.0f, Row.ImmunitySeconds, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("ProjectileCount"), Row.ProjectileCount, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("SpreadAngleDegrees"), 0.0f, 360.0f, Row.SpreadAngleDegrees, Issues);
		ReEchoCsv::RequireBool(Table, CsvRow, TEXT("bMovementDuringCast"), Row.bMovementDuringCast, Issues);
		ReEchoCsv::RequireCell(Table, CsvRow, TEXT("SourceSheet"), Row.SourceSheet, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("SourceRow"), Row.SourceRow, Issues);
		ReEchoCsv::ReadOptionalCell(CsvRow, TEXT("Notes"), Row.Notes);

		if (Row.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(Row.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, CsvRow.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		FReEchoCsvEnemyRow* Owner = Snapshot.Enemies.Find(Row.OwnerEnemyId);
		if (!Owner)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("OwnerEnemyId"), TEXT("Unknown Enemies.Id reference"));
			continue;
		}
		if (!Owner->bEnabled)
		{
			ReEchoCsv::AddIssue(Issues,
			                    Table.File,
			                    CsvRow.Line,
			                    TEXT("OwnerEnemyId"),
			                    TEXT("Abilities require an enabled enemy owner"));
		}
		if (!AllowedBehaviors.Contains(Row.BehaviorId) ||
		    !FReEchoCsvDataRegistry::IsBehaviorIdRegistered(Row.BehaviorId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("BehaviorId"), TEXT("Unknown registered C++ enemy behavior id"));
		}
		if (Row.BehaviorId.ToString().StartsWith(TEXT("Boss.")) && !Owner->bBoss)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("OwnerEnemyId"), TEXT("Boss behavior requires a Boss owner"));
		}
		if (Row.BehaviorId == TEXT("Enemy.RangedBurst") && Owner->Archetype != TEXT("Ranged"))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("OwnerEnemyId"), TEXT("RangedBurst requires a Ranged owner"));
		}
		if (Row.BehaviorId == TEXT("Enemy.EliteDash") && Owner->Archetype != TEXT("Elite"))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("OwnerEnemyId"), TEXT("EliteDash requires an Elite owner"));
		}
		if (Row.MinRangeCm > Row.MaxRangeCm)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("MinRangeCm"), TEXT("Minimum range cannot exceed maximum range"));
		}
		const bool bCleanse = Row.BehaviorId == TEXT("Boss.ElementCleanse");
		if (bCleanse)
		{
			if (Row.SequenceOrder != 0)
			{
				ReEchoCsv::AddIssue(
				    Issues, Table.File, CsvRow.Line, TEXT("SequenceOrder"), TEXT("Passive cleanse must use zero"));
			}
			if (!IsZero(Row.Damage) || !IsZero(Row.WindupSeconds) || !IsZero(Row.ActiveSeconds) ||
			    !IsZero(Row.RecoverySeconds) || !IsZero(Row.CooldownSeconds) || !IsZero(Row.MinRangeCm) ||
			    !IsZero(Row.MaxRangeCm) || !IsZero(Row.RadiusCm) || !IsZero(Row.WidthCm) || !IsZero(Row.LengthCm) ||
			    !IsZero(Row.ProjectileSpeedCmPerSecond) || !IsZero(Row.TeleportOffsetCm))
			{
				ReEchoCsv::AddIssue(Issues,
				                    Table.File,
				                    CsvRow.Line,
				                    TEXT("Damage"),
				                    TEXT("Cleanse damage and spatial fields must use explicit zero"));
			}
			if (Row.CleanseIntervalSeconds <= 0.0f || Row.ImmunitySeconds <= 0.0f)
			{
				ReEchoCsv::AddIssue(Issues,
				                    Table.File,
				                    CsvRow.Line,
				                    TEXT("CleanseIntervalSeconds"),
				                    TEXT("Cleanse interval and immunity must be positive"));
			}
		}
		else
		{
			if (Row.SequenceOrder <= 0)
			{
				ReEchoCsv::AddIssue(Issues,
				                    Table.File,
				                    CsvRow.Line,
				                    TEXT("SequenceOrder"),
				                    TEXT("Active abilities require a positive order"));
			}
			const FString OrderKey = Row.OwnerEnemyId.ToString() + TEXT("/") + FString::FromInt(Row.SequenceOrder);
			if (SeenOrders.Contains(OrderKey))
			{
				ReEchoCsv::AddIssue(
				    Issues, Table.File, CsvRow.Line, TEXT("SequenceOrder"), TEXT("Duplicate owner and sequence order"));
			}
			SeenOrders.Add(OrderKey);
			if (Row.bEnabled)
			{
				if (int32* ActiveCount = EnabledActiveCounts.Find(Row.OwnerEnemyId))
				{
					++(*ActiveCount);
				}
			}
			if (!IsZero(Row.CleanseIntervalSeconds) || !IsZero(Row.ImmunitySeconds))
			{
				ReEchoCsv::AddIssue(Issues,
				                    Table.File,
				                    CsvRow.Line,
				                    TEXT("CleanseIntervalSeconds"),
				                    TEXT("Active abilities must use explicit zero"));
			}
			if (Row.BehaviorId == TEXT("Boss.Projectile") && Row.ProjectileSpeedCmPerSecond <= 0.0f)
			{
				ReEchoCsv::AddIssue(Issues,
				                    Table.File,
				                    CsvRow.Line,
				                    TEXT("ProjectileSpeedCmPerSecond"),
				                    TEXT("Projectile speed must be positive"));
			}
			if (Row.BehaviorId == TEXT("Boss.BlinkSlam") && (Row.WindupSeconds <= 0.0f || Row.RadiusCm <= 0.0f))
			{
				ReEchoCsv::AddIssue(Issues,
				                    Table.File,
				                    CsvRow.Line,
				                    TEXT("RadiusCm"),
				                    TEXT("Blink slam requires a warning and radius"));
			}
		}
		SeenIds.Add(Row.Id);
		Owner->Abilities.Add(MoveTemp(Row));
	}
	for (const TPair<FName, int32>& Pair : EnabledActiveCounts)
	{
		if (Pair.Value == 0)
		{
			ReEchoCsv::AddIssue(
			    Issues,
			    Table.File,
			    1,
			    TEXT("OwnerEnemyId"),
			    FString::Printf(TEXT("Enabled boss %s has no enabled active ability"), *Pair.Key.ToString()));
		}
	}
	for (TPair<FName, FReEchoCsvEnemyRow>& Pair : Snapshot.Enemies)
	{
		Pair.Value.Abilities.Sort(
		    [](const FReEchoCsvEnemyAbilityRow& Left, const FReEchoCsvEnemyAbilityRow& Right)
		    {
			    return Left.SequenceOrder == Right.SequenceOrder ? Left.Id.LexicalLess(Right.Id)
			                                                     : Left.SequenceOrder < Right.SequenceOrder;
		    });
	}
	return Issues.Num() == 0;
}

bool ReadBossPhases(const FString& DataDirectory,
                    const ReEchoCsv::FManifestEntry& Entry,
                    FReEchoCsvDataSnapshot& Snapshot,
                    TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::FTable Table;
	if (!ReadTable(DataDirectory, Entry, Table, Issues))
	{
		return false;
	}
	ReEchoCsv::HasExactColumns(Table,
	                           {TEXT("Id"),
	                            TEXT("BossEnemyId"),
	                            TEXT("PhaseIndex"),
	                            TEXT("TriggerSeconds"),
	                            TEXT("EchoPolicy"),
	                            TEXT("PhysicalAttackMultiplier"),
	                            TEXT("ElementalAttackMultiplier"),
	                            TEXT("AttackSpeedMultiplier"),
	                            TEXT("MovementSpeedMultiplier"),
	                            TEXT("RefillHealthPolicy"),
	                            TEXT("PhaseMaxHealth"),
	                            TEXT("Enabled"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow"),
	                            TEXT("Notes")},
	                           Issues);

	TSet<FName> SeenIds;
	TSet<FString> SeenPhases;
	for (const ReEchoCsv::FRow& CsvRow : Table.Rows)
	{
		FReEchoCsvBossPhaseRow Row;
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("Id"), Row.Id, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("BossEnemyId"), Row.BossEnemyId, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("PhaseIndex"), Row.PhaseIndex, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("TriggerSeconds"), 0.0f, 3600.0f, Row.TriggerSeconds, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("EchoPolicy"), Row.EchoPolicy, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("PhysicalAttackMultiplier"), 0.0f, 100.0f, Row.PhysicalAttackMultiplier, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("ElementalAttackMultiplier"), 0.0f, 100.0f, Row.ElementalAttackMultiplier, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("AttackSpeedMultiplier"), 0.0f, 100.0f, Row.AttackSpeedMultiplier, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("MovementSpeedMultiplier"), 0.0f, 100.0f, Row.MovementSpeedMultiplier, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("RefillHealthPolicy"), Row.RefillHealthPolicy, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("PhaseMaxHealth"), 0.0f, 1000000.0f, Row.PhaseMaxHealth, Issues);
		ReEchoCsv::RequireBool(Table, CsvRow, TEXT("Enabled"), Row.bEnabled, Issues);
		ReEchoCsv::RequireCell(Table, CsvRow, TEXT("SourceSheet"), Row.SourceSheet, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("SourceRow"), Row.SourceRow, Issues);
		ReEchoCsv::ReadOptionalCell(CsvRow, TEXT("Notes"), Row.Notes);

		if (Row.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(Row.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, CsvRow.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		FReEchoCsvEnemyRow* Boss = Snapshot.Enemies.Find(Row.BossEnemyId);
		if (!Boss)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("BossEnemyId"), TEXT("Unknown Enemies.Id reference"));
			continue;
		}
		if (!Boss->bEnabled || !Boss->bBoss)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("BossEnemyId"), TEXT("Phases require an enabled boss owner"));
		}
		const FString PhaseKey = Row.BossEnemyId.ToString() + TEXT("/") + FString::FromInt(Row.PhaseIndex);
		if (SeenPhases.Contains(PhaseKey))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("PhaseIndex"), TEXT("Duplicate boss phase index"));
		}
		// EchoPolicy must be a supported enum string (see EReEchoBossEchoPolicy in ReEchoEnemyTypes.h).
		if (Row.EchoPolicy != TEXT("None") && Row.EchoPolicy != TEXT("DestroyEncounterEchoes"))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("EchoPolicy"), TEXT("Unsupported echo phase policy"));
		}
		// RefillHealthPolicy must be a supported enum string (see EReEchoBossRefillHealthPolicy).
		if (Row.RefillHealthPolicy != TEXT("None") && Row.RefillHealthPolicy != TEXT("RefillToMaximum"))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("RefillHealthPolicy"), TEXT("Unsupported refill health policy"));
		}
		SeenIds.Add(Row.Id);
		SeenPhases.Add(PhaseKey);
		Boss->BossPhases.Add(MoveTemp(Row));
	}
	for (TPair<FName, FReEchoCsvEnemyRow>& Pair : Snapshot.Enemies)
	{
		Pair.Value.BossPhases.Sort(
		    [](const FReEchoCsvBossPhaseRow& Left, const FReEchoCsvBossPhaseRow& Right)
		    {
			    return Left.PhaseIndex == Right.PhaseIndex ? Left.Id.LexicalLess(Right.Id)
			                                               : Left.PhaseIndex < Right.PhaseIndex;
		    });
	}
	return Issues.Num() == 0;
}
}

bool ReadEnemyCombatStats(const FString& DataDirectory,
                          const ReEchoCsv::FManifestEntry& Entry,
                          FReEchoCsvDataSnapshot& Snapshot,
                          TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::FTable Table;
	if (!ReadTable(DataDirectory, Entry, Table, Issues))
	{
		return false;
	}
	ReEchoCsv::HasExactColumns(Table,
	                           {TEXT("Id"),
	                            TEXT("EnemyId"),
	                            TEXT("CombatIndex"),
	                            TEXT("MaxHealth"),
	                            TEXT("ContactDamage"),
	                            TEXT("AttackIntervalSeconds")},
	                           Issues);

	for (const ReEchoCsv::FRow& CsvRow : Table.Rows)
	{
		FReEchoCsvEnemyCombatStatRow Row;
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("Id"), Row.Id, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("EnemyId"), Row.EnemyId, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("CombatIndex"), Row.CombatIndex, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("MaxHealth"), 1.0f, 1000000.0f, Row.MaxHealth, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("ContactDamage"), 0.0f, 100000.0f, Row.ContactDamage, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("AttackIntervalSeconds"), 0.0f, 3600.0f, Row.AttackIntervalSeconds, Issues);

		if (Row.Id.IsNone())
		{
			continue;
		}
		const FName CompositeKey = FName(*FString::Printf(TEXT("%s#%d"), *Row.EnemyId.ToString(), Row.CombatIndex));
		Snapshot.EnemyCombatStats.Add(CompositeKey, MoveTemp(Row));
		Snapshot.EnemyCombatStatOrder.Add(CompositeKey);
	}
	return Issues.Num() == 0;
}

bool ReadTables(const FString& DataDirectory,
                const TMap<FString, ReEchoCsv::FManifestEntry>& ManifestEntries,
                FReEchoCsvDataSnapshot& Snapshot,
                TArray<FReEchoCsvIssue>& Issues)
{
	if (!ReadEnemies(DataDirectory, ManifestEntries[EnemiesTableId], Snapshot, Issues))
	{
		return false;
	}
	if (!ReadAbilities(DataDirectory, ManifestEntries[EnemyAbilitiesTableId], Snapshot, Issues))
	{
		return false;
	}
	if (!ReadBossPhases(DataDirectory, ManifestEntries[BossPhasesTableId], Snapshot, Issues))
	{
		return false;
	}
	if (!ReadEnemyShardDrops(DataDirectory, ManifestEntries[EnemyShardDropsTableId], Snapshot, Issues))
	{
		return false;
	}
	return ReadEnemyCombatStats(DataDirectory, ManifestEntries[EnemyCombatStatsTableId], Snapshot, Issues);
}
}
