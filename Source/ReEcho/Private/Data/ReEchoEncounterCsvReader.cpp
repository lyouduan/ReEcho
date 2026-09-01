#include "ReEchoEncounterCsvReader.h"

#include "Misc/Paths.h"

namespace ReEchoEncounterCsv
{
namespace
{
constexpr const TCHAR* StagesTableId = TEXT("Stages");
constexpr const TCHAR* EncountersTableId = TEXT("Encounters");
constexpr const TCHAR* EncounterWavesTableId = TEXT("EncounterWaves");
constexpr const TCHAR* SpawnProfilesTableId = TEXT("SpawnProfiles");
constexpr const TCHAR* SpawnPolicyTableId = TEXT("SpawnPolicy");

bool ReadTable(const FString& DataDirectory,
               const ReEchoCsv::FManifestEntry& Entry,
               ReEchoCsv::FTable& OutTable,
               TArray<FReEchoCsvIssue>& Issues)
{
	return ReEchoCsv::ParseCsvFile(FPaths::Combine(DataDirectory, Entry.FileName), OutTable, Issues);
}

void ReadSource(const ReEchoCsv::FTable& Table,
                const ReEchoCsv::FRow& CsvRow,
                FString& OutSourceSheet,
                int32& OutSourceRow,
                FString& OutNotes,
                TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::RequireCell(Table, CsvRow, TEXT("SourceSheet"), OutSourceSheet, Issues);
	ReEchoCsv::RequireInt(Table, CsvRow, TEXT("SourceRow"), OutSourceRow, Issues);
	ReEchoCsv::ReadOptionalCell(CsvRow, TEXT("Notes"), OutNotes);
}

bool ReadStages(const FString& DataDirectory,
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
	                            TEXT("StageIndex"),
	                            TEXT("SceneId"),
	                            TEXT("FirstEncounterIndex"),
	                            TEXT("LastEncounterIndex"),
	                            TEXT("PreserveEnemiesBetweenEncounters"),
	                            TEXT("ClearEnemiesOnEnter"),
	                            TEXT("Enabled"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow"),
	                            TEXT("Notes")},
	                           Issues);

	TSet<FName> SeenIds;
	TSet<int32> SeenIndexes;
	for (const ReEchoCsv::FRow& CsvRow : Table.Rows)
	{
		FReEchoCsvStageRow Row;
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("Id"), Row.Id, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("StageIndex"), Row.StageIndex, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("SceneId"), Row.SceneId, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("FirstEncounterIndex"), Row.FirstEncounterIndex, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("LastEncounterIndex"), Row.LastEncounterIndex, Issues);
		ReEchoCsv::RequireBool(
		    Table, CsvRow, TEXT("PreserveEnemiesBetweenEncounters"), Row.bPreserveEnemiesBetweenEncounters, Issues);
		ReEchoCsv::RequireBool(Table, CsvRow, TEXT("ClearEnemiesOnEnter"), Row.bClearEnemiesOnEnter, Issues);
		ReEchoCsv::RequireBool(Table, CsvRow, TEXT("Enabled"), Row.bEnabled, Issues);
		ReadSource(Table, CsvRow, Row.SourceSheet, Row.SourceRow, Row.Notes, Issues);

		if (Row.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(Row.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, CsvRow.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (Row.StageIndex <= 0 || SeenIndexes.Contains(Row.StageIndex))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("StageIndex"), TEXT("Stage index must be unique and positive"));
		}
		if (Row.FirstEncounterIndex <= 0 || Row.LastEncounterIndex < Row.FirstEncounterIndex)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("FirstEncounterIndex"), TEXT("Invalid encounter range"));
		}
		SeenIds.Add(Row.Id);
		SeenIndexes.Add(Row.StageIndex);
		Snapshot.StageOrder.Add(Row.Id);
		Snapshot.Stages.Add(Row.Id, MoveTemp(Row));
	}
	return Issues.Num() == 0;
}

