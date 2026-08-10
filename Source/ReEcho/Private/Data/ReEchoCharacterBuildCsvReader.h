#pragma once

#include "ReEchoCsvDataReader.h"

namespace ReEchoCharacterBuildCsv
{
bool ReadTables(const FString& DataDirectory,
                const TMap<FString, ReEchoCsv::FManifestEntry>& ManifestEntries,
                FReEchoCsvDataSnapshot& Snapshot,
                TArray<FReEchoCsvIssue>& Issues);
}
