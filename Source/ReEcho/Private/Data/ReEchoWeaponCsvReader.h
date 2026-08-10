#pragma once

#include "ReEchoCsvDataReader.h"

namespace ReEchoWeaponCsv
{
bool ReadTables(const FString& DataDirectory,
                const TMap<FString, ReEchoCsv::FManifestEntry>& ManifestEntries,
                FReEchoCsvDataSnapshot& Snapshot,
                TArray<FReEchoCsvIssue>& Issues);
}