bool ReadEncounters(const FString& DataDirectory,
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
	                            TEXT("EncounterIndex"),
	                            TEXT("StageId"),
	                            TEXT("DurationSeconds"),
	                            TEXT("EndCondition"),
	                            TEXT("EchoAnchorRatio"),
	                            TEXT("PlayerAnchorRatio"),
	                            TEXT("MeleeTargetingPolicy"),
	                            TEXT("RangedBurstLimit"),
	                            TEXT("RangedBurstWindowSeconds"),
	                            TEXT("EliteSkillConcurrency"),
	                            TEXT("ActiveUnitLimit"),
	                            TEXT("BossCountsTowardUnitLimit"),
	                            TEXT("ReplayPolicy"),
	                            TEXT("Enabled"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow"),
	                            TEXT("Notes")},
	                           Issues);

	TSet<FName> SeenIds;
	TSet<int32> SeenIndexes;
	for (const ReEchoCsv::FRow& CsvRow : Table.Rows)
	{
		FReEchoCsvEncounterRow Row;
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("Id"), Row.Id, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("EncounterIndex"), Row.EncounterIndex, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("StageId"), Row.StageId, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("DurationSeconds"), 0.0f, 3600.0f, Row.DurationSeconds, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("EndCondition"), Row.EndCondition, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("EchoAnchorRatio"), 0.0f, 1.0f, Row.EchoAnchorRatio, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("PlayerAnchorRatio"), 0.0f, 1.0f, Row.PlayerAnchorRatio, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("MeleeTargetingPolicy"), Row.MeleeTargetingPolicy, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("RangedBurstLimit"), Row.RangedBurstLimit, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("RangedBurstWindowSeconds"), 0.0f, 3600.0f, Row.RangedBurstWindowSeconds, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("EliteSkillConcurrency"), Row.EliteSkillConcurrency, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("ActiveUnitLimit"), Row.ActiveUnitLimit, Issues);
		ReEchoCsv::RequireBool(
		    Table, CsvRow, TEXT("BossCountsTowardUnitLimit"), Row.bBossCountsTowardUnitLimit, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("ReplayPolicy"), Row.ReplayPolicy, Issues);
		ReEchoCsv::RequireBool(Table, CsvRow, TEXT("Enabled"), Row.bEnabled, Issues);
		ReadSource(Table, CsvRow, Row.SourceSheet, Row.SourceRow, Row.Notes, Issues);

		if (Row.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(Row.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, CsvRow.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (!Snapshot.Stages.Contains(Row.StageId))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, CsvRow.Line, TEXT("StageId"), TEXT("Unknown Stages.Id reference"));
		}
		if (Row.EncounterIndex <= 0 || SeenIndexes.Contains(Row.EncounterIndex))
		{
			ReEchoCsv::AddIssue(Issues,
			                    Table.File,
			                    CsvRow.Line,
			                    TEXT("EncounterIndex"),
			                    TEXT("Encounter index must be unique and positive"));
		}
		if (!FMath::IsNearlyEqual(Row.EchoAnchorRatio + Row.PlayerAnchorRatio, 1.0f, KINDA_SMALL_NUMBER))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("EchoAnchorRatio"), TEXT("Anchor ratios must sum to one"));
		}
		SeenIds.Add(Row.Id);
		SeenIndexes.Add(Row.EncounterIndex);
		Snapshot.EncounterOrder.Add(Row.Id);
		Snapshot.Encounters.Add(Row.Id, MoveTemp(Row));
	}
	return Issues.Num() == 0;
}

