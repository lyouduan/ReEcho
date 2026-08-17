#pragma once

#include "Data/ReEchoCsvDataRegistry.h"
#include "ReEchoCsvDataReader.h"

namespace ReEchoEncounterCsv
{
bool ReadTables(const FString& DataDirectory,
                const TMap<FString, ReEchoCsv::FManifestEntry>& ManifestEntries,
                FReEchoCsvDataSnapshot& Snapshot,
                TArray<FReEchoCsvIssue>& Issues);
}
