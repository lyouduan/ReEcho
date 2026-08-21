#pragma once

#include "Data/ReEchoCsvDataRegistry.h"
#include "ReEchoCsvDataReader.h"

namespace ReEchoEnemyCsv
{
bool ReadTables(const FString& DataDirectory,
                const TMap<FString, ReEchoCsv::FManifestEntry>& ManifestEntries,
                FReEchoCsvDataSnapshot& Snapshot,
                TArray<FReEchoCsvIssue>& Issues);

bool ReadEnemyCombatStats(const FString& DataDirectory,
                          const ReEchoCsv::FManifestEntry& Entry,
                          FReEchoCsvDataSnapshot& Snapshot,
                          TArray<FReEchoCsvIssue>& Issues);
}