bool ReadEncounterWaves(const FString& DataDirectory,
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
	                            TEXT("EncounterId"),
	                            TEXT("WaveIndex"),
	                            TEXT("TriggerSeconds"),
	                            TEXT("MeleeCount"),
	                            TEXT("RangedCount"),
	                            TEXT("EliteCount"),
	                            TEXT("BossEnemyId"),
	                            TEXT("Enabled"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow"),
	                            TEXT("Notes")},
	                           Issues);

	TSet<FName> SeenIds;
	TSet<FString> SeenOrders;
	for (const ReEchoCsv::FRow& CsvRow : Table.Rows)
	{
		FReEchoCsvEncounterWaveRow Row;
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("Id"), Row.Id, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("EncounterId"), Row.EncounterId, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("WaveIndex"), Row.WaveIndex, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("TriggerSeconds"), 0.0f, 3600.0f, Row.TriggerSeconds, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("MeleeCount"), Row.MeleeCount, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("RangedCount"), Row.RangedCount, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("EliteCount"), Row.EliteCount, Issues);
		FString BossEnemyId;
		ReEchoCsv::ReadOptionalCell(CsvRow, TEXT("BossEnemyId"), BossEnemyId);
		Row.BossEnemyId = FName(*BossEnemyId);
		ReEchoCsv::RequireBool(Table, CsvRow, TEXT("Enabled"), Row.bEnabled, Issues);
		ReadSource(Table, CsvRow, Row.SourceSheet, Row.SourceRow, Row.Notes, Issues);

		if (Row.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(Row.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, CsvRow.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (!Snapshot.Encounters.Contains(Row.EncounterId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("EncounterId"), TEXT("Unknown Encounters.Id reference"));
		}
		if (!Row.BossEnemyId.IsNone() && !Snapshot.Enemies.Contains(Row.BossEnemyId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("BossEnemyId"), TEXT("Unknown Enemies.Id reference"));
		}
		if (Row.WaveIndex <= 0 || Row.MeleeCount < 0 || Row.RangedCount < 0 || Row.EliteCount < 0)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("WaveIndex"), TEXT("Wave index and counts must be non-negative"));
		}
		const FString OrderKey = FString::Printf(TEXT("%s:%d"), *Row.EncounterId.ToString(), Row.WaveIndex);
		if (SeenOrders.Contains(OrderKey))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("WaveIndex"), TEXT("Duplicate encounter wave index"));
		}
		SeenIds.Add(Row.Id);
		SeenOrders.Add(OrderKey);
		Snapshot.EncounterWaveOrder.Add(Row.Id);
		Snapshot.EncounterWaves.Add(Row.Id, MoveTemp(Row));
	}
	return Issues.Num() == 0;
}

bool ReadSpawnProfiles(const FString& DataDirectory,
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
	                            TEXT("EnemyRole"),
	                            TEXT("EnemyId"),
	                            TEXT("MinAnchorDistanceCm"),
	                            TEXT("MaxAnchorDistanceCm"),
	                            TEXT("MinSpacingCm"),
	                            TEXT("WarningLeadSeconds"),
	                            TEXT("DistributionPolicy"),
	                            TEXT("SpacingPolicy"),
	                            TEXT("Enabled"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow"),
	                            TEXT("Notes")},
	                           Issues);

	TSet<FName> SeenIds;
	TSet<FName> SeenRoles;
	for (const ReEchoCsv::FRow& CsvRow : Table.Rows)
	{
		FReEchoCsvSpawnProfileRow Row;
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("Id"), Row.Id, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("EnemyRole"), Row.EnemyRole, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("EnemyId"), Row.EnemyId, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("MinAnchorDistanceCm"), 0.0f, 100000.0f, Row.MinAnchorDistanceCm, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("MaxAnchorDistanceCm"), 0.0f, 100000.0f, Row.MaxAnchorDistanceCm, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("MinSpacingCm"), 0.0f, 100000.0f, Row.MinSpacingCm, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("WarningLeadSeconds"), 0.0f, 3600.0f, Row.WarningLeadSeconds, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("DistributionPolicy"), Row.DistributionPolicy, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("SpacingPolicy"), Row.SpacingPolicy, Issues);
		ReEchoCsv::RequireBool(Table, CsvRow, TEXT("Enabled"), Row.bEnabled, Issues);
		ReadSource(Table, CsvRow, Row.SourceSheet, Row.SourceRow, Row.Notes, Issues);

		if (Row.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(Row.Id) || SeenRoles.Contains(Row.EnemyRole))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("Id"), TEXT("Duplicate spawn profile id or role"));
		}
		if (!Snapshot.Enemies.Contains(Row.EnemyId))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, CsvRow.Line, TEXT("EnemyId"), TEXT("Unknown Enemies.Id reference"));
		}
		if (Row.MinAnchorDistanceCm > Row.MaxAnchorDistanceCm)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("MinAnchorDistanceCm"), TEXT("Cannot exceed maximum"));
		}
		SeenIds.Add(Row.Id);
		SeenRoles.Add(Row.EnemyRole);
		Snapshot.SpawnProfileOrder.Add(Row.Id);
		Snapshot.SpawnProfiles.Add(Row.Id, MoveTemp(Row));
	}
	return Issues.Num() == 0;
}

