#pragma once

#include "ReEchoCsvDataReader.h"

namespace ReEchoElementReactionCsv
{
bool ReadTables(const FString& DataDirectory,
                const TMap<FString, ReEchoCsv::FManifestEntry>& ManifestEntries,
                FReEchoCsvDataSnapshot& Snapshot,
                TArray<FReEchoCsvIssue>& Issues);
}