bool ReadSpawnPolicy(const FString& DataDirectory,
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
	                            TEXT("AnchorLeadSeconds"),
	                            TEXT("MinPlayerDistanceCm"),
	                            TEXT("MinEchoDistanceCm"),
	                            TEXT("BoundaryPolicy"),
	                            TEXT("CandidatePolicy"),
	                            TEXT("PlayerPredictionPolicy"),
	                            TEXT("MultiEchoPolicy"),
	                            TEXT("MaxCandidateAttempts"),
	                            TEXT("FullMapRandom"),
	                            TEXT("Enabled"),
	                            TEXT("SourceSheet"),
	                            TEXT("SourceRow"),
	                            TEXT("Notes")},
	                           Issues);

	TSet<FName> SeenIds;
	for (const ReEchoCsv::FRow& CsvRow : Table.Rows)
	{
		FReEchoCsvSpawnPolicyRow Row;
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("Id"), Row.Id, Issues);
		ReEchoCsv::RequireFloat(Table, CsvRow, TEXT("AnchorLeadSeconds"), 0.0f, 3600.0f, Row.AnchorLeadSeconds, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("MinPlayerDistanceCm"), 0.0f, 100000.0f, Row.MinPlayerDistanceCm, Issues);
		ReEchoCsv::RequireFloat(
		    Table, CsvRow, TEXT("MinEchoDistanceCm"), 0.0f, 100000.0f, Row.MinEchoDistanceCm, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("BoundaryPolicy"), Row.BoundaryPolicy, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("CandidatePolicy"), Row.CandidatePolicy, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("PlayerPredictionPolicy"), Row.PlayerPredictionPolicy, Issues);
		ReEchoCsv::RequireStableId(Table, CsvRow, TEXT("MultiEchoPolicy"), Row.MultiEchoPolicy, Issues);
		ReEchoCsv::RequireInt(Table, CsvRow, TEXT("MaxCandidateAttempts"), Row.MaxCandidateAttempts, Issues);
		ReEchoCsv::RequireBool(Table, CsvRow, TEXT("FullMapRandom"), Row.bFullMapRandom, Issues);
		ReEchoCsv::RequireBool(Table, CsvRow, TEXT("Enabled"), Row.bEnabled, Issues);
		ReadSource(Table, CsvRow, Row.SourceSheet, Row.SourceRow, Row.Notes, Issues);

		if (Row.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(Row.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, CsvRow.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (Row.MaxCandidateAttempts <= 0)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, CsvRow.Line, TEXT("MaxCandidateAttempts"), TEXT("Must be positive"));
		}
		SeenIds.Add(Row.Id);
		Snapshot.SpawnPolicies.Add(Row.Id, MoveTemp(Row));
	}
	return Issues.Num() == 0;
}
}

bool ReadTables(const FString& DataDirectory,
                const TMap<FString, ReEchoCsv::FManifestEntry>& ManifestEntries,
                FReEchoCsvDataSnapshot& Snapshot,
                TArray<FReEchoCsvIssue>& Issues)
{
	if (!ReadStages(DataDirectory, ManifestEntries[StagesTableId], Snapshot, Issues))
	{
		return false;
	}
	if (!ReadEncounters(DataDirectory, ManifestEntries[EncountersTableId], Snapshot, Issues))
	{
		return false;
	}
	if (!ReadEncounterWaves(DataDirectory, ManifestEntries[EncounterWavesTableId], Snapshot, Issues))
	{
		return false;
	}
	if (!ReadSpawnProfiles(DataDirectory, ManifestEntries[SpawnProfilesTableId], Snapshot, Issues))
	{
		return false;
	}
	return ReadSpawnPolicy(DataDirectory, ManifestEntries[SpawnPolicyTableId], Snapshot, Issues);
}
}
